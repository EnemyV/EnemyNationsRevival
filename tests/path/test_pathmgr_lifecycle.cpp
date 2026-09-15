// CPathMgr construct / Init / search / Close / Init / search / destruct.
//
// The step-A branch changed one thing that runs with the shadow switch OFF and with
// EN_PATH_PROBES compiled out: m_cs is now owned by an explicit m_bCsInit flag instead
// of being inferred from "m_paCells != NULL". That inference was wrong in two ways -
// Close() leaves the section live while nulling the arena, so a later Init() re-created
// a live section without deleting it, and ~CPathMgr after a Close() deleted nothing at
// all. Neither is visible on one process-lifetime global; both matter the moment a
// second instance is created and destroyed per game.
//
// So this fixture runs the lifecycle and counts the Win32 section calls. The bodies of
// the ctors, Init, Close, ClearArray and the dtor are the PRODUCTION text, extracted
// verbatim from cpathmgr.cpp by run-pathmgr-lifecycle.py; everything above the include
// is the smallest scaffold those bodies need.

#include <windows.h>
#include <cstdio>
#include <cstring>

static int g_checks = 0;
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

// ---------------------------------------------------------------- CS tracking
//
// A tiny registry so "initialise a section that is already live", "delete one that
// is not", and "enter a dead one" are detectable rather than undefined behaviour.

static int g_csInitCalls   = 0;
static int g_csDeleteCalls = 0;
static int g_csEnterCalls  = 0;
static int g_csDoubleInit  = 0;
static int g_csBadDelete   = 0;
static int g_csDeadEnter   = 0;

static const int   MAX_TRACKED = 8;
static const void* g_csPtr[MAX_TRACKED];
static bool        g_csLive[MAX_TRACKED];
static int         g_csCount = 0;

static int CsSlot( const void* p )
{
    for ( int i = 0; i < g_csCount; ++i )
        if ( g_csPtr[i] == p )
            return i;
    if ( g_csCount >= MAX_TRACKED )
        return -1;
    g_csPtr[g_csCount]  = p;
    g_csLive[g_csCount] = false;
    return g_csCount++;
}

static int CsLiveCount( void )
{
    int n = 0;
    for ( int i = 0; i < g_csCount; ++i )
        if ( g_csLive[i] )
            ++n;
    return n;
}

static void TrackInit( const void* p )
{
    ++g_csInitCalls;
    int s = CsSlot( p );
    if ( s >= 0 )
    {
        if ( g_csLive[s] )
            ++g_csDoubleInit;   // re-Initialize over a live section: the old leak
        g_csLive[s] = true;
    }
}

static void TrackDelete( const void* p )
{
    ++g_csDeleteCalls;
    int s = CsSlot( p );
    if ( s >= 0 )
    {
        if ( !g_csLive[s] )
            ++g_csBadDelete;
        g_csLive[s] = false;
    }
}

// Returns false when the section is NOT live, so the fixture can REPORT that instead
// of dying inside Win32. Against a build whose lifetime is wrong, that is the
// difference between a readable failure list and an access violation.
static bool TrackEnter( const void* p )
{
    ++g_csEnterCalls;
    int s = CsSlot( p );
    if ( s >= 0 && !g_csLive[s] )
    {
        ++g_csDeadEnter;
        return false;
    }
    return true;
}

static bool CsIsLive( const void* p )
{
    int s = CsSlot( p );
    return ( s < 0 || g_csLive[s] );
}

#define InitializeCriticalSection( p ) \
    do { TrackInit( p ); ( InitializeCriticalSection )( p ); } while ( 0 )
#define DeleteCriticalSection( p ) \
    do { bool _live = CsIsLive( p ); TrackDelete( p ); if ( _live ) ( DeleteCriticalSection )( p ); } while ( 0 )
#define EnterCriticalSection( p ) \
    do { if ( TrackEnter( p ) ) ( EnterCriticalSection )( p ); } while ( 0 )
#define LeaveCriticalSection( p ) \
    do { if ( CsIsLive( p ) ) ( LeaveCriticalSection )( p ); } while ( 0 )

