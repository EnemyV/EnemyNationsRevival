// test_drag_place.cpp -- #38 step 6 (drag-place), driven by the PRODUCTION bodies.
//
// run-drag-place.py extracts these verbatim from enations_latest/src/area.cpp into
// dragplace_actual.inc (tests/orders/run-order-lifecycle.py's technique) and compiles them
// against the stand-in scene below:
//
//     struct CBuildVerdict           the shared verdict's result
//     BuildSiteVerdict()             THE per-site verdict - the hover's own body
//     BuildDragLine()                the line generator
//     HasMoveStops()                 the one-list predicate
//     CWndArea::ToBuildUL()          the footprint anchor
//     CWndArea::UpdateBuildDrag()    lay the line + judge every site
//     CWndArea::EndBuildDrag()
//     CWndArea::CommitDragTest()     OnLButtonUp's drag-commit block, lifted verbatim
//
// So a check here is a statement about the shipped code, not about a mirror of it. The one
// exception is marked: MirrorVerdict1996 is a HAND-WRITTEN copy of the pre-extraction hover
// rules (area.cpp @69b30f39), and the differential sweep asserts the extracted verdict and
// that mirror agree on every combination - which is what "the extraction did not change the
// single-site answer" means as a test.
//
// What the scene is NOT: the engine. theMap.FoundationCost, CFarmBuilding::LandMult and
// CMineBuilding::TotalQuantity/TotalDensity are scripted stubs, the map is a power-of-two
// torus the way wrldinit.cpp builds one (mask = eX-1, half = eX/2), and CWndArea::StopRoute
// is a stand-in that counts and clears. Nothing here proves pathing, rendering or the wire.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../ai/microtest.h"

using BOOL  = int;
using BYTE  = unsigned char;
using DWORD = unsigned long;
static const BOOL TRUE = 1, FALSE = 0;

#define ASSERT( x )              ( (void)0 )
#define ASSERT_VALID( x )        ( (void)0 )
#define ASSERT_STRICT( x )       ( (void)0 )
#define ASSERT_STRICT_VALID( x ) ( (void)0 )
static int g_traps = 0;
#define TRAP() ( ++g_traps )

#if !defined( __max )
#define __max( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

// the real constants (minerals.h), so the mine arithmetic is the shipped arithmetic
static const int MAX_MINERAL_DENSITY       = 255;
static const int MAX_MINERAL_QUANTITY      = 1000 * 180;
static const int MAX_MINERAL_COAL_QUANTITY = 1000 * 120;
static const int MAX_MINERAL_IRON_QUANTITY = 1000 * 180;
static const int MAX_MINERAL_OIL_QUANTITY  = 60 * 120;
static const int MAX_MINERAL_XIL_QUANTITY  = 30 * 120;

// area.h's cap, mirrored (lint-pinned in test_order_serialize.cpp's companion lint below)
static const int MAX_DRAG_PLACE = 64;

// --------------------------------------------------------------------------- the torus
// wrldinit.cpp:423 - m_iHexMask = m_eX - 1, m_iWidthHalf = m_eX / 2, one mask for both
// axes. 256 hexes keeps the seam arithmetic small enough to hand-check.
struct World
{
    int eX   = 256;
    int mask = 255;
    int half = 128;
    void Set( int n )
    {
        eX   = n;
        mask = n - 1;
        half = n / 2;
    }
};
static World g_world;

struct CPoint
{
    int x, y;
    CPoint( ): x( 0 ), y( 0 ) {}
    CPoint( int a, int b ): x( a ), y( b ) {}
};

struct CHexCoord
{
    int m_iX, m_iY;
    CHexCoord( ): m_iX( 0 ), m_iY( 0 ) {}
    CHexCoord( int x, int y ): m_iX( x ), m_iY( y ) {}
    int  X( ) const { return m_iX; }
    int  Y( ) const { return m_iY; }
    int& X( ) { return m_iX; }
    int& Y( ) { return m_iY; }
    // terrain.inl, verbatim arithmetic
    CHexCoord& Wrap( )
    {
        m_iX &= g_world.mask;
        m_iY &= g_world.mask;
        return ( *this );
    }
    static int Diff( int iVal )
    {
        iVal += g_world.half;
        iVal &= g_world.mask;
        iVal -= g_world.half;
        return ( iVal );
    }
    BOOL operator==( CHexCoord o ) const { return ( ( m_iX == o.m_iX ) && ( m_iY == o.m_iY ) ); }
    BOOL operator!=( CHexCoord o ) const { return ( !( *this == o ) ); }
};

// --------------------------------------------------------------------- building data
struct CStructureData
{
    // the ids the verdict switches on (building.h's enum, values irrelevant - only the
    // NAMES are referenced by the extracted body)
    enum
    {
        apartment = 1,
        farm,
        lumber,
        coal,
        iron,
        oil_well,
        copper,
        rocket,
        kNumIds
    };
    int m_cx = 2, m_cy = 2;
    int GetCX( ) const { return m_cx; }
    int GetCY( ) const { return m_cy; }
};

struct Structures
{
    CStructureData m_a[CStructureData::kNumIds];
    int            GetNumBuildings( ) const { return ( CStructureData::kNumIds - 1 ); }
    CStructureData const* GetData( int i ) const
    {
        if ( ( i <= 0 ) || ( i >= CStructureData::kNumIds ) )
            return ( nullptr );
        return ( &m_a[i] );
    }
    void SetSize( int i, int cx, int cy )
    {
        m_a[i].m_cx = cx;
        m_a[i].m_cy = cy;
    }
};
static Structures theStructures;

