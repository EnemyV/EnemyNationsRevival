//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __BASE_H__
#define __BASE_H__



#include "ourlog.h"

#include <dibwnd.h>

const int FRAME_RATE = 24;

const int TRUCK_JUMP_TIME = 60 * 6;  // 6 minutes and we jump it

// #define _LOG_LAG
// #define	HACK_TEST_AI 1

const int DEMO_SINGLE_TIME_LIMIT = 90 * 60;
const int DEMO_MULTI_TIME_LIMIT  = 60 * 60;
const int DEMO_RESEARCH_LIMIT    = 6;


#ifdef _DEBUG
const WORD _wDebug = 1;
#else
const WORD  _wDebug = 0;
#endif
#ifdef _CHEAT
const WORD _wCheat = 1;
#else
const WORD  _wCheat = 0;
#endif

const int MAX_NUM_MESSAGES = 50;  // tell other machines to pause when go over this
const int MIN_NUM_MESSAGES = 20;  // tell other machines to unpause when go under this

const int HASH_INIT = 409;

const int NUM_FRAMES_SHOW_HIT = 18;  // world map
const int NUM_FRAMES_SHOW_RES = 24;  // world map

const int MIN_TEXT = 40;  // min number of chars guaranteed to be in a
// text bar

const int MAX_DEF_MULT = 12;  // maximum terrain defense multiplier

const int GAS_PER_ROAD = 5;  // amount of gas used to build 1 road tile

const int MAX_SPAN  = 7;  // max span of a bridge including end pieces
const int MAX_WATER = 5;  // maximum water it can span

const int NUM_SPEEDS    = 17;
const int AVG_SPEED_MUL = 16;

const int TLP_GAME_ID   = 0xFFFF0001;
const int MIN_SIDE_SIZE = 32;
const int MAX_SIDE_SIZE = 128;
const int SLOWEST_SPEED = 128;
const int MAX_CARGO     = 5;

const int MAX_HEX_HT  = 64;
const int STEPS_HEX   = MAX_HEX_HT / 4;
const int FULL_ROT    = STEPS_HEX * 8;
const int EIGHTH_ROT  = FULL_ROT / 8;
const int QUARTER_ROT = FULL_ROT / 8;

const int HEX_HT_MASK     = MAX_HEX_HT - 1;
const int HEX_HT_NOT_MASK = ~HEX_HT_MASK;
const int HEX_HT_PWR      = 6;  // Log base 2 of MAX_HEX_HT
const int HEX_HT_PWR_2    = 2 * HEX_HT_PWR;

const int TERRAIN_HT_SHIFT = 3;  // 4 for Scotland, 3 for Holland

// note - this is all hardcoded in CVehicle::***Spotting
const int MAX_SPOTTING        = 15;
const int SPOTTING_LINE       = ( MAX_SPOTTING + 1 ) * 2;
const int SPOTTING_DW_LINE    = SPOTTING_LINE / 32;
const int SPOTTING_ARRAY_SIZE = ( SPOTTING_LINE * SPOTTING_LINE + 31 ) / 32;

class CPathMgr;
class CCell;
class CSubHex;
class CHexCoord;
class CUnit;
class CMapLoc3D;
class CHex;
class CMapLoc;
class CGameMap;
class CBridgeUnit;

int  MyRand( );
void MySrand( DWORD dwSeed );
int  RandNum( int iMax );

extern BOOL             bcsOk;
extern CRITICAL_SECTION cs;


//------------------------ C V i e w H e x C o o r d -----------------------
//
// Hex in view ccords

class CViewHexCoord : public CPoint
{
  public:
    CViewHexCoord( int iX, int iY ): CPoint( iX, iY ) {}

    CViewHexCoord( const CHexCoord& );  // Convert map hex coords to
                                        // view hex coords
};

//--------------------------- C B i t M a t r i x --------------------------

class CBitMatrix
{

  public:
    CBitMatrix( int cx, int cy );
    virtual ~CBitMatrix( );

    BOOL Get( int iX, int iY ) const;  // Get bit at (x,y)
    void Set( int iX, int iY );        // Set bit (x,y) to 1
    void Reset( int iX, int iY );      // Set bit (x,y) to 0
    void Clear( );                     // Set all bits to 0

  private:
    BYTE* m_pby;
    int   m_nBytes;
    int   m_iPitchBytes;
};

