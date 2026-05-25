//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------



#include "stdafx.h"
#include "_windwrd.h"
#include "w22_settings.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "blt.h"

typedef HDC     ( WINGAPI* WINGCREATEDC_FUNC )           ();
typedef BOOL    ( WINGAPI* WINGRECOMMENDDIBFORMAT_FUNC ) ( BITMAPINFO* );
typedef HBITMAP ( WINGAPI* WINGCREATEBITMAP_FUNC )       ( HDC, BITMAPINFO*, void** );
typedef void*   ( WINGAPI* WINGGETDIBPOINTER_FUNC )      ( HBITMAP, BITMAPINFO* );
typedef UINT    ( WINGAPI* WINGSETDIBCOLORTABLE_FUNC )   ( HDC, UINT, UINT, RGBQUAD const* );
typedef BOOL    ( WINGAPI* WINGBITBLT_FUNC )             ( HDC, int, int, int, int, HDC, int, int );
typedef BOOL    ( WINGAPI* WINGSTRETCHBLT_FUNC )         ( HDC, int, int, int, int, HDC, int, int, int, int );

//
// 3/14/96 - BobP
// we install direct draw, but use this in lieue of a COM class factory create,
// in case it didn't get installed correctly
//

static WINGCREATEDC_FUNC            pfnWinGCreateDC            = nullptr;
static WINGRECOMMENDDIBFORMAT_FUNC  pfnWinGRecommendDIBFormat  = nullptr;
static WINGCREATEBITMAP_FUNC        pfnWinGCreateBitmap        = nullptr;
static WINGGETDIBPOINTER_FUNC       pfnWinGGetDIBPointer       = nullptr;
static WINGSETDIBCOLORTABLE_FUNC    pfnWinGSetDIBColorTable    = nullptr;
static WINGBITBLT_FUNC              pfnWinGBitBlt              = nullptr;
static WINGSTRETCHBLT_FUNC          pfnWinGStretchBlt          = nullptr;

Ptr< CWinG >        ptrtheWinG;
Ptr< CBLTFormat >   ptrthebltformat;

//--------------------------- C B L T F o r m a t ---------------------------

//---------------------------------------------------------------------------
// CBltFormat::CBltFormat - Use screen color format
//---------------------------------------------------------------------------
CBLTFormat::CBLTFormat()
{
    Init();
}

//---------------------------------------------------------------------------
// CBltFormat::CBltFormat - Use specified color format
//---------------------------------------------------------------------------
CBLTFormat::CBLTFormat(
    CColorFormat const & colorformat )
    :
        m_colorformat( colorformat )
{
    Init();
}

//---------------------------------------------------------------------------
// CBltFormat::Init
//---------------------------------------------------------------------------
BOOL
CBLTFormat::Init()
{
    m_eType   = CalcBltMethod();
    m_eDirection   = DIR_BOTTOMUP;

    if ( DIB_DIBSECTION == m_eType )
        m_eDirection  = DIR_TOPDOWN;

    // BUGBUG: profile to get best dib direction (etc.)
    // BUGBUG: exception on failure?
    // BUGBUG: WinG supports only 8bpp under Win32s?

    if ( DIB_WING == m_eType )
        if ( !CWinG::GetTheWinG() )
            if ( W32s == iWinType )
                m_eType = DIB_MEMORY;
            else
                m_eType = DIB_DIBSECTION;

    if ( DIB_DIBSECTION == m_eType )
        m_eDirection  = DIR_TOPDOWN;

    // Phase 6 Stage 2: SDL_Surface storage is always top-down (SDL convention),
    // same as the DDraw and DIB section paths above. Without this forcing,
    // every primary-format CDIB inherits DIR_BOTTOMUP, GetRow()/GetOffset()
    // flip every write into memory, and the game renders upside-down
    // (verified empirically when BLT=5 was first tested).
    if ( DIB_SDL_SURFACE == m_eType )
        m_eDirection  = DIR_TOPDOWN;

    // check dir for WinG
    if ( DIB_WING == m_eType )
        {
        BITMAPINFO bmi;
        memset ( &bmi, 0, sizeof (bmi) );
        bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        ptrtheWinG.Value ()->RecommendDIBFormat ( &bmi );
        if ( bmi.bmiHeader.biHeight < 0 )
            m_eDirection = DIR_TOPDOWN;
        else
            m_eDirection = DIR_BOTTOMUP;
        }

    int iDir = w22::GetProfileInt( "Advanced", "Direction", m_eDirection );
    if ( (iDir != m_eDirection) && ( abs (iDir) == 1) )
        if ( (iWinType != W32s) || (m_eType != DIB_MEMORY) )
            m_eDirection = (DIB_DIRECTION) iDir;

    w22::WriteProfileInt( "Advanced", "BLTUsed", 1 + m_eType );
    w22::WriteProfileInt( "Advanced", "DirUsed", m_eDirection );
    w22::WriteProfileInt( "Advanced", "DepthUsed", GetBitsPerPixel() );

    ASSERT_STRICT_VALID( this );

    return TRUE;
}