// --------------------------------------------------------------- the scripted terrain
// FoundationCost's answer per hex. Default = a buildable site; an entry in m_refuse makes
// the hex refuse with that iWhy. Keyed on the wrapped hex, as the production call is.
struct Map
{
    struct Refusal
    {
        int x, y, iWhy;
    };
    std::vector<Refusal> m_refuse;
    int                  m_iCost  = 7;
    int                  m_nCalls = 0;

    void Clear( )
    {
        m_refuse.clear( );
        m_nCalls = 0;
    }
    void Refuse( int x, int y, int iWhy = 3 ) { m_refuse.push_back( Refusal{ x, y, iWhy } ); }

    int FoundationCost( CHexCoord const& hex, int iType, int iDir, void const* pVeh, int* piAlt,
                        int* piWhy )
    {
        (void)iType;
        (void)iDir;
        (void)pVeh;
        (void)piAlt;
        ++m_nCalls;
        for ( size_t i = 0; i < m_refuse.size( ); ++i )
            if ( ( m_refuse[i].x == hex.X( ) ) && ( m_refuse[i].y == hex.Y( ) ) )
            {
                if ( piWhy != nullptr )
                    *piWhy = m_refuse[i].iWhy;
                return ( -1 );
            }
        if ( piWhy != nullptr )
            *piWhy = 0;
        return ( m_iCost );
    }
};
static Map theMap;

// the farm / mine measurements, scripted the same way
static int g_iLandMult    = 9;
static int g_iQuantity    = 100000;
static int g_iDensity     = 400;
static int g_nLandCalls   = 0;
static int g_nQuanCalls   = 0;
static int g_nDenCalls    = 0;

struct CFarmBuilding
{
    static int LandMult( CHexCoord hex, int iTyp, int iDir )
    {
        (void)hex;
        (void)iTyp;
        (void)iDir;
        ++g_nLandCalls;
        return ( g_iLandMult );
    }
};

struct CMineBuilding
{
    static int TotalQuantity( CHexCoord const& hex, int iTyp, int iDir )
    {
        (void)hex;
        (void)iTyp;
        (void)iDir;
        ++g_nQuanCalls;
        return ( g_iQuantity );
    }
    static int TotalDensity( CHexCoord const& hex, int iTyp, int iDir )
    {
        (void)hex;
        (void)iTyp;
        (void)iDir;
        ++g_nDenCalls;
        return ( g_iDensity );
    }
};

// --------------------------------------------------------------------------- vehicle
class CRoute
{
  public:
    enum
    {
        waypoint,
        unload,
        load,
        build,
        build_road,
        repair
    };
    CRoute( CHexCoord const& hex, int iType, int iBldgType, int iDir )
        : m_hex( hex ), m_iType( iType ), m_iBldgType( iBldgType ), m_iDir( iDir )
    {
    }
    CHexCoord const& GetCoord( ) const { return m_hex; }
    int              GetRouteType( ) const { return m_iType; }
    int              GetBldgType( ) const { return m_iBldgType; }
    int              GetBldgDir( ) const { return m_iDir; }
    static BOOL      IsOrder( int iType ) { return ( iType >= build ); }

  private:
    CHexCoord m_hex;
    int       m_iType, m_iBldgType, m_iDir;
};

// the CList<CRoute*,CRoute*> API the extracted HasMoveStops uses
struct Node
{
    CRoute* val;
    Node*   next;
};
using POSITION = Node*;

struct RouteList
{
    Node* head = nullptr;
    Node* tail = nullptr;
    int   n    = 0;

    POSITION AddTail( CRoute* v )
    {
        Node* p = new Node{ v, nullptr };
        if ( tail != nullptr )
            tail->next = p;
        else
            head = p;
        tail = p;
        ++n;
        return ( p );
    }
    POSITION GetHeadPosition( ) const { return head; }
    CRoute*  GetNext( POSITION& p ) const
    {
        CRoute* v = p->val;
        p         = p->next;
        return ( v );
    }
    int  GetCount( ) const { return n; }
    void RemoveAll( )
    {
        while ( head != nullptr )
        {
            Node* p = head;
            head    = head->next;
            delete p;
        }
        tail = nullptr;
        n    = 0;
    }
};

class CVehicle
{
  public:
    RouteList m_route;
    int       m_nNextOrder = 0;   // how many times NextOrder was called

    RouteList& GetRouteList( ) { return ( m_route ); }
    void       AddOrder( CHexCoord const& hex, int iType, int iBldgType, int iDir,
                         CHexCoord const* pHexEnd = nullptr )
    {
        (void)pHexEnd;
        m_route.AddTail( new CRoute( hex, iType, iBldgType, iDir ) );
    }
    BOOL NextOrder( )
    {
        ++m_nNextOrder;
        return ( FALSE );
    }
};

// ------------------------------------------------------------------ the area window
// Only the members the extracted bodies touch. The name is CWndArea on purpose: the
// production method bodies come over verbatim, signature and all.
class CWndArea
{
  public:
    enum
    {
        normal,
        normal_select,
        build_ready,
        build_loc
    };

    struct AnimAtr
    {
        int       m_iDir = 0;
        CHexCoord m_hexAt;                                  // what the cursor is over
        CHexCoord WindowToHex( CPoint pt ) const
        {
            (void)pt;
            CHexCoord h( m_hexAt );
            return ( h.Wrap( ) );
        }
    };

    AnimAtr   m_aa;
    int       m_iMode      = build_loc;
    int       m_iBuild     = CStructureData::apartment;
    int       m_iBuildDir  = 0;
    BOOL      m_bBuildDrag = FALSE;
    CHexCoord m_hexDragDn;
    CHexCoord m_ahexDrag[MAX_DRAG_PLACE];
    BYTE      m_abDragOk[MAX_DRAG_PLACE];
    int       m_nDragSites = 0;
    int       m_iDragCx    = 0;
    int       m_iDragCy    = 0;

