//---------------------------------------------------------------------------
//
// mfc_compat_text.cpp — Phase 6 Stage 5 (Phase C)
//
// SDL_ttf-backed text rendering for CDC text methods, when the CDC is
// drawing into a CDIB whose backing is an SDL_Surface (i.e. the Phase 6
// Stage 2+3 default). See mfc_compat_text.h for the contract.
//
// Out of scope: CDC text on a Win32 window HDC (loading splash, credits).
// Those keep the existing GDI path because there's no SDL_Surface
// registered for the HDC.
//
// Font handling: minimal. CDC::SelectObject(CFont*) cannot trivially be
// mapped to SDL_ttf (Windows LOGFONT vs. TTF file paths), so this layer
// uses a fixed-face TTF (arial.ttf, with tahoma/segoeui fallbacks per the
// SDL2Panel pattern) and varies only the pixel size. CDC's SelectObject
// mirrors the LOGFONT pixel height via Wind22_SetFontHeight; if no font
// was selected we default to 14px. Visual fidelity is approximate; tighter
// font matching is a follow-up if it bites.
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "en_logpath.h"   // EnLogPath - logs to the launch dir, not the exe dir
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include "_windwrd.h"
#include "mfc_compat_text.h"
#include "dib.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <unordered_map>
#include <string>

const char* EnResolveFontPath();   // defined below, OUTSIDE the anonymous
                                   // namespace so the game layer can link to it.

namespace {

struct HdcState {
    CDIB*     pdib       = nullptr;
    COLORREF  textColor  = RGB( 0, 0, 0 );
    COLORREF  bkColor    = RGB( 255, 255, 255 );
    int       bkMode     = TRANSPARENT;
    int       fontHeight = 0;  // 0 => use default
};

// Global state. wind22 is single-threaded for paint operations (the SDL2
// composition runs on the main thread); a plain map is fine.
std::unordered_map<HDC, HdcState>* g_pMap = nullptr;

std::unordered_map<HDC, HdcState>& GetMap() {
    if ( !g_pMap )
        g_pMap = new std::unordered_map<HDC, HdcState>();
    return *g_pMap;
}

HdcState* FindState( HDC hdc ) {
    auto& m = GetMap();
    auto it = m.find( hdc );
    return ( it == m.end() ) ? nullptr : &it->second;
}

// --- Font cache --------------------------------------------------------------
// Mirrors the SDL2Panel font-loading pattern. Cached per pixel size.

const char* PickFontPath() {
    return EnResolveFontPath();
}

TTF_Font* GetFont( int pixelHeight ) {
    if ( pixelHeight <= 0 )
        pixelHeight = 14;
    // Sanity clamp — gameplay HUD text shouldn't be larger than this.
    if ( pixelHeight > 96 )
        pixelHeight = 96;

    static std::unordered_map<int, TTF_Font*>* s_pCache = nullptr;
    if ( !s_pCache )
        s_pCache = new std::unordered_map<int, TTF_Font*>();

    auto it = s_pCache->find( pixelHeight );
    if ( it != s_pCache->end() )
        return it->second;

    const char* path = PickFontPath();
    if ( !path )
        return nullptr;

    // TTF_Init is idempotent and reference-counted; safe to call lazily.
    static bool s_ttfInited = false;
    if ( !s_ttfInited ) {
        if ( TTF_Init() < 0 ) {
            OutputDebugStringA( "Wind22 TTF_Init failed\n" );
            return nullptr;
        }
        s_ttfInited = true;
    }

    TTF_Font* font = TTF_OpenFont( path, pixelHeight );
    ( *s_pCache )[pixelHeight] = font;  // cache even nullptr to avoid retry storms
    return font;
}

inline SDL_Color ToSDL( COLORREF cr ) {
    SDL_Color c;
    c.r = GetRValue( cr );
    c.g = GetGValue( cr );
    c.b = GetBValue( cr );
    c.a = 255;
    return c;
}

// Render `text` into a fresh SDL_Surface honoring the bk mode/color.
SDL_Surface* RenderToSurface( TTF_Font* font, const char* text,
                              COLORREF textColor, COLORREF bkColor, int bkMode,
                              int wrapWidth /* 0 = no wrap */ )
{
    if ( !font || !text || !*text )
        return nullptr;

    SDL_Color fg = ToSDL( textColor );

    if ( wrapWidth > 0 ) {
        // Wrapped paths always use blended (no shaded-wrapped exists in
        // older SDL_ttf revisions; we get bg via a fill behind the blit
        // if bkMode == OPAQUE).
        return TTF_RenderText_Blended_Wrapped( font, text, fg, (Uint32)wrapWidth );
    }
    if ( bkMode == OPAQUE ) {
        SDL_Color bg = ToSDL( bkColor );
        return TTF_RenderText_Shaded( font, text, fg, bg );
    }
    return TTF_RenderText_Blended( font, text, fg );
}

// Compute single-line text size; returns true on success.
bool MeasureLine( TTF_Font* font, const char* text, int* pcx, int* pcy ) {
    if ( !font )
        return false;
    int w = 0, h = 0;
    if ( TTF_SizeText( font, text ? text : "", &w, &h ) != 0 )
        return false;
    if ( pcx ) *pcx = w;
    if ( pcy ) *pcy = h;
    return true;
}

// Owned copy of (psz, nLen) as a null-terminated std::string. Handles
// nLen == -1 (caller passed a C-string).
std::string ToString( LPCSTR psz, int nLen ) {
    if ( !psz )
        return std::string();
    if ( nLen < 0 )
        return std::string( psz );
    return std::string( psz, (size_t)nLen );
}

}  // namespace