//------------------------ C H e x V a l i d G r i d -----------------------

class CHexValidMatrix : public CBitMatrix
{

  public:
    CHexValidMatrix( int cxPowerOf2, int cyPowerOf2 );

    void SetInvalidated( int iX, int iY ) { Set( iX & m_iMaskX, iY & m_iMaskY ); }

    BOOL IsInvalidated( int iX, int iY ) const { return Get( iX & m_iMaskX, iY & m_iMaskY ); }

  private:
    int m_iMaskX;
    int m_iMaskY;
};

/////////////////////////////////////////////////////////////////////////////
// CHexCoord - which hex (x,y) we are on

class CHexCoord
{
    friend class CCell;
    friend class CPathMgr;
    friend class CSubHex;
    friend class CGameMap;  // for enumHex only!!!
  public:
    CHexCoord( )
    {
#ifdef _DEBUG
        m_iX = m_iY = -1;
#endif
    }
    CHexCoord( int x, int y )
    {
        m_iX = x;
        m_iY = y;
    }
    CHexCoord( CSubHex const& sub );
    CHexCoord( const CViewHexCoord& viewhexcoord, BOOL bRight );
    CHexCoord( CHexCoord const& src )
    {
        m_iX = src.m_iX;
        m_iY = src.m_iY;
    }
    CHexCoord( CMapLoc const& pt );
    CHexCoord( unsigned long val);

    const CHexCoord& operator=( CHexCoord const& src )
    {
        m_iX = src.m_iX;
        m_iY = src.m_iY;
        return ( *this );
    }
    operator unsigned long( ) const { return ( ( ( (unsigned long)m_iX ) << 16 ) | m_iY ); }

    const CHexCoord& operator+=( CHexCoord const& src )
    {
        m_iX += src.m_iX;
        m_iY += src.m_iY;
        Wrap( );
        return ( *this );
    }

    void GetWorldHex( CMapLoc3D amaploc3d[4] ) const;
    void GetHexCorners( CHex* aphex[4] ) const;

    int  X( ) const { return ( m_iX ); }
    int  Y( ) const { return ( m_iY ); }
    int& X( ) { return ( m_iX ); }
    int& Y( ) { return ( m_iY ); }

    inline void X( int _x );  // note: in terrain.inl cause uses theMap
    inline void Y( int _y );
    int         MapX( ) const;
    int         MapY( ) const;

    inline void Xinc( );
    inline void Yinc( );
    inline void Xdec( );
    inline void Ydec( );

    CHexCoord& Wrap( );
    CHexCoord& WrapX( );
    CHexCoord& WrapY( );
    BOOL       SameHex( CSubHex src ) const;

    void Flatten( int iAlt ) const;  // Set 4 corners to same height and invalidate surrounding 3x3 hexes

    static int Wrap( int iVal );
    static int Diff( int iVal );
    static int Dist( CHexCoord const& hex1, CHexCoord const& hex2 )
    {
        int xDif  = abs( Diff( hex1.m_iX - hex2.m_iX ) );
        int yDif  = abs( Diff( hex1.m_iY - hex2.m_iY ) );
        int iDiag = __min( xDif, yDif );
        int iStrt = __max( xDif, yDif ) - iDiag;
        return ( iStrt + iDiag + iDiag / 2 );
    }

    BOOL operator==( CHexCoord src ) const { return ( ( m_iX == src.m_iX ) && ( m_iY == src.m_iY ) ); }
    BOOL operator!=( CHexCoord src ) const { return ( ( m_iX != src.m_iX ) || ( m_iY != src.m_iY ) ); }

    void        SetInvalidated( );
    BOOL        IsInvalidated( ) const;
    static void ClearInvalidated( );

    CHexCoord& ToNearestHex( const CHexCoord& hexcoord );

    friend CArchive& operator<<( CArchive& ar, const CHexCoord hex );
    friend CArchive& operator>>( CArchive& ar, CHexCoord& hex );

  public:
    int m_iX;
    int m_iY;

#ifdef _DEBUG
  public:
    //	virtual void AssertValid() const;
    void AssertStrictValid( ) const;
#endif
};

#ifdef _DEBUG
#define ASSERT_HEX_COORD( p ) ( p )->AssertStrictValid( );
#else
#define ASSERT_HEX_COORD( p )
#endif