    // the commit block's two locals, hoisted so the lifted code compiles unchanged
    CVehicle* pVehBuild  = nullptr;
    BOOL      bDragBuild = FALSE;
    int       m_nStopRoute = 0;   // how many times the one-list take-over fired

    // area.h, verbatim (lint-pinned)
    int GetBuildDir( ) const { return ( ( m_aa.m_iDir + m_iBuildDir ) & 3 ); }

    // a stand-in: the real one releases the auto-router and refreshes the route window
    void StopRoute( CVehicle* pVeh )
    {
        ++m_nStopRoute;
        pVeh->m_route.RemoveAll( );
    }

    // the PRODUCTION bodies (dragplace_actual.inc)
    CHexCoord ToBuildUL( CHexCoord& hexCur );
    void      UpdateBuildDrag( CPoint point );
    void      EndBuildDrag( );
    void      CommitDragTest( );   // OnLButtonUp's drag-commit block, verbatim
};

#include "dragplace_actual.inc"

// ===========================================================================
// the 1996 single-site rules, HAND-WRITTEN from area.cpp @69b30f39's OnMouseMove -
// the one mirror in this file. The differential sweep below asserts the extracted
// BuildSiteVerdict answers exactly this, for every combination it is fed.
// ===========================================================================
struct Mirror1996
{
    int iFound;
    int iCurType;
    BOOL bEarly;
};

static Mirror1996 MirrorVerdict1996( CHexCoord const& hexUL, int iBuild, int iDir, BOOL bBuildOk )
{
    Mirror1996 m;
    int        iWhy = 0;
    m.iFound        = theMap.FoundationCost( hexUL, iBuild, iDir, NULL, NULL, &iWhy );
    m.iCurType      = 0;
    m.bEarly        = ( m.iFound < 0 ) || ( !bBuildOk );
    if ( m.bEarly )
    {
        m.iCurType = 1;
        return ( m );
    }

    m.iCurType = m.iFound < 0 ? 1 : 0;
    switch ( iBuild )
    {
    case CStructureData::farm:
    case CStructureData::lumber: {
        int iMul = CFarmBuilding::LandMult( hexUL, iBuild, iDir );
        if ( ( iMul < 2 ) || ( m.iFound < 0 ) )
        {
            m.iFound   = -1;
            m.iCurType = 1;
        }
        else if ( iMul < 5 )
            m.iCurType = 2;
        break;
    }
    case CStructureData::coal:
    case CStructureData::iron:
    case CStructureData::oil_well:
    case CStructureData::copper: {
        CStructureData const* pData = theStructures.GetData( iBuild );
        int                   iSize = pData->GetCX( ) * pData->GetCY( );
        int                   qMul  = CMineBuilding::TotalQuantity( hexUL, iBuild, iDir );
        int                   iDiv;
        switch ( iBuild )
        {
        case CStructureData::coal:
            iDiv = MAX_MINERAL_COAL_QUANTITY;
            break;
        case CStructureData::iron:
            iDiv = MAX_MINERAL_IRON_QUANTITY;
            break;
        case CStructureData::oil_well:
            iDiv = MAX_MINERAL_OIL_QUANTITY;
            break;
        case CStructureData::copper:
            iDiv = MAX_MINERAL_XIL_QUANTITY;
            break;
        default:
            iDiv = MAX_MINERAL_QUANTITY;
            break;
        }
        int iQuan = ( qMul * 1000 ) / ( iDiv * iSize );
        if ( qMul > 0 )
            iQuan = __max( 1, iQuan );
        int dMul = CMineBuilding::TotalDensity( hexUL, iBuild, iDir );
        int iDen = ( dMul * 100 ) / ( MAX_MINERAL_DENSITY * iSize );
        if ( dMul > 0 )
            iDen = __max( 1, iDen );
        if ( ( qMul < 2 ) || ( m.iFound < 0 ) || ( dMul < 1 ) )
        {
            m.iFound   = -1;
            m.iCurType = 1;
        }
        else if ( ( iQuan < 400 / ( iSize / 2 ) ) && ( iDen < 40 / ( iSize / 2 ) ) )
            m.iCurType = 2;
        break;
    }
    default:
        break;
    }
    return ( m );
}

// =========================================================================== helpers
static void ResetScene( )
{
    g_world.Set( 256 );
    theMap.Clear( );
    theMap.m_iCost = 7;
    g_iLandMult    = 9;
    g_iQuantity    = 100000;
    g_iDensity     = 400;
    g_nLandCalls = g_nQuanCalls = g_nDenCalls = 0;
    for ( int i = 1; i < CStructureData::kNumIds; ++i )
        theStructures.SetSize( i, 2, 2 );
}

// drive the production UpdateBuildDrag: press at hexFrom, cursor now at hexTo
static void LayLine( CWndArea& w, CHexCoord hexFrom, CHexCoord hexTo )
{
    w.m_bBuildDrag = TRUE;
    w.m_hexDragDn  = hexFrom;
    w.m_aa.m_hexAt = hexTo;
    w.UpdateBuildDrag( CPoint( 0, 0 ) );
}

