// PathService: submit / drain / stop / quiesce, with the production bodies.
//
// Step C moves a movement search onto a worker thread. The thing that can go wrong in
// step C is not arithmetic, it is plumbing: a result handed to the wrong request, a
// route freed twice or never, a CPathMgr destroyed on the wrong thread, a join that
// never returns, a save that serializes while a search is in flight. So the scaffold
// below supplies a small deterministic snapshot search - the real A* has no place in a
// unit test, and its snapshot-vs-live equivalence is a different fixture's gate - and
// everything ABOVE that search is the production text, extracted verbatim from
// pathservice.h / pathservice.cpp by run-pathservice.py.
//
// Test 4 is separate and is about the request contract: with no CVehicle to hand a
// worker, the request must carry the integer that makes theTransports.GetData() return
// the very same CTransportData* the vehicle would have given. The two production
// accessors are compiled here and the round trip is asserted, rather than read.

#include <windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

static int g_checks   = 0;
static int g_failures = 0;

static void CHECK( bool cond, const char* what )
{
    ++g_checks;
    if ( !cond )
    {
        ++g_failures;
        std::printf( "FAIL: %s\n", what );
    }
}

// ------------------------------------------------------------------ game types

typedef int BOOL;
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

// Route arrays are counted, so "every queued path freed" and "no leak across 200
// start/stop cycles" are measurements rather than hopes. A class-level array
// operator new[]/delete[] catches the production `new CHexCoord[n]` and `delete[]`
// without changing a character of either.
static std::atomic<long> g_arrayNew( 0 );
static std::atomic<long> g_arrayDelete( 0 );

class CHexCoord
{
  public:
    CHexCoord( ): m_x( 0 ), m_y( 0 ) {}
    CHexCoord( int x, int y ): m_x( x ), m_y( y ) {}
    int  X( ) const { return ( m_x ); }
    int  Y( ) const { return ( m_y ); }
    bool operator==( CHexCoord const& o ) const { return ( m_x == o.m_x && m_y == o.m_y ); }
    bool operator!=( CHexCoord const& o ) const { return ( !( *this == o ) ); }

    static void* operator new[]( size_t cb )
    {
        ++g_arrayNew;
        return ( ::operator new( cb ) );
    }
    static void operator delete[]( void* p )
    {
        if ( p != NULL )
            ++g_arrayDelete;
        ::operator delete( p );
    }

  private:
    int m_x;
    int m_y;
};

// ------------------------------------------------------------- the snapshot
//
// PathWorld::Hex is the production record (extracted by the driver). The container
// around it is the fixture's: a square of hexes with a deterministic obstacle pattern,
// published by shared_ptr exactly as the game publishes one.

typedef unsigned char BYTE;

namespace PwHexOnly
{
#include "ps_shared.inc"   // struct Hex; struct SNAPSEARCH;
}

class PathWorld
{
  public:
    enum
    {
        side = 64
    };

    PathWorld( uint64_t uEpoch ): m_uEpoch( uEpoch )
    {
        m_aHex.resize( (size_t)side * side );
        for ( int y = 0; y < side; ++y )
            for ( int x = 0; x < side; ++x )
            {
                PwHexOnly::Hex& h = m_aHex[(size_t)y * side + x];
                h.bUnits          = 0;
                h.bAlt            = (BYTE)( ( x * 3 + y * 7 ) & 0x0F );
                h.bOcc            = 0;
                // A sparse, deterministic obstacle field: enough that routes differ by
                // endpoint and by vehicle type, never so much that nothing gets through.
                h.bType = (BYTE)( ( ( x * 13 + y * 29 ) % 23 == 0 ) ? 1 : 0 );
            }
    }

    PwHexOnly::Hex const& At( int x, int y ) const
    {
        return ( m_aHex[(size_t)( y & ( side - 1 ) ) * side + (size_t)( x & ( side - 1 ) )] );
    }
    BOOL     Blocked( int x, int y ) const { return ( At( x, y ).bType ? TRUE : FALSE ); }
    uint64_t Epoch( ) const { return ( m_uEpoch ); }

