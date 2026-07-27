#ifndef __MFC_COMPAT_TEXT_H__
#define __MFC_COMPAT_TEXT_H__

//---------------------------------------------------------------------------
//
// mfc_compat_text.h — Phase 6 Stage 5 (Phase C)
//
// SDL_ttf-backed implementations of CDC text methods, for the case where
// the CDC wraps an HDC obtained from CDIB::GetDC(). The CDIB's underlying
// SDL_Surface is the real paint target; this header exposes the helpers
// that CDC::DrawText / TextOut / GetTextExtent call before falling back
// to the legacy ::DrawTextA path.
//
// Registration model: CDIB::GetDC() calls Wind22_RegisterDibForHdc with
// the HDC it just issued and `this`; CDIB::ReleaseDC() calls
// Wind22_UnregisterDibForHdc. CDC text methods then look up the source
// CDIB by HDC at the moment of the draw call.
//
// The Win32 main-window text path (lastplnt.cpp credits, main.cpp loading
// splash) does not go through CDIB::GetDC, so it never registers and
// keeps using the existing GDI path. That's intentional — the splash
// window is initialised before SDL2.
//
//---------------------------------------------------------------------------

#include <windows.h>

class CDIB;

// Called from CDIB::GetDC / ReleaseDC. Idempotent in the sense that a
// double-unregister is a no-op.
void Wind22_RegisterDibForHdc( HDC hdc, CDIB* pdib );
void Wind22_UnregisterDibForHdc( HDC hdc );

// True if hdc has an SDL_Surface backing registered (i.e. came from a
// CDIB::GetDC on a DIB_SDL_SURFACE-typed CDIB).
bool Wind22_HdcHasSdlSurface( HDC hdc );

// Set per-HDC text state. CDC mirrors SetTextColor / SetBkColor /
// SetBkMode into these so the SDL renderer picks them up.
void Wind22_SetTextColor( HDC hdc, COLORREF cr );
void Wind22_SetBkColor( HDC hdc, COLORREF cr );
void Wind22_SetBkMode( HDC hdc, int mode );  // TRANSPARENT or OPAQUE
void Wind22_SetFontHeight( HDC hdc, int absHeight );  // pixel height; <=0 -> default

// Returns TRUE on success (text rendered via SDL_ttf into the registered
// CDIB's SDL_Surface). Returns FALSE if the hdc isn't registered or the
// CDIB has no SDL backing — caller should fall back to ::DrawTextA.
BOOL Wind22_SDLDrawText( HDC hdc, LPCSTR psz, int nLen, LPRECT pRect, UINT uFormat );
BOOL Wind22_SDLTextOut( HDC hdc, int x, int y, LPCSTR psz, int nLen );

// Returns TRUE and fills (*pcx, *pcy) on success; FALSE if no SDL path.
BOOL Wind22_SDLTextExtent( HDC hdc, LPCSTR psz, int nLen, int* pcx, int* pcy );

#endif  // __MFC_COMPAT_TEXT_H__