// ------------------------------------------------------- array allocation tracking
//
// Section 7 needs the one thing a post-state check cannot show: what happens when the
// `new CCell[...]` inside a re-Init THROWS. By then Init has already freed the old
// arena and torn the old section down, so whether the destructor then deletes the
// freed arena a second time is decided entirely by whether m_paCells was nulled in
// between. So the fixture replaces the global array operators:
//
//   * g_bFailNextArrayNew makes the next operator new[] throw std::bad_alloc and
//     clears itself, so exactly one allocation - the arena of the second Init - fails.
//   * every pointer handed to operator delete[] is recorded, and a second delete of a
//     pointer that has not been handed back out since is COUNTED AND SKIPPED instead of
//     being passed to the real free. Against a build that double-frees the arena that
//     is the difference between a reported failure and an access violation in the CRT.
//
// A block is keyed by address and its free count is reset when the allocator hands the
// same address out again, so ordinary heap reuse across sections 1-6 is not a
// double free.

#include <new>
#include <cstddef>
#include <cstdlib>

static bool        g_bFailNextArrayNew  = false;
static int         g_arrayNewCalls      = 0;
static int         g_arrayNewFailures   = 0;
static std::size_t g_lastFailedNewBytes = 0;
static int         g_doubleFrees        = 0;

static const int   MAX_BLOCKS = 64;
static const void* g_blockPtr[MAX_BLOCKS];
static int         g_blockFreed[MAX_BLOCKS];
static int         g_blockCount    = 0;
static int         g_blockOverflow = 0;

static int BlockSlot( const void* p )
{
    for ( int i = 0; i < g_blockCount; ++i )
        if ( g_blockPtr[i] == p )
            return i;
    if ( g_blockCount >= MAX_BLOCKS )
    {
        ++g_blockOverflow;
        return -1;
    }
    g_blockPtr[g_blockCount]   = p;
    g_blockFreed[g_blockCount] = 0;
    return g_blockCount++;
}

// How many times the block at this address has been freed since it was last handed
// out; -1 for an address this fixture never allocated. CCell has no destructor, so
// MSVC lays the array down with no cookie and the arena pointer IS the allocation
// pointer; the small-offset fallback keeps the query honest if that ever changes.
static int FreedCount( const void* p )
{
    for ( int i = 0; i < g_blockCount; ++i )
        if ( g_blockPtr[i] == p )
            return g_blockFreed[i];
    for ( int i = 0; i < g_blockCount; ++i )
    {
        std::ptrdiff_t off = (const char*)p - (const char*)g_blockPtr[i];
        if ( off > 0 && off <= 16 )
            return g_blockFreed[i];
    }
    return -1;
}

void* operator new[]( std::size_t n )
{
    ++g_arrayNewCalls;
    if ( g_bFailNextArrayNew )
    {
        g_bFailNextArrayNew  = false;   // one shot: only the injected allocation fails
        g_lastFailedNewBytes = n;
        ++g_arrayNewFailures;
        throw std::bad_alloc( );
    }
    void* p = std::malloc( n ? n : 1 );
    if ( p == NULL )
        throw std::bad_alloc( );
    int s = BlockSlot( p );
    if ( s >= 0 )
        g_blockFreed[s] = 0;   // handed out again: the earlier frees are history
    return p;
}

void operator delete[]( void* p ) noexcept
{
    if ( p == NULL )
        return;
    int s = BlockSlot( p );
    if ( s >= 0 )
    {
        if ( g_blockFreed[s] > 0 )
        {
            ++g_doubleFrees;
            ++g_blockFreed[s];
            // Report and DO NOT free: the point is a failure list, not an AV.
            std::printf( "DOUBLE FREE: delete[] %p already freed - real free skipped\n", p );
            return;
        }
        ++g_blockFreed[s];
    }
    std::free( p );
}

void operator delete[]( void* p, std::size_t ) noexcept
{
    ::operator delete[]( p );
}

// ---------------------------------------------------------------- scaffold

#define PATH_TIMING 0
#define MAX_PATH_RANGE 80

#ifndef min
#define min( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

class CHexCoord
{
    int m_iX;
    int m_iY;

public:
    CHexCoord( ) : m_iX( 0 ), m_iY( 0 ) { }
    CHexCoord( int x, int y ) : m_iX( x ), m_iY( y ) { }
    int  X( ) const { return m_iX; }
    int  Y( ) const { return m_iY; }
    void X( int v ) { m_iX = v; }
    void Y( int v ) { m_iY = v; }
};

// Next prime at or above iMin - same contract as the production GetPrime(), which
// lives in newworld.cpp and pulls in the world.
int GetPrime( int iMin )
{
    if ( iMin < 2 )
        return 2;
    for ( int n = iMin;; ++n )
    {
        bool prime = true;
        for ( int d = 2; d * d <= n; ++d )
            if ( n % d == 0 )
            {
                prime = false;
                break;
            }
        if ( prime )
            return n;
    }
}