  private:
    std::vector<PwHexOnly::Hex> m_aHex;
    uint64_t                    m_uEpoch;
};

typedef PwHexOnly::SNAPSEARCH SNAPSEARCH_T;

// ------------------------------------------------------------------ Perf stub

namespace Perf
{
static std::mutex                       g_mtx;
static std::map<std::string, long long> g_counters;

inline void Bump( const char* name, long long by )
{
    std::lock_guard<std::mutex> lk( g_mtx );
    g_counters[name] += by;
}
inline long long Get( const char* name )
{
    std::lock_guard<std::mutex> lk( g_mtx );
    return ( g_counters[name] );
}
inline void Reset( )
{
    std::lock_guard<std::mutex> lk( g_mtx );
    g_counters.clear( );
}
inline void     CounterInc( const char* name, int64_t by = 1 ) { Bump( name, by ); }
inline void     GaugeSet( const char*, int64_t ) {}
inline uint64_t NowIfEnabled( ) { return ( 1 ); }
inline void     CounterAddElapsedUs( const char* name, uint64_t ) { Bump( name, 0 ); }
}  // namespace Perf

// --------------------------------------------------------------- CPathMgr stub
//
// Counts its own construction and destruction, and records the thread each happened
// on: "a worker owns its CPathMgr and destroys it on its own thread before join
// returns" is the rule the per-AI CPathMap precedent broke.

static std::atomic<long> g_mgrCtor( 0 );
static std::atomic<long> g_mgrDtor( 0 );
static std::atomic<long> g_mgrWrongThread( 0 );
static std::atomic<int>  g_searchDelayMs( 0 );
static std::atomic<long> g_searchesRunning( 0 );

static std::thread::id g_mainThreadId;

class CPathMgr
{
  public:
    typedef SNAPSEARCH_T SNAPSEARCH;

    CPathMgr( ): m_bInit( FALSE ), m_idCtor( std::this_thread::get_id( ) ) { ++g_mgrCtor; }
    ~CPathMgr( )
    {
        if ( m_idCtor != std::this_thread::get_id( ) )
            ++g_mgrWrongThread;
        ++g_mgrDtor;
    }
    CPathMgr( CPathMgr const& )            = delete;
    CPathMgr& operator=( CPathMgr const& ) = delete;

    void MarkShadow( void ) { m_bShadow = TRUE; }
    BOOL Init( int iMapEX, int iMapEY )
    {
        m_iMapEX = iMapEX;
        m_iMapEY = iMapEY;
        m_bInit  = TRUE;
        return ( TRUE );
    }

    // A deterministic walk over the snapshot. Not the A*: the point of this fixture is
    // that whatever the search answers arrives intact at the right request.
    void SearchSnapshot( PathWorld const& pw, CHexCoord hexFrom, CHexCoord hexTo, int iVehType, BOOL bVehBlock,
                         BOOL bDirectPath, SNAPSEARCH& out );

  private:
    BOOL            m_bInit;
    BOOL            m_bShadow = FALSE;
    int             m_iMapEX  = 0;
    int             m_iMapEY  = 0;
    std::thread::id m_idCtor;
};

