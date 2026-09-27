// Compile the snapshot view's terrain-cost / range bodies beside the production
// CGameMap ones and prove they agree, and that a published snapshot does not follow
// the live world. Every body under test is extracted verbatim by
// run-pathworld-snapshot.py, so the test fails if either copy drifts.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef int           BOOL;
typedef unsigned char BYTE;
typedef unsigned int  DWORD;
#define TRUE 1
#define FALSE 0


static int g_iChecks   = 0;
static int g_iFailures = 0;

static void check( bool ok, const char* what )
{
    ++g_iChecks;
    if ( !ok )
    {
        ++g_iFailures;
        printf( "FAIL: %s\n", what );
    }
}

// ---- scaffold -------------------------------------------------------------

class CHexCoord
{
  public:
    int m_iX, m_iY;
    CHexCoord( ): m_iX( 0 ), m_iY( 0 ) {}
    CHexCoord( int x, int y ): m_iX( x ), m_iY( y ) {}
    int  X( ) const { return m_iX; }
    int  Y( ) const { return m_iY; }
    static int Diff( int iVal );   // terrain.inl:96-98, defined once theMap exists
    static int Wrap( int iVal );   // terrain.inl:91-93, ditto (extracted verbatim)
    bool operator==( CHexCoord const& o ) const { return ( m_iX == o.m_iX ) && ( m_iY == o.m_iY ); }
    bool operator!=( CHexCoord const& o ) const { return !( *this == o ); }
};

#include "pw_wheels.inc"   // NUM_WHEEL_TYPES + class CWheelTypes, verbatim from base.h

class CHex
{
  public:
#include "pw_hexenums.inc"   // terrain-type and units enums, verbatim from terrain.h

    BYTE      m_bUnit;
    BYTE      m_bTypeVal;
    BYTE      m_bAltVal;
    CHexCoord m_hex;

    BYTE      GetUnits( ) const { return m_bUnit; }
    int       GetType( ) const { return (int)m_bTypeVal; }
    int       GetAlt( ) const { return (int)m_bAltVal; }
    CHexCoord GetHex( ) const { return m_hex; }
};

// terrain wheel-mult table, the one both sides read
static int g_aiWheelMult[CHex::num_types][NUM_WHEEL_TYPES];

class CTerrainDataStub
{
  public:
    int m_iType;
    int GetWheelMult( int iWheel ) const { return g_aiWheelMult[m_iType][iWheel]; }
};
class CTerrainStub
{
  public:
    CTerrainDataStub GetData( int iType ) const
    {
        CTerrainDataStub d;
        d.m_iType = iType;
        return d;
    }
};
static CTerrainStub theTerrain;

// one building, at the one hex the tests place it on
static bool      g_bBldgPresent  = false;
static bool      g_bBldgShipExit = false;
static CHexCoord g_hexBldg( 3, 4 );

class CStructureDataStub
{
  public:
    BOOL HasShipExit( ) const { return g_bBldgShipExit ? TRUE : FALSE; }
};
class CBuilding
{
  public:
    CStructureDataStub const* GetData( ) const
    {
        static CStructureDataStub s;
        return &s;
    }
};
class CBuildingHexStub
{
  public:
    CBuilding* _GetBuilding( CHexCoord const& hex ) const
    {
        static CBuilding s;
        if ( g_bBldgPresent && hex == g_hexBldg )
            return &s;
        return NULL;
    }
};
static CBuildingHexStub theBuildingHex;

// ---- the snapshot ---------------------------------------------------------

#include "pw_pwindex.inc"   // class CPwIndex + its three bodies, verbatim
#include "pw_dirty.inc"     // class PwDirty, verbatim from pathworld.h

class PathWorld
{
  public:
    enum { occ_none = 0, occ_moving = 1, occ_blocked = 2 };
#include "pw_structs.inc"   // struct Hex / struct Bldg, verbatim from pathworld.h

    enum { kSide = 8, kSideShift = 3 };
    Hex  m_aHex[kSide * kSide];
    Bldg m_bldg;
    bool m_bHaveBldg;
    int  m_iHexMask;
    int  m_iWidthHalf;
    int  m_iSideShift;