// Stand-in for the pooled CMap<DWORD,DWORD,CCell*,...> m_mapCell. Only the three
// members the lifecycle bodies touch are needed; the pooled allocator itself is
// per-thread and shared between instances, so it is not what this test is about.
struct FakeCellMap
{
    int m_iBuckets;
    int m_iEntries;
    int m_iRemoveAllCalls;
    FakeCellMap( ) : m_iBuckets( 0 ), m_iEntries( 0 ), m_iRemoveAllCalls( 0 ) { }
    void RemoveAll( )
    {
        m_iEntries = 0;
        ++m_iRemoveAllCalls;
    }
    void InitHashTable( int n ) { m_iBuckets = n; }
    int  GetCount( ) const { return m_iEntries; }
};

#include "pathmgr_cell.inc"   // class CCell + MAX_BOTH_INDEX, from cpathmgr.h

class CPathMgr
{
public:
    CRITICAL_SECTION m_cs;
    BOOL             m_bCsInit;
    BOOL             m_bShadow;

    CCell*      m_paCells;
    FakeCellMap m_mapCell;

    int m_iPaths, m_iOrtho, m_iHP;
    int m_iPathTicks, m_iStepCnt, m_iHangTicks, m_iHangCnt;
    int m_iWidth, m_iHeight, m_iMapEX, m_iMapEY;

    CHexCoord m_lastFrom, m_lastTo;
    BOOL      m_bVehBlock;
    int       m_iDistFactor;

    int m_iNumOfCells, m_iNextSlot, m_iFirst, m_iLast, m_iMaxPath;

    CCell* m_acBoth[MAX_BOTH_INDEX + 1];
    int    m_iLowestBoth;

    CPathMgr( int iMapEX, int iMapEY );
    CPathMgr( void );
    ~CPathMgr( );
    CPathMgr( CPathMgr const& )            = delete;
    CPathMgr& operator=( CPathMgr const& ) = delete;

    BOOL Init( int iMapEX, int iMapEY );
    void Close( );
    void ClearArray( void );
    void MarkShadow( void ) { m_bShadow = TRUE; }

    // Stand-in for one GetPath(): it is the lock acquisition that matters here, not
    // the search. Entering a deleted section is exactly what the old lifetime bug
    // would have produced on a second instance.
    void FakeSearch( )
    {
        EnterCriticalSection( &m_cs );
        LeaveCriticalSection( &m_cs );
    }
};

#include "pathmgr_actual.inc"   // production ctors / Init / Close / ClearArray / dtor

// ---------------------------------------------------------------- the lifecycle