void CPathMgr::SearchSnapshot( PathWorld const& pw, CHexCoord hexFrom, CHexCoord hexTo, int iVehType,
                               BOOL bVehBlock, BOOL bDirectPath, SNAPSEARCH& out )
{
    (void)bVehBlock;
    (void)bDirectPath;
    out = SNAPSEARCH( );
    if ( !m_bInit )
        return;

    ++g_searchesRunning;
    const int iDelay = g_searchDelayMs.load( );
    if ( iDelay > 0 )
        std::this_thread::sleep_for( std::chrono::milliseconds( iDelay ) );

    if ( hexFrom == hexTo )
    {
        --g_searchesRunning;
        return;   // trivial: NULL route, length 0
    }

    CHexCoord aStep[512];
    int       iLen = 0;
    int       x    = hexFrom.X( );
    int       y    = hexFrom.Y( );
    // The vehicle type picks the axis order, so a request routed with the wrong type
    // produces a different route instead of the same one.
    const bool bXFirst = ( ( iVehType & 1 ) == 0 );

    for ( int iGuard = 0; iGuard < 500 && iLen < 500; ++iGuard )
    {
        if ( x == hexTo.X( ) && y == hexTo.Y( ) )
            break;
        const bool bMoveX = bXFirst ? ( x != hexTo.X( ) ) : ( y == hexTo.Y( ) );
        if ( bMoveX )
            x += ( hexTo.X( ) > x ) ? 1 : -1;
        else
            y += ( hexTo.Y( ) > y ) ? 1 : -1;

        if ( pw.Blocked( x, y ) )
            break;   // clamped at the last reachable hex
        aStep[iLen++] = CHexCoord( x, y );
    }

#ifdef PS_PERTURB_RESULT
    // Failing direction for test 1: a worker (and only a worker) answers slightly
    // differently. If the test still passes, it is not comparing what it claims to.
    if ( std::this_thread::get_id( ) != g_mainThreadId && iLen > 0 )
        aStep[iLen - 1] = CHexCoord( aStep[iLen - 1].X( ) + 1, aStep[iLen - 1].Y( ) );
#endif

    if ( iLen > 0 )
    {
        out.phexPath = new CHexCoord[iLen];
        for ( int i = 0; i < iLen; ++i )
            out.phexPath[i] = aStep[i];
        out.iPathLen = iLen;
        out.iClass   = ( x == hexTo.X( ) && y == hexTo.Y( ) ) ? 2 : 3;
    }
    else
    {
        out.iClass = 4;
    }
    out.bCapArena = FALSE;
    out.bCapIter  = ( iLen >= 500 ) ? TRUE : FALSE;
    (void)pw;
    --g_searchesRunning;
}

// ------------------------------------------------------- the production text

#include "ps_decls.inc"    // struct PathRequest; struct PathResult; class PathService;

static int s_iWorkerOn = -1;   // resolved once by Enabled(), as in production

#include "ps_bodies.inc"   // every PathService method body, verbatim

// -------------------------------------------------------------------- helpers

static std::shared_ptr<const PathWorld> MakeWorld( uint64_t uEpoch )
{
    return ( std::shared_ptr<const PathWorld>( new PathWorld( uEpoch ) ) );
}

static PathRequest MakeRequest( std::shared_ptr<const PathWorld> const& pw, int i )
{
    PathRequest req;
    req.gameGeneration  = 7;
    req.vehicleId       = (uint32_t)( 1000 + i );
    req.orderGeneration = (uint32_t)( i % 5 );
    req.from            = CHexCoord( i % 60, ( i * 7 ) % 60 );
    req.to              = CHexCoord( ( i * 11 ) % 60, ( i * 3 ) % 60 );
    req.iVehType        = i % 4;
    req.bVehBlock       = FALSE;
    req.bDirectPath     = FALSE;
    req.tSubmit         = 0;
    req.world           = pw;
    return ( req );
}

// The main-thread reference for one request: the same search, on the same snapshot.
static void Reference( PathRequest const& req, SNAPSEARCH_T& out )
{
    static CPathMgr* s_pRef = NULL;
    if ( s_pRef == NULL )
    {
        s_pRef = new CPathMgr( );
        s_pRef->Init( PathWorld::side, PathWorld::side );
    }
    s_pRef->SearchSnapshot( *req.world, req.from, req.to, req.iVehType, req.bVehBlock, req.bDirectPath, out );
}

static bool SameAnswer( SNAPSEARCH_T const& ref, PathResult const& res )
{
    if ( ( ref.phexPath == NULL ) != ( res.path == NULL ) )
        return ( false );
    if ( ref.iPathLen != res.pathLen )
        return ( false );
    if ( ref.iClass != res.exitClass )
        return ( false );
    if ( ref.bCapArena != res.bCapArena || ref.bCapIter != res.bCapIter )
        return ( false );
    for ( int i = 0; i < ref.iPathLen; ++i )
        if ( ref.phexPath[i] != res.path[i] )
            return ( false );
    return ( true );
}

// ------------------------------------------------------------------- the tests