// ===========================================================================
// 1. the line generator: spacing, direction, dominant axis, the seam, degenerate cases
// ===========================================================================
static void test_line_spacing_per_axis( )
{
    ResetScene( );
    CHexCoord sites[MAX_DRAG_PLACE];

    // cx = 3 going east: one footprint every 3 hexes, the far end INCLUDED only when it
    // is a whole number of footprints away (10 / 3 = 3 steps -> 4 sites, last at +9)
    int n = BuildDragLine( CHexCoord( 20, 20 ), CHexCoord( 30, 20 ), 3, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 4 );
    CHECK( sites[0] == CHexCoord( 20, 20 ) );
    CHECK( sites[1] == CHexCoord( 23, 20 ) );
    CHECK( sites[2] == CHexCoord( 26, 20 ) );
    CHECK( sites[3] == CHexCoord( 29, 20 ) );

    // the SAME drag with the other axis dominant uses cy, not cx
    n = BuildDragLine( CHexCoord( 20, 20 ), CHexCoord( 20, 30 ), 3, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 6 );
    CHECK( sites[1] == CHexCoord( 20, 22 ) );
    CHECK( sites[5] == CHexCoord( 20, 30 ) );

    // exactly one footprint of travel = two sites, abutting
    n = BuildDragLine( CHexCoord( 5, 5 ), CHexCoord( 8, 5 ), 3, 3, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 2 );
    CHECK( sites[1] == CHexCoord( 8, 5 ) );

    // a drag SHORTER than one footprint lays one site: the line never places a building
    // the player did not drag far enough to ask for
    n = BuildDragLine( CHexCoord( 5, 5 ), CHexCoord( 7, 5 ), 3, 3, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 1 );
    CHECK( sites[0] == CHexCoord( 5, 5 ) );
}