    PathWorld( )
        : m_bHaveBldg( false ), m_iHexMask( kSide - 1 ), m_iWidthHalf( kSide / 2 ), m_iSideShift( kSideShift )
    {
    }

#include "pw_snapindex.inc"   // PathWorld::At, verbatim from pathworld.h

    Bldg const* FindBldg( int x, int y ) const
    {
        if ( m_bHaveBldg && x == m_bldg.hexExit.X( ) && y == m_bldg.hexExit.Y( ) )
            return &m_bldg;
        return NULL;
    }
    int WheelMult( int iType, int iWheel ) const { return g_aiWheelMult[iType][iWheel]; }
    int Wrap( int iVal ) const { return ( iVal & m_iHexMask ); }
    int Diff( int iVal ) const { return ( ( ( iVal + m_iWidthHalf ) & m_iHexMask ) - m_iWidthHalf ); }
};

class CEnSnapNavView
{
  public:
    explicit CEnSnapNavView( PathWorld const& pw ): m_pw( pw ) {}
#include "pw_snapcost.inc"   // GetTerrainCost + GetRangeDistance, verbatim from ennavview.h

    // fixture-only reach-in, so a test can ask what the snapshot still holds
    int GetHexTypeForTest( int x, int y ) const { return (int)m_pw.At( x, y ).bType; }
  private:
    PathWorld const& m_pw;
};

// ---- the live bodies ------------------------------------------------------

class CGameMap
{
  public:
    int          _GetTerrainCost( CHex const* pHex, CHex const* pHexDest, int iDir, int iWheel );
    int          GetRangeDistance( CHexCoord& hex1, CHexCoord& hex2 );
    CHex const*  GetHex( int x, int y ) const;
    CHex const* _GetHex( int x, int y ) const;
    int          m_iHexMask, m_iWidthHalf;
    // the three members the live indexer reads
    CHex* m_pHex;
    int   m_eX, m_eY, m_iSideShift;
};
static CGameMap theMap;

#define ASSERT( x ) ( (void)0 )
#define ASSERT_STRICT( x ) ( (void)0 )
#define ASSERT_STRICT_VALID( x ) ( (void)0 )

#include "pw_livecost.inc"   // CGameMap::_GetTerrainCost + GetRangeDistance, verbatim from terrain.cpp
#include "pw_livehex.inc"    // CGameMap::GetHex/_GetHex + CHexCoord::Wrap, verbatim from terrain.inl

// The live Diff the production GetRangeDistance calls (terrain.inl:96-98), over the
// same two map members the snapshot copies.
int CHexCoord::Diff( int iVal )
{
    iVal += theMap.m_iWidthHalf;
    iVal &= theMap.m_iHexMask;
    iVal -= theMap.m_iWidthHalf;
    return iVal;
}

// ---- the world both sides read -------------------------------------------

static CHex      g_aLive[PathWorld::kSide * PathWorld::kSide];
static PathWorld g_pw;

static CHex* LiveAt( int x, int y ) { return &g_aLive[y * PathWorld::kSide + x]; }

// Build the snapshot from the live hexes, exactly as PathWorld::Build does.
static void BuildSnapshot( )
{
    for ( int y = 0; y < PathWorld::kSide; ++y )
        for ( int x = 0; x < PathWorld::kSide; ++x )
        {
            CHex const*    p = LiveAt( x, y );
            PathWorld::Hex h;
            h.bUnits                             = p->GetUnits( );
            h.bType                              = (BYTE)p->GetType( );
            h.bAlt                               = (BYTE)p->GetAlt( );
            h.bOcc                               = PathWorld::occ_none;
            g_pw.m_aHex[y * PathWorld::kSide + x] = h;
        }
    g_pw.m_bHaveBldg = g_bBldgPresent;
    if ( g_bBldgPresent )
    {
        g_pw.m_bldg.uKey      = 1;
        g_pw.m_bldg.hexExit   = g_hexBldg;
        g_pw.m_bldg.hexShip   = g_hexBldg;
        g_pw.m_bldg.iExitDir  = 0;
        g_pw.m_bldg.iShipDir  = 0;
        g_pw.m_bldg.bShipExit = g_bBldgShipExit ? TRUE : FALSE;
    }
}