static void Test1_EqualsReference( int nWorkers )
{
    char szWhat[128];
    Perf::Reset( );

    PathService svc;
    CHECK( svc.Start( PathWorld::side, PathWorld::side, nWorkers ) == TRUE, "Start succeeds" );
    CHECK( svc.IsRunning( ) == TRUE, "IsRunning after Start" );

    std::shared_ptr<const PathWorld> pw = MakeWorld( 42 );

    const int                        kN = 1000;
    std::map<uint64_t, PathRequest>  mapSent;
    for ( int i = 0; i < kN; ++i )
    {
        PathRequest req = MakeRequest( pw, i );
        uint64_t    uId = 0;
        // The first submits can land before a worker has finished its own Init, which
        // is the one case Submit legitimately refuses; retry rather than call it a bug.
        for ( int iTry = 0; iTry < 2000 && uId == 0; ++iTry )
        {
            uId = svc.Submit( req );
            if ( uId == 0 )
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        CHECK( uId != 0, "Submit assigns a requestId" );
        req.requestId = uId;
        mapSent[uId]  = req;
    }

    sprintf( szWhat, "%d workers: pq.submit counted %d", nWorkers, kN );
    CHECK( Perf::Get( "pq.submit" ) == kN, szWhat );

    int        nGot = 0;
    PathResult res;
    for ( int iSpin = 0; iSpin < 60000 && nGot < kN; ++iSpin )
    {
        while ( svc.PopResult( res ) )
        {
            std::map<uint64_t, PathRequest>::iterator it = mapSent.find( res.requestId );
            CHECK( it != mapSent.end( ), "result carries a requestId we submitted" );
            if ( it != mapSent.end( ) )
            {
                PathRequest const& req = it->second;
                CHECK( res.vehicleId == req.vehicleId, "result carries its own vehicleId" );
                CHECK( res.orderGeneration == req.orderGeneration, "result carries its own orderGeneration" );
                CHECK( res.gameGeneration == req.gameGeneration, "result carries its own gameGeneration" );
                CHECK( res.worldEpoch == req.world->Epoch( ), "result carries the snapshot's epoch" );

                SNAPSEARCH_T ref;
                Reference( req, ref );
                CHECK( SameAnswer( ref, res ), "worker answer equals the same-snapshot reference" );
                CHECK( res.success == ( res.path != NULL ? TRUE : FALSE ), "success agrees with the route" );
                delete[] ref.phexPath;
                mapSent.erase( it );
            }
            PathService::FreeResult( res );
            ++nGot;
        }
        if ( nGot < kN )
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }

    sprintf( szWhat, "%d workers: every one of %d requests came back", nWorkers, kN );
    CHECK( nGot == kN, szWhat );
    CHECK( mapSent.empty( ), "no request left unanswered" );
    CHECK( Perf::Get( "pq.done" ) == kN, "pq.done counted every result" );
    CHECK( Perf::Get( "pq.worker.calls" ) == kN, "pq.worker.calls counted every search" );
    CHECK( svc.QueueDepth( ) == 0, "queue empty once everything came back" );

    svc.Stop( );
    CHECK( svc.IsRunning( ) == FALSE, "IsRunning is FALSE after Stop" );
}

static void Test2_StopUnderLoad( )
{
    Perf::Reset( );
    const long ctor0 = g_mgrCtor.load( );
    const long dtor0 = g_mgrDtor.load( );
    const long new0  = g_arrayNew.load( );
    const long del0  = g_arrayDelete.load( );

    PathService svc;
    g_searchDelayMs.store( 2 );   // so a search is certainly in flight when Stop lands
    CHECK( svc.Start( PathWorld::side, PathWorld::side, 2 ) == TRUE, "Start (stop-under-load)" );

    std::shared_ptr<const PathWorld> pw = MakeWorld( 99 );
    int nSubmitted = 0;
    for ( int i = 0; i < 1000; ++i )
        if ( svc.Submit( MakeRequest( pw, i ) ) != 0 )
            ++nSubmitted;

    // Let some finish - their results pile up unpopped, which is what Stop has to free -
    // while the rest are still queued and one is inside a search.
    std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
    CHECK( nSubmitted > 0, "requests were accepted" );

    const DWORD dwT0 = GetTickCount( );
    svc.Stop( );                       // must JOIN, not abandon
    const DWORD dwJoinMs = GetTickCount( ) - dwT0;
    g_searchDelayMs.store( 0 );

    CHECK( dwJoinMs < 10000, "Stop joined rather than hung" );
    CHECK( svc.QueueDepth( ) == 0, "Stop dropped the queued requests" );
    PathResult res;
    CHECK( svc.PopResult( res ) == FALSE, "Stop freed the results nobody popped" );
    CHECK( g_searchesRunning.load( ) == 0, "no search still running after join" );

    CHECK( g_mgrCtor.load( ) - ctor0 == g_mgrDtor.load( ) - dtor0, "every worker CPathMgr was destroyed" );
    CHECK( g_mgrWrongThread.load( ) == 0, "every CPathMgr was destroyed on the thread that made it" );
    CHECK( g_arrayNew.load( ) - new0 == g_arrayDelete.load( ) - del0, "every route allocated was freed" );

    // The snapshot the requests pinned is released, so the fixture is the last owner.
    CHECK( pw.use_count( ) == 1, "Stop released every snapshot the queue pinned" );
}

static void Test3_StartStopCycles( )
{
    Perf::Reset( );
    const long ctor0 = g_mgrCtor.load( );
    const long dtor0 = g_mgrDtor.load( );
    const long new0  = g_arrayNew.load( );
    const long del0  = g_arrayDelete.load( );

    std::shared_ptr<const PathWorld> pw = MakeWorld( 5 );

    for ( int iCycle = 0; iCycle < 200; ++iCycle )
    {
        PathService svc;
        if ( svc.Start( PathWorld::side, PathWorld::side, ( iCycle % 2 ) ? 2 : 1 ) != TRUE )
        {
            CHECK( false, "Start failed inside the cycle loop" );
            break;
        }
        for ( int i = 0; i < 8; ++i )
            svc.Submit( MakeRequest( pw, iCycle * 8 + i ) );

        // Half the cycles pop what came back, half leave it for Stop to free: both are
        // real teardown orders and both must balance.
        if ( iCycle & 1 )
        {
            PathResult res;
            while ( svc.PopResult( res ) )
                PathService::FreeResult( res );
        }
        svc.Stop( );
    }

    CHECK( g_mgrCtor.load( ) - ctor0 == g_mgrDtor.load( ) - dtor0, "200 cycles: every CPathMgr destroyed" );
    CHECK( g_mgrWrongThread.load( ) == 0, "200 cycles: none destroyed on a foreign thread" );
    CHECK( g_arrayNew.load( ) - new0 == g_arrayDelete.load( ) - del0, "200 cycles: every route freed" );
    CHECK( pw.use_count( ) == 1, "200 cycles: every snapshot released" );
}

static void Test3b_Quiesce( )
{
    Perf::Reset( );
    PathService svc;
    CHECK( svc.Start( PathWorld::side, PathWorld::side, 2 ) == TRUE, "Start (quiesce)" );

    std::shared_ptr<const PathWorld> pw = MakeWorld( 11 );
    g_searchDelayMs.store( 1 );
    for ( int i = 0; i < 200; ++i )
        svc.Submit( MakeRequest( pw, i ) );

    svc.Quiesce( );
    g_searchDelayMs.store( 0 );

    // The save seam's whole promise: nothing is in flight, and nothing new gets in.
    CHECK( svc.QueueDepth( ) == 0, "Quiesce leaves nothing queued and nothing in flight" );
    CHECK( g_searchesRunning.load( ) == 0, "Quiesce leaves no search running" );
    CHECK( svc.Submit( MakeRequest( pw, 999 ) ) == 0, "Submit refuses while quiesced" );

    svc.Resume( );
    CHECK( svc.Submit( MakeRequest( pw, 998 ) ) != 0, "Submit works again after Resume" );

    svc.Stop( );

    PathResult res;
    while ( svc.PopResult( res ) )
        PathService::FreeResult( res );
}

// ------------------------------------------------- test 4: the iVehType proof

class CTransportData
{
  public:
    int m_iType;
    int GetType( ) const { return ( m_iType ); }
};

class CTransport
{
  public:
    CTransportData const* GetData( int iIndex ) const;
    int                   GetIndex( CTransportData const* pData ) const;

    CTransportData* m_pData;
    int             m_iNumTransports;
};

#define ASSERT( x )              ( (void)0 )
#define ASSERT_STRICT_VALID( x ) ( (void)0 )

#include "ps_vehtype.inc"   // CTransport::GetData and CTransport::GetIndex, verbatim

static void Test4_VehTypeRoundTrip( )
{
    // 24 transports whose TRANS_TYPE deliberately does NOT equal the slot index: that is
    // what makes the choice of accessor load-bearing rather than incidental.
    const int      kN = 24;
    CTransportData aData[kN];
    for ( int i = 0; i < kN; ++i )
        aData[i].m_iType = ( i * 5 + 3 ) % kN;

    CTransport tr;
    tr.m_pData          = aData;
    tr.m_iNumTransports = kN;

    for ( int i = 0; i < kN; ++i )
    {
        CTransportData const* p = tr.GetData( i );
        CHECK( p == &aData[i], "GetData( i ) is the i'th record" );
        CHECK( tr.GetIndex( p ) == i, "GetIndex round-trips every transport index" );
        // The request builder uses GetIndex; this is what using GetType() instead
        // would have done.
        CHECK( tr.GetData( tr.GetIndex( p ) ) == p, "GetData( GetIndex( p ) ) == p, for every type" );
    }

    int nTypeWouldBeWrong = 0;
    for ( int i = 0; i < kN; ++i )
        if ( tr.GetData( aData[i].GetType( ) ) != &aData[i] )
            ++nTypeWouldBeWrong;
    CHECK( nTypeWouldBeWrong > 0, "GetType() is NOT interchangeable with the index" );
}

// -------------------------------------------------------------------- driver

int main( )
{
    g_mainThreadId = std::this_thread::get_id( );

    // Enabled() is resolved once from the environment, exactly as the game resolves it.
    _putenv( "EN_PATH_WORKER=1" );
    CHECK( PathService::Enabled( ) == TRUE, "EN_PATH_WORKER=1 enables the service" );
    s_iWorkerOn = -1;
    _putenv( "EN_PATH_WORKER=0" );
    CHECK( PathService::Enabled( ) == FALSE, "EN_PATH_WORKER=0 leaves it OFF" );
    s_iWorkerOn = -1;
    _putenv( "EN_PATH_WORKER=" );
    CHECK( PathService::Enabled( ) == FALSE, "EN_PATH_WORKER unset leaves it OFF" );
    s_iWorkerOn = -1;
    _putenv( "EN_PATH_WORKER=on" );
    CHECK( PathService::Enabled( ) == TRUE, "EN_PATH_WORKER=on enables the service" );

    _putenv( "EN_PATH_WORKERS=" );
    CHECK( PathService::ConfiguredWorkers( ) == (int)PathService::default_workers, "default_workers by default" );
    CHECK( (int)PathService::default_workers == 2, "default_workers is 2" );
    CHECK( (int)PathService::default_workers <= (int)PathService::max_workers, "default_workers is within the clamp" );
    _putenv( "EN_PATH_WORKERS=3" );
    CHECK( PathService::ConfiguredWorkers( ) == 3, "EN_PATH_WORKERS is honoured" );
    _putenv( "EN_PATH_WORKERS=99" );
    CHECK( PathService::ConfiguredWorkers( ) == (int)PathService::max_workers, "EN_PATH_WORKERS is capped" );
    _putenv( "EN_PATH_WORKERS=" );

    Test1_EqualsReference( 1 );
    Test1_EqualsReference( 2 );
    Test2_StopUnderLoad( );
    Test3_StartStopCycles( );
    Test3b_Quiesce( );
    Test4_VehTypeRoundTrip( );

    std::printf( "%d checks, %d failures\n", g_checks, g_failures );
    return ( g_failures ? 1 : 0 );
}