static void test_line_direction( )
{
    ResetScene( );
    CHexCoord sites[MAX_DRAG_PLACE];

    int n = BuildDragLine( CHexCoord( 40, 40 ), CHexCoord( 30, 40 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 6 );
    CHECK( sites[1] == CHexCoord( 38, 40 ) );
    CHECK( sites[5] == CHexCoord( 30, 40 ) );

    n = BuildDragLine( CHexCoord( 40, 40 ), CHexCoord( 40, 34 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 4 );
    CHECK( sites[1] == CHexCoord( 40, 38 ) );
    CHECK( sites[3] == CHexCoord( 40, 34 ) );
}

static void test_line_dominant_axis( )
{
    ResetScene( );
    CHexCoord sites[MAX_DRAG_PLACE];

    // |dx| > |dy| -> a pure X line, the off-axis drift DISCARDED (one axis only)
    int n = BuildDragLine( CHexCoord( 10, 10 ), CHexCoord( 18, 13 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 5 );
    for ( int i = 0; i < n; ++i )
        CHECK_EQ( sites[i].Y( ), 10 );

    // |dy| > |dx| -> a pure Y line
    n = BuildDragLine( CHexCoord( 10, 10 ), CHexCoord( 13, 18 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 5 );
    for ( int i = 0; i < n; ++i )
        CHECK_EQ( sites[i].X( ), 10 );

    // a TIE goes to X - the same tie-break the road preview makes (abs(x) >= abs(y))
    n = BuildDragLine( CHexCoord( 10, 10 ), CHexCoord( 16, 16 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 4 );
    CHECK( sites[3] == CHexCoord( 16, 10 ) );
}

static void test_line_crosses_the_seam( )
{
    ResetScene( );   // 256-wide torus: mask 255, half 128
    CHexCoord sites[MAX_DRAG_PLACE];

    // 254 -> 4 is SIX hexes east the short way round, not 250 west
    int n = BuildDragLine( CHexCoord( 254, 9 ), CHexCoord( 4, 9 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 4 );
    CHECK( sites[0] == CHexCoord( 254, 9 ) );
    CHECK( sites[1] == CHexCoord( 0, 9 ) );     // wrapped, so an order can carry it
    CHECK( sites[2] == CHexCoord( 2, 9 ) );
    CHECK( sites[3] == CHexCoord( 4, 9 ) );

    // and the other way across it
    n = BuildDragLine( CHexCoord( 2, 9 ), CHexCoord( 252, 9 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 4 );
    CHECK( sites[1] == CHexCoord( 0, 9 ) );
    CHECK( sites[2] == CHexCoord( 254, 9 ) );
    CHECK( sites[3] == CHexCoord( 252, 9 ) );

    // the Y seam behaves the same
    n = BuildDragLine( CHexCoord( 9, 255 ), CHexCoord( 9, 3 ), 2, 2, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 3 );
    CHECK( sites[1] == CHexCoord( 9, 1 ) );
    CHECK( sites[2] == CHexCoord( 9, 3 ) );

    // every site is canonical (inside the world) no matter where the drag started
    n = BuildDragLine( CHexCoord( 250, 250 ), CHexCoord( 20, 250 ), 4, 4, sites, MAX_DRAG_PLACE );
    CHECK( n > 1 );
    for ( int i = 0; i < n; ++i )
        CHECK( ( sites[i].X( ) >= 0 ) && ( sites[i].X( ) < 256 ) && ( sites[i].Y( ) >= 0 ) &&
               ( sites[i].Y( ) < 256 ) );
}

static void test_line_degenerate( )
{
    ResetScene( );
    CHexCoord sites[MAX_DRAG_PLACE];

    // a zero-length drag is ONE site - a plain Shift-place, unchanged
    int n = BuildDragLine( CHexCoord( 11, 12 ), CHexCoord( 11, 12 ), 2, 3, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, 1 );
    CHECK( sites[0] == CHexCoord( 11, 12 ) );

    // the cap is honoured and never overrun: a half-world drag at 1-hex spacing
    n = BuildDragLine( CHexCoord( 0, 0 ), CHexCoord( 127, 0 ), 1, 1, sites, MAX_DRAG_PLACE );
    CHECK_EQ( n, MAX_DRAG_PLACE );

    // nonsense inputs refuse rather than divide by zero or write through a null
    CHECK_EQ( BuildDragLine( CHexCoord( 0, 0 ), CHexCoord( 9, 0 ), 0, 0, sites, MAX_DRAG_PLACE ), 10 );
    CHECK_EQ( BuildDragLine( CHexCoord( 0, 0 ), CHexCoord( 9, 0 ), 2, 2, nullptr, MAX_DRAG_PLACE ), 0 );
    CHECK_EQ( BuildDragLine( CHexCoord( 0, 0 ), CHexCoord( 9, 0 ), 2, 2, sites, 0 ), 0 );
}

// ===========================================================================
// 2. the shared verdict: identical to the 1996 single-site rules, and the line uses it
// ===========================================================================
static void test_verdict_matches_the_1996_rules( )
{
    ResetScene( );
    const int aiBuild[] = { CStructureData::apartment, CStructureData::farm, CStructureData::lumber,
                            CStructureData::coal,      CStructureData::iron, CStructureData::oil_well,
                            CStructureData::copper };
    const int aiLand[]  = { 0, 1, 2, 4, 5, 9 };
    const int aiQuan[]  = { 0, 1, 2, 500, 100000, 2000000 };
    const int aiDen[]   = { 0, 1, 40, 400 };

    for ( int iB = 0; iB < 7; ++iB )
        for ( int iRefuse = 0; iRefuse < 2; ++iRefuse )
            for ( int iOk = 0; iOk < 2; ++iOk )
                for ( int iL = 0; iL < 6; ++iL )
                    for ( int iQ = 0; iQ < 6; ++iQ )
                        for ( int iD = 0; iD < 4; ++iD )
                            for ( int iSize = 0; iSize < 2; ++iSize )
                            {
                                theMap.Clear( );
                                if ( iRefuse )
                                    theMap.Refuse( 30, 30, 4 );
                                g_iLandMult = aiLand[iL];
                                g_iQuantity = aiQuan[iQ];
                                g_iDensity  = aiDen[iD];
                                theStructures.SetSize( aiBuild[iB], iSize ? 4 : 2, iSize ? 4 : 2 );

                                CHexCoord hex( 30, 30 );
                                CBuildVerdict v;
                                BuildSiteVerdict( hex, aiBuild[iB], 1, iOk ? TRUE : FALSE, v );
                                Mirror1996 m = MirrorVerdict1996( hex, aiBuild[iB], 1, iOk ? TRUE : FALSE );

                                CHECK_EQ( v.m_iFound, m.iFound );
                                CHECK_EQ( v.m_iCurType, m.iCurType );
                                CHECK_EQ( v.m_bRefused ? 1 : 0, m.bEarly ? 1 : 0 );
                                // what the drag reads, stated independently of either
                                CHECK_EQ( v.CanBuild( ) ? 1 : 0,
                                          ( ( !m.bEarly ) && ( m.iFound >= 0 ) ) ? 1 : 0 );
                            }
}

static void test_verdict_measures_each_hex_once( )
{
    ResetScene( );
    CBuildVerdict v;
    CHexCoord     hex( 12, 12 );

    // a plain building asks FoundationCost and nothing else
    theMap.m_nCalls = 0;
    BuildSiteVerdict( hex, CStructureData::apartment, 0, TRUE, v );
    CHECK_EQ( theMap.m_nCalls, 1 );
    CHECK_EQ( g_nLandCalls, 0 );
    CHECK_EQ( g_nQuanCalls, 0 );

    // a farm measures the land ONCE (the hover used to call LandMult once; the extraction
    // must not have turned that into two)
    g_nLandCalls = 0;
    BuildSiteVerdict( hex, CStructureData::farm, 0, TRUE, v );
    CHECK_EQ( g_nLandCalls, 1 );
    CHECK_EQ( v.m_iKind, CBuildVerdict::farm_kind );
    CHECK_EQ( v.m_iMul, 9 );

    // a mine measures quantity and density once each, and reports the DISPLAYED numbers
    g_nQuanCalls = g_nDenCalls = 0;
    theStructures.SetSize( CStructureData::coal, 2, 2 );
    g_iQuantity = MAX_MINERAL_COAL_QUANTITY * 4;   // a full 2x2 coal field
    g_iDensity  = MAX_MINERAL_DENSITY * 4;
    BuildSiteVerdict( hex, CStructureData::coal, 0, TRUE, v );
    CHECK_EQ( g_nQuanCalls, 1 );
    CHECK_EQ( g_nDenCalls, 1 );
    CHECK_EQ( v.m_iKind, CBuildVerdict::mine_kind );
    CHECK_EQ( v.m_iQuan, 1000 );
    CHECK_EQ( v.m_iDen, 100 );
    CHECK_EQ( v.m_iQuality, CBuildVerdict::good );

    // a REFUSED site measures nothing else at all - the hover returned before the farm
    // block, and the verdict has to return there too or a refused farm hex would call
    // LandMult that the hover never called
    ResetScene( );
    theMap.Refuse( 12, 12, 2 );
    theMap.m_nCalls = 0;
    g_nLandCalls    = 0;
    BuildSiteVerdict( hex, CStructureData::farm, 0, TRUE, v );
    CHECK_EQ( theMap.m_nCalls, 1 );
    CHECK_EQ( g_nLandCalls, 0 );
    CHECK( v.m_bRefused );
    CHECK_EQ( v.m_iWhy, 2 );
    CHECK( !v.CanBuild( ) );
}

static void test_verdict_overrides_reject_the_site( )
{
    ResetScene( );
    CBuildVerdict v;
    CHexCoord     hex( 12, 12 );

    // a farm on land that yields too little: buildable ground, unbuildable FARM
    g_iLandMult = 1;
    BuildSiteVerdict( hex, CStructureData::farm, 0, TRUE, v );
    CHECK_EQ( v.m_iFound, -1 );
    CHECK( !v.CanBuild( ) );
    CHECK( !v.m_bRefused );   // the ground was fine; the override did this
    CHECK_EQ( v.m_iCurType, 1 );

    // a poor-but-legal farm is still a site, and is flagged as poor
    g_iLandMult = 4;
    BuildSiteVerdict( hex, CStructureData::farm, 0, TRUE, v );
    CHECK( v.CanBuild( ) );
    CHECK_EQ( v.m_iQuality, CBuildVerdict::warn );
    CHECK_EQ( v.m_iCurType, 2 );

    // a mine with no ore
    g_iQuantity = 1;
    g_iDensity  = 400;
    BuildSiteVerdict( hex, CStructureData::iron, 0, TRUE, v );
    CHECK( !v.CanBuild( ) );
    g_iQuantity = 100000;
    g_iDensity  = 0;
    BuildSiteVerdict( hex, CStructureData::iron, 0, TRUE, v );
    CHECK( !v.CanBuild( ) );

    // the rocket exit test failing refuses the site without touching m_iFound's cost -
    // the 1996 behaviour the extraction had to preserve
    ResetScene( );
    BuildSiteVerdict( hex, CStructureData::rocket, 0, FALSE, v );
    CHECK( v.m_bRefused );
    CHECK_EQ( v.m_iFound, 7 );
    CHECK( !v.CanBuild( ) );
}

// ===========================================================================
// 3. the line and the verdict together: a bad site in the middle is skipped, not fatal
// ===========================================================================
static void test_line_judges_every_site( )
{
    ResetScene( );
    CWndArea w;
    theStructures.SetSize( CStructureData::apartment, 2, 2 );

    // five 2x2 sites east from 20,20; refuse the MIDDLE one
    // the sites are footprint ANCHORS, so a dir-0 2x2 sits 2 hexes north of the cursor hex
    theMap.Refuse( 24, 18, 3 );
    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 28, 20 ) );
    CHECK_EQ( w.m_nDragSites, 5 );
    CHECK( w.m_ahexDrag[0] == CHexCoord( 20, 18 ) );
    CHECK_EQ( w.m_abDragOk[0], 1 );
    CHECK_EQ( w.m_abDragOk[1], 1 );
    CHECK_EQ( w.m_abDragOk[2], 0 );   // 24,18
    CHECK_EQ( w.m_abDragOk[3], 1 );
    CHECK_EQ( w.m_abDragOk[4], 1 );

    // the footprint the line is spaced by is the DIR-SWAPPED one, as the cursor's is
    theStructures.SetSize( CStructureData::apartment, 4, 2 );
    w.m_aa.m_iDir = 0;
    w.m_iBuildDir = 0;
    LayLine( w, CHexCoord( 20, 40 ), CHexCoord( 32, 40 ) );
    CHECK_EQ( w.m_iDragCx, 4 );
    CHECK_EQ( w.m_iDragCy, 2 );
    CHECK_EQ( w.m_nDragSites, 4 );

    w.m_iBuildDir = 1;   // facing turned 90 degrees: the 4x2 is now 2x4
    LayLine( w, CHexCoord( 20, 40 ), CHexCoord( 32, 40 ) );
    CHECK_EQ( w.m_iDragCx, 2 );
    CHECK_EQ( w.m_iDragCy, 4 );
    CHECK_EQ( w.m_nDragSites, 7 );

    // an unarmed window lays no line at all (no building, no drag)
    w.m_iBuildDir = 0;
    w.m_iBuild    = 0;
    LayLine( w, CHexCoord( 20, 40 ), CHexCoord( 32, 40 ) );
    CHECK_EQ( w.m_nDragSites, 0 );

    // EndBuildDrag takes the preview off and disarms the gesture
    w.m_iBuild = CStructureData::apartment;
    LayLine( w, CHexCoord( 20, 40 ), CHexCoord( 32, 40 ) );
    CHECK( w.m_nDragSites > 1 );
    w.EndBuildDrag( );
    CHECK_EQ( w.m_nDragSites, 0 );
    CHECK_EQ( w.m_bBuildDrag, FALSE );
    // ... and UpdateBuildDrag on a disarmed window lays nothing, so a release after a
    // cancel cannot queue a line
    w.UpdateBuildDrag( CPoint( 0, 0 ) );
    CHECK_EQ( w.m_nDragSites, 0 );
}

static void test_line_anchors_on_the_footprint_ul( )
{
    ResetScene( );
    CWndArea w;
    theStructures.SetSize( CStructureData::apartment, 2, 3 );

    // dir 0: ToBuildUL lifts the anchor cy hexes north of the cursor hex, and the LINE is
    // laid on the anchors, so every site is a real placement
    w.m_aa.m_iDir = 0;
    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 24, 20 ) );
    CHECK_EQ( w.m_nDragSites, 3 );
    CHECK( w.m_ahexDrag[0] == CHexCoord( 20, 17 ) );
    CHECK( w.m_ahexDrag[2] == CHexCoord( 24, 17 ) );

    // dir 2 shifts it west by cx instead
    w.m_aa.m_iDir = 2;
    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 24, 20 ) );
    CHECK( w.m_ahexDrag[0] == CHexCoord( 18, 20 ) );
}

// ===========================================================================
// 4. the commit: appends in drag order, skips the bad ones, one NextOrder, one list
// ===========================================================================
static void test_commit_appends_in_drag_order( )
{
    ResetScene( );
    CWndArea w;
    CVehicle veh;
    w.pVehBuild  = &veh;
    w.bDragBuild = TRUE;
    w.m_iBuild   = CStructureData::apartment;

    theMap.Refuse( 24, 18, 3 );
    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 28, 20 ) );
    CHECK_EQ( w.m_nDragSites, 5 );

    w.CommitDragTest( );

    // four orders, the skipped site absent, the rest in DRAG ORDER
    CHECK_EQ( veh.m_route.GetCount( ), 4 );
    POSITION  pos = veh.m_route.GetHeadPosition( );
    const int aiX[] = { 20, 22, 26, 28 };
    for ( int i = 0; i < 4; ++i )
    {
        CRoute* pR = veh.m_route.GetNext( pos );
        CHECK( pR != nullptr );
        CHECK_EQ( pR->GetRouteType( ), CRoute::build );
        CHECK_EQ( pR->GetBldgType( ), CStructureData::apartment );
        CHECK_EQ( pR->GetCoord( ).X( ), aiX[i] );
        CHECK_EQ( pR->GetCoord( ).Y( ), 18 );
    }

    // NextOrder exactly ONCE for the whole line - an idle crane starts, a busy one is
    // untouched, and the N-1 refusals a per-site call would make never happen
    CHECK_EQ( veh.m_nNextOrder, 1 );
    // the gesture is over, so the preview comes off and the orders' own ghosts take over
    CHECK_EQ( w.m_nDragSites, 0 );
    CHECK_EQ( w.m_bBuildDrag, FALSE );
}