int main( )
{
    // Unbuffered: a build whose lifetime is wrong can still die inside Win32 after the
    // failures are printed, and a buffered failure list is no failure list at all.
    setvbuf( stdout, NULL, _IONBF, 0 );

    // ---- 1. the full cycle the coordinator named, on ONE instance ----------
    {
        int initBefore = g_csInitCalls, delBefore = g_csDeleteCalls;
        CPathMgr pm;
        CHECK( CsLiveCount( ) == 0, "default ctor creates no critical section" );
        CHECK( pm.m_paCells == NULL, "default ctor allocates no arena" );

        CHECK( pm.Init( 64, 64 ) == TRUE, "Init returns TRUE" );
        CHECK( CsLiveCount( ) == 1, "Init creates the section" );
        CHECK( pm.m_paCells != NULL, "Init allocates the arena" );
        CHECK( pm.m_iNumOfCells == ( 64 + 64 ) * 5, "arena is (W+H)*5 cells" );
        CHECK( pm.m_mapCell.m_iBuckets == GetPrime( pm.m_iNumOfCells + 1 ), "hash table sized by Init" );

        pm.FakeSearch( );
        CHECK( g_csDeadEnter == 0, "search enters a live section" );

        pm.Close( );
        CHECK( pm.m_paCells == NULL, "Close frees the arena" );
        CHECK( CsLiveCount( ) == 1, "Close deliberately leaves the section live" );

        int initMid = g_csInitCalls, delMid = g_csDeleteCalls;
        CHECK( pm.Init( 64, 64 ) == TRUE, "second Init returns TRUE" );
        CHECK( g_csDeleteCalls == delMid + 1, "Init after Close DELETES the old section first" );
        CHECK( g_csInitCalls == initMid + 1, "Init after Close creates exactly one new section" );
        CHECK( g_csDoubleInit == 0, "no section is initialised while already live" );
        CHECK( CsLiveCount( ) == 1, "still exactly one live section" );

        pm.FakeSearch( );
        CHECK( g_csDeadEnter == 0, "search after the second Init enters a live section" );

        pm.Close( );
        // destructor runs at the end of this scope
        (void)initBefore;
        (void)delBefore;
    }
    CHECK( CsLiveCount( ) == 0, "destructor after Close deletes the section" );
    CHECK( g_csInitCalls == g_csDeleteCalls, "every section created was destroyed" );
    CHECK( g_csBadDelete == 0, "no section deleted twice" );

    // ---- 2. never Init'd: ctor + destruct must be clean --------------------
    {
        int del = g_csDeleteCalls;
        CPathMgr pm;
        (void)pm;
        CHECK( g_csDeleteCalls == del, "destructing a never-Init'd instance deletes nothing" );
    }

    // ---- 3. Init then destruct with NO Close (the old shape) ---------------
    {
        CPathMgr pm;
        pm.Init( 32, 32 );
        CHECK( CsLiveCount( ) == 1, "Init created a section" );
    }
    CHECK( CsLiveCount( ) == 0, "destructor without a Close still deletes the section" );
    CHECK( g_csInitCalls == g_csDeleteCalls, "balanced after the no-Close cycle" );

    // ---- 4. TWO instances at once: independent sections and arenas ---------
    {
        CPathMgr prod;
        CPathMgr shadow;
        shadow.MarkShadow( );
        CHECK( prod.m_bShadow == FALSE, "production instance is not a shadow" );
        CHECK( shadow.m_bShadow == TRUE, "MarkShadow marks only the shadow" );

        prod.Init( 64, 64 );
        shadow.Init( 64, 64 );
        CHECK( CsLiveCount( ) == 2, "two instances hold two distinct sections" );
        CHECK( prod.m_paCells != shadow.m_paCells, "two instances hold two distinct arenas" );
        CHECK( prod.m_iNumOfCells == shadow.m_iNumOfCells, "same Init dimensions give the same arena size" );

        // Interleave the way the shadow comparison does: production search, then the
        // private one, repeatedly.
        for ( int i = 0; i < 4; ++i )
        {
            prod.FakeSearch( );
            shadow.FakeSearch( );
        }
        CHECK( g_csDeadEnter == 0, "interleaved searches never touch a dead section" );

        // Close and re-Init only the shadow - the production instance must be untouched.
        CCell* prodArena = prod.m_paCells;
        shadow.Close( );
        shadow.Init( 64, 64 );
        CHECK( prod.m_paCells == prodArena, "re-Initing the shadow does not disturb production's arena" );
        prod.FakeSearch( );
        CHECK( g_csDeadEnter == 0, "production's section survived the shadow's re-Init" );

        shadow.Close( );
        prod.Close( );
    }
    CHECK( CsLiveCount( ) == 0, "both instances released their sections" );
    CHECK( g_csInitCalls == g_csDeleteCalls, "balanced across two instances" );

    // ---- 5. the dimension ctor, which used to leave m_cs uninitialised -----
    {
        CPathMgr pm( 48, 48 );
        CHECK( CsLiveCount( ) == 1, "CPathMgr(int,int) creates its section" );
        CHECK( pm.m_paCells != NULL, "CPathMgr(int,int) allocates its arena" );
        pm.FakeSearch( );
        CHECK( g_csDeadEnter == 0, "a search on a dimension-constructed instance is legal" );
    }
    CHECK( CsLiveCount( ) == 0, "dimension-constructed instance released its section" );

    // ---- 6. Init TWICE with NO intervening Close ---------------------------
    //
    // Init is re-entrant on a live instance: it frees the arena, tears the old section
    // down, then allocates both again. The freed arena pointer is nulled between the
    // delete[] and the new[] so a throwing new cannot leave the dtor a dangling arena;
    // that window is not observable from a re-Init that succeeds (the reallocated block
    // may reuse the freed address), so what is asserted HERE is the post-state: one live
    // section, no leak, a usable arena. Section 7 makes the new[] throw and checks the
    // window itself.
    {
        CPathMgr pm;
        pm.Init( 64, 64 );
        int initMid = g_csInitCalls, delMid = g_csDeleteCalls;

        CHECK( pm.Init( 40, 40 ) == TRUE, "second Init with no Close returns TRUE" );
        CHECK( g_csDeleteCalls == delMid + 1, "Init with no Close DELETES the old section first" );
        CHECK( g_csInitCalls == initMid + 1, "Init with no Close creates exactly one new section" );
        CHECK( CsLiveCount( ) == 1, "Init with no Close leaks no section" );
        CHECK( g_csDoubleInit == 0, "Init with no Close never re-initialises a live section" );
        CHECK( pm.m_paCells != NULL, "Init with no Close leaves a live arena" );
        CHECK( pm.m_iNumOfCells == ( 40 + 40 ) * 5, "the second Init's dimensions won" );

        pm.FakeSearch( );
        CHECK( g_csDeadEnter == 0, "search after a Close-less re-Init enters a live section" );
    }
    CHECK( CsLiveCount( ) == 0, "destructor after a Close-less re-Init deletes the section" );
    CHECK( g_csInitCalls == g_csDeleteCalls, "balanced after the Close-less re-Init cycle" );

    // ---- 7. the arena allocation FAILS inside a Close-less re-Init ---------
    //
    // Section 6 asserts the post-state of a re-Init that SUCCEEDS, which cannot see the
    // window this exercises. Here the injected operator new[] throws at exactly the
    // point Init has already freed the old arena and deleted the old section. Whether
    // the destructor that follows deletes the freed arena a second time is decided by
    // whether Init nulled m_paCells between the delete[] and the new[].
    //
    // On the old code m_paCells still holds the freed pointer here and the destructor
    // frees it again; the freed-pointer record catches that BEFORE the real free, so
    // the run reports a failure instead of dying in the CRT.
    {
        int    initBefore = g_csInitCalls;
        int    delBefore  = g_csDeleteCalls;
        int    badBefore  = g_csBadDelete;
        int    dblBefore  = g_doubleFrees;
        CCell* oldArena   = NULL;
        bool   threw      = false;

        {
            CPathMgr pm;
            CHECK( pm.Init( 64, 64 ) == TRUE, "7: the first Init returns TRUE" );
            oldArena = pm.m_paCells;
            CHECK( oldArena != NULL, "7: the first Init allocated an arena" );
            CHECK( FreedCount( oldArena ) == 0, "7: that arena is live going into the second Init" );

            g_bFailNextArrayNew = true;
            try
            {
                pm.Init( 40, 40 );
            }
            catch ( std::bad_alloc const& )
            {
                threw = true;
            }

            CHECK( threw, "7: the injected arena allocation threw std::bad_alloc out of Init" );
            CHECK( !g_bFailNextArrayNew, "7: the one-shot injection flag was consumed" );
            CHECK( g_lastFailedNewBytes == (std::size_t)( ( 40 + 40 ) * 5 ) * sizeof( CCell ),
                   "7: the allocation that failed was the (40+40)*5 cell arena" );

            CHECK( pm.m_paCells == NULL, "7: the failed Init left m_paCells NULL, not dangling" );
            CHECK( FreedCount( oldArena ) == 1, "7: the failed Init freed the old arena exactly once" );
            CHECK( CsLiveCount( ) == 0, "7: the failed Init left no live section" );

            // destructor runs at the end of this scope
        }

        CHECK( FreedCount( oldArena ) == 1, "7: the destructor did NOT free the old arena a second time" );
        CHECK( g_doubleFrees == dblBefore, "7: no arena was deleted twice across the failed-Init cycle" );
        CHECK( g_blockOverflow == 0, "7: the freed-pointer record never ran out of slots" );
        CHECK( CsLiveCount( ) == 0, "7: nothing left live after the failed Init and destruct" );
        CHECK( g_csInitCalls - initBefore == g_csDeleteCalls - delBefore,
               "7: sections created and destroyed balance across the failed Init" );
        CHECK( g_csBadDelete == badBefore, "7: the failed Init caused no stray section deletion" );
        CHECK( g_arrayNewFailures == 1, "7: exactly one allocation failure was injected" );
    }

    // ---- totals ------------------------------------------------------------
    CHECK( g_csInitCalls == g_csDeleteCalls, "final: create/destroy balanced" );
    CHECK( g_csDoubleInit == 0, "final: no double initialisation" );
    CHECK( g_csBadDelete == 0, "final: no stray deletion" );
    CHECK( g_csDeadEnter == 0, "final: no entry into a dead section" );
    CHECK( g_csEnterCalls > 0, "final: the fixture actually took the lock" );

    std::printf( "sections: init=%d delete=%d enter=%d\n", g_csInitCalls, g_csDeleteCalls, g_csEnterCalls );
    std::printf( "array new[]: calls=%d injected-failures=%d double-frees=%d\n", g_arrayNewCalls,
                 g_arrayNewFailures, g_doubleFrees );
    std::printf( "%d checks, %d failures\n", g_checks, g_failures );
    return g_failures ? 1 : 0;
}