// ---- the two index tests --------------------------------------------------
//
// 1. THE WRAP SEAM. The live view reads every hex through CGameMap::GetHex(int,int),
//    which masks BOTH axes and then indexes. PathWorld::At must land on that same hex
//    for every coordinate a search can form - a search running along the top or bottom
//    edge of the map forms y == -1 and y == side, and an unmasked At walks into
//    another row there while the live read wraps.
static void TestSeamIndexing( )
{
    for ( int y = 0; y < PathWorld::kSide; ++y )
        for ( int x = 0; x < PathWorld::kSide; ++x )
        {
            CHex* p       = LiveAt( x, y );
            p->m_bTypeVal = (BYTE)( ( x * 3 + y * 5 ) % CHex::num_types );
            p->m_bAltVal  = (BYTE)( ( x * 7 + y * 11 ) & 0x7F );
            p->m_bUnit    = (BYTE)( ( x + y ) & 3 );
            p->m_hex      = CHexCoord( x, y );
        }
    g_bBldgPresent = false;
    BuildSnapshot( );

    int iOffMap = 0, iSeam = 0;
    for ( int y = -PathWorld::kSide; y < 2 * PathWorld::kSide; ++y )
        for ( int x = -PathWorld::kSide; x < 2 * PathWorld::kSide; ++x )
        {
            CHex const*           pLive = theMap.GetHex( x, y );
            PathWorld::Hex const& hSnap = g_pw.At( x, y );
            const bool            bOff  = ( x < 0 || x >= PathWorld::kSide || y < 0 || y >= PathWorld::kSide );
            if ( bOff )
                ++iOffMap;
            if ( y == -1 || y == PathWorld::kSide )
                ++iSeam;

            if ( (int)hSnap.bType != pLive->GetType( ) || (int)hSnap.bAlt != pLive->GetAlt( ) ||
                 hSnap.bUnits != pLive->GetUnits( ) )
            {
                check( false, "PathWorld::At lands on the hex CGameMap::GetHex lands on" );
                if ( g_iFailures < 6 )
                    printf( "   at %d,%d  live type=%d alt=%d units=%d  snap type=%d alt=%d units=%d\n", x, y,
                            pLive->GetType( ), pLive->GetAlt( ), (int)pLive->GetUnits( ), (int)hSnap.bType,
                            (int)hSnap.bAlt, (int)hSnap.bUnits );
            }
        }
    check( iOffMap > 100, "the seam comparison actually probed off-map coordinates" );
    check( iSeam > 0, "the seam comparison actually probed the row above and below the map" );

    // and the cost of a step ACROSS the seam: y = kSide-1 -> y = kSide, which is row 0.
    int iSeamCosts = 0;
    for ( int x = 0; x < PathWorld::kSide; ++x )
        for ( int iWheel = 0; iWheel < NUM_WHEEL_TYPES; ++iWheel )
        {
            const CEnSnapNavView view( g_pw );
            const int iSnap = view.GetTerrainCost( CHexCoord( x, PathWorld::kSide - 1 ),
                                                   CHexCoord( x, PathWorld::kSide ), 4, iWheel );
            const int iLive = theMap._GetTerrainCost( LiveAt( x, PathWorld::kSide - 1 ), LiveAt( x, 0 ), 4, iWheel );
            if ( iSnap != iLive )
                check( false, "seam-crossing terrain cost agrees" );
            ++iSeamCosts;
        }
    check( iSeamCosts > 0, "seam-crossing costs were compared" );
}

// 2. INDEX COVERAGE. theBuildingHex / theBridgeHex hold one entry per FOOTPRINT hex,
//    and the live lookup answers for each of them. PathWorld::Build feeds the index
//    from those same entries, so FindBldg must answer for every hex the live map does
//    - not only for a building's anchor.
static DWORD PwKey( int x, int y ) { return ( ( (DWORD)x << 16 ) | (DWORD)y ); }

