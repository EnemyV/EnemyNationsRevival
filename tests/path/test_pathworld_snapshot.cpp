// Compile the snapshot view's terrain-cost / range bodies beside the production
// CGameMap ones and prove they agree, and that a published snapshot does not follow
// the live world. Every body under test is extracted verbatim by
// run-pathworld-snapshot.py, so the test fails if either copy drifts.

#include <cstdio>
#include <cstdlib>

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

class PathWorld
{
  public:
    enum { occ_none = 0, occ_moving = 1, occ_blocked = 2 };
#include "pw_structs.inc"   // struct Hex / struct Bldg, verbatim from pathworld.h

    enum { kSide = 8 };
    Hex  m_aHex[kSide * kSide];
    Bldg m_bldg;
    bool m_bHaveBldg;
    int  m_iHexMask;
    int  m_iWidthHalf;

    PathWorld( ): m_bHaveBldg( false ), m_iHexMask( kSide - 1 ), m_iWidthHalf( kSide / 2 ) {}

    Hex const&  At( int x, int y ) const { return m_aHex[y * kSide + x]; }
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
    int _GetTerrainCost( CHex const* pHex, CHex const* pHexDest, int iDir, int iWheel );
    int GetRangeDistance( CHexCoord& hex1, CHexCoord& hex2 );
    int m_iHexMask, m_iWidthHalf;
};
static CGameMap theMap;

#define ASSERT( x ) ( (void)0 )

#include "pw_livecost.inc"   // CGameMap::_GetTerrainCost + GetRangeDistance, verbatim from terrain.cpp

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

int main( )
{
    theMap.m_iHexMask   = PathWorld::kSide - 1;
    theMap.m_iWidthHalf = PathWorld::kSide / 2;

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

    printf( "%d checks, %d failures\n", g_iChecks, g_iFailures );
    return ( g_iFailures ? 1 : 0 );
}