static void test_commit_all_sites_bad_queues_nothing( )
{
    ResetScene( );
    CWndArea w;
    CVehicle veh;
    w.pVehBuild  = &veh;
    w.bDragBuild = TRUE;

    for ( int x = 20; x <= 28; ++x )
        theMap.Refuse( x, 18, 3 );
    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 28, 20 ) );
    CHECK_EQ( w.m_nDragSites, 5 );

    w.CommitDragTest( );
    CHECK_EQ( veh.m_route.GetCount( ), 0 );
    CHECK_EQ( veh.m_nNextOrder, 1 );   // harmless: NextOrder on an empty queue is a no-op
    CHECK_EQ( w.m_bBuildDrag, FALSE );
}

static void test_commit_takes_a_movement_route_over( )
{
    ResetScene( );
    CWndArea w;
    CVehicle veh;
    w.pVehBuild  = &veh;
    w.bDragBuild = TRUE;

    // the crane is running two movement stops: the FIRST order appended takes the list
    // over (#38's one-list rule), so no list ever holds both kinds
    veh.m_route.AddTail( new CRoute( CHexCoord( 1, 1 ), CRoute::waypoint, 0, 0 ) );
    veh.m_route.AddTail( new CRoute( CHexCoord( 2, 2 ), CRoute::waypoint, 0, 0 ) );
    CHECK( HasMoveStops( &veh ) );

    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 24, 20 ) );
    w.CommitDragTest( );

    CHECK_EQ( w.m_nStopRoute, 1 );
    CHECK_EQ( veh.m_route.GetCount( ), 3 );
    CHECK( !HasMoveStops( &veh ) );

    // a crane holding ORDERS only is not "running stops", so a second drag appends behind
    // the first line instead of wiping it
    w.bDragBuild = TRUE;
    LayLine( w, CHexCoord( 40, 40 ), CHexCoord( 42, 40 ) );
    w.CommitDragTest( );
    CHECK_EQ( w.m_nStopRoute, 1 );
    CHECK_EQ( veh.m_route.GetCount( ), 5 );
}

