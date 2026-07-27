//---------------------------------------------------------------------------
// win32_legacy_stubs.cpp — Linux-only stub definitions for the legacy WinG/
// palette/DIB-format symbols whose source files (apppalet.cpp, blt.cpp) are
// excluded from the Linux build (they bottom out on WinG/GDI). The game still
// references these globals/methods at link time; on the SDL2 render path they
// are dead at runtime, so empty stubs suffice. Excluded from the MSVC build.
//---------------------------------------------------------------------------

#ifdef _WIN32
#error "win32_legacy_stubs.cpp is Linux-only"
#endif

struct BITMAPINFO256;  // fwd-decl (defined in dib.h; games.h only takes it by ref)
#include "games.h"   // CAppPalette, CColorBuffer, thePal
#include "blt.h"     // CWinG, CColorFormat, CBLTFormat, ptrthebltformat, ptrtheWinG

// ---- globals ----
CAppPalette       thePal;
Ptr<CBLTFormat>   ptrthebltformat;
Ptr<CWinG>        ptrtheWinG;

// ---- CAppPalette ----
// NOT no-op stubs: the 256-entry device-color table (m_pdwColors) is live on the
// SDL2 path. GetDeviceColor(int) returns &m_pdwColors[i] and is dereferenced by
// CColorBuffer::FillColor (terrain hatch/cursor draw) — a NULL table crashed the
// area map on mouse-hover. Allocate it here and fill it from the game's loaded
// palette via SetColors (lastplnt.cpp loads a 256-color BMP palette at startup).
CAppPalette::CAppPalette() : m_hPal(NULL), m_hNotActivePal(NULL), m_iNumSys(0),
                             m_bHalf(FALSE), m_bSysClrs(FALSE), m_pdwColors(NULL) {
    m_pdwColors = new DWORD[256];
    for ( int i = 0; i < 256; ++i ) m_pdwColors[i] = 0;
}
CAppPalette::~CAppPalette() { delete[] m_pdwColors; m_pdwColors = NULL; }
BOOL  CAppPalette::Init( BOOL, BOOL ) {
    if ( !m_pdwColors ) m_pdwColors = new DWORD[256];
    return TRUE;
}
void  CAppPalette::Exit() {}
// NOT a stub: this is live on the SDL2 path — CWndWorld::ApplyColors builds the
// radar/world-map terrain color table through it. Returning 0 made the whole
// minimap render black. Mirrors the real CAppPalette::GetColorValue in the
// excluded apppalet.cpp (15/16/24/32-bpp packing). COLORREF is 0x00BBGGRR.
DWORD CAppPalette::GetColorValue( COLORREF colorref, int iBitsPerPixel ) const {
    switch ( iBitsPerPixel ) {
    case 15:
        return (DWORD)( (WORD)( GetRValue( colorref ) >> 3 ) << 10 |
                        (WORD)( GetGValue( colorref ) >> 3 ) << 5  |
                        (WORD)( GetBValue( colorref ) >> 3 ) );
    case 16:
        return (DWORD)( (WORD)( GetRValue( colorref ) >> 3 ) << 11 |
                        (WORD)( GetGValue( colorref ) >> 2 ) << 5  |
                        (WORD)( GetBValue( colorref ) >> 3 ) );
    case 24:
    case 32:
    default:
        return (DWORD)GetRValue( colorref ) << 16 |
               (DWORD)GetGValue( colorref ) << 8  |
               (DWORD)GetBValue( colorref );
    }
}
void  CAppPalette::Activate( HWND, HDC, BOOL ) {}
void  CAppPalette::Paint( HDC ) {}
void  CAppPalette::EndPaint( HDC ) {}
long  CAppPalette::PalMsg( HDC, HWND, UINT, WPARAM, LPARAM ) const { return 0; }
void  CAppPalette::SetColors( RGBQUAD* pRgb, int iFirst, int iNumClrs ) {
    if ( !m_pdwColors || !pRgb ) return;
    for ( int i = 0; i < iNumClrs; ++i ) {
        int idx = iFirst + i;
        if ( idx < 0 || idx >= 256 ) continue;
        // Device color is 0x00RRGGBB to match GetColorValue's 24/32-bpp packing.
        m_pdwColors[idx] = ( (DWORD)pRgb[i].rgbRed   << 16 ) |
                           ( (DWORD)pRgb[i].rgbGreen << 8  ) |
                           ( (DWORD)pRgb[i].rgbBlue        );
    }
}
void  CAppPalette::GetColors( RGBQUAD* pRgb, int iFirst, int iNumClrs ) const {
    if ( !m_pdwColors || !pRgb ) return;
    for ( int i = 0; i < iNumClrs; ++i ) {
        int idx = iFirst + i;
        if ( idx < 0 || idx >= 256 ) continue;
        DWORD c = m_pdwColors[idx];
        pRgb[i].rgbRed   = (BYTE)( ( c >> 16 ) & 0xff );
        pRgb[i].rgbGreen = (BYTE)( ( c >> 8  ) & 0xff );
        pRgb[i].rgbBlue  = (BYTE)(   c         & 0xff );
        pRgb[i].rgbReserved = 0;
    }
}
void  CAppPalette::NewWnd( BITMAPINFO256& ) {}
DWORD CAppPalette::GetDeviceColor( int, int ) const { return 0; }
void  CAppPalette::UpdateDeviceColors( int, int ) {}
void  CAppPalette::SysColors( BOOL, HDC ) {}
#ifdef _DEBUG
void  CAppPalette::AssertValid() const {}
#endif