/////////////////////////////////////////////////////////////////////////////
// CSubHex - CHex divided into 4 sub hexes

class CSubHex : public CPoint
{
    friend class CHexCoord;

  public:
    CSubHex( )
    {
#ifdef _DEBUG
        x = y = -1;
#endif
    }
    CSubHex( int _x, int _y )
    {
        x = _x;
        y = _y;
    }
    CSubHex( CMapLoc const& pt );
    CSubHex( const CHexCoord& );

    CSubHex& Wrap( );
    CSubHex& WrapX( );
    CSubHex& WrapY( );
    int      Dist( const CSubHex& dest )
    {
        int xDif  = abs( Diff( x - dest.x ) );
        int yDif  = abs( Diff( y - dest.y ) );
        int iDiag = __min( xDif, yDif );
        int iStrt = __max( xDif, yDif ) - iDiag;
        return ( iStrt + iDiag + iDiag / 2 );
    }

    static int Wrap( int iVal );
    static int Diff( int iVal );

    BOOL operator==( CSubHex src ) const { return ( ( x == src.x ) && ( y == src.y ) ); }
    BOOL operator!=( CSubHex src ) const { return ( ( x != src.x ) || ( y != src.y ) ); }

    friend CArchive& operator<<( CArchive& ar, const CSubHex sub );
    friend CArchive& operator>>( CArchive& ar, CSubHex& sub );

    BOOL SameHex( CSubHex src ) const
    {
        return ( ( ( x & ~1 ) == ( src.x & ~1 ) ) && ( ( y & ~1 ) == ( src.y & ~1 ) ) );
    }
    BOOL      SameHex( CHexCoord src ) const { return ( ( x >> 1 == src.m_iX ) && ( y >> 1 == src.m_iY ) ); }
    CHexCoord ToCoord( ) const { return ( CHexCoord( x >> 1, y >> 1 ) ); }

#ifdef _DEBUG
  public:
    virtual void AssertValid( ) const;
#endif
};

//------------------------------ C M a p L o c -----------------------------
//
// Map coordinates

class CMapLoc : public CPoint
{
  public:
    CMapLoc( )
    {
#ifdef _DEBUG
        x = y = -1;
#endif
    }

    CMapLoc( int iX, int iY ): CPoint( iX, iY ) {}
    CMapLoc( CPoint pt ): CPoint( pt ) {}
    CMapLoc( const CHexCoord& rhexcoord );
    CMapLoc( const CSubHex& rsubhex );

    CMapLoc& ToNearest( const CMapLoc& maploc );

    CMapLoc&   Wrap( );
    static int Wrap( int iVal );
    static int Diff( int iVal );

    friend CMapLoc operator+( CMapLoc const& src1, CMapLoc const& src2 )
    {
        return ( CMapLoc( src1.x + src2.x, src1.y + src2.y ) );
    }

    friend CArchive& operator<<( CArchive& ar, const CMapLoc loc );
    friend CArchive& operator>>( CArchive& ar, CMapLoc& loc );

    int HexX( ) const { return x & HEX_HT_NOT_MASK; }
    int HexY( ) const { return y & HEX_HT_NOT_MASK; }
    int HexOffX( ) const { return x & HEX_HT_MASK; }
    int HexOffY( ) const { return y & HEX_HT_MASK; }

    BOOL   IsHexHit( CPoint, CRect& ) const; // second param was CRect& = CRect( )
    Fixed  CalcAltitude( ) const;
    CPoint CalcNormal( ) const;
};

//---------------------------- C M a p L o c 3 D ---------------------------
//
// Global coordinates (map + height)

class CMapLoc3D : public CMapLoc
{
  public:
    CMapLoc3D( )
    {
#ifdef _DEBUG
        m_fixZ = -1;
#endif
    }

    CMapLoc3D( int x, int y, Fixed fixZ ): CMapLoc( x, y ), m_fixZ( fixZ ) {}

    CMapLoc3D( CMapLoc maploc ): CMapLoc( maploc ), m_fixZ( maploc.CalcAltitude( ) ) {}

    CMapLoc3D( CMapLoc maploc, Fixed fixZ ): CMapLoc( maploc ), m_fixZ( fixZ ) {}