//---------------------------------------------------------------------------
// CBltFormat::CalcBltMethod
//---------------------------------------------------------------------------
CBLTFormat::DIB_TYPE
CBLTFormat::CalcBltMethod()
{
    int iType = w22::GetProfileInt( "Advanced", "BLT", 0 );
    iType = __max ( 0, iType );
    iType = __min ( DIB_NUM_TYPES, iType );

    if ( 0 == iType )
        ; // FIXIT: If "fastest" selected, test all methods

    DIB_TYPE eType = ( DIB_TYPE )( iType - 1 );

    switch ( iWinType )
    {
        case W32s: // WinG, SetDIBitsToDevice

            if ( DIB_WING != eType )
                eType = DIB_MEMORY;

            break;

        case W95:
        case WNT:

            if ( 0 == iType ) {
                // Phase 6 Stage 3+4: SDL_Surface is the default backing.
                // The DDraw path is gone. The BLT registry value still
                // works for diagnostics: 1=DIB_WING, 2=DIB_DIBSECTION,
                // 3=DIB_MEMORY, 4=DIB_SDL_SURFACE. None of 1-3 are
                // exercised in the live game today; 4 is redundant with
                // the default but pins the choice unambiguously.
                eType = DIB_SDL_SURFACE;
            }

            break;

        default: ASSERT_STRICT( 0 );
    }

    // SDL_SURFACE allocates 32-bit RGB888 explicitly via SDL, regardless of
    // screen depth, so no bpp-mismatch fallback is needed.

    return eType;
}

CBLTFormat::DIB_DIRECTION CBLTFormat::GetMemDirection () const
{

    // set DIBBits.. always bottom up
    if ( DIB_MEMORY == m_eType )
        return DIR_BOTTOMUP;

    // NT/95 can handle top-down
    if ( iWinType != W32s ) 
        return m_eDirection;

    // 3.1 cannot
    return DIR_BOTTOMUP;
}

//-------------------------------------------------------------------------
// CBltFormat::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CBLTFormat::AssertValid() const
{
}
#endif

//-------------------------------- C W i n G -------------------------------

//-------------------------------------------------------------------------
// CWinG::GetTheWinG
//-------------------------------------------------------------------------
CWinG *
CWinG::GetTheWinG()
{
    if ( !ptrtheWinG.Value() )
    {
        ptrtheWinG = new CWinG;

        if ( !ptrtheWinG->IsValid() )
            ptrtheWinG = NULL;
    }

    return ptrtheWinG.Value();
}

//-------------------------------------------------------------------------
// CWinG::CWinG
//-------------------------------------------------------------------------
CWinG::CWinG()
{
    UINT uOld = ::SetErrorMode ( SEM_NOOPENFILEERRORBOX );
    m_hInstLib = LoadLibrary( "wing32" );
    SetErrorMode ( uOld );

    if ( m_hInstLib )
    {
        pfnWinGCreateDC      = ( WINGCREATEDC_FUNC     )GetProcAddress( m_hInstLib, "WinGCreateDC"           );
        pfnWinGRecommendDIBFormat = ( WINGRECOMMENDDIBFORMAT_FUNC )GetProcAddress( m_hInstLib, "WinGRecommendDIBFormat" );
        pfnWinGCreateBitmap       = ( WINGCREATEBITMAP_FUNC   )GetProcAddress( m_hInstLib, "WinGCreateBitmap"       );
        pfnWinGGetDIBPointer      = ( WINGGETDIBPOINTER_FUNC   )GetProcAddress( m_hInstLib, "WinGGetDIBPointer"      );
        pfnWinGSetDIBColorTable   = ( WINGSETDIBCOLORTABLE_FUNC   )GetProcAddress( m_hInstLib, "WinGSetDIBColorTable"  );
        pfnWinGBitBlt      = ( WINGBITBLT_FUNC     )GetProcAddress( m_hInstLib, "WinGBitBlt"       );
        pfnWinGStretchBlt     = ( WINGSTRETCHBLT_FUNC    )GetProcAddress( m_hInstLib, "WinGStretchBlt"      );

        if ( pfnWinGCreateDC     && 
              pfnWinGRecommendDIBFormat && 
              pfnWinGCreateBitmap    && 
              pfnWinGGetDIBPointer    && 
              pfnWinGSetDIBColorTable  && 
              pfnWinGBitBlt      &&
              pfnWinGStretchBlt )

            SetValid( TRUE );
    }

    ASSERT_STRICT_VALID( this );
}

//-------------------------------------------------------------------------
// CWinG::~CWinG
//-------------------------------------------------------------------------
CWinG::~CWinG() {
    if ( m_hInstLib )
        FreeLibrary( m_hInstLib );
}

//-------------------------------------------------------------------------
// CWinG::CreateDC
//-------------------------------------------------------------------------
HDC
CWinG::CreateDC()
{
    ASSERT_STRICT_VALID( this );

    return pfnWinGCreateDC();
}