static void TestIndexCoverage( )
{
    enum { kMap = 64, kFootX = 2, kFootY = 3 };

    // the reference: which building (1-based id, 0 = none) owns each hex
    static DWORD aRef[kMap * kMap];
    for ( int i = 0; i < kMap * kMap; ++i ) aRef[i] = 0;

    std::vector<DWORD> aKey, aVal;
    DWORD              dwId = 0;
    for ( int by = 0; by + kFootY <= kMap; by += 7 )
        for ( int bx = 0; bx + kFootX <= kMap; bx += 5 )
        {
            ++dwId;
            for ( int fy = 0; fy < kFootY; ++fy )
                for ( int fx = 0; fx < kFootX; ++fx )
                {
                    aRef[( by + fy ) * kMap + ( bx + fx )] = dwId;
#ifdef PW_PERTURB_ANCHORONLY
                    if ( fx || fy )
                        continue;   // register the anchor hex only
#endif
                    aKey.push_back( PwKey( bx + fx, by + fy ) );
                    aVal.push_back( dwId );
                }
        }

    CPwIndex idx;
    idx.Reserve( aKey.size( ) );
    for ( size_t i = 0; i < aKey.size( ); ++i ) idx.Insert( aKey[i], aVal[i] );

    int iHits = 0, iMisses = 0;
    for ( int y = 0; y < kMap; ++y )
        for ( int x = 0; x < kMap; ++x )
        {
            const DWORD dwWant = aRef[y * kMap + x];
            const DWORD dwGot  = idx.Find( PwKey( x, y ) );
            const DWORD dwNorm = ( dwGot == (DWORD)CPwIndex::none ) ? 0u : dwGot;
            if ( dwWant )
                ++iHits;
            else
                ++iMisses;
            if ( dwWant != dwNorm )
            {
                check( false, "the snapshot index answers for every registered footprint hex" );
                if ( g_iFailures < 6 )
                    printf( "   hex %d,%d  want=%u got=%u\n", x, y, dwWant, dwNorm );
            }
        }
    check( iHits > 500, "the coverage test registered a real number of footprint hexes" );
    check( iMisses > 500, "the coverage test also checked hexes with no building" );
    check( dwId > 50, "the coverage test placed a real number of buildings" );
}

#include "pw_fill.inc"   // PwEncodeRow + PwFillHexes, verbatim from pathworld.cpp

// 3. THE INCREMENTAL REBUILD.
//
// PublishTick builds the new snapshot by copying the previous one's hex array and
// re-encoding only the rows the dirty set names. That is only sound if it is
// BYTE-IDENTICAL to a full rebuild of the same live map, so the two are run side by
// side here over several publishes: one arm is chained (each build copies the previous
// build's array), the other is built from scratch every time, and the two arrays are
// compared after each publish.
//
// Occupancy is the trap. It is written after the hex array is filled and is NOT part
// of the epoch, so a copied array still carries the PREVIOUS publish's occupancy while
// a from-scratch one starts blank. The occupancy the last build wrote has to be put
// back to occ_none by the copy, which is what pPrevOcc is for.

enum { kHexes = PathWorld::kSide * PathWorld::kSide };

static std::vector<PathWorld::Hex> g_aFull;
static std::vector<PathWorld::Hex> g_aIncA;
static std::vector<PathWorld::Hex> g_aIncB;

// The stand-in for PathWorld::Build's occupancy pass: a few hexes hold a vehicle, and
// which ones changes from publish to publish. Records exactly the hexes it set
// non-occ_none, the way the production pass records into s_aPrevOcc.
static void ApplyOcc( std::vector<PathWorld::Hex>& p, int iPhase, std::vector<uint32_t>& aOcc )
{
    std::vector<uint32_t> a;
    for ( int i = 0; i < 3; ++i )
    {
        const uint32_t uAt = (uint32_t)( ( iPhase * 7 + i * 19 ) % kHexes );
        p[uAt].bOcc        = (BYTE)( ( i & 1 ) ? PathWorld::occ_blocked : PathWorld::occ_moving );
        a.push_back( uAt );
    }
    aOcc.swap( a );
}