// ---------------------------------------------------------------------------
// EnResolveFontPath — the ONE font resolver for the whole build (T-0073).
//
// Lives in wind22 because BOTH layers need it: the SDL2 UI in enations_latest AND
// this compat text renderer. It previously existed only in the upper layer, so
// PickFontPath() below kept its own Debian-only list and stayed broken on other
// distros even after the upper-layer fix — found by linux2 on real Void Linux.
//
// Order: the platform's own system fonts FIRST (every platform keeps the exact
// appearance it had; a wider face can truncate text laid out for a narrow one),
// then the bundled copy beside the executable as a LAST RESORT for a host that
// carries none of them. "/usr/share/fonts/truetype/<family>/" is a DEBIAN layout,
// not a Linux one: Fedora, Arch and openSUSE keep the very same fonts elsewhere,
// and looking only there rendered NO TEXT AT ALL on those systems.
// Resolved once and logged, so a "no text" report is self-diagnosing.
// ---------------------------------------------------------------------------
const char* EnResolveFontPath() {
    static std::string s_path;
    static bool        s_resolved = false;
    if ( s_resolved )
        return s_path.c_str();
    s_resolved = true;

    std::vector<std::string> cand;
    static const char* kSystem[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",                 // Debian/Ubuntu
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", // Debian/Ubuntu
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",               // Fedora/RHEL
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",                          // Fedora (alt)
        "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",     // Fedora
        "/usr/share/fonts/TTF/DejaVuSans.ttf",                             // Arch, Void
        "/usr/share/fonts/truetype/DejaVuSans.ttf",                        // openSUSE (flat)
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",                      // Noto-only systems
        "/System/Library/Fonts/Supplemental/Arial.ttf",                    // macOS
        "/System/Library/Fonts/Supplemental/Times New Roman.ttf",          // macOS
        "C:\\Windows\\Fonts\\arial.ttf",                                   // Windows
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        nullptr };
    for ( int i = 0; kSystem[i]; ++i )
        cand.push_back( kSystem[i] );
    if ( char* base = SDL_GetBasePath() ) {          // bundled: LAST RESORT
        std::string b( base ); SDL_free( base );
        cand.push_back( b + "res/DejaVuSans.ttf" );
        cand.push_back( b + "DejaVuSans.ttf" );
    }

    for ( size_t k = 0; k < cand.size(); ++k ) {
        FILE* f = fopen( cand[k].c_str(), "rb" );
        if ( f ) { fclose( f ); s_path = cand[k]; break; }
    }
    // Report to BOTH stderr and GameWindow_Debug.log. stderr alone is not enough:
    // a player launching from a desktop icon has no terminal, and "grep FONT: in
    // GameWindow_Debug.log" is the check other nodes already use to tell a fixed
    // build from a broken one (linux2 used its absence as proof the published zip
    // lacked this fix). Moving the resolver into wind22 must not cost that.
    {
        const std::string msg = s_path.empty()
            ? std::string( "FONT: no usable font found - UI TEXT WILL NOT RENDER" )
            : ( "FONT: using " + s_path );
        fprintf( stderr, "%s\n", msg.c_str() );
        std::ofstream log( EnLogPath( "GameWindow_Debug.log" ).c_str(), std::ios::app );
        if ( log.is_open() )
            log << msg << std::endl;
    }
    return s_path.c_str();
}

// Kept as the compat layer's entry point; the list it used to own was Debian-only.


//---------------------------------------------------------------------------
// Public API
//---------------------------------------------------------------------------

void Wind22_RegisterDibForHdc( HDC hdc, CDIB* pdib ) {
    if ( !hdc || !pdib )
        return;
    HdcState& s = GetMap()[hdc];
    s.pdib = pdib;
    // textColor / bkColor / bkMode / fontHeight retain prior values across
    // GetDC/ReleaseDC cycles; the CDC re-pushes them via the setters
    // before each draw if it cares.
}

void Wind22_UnregisterDibForHdc( HDC hdc ) {
    if ( !hdc )
        return;
    auto& m = GetMap();
    auto it = m.find( hdc );
    if ( it != m.end() )
        m.erase( it );
}

bool Wind22_HdcHasSdlSurface( HDC hdc ) {
    HdcState* st = FindState( hdc );
    return st && st->pdib && st->pdib->GetSDLSurface() != nullptr;
}