// ---- CColorBuffer ----
// Real implementation (mirrors the excluded apppalet.cpp): a run of N pixels all
// set to one palette color, used as the source for run-length terrain hatch/cursor
// fills (CSpriteDIB::TerrainDrawQuad memcpy's from GetBuffer()). The previous
// no-op stub left m_pby NULL, so the hatch memcpy read from NULL → SIGSEGV.
CColorBuffer::CColorBuffer( int iIndexColor ) : m_pby(NULL), m_nPixelCount(0),
                             m_nPixelCapacity(0), m_iIndexColor(iIndexColor) {}
CColorBuffer::~CColorBuffer() { delete[] m_pby; }
void CColorBuffer::SetSize( int nPixelCapacity ) {
    m_nPixelCapacity = nPixelCapacity;
    int bpp = ptrthebltformat.Value() ? ptrthebltformat->GetBytesPerPixel() : 4;
    if ( bpp <= 0 ) bpp = 4;
    delete[] m_pby;
    m_pby = new BYTE[ (size_t)m_nPixelCapacity * bpp ];
}
void CColorBuffer::FillColor( int iFirst, int iLast ) {
    int bpp = ptrthebltformat.Value() ? ptrthebltformat->GetBytesPerPixel() : 4;
    if ( bpp <= 0 ) bpp = 4;
    int nCount = iLast - iFirst + 1;
    BYTE* pbyColor = thePal.GetDeviceColor( m_iIndexColor );
    BYTE* pby = m_pby + (size_t)bpp * iFirst;
    for ( int i = 0; i < nCount; ++i, pby += bpp )
        memcpy( pby, pbyColor, bpp );
}
void CColorBuffer::SetColor( int iIndexColor, int nPixels ) {
    if ( nPixels > m_nPixelCapacity ) {
        SetSize( nPixels + ( nPixels >> 1 ) );
        m_iIndexColor = iIndexColor; m_nPixelCount = nPixels;
        if ( nPixels > 0 ) FillColor( 0, nPixels - 1 );
    } else if ( iIndexColor != m_iIndexColor ) {
        m_iIndexColor = iIndexColor; m_nPixelCount = nPixels;
        if ( nPixels > 0 ) FillColor( 0, nPixels - 1 );
    } else if ( nPixels > m_nPixelCount ) {
        FillColor( m_nPixelCount, nPixels - 1 );
        m_nPixelCount = nPixels;
    }
}
BYTE const* CColorBuffer::GetBuffer( int nPixels ) {
    if ( nPixels > m_nPixelCapacity ) {
        SetSize( nPixels + ( nPixels >> 1 ) );
        if ( nPixels > 0 ) FillColor( 0, nPixels - 1 );
        m_nPixelCount = nPixels;
    } else if ( nPixels > m_nPixelCount ) {
        FillColor( m_nPixelCount, nPixels - 1 );
        m_nPixelCount = nPixels;
    }
    return m_pby;
}

// ---- CColorFormat ----
CColorFormat::CColorFormat( COLOR_DEPTH d ) : m_iBitsPerPixel((int)d), m_iBytesPerPixel(((int)d + 7) / 8) {}
void CColorFormat::CalcScreenFormat() { m_iBitsPerPixel = 32; m_iBytesPerPixel = 4; }
void CColorFormat::SetBitsPerPixel( int iBits ) { m_iBitsPerPixel = iBits; CalcBytesPerPixel(); }
void CColorFormat::CalcBytesPerPixel() { m_iBytesPerPixel = (m_iBitsPerPixel + 7) / 8; }
#ifdef _DEBUG
void CColorFormat::AssertValid() const {}
#endif

// ---- CBLTFormat ----
CBLTFormat::CBLTFormat() : m_eType(DIB_SDL_SURFACE), m_eDirection(DIR_TOPDOWN), m_colorformat() {}
CBLTFormat::CBLTFormat( CColorFormat const& cf ) : m_eType(DIB_SDL_SURFACE), m_eDirection(DIR_TOPDOWN), m_colorformat(cf) {}
BOOL CBLTFormat::Init() { return TRUE; }
CBLTFormat::DIB_DIRECTION CBLTFormat::GetMemDirection() const { return DIR_TOPDOWN; }
CBLTFormat::DIB_TYPE CBLTFormat::CalcBltMethod() { return DIB_SDL_SURFACE; }
#ifdef _DEBUG
void CBLTFormat::AssertValid() const {}
#endif

// ---- CWinG ----
CWinG::CWinG() : m_hInstLib(NULL) {}
CWinG::~CWinG() {}
CWinG*  CWinG::GetTheWinG() { return NULL; }    // WinG path dead on Linux/SDL2
HDC     CWinG::CreateDC() { return NULL; }
BOOL    CWinG::RecommendDIBFormat( BITMAPINFO* ) { return FALSE; }
HBITMAP CWinG::CreateBitmap( HDC, BITMAPINFO*, void** ) { return NULL; }
void*   CWinG::GetDIBPointer( HBITMAP, BITMAPINFO* ) { return NULL; }
UINT    CWinG::SetDIBColorTable( HDC, UINT, UINT, RGBQUAD const* ) { return 0; }
BOOL    CWinG::BitBlt( HDC, int, int, int, int, HDC, int, int ) { return FALSE; }
BOOL    CWinG::StretchBlt( HDC, int, int, int, int, HDC, int, int, int, int ) { return FALSE; }
#ifdef _DEBUG
void    CWinG::AssertValid() const {}
#endif