    CMapLoc3D& operator=( CMapLoc maploc )
    {
        CMapLoc::operator=( maploc );
        m_fixZ           = maploc.CalcAltitude( );
        return *this;
    }

    CMapLoc3D( const CHexCoord& rhexcoord );

    long&       operator[]( int iIndex ) { return ( &x )[iIndex]; }
    const long& operator[]( int iIndex ) const { return ( &x )[iIndex]; }

    Fixed m_fixZ;
};

//---------------------------- C H i t I n f o ------------------------------
// Result of a hit test
//---------------------------------------------------------------------------

class CVehicle;
class CBuilding;

// Exactly one  of GetUnit() and GetHex() will return a non-NULL pointer.
// If the cursor hits a building or vehicle, then GetUnit() and one of
// GetVehicle() and GetBuilding() will return non-NULL. Otherwise,
// GetHex() returns non-NULL.
//
// GetHexCoord() always returns the corrdinate of the hex on which the cursor
//	is positioned (ie. no matter what was hit.)

class CHitInfo
#ifdef _DEBUG
    : public CObject
#endif
{
  public:
    CHitInfo( const CHexCoord& );
    CHitInfo( const CHitInfo& );  // The compiler won't generate a default copy constructor!

    CUnit* GetUnit( ) const
    {
        ASSERT_STRICT_VALID( this );
        return m_punit;
    }
    CHex* GetHex( ) const
    {
        ASSERT_STRICT_VALID( this );
        return m_phex;
    }
    CBridgeUnit* GetBridge( ) const
    {
        ASSERT_STRICT_VALID( this );
        return m_pbridgeunit;
    }

    CHexCoord _GetHexCoord( ) const
    {
        ASSERT_STRICT_VALID( this );
        return m_hexcoord;
    }  // Not wrapped

    CVehicle* GetVehicle( ) const
    {
        ASSERT_STRICT_VALID( this );
        return m_pvehicle;
    }
    CBuilding* GetBuilding( ) const
    {
        ASSERT_STRICT_VALID( this );
        return m_pbuilding;
    }

    void AssertValid( );

  protected:
    friend class CAnimAtr;

    void SetVehicleHit( CVehicle* );
    void SetBuildingHit( CBuilding* );
    void SetBridgeHit( CBridgeUnit* );

  private:
    CUnit*       m_punit;
    CHex*        m_phex;
    CBridgeUnit* m_pbridgeunit;
    CVehicle*    m_pvehicle;
    CBuilding*   m_pbuilding;
    CHexCoord    m_hexcoord;
};

//---------------------------- C A n i m A t r -----------------------------
//
// Atributes for drawing the map in a window
// because its for drawing in a window it can't have CTile specific
// data like damage or true vehicle direction.

class CAnimAtr
{
  public:
    enum TURN_DIR
    {
        TURN_CLOCKWISE,
        TURN_COUNTERCLOCKWISE
    };
    enum ZOOM_LEVEL
    {
        ZOOM_IN,
        ZOOM_OUT
    };

    CAnimAtr( ): m_pwnd( NULL ), m_maploc( 0, 0 ), m_iDir( -1 ), m_iZoom( -1 ), m_dirtyrects( &m_dibwnd ) {}

    CAnimAtr( CWnd* pwnd, CMapLoc maplocCenter, int iDir, int iZoom ): m_pwnd( pwnd ), m_dirtyrects( &m_dibwnd )
    {
        SetWnd( pwnd );
        Set( maplocCenter, iDir, iZoom );
    }

    ~CAnimAtr( );

    void SetWnd( CWnd* pwnd ) { m_pwnd = pwnd; }
    void Set( CMapLoc maplocCenter, int iDir, int iZoom );

    CMapLoc GetCenter( ) const { return m_maploc; }
    CPoint  GetUL( ) const { return m_ptUL; }

    int  Turn( TURN_DIR );
    int  Zoom( ZOOM_LEVEL );
    void Resized( );

    void Render( );

    enum SET_CENTER_METHOD
    {
        SET_CENTER_SCROLL,
        SET_CENTER_NOP,
        SET_CENTER_INVALIDATE
    };

    const CMapLoc& SetCenter( CMapLoc maploc, SET_CENTER_METHOD eInvalidate );
    const CMapLoc& MoveCenterPixels( int iDelx, int iDely );
    const CMapLoc& MoveCenterViewHexes( int iDelX, int iDelY );
    const CMapLoc& MoveCenterHexes( int iDelX, int iDelY );