void Wind22_SetTextColor( HDC hdc, COLORREF cr ) {
    if ( HdcState* st = FindState( hdc ) ) st->textColor = cr;
}
void Wind22_SetBkColor( HDC hdc, COLORREF cr ) {
    if ( HdcState* st = FindState( hdc ) ) st->bkColor = cr;
}
void Wind22_SetBkMode( HDC hdc, int mode ) {
    if ( HdcState* st = FindState( hdc ) ) st->bkMode = mode;
}
void Wind22_SetFontHeight( HDC hdc, int absHeight ) {
    if ( HdcState* st = FindState( hdc ) ) st->fontHeight = absHeight;
}

BOOL Wind22_SDLTextExtent( HDC hdc, LPCSTR psz, int nLen, int* pcx, int* pcy ) {
    HdcState* st = FindState( hdc );
    if ( !st || !st->pdib || !st->pdib->GetSDLSurface() )
        return FALSE;

    TTF_Font* font = GetFont( st->fontHeight );
    if ( !font )
        return FALSE;

    std::string s = ToString( psz, nLen );
    return MeasureLine( font, s.c_str(), pcx, pcy ) ? TRUE : FALSE;
}

BOOL Wind22_SDLTextOut( HDC hdc, int x, int y, LPCSTR psz, int nLen ) {
    HdcState* st = FindState( hdc );
    if ( !st || !st->pdib )
        return FALSE;
    SDL_Surface* dst = st->pdib->GetSDLSurface();
    if ( !dst )
        return FALSE;

    TTF_Font* font = GetFont( st->fontHeight );
    if ( !font )
        return FALSE;

    std::string s = ToString( psz, nLen );
    if ( s.empty() )
        return TRUE;

    SDL_Surface* textSurf = RenderToSurface(
        font, s.c_str(), st->textColor, st->bkColor, st->bkMode, /*wrap*/ 0 );
    if ( !textSurf )
        return FALSE;

    SDL_Rect dstRect = { x, y, textSurf->w, textSurf->h };
    SDL_BlitSurface( textSurf, nullptr, dst, &dstRect );
    SDL_FreeSurface( textSurf );
    return TRUE;
}

BOOL Wind22_SDLDrawText( HDC hdc, LPCSTR psz, int nLen, LPRECT pRect, UINT uFormat ) {
    HdcState* st = FindState( hdc );
    if ( !st || !st->pdib )
        return FALSE;
    SDL_Surface* dst = st->pdib->GetSDLSurface();
    if ( !dst || !pRect )
        return FALSE;

    TTF_Font* font = GetFont( st->fontHeight );
    if ( !font )
        return FALSE;

    std::string s = ToString( psz, nLen );

    int rectW = pRect->right  - pRect->left;
    int rectH = pRect->bottom - pRect->top;
    if ( rectW <= 0 )
        rectW = 1;

    bool wantWrap     = ( uFormat & DT_WORDBREAK ) && !( uFormat & DT_SINGLELINE );
    bool calcOnly     = ( uFormat & DT_CALCRECT ) != 0;
    int  alignH       = uFormat & ( DT_CENTER | DT_RIGHT );  // 0 => DT_LEFT
    int  alignV       = uFormat & ( DT_VCENTER | DT_BOTTOM ); // 0 => DT_TOP

    // Render (or measure) the text surface.
    SDL_Surface* textSurf = nullptr;
    int  outW = 0, outH = 0;

    if ( wantWrap ) {
        // Wrapped path: TTF_RenderText_Blended_Wrapped is the only TTF API
        // that does word-break in older SDL_ttf. We render it even for
        // DT_CALCRECT so we get the post-wrap size honestly.
        textSurf = RenderToSurface( font, s.c_str(),
                                    st->textColor, st->bkColor, st->bkMode,
                                    rectW );
        if ( textSurf ) {
            outW = textSurf->w;
            outH = textSurf->h;
        }
    } else {
        if ( s.empty() ) {
            outW = 0;
            outH = TTF_FontHeight( font );
        } else {
            MeasureLine( font, s.c_str(), &outW, &outH );
            if ( !calcOnly )
                textSurf = RenderToSurface( font, s.c_str(),
                                            st->textColor, st->bkColor, st->bkMode,
                                            0 );
        }
    }

    if ( calcOnly ) {
        pRect->right  = pRect->left + outW;
        pRect->bottom = pRect->top  + outH;
        if ( textSurf )
            SDL_FreeSurface( textSurf );
        return TRUE;
    }

    if ( !textSurf )
        return TRUE;  // nothing to paint but the caller succeeded

    int x = pRect->left;
    if ( alignH & DT_CENTER )
        x = pRect->left + ( rectW - outW ) / 2;
    else if ( alignH & DT_RIGHT )
        x = pRect->right - outW;

    int y = pRect->top;
    if ( alignV & DT_VCENTER )
        y = pRect->top + ( rectH - outH ) / 2;
    else if ( alignV & DT_BOTTOM )
        y = pRect->bottom - outH;

    SDL_Rect dstRect = { x, y, outW, outH };
    SDL_BlitSurface( textSurf, nullptr, dst, &dstRect );
    SDL_FreeSurface( textSurf );
    return TRUE;
}