static void test_commit_is_skipped_when_it_is_not_a_drag( )
{
    ResetScene( );
    CWndArea w;
    CVehicle veh;
    w.pVehBuild  = &veh;
    w.bDragBuild = FALSE;   // OnLButtonUp decided this release was a single placement

    LayLine( w, CHexCoord( 20, 20 ), CHexCoord( 28, 20 ) );
    w.CommitDragTest( );
    CHECK_EQ( veh.m_route.GetCount( ), 0 );
    CHECK_EQ( veh.m_nNextOrder, 0 );
}

// =========================================================================== lint
static std::string Slurp( const char* path, bool& bOk )
{
    bOk = false;
    std::FILE* f = std::fopen( path, "rb" );
    if ( f == nullptr )
        return ( std::string( ) );
    std::string s;
    char        buf[65536];
    size_t      n;
    while ( ( n = std::fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        s.append( buf, n );
    std::fclose( f );
    bOk = true;
    // the source is CRLF on disk; the multi-line markers below are written LF
    std::string out;
    out.reserve( s.size( ) );
    for ( size_t i = 0; i < s.size( ); ++i )
        if ( s[i] != '\r' )
            out += s[i];
    return ( out );
}

static void lint_needs( std::string const& src, const char* what, const char* desc )
{
    ++microtest::Checks( );
    if ( src.find( what ) == std::string::npos )
    {
        ++microtest::Fails( );
        std::printf( "FAIL lint: %s -- not found: %s\n", desc, what );
    }
}

static void lint_forbids( std::string const& src, const char* what, const char* desc )
{
    ++microtest::Checks( );
    if ( src.find( what ) != std::string::npos )
    {
        ++microtest::Fails( );
        std::printf( "FAIL lint: %s -- still present: %s\n", desc, what );
    }
}

// The extraction itself, pinned the way test_order_serialize.cpp pins the serialize
// model: these are the lines no fixture can see, because they live in the WIRING.
static int test_source_lint( const char* areaCpp, const char* areaH )
{
    bool        bOk = false;
    std::string s   = Slurp( areaCpp, bOk );
    if ( !bOk )
    {
        std::printf( "[orders] SKIP drag-place lint (cannot open %s)\n", areaCpp );
        return ( 0 );
    }

    // the hover goes through the SHARED verdict ...
    lint_needs( s, "BuildSiteVerdict( _hexBuild, m_iBuild, GetBuildDir( ), bBuildOk, v );",
                "the hover calls the shared verdict" );
    lint_needs( s, "m_iFound = v.m_iFound;", "and takes its m_iFound from it" );
    lint_needs( s, "int iCurType = v.m_iCurType;", "and its cursor type" );
    // ... and does NOT keep a second copy of the rules
    lint_forbids( s, "theMap.FoundationCost( _hexBuild",
                  "the hover no longer calls FoundationCost itself" );
    lint_forbids( s, "int                   iMul = CFarmBuilding::LandMult(",
                  "nor measures the farm land itself" );
    lint_forbids( s, "qMul  = CMineBuilding::TotalQuantity( _hexBuild",
                  "nor the mine quantity" );
    lint_forbids( s, "CMineBuilding::TotalDensity( _hexBuild", "nor the mine density" );
    // the verdict is the ONLY caller of each rule primitive
    lint_needs( s, "v.m_iFound = theMap.FoundationCost( hexUL, iBuild, iDir, NULL, NULL, &v.m_iWhy );",
                "the verdict owns the FoundationCost call" );
    lint_needs( s, "v.m_iMul  = CFarmBuilding::LandMult( hexUL, iBuild, iDir );",
                "the verdict owns the land measurement" );

    // the drag's own wiring
    lint_needs( s, "static int BuildDragLine(", "the line generator exists" );
    lint_needs( s, "int dx = CHexCoord::Diff( hexTo.X( ) - hexFrom.X( ) );",
                "the line's deltas are wrap-aware" );
    lint_needs( s, "if ( abs( dx ) >= abs( dy ) )", "the dominant axis ties to X" );
    lint_needs( s, "BuildSiteVerdict( m_ahexDrag[iOn], m_iBuild, iDir, TRUE, v );",
                "every site on the line goes through the SAME verdict" );
    lint_needs( s, "m_iDragCx      = ( iDir & 1 ) ? pData->GetCY( ) : pData->GetCX( );",
                "the spacing uses the dir-swapped footprint" );
    lint_needs( s, "if ( HasMoveStops( pVehBuild ) )\n                StopRoute( pVehBuild );",
                "the commit takes a movement route over" );
    lint_needs( s, "EndBuildDrag( );   // the preview comes off now the orders carry their own ghosts\n"
                   "            pVehBuild->NextOrder( );",
                "one NextOrder after the appends, and the gesture ends" );
    lint_needs( s, "const BOOL bDragBuild = bQueueBuild && m_bBuildDrag && ( m_nDragSites > 1 );",
                "a one-site drag is a plain Shift-place" );
    lint_needs( s, "if ( ( m_iFound < 0 ) && ( !bDragBuild ) )",
                "a drag is not failed by its end site" );
    // every way out of the gesture
    lint_needs( s, "EndBuildDrag( );   // #38 drag-place is a drag gesture too",
                "losing activation drops the drag" );
    lint_needs( s, "EndBuildDrag( );   // #38: a refused placement ends the gesture with it",
                "a refused placement drops the drag" );
    lint_needs( s, "EndBuildDrag( );   // a press never inherits the previous gesture's line",
                "a press starts clean" );
    lint_needs( s, "EndBuildDrag( );   // #38: leaving placement mode",
                "BldgCurOff (Esc / deselect / SelectOff) drops the drag" );
    lint_needs( s, "if ( ( m_iMode != build_loc ) || ( !( nFlags & MK_LBUTTON ) ) || ( !( nFlags & MK_SHIFT ) ) )",
                "the drag lives only while the button and Shift are held" );

    s = Slurp( areaH, bOk );
    if ( !bOk )
    {
        std::printf( "[orders] SKIP area.h lint (cannot open %s)\n", areaH );
        return ( 0 );
    }
    lint_needs( s, "const int MAX_DRAG_PLACE = 64;", "the site cap" );
    lint_needs( s, "int\t\t\tGetDragPlaceCount () const { return ((m_iMode == build_loc) && m_bBuildDrag ? m_nDragSites : 0); }",
                "the preview is published only while a drag is live in build_loc" );
    lint_needs( s, "CHexCoord\t\t\tm_ahexDrag[MAX_DRAG_PLACE];",
                "the sites live in a fixed array (the preview pass allocates nothing)" );
    lint_needs( s, "int\t\tGetBuildDir () const { return ((m_aa.m_iDir + m_iBuildDir) & 3); }",
                "GetBuildDir is what this suite mirrors" );
    return ( 0 );
}

int main( int argc, char** argv )
{
    test_line_spacing_per_axis( );
    test_line_direction( );
    test_line_dominant_axis( );
    test_line_crosses_the_seam( );
    test_line_degenerate( );

    test_verdict_matches_the_1996_rules( );
    test_verdict_measures_each_hex_once( );
    test_verdict_overrides_reject_the_site( );

    test_line_judges_every_site( );
    test_line_anchors_on_the_footprint_ul( );

    test_commit_appends_in_drag_order( );
    test_commit_all_sites_bad_queues_nothing( );
    test_commit_takes_a_movement_route_over( );
    test_commit_is_skipped_when_it_is_not_a_drag( );

    if ( argc >= 3 )
        test_source_lint( argv[1], argv[2] );
    else
        std::printf( "[orders] (drag-place source lint skipped)\n" );

    return ( microtest::Summary( ) );
}