    CMapLoc3D WorldToCenterWorld( const CMapLoc3D& maploc3d ) const;
    CPoint    WrapWorldToWindow( const CMapLoc3D& maploc3d ) const
    {
        return WorldToWindow( WorldToCenterWorld( maploc3d ) );
    }

    CPoint    WindowToView( CPoint ) const;
    CMapLoc   WindowToMap( CPoint, Fixed fixZ ) const;
    CHexCoord _WindowToHex( CPoint ) const;
    CHexCoord WindowToHex( CPoint pt ) const
    {
        CHexCoord _hex = _WindowToHex( pt );
        _hex.Wrap( );
        return ( _hex );
    }
    CSubHex WindowToSubHex( CPoint ) const;
    CPoint  WorldToView( const CMapLoc3D& ) const;

    BOOL MapToWindowHex( const CHexCoord&, CPoint[4] ) const;

    BOOL   WorldToWindowHex( CMapLoc3D[4], CPoint[4] ) const;
    CPoint WorldToWindow( CMapLoc3D maploc3d ) const { return ViewToWindow( WorldToView( maploc3d ) ); }

    CPoint  ViewToWindow( CPoint ptView ) const;
    CMapLoc ViewToMap( CPoint pt, Fixed fixZ ) const;

    BOOL     CalcWindowHexBoundClipped( const CHexCoord& hexcoord, CRect& rectResult ) const;
    BOOL     CalcWindowHexBound( const CHexCoord& hexcoord, CRect& rectResult ) const;
    BOOL     IsHexHit( const CHexCoord&, CPoint, CRect& ) const;
    BOOL     IsPtRight( CPoint pt, const CHexCoord& hexcoord ) const;
    CHitInfo GetHit( CPoint ) const;
    void     SetCurrent( ) const;
    void     Scroll( int iDelX, int iDelY );

    CDirtyRects* GetDirtyRects( ) const { return &( (CAnimAtr*)this )->m_dirtyrects; }

    CMapLoc     m_maploc;  // Map coords of center of window
    int         m_iDir;    // direction to render at
    int         m_iZoom;   // zoom
    CDIBWnd     m_dibwnd;  // window we blt into
    CDirtyRects m_dirtyrects;
    CPoint      m_ptUL;  // View coords of UL corner of window
    CWnd*       m_pwnd;  // Owner window

#ifdef _DEBUG
  public:
    virtual void AssertValid( ) const;
    void         CheckPt( CPoint, const CHexCoord& ) const;
#endif
};

// we have 4 types of wheels (so far)
const int NUM_WHEEL_TYPES = 5;
class CWheelTypes
{
  public:
    enum
    {
        walk  = 0,
        wheel = 1,
        track = 2,
        hover = 3,
        water = 4
    };
};

// these are the materials (i'm afraid to use the word resources in a
// windows program) that are mined, farmed, manufactured, etc.
// They are held in an array but should be accessed by ID (for the
// future when existing materials may be removed)

class CMaterialTypes
{
  public:
    enum
    {
        lumber,  // lumber - goods must be 0-based
        steel,
        copper, // Xil
        moly,
        goods,

        food,
        oil,
        gas,
        coal,
        iron,
        num_types,
        num_build_types = food
    };  // goods + 1

    static void ctor( );
    static void dtor( );

    static int            GetNumTypes( ) { return ( num_types ); }
    static int            GetNumBuildTypes( ) { return ( num_build_types ); }
    static CString const& GetDesc( int iInd )
    {
        ASSERT_STRICT( ( 0 <= iInd ) && ( iInd < num_types ) );
        return ( m_saDesc[iInd] );
    }
    static COLORREF GetRGB( int iInd )
    {
        ASSERT_STRICT( ( 0 <= iInd ) && ( iInd <= num_types ) );
        return ( m_rgb[iInd] );
    }

  protected:
    static CString  m_saDesc[num_types];
    static COLORREF m_rgb[num_types + 1];
};


int FastATan( int x, int y );
int GetPrime( int iMin );


#ifdef _CHEAT
void DebugPrintf( char const* pFrmt, ... );
#else
inline void DebugPrintf( char const* pFrmt, ... )
{
}
#endif

#endif