//-------------------------------------------------------------------------
// CWinG::RecommendDIBFormat
//-------------------------------------------------------------------------
BOOL
CWinG::RecommendDIBFormat(
    BITMAPINFO *pbitmapinfo )
{
    ASSERT_STRICT_VALID( this );

    return pfnWinGRecommendDIBFormat( pbitmapinfo );
}

//-------------------------------------------------------------------------
// CWinG::CreateBitmap
//-------------------------------------------------------------------------
HBITMAP
CWinG::CreateBitmap(
    HDC    hdc,
    BITMAPINFO * pbitmapinfo,
    void      ** ppv )
{
    ASSERT_STRICT_VALID( this );

    return pfnWinGCreateBitmap( hdc, pbitmapinfo, ppv );
}

//-------------------------------------------------------------------------
// CWinG::GetDIBPointer
//-------------------------------------------------------------------------
void* CWinG::GetDIBPointer( HBITMAP hbitmap, BITMAPINFO* pbitmapinfo ) {
    ASSERT_STRICT_VALID( this );

    return pfnWinGGetDIBPointer( hbitmap, pbitmapinfo );
}

//-------------------------------------------------------------------------
// CWinG::SetDIBColorTable
//-------------------------------------------------------------------------
UINT CWinG::SetDIBColorTable( HDC hdc, UINT uStart, UINT uNum, RGBQUAD const * prgbquad )
{
    ASSERT_STRICT_VALID( this );

    return pfnWinGSetDIBColorTable( hdc, uStart, uNum, prgbquad );
}

//-------------------------------------------------------------------------
// CWinG::BltBlt
//-------------------------------------------------------------------------
BOOL CWinG::BitBlt( HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc ) {
    ASSERT_STRICT_VALID( this );

    return pfnWinGBitBlt( hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc );
}

//-------------------------------------------------------------------------
// CWinG::StretchBlt
//-------------------------------------------------------------------------
BOOL CWinG::StretchBlt( HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc ) {
    return pfnWinGStretchBlt(  hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc );
}

//-------------------------------------------------------------------------
// CWinG::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CWinG::AssertValid() const
{
    CBLTBase::AssertValid();

    ASSERT_STRICT( m_bValid );
    ASSERT_STRICT( m_hInstLib );
    ASSERT_STRICT( pfnWinGCreateDC );
    ASSERT_STRICT( pfnWinGRecommendDIBFormat );
    ASSERT_STRICT( pfnWinGCreateBitmap );
    ASSERT_STRICT( pfnWinGGetDIBPointer );
    ASSERT_STRICT( pfnWinGSetDIBColorTable );
    ASSERT_STRICT( pfnWinGBitBlt );
    ASSERT_STRICT( pfnWinGStretchBlt );
}
#endif

//-------------------------- C C o l o r F o r m a t ----------------------
//
// Calc/store screen device bits/bytes-per-pixel

//-------------------------------------------------------------------------
// CColorFormat::CColorFormat
//-------------------------------------------------------------------------
CColorFormat::CColorFormat( CColorFormat::COLOR_DEPTH eDepth ) {
    m_iBitsPerPixel = int( eDepth );

    CalcBytesPerPixel();
}

//-------------------------------------------------------------------------
// CColorFormat::CalcBytesPerPixel
//-------------------------------------------------------------------------
void CColorFormat::CalcBytesPerPixel() {
    m_iBytesPerPixel = ( m_iBitsPerPixel + 7 ) >> 3;
}

//-------------------------------------------------------------------------
// CColorFormat::CalcScreenFormat
//-------------------------------------------------------------------------
void CColorFormat::CalcScreenFormat() {
    HDC hdc = GetDC( NULL );

    if ( !hdc ) {
        m_iBitsPerPixel = 0;
        m_iBytesPerPixel = 0;

        return;
    }

    int iPlanes = ::GetDeviceCaps( hdc, PLANES );
    int iBits   = ::GetDeviceCaps( hdc, BITSPIXEL );

    m_iBitsPerPixel = iPlanes * iBits;

    if ( 16 == m_iBitsPerPixel && 0x0000ffff > ::GetDeviceCaps( hdc, NUMCOLORS ))
        m_iBitsPerPixel = 15;

    CalcBytesPerPixel();

    ::ReleaseDC ( NULL, hdc );
}

//-------------------------------------------------------------------------
// CColorFormat::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CColorFormat::AssertValid() const
{

    ASSERT_STRICT(  8 == m_iBitsPerPixel ||
              15 == m_iBitsPerPixel ||
              16 == m_iBitsPerPixel ||
              24 == m_iBitsPerPixel ||
              32 == m_iBitsPerPixel );

    ASSERT_STRICT(( m_iBitsPerPixel + 7 ) >> 3 == m_iBytesPerPixel );
    ASSERT_STRICT( 1 <= m_iBytesPerPixel && m_iBytesPerPixel <= 4 );
}

#endif