static void TestIncrementalRebuild( )
{
    for ( int y = 0; y < PathWorld::kSide; ++y )
        for ( int x = 0; x < PathWorld::kSide; ++x )
        {
            CHex* p       = LiveAt( x, y );
            p->m_bTypeVal = (BYTE)( ( x * 5 + y * 3 ) % CHex::num_types );
            p->m_bAltVal  = (BYTE)( ( x * 13 + y * 7 ) & 0x7F );
            p->m_bUnit    = (BYTE)( ( x * y ) & 3 );
            p->m_hex      = CHexCoord( x, y );
        }

    PwDirty dirtyAll;   // default-constructed is IsAll( ) - the full build
    PwDirty dirty;
    dirty.Reset( PathWorld::kSide );

    std::vector<uint32_t> aIncOcc, aFullOcc;

    std::vector<PathWorld::Hex>* pCur  = &g_aIncA;
    std::vector<PathWorld::Hex>* pNext = &g_aIncB;

    // first publish: no previous snapshot, so both arms are full builds
    PwFillHexes( *pCur, NULL, NULL, 0, PathWorld::kSide, PathWorld::kSide, PathWorld::kSideShift, dirtyAll );
    ApplyOcc( *pCur, 0, aIncOcc );
    dirty.Reset( PathWorld::kSide );

    // Several publishes, a few hexes each, of every kind the epoch counts - and at the
    // rows and columns the map wraps at (row 0 and row kSide-1 are the y-wrap seam,
    // column 0 and kSide-1 the x-wrap).
    static const struct
    {
        int x, y, iKind;
    } aMut[] = {
        { 4, 4, 0 },                     // road  (SetType / ChangeToRoad)
        { 0, 0, 1 }, { 1, 0, 1 },        // building bits at the top seam row
        { 7, 7, 2 }, { 7, 0, 2 },        // bridge bits at both x and y wrap corners
        { 0, 7, 3 },                     // altitude at the bottom seam row
        { 3, 0, 4 }, { 3, 7, 4 },        // terrain type on both seam rows
        { 7, 3, 0 },                     // road at the far x edge
        { 0, 3, 1 },                     // building bit at the near x edge
    };

    int iRounds = 0, iSmallRounds = 0, iSingleRow = 0, iOccCleared = 0;

    for ( int iRound = 0; iRound < (int)( sizeof( aMut ) / sizeof( aMut[0] ) ); ++iRound )
    {
        const int x = aMut[iRound].x, y = aMut[iRound].y;
        CHex*     p = LiveAt( x, y );
        switch ( aMut[iRound].iKind )
        {
            case 0: p->m_bTypeVal = CHex::road; break;
            case 1: p->m_bUnit    = (BYTE)( p->m_bUnit | CHex::bldg ); break;
            case 2: p->m_bUnit    = (BYTE)( p->m_bUnit | CHex::bridge ); break;
            case 3: p->m_bAltVal  = (BYTE)( ( p->m_bAltVal + 17 ) & 0x7F ); break;
            default: p->m_bTypeVal = (BYTE)( ( p->m_bTypeVal + 1 ) % CHex::num_types ); break;
        }
        // what EnNavTouchHexAt / EnNavTouchUnits record for that write
        dirty.Row( y );

        // the reference: a full rebuild of the live map as it stands now
        PwFillHexes( g_aFull, NULL, NULL, 0, PathWorld::kSide, PathWorld::kSide, PathWorld::kSideShift, dirtyAll );
        ApplyOcc( g_aFull, iRound + 1, aFullOcc );

        // the incremental arm, chained on the previous publish's array
        const int iRows = PwFillHexes( *pNext, pCur, aIncOcc.empty( ) ? NULL : &aIncOcc[0], aIncOcc.size( ),
                                       PathWorld::kSide, PathWorld::kSide, PathWorld::kSideShift, dirty );
        ApplyOcc( *pNext, iRound + 1, aIncOcc );

        // a hex the PREVIOUS publish had occupied and this one does not - the case a
        // copied array gets wrong unless the old occupancy is cleared
        for ( int i = 0; i < kHexes; ++i )
            if ( ( *pCur )[i].bOcc != PathWorld::occ_none && g_aFull[i].bOcc == PathWorld::occ_none )
                ++iOccCleared;

        check( (int)pNext->size( ) == kHexes && (int)g_aFull.size( ) == kHexes, "both arms sized the array alike" );
        if ( memcmp( &( *pNext )[0], &g_aFull[0], kHexes * sizeof( PathWorld::Hex ) ) != 0 )
        {
            check( false, "the incremental rebuild is byte-identical to a full rebuild" );
            if ( g_iFailures < 6 )
                for ( int i = 0; i < kHexes; ++i )
                    if ( memcmp( &( *pNext )[i], &g_aFull[i], sizeof( PathWorld::Hex ) ) != 0 )
                    {
                        printf( "   round %d (mutated %d,%d kind %d) first diff at hex %d,%d: "
                                "inc units=%02x type=%d alt=%d occ=%d  full units=%02x type=%d alt=%d occ=%d\n",
                                iRound, x, y, aMut[iRound].iKind, i & ( PathWorld::kSide - 1 ),
                                i >> PathWorld::kSideShift, (int)( *pNext )[i].bUnits, (int)( *pNext )[i].bType,
                                (int)( *pNext )[i].bAlt, (int)( *pNext )[i].bOcc, (int)g_aFull[i].bUnits,
                                (int)g_aFull[i].bType, (int)g_aFull[i].bAlt, (int)g_aFull[i].bOcc );
                        break;
                    }
        }

        ++iRounds;
        if ( iRows < PathWorld::kSide )
            ++iSmallRounds;
        if ( iRows == 1 )
            ++iSingleRow;

        std::vector<PathWorld::Hex>* pSwap = pCur;
        pCur                               = pNext;
        pNext                              = pSwap;
        dirty.Reset( PathWorld::kSide );
    }

    printf( "incremental publishes compared: %d  (rows re-encoded < map height in %d, exactly one row in %d, "
            "stale-occupancy hexes cleared %d)\n",
            iRounds, iSmallRounds, iSingleRow, iOccCleared );

    check( iRounds > 5, "several publishes were compared" );
    // A run that fell back to a full rebuild every time would pass the memcmp and prove
    // nothing about the incremental path.
    check( iSmallRounds == iRounds, "every publish really was incremental" );
    check( iSingleRow > 0, "a one-hex mutation re-encoded exactly one row" );
    // Likewise a run where occupancy never moved would never exercise the reset.
    check( iOccCleared > 0, "occupancy carried by the copy actually had to be cleared" );

    // And the overflow valve: once every row is dirty the tracker says so, and the build
    // is a full one again.
    {
        PwDirty over;
        over.Reset( PathWorld::kSide );
        check( !over.IsAll( ), "a freshly reset dirty set is not a full rebuild" );
        for ( int y = 0; y < PathWorld::kSide; ++y ) over.Row( y );
        check( over.IsAll( ), "dirtying every row falls back to a full rebuild" );
        PwDirty oob;
        oob.Reset( PathWorld::kSide );
        oob.Row( PathWorld::kSide );   // a row this map does not have
        check( oob.IsAll( ), "an out-of-range row falls back to a full rebuild" );
    }
}

int main( )
{
    theMap.m_iHexMask   = PathWorld::kSide - 1;
    theMap.m_iWidthHalf = PathWorld::kSide / 2;
    theMap.m_pHex       = g_aLive;
    theMap.m_eX = theMap.m_eY = PathWorld::kSide;
    theMap.m_iSideShift       = PathWorld::kSideShift;

    // A wheel table with a 0 for water on land and on rivers/coastline, so every
    // fallback arm in _GetTerrainCost is reachable.
    for ( int t = 0; t < CHex::num_types; ++t )
        for ( int w = 0; w < NUM_WHEEL_TYPES; ++w ) g_aiWheelMult[t][w] = ( t + 1 ) * ( w + 2 );
    for ( int t = 0; t < CHex::num_types; ++t ) g_aiWheelMult[t][CWheelTypes::water] = 0;
    g_aiWheelMult[CHex::ocean][CWheelTypes::water] = 7;
    g_aiWheelMult[CHex::lake][CWheelTypes::water]  = 5;
    g_aiWheelMult[CHex::forest][CWheelTypes::walk] = 0;   // a land 0, so the 0 return is exercised

    int iCompared = 0, iNonZero = 0, iZero = 0, iShip = 0;

    for ( int iBldg = 0; iBldg < 2; ++iBldg )
        for ( int iShipExit = 0; iShipExit < 2; ++iShipExit )
        {
            g_bBldgPresent  = ( iBldg != 0 );
            g_bBldgShipExit = ( iShipExit != 0 );

            for ( int iTypeFrom = 0; iTypeFrom < CHex::num_types; ++iTypeFrom )
                for ( int iTypeTo = 0; iTypeTo < CHex::num_types; ++iTypeTo )
                    for ( int iUnits = 0; iUnits < 4; ++iUnits )
                        for ( int iAltFrom = 0; iAltFrom < 40; iAltFrom += 13 )
                            for ( int iAltTo = 0; iAltTo < 40; iAltTo += 13 )
                                for ( int iDir = 0; iDir < 8; ++iDir )
                                    for ( int iWheel = 0; iWheel < NUM_WHEEL_TYPES; ++iWheel )
                                    {
                                        static const BYTE abUnits[4] = { 0, CHex::bldg, CHex::bridge,
                                                                         (BYTE)( CHex::bldg | CHex::bridge ) };

                                        // from = (2,4); to = the building hex (3,4)
                                        CHex* pFrom       = LiveAt( 2, 4 );
                                        CHex* pTo         = LiveAt( 3, 4 );
                                        pFrom->m_bTypeVal = (BYTE)iTypeFrom;
                                        pFrom->m_bAltVal  = (BYTE)iAltFrom;
                                        pFrom->m_bUnit    = 0;
                                        pFrom->m_hex      = CHexCoord( 2, 4 );
                                        pTo->m_bTypeVal   = (BYTE)iTypeTo;
                                        pTo->m_bAltVal    = (BYTE)iAltTo;
                                        pTo->m_bUnit      = abUnits[iUnits];
                                        pTo->m_hex        = CHexCoord( 3, 4 );

                                        BuildSnapshot( );

                                        const int iLive = theMap._GetTerrainCost( pFrom, pTo, iDir, iWheel );
                                        const CEnSnapNavView view( g_pw );
                                        const int            iSnap =
                                            view.GetTerrainCost( CHexCoord( 2, 4 ), CHexCoord( 3, 4 ), iDir, iWheel );

                                        ++iCompared;
                                        if ( iLive == 0 )
                                            ++iZero;
                                        else
                                            ++iNonZero;
                                        if ( iLive == 1 && iWheel == CWheelTypes::water &&
                                             ( pTo->m_bUnit & CHex::bldg ) && g_bBldgPresent && g_bBldgShipExit )
                                            ++iShip;

                                        if ( iLive != iSnap )
                                        {
                                            check( false, "GetTerrainCost mirror" );
                                            if ( g_iFailures < 6 )
                                                printf( "   typeFrom=%d typeTo=%d units=%02x altFrom=%d altTo=%d "
                                                        "dir=%d wheel=%d bldg=%d shipExit=%d  live=%d snap=%d\n",
                                                        iTypeFrom, iTypeTo, (int)pTo->m_bUnit, iAltFrom, iAltTo, iDir,
                                                        iWheel, iBldg, iShipExit, iLive, iSnap );
                                        }
                                    }
        }

    printf( "terrain-cost pairs compared: %d  (non-zero %d, zero %d, ship-exit-entry %d)\n", iCompared, iNonZero, iZero,
            iShip );
    // A comparison that only ever produced 0 would prove nothing.
    check( iNonZero > 1000, "the cost comparison produced non-zero costs" );
    check( iZero > 0, "the cost comparison produced impassable (0) results" );
    check( iShip > 0, "the boat-into-ship-exit-building special case was exercised" );
    check( g_iFailures == 0, "every terrain-cost pair agreed" );

    // same hex: the live entry forces iDir = 0, so the mirror must too
    {
        CHex* p     = LiveAt( 2, 4 );
        p->m_bUnit  = 0;
        p->m_bTypeVal = CHex::plain;
        p->m_bAltVal  = 20;
        g_bBldgPresent = false;
        BuildSnapshot( );
        const CEnSnapNavView view( g_pw );
        for ( int iWheel = 0; iWheel < NUM_WHEEL_TYPES; ++iWheel )
        {
            const int iLive0 = theMap._GetTerrainCost( p, p, 0, iWheel );
            const int iSnap1 = view.GetTerrainCost( CHexCoord( 2, 4 ), CHexCoord( 2, 4 ), 1, iWheel );
            check( iLive0 == iSnap1, "same-hex cost forces iDir = 0 on both sides" );
        }
    }

    // range distance
    {
        BuildSnapshot( );
        const CEnSnapNavView view( g_pw );
        int                  iSeen = 0;
        for ( int x1 = 0; x1 < PathWorld::kSide; ++x1 )
            for ( int y1 = 0; y1 < PathWorld::kSide; ++y1 )
                for ( int x2 = 0; x2 < PathWorld::kSide; ++x2 )
                    for ( int y2 = 0; y2 < PathWorld::kSide; ++y2 )
                    {
                        CHexCoord a( x1, y1 ), b( x2, y2 );
                        const int iLive = theMap.GetRangeDistance( a, b );
                        const int iSnap = view.GetRangeDistance( a, b );
                        if ( iLive != iSnap )
                            check( false, "GetRangeDistance mirror" );
                        if ( iLive > 0 )
                            ++iSeen;
                    }
        check( iSeen > 100, "the range comparison produced non-zero distances" );
    }

    // THE SNAPSHOT MUST NOT FOLLOW THE LIVE WORLD.
    {
        g_bBldgPresent = false;
        CHex* p        = LiveAt( 5, 5 );
        p->m_bUnit     = 0;
        p->m_bAltVal   = 20;
        p->m_hex       = CHexCoord( 5, 5 );
        p->m_bTypeVal  = CHex::plain;
        BuildSnapshot( );

        const CEnSnapNavView view( g_pw );
        check( view.GetHexTypeForTest( 5, 5 ) == CHex::plain, "snapshot has the type it was built from" );

        p->m_bTypeVal = CHex::mountain;   // the live world moves on
        check( LiveAt( 5, 5 )->GetType( ) == CHex::mountain, "the live hex changed" );
        check( view.GetHexTypeForTest( 5, 5 ) == CHex::plain, "the published snapshot did NOT change" );

        // and the cost read through the snapshot is still the OLD one
        CHex* pFrom       = LiveAt( 4, 5 );
        pFrom->m_bUnit    = 0;
        pFrom->m_bTypeVal = CHex::plain;
        pFrom->m_bAltVal  = 20;
        pFrom->m_hex      = CHexCoord( 4, 5 );

        const int iSnapCost = view.GetTerrainCost( CHexCoord( 4, 5 ), CHexCoord( 5, 5 ), 2, CWheelTypes::wheel );
        const int iLiveCost = theMap._GetTerrainCost( pFrom, LiveAt( 5, 5 ), 2, CWheelTypes::wheel );
        check( iSnapCost != iLiveCost, "snapshot cost differs from the live cost after the live mutation" );

        BuildSnapshot( );   // republish
        const CEnSnapNavView view2( g_pw );
        check( view2.GetTerrainCost( CHexCoord( 4, 5 ), CHexCoord( 5, 5 ), 2, CWheelTypes::wheel ) == iLiveCost,
               "a republished snapshot picks the mutation up" );
    }

    TestSeamIndexing( );
    TestIndexCoverage( );
    TestIncrementalRebuild( );

    printf( "%d checks, %d failures\n", g_iChecks, g_iFailures );
    return ( g_iFailures ? 1 : 0 );
}
