//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// area.cpp : area window
//

#include "area.h"

#include "bitmaps.h"
#include "bmbutton.h"
#include "bridge.h"
#include "building.inl"
#include "chproute.hpp"
#include "error.h"
#include "event.h"
#include "lastplnt.h"
#include "minerals.inl"
#include "player.h"
#include "relation.h"
#include "edicts.h"       // g_aEdicts / EDICT_COUNT (HarnessDumpEdicts)
#include "altoutput.h"    // AltOutput::Available / AltOutputDef (HarnessDumpAltBuildings)
#include "en_harness.h"   // HarnessDumpUnits (defined at end of file)
#include "sfx.h"
#include "sprite.h"
#include "stdafx.h"
#include "terrain.inl"
#include "ui.inl"
#include "unit.inl"
#include "vehicle.inl"
#include "GameWindow.h"
#include "RenderingAdapter.h"
#include "SDL2AreaBar.h"
#include "SDL2Compositor.h"
#include "SDL2Panel.h"
#include "SDL2RouteWindow.h"
#include "Perf.h"
#include "SDL2GameDialogs.h"
#include "SDL2Dialogs.h"   // SDL2_RunLoadSinglePlayerFlow (HarnessLoadGame)
#include <SDL.h>
#include <SDL_syswm.h>
#include <unordered_map>
#include <vector>

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CAreaList theAreaList;

// Convert SDL keyboard modifier state to MFC nFlags
static UINT SDLModToMFC() {
    UINT flags = 0;
    Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
    if (mouseState & SDL_BUTTON_LMASK) flags |= MK_LBUTTON;
    if (mouseState & SDL_BUTTON_RMASK) flags |= MK_RBUTTON;
    if (mouseState & SDL_BUTTON_MMASK) flags |= MK_MBUTTON;
    SDL_Keymod km = SDL_GetModState();
    if (km & KMOD_SHIFT) flags |= MK_SHIFT;
    if (km & KMOD_CTRL)  flags |= MK_CONTROL;
    return flags;
}

// Convert SDL scancode to Windows virtual key code
static UINT SDLKeyToVK(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return VK_SHIFT;
    case SDL_SCANCODE_LCTRL:  case SDL_SCANCODE_RCTRL:  return VK_CONTROL;
    case SDL_SCANCODE_LALT:   case SDL_SCANCODE_RALT:   return VK_MENU;
    case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
    case SDL_SCANCODE_RETURN: return VK_RETURN;
    case SDL_SCANCODE_TAB:    return VK_TAB;
    case SDL_SCANCODE_DELETE: return VK_DELETE;
    case SDL_SCANCODE_INSERT: return VK_INSERT;   // IDA_STOP_DESTROY (was unmapped -> key dropped)
    case SDL_SCANCODE_HOME:   return VK_HOME;     // IDA_CENTER: center on selection/rocket (was unmapped)
    case SDL_SCANCODE_LEFT:   return VK_LEFT;
    case SDL_SCANCODE_RIGHT:  return VK_RIGHT;
    case SDL_SCANCODE_UP:     return VK_UP;
    case SDL_SCANCODE_DOWN:   return VK_DOWN;
    case SDL_SCANCODE_F1:     return VK_F1;
    case SDL_SCANCODE_F2:     return VK_F2;
    case SDL_SCANCODE_F3:     return VK_F3;
    case SDL_SCANCODE_F4:     return VK_F4;
    case SDL_SCANCODE_F5:     return VK_F5;
    case SDL_SCANCODE_F12:    return VK_F12;
    case SDL_SCANCODE_0: return '0';
    case SDL_SCANCODE_1: return '1';
    case SDL_SCANCODE_2: return '2';
    case SDL_SCANCODE_3: return '3';
    case SDL_SCANCODE_4: return '4';
    case SDL_SCANCODE_5: return '5';
    case SDL_SCANCODE_6: return '6';
    case SDL_SCANCODE_7: return '7';
    case SDL_SCANCODE_8: return '8';
    case SDL_SCANCODE_9: return '9';
    default:
        // For letter keys A-Z, SDL_SCANCODE_A = 4
        if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
            return 'A' + (sc - SDL_SCANCODE_A);
        return 0;
    }
}

// ===========================================================================
// SDL-native cursor bridge
// ---------------------------------------------------------------------------
// The area map changes its cursor with ::SetCursor(HCURSOR) in ~40 places
// (SetMouseState plus the build / road / rocket flows). On the SDL window the
// OS cursor is owned by SDL, so a bare ::SetCursor doesn't reliably stick —
// the game cursor would blink to the system arrow or vanish on the next mouse
// move (the long-standing "area cursor invisible" bug).
//
// AreaApplyCursor() is the single chokepoint every one of those call sites now
// routes through. It converts each Win32 HCURSOR (including the custom
// IDC_MOVE* / IDC_SELECT* / IDC_TARGET* resource cursors) into an SDL_Cursor*
// once, caches it by handle, and drives SDL_SetCursor (which applies the
// cursor through the OS immediately). A NULL handle means "hide" — the
// original game draws its own build-footprint / rocket cursor onto the map
// canvas in those modes.
// ===========================================================================
static std::unordered_map<HCURSOR, SDL_Cursor*> s_sdlCursorCache;

#ifndef _WIN32
// Linux: the Win32 GDI cursor pipeline (GetIconInfo/GetDIBits) is stubbed, so we
// load the original .CUR resources directly. They're all 1-bpp 32x32 monochrome
// (XOR color + AND transparency mask). Resource id → filename is the .rc CURSOR
// table; the art lives in enations_latest/src/res/.
static const char* CurFileForId( int id )
{
    switch ( id ) {
    case 171: return "goto1.cur";    case 172: return "cur00006.cur";
    case 168: return "cur00004.cur"; case 173: return "goto3.cur";
    case 184: return "cur00007.cur"; case 232: return "move1.cur";
    case 240: return "move2.cur";    case 237: return "move3.cur";
    case 241: return "move4.cur";    case 238: return "move5.cur";
    case 242: return "move6.cur";    case 239: return "move7.cur";
    case 165: return "cur00002.cur"; case 178: return "target1.cur";
    case 179: return "target2.cur";  case 180: return "target3.cur";
    case 167: return "cur00003.cur"; case 181: return "select1.cur";
    case 182: return "select2.cur";  case 183: return "select3.cur";
    case 161: return "cursor1.cur";  case 217: return "cur00001.cur";
    case 218: return "cur00012.cur"; case 219: return "cur00013.cur";
    case 162: return "road_beg.cur"; case 220: return "cur00014.cur";
    case 221: return "cur00015.cur"; case 222: return "road_set.cur";
    case 170: return "cur00005.cur"; case 185: return "cur00008.cur";
    case 191: return "cur00011.cur"; case 192: return "repair1.cur";
    case 190: return "cur00010.cur"; case 226: return "unload1.cur";
    case 227: return "unload2.cur";  case 228: return "unload3.cur";
    case 236: return "load4.cur";    case 223: return "load1.cur";
    case 224: return "load2.cur";    case 225: return "load3.cur";
    default:  return nullptr;
    }
}
static std::string FindCursorDir( )
{
    static std::string s_dir = "?";
    if ( s_dir != "?" ) return s_dir;
    const char* cands[] = { "res", "../../../enations_latest/src/res",
                            "enations_latest/src/res", "../src/enations_latest/res", nullptr };
    auto probe = []( const std::string& p ) -> bool {
        if ( FILE* t = fopen( ( p + "/cursor1.cur" ).c_str(), "rb" ) ) { fclose( t ); return true; }
        return false;
    };
    s_dir = "";
    // 1) cwd-relative.
    for ( int i = 0; cands[i]; ++i )
        if ( probe( cands[i] ) ) { s_dir = cands[i]; return s_dir; }
    // 2) EXE-relative — robust to the launch working directory (mirrors the baked
    // terrain set's FindAssetDir). Walk up from the exe dir trying each candidate.
    char exePath[MAX_PATH];
    DWORD n = GetModuleFileNameA( NULL, exePath, MAX_PATH );
    if ( n > 0 && n < MAX_PATH ) {
        std::string dir( exePath, n );
        size_t slash = dir.find_last_of( "\\/" );
        if ( slash != std::string::npos ) dir = dir.substr( 0, slash );
        for ( int up = 0; up < 8 && !dir.empty(); ++up ) {
            for ( int i = 0; cands[i]; ++i ) {
                std::string cand = dir + "/" + cands[i];
                if ( probe( cand ) ) { s_dir = cand; return s_dir; }
            }
            size_t s = dir.find_last_of( "\\/" );
            if ( s == std::string::npos ) break;
            dir = dir.substr( 0, s );
        }
    }
    if ( getenv( "EN_DIAG" ) )
        fprintf( stderr, "[DIAG] cursor dir = '%s'\n", s_dir.empty() ? "(not found)" : s_dir.c_str() );
    return s_dir;
}
static SDL_Cursor* LoadCurFromFile( const std::string& path )
{
    FILE* f = fopen( path.c_str(), "rb" );
    if ( !f ) return nullptr;
    fseek( f, 0, SEEK_END ); long sz = ftell( f ); fseek( f, 0, SEEK_SET );
    if ( sz < 22 ) { fclose( f ); return nullptr; }
    std::vector<unsigned char> b( (size_t)sz );
    size_t got = fread( b.data(), 1, (size_t)sz, f );
    fclose( f );
    if ( got != (size_t)sz ) return nullptr;
    auto r16 = [&]( size_t o ){ return (int)( b[o] | ( b[o+1] << 8 ) ); };
    auto r32 = [&]( size_t o ){ return (uint32_t)( b[o] | ( b[o+1] << 8 ) | ( b[o+2] << 16 ) | ( (uint32_t)b[o+3] << 24 ) ); };
    if ( r16( 4 ) < 1 ) return nullptr;                       // idCount
    int      hotX = r16( 6 + 4 ), hotY = r16( 6 + 6 );        // ICONDIRENTRY hotspot
    uint32_t off  = r32( 6 + 12 );                            // image offset
    if ( off + 40 > b.size() ) return nullptr;
    int W = (int)r32( off + 4 ), H = (int)r32( off + 8 ) / 2; // BIH width / (height/2)
    int bpp = r16( off + 14 );
    if ( W <= 0 || H <= 0 || bpp != 1 ) return nullptr;       // all game cursors are 1-bpp
    size_t pal = off + 40;                                    // 2-entry color table
    uint32_t col0 = ( b[pal+2] << 16 ) | ( b[pal+1] << 8 ) | b[pal];
    uint32_t col1 = ( b[pal+6] << 16 ) | ( b[pal+5] << 8 ) | b[pal+4];
    int    rowBytes = ( ( W + 31 ) / 32 ) * 4;                // 1-bpp DWORD-aligned
    size_t xorOff = pal + 8, andOff = xorOff + (size_t)rowBytes * H;
    if ( andOff + (size_t)rowBytes * H > b.size() ) return nullptr;
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat( 0, W, H, 32, SDL_PIXELFORMAT_ARGB8888 );
    if ( !surf ) return nullptr;
    Uint32* px = (Uint32*)surf->pixels; int pitchPx = surf->pitch / 4;
    for ( int y = 0; y < H; ++y ) {
        int sr = H - 1 - y;                                   // BMP rows are bottom-up
        const unsigned char* xr = &b[xorOff + (size_t)sr * rowBytes];
        const unsigned char* ar = &b[andOff + (size_t)sr * rowBytes];
        for ( int x = 0; x < W; ++x ) {
            int xb = ( xr[x >> 3] >> ( 7 - ( x & 7 ) ) ) & 1;
            int ab = ( ar[x >> 3] >> ( 7 - ( x & 7 ) ) ) & 1;
            Uint32 p;
            if ( ab ) p = xb ? 0xFF000000u : 0x00000000u;     // AND=1: XOR=1 invert→black, else transparent
            else      p = 0xFF000000u | ( xb ? col1 : col0 ); // AND=0: opaque XOR color
            px[(size_t)y * pitchPx + x] = p;
        }
    }
    SDL_Cursor* c = SDL_CreateColorCursor( surf, hotX, hotY );
    SDL_FreeSurface( surf );
    return c;
}
static SDL_Cursor* SdlCursorFromResId( int id )
{
    // Win32 standard cursors (LoadStandardCursor) → SDL system cursors.
    switch ( id ) {
    case 32512: return SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_ARROW );      // IDC_ARROW
    case 32514: return SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_WAIT );       // IDC_WAIT
    case 32650: return SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_WAITARROW );  // IDC_APPSTARTING
    case 32646: return SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_SIZEALL );    // IDC_SIZEALL
    }
    if ( const char* fn = CurFileForId( id ) ) {
        std::string dir = FindCursorDir();
        if ( !dir.empty() )
            if ( SDL_Cursor* c = LoadCurFromFile( dir + "/" + fn ) )
                return c;
    }
    static SDL_Cursor* s_arrow = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_ARROW );
    return s_arrow;
}
#endif // !_WIN32

static SDL_Cursor* SdlCursorFromHCURSOR( HCURSOR hCur )
{
#ifndef _WIN32
    // Linux: hCur is the resource id (the LoadCursor shim returns (HCURSOR)name).
    return SdlCursorFromResId( (int)(intptr_t)hCur );
#else
    ICONINFO ii = {};
    if ( !::GetIconInfo( hCur, &ii ) )
        return nullptr;

    // Only color cursors are handled here (every game cursor, and the modern
    // system cursors, are color). Monochrome handles return nullptr and the
    // caller falls back to the plain Win32 ::SetCursor path.
    SDL_Cursor* result = nullptr;
    if ( ii.hbmColor != NULL )
    {
        BITMAP bm = {};
        ::GetObjectA( ii.hbmColor, sizeof( bm ), &bm );
        int w = bm.bmWidth;
        int h = bm.bmHeight;
        if ( ( w > 0 ) && ( h > 0 ) )
        {
            BITMAPINFO bi = {};
            bi.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
            bi.bmiHeader.biWidth       = w;
            bi.bmiHeader.biHeight      = -h;            // top-down
            bi.bmiHeader.biPlanes      = 1;
            bi.bmiHeader.biBitCount    = 32;
            bi.bmiHeader.biCompression = BI_RGB;

            std::vector<DWORD> color( (size_t)w * h, 0 );
            std::vector<DWORD> mask ( (size_t)w * h, 0 );
            HDC hdc = ::GetDC( NULL );
            ::GetDIBits( hdc, ii.hbmColor, 0, h, color.data(), &bi, DIB_RGB_COLORS );
            if ( ii.hbmMask )
                ::GetDIBits( hdc, ii.hbmMask, 0, h, mask.data(), &bi, DIB_RGB_COLORS );
            ::ReleaseDC( NULL, hdc );

            // If the color DIB carries a real alpha channel, trust it; else
            // derive transparency from the AND mask (white pixel = clear).
            bool bHasAlpha = false;
            for ( size_t i = 0; i < color.size(); ++i )
                if ( ( color[i] & 0xFF000000 ) != 0 ) { bHasAlpha = true; break; }

            SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
                0, w, h, 32, SDL_PIXELFORMAT_ARGB8888 );
            if ( surf )
            {
                Uint32* px      = (Uint32*)surf->pixels;
                int     pitchPx = surf->pitch / 4;
                for ( int y = 0; y < h; ++y )
                    for ( int x = 0; x < w; ++x )
                    {
                        DWORD c   = color[(size_t)y * w + x];
                        DWORD rgb = c & 0x00FFFFFF;
                        BYTE  a;
                        if ( bHasAlpha )
                            a = (BYTE)( c >> 24 );
                        else
                            a = ( mask[(size_t)y * w + x] & 0x00FFFFFF ) ? 0x00 : 0xFF;
                        px[(size_t)y * pitchPx + x] = ( (Uint32)a << 24 ) | rgb;
                    }
                result = SDL_CreateColorCursor( surf, ii.xHotspot, ii.yHotspot );
                SDL_FreeSurface( surf );
            }
        }
    }

    if ( ii.hbmColor ) ::DeleteObject( ii.hbmColor );
    if ( ii.hbmMask )  ::DeleteObject( ii.hbmMask );
    return result;
#endif // !_WIN32
}

static void AreaApplyCursor( HCURSOR hCur )
{
    if ( hCur == NULL )
    {
        // Build / rocket placement: hide the OS cursor; the footprint is drawn
        // onto the map canvas (matches the original game behavior).
        SDL_ShowCursor( SDL_DISABLE );
        ::SetCursor( NULL );
        return;
    }
    SDL_ShowCursor( SDL_ENABLE );

    SDL_Cursor* sc;
    std::unordered_map<HCURSOR, SDL_Cursor*>::iterator it = s_sdlCursorCache.find( hCur );
    if ( it != s_sdlCursorCache.end() )
        sc = it->second;
    else
    {
        sc = SdlCursorFromHCURSOR( hCur );
        s_sdlCursorCache[hCur] = sc;   // cache even nullptr so we don't retry
    }

    if ( sc )
        SDL_SetCursor( sc );           // applies through the OS immediately
    else
        ::SetCursor( hCur );           // fallback: Win32 cursor (subclass keeps it)
}

std::string CWndArea::sWndCls;

const int SEL_WIDTH = 2;

// Captured freeform path for line movement (drawn formation). Client-px points of
// the current RMB drag, starting at m_ptRMDN. File-static (only one line-move drag
// happens at a time) so CWndArea's layout doesn't change. Cleared on RMB-down.
static std::vector<CPoint> s_linePath;

// Total pixel length of the polyline.
static float LinePathLength( )
{
    float total = 0;
    for ( size_t i = 1; i < s_linePath.size( ); ++i )
    {
        float dx = (float)( s_linePath[i].x - s_linePath[i - 1].x );
        float dy = (float)( s_linePath[i].y - s_linePath[i - 1].y );
        total += sqrt( dx * dx + dy * dy );
    }
    return total;
}

// Point at arc-length s along the polyline (clamped to the ends).
static CPoint LinePathAt( float s )
{
    if ( s_linePath.empty( ) )
        return CPoint( 0, 0 );
    if ( s <= 0 || s_linePath.size( ) == 1 )
        return s_linePath.front( );
    float acc = 0;
    for ( size_t i = 1; i < s_linePath.size( ); ++i )
    {
        float dx  = (float)( s_linePath[i].x - s_linePath[i - 1].x );
        float dy  = (float)( s_linePath[i].y - s_linePath[i - 1].y );
        float seg = sqrt( dx * dx + dy * dy );
        if ( acc + seg >= s )
        {
            float t = ( seg > 0 ) ? ( s - acc ) / seg : 0;
            return CPoint( s_linePath[i - 1].x + (int)( dx * t ),
                           s_linePath[i - 1].y + (int)( dy * t ) );
        }
        acc += seg;
    }
    return s_linePath.back( );
}

// Arc-length of the point on the polyline closest to p (for ordering units along a
// curve so the formation doesn't cross over itself).
static float LinePathClosestArc( CPoint p )
{
    float best = 1e18f, bestArc = 0, acc = 0;
    for ( size_t i = 1; i < s_linePath.size( ); ++i )
    {
        float ax = (float)s_linePath[i - 1].x, ay = (float)s_linePath[i - 1].y;
        float dx = (float)s_linePath[i].x - ax, dy = (float)s_linePath[i].y - ay;
        float seg2 = dx * dx + dy * dy;
        float t    = ( seg2 > 0 ) ? ( ( p.x - ax ) * dx + ( p.y - ay ) * dy ) / seg2 : 0;
        if ( t < 0 ) t = 0;
        if ( t > 1 ) t = 1;
        float cx = ax + dx * t, cy = ay + dy * t;
        float d  = ( p.x - cx ) * ( p.x - cx ) + ( p.y - cy ) * ( p.y - cy );
        if ( d < best )
        {
            best    = d;
            bestArc = acc + sqrt( seg2 ) * t;
        }
        acc += sqrt( seg2 );
    }
    return bestArc;
}


int     CWndArea::m_iCount = 0;
std::string CWndArea::m_sHelp;
std::string CWndArea::m_sHelpBuild;
std::string CWndArea::m_sHelpRoad;
std::string CWndArea::m_sHelpCantBuild[9];
std::string CWndArea::m_sHelpRMB;
std::string CWndArea::m_sHelpOkFarm;
std::string CWndArea::m_sHelpBadFarm;
std::string CWndArea::m_sHelpNoFarm;
std::string CWndArea::m_sHelpOkMine;
std::string CWndArea::m_sHelpBadMine;
std::string CWndArea::m_sHelpNoMine;

HCURSOR CWndArea::m_hCurReg;
HCURSOR CWndArea::m_hCurGoto[4];
HCURSOR CWndArea::m_hCurWait;
HCURSOR CWndArea::m_hCurStart;
HCURSOR CWndArea::m_hCurRoadBgn[4];
HCURSOR CWndArea::m_hCurRoadSet[4];
HCURSOR CWndArea::m_hCurTarget[4];
HCURSOR CWndArea::m_hCurSelect[4];
HCURSOR CWndArea::m_hCurRoute;
HCURSOR CWndArea::m_hCurMove[9];
HCURSOR CWndArea::m_hCurLoad[4];
HCURSOR CWndArea::m_hCurUnload[4];
HCURSOR CWndArea::m_hCurRepair;
HCURSOR CWndArea::m_hCurNoRepair;

// _bShowPos / _bClickAny are the registry-backed cheat globals (default FALSE),
// defined in lastplnt.cpp and declared extern in lastplnt.h under _CHEAT.
// (Removed local `static ... = 1` shadows that forced both cheats ON and hid the registry value.)

// accumulator for mouse wheel deltas
static int s_areaWheelAccum = 0;

// 1 if inc to next, 0 if not
const int abPos[NUM_AREA_BUTTONS] = { 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1 };
const int abID[NUM_AREA_BUTTONS]  = { IDC_AREA_COMBAT,  IDC_AREA_CLOCK,        IDC_AREA_COUNTER,
                                     IDC_AREA_ZOOM_IN, IDC_AREA_ZOOM_OUT,     IDC_AREA_RES,

                                     IDC_UNIT_STOP,    IDC_UNIT_RESUME,

                                     IDC_UNIT_BUILD,   IDC_UNIT_CANCEL_BUILD, IDC_UNIT_ROUTE,
                                     IDC_UNIT_UNLOAD,  IDC_UNIT_RETREAT,

                                     IDC_UNIT_ROAD,    IDC_UNIT_CANCEL_ROAD,

                                     IDC_UNIT_REPAIR,  IDC_UNIT_CANCEL_REPAIR };

const int abBtn[NUM_AREA_BUTTONS] = { 54, 37, 36, 25, 26, 14, 39, 34, 4, 22, 47, 40, 35, 5, 23, 32, 45 };

const int abHelp[NUM_AREA_BUTTONS] = { IDH_AREA_COMBAT,      IDH_AREA_CLOCK,  IDH_AREA_COUNTER, IDH_AREA_ZOOM_IN,
                                       IDH_AREA_ZOOM_OUT,    IDH_AREA_RES,

                                       IDH_UNIT_STOP,        IDH_UNIT_RESUME, IDH_UNIT_BUILD,   IDH_UNIT_CANCEL_BUILD,
                                       IDH_UNIT_ROUTE,       IDH_UNIT_UNLOAD, IDH_UNIT_RETREAT, IDH_UNIT_ROAD,
                                       IDH_UNIT_CANCEL_ROAD, IDH_UNIT_REPAIR, IDH_UNIT_CANCEL };

const int NUM_ORD_BUTTONS = 11;
const int ORD_OFFSET      = 6;
enum
{
    enableID,
    disableID,
    hideID
};
// buttons on if nothing selected
const int asNoneID[NUM_ORD_BUTTONS] = { disableID, hideID, disableID, hideID, hideID, hideID,
                                        hideID,    hideID, hideID,    hideID, hideID };
// buttons on if cranes only
const int asCraneID[NUM_ORD_BUTTONS] = { enableID, hideID,   enableID, hideID,   hideID, hideID,
                                         hideID,   enableID, hideID,   enableID, hideID };
// buttons on if trucks only
const int asTruckID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, enableID, hideID,
                                         hideID,   hideID, hideID, hideID, hideID };
// buttons on if unloadable carriers
const int asUnloadID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, hideID, enableID,
                                          hideID,   hideID, hideID, hideID, hideID };
// buttons on if vehicles
const int asVehID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, enableID, hideID,
                                       hideID,   hideID, hideID, hideID, hideID };
// buttons on if bldgs
const int asBldgID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, hideID, hideID,
                                        hideID,   hideID, hideID, hideID, hideID };
// buttons on if 1 factory
const int asFacID[NUM_ORD_BUTTONS] = { enableID, hideID, enableID, hideID, hideID, hideID,
                                       hideID,   hideID, hideID,   hideID, hideID };
// buttons on if bldgs & vehicles
const int asUnitID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, hideID, hideID,
                                        hideID,   hideID, hideID, hideID, hideID };


CHexCoord CWndArea::ToBuildUL( CHexCoord& hexCur )
{

    CStructureData const* pData = theStructures.GetData( m_iBuild );
    CHexCoord             _hex( hexCur );
    switch ( m_aa.m_iDir )
    {
    case 0:
        _hex.Y( ) -= pData->GetCY( );
        break;
    case 2:
        _hex.X( ) -= pData->GetCX( );
        break;
    case 3:
        _hex.X( ) -= pData->GetCX( );
        _hex.Y( ) -= pData->GetCY( );
        break;
    }
    _hex.Wrap( );
    return ( _hex );
}

/////////////////////////////////////////////////////////////////////////////
// MouseHook - used to turn off our cursors when the mouse leaves the window

HHOOK CWndArea::m_hhk = NULL;

LRESULT CALLBACK CWndArea::MouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{

    // we have to be over an area map
    if ( ( nCode == HC_ACTION ) && ( theMap.HaveBldgCur( ) ) )
    {
        POSITION pos;
        for ( pos = theAreaList.GetHeadPosition( ); pos != NULL; )
        {
            CWndArea* pWndArea = theAreaList.GetNext( pos );
            // it's going to an area map
            if ( pWndArea->m_hWnd == ( (MOUSEHOOKSTRUCT*)lParam )->hwnd )
                return ( CallNextHookEx( m_hhk, nCode, wParam, lParam ) );
        }

        // it's not on an area map - kill the cursor
        theMap.ClrBldgCur( );
    }

    return ( CallNextHookEx( m_hhk, nCode, wParam, lParam ) );
}


/////////////////////////////////////////////////////////////////////////////
// CAreaList

CAreaList::CAreaList( )
{

    m_hexLastCombat = CHexCoord( 0, 0 );
    m_bLcSet        = FALSE;
}

void CAreaList::MoveSizeToNew( int xOld, int yOld )
{
#ifdef LOGGINGON
    OutputDebugStringA( "MoveSizeToNew" );
#endif

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWnd = GetNext( pos );
        CRect     rect;
        pWnd->GetWindowRect( &rect );
        pWnd->SetWindowPos( NULL, ( rect.left * theApp.m_iScrnX ) / xOld, ( rect.top * theApp.m_iScrnY ) / yOld,
                            ( rect.Width( ) * theApp.m_iScrnX ) / xOld, ( rect.Height( ) * theApp.m_iScrnY ) / yOld,
                            SWP_NOZORDER );
    }
}

void CAreaList::SetLastAttack( CHexCoord const& _hex )
{

    m_hexLastCombat = _hex;

    if ( m_bLcSet )
        return;

    // enable the button
    m_bLcSet = TRUE;

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->EnableButton( IDC_AREA_COMBAT, TRUE );
    }
}

void CAreaList::AddWindow( CWndArea* pWnd )
{

    ASSERT_VALID( this );
    ASSERT_VALID( pWnd );

    AddTail( pWnd );
    m_pTopArea = pWnd;  // newest opened window becomes the top
}

void CAreaList::DestroyAllWindows( )
{

    while ( GetCount( ) > 0 )
    {
        CWndArea* pWnd = GetHead( );
        RemoveHead( );
        pWnd->DestroyWindow( );
    }

    RemoveAll( );
    theApp.m_wndWorld.NewAreaMap( NULL );
}

void CAreaList::EnableWindows( BOOL bEnable )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea ); // what's wrong here?
        pWndArea->EnableWindow( bEnable );
    }
}

CWndArea* CAreaList::GetTop( )
{

    if ( GetCount( ) < 1 )
        return ( NULL );

    if ( GetCount( ) == 1 )
        return ( GetHead( ) );

    // SDL2: the old MFC path cast FindWindow()'s HWND straight to CWndArea* and
    // dereferenced it — garbage that crashed once multi-area (radio) was enabled.
    // Return the last-focused area window if it is still open, else the head.
    if ( m_pTopArea != NULL )
    {
        POSITION pos = GetHeadPosition( );
        while ( pos != NULL )
            if ( GetNext( pos ) == m_pTopArea )
                return ( m_pTopArea );
    }
    return ( GetHead( ) );
}

CWndArea* CAreaList::BringToTop( )
{

    CWndArea* pWnd = GetTop( );
    if ( pWnd == NULL )
        return ( NULL );

    pWnd->ShowWindow( SW_RESTORE );
    pWnd->SetFocus( );

    return ( pWnd );
}

void CAreaList::MaterialChange( CUnit const* pUnit ) const
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->MaterialChange( pUnit );
    }
}

void CAreaList::InvalidateStatus( CUnit const* pUnit ) const
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        if ( ( pWndArea->m_pUnit == pUnit ) || ( pWndArea->GetStaticUnit( ) == pUnit ) )
            pWndArea->InvalidateStatus( );
    }
}

void CAreaList::UnitDying( CUnit* pUnit )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->UnitDying( pUnit );
    }
}

void CAreaList::SelectOff( )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->SelectOff( );
    }
}

void CAreaList::XilDiscovered( )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        if ( pWndArea->m_bShowRes )
        {
            pWndArea->ResClicked( );
            pWndArea->ResClicked( );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CListUnits

void CListUnits::AddUnit( CUnit* pUnit, BOOL bDoList )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pUnit );

    // add if not already in the list
    if ( Find( pUnit ) == NULL )
    {
        pUnit->SetSelected( bDoList );
        AddHead( pUnit );
    }

    // if we changed selection we turn off whatever
    theAreaList.SelectOff( );
}

void CListUnits::RemoveUnit( CUnit* pUnit )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pUnit );

    POSITION pos = Find( pUnit );
    if ( pos != NULL )
        RemoveAt( pos );

    pUnit->SetUnselected( TRUE );
}

void CListUnits::RemoveAllUnits( BOOL bDoList )
{

    ASSERT_STRICT_VALID( this );

    if ( bDoList )
    {
        theApp.m_wndBldgs.m_ListBox.SetRedraw( FALSE );
        theApp.m_wndVehicles.m_ListBox.SetRedraw( FALSE );
    }

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = GetNext( pos );
        ASSERT_STRICT_VALID( pUnit ); // hmm
        pUnit->SetUnselected( bDoList );
    }

    if ( bDoList )
    {
        theApp.m_wndBldgs.m_ListBox.SetRedraw( TRUE );
        theApp.m_wndVehicles.m_ListBox.SetRedraw( TRUE );
        theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL, FALSE );
        theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL, FALSE );
    }

    RemoveAll( );
}


/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat

CWndUnitStat::CWndUnitStat( )
{

    m_pUnit = NULL;
}

void CWndUnitStat::SetUnit( CUnit* pUnit )
{

    m_pUnit = pUnit;
}

BEGIN_MESSAGE_MAP( CWndUnitStat, CWndStatBar )
//{{AFX_MSG_MAP(CWndUnitStat)
ON_WM_PAINT( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

void CWndUnitStat::SetText( char const* pStr, CStatInst::IMPORTANCE iImp )
{

    CWndStatBar::SetText( pStr, iImp );
}

// for UpdateMaterials
void CWndUnitStat::UpdateStat( )
{

    CPoint pt;
    ::GetCursorPos( &pt );
    if ( ::WindowFromPoint( pt ) == m_hWnd )
        OnMouseMove( 0, pt );
}

// display info about this stuff
void CWndUnitStat::OnMouseMove( UINT, CPoint )
{

    if ( m_pUnit == NULL )
        CWndStatBar::SetText( NULL );
    else
    {
        std::string str;
        ::UnitStatusText( m_pUnit, str );
        theApp.m_wndBar.SetStatusText( 1, str.c_str( ) );
    }
}

void CWndUnitStat::OnPaint( )
{

    if ( m_pUnit == NULL )
        CWndStatBar::OnPaint( );
    else
    {
        CPaintDC dc( this );
        thePal.Paint( dc.m_hDC );

        CRect rect;
        GetClientRect( &rect );
        CPoint pt( 0, 0 );
        ::MapWindowPoints( m_hWnd, GetParent( )->m_hWnd, &pt, 1 );
        ::UnitShowStatus( m_pUnit, (CDC*)dc, rect, theBitmaps.GetByIndex( DIB_AREA_BAR ), pt );

        thePal.EndPaint( dc.m_hDC );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CWndAreaStatic

CWndAreaStatic::CWndAreaStatic( )
{

    m_iNumStatusText = NUM_STATUS_TEXT;
    m_iStatusStrt = m_iStatusNoCraneStrt = 7 * ( theBmBtnData.Width( ) + AREA_BTN_X_SKIP );
    m_iStatusCraneStrt                   = 9 * ( theBmBtnData.Width( ) + AREA_BTN_X_SKIP );
}

CWndAreaStatic::~CWndAreaStatic( )
{
    delete m_sdl2Bar;
    m_sdl2Bar = nullptr;

    if ( m_sdlPanel && theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        theApp.m_gameWindow->GetCompositor()->RemovePanel( m_sdlPanel );
        m_sdlPanel = nullptr;
    }
}

BOOL CWndAreaStatic::PreCreate( )
{

    // make sure we have enough room for all the buttons & status lines
    CRect rect( 0, 0, m_iStatusStrt + theApp.TextWid( ) * MIN_TEXT_WID * 2, AREA_BTN_HT );

    AdjustWindowRect( &rect, dwStatusWndStyle, FALSE );
    m_iXmin = rect.Width( );
    m_iYmin = rect.Height( );

    // finally, if its wider than the screen - we bring it in
    m_iXmin = __min( m_iXmin, theApp.m_iScrnX );

    return ( TRUE );
}

BEGIN_MESSAGE_MAP( CWndAreaStatic, CWndBase )
//{{AFX_MSG_MAP(CWndAreaStatic)
ON_WM_CREATE( )
ON_WM_PAINT( )
ON_WM_SIZE( )
ON_WM_DESTROY( )
ON_WM_ERASEBKGND( )
ON_MESSAGE( WM_BUTTONMOUSEMOVE, OnChildMouseMove )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CWndAreaStatic message handlers

LRESULT CWndAreaStatic::OnChildMouseMove( WPARAM, LPARAM lParam )
{

    theApp.m_wndBar.SetStatusText( 1, (char*)lParam );
    return ( 0 );
}

int CWndAreaStatic::OnCreate( LPCREATESTRUCT lpCS )
{

    if ( CWndBase::OnCreate( lpCS ) == -1 )
        return -1;

    // we had to start with the load icon to get a different class
    ::SetClassLongPtr( m_hWnd, GCLP_HCURSOR, NULL );

    // create the position buttons
    CRect rect( AREA_BTN_X_SKIP, AREA_BTN_Y_START, AREA_BTN_X_SKIP + theBmBtnData.Width( ),
                AREA_BTN_Y_START + theBmBtnData.Height( ) );

    // create the buttons
    CBmButton* pBtn = m_Btns;
    for ( int iOn = 0; iOn < NUM_AREA_BUTTONS; iOn++, pBtn++ )
    {
        pBtn->Create( abBtn[iOn], abHelp[iOn], &theBmBtnData, rect, theBitmaps.GetByIndex( DIB_AREA_BAR ), this,
                      abID[iOn] );

        // how we show it
        if ( ( ORD_OFFSET <= iOn ) && ( iOn < NUM_ORD_BUTTONS + ORD_OFFSET ) )
        {
            pBtn->EnableWindow( FALSE );
            if ( asNoneID[iOn - ORD_OFFSET] == hideID )
                pBtn->ShowWindow( SW_HIDE );
        }

        if ( abPos[iOn] )
            rect.OffsetRect( theBmBtnData.Width( ) + AREA_BTN_X_SKIP, 0 );

        if ( abID[iOn] == IDC_UNIT_ROAD )
            m_iStatusNoCraneStrt = m_iStatusStrt = rect.left + AREA_BTN_X_SKIP;
    }
    rect.left += AREA_BTN_X_SKIP;
    m_iStatusCraneStrt = rect.left;

    // create the status windows
    m_wndStat.Create( &theIcons, ICON_BAR_TEXT, rect, this, theBitmaps.GetByIndex( DIB_AREA_BAR ) );
    SizeStatus( );

    // Panel and SDL2AreaBar are now created by CWndArea::OnCreate.
    // Just make MFC window transparent here. WS_EX_TRANSPARENT also makes the
    // window click-through so clicks reach the SDL panel rendered behind it.
    if ( theApp.m_gameWindow ) {
        ::SetWindowLong( m_hWnd, GWL_EXSTYLE,
            ::GetWindowLong( m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED | WS_EX_TRANSPARENT );
        ::SetLayeredWindowAttributes( m_hWnd, 0, 0, LWA_ALPHA );
    }

    return 0;
}

BOOL CWndAreaStatic::OnCommand( WPARAM wParam, LPARAM lParam )
{

    if ( HIWORD( wParam ) == BN_CLICKED )
    {
        GetParent( )->SendMessage( WM_COMMAND, wParam, lParam );
        return ( TRUE );
    }

    return ( CWndBase::OnCommand( wParam, lParam ) );
}

void CWndAreaStatic::CaptureToPanel() {
    if ( !m_sdlPanel || !m_hWnd )
        return;

    SDL_Surface* panelSurface = m_sdlPanel->GetSurface();
    if ( !panelSurface )
        return;

    int w = m_sdlPanel->GetWidth();
    int h = m_sdlPanel->GetHeight();
    if ( w <= 0 || h <= 0 )
        return;

    HDC hdcScreen = ::GetDC( NULL );
    HDC hdcMem    = ::CreateCompatibleDC( hdcScreen );
    HBITMAP hBmp  = ::CreateCompatibleBitmap( hdcScreen, w, h );
    HBITMAP hOld  = (HBITMAP)::SelectObject( hdcMem, hBmp );

    ::PrintWindow( m_hWnd, hdcMem, PW_RENDERFULLCONTENT );

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    if ( SDL_LockSurface( panelSurface ) == 0 ) {
        ::GetDIBits( hdcMem, hBmp, 0, h, panelSurface->pixels, &bmi, DIB_RGB_COLORS );
        SDL_UnlockSurface( panelSurface );
        m_sdlPanel->SetDirty();
    }

    ::SelectObject( hdcMem, hOld );
    ::DeleteObject( hBmp );
    ::DeleteDC( hdcMem );
    ::ReleaseDC( NULL, hdcScreen );
}

BOOL CWndAreaStatic::OnEraseBkgnd( CDC* )
{

    return TRUE;
}

void CWndAreaStatic::OnPaint( )
{

    ASSERT_STRICT_VALID( this );
    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    CRect rect;
    CWndBase::GetClientRect( &rect );

    theBitmaps.GetByIndex( DIB_AREA_BAR )->Tile( dc, rect );

    thePal.EndPaint( dc.m_hDC );
    // Do not call CWndBase::OnPaint() for painting messages
}

void CWndAreaStatic::OnSize( UINT nType, int cx, int cy )
{
#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "CWndAreaStatic::OnSize cx=%d cy=%d\n", cx, cy );
    OutputDebugStringA( buf );
#endif


    CWndBase::OnSize( nType, cx, cy );

    SizeStatus( );
}

void CWndAreaStatic::SizeStatus( )
{

    // adjust status window rects
    CRect rect;
    CWndBase::GetClientRect( &rect );

    // adjust the text windows
    rect.left = m_iStatusStrt;
    rect.right -= AREA_BTN_X_SKIP;
    rect.top += ( AREA_BTN_HT - AREA_TEXT_HT ) / 2;
    rect.bottom = rect.top + AREA_TEXT_HT;

    m_wndStat.SetWindowPos( NULL, rect.left, rect.top, rect.Width( ), rect.Height( ), SWP_NOZORDER );
}

BOOL CWndArea::IsButtonEnabled( int ID ) const
{

    auto pBtn = m_WndStatic.GetDlgItem( ID );
    ASSERT_STRICT_VALID( pBtn );
    return ( pBtn->IsWindowEnabled( ) );
}

void CWndAreaStatic::EnableButton( int ID, BOOL bEnable )
{

    auto pBtn = GetDlgItem( ID );
    ASSERT_STRICT_VALID( pBtn );
    pBtn->EnableWindow( bEnable );
}

void CWndAreaStatic::ShowButton( int ID, BOOL bShow )
{

    auto pBtn = GetDlgItem( ID );
    ASSERT_STRICT_VALID( pBtn );
    // When using SDL panel, keep buttons visible but disabled instead of hidden
    if ( m_sdlPanel ) {
        pBtn->ShowWindow( SW_SHOW );
        if ( !bShow )
            pBtn->EnableWindow( FALSE );
    } else {
        pBtn->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CWndArea

void CWndArea::LoadStaticResources( )
{

    m_iCount++;
    if ( m_iCount != 1 )
        return;

    // set the mouse hook func
    m_hhk = SetWindowsHookEx( WH_MOUSE, MouseProc, NULL, theApp.m_nThreadID );

    // load them
    m_sHelp = EnLoadStdString( IDH_AREA_WIN );
    m_sHelpBuild = EnLoadStdString( IDH_AREA_WIN_BUILD );
    m_sHelpRoad = EnLoadStdString( IDH_AREA_WIN_ROAD );
    m_sHelpCantBuild[0] = EnLoadStdString( IDH_AREA_CANT_BLDG_NEXT );
    m_sHelpCantBuild[1] = EnLoadStdString( IDH_AREA_CANT_WATER_NEXT );
    m_sHelpCantBuild[2] = EnLoadStdString( IDH_AREA_CANT_BLDG_RIVER_NEXT );
    m_sHelpCantBuild[3] = EnLoadStdString( IDH_AREA_CANT_VEH_IN_WAY );
    m_sHelpCantBuild[4] = EnLoadStdString( IDH_AREA_CANT_ON_WATER );
    m_sHelpCantBuild[5] = EnLoadStdString( IDH_AREA_CANT_NO_WATER );
    m_sHelpCantBuild[6] = EnLoadStdString( IDH_AREA_CANT_NO_LAND_EXIT );
    m_sHelpCantBuild[7] = EnLoadStdString( IDH_AREA_CANT_NO_WATER_EXIT );
    m_sHelpCantBuild[8] = EnLoadStdString( IDH_AREA_CANT_TOO_STEEP );
    m_sHelpRMB = EnLoadStdString( IDH_AREA_WIN_RMB );
    m_sHelpOkFarm = EnLoadStdString( IDH_AREA_OK_FARM );
    m_sHelpBadFarm = EnLoadStdString( IDH_AREA_BAD_FARM );
    m_sHelpNoFarm = EnLoadStdString( IDH_AREA_NO_FARM );
    m_sHelpOkMine = EnLoadStdString( IDH_AREA_OK_MINE );
    m_sHelpBadMine = EnLoadStdString( IDH_AREA_BAD_MINE );
    m_sHelpNoMine = EnLoadStdString( IDH_AREA_NO_MINE );

    // need our own class so we can change the cursor
    m_hCurReg        = theApp.LoadStandardCursor( IDC_ARROW );
    m_hCurGoto[0]    = theApp.LoadCursor( IDC_GOTO0 );
    m_hCurGoto[1]    = theApp.LoadCursor( IDC_GOTO1 );
    m_hCurGoto[2]    = theApp.LoadCursor( IDC_GOTO2 );
    m_hCurGoto[3]    = theApp.LoadCursor( IDC_GOTO3 );
    m_hCurWait       = theApp.LoadStandardCursor( IDC_WAIT );
    m_hCurRoadBgn[0] = theApp.LoadCursor( IDC_ROAD_BEGIN0 );
    m_hCurRoadBgn[1] = theApp.LoadCursor( IDC_ROAD_BEGIN1 );
    m_hCurRoadBgn[2] = theApp.LoadCursor( IDC_ROAD_BEGIN2 );
    m_hCurRoadBgn[3] = theApp.LoadCursor( IDC_ROAD_BEGIN3 );
    m_hCurRoadSet[0] = theApp.LoadCursor( IDC_ROAD_SET0 );
    m_hCurRoadSet[1] = theApp.LoadCursor( IDC_ROAD_SET1 );
    m_hCurRoadSet[2] = theApp.LoadCursor( IDC_ROAD_SET2 );
    m_hCurRoadSet[3] = theApp.LoadCursor( IDC_ROAD_SET3 );
    m_hCurStart      = theApp.LoadStandardCursor( IDC_APPSTARTING );
    m_hCurTarget[0]  = theApp.LoadCursor( IDC_TARGET0 );
    m_hCurTarget[1]  = theApp.LoadCursor( IDC_TARGET1 );
    m_hCurTarget[2]  = theApp.LoadCursor( IDC_TARGET2 );
    m_hCurTarget[3]  = theApp.LoadCursor( IDC_TARGET3 );
    m_hCurSelect[0]  = theApp.LoadCursor( IDC_SELECT0 );
    m_hCurSelect[1]  = theApp.LoadCursor( IDC_SELECT1 );
    m_hCurSelect[2]  = theApp.LoadCursor( IDC_SELECT2 );
    m_hCurSelect[3]  = theApp.LoadCursor( IDC_SELECT3 );
    m_hCurRoute      = theApp.LoadCursor( IDC_ROUTE );
    m_hCurMove[0]    = theApp.LoadCursor( IDC_MOVE0 );
    m_hCurMove[1]    = theApp.LoadCursor( IDC_MOVE1 );
    m_hCurMove[2]    = theApp.LoadCursor( IDC_MOVE2 );
    m_hCurMove[3]    = theApp.LoadCursor( IDC_MOVE3 );
    m_hCurMove[4]    = theApp.LoadCursor( IDC_MOVE4 );
    m_hCurMove[5]    = theApp.LoadCursor( IDC_MOVE5 );
    m_hCurMove[6]    = theApp.LoadCursor( IDC_MOVE6 );
    m_hCurMove[7]    = theApp.LoadCursor( IDC_MOVE7 );
    m_hCurMove[8]    = theApp.LoadStandardCursor( IDC_SIZEALL );
    m_hCurLoad[0]    = theApp.LoadCursor( IDC_LOAD0 );
    m_hCurLoad[1]    = theApp.LoadCursor( IDC_LOAD1 );
    m_hCurLoad[2]    = theApp.LoadCursor( IDC_LOAD2 );
    m_hCurLoad[3]    = theApp.LoadCursor( IDC_LOAD3 );
    m_hCurUnload[0]  = theApp.LoadCursor( IDC_UNLOAD0 );
    m_hCurUnload[1]  = theApp.LoadCursor( IDC_UNLOAD1 );
    m_hCurUnload[2]  = theApp.LoadCursor( IDC_UNLOAD2 );
    m_hCurUnload[3]  = theApp.LoadCursor( IDC_UNLOAD3 );
    m_hCurRepair     = theApp.LoadCursor( IDC_REPAIR );
    m_hCurNoRepair   = theApp.LoadCursor( IDC_NO_REPAIR );
}

void CWndArea::UnloadStaticResources( )
{

    m_iCount--;
    if ( m_iCount > 0 )
        return;

    // undo the hook
    UnhookWindowsHookEx( m_hhk );

    // unload them
    m_sHelp.clear( );
    m_sHelpBuild.clear( );
    m_sHelpRoad.clear( );
    m_sHelpRMB.clear( );
    for ( int iOn = 0; iOn < 9; iOn++ ) m_sHelpCantBuild[iOn].clear( );

    // BUGBUG - is there no way to delete a cursor?
}

void CWndArea::InvalidateStatus( )
{

    ASSERT_STRICT_VALID( this );

    m_WndStatic.m_wndStat.InvalidateRect( NULL );
}

CWndArea::CWndArea( )
{
    m_aa.SetWnd( this );
    m_iMoveCur = 8;

    m_iFound      = 0;
    m_iBuild      = -1;
    m_iMode       = normal;
    m_bPanBtnDown = FALSE;
    m_bRmbCmdDown = FALSE;
    m_bCapMouse   = FALSE;
    m_pWndInfo    = NULL;
    m_pSdlInfo    = NULL;
    m_iBuildDir   = 0;
    m_bNewPos     = TRUE;
    m_uMouseMode  = lmb_nothing;
    m_bNewSound   = TRUE;
    m_bShowRes    = FALSE;
    m_pSelUnder   = NULL;
    m_bScrollBars = FALSE;

    m_bLineMove = FALSE;

    m_phexRoadPath  = NULL;
    m_ppUnderSprite = NULL;
    m_iNumRoadHex   = 0;
}

void CWndArea::PostNcDestroy( )
{

    // remove it from the list
    POSITION pos = theAreaList.Find( this );
    if ( pos != NULL )
        theAreaList.RemoveAt( pos );

    ReleaseMouse( );
    ::ClipCursor( NULL );

    delete m_pWndInfo;
    delete m_pSdlInfo;

    UnloadStaticResources( );

    delete this;
}

CWndArea::~CWndArea( )
{
    // Remove SDL2 panel from compositor
    if ( m_aa.m_sdlPanel && theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        theApp.m_gameWindow->GetCompositor()->RemovePanel( m_aa.m_sdlPanel );
        m_aa.m_sdlPanel = nullptr;
    }

    delete[] m_pSelUnder;

    // remove it from the list
    POSITION pos = theAreaList.Find( this );
    ASSERT( pos == NULL );
    if ( pos != NULL )
        theAreaList.RemoveAt( pos );

    ReleaseMouse( );
    ::ClipCursor( NULL );
}

BOOL CWndArea::PreCreateWindow( CREATESTRUCT& cs )
{

    CWndAnim::PreCreateWindow( cs );

    // get the mins for the static window
    m_WndStatic.PreCreate( );

    CRect rect( 0, 0, m_WndStatic.m_iXmin, m_WndStatic.m_iYmin * 2 );
    AdjustWindowRect( &rect, dwPopWndStyle, FALSE );
    m_iXmin = rect.Width( );
    m_iYmin = rect.Height( );

    LoadStaticResources( );

    return ( TRUE );
}

void CWndArea::GetClientRect( LPRECT lpRect ) const
{

    CWndAnim::GetClientRect( lpRect );
    CRect rStatic;
    m_WndStatic.GetClientRect( &rStatic );
    if ( rStatic.Height( ) < lpRect->bottom )
        lpRect->bottom -= rStatic.Height( );
    else
        lpRect->bottom = 0;

    if ( m_bScrollBars )
    {
        lpRect->right -= GetSystemMetrics( SM_CXVSCROLL );
        lpRect->bottom -= GetSystemMetrics( SM_CYHSCROLL );
    }
}

#ifdef LOGGINGON
int  created = 0;
#endif

void CWndArea::Create( CMapLoc const& ml, CUnit* pUnit, BOOL bFirst )
{
#ifdef LOGGINGON
    created++;
    char buf[128];
    sprintf_s( buf, "CWndArea::Create (created=%d)\n", created );
    OutputDebugStringA( buf );
#endif

    ASSERT_VALID_OR_NULL( pUnit );

    if ( ( m_pUnit = pUnit ) != NULL )
    {
        m_lstUnits.AddUnit( pUnit, TRUE );
        ASSERT( m_lstUnits.GetCount( ) == 1 );
    }

    // CWndStub::CreateEx() bypasses MFC's PreCreateWindow flow. PreCreateWindow
    // is where LoadStaticResources() loads the area cursors (m_hCurReg /
    // m_hCurGoto / m_hCurTarget / ...). Without this call those HCURSORs stay
    // NULL and SetMouseState's AreaApplyCursor(m_hCurReg) hides the cursor.
    {
        CREATESTRUCT cs = {};
        PreCreateWindow( cs );
    }

    if ( sWndCls.empty( ) )
        sWndCls = CConquerApp::EnRegisterWndClass( "EnAreaWnd",
                      CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, m_hCurMove[8] );

    m_aa.Set( ml, 0, max( 1, theApp.GetZoomData( )->GetFirstZoom( ) ) );

    // figure the size
    CRect     rect;
    CWndArea* pPrev = theAreaList.GetTop( );
    if ( pPrev == NULL )
        rect.SetRect( EnGetProfileInt( theApp.m_sResIni.c_str(), "AreaX", theApp.m_iCol1 ),
                      EnGetProfileInt( theApp.m_sResIni.c_str(), "AreaY", 0 ),
                      EnGetProfileInt( theApp.m_sResIni.c_str(), "AreaEX", theApp.m_iScrnX ),
                      EnGetProfileInt( theApp.m_sResIni.c_str(), "AreaEY", theApp.m_iRow3 ) );
    else
    {
        // Additional area maps (radio multi-window) open SMALL and cascaded — the
        // original game used a small secondary window, and a full-window second map
        // doubles the GPU terrain/sprite render every frame (heavy lag). A small view
        // draws far fewer hexes, cutting that cost. ~45% of the main area, offset so it
        // doesn't bury the first.
        int iFullW = theApp.m_iScrnX - theApp.m_iCol1;
        int iFullH = theApp.m_iRow3;
        int iW = __max( 360, iFullW * 45 / 100 );
        int iH = __max( 280, iFullH * 45 / 100 );
        int iN = theAreaList.GetCount( );          // existing windows → cascade offset
        int iX = theApp.m_iCol1 + 40 + ( iN * 32 );
        int iY = 40 + ( iN * 32 );
        rect.SetRect( iX, iY, iX + iW, iY + iH );

        // set to the same dir & zoom
        m_aa.m_iDir  = pPrev->m_aa.m_iDir;
        m_aa.m_iZoom = pPrev->m_aa.m_iZoom;
    }

    std::string sTitle = EnLoadStdString( IDS_TITLE_AREA_MAP );
    DWORD dwStyle = dwPopWndStyle;

    if ( CreateEx( 0, sWndCls.c_str( ), sTitle.c_str( ), dwStyle, rect.left, rect.top, rect.Width( ), rect.Height( ),
                   theApp.m_pMainWnd->m_hWnd, NULL, NULL ) == 0 )
        throw( ERR_RES_CREATE_WND );

    if ( m_bScrollBars )
    {
        CRect rectClient;

        GetClientRect( &rectClient );

        CRect rectH( 0, rectClient.Height( ), rectClient.Width( ),
                     rectClient.Height( ) + GetSystemMetrics( SM_CYHSCROLL ) );
        CRect rectV( rectClient.Width( ), 0, rectClient.Width( ) + GetSystemMetrics( SM_CXVSCROLL ),
                     rectClient.Height( ) );

        if ( !m_scrollbarH.Create( SBS_HORZ | WS_CHILD | WS_VISIBLE, rectH, CWnd::FromHandle( m_hWnd ), unsigned( -1 ) ) )
            throw( ERR_RES_CREATE_WND );

        if ( !m_scrollbarV.Create( SBS_VERT | WS_CHILD | WS_VISIBLE, rectV, CWnd::FromHandle( m_hWnd ), unsigned( -1 ) ) )
            throw( ERR_RES_CREATE_WND );

        // set up the scroll bars
        //   note: the button is always in the middle and the range is always the map size
        CSize szMap = theMap.GetSize( );
        m_scrollbarH.SetScrollRange( 0, szMap.cx - 1, FALSE );
        m_scrollbarH.SetScrollPos( ( szMap.cx - 1 ) / 2, FALSE );
        m_scrollbarV.SetScrollRange( 0, szMap.cy - 1, FALSE );
        m_scrollbarV.SetScrollPos( ( szMap.cy - 1 ) / 2, FALSE );
    }

    m_colorbuffer.SetColor( 255, rect.Width( ) );

    // add us to the list of area windows
    theAreaList.AddWindow( this );

    // the area map is always added at the head, everything else at the tail
    theAnimList.push_front( this );

    // set the cursor of starting
    if ( bFirst )
        SetupStart( );  // first one - rocket
    else
        SetButtonState( );

    // set it up
    InvalidateWindow( );
    InvalidateSound( );
    ReRender( );

    EnableButton( IDC_AREA_COMBAT, theAreaList.HaveAttack( ) );
    CWndArea::CheckZoomBtns( );

    if ( !bFirst )
        ShowWindow( SW_SHOW );
}

BEGIN_MESSAGE_MAP( CWndArea, CWndAnim )
//{{AFX_MSG_MAP(CWndArea)
ON_WM_GETMINMAXINFO( )
ON_WM_SYSCOMMAND( )
ON_WM_HSCROLL( )
ON_WM_MOUSEMOVE( )
ON_WM_PAINT( )
ON_WM_VSCROLL( )
ON_WM_CREATE( )
ON_WM_SIZE( )
ON_WM_MOVE( )
ON_WM_DESTROY( )
ON_BN_CLICKED( IDC_AREA_COMBAT, LastCombat )
ON_BN_CLICKED( IDC_AREA_ZOOM_IN, ZoomIn )
ON_BN_CLICKED( IDC_AREA_ZOOM_OUT, ZoomOut )
ON_BN_CLICKED( IDC_AREA_CLOCK, TurnClock )
ON_BN_CLICKED( IDC_AREA_COUNTER, TurnCounter )
ON_BN_CLICKED( IDC_AREA_RES, ResClicked )
ON_BN_CLICKED( IDC_UNIT_STOP, StopUnit )
ON_BN_CLICKED( IDC_UNIT_RESUME, ResumeUnit )
ON_COMMAND( IDA_CENTER, CenterUnit )
ON_COMMAND( IDA_DESTROY, DestroyUnit )
ON_COMMAND( IDA_STOP_DESTROY, StopDestroyUnit )
ON_COMMAND( IDA_CUR_UP, CurUp )
ON_COMMAND( IDA_CUR_RIGHT, CurRight )
ON_COMMAND( IDA_CUR_DOWN, CurDown )
ON_COMMAND( IDA_CUR_LEFT, CurLeft )
ON_COMMAND( IDA_OPPO, OppoUnit )
ON_BN_CLICKED( IDC_UNIT_ROAD, RoadUnit )
ON_BN_CLICKED( IDC_UNIT_CANCEL_ROAD, CancelRoadUnit )
ON_BN_CLICKED( IDC_UNIT_BUILD, BuildUnit )
ON_BN_CLICKED( IDC_UNIT_CANCEL_BUILD, CancelBuildUnit )
ON_BN_CLICKED( IDC_UNIT_ROUTE, RouteUnit )
ON_BN_CLICKED( IDC_UNIT_UNLOAD, UnloadUnit )
ON_BN_CLICKED( IDC_UNIT_RETREAT, RetreatUnit )
ON_BN_CLICKED( IDC_UNIT_REPAIR, RepairUnit )
ON_BN_CLICKED( IDC_UNIT_CANCEL_REPAIR, CancelRepairUnit )
ON_WM_LBUTTONDOWN( )
ON_WM_RBUTTONDOWN( )
ON_WM_RBUTTONUP( )
ON_WM_RBUTTONDBLCLK( )
ON_WM_ACTIVATE( )
ON_WM_LBUTTONDBLCLK( )
ON_WM_LBUTTONUP( )
ON_WM_SETCURSOR( )
ON_WM_KEYUP( )
ON_WM_KEYDOWN( )
ON_COMMAND( IDA_CLOSE_WIN, OnCloseWin )
ON_COMMAND( IDA_DESELECT, OnDeselect )
ON_WM_ERASEBKGND( )
ON_COMMAND( IDA_BUILD, BuildUnit )
ON_COMMAND( IDA_RETREAT, RetreatUnit )
ON_COMMAND( IDA_ROUTE, RouteUnit )
ON_COMMAND( IDA_UNLOAD, UnloadUnit )
ON_WM_CTLCOLOR( )
ON_WM_MOUSEWHEEL( )
ON_WM_MOUSEMOVE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CWndArea message handlers

BOOL CWndArea::OnEraseBkgnd( CDC* )
{

    return TRUE;
}

BOOL CWndArea::OnSetCursor( CWnd* pWnd, UINT nHitTest, UINT message )
{

    if ( ( pWnd->GetSafeHwnd() != m_hWnd ) || ( nHitTest != HTCLIENT ) )
        return CWndAnim::OnSetCursor( pWnd, nHitTest, message );

    SetMouseState( );
    return ( TRUE );
}

void CWndArea::CurUp( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( 0, -rect.Height( ) / 4 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::CurRight( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( rect.Width( ) / 4, 0 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::CurDown( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( 0, rect.Height( ) / 4 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::CurLeft( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( -rect.Width( ) / 4, 0 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::ReRender( )
{
    Perf::ScopeCounter _crr( "rr.area" );   // PROFILE: area-map ReRender cost (r.inval split)

    CDIB* pdib = m_aa.m_dibwnd.GetDIB( );
    CRect rect;
    GetClientRect( &rect );

    // if the pan button (MMB) is held & near the edge then we continuously scroll
    if ( m_bPanBtnDown )
    {
        CPoint pt;
        ::GetCursorPos( &pt );
        ScreenToClient( &pt );

        int x, y;
        int iWid = rect.Width( );
        if ( pt.x < iWid / 8 )
            x = -( iWid / 8 - pt.x );
        else
        {
            int iTmp = iWid - iWid / 8;
            if ( pt.x > iTmp )
                x = pt.x - iTmp;
            else
                x = 0;
        }
        int iHt = rect.Height( );
        if ( pt.y < iHt / 8 )
            y = -( iHt / 8 - pt.y );
        else
        {
            int iTmp = iHt - iHt / 8;
            if ( pt.y > iTmp )
                y = pt.y - iTmp;
            else
                y = 0;
        }

        // which cursor?
        switch ( ( __minmax( -1, 1, x ) + 2 ) | ( ( __minmax( -1, 1, y ) + 2 ) << 2 ) )
        {
        case 0x06:  // up
            m_iMoveCur = 0;
            break;
        case 0x07:  // UR
            m_iMoveCur = 1;
            break;
        case 0x0B:  // right
            m_iMoveCur = 2;
            break;
        case 0x0F:  // LR
            m_iMoveCur = 3;
            break;
        case 0x0E:  // down
            m_iMoveCur = 4;
            break;
        case 0x0D:  // LL
            m_iMoveCur = 5;
            break;
        case 0x09:  // left
            m_iMoveCur = 6;
            break;
        case 0x05:  // UL
            m_iMoveCur = 7;
            break;
        default:
            m_iMoveCur = 8;
            break;
        }
        AreaApplyCursor( m_hCurMove[m_iMoveCur] );

        if ( ( x != 0 ) || ( y != 0 ) )
        {
            m_aa.MoveCenterPixels( x * 2, y * 2 );
            // GGTESTING			InvalidateWindow();
            theApp.m_wndWorld.NewLocation( );
        }
    }

    bInvAmb = FALSE;

    // GPU full-redraw: the draw pass re-presents the whole viewport every frame and
    // ignores dirty rects, so theMap.Update's invalidate walk (O(visible hexes), the
    // old per-hex scan) is pure overhead here. Animation/ambients advance in the DRAW
    // pass (CDrawParms::draw "Draw and update ambients"), not in the invalidate pass,
    // so skipping it is safe. This was the #1 render cost at max zoom (r.inval).
    if ( !m_aa.IsGpuFull( ) )
    {
        BOOL bSave;
        if ( m_bUpdateAll )
        {
            bSave      = bForceDraw;
            bForceDraw = TRUE;
        }

        theMap.Update( m_aa );

        // Generate a paint message for each coalesced dirty rect

        // Render each rect in the current dirty rect list and add it to
        // the list of rects to be blitted

        // GGTESTING m_aa.Render(  ); this creates crazy trails on vehicles

        if ( m_bUpdateAll )
        {
            bForceDraw   = bSave;
            m_bUpdateAll = FALSE;
        }
    }

    // unit may have moved under (or been created)
    CPoint pt;
    ::GetCursorPos( &pt );
    if ( ::WindowFromPoint( pt ) == m_hWnd )
    {
        // make sure in client area
        ScreenToClient( &pt );
        if ( ( pt.x >= 0 ) && ( pt.y >= 0 ) && ( pt.x < rect.right ) && ( pt.y < rect.bottom ) )
            SetMouseState( );
    }

    // we handle new sound here since paint's are defered
    if ( m_bNewSound )
    {
        m_bNewSound = FALSE;
        if ( theAreaList.GetTop( ) == this )
            UpdateSound( );
    }
}

// when on (or NULL if off) a unit - shows
void CWndArea::StatUnit( CUnit* pUnit )
{

    // we only see our own
    if ( ( pUnit != NULL ) && ( !pUnit->GetOwner( )->IsMe( ) ) )
        pUnit = NULL;

    if ( pUnit == GetStaticUnit( ) )
        return;

    m_WndStatic.m_wndStat.SetUnit( pUnit );
    m_WndStatic.m_wndStat.InvalidateRect( NULL );
}

static int fnEnumIsVisNoVeh( CHex* pHex, CHexCoord, void* pData )
{

    // must be visible
    if ( ( !pHex->GetVisibility( ) ) || ( pHex->GetUnits( ) & CHex::unit ) )
    {
        *( (BOOL*)pData ) = FALSE;
        return ( TRUE );
    }

    return ( FALSE );
}

void CWndArea::MaterialChange( CUnit const* pUnit )
{

    // see if this is displayed in the tooltip
    if ( m_pSdlInfo && m_pSdlInfo->IsVisible() && m_pSdlInfo->GetUnit() == pUnit )
        m_pSdlInfo->Update();
    CWndInfo* pWndInfo = GetInfo( );
    ASSERT_STRICT_VALID_OR_NULL( pWndInfo );
    if ( ( pWndInfo != NULL ) && ( pWndInfo->m_hWnd != NULL ) && ( pWndInfo->GetUnit( ) == pUnit ) )
        pWndInfo->Refigure( );

    // update buttons
    if ( m_pUnit == pUnit )
        SetButtonState( );

    if ( pUnit != GetStaticUnit( ) )
        return;

    // update the status bars
    m_WndStatic.UpdateStat( );
}

void CWndArea::OnMouseMove( UINT nFlags, CPoint point )
{

    // kill the info window - if they move > 4 pixels
    if ( ( abs( point.x - m_ptRMDN.x ) > theApp.m_iScrnX / 160 ) ||
         ( abs( point.y - m_ptRMDN.y ) > theApp.m_iScrnX / 160 ) )
    {
        if ( m_pSdlInfo && m_pSdlInfo->IsVisible() )
            m_pSdlInfo->Hide();
        if ( ( m_pWndInfo ) && ( m_pWndInfo->m_hWnd != NULL ) )
            m_pWndInfo->DestroyWindow( );
    }

    CRect rect;
    GetClientRect( &rect );

    // sometimes the point is outside the window
    if ( !rect.PtInRect( point ) )
    {
        CWndBase::OnMouseMove( nFlags, point );
        return;
    }

    // drawing the selection box (LMB drag is always a box-select now — line
    // movement lives on the RMB drag below)
    if ( m_iMode == normal_select )
    {
        m_selRect.SetRect( m_selOrig.x, m_selOrig.y, point.x, point.y );
        m_selRect.NormalizeRect( );
        m_selRect &= rect;
    }

    // RMB drag with 2+ units selected = line movement (drawn formation). A plain
    // right-click (no drag) stays a command — DoCommandAt fires on RMB-up.
    if ( m_bRmbCmdDown && ( m_iMode == normal ) )
    {
        BOOL bDragged = ( abs( point.x - m_ptRMDN.x ) >= theMap.HexWid( m_aa.m_iZoom ) / 2 ) ||
                        ( abs( point.y - m_ptRMDN.y ) >= theMap.HexHt( m_aa.m_iZoom ) / 2 );
        if ( bDragged && ( m_lstUnits.GetCount( ) >= 2 ) )
            m_bLineMove = TRUE;

        if ( m_bLineMove )
        {
            // Record the freeform path the cursor traces (throttled), starting at the
            // drag origin. DoLineMove distributes the units along this polyline.
            if ( s_linePath.empty( ) )
                s_linePath.push_back( m_ptRMDN );
            CPoint last = s_linePath.back( );
            if ( abs( point.x - last.x ) + abs( point.y - last.y ) >= 4 )
                s_linePath.push_back( point );
            m_lineEnd = point;
        }
    }

    // if the pan button (MMB) is held we scroll: drag-pan ("grab the map") here,
    // plus the original continuous edge-band scroll in ReRender.
    if ( m_bPanBtnDown )
    {
        ASSERT_STRICT_VALID_STRUCT( &m_aa );

        // grab-style: the content follows the cursor, so the view center moves
        // opposite to the mouse delta
        m_aa.MoveCenterPixels( m_ptRMB.x - point.x, m_ptRMB.y - point.y );

        m_ptRMB = point;
        theApp.m_wndWorld.NewLocation( );
        // Keep the road drag-preview in sync while grab-panning. OnMouseMove returns early
        // here for an MMB pan, so without this the previewed road tiles (swapped into the hex
        // sprites + baked into the GPU terrain cache by SetRoadIcons) are never re-laid or
        // cleared as the view scrolls -> the preview STICKS in the cached terrain (ghost road
        // segments left behind in the trees). Re-lay it at the hex now under the cursor; the
        // ClrRoadIcons at the top of SetRoadIcons restores the previous (now stale) path.
        if ( m_iMode == road_set )
        {
            CHexCoord hcPan( m_aa.GetHit( point )._GetHexCoord( ) );
            hcPan.Wrap( );
            SetRoadIcons( hcPan );
        }
        CWndBase::OnMouseMove( nFlags, point );
        m_bNewPos = TRUE;

        return;
    }

    // where are we
    CHitInfo  hitinfo = m_aa.GetHit( point );
    CHexCoord hexcoord( hitinfo._GetHexCoord( ) );
    hexcoord.Wrap( );
    CHex*  pHex  = theMap._GetHex( hexcoord );
    CUnit* pUnit = hitinfo.GetUnit( );

    if ( m_iMode == road_set )
        SetRoadIcons( hexcoord );

    // if not visible then it's not there
    if ( pUnit != NULL )
        if ( ( ( pUnit->GetUnitType( ) == CUnit::building ) && ( !pUnit->IsVisible( ) ) ) ||
             ( ( pUnit->GetUnitType( ) != CUnit::building ) && ( !pHex->GetVisible( ) ) ) )
            pUnit = NULL;

#ifdef BUGBUG
    // if it's not ours it has to be at alliance
    if ( ( pUnit != NULL ) && ( !pUnit->GetOwner( )->IsMe( ) ) )
        if ( pUnit->GetOwner( )->GetRelations( ) != RELATIONS_ALLIANCE )
            pUnit = NULL;
#endif

#ifdef _CHEAT
    if ( _bShowPos )
    {
        std::string sBuf = IntToStr( hexcoord.X( ) ) + "," + IntToStr( hexcoord.Y( ) ) + " (" +
                           IntToStr( pHex->GetAlt( ) ) + ") ";
        theApp.m_wndBar.SetStatusText( 0, sBuf.c_str( ) );
    }
#endif

    // if no unit show the terrain
    if ( pUnit == NULL )
    {
        theApp.m_wndBar.SetStatusFunc( 1, TerrainShowStatus, pHex );

        // tell them what they can do
        SetStatusText( m_sHelp.c_str( ) );
    }
    else
        // ok, we have a unit
        theApp.m_wndBar.SetStatusFunc( 1, UnitShowStatus, pUnit );

    if ( m_iMode == road_begin )
        theApp.m_wndBar.SetStatusText( 0, m_sHelpRoad.c_str( ) );

    // rocket pos - special test
    BOOL bBuildOk = TRUE;
    if ( ( m_iMode == rocket_ready ) || ( m_iMode == rocket_pos ) )
    {
        // we test to make sure that all vehicles can get out of the rocket
        CHexCoord _hexBuild = ToBuildUL( hexcoord );
        if ( !CStructureData::CanBuild( _hexBuild, GetBuildDir( ), CStructureData::rocket, FALSE, TRUE ) )
            bBuildOk = FALSE;
        else
        {
            CStructureData const* pData = theStructures.GetData( m_iBuild );
            theMap.EnumHexes( _hexBuild, GetBuildDir( ) & 1 ? pData->GetCY( ) : pData->GetCX( ),
                              GetBuildDir( ) & 1 ? pData->GetCX( ) : pData->GetCY( ), fnEnumIsVisNoVeh, &bBuildOk );
        }
    }

    // if we are going to build we talk about the terrain
    if ( ( m_iMode == build_ready ) || ( m_iMode == build_loc ) || ( m_iMode == rocket_ready ) ||
         ( m_iMode == rocket_pos ) )
    {
        CHexCoord _hexBuild = ToBuildUL( hexcoord );
        int       iWhy;
        m_iFound = theMap.FoundationCost( _hexBuild, m_iBuild, GetBuildDir( ), NULL, NULL, &iWhy );
        if ( ( m_iFound < 0 ) || ( !bBuildOk ) )
        {
            theMap.SetBldgCur( _hexBuild, m_iBuild, GetBuildDir( ), 1 );
            if ( ( iWhy > 0 ) && ( iWhy <= 9 ) && ( !m_sHelpCantBuild[iWhy - 1].empty( ) ) )
            {
                TRAP( m_sHelpCantBuild[iWhy - 1].empty( ) );
                theApp.m_wndBar.SetStatusText( 0, m_sHelpCantBuild[iWhy - 1].c_str( ), CStatInst::critical );
            }
            else
                theApp.m_wndBar.SetStatusText( 0, m_sHelpCantBuild[6].c_str( ), CStatInst::critical );
            CWndBaseSuper::OnMouseMove( nFlags, point );
            return;
        }

        int iCurType = m_iFound < 0 ? 1 : 0;
        switch ( m_iBuild )
        {
        case CStructureData::farm:
        case CStructureData::lumber: {
            int                   iMul = CFarmBuilding::LandMult( _hexBuild, m_iBuild, GetBuildDir( ) );
            std::string           sText = "(" + IntToStr( iMul ) + ") ";
            CStatInst::IMPORTANCE iImp;
            if ( ( iMul < 2 ) || ( m_iFound < 0 ) )
            {
                m_iFound = -1;
                iCurType = 1;
                sText += m_sHelpNoFarm;
                iImp = CStatInst::critical;
            }
            else if ( iMul < 5 )
            {
                iCurType = 2;
                sText += m_sHelpBadFarm;
                iImp = CStatInst::warn;
            }
            else
            {
                sText += m_sHelpOkFarm;
                iImp = CStatInst::status;
            }
            theApp.m_wndBar.SetStatusText( 0, sText.c_str( ), iImp );
            break;
        }

        case CStructureData::coal:
        case CStructureData::iron:
        case CStructureData::oil_well:
        case CStructureData::copper: {
            CStructureData const* pData = theStructures.GetData( m_iBuild );
            int                   iSize = pData->GetCX( ) * pData->GetCY( );
            int                   qMul  = CMineBuilding::TotalQuantity( _hexBuild, m_iBuild, GetBuildDir( ) );
            int                   iDiv;
            switch ( m_iBuild )
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
            int dMul = CMineBuilding::TotalDensity( _hexBuild, m_iBuild, GetBuildDir( ) );
            int iDen = ( dMul * 100 ) / ( MAX_MINERAL_DENSITY * iSize );
            if ( dMul > 0 )
                iDen = __max( 1, iDen );

            std::string           sText = "(" + IntToStr( iQuan, 10, true ) + ", " + IntToStr( iDen ) + ") ";
            CStatInst::IMPORTANCE iImp;
            if ( ( qMul < 2 ) || ( m_iFound < 0 ) || ( dMul < 1 ) )
            {
                m_iFound = -1;
                iCurType = 1;
                sText += m_sHelpNoMine;
                iImp = CStatInst::critical;
            }
            else if ( ( iQuan < 400 / ( iSize / 2 ) ) && ( iDen < 40 / ( iSize / 2 ) ) )
            {
                iCurType = 2;
                sText += m_sHelpBadMine;
                iImp = CStatInst::warn;
            }
            else
            {
                sText += m_sHelpOkMine;
                iImp = CStatInst::status;
            }
            theApp.m_wndBar.SetStatusText( 0, sText.c_str( ), iImp );
            break;
        }

        default:
            theApp.m_wndBar.SetStatusText( 0, m_sHelpBuild.c_str( ) );
            break;
        }

        // set the building cursor
        theMap.SetBldgCur( _hexBuild, m_iBuild, GetBuildDir( ), iCurType );
    }

    CWndBaseSuper::OnMouseMove( nFlags, point );
}

//--------------------------------------------------------------------------
// CWndArea::Draw
//--------------------------------------------------------------------------
void CWndArea::Draw( )
{
    m_aa.Render( );

    // Draw any overlays here

    // Subtle dotted lines through selected vehicles' queued routes (only while Shift held).
    DrawRouteWaypoints( );

    // Draw new selection rect (note: doesn't force render of interior)

    if ( m_bLineMove )
    {
        // Drawn-formation preview (RMB drag): dotted line + per-unit destination dots.
        DrawLineMove( );
    }
    else if ( m_iMode == normal_select )
    {
        // GPU split path: m_dibwnd (terrain) is never presented — PresentOwn composites
        // the GPU terrain mesh + the color-keyed sprite layer (m_dibSprite) directly. So
        // the box has to go into m_dibSprite, and via a self-contained draw (no m_pSelUnder
        // save buffer, which is sized for m_dibwnd's format and would overflow).
        if ( m_aa.IsGpuFull( ) )
            DrawSelectionRectGpu( );
        else
            DrawSelectionRect( );

        // Add selection rectangle to the list of rects to get blted this frame

        m_aa.GetDirtyRects( )->AddRect( &m_selRect, CDirtyRects::RECT_LIST::LIST_BLT );
    }

    // Weapon-range overlay (toggled from the building-info window).
    if ( s_dwShowRangeID != 0 )
        DrawRangeCircle( );

    // Blt the dirty rects to the screen

    // How much of the map got repainted this frame. Small = incremental (just moving
    // units/animations); large = full-viewport redraw (scrolling, or something forcing
    // a full invalidate). Pair with ui.dialogs to see whether opening the research
    // window is what pushes the area map into full redraws every frame.
    if ( Perf::IsEnabled() )
        Perf::CounterAdd( "area.paintrects", m_aa.GetDirtyRects( )->m_nRectPaintCur );

    m_aa.GetDirtyRects( )->BltRects( );

    // Re-copy DIB to SDL panel after overlays (selection rect) are drawn.
    // CAnimAtr::Render() copied the DIB before overlays, so we need a second pass.
    if ( m_aa.m_sdlPanel )
        RenderingAdapter::RenderToPanel( &m_aa, m_aa.m_sdlPanel );

    // Sync area static bar position and z-order to area panel
    // Resize the bar panel surface to match the area panel width
    if ( m_aa.m_sdlPanel && m_WndStatic.m_sdlPanel ) {
        int staticH = m_WndStatic.m_iYmin;
        m_WndStatic.m_sdlPanel->SetSize(
            m_aa.m_sdlPanel->GetWidth(), staticH );
    }
    if ( m_WndStatic.m_sdl2Bar )
    {
        m_WndStatic.m_sdl2Bar->Render();
        // Blit the bar into the bottom of the area panel (its only display location).
        if ( m_aa.m_sdlPanel && m_WndStatic.m_sdlPanel )
        {
            SDL_Surface* barSurf  = m_WndStatic.m_sdlPanel->GetSurface();
            SDL_Surface* areaSurf = m_aa.m_sdlPanel->GetSurface();
            if ( barSurf && areaSurf )
            {
                int barOffY = m_aa.m_sdlPanel->GetHeight() - m_WndStatic.m_sdlPanel->GetHeight();
                if ( barOffY >= 0 )
                {
                    SDL_Rect dst = { 0, barOffY, barSurf->w, barSurf->h };
                    SDL_BlitSurface( barSurf, nullptr, areaSurf, &dst );
                    m_aa.m_sdlPanel->SetDirty();
                }
            }
        }
    }
    else
        m_WndStatic.CaptureToPanel();

    // Erase the selection rect (blted next frame)

    // (line-move draws into m_dibSprite/m_dibwnd and clears itself — see DrawLineMove)
    if ( ( m_iMode == normal_select ) && !m_bLineMove )
    {
        // GPU split path: the rect was drawn into m_dibSprite, which is fully
        // cleared+regenerated by the next frame's Render() — so there is nothing to
        // restore (and restoring here would erase the rect before Composite()/PresentOwn
        // reads m_dibSprite live, making the box invisible). The CPU path still needs
        // the restore so its working DIB is clean for the next render.
        if ( !m_aa.IsGpuFull( ) )
        {
            RestoreSelectionRect( );  // note: doesn't force render of interior

            // Add selection rectangle to the list of rects to get blted next frame

            m_aa.GetDirtyRects( )->AddRect( &m_selRect, CDirtyRects::RECT_LIST::LIST_BLT );
        }
    }

    // Fill the backbuffer with magenta (for testing)

#ifdef bugbug_CHEAT
    if ( _bClearWindow )
        m_aa.m_dibwnd.GetDIB( )->Clear( NULL, 253 );
#endif
}

void CWndArea::OnPaint( )
{
    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    // Render the rect

    CRect rect    = dc.m_ps.rcPaint;
    CRect rectDIB = rect & m_aa.m_dibwnd.GetRect( );

    if ( !rectDIB.IsRectEmpty( ) )
    {
        // Render the rect
        theMap.UpdateRect( m_aa, rectDIB, CDrawParms::draw );

        // Paint the rect
        m_aa.m_dibwnd.Paint( &rectDIB );
    }

    if ( m_bScrollBars )
    {
        // Paint the rect where the scroll bars meet

        CRect rectClient;

        GetClientRect( (RECT*)&rectClient );

        rect.SetRect( 0, 0, GetSystemMetrics( SM_CXVSCROLL ), GetSystemMetrics( SM_CYHSCROLL ) );
        rect += rectClient.BottomRight( );

        COLORREF colorref = PALETTERGB( 230, 180, 115 );

        dc.FillSolidRect( &rect, colorref );
    }

    // Do not call CWndBase::OnPaint() for painting messages

    thePal.EndPaint( dc.m_hDC );
}

//---------------------------------------------------------------------------
// CWndArea::DrawSelectionRect
// this now does an XOR. It can be called fresh on a newly rendered
// bitmap or it can be called with the old rect to undo and then the new rect
// to do it
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// CWndArea::DrawSelectionRectGpu
// GPU split path: draw the selection box directly into the color-keyed sprite
// layer (m_dibSprite), which is what PresentOwn composites over the GPU terrain
// mesh. No pixel save/restore — m_dibSprite is fully cleared+regenerated every
// frame (CAnimAtr::Render full-viewport pass), so the box vanishes on its own
// next frame. Bounded entirely within the DIB (m_selRect is clamped to the
// client rect in OnMouseMove), so there is no external buffer to overflow.
//---------------------------------------------------------------------------
void CWndArea::DrawSelectionRectGpu( )
{
    CDIB* pdib = m_aa.m_dibSprite.GetDIB( );
    if ( pdib == NULL )
        return;

    int     iBpp  = pdib->GetBytesPerPixel( );
    CDIBits bits  = pdib->GetBits( );

    int iLeft = m_selRect.left, iTop = m_selRect.top;
    int iWid  = m_selRect.Width( ), iHt = m_selRect.Height( );
    if ( iWid <= 0 || iHt <= 0 )
        return;

    BYTE const* pColor    = m_colorbuffer.GetBuffer( Max( iWid, SEL_WIDTH ) );
    int         iWidBytes = iWid * iBpp;

    // top + bottom horizontal bands (SEL_WIDTH rows each)
    for ( int r = 0; r < SEL_WIDTH && r < iHt; ++r )
    {
        memcpy( bits + pdib->GetOffset( iLeft, iTop + r ), pColor, iWidBytes );

        int rb = iTop + iHt - 1 - r;
        if ( rb > iTop + r )
            memcpy( bits + pdib->GetOffset( iLeft, rb ), pColor, iWidBytes );
    }

    // left + right vertical bands for the interior rows
    int iColBytes  = Min( iWid, SEL_WIDTH ) * iBpp;
    int iRightOff  = iWidBytes - iColBytes;
    for ( int r = SEL_WIDTH; r < iHt - SEL_WIDTH; ++r )
    {
        BYTE* pRow = bits + pdib->GetOffset( iLeft, iTop + r );
        memcpy( pRow, pColor, iColBytes );
        if ( iRightOff > 0 )
            memcpy( pRow + iRightOff, pColor, iColBytes );
    }
}

//---------------------------------------------------------------------------
// CWndArea::DrawLineMove
// Drawn-formation preview: a dotted line from the drag origin to the cursor, plus
// a brighter dot at each selected vehicle's computed destination. Drawn into the
// same layer as the selection box (m_dibSprite in GPU mode, m_dibwnd otherwise) so
// it composites on top. GPU regenerates that layer every frame; the software path
// is force-repainted (m_bUpdateAll) to erase the previous frame's line.
//---------------------------------------------------------------------------
void CWndArea::DrawLineMove( )
{
    bool  bGpu = m_aa.IsGpuFull( );
    CDIB* pdib = bGpu ? m_aa.m_dibSprite.GetDIB( ) : m_aa.m_dibwnd.GetDIB( );
    if ( pdib == NULL )
        return;

    int     iBpp = pdib->GetBytesPerPixel( );
    CDIBits bits = pdib->GetBits( );
    int     W    = pdib->GetWidth( );
    int     H    = pdib->GetHeight( );

    BYTE const* pColor = m_colorbuffer.GetBuffer( 8 );  // index-255 (white); >= max dot width

    // plot a filled square of `sz` px centred at (cx,cy), clipped to the DIB
    auto plot = [&]( int cx, int cy, int sz )
    {
        int x0 = cx - sz / 2, y0 = cy - sz / 2;
        int x1 = Max( 0, x0 ), x2 = Min( W, x0 + sz );
        if ( x2 <= x1 )
            return;
        int wBytes = ( x2 - x1 ) * iBpp;
        for ( int yy = Max( 0, y0 ); yy < Min( H, y0 + sz ); ++yy )
            memcpy( bits + pdib->GetOffset( x1, yy ), pColor, wBytes );
    };

    // dotted freeform line: walk the captured polyline by arc length
    float total = LinePathLength( );
    if ( total < 1 )
        total = 1;
    for ( float s = 0; s <= total; s += 7.0f )
    {
        CPoint p = LinePathAt( s );
        plot( p.x, p.y, 2 );
    }

    // one destination dot per selected vehicle, evenly spaced ALONG the curve
    int n = 0;
    for ( POSITION pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        if ( m_lstUnits.GetNext( pos )->GetUnitType( ) == CUnit::vehicle )
            ++n;
    for ( int i = 0; i < n; ++i )
    {
        float s = ( n == 1 ) ? total : total * i / ( n - 1 );
        CPoint p = LinePathAt( s );
        plot( p.x, p.y, 5 );
    }

    if ( !bGpu )
        m_bUpdateAll = TRUE;  // software path: clear this frame's line next frame
}

//---------------------------------------------------------------------------
// CWndArea::DrawRouteWaypoints
// While Shift is held, draw a SUBTLE dotted line through each selected vehicle's queued
// route (vehicle -> wp1 -> wp2 -> ...) so the player sees the order being built. Drawn into
// the same overlay layer as DrawLineMove (m_dibSprite in GPU mode), regenerated each frame.
//---------------------------------------------------------------------------
void CWndArea::DrawRouteWaypoints( )
{
    if ( !( GetKeyState( VK_SHIFT ) & ~1 ) )   // only while Shift is held
        return;
    if ( m_lstUnits.GetCount( ) == 0 )
        return;

    bool  bGpu = m_aa.IsGpuFull( );
    CDIB* pdib = bGpu ? m_aa.m_dibSprite.GetDIB( ) : m_aa.m_dibwnd.GetDIB( );
    if ( pdib == NULL )
        return;

    int     iBpp = pdib->GetBytesPerPixel( );
    CDIBits bits = pdib->GetBits( );
    int     W    = pdib->GetWidth( );
    int     H    = pdib->GetHeight( );

    BYTE const* pColor = m_colorbuffer.GetBuffer( 4 );   // white; subtlety from sparse 1px dots

    auto plot = [&]( int cx, int cy, int sz )
    {
        int x0 = cx - sz / 2, y0 = cy - sz / 2;
        int x1 = Max( 0, x0 ), x2 = Min( W, x0 + sz );
        if ( x2 <= x1 ) return;
        int wBytes = ( x2 - x1 ) * iBpp;
        for ( int yy = Max( 0, y0 ); yy < Min( H, y0 + sz ); ++yy )
            memcpy( bits + pdib->GetOffset( x1, yy ), pColor, wBytes );
    };

    // dotted segment between two window points — subtle: 1px dots ~8px apart, no sqrt
    auto seg = [&]( CPoint a, CPoint b )
    {
        int dx = b.x - a.x, dy = b.y - a.y;
        int steps = Max( Max( abs( dx ), abs( dy ) ) / 8, 1 );
        for ( int i = 0; i <= steps; ++i )
            plot( a.x + dx * i / steps, a.y + dy * i / steps, 1 );
    };

    // project a hex to its window-space centre (average of the 4 corners)
    auto hexWin = [&]( CHexCoord hc ) -> CPoint
    {
        CPoint p[4];
        m_aa.MapToWindowHex( hc, p );
        return CPoint( ( p[0].x + p[1].x + p[2].x + p[3].x ) / 4,
                       ( p[0].y + p[1].y + p[2].y + p[3].y ) / 4 );
    };

    // TORUS WRAP: the map repeats, so a destination hex has many valid window positions
    // (one per wrap period). Stepping a full map width/height shifts the projection by a
    // constant vector (the iso transform is affine; CHexCoord extends linearly past the
    // edge). Compute those period vectors in window px, then snap each segment endpoint to
    // the COPY nearest the start — so a seam-crossing move draws the SHORT path over the
    // wrap, not a line clear across the planet. (operator: shift-preview ignored wrapping.)
    const int    eX   = theMap.Get_eX( ), eY = theMap.Get_eY( );
    const CPoint perO = hexWin( CHexCoord( 0,  0  ) );
    const CPoint perPX= hexWin( CHexCoord( eX, 0  ) );
    const CPoint perPY= hexWin( CHexCoord( 0,  eY ) );
    const int    perXx = perPX.x - perO.x, perXy = perPX.y - perO.y;
    const int    perYx = perPY.x - perO.x, perYy = perPY.y - perO.y;
    auto wrapNear = [&]( CPoint wp, CPoint ref ) -> CPoint
    {
        CPoint best = wp;
        long   bestD = (long)( wp.x - ref.x ) * ( wp.x - ref.x ) + (long)( wp.y - ref.y ) * ( wp.y - ref.y );
        for ( int a = -2; a <= 2; ++a )
            for ( int b = -2; b <= 2; ++b )
            {
                if ( a == 0 && b == 0 ) continue;
                long x = (long)wp.x + (long)a * perXx + (long)b * perYx;
                long y = (long)wp.y + (long)a * perXy + (long)b * perYy;
                long dx = x - ref.x, dy = y - ref.y, d = dx * dx + dy * dy;
                if ( d < bestD ) { bestD = d; best = CPoint( (int)x, (int)y ); }
            }
        return best;
    };

    for ( POSITION pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        if ( pUnit->GetUnitType( ) != CUnit::vehicle )
            continue;
        CVehicle* pVeh = (CVehicle*)pUnit;

        // start at the vehicle's current screen position
        CPoint prev = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( pVeh->GetWorldPixels( ) ) );

        // No queued route? If the vehicle is moving to a DIRECT destination (a normal
        // right-click or a line-move — these use SetDest, not a route), preview that single
        // leg veh -> dest. (#7 / operator Note 20: the Shift-preview must also show direct
        // moves, not only queued routes.)
        if ( pVeh->GetRouteList( ).GetCount( ) == 0 )
        {
            if ( pVeh->GetRouteMode( ) == CVehicle::moving )   // VEH_MODE — codebase idiom (area.cpp:6479, caitmgr, projbase)
            {
                CPoint wp = wrapNear( hexWin( pVeh->GetHexDest( ) ), prev );   // torus: short path
                seg( prev, wp );
                plot( wp.x, wp.y, 3 );
            }
            continue;
        }

        for ( POSITION rp = pVeh->GetRouteList( ).GetHeadPosition( ); rp != NULL; )
        {
            CRoute* pR = pVeh->GetRouteList( ).GetNext( rp );
            CPoint  wp = wrapNear( hexWin( pR->GetCoord( ) ), prev );   // torus: short path over the seam
            seg( prev, wp );
            plot( wp.x, wp.y, 3 );   // a slightly bigger dot marks each waypoint
            prev = wp;
        }
    }

    if ( !bGpu )
        m_bUpdateAll = TRUE;
}

//---------------------------------------------------------------------------
// CWndArea::DoLineMove
// Distribute the selected vehicles evenly along the freeform drawn path (s_linePath)
// and issue a move order to each. Units are ordered by where they project onto the
// path (arc length) so the formation doesn't cross over itself. Each order goes
// through SetDest (CMsgVehSetDest -> server), so this is multiplayer-safe.
//---------------------------------------------------------------------------
void CWndArea::DoLineMove( CPoint ptEnd )
{
    // make sure the path includes the final cursor point
    if ( s_linePath.empty( ) )
        s_linePath.push_back( m_ptRMDN );
    if ( s_linePath.back( ) != ptEnd )
        s_linePath.push_back( ptEnd );

    // gather selected vehicles
    std::vector<CVehicle*> vehs;
    for ( POSITION pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
            vehs.push_back( (CVehicle*)pUnit );
    }
    int n = (int)vehs.size( );
    if ( n == 0 )
        return;

    float total = LinePathLength( );

    // order units by where they project onto the path (arc length), so the start of
    // the line gets the unit nearest the start and the formation doesn't cross.
    std::vector<float> key( n );
    for ( int i = 0; i < n; ++i )
    {
        CPoint sp = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( vehs[i]->GetWorldPixels( ) ) );
        key[i] = LinePathClosestArc( sp );
    }
    for ( int i = 0; i < n - 1; ++i )  // selection sort (unit counts are small)
    {
        int iMin = i;
        for ( int j = i + 1; j < n; ++j )
            if ( key[j] < key[iMin] )
                iMin = j;
        if ( iMin != i )
        {
            float tk = key[i]; key[i] = key[iMin]; key[iMin] = tk;
            CVehicle* tv = vehs[i]; vehs[i] = vehs[iMin]; vehs[iMin] = tv;
        }
    }

    // Shift held? a line-move then APPENDS to each unit's order queue (one-shot route)
    // instead of replacing its orders — mirrors the single-point Shift-queue
    // (ShiftQueueMove). (operator Note 27: Shift must queue line-moves too, not only
    // single-point moves.) GetKeyState&~1 = "held" (masks the toggle bit), as in
    // DrawRouteWaypoints.
    const bool bShiftQueue = ( m_bRmbShift != FALSE );   // Shift captured at RMB-press (set by OnRButtonDown)

    // emit one move per unit, spread evenly by arc length along the drawn path
    for ( int i = 0; i < n; ++i )
    {
        float   s = ( n == 1 ) ? total : total * i / ( n - 1 );
        CPoint  p = LinePathAt( s );

        CSubHex sub = m_aa.WindowToSubHex( p );
        sub.Wrap( );

        CVehicle* pVeh = vehs[i];
        if ( bShiftQueue )
        {
            ShiftQueueMove( pVeh, sub );   // Note 27: append this unit's line dest to its queue
        }
        else
        {
            pVeh->TempTargetOff( );
            pVeh->SetEvent( CVehicle::none );
            pVeh->ResumeUnit( );
            StopRoute( pVeh );   // #6: a manual line-move stops/overrides any active route (incl. loop/haul)
            SetDestAndSfx( pVeh, sub );
            pVeh->_SetTarget( NULL );
        }
    }
}

// Red circle around the building whose range is being visualized (s_dwShowRangeID).
// Drawn into the same layer as the other overlays (m_dibSprite in GPU mode, m_dibwnd
// otherwise). Radius = weapon range (hexes) * hex width (pixels) at the current zoom.
DWORD CWndArea::s_dwShowRangeID = 0;

void CWndArea::DrawRangeCircle( )
{
    CBuilding* pBldg = theBuildingMap.GetBldg( s_dwShowRangeID );
    if ( ( pBldg == NULL ) || ( !pBldg->IsVisible( ) ) || ( pBldg->GetRange( ) <= 0 ) )
    {
        s_dwShowRangeID = 0;   // gone / no weapon -> stop showing
        return;
    }

    bool  bGpu = m_aa.IsGpuFull( );
    CDIB* pdib = bGpu ? m_aa.m_dibSprite.GetDIB( ) : m_aa.m_dibwnd.GetDIB( );
    if ( pdib == NULL )
        return;

    int     iBpp = pdib->GetBytesPerPixel( );
    CDIBits bits = pdib->GetBits( );
    int     W    = pdib->GetWidth( );
    int     H    = pdib->GetHeight( );

    CPoint c   = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( pBldg->GetWorldPixels( ) ) );
    // #61 (operator): scale the drawn radius to 80% (20% smaller). The raw GetRange()*HexWid read
    // as too large in-game across repeated checks; this matches the operator's eye. Whole ellipse
    // (glow band + edge ring below) derives from `rad`, so it shrinks uniformly.
    int    rad = ( pBldg->GetRange( ) * theMap.HexWid( m_aa.m_iZoom ) * 4 ) / 5;
    if ( rad < 1 )
        return;

    // Draw the range as the EDGE OF A SHOCKWAVE: a WIDE band that is brightest at the
    // true range and fades INWARD over a long thickness. The overlay layer composites
    // with a color key (no per-pixel alpha), so the fade can't be true blending — it's
    // faked two ways at once: a dim red shade that lightens toward the edge, AND a fine
    // 8x8 ordered (Bayer) dither that drops more and more pixels toward the inside, so
    // the map shows through like a smooth gradient. Only the annulus is touched.
    int T = rad / 3;                       // wide, soft band
    if ( T < 20 ) T = 20;
    if ( T > 40 ) T = 40;
    int innerR = rad - T;
    if ( innerR < 0 ) innerR = 0;
    double outerR2 = (double)rad * rad;
    double innerR2 = (double)innerR * innerR;

    // Dim red ramp (kept well below full brightness so it reads as a soft glow, not a
    // hard line). 1 palette lookup per step.
    const int NSHADE = 8;
    DWORD shade[NSHADE];
    for ( int k = 0; k < NSHADE; k++ ) {
        int r = 36 + ( 120 * k ) / ( NSHADE - 1 );   // 36 .. 156, intentionally muted
        int g = ( r * 20 ) / 255;
        shade[k] = thePal.GetColorValue( PALETTERGB( r, g, g ), iBpp * 8 );
    }
    static const int kBayer8[8][8] = {
        {  0, 32,  8, 40,  2, 34, 10, 42 }, { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 }, { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 }, { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 }, { 63, 31, 55, 23, 61, 29, 53, 21 },
    };

    // Hexes are 2:1 in pixels (HexWid = 2*HexHt, terrain.h:80-81; horiz pitch HexWid vs
    // vert pitch HexHt, terrain.cpp:982). GetRange() hexes is `rad` px wide but only `rad/2`
    // px tall, so draw an ELLIPSE (vertical radius rad/2) — an isotropic circle reached
    // GetRange()*HexWid vertically = ~2x the real range = "show range too large" (G1).
    int radY = ( rad + 1 ) / 2;
    int x0 = c.x - rad,  x1 = c.x + rad;
    int y0 = c.y - radY, y1 = c.y + radY;
    if ( x0 < 0 ) x0 = 0;  if ( x1 >= W ) x1 = W - 1;
    if ( y0 < 0 ) y0 = 0;  if ( y1 >= H ) y1 = H - 1;

    for ( int y = y0; y <= y1; y++ )
    {
        double dy = (double)( y - c.y ) * 2.0;   // compress vertical to the 2:1 hex aspect (G1)
        for ( int x = x0; x <= x1; x++ )
        {
            double dx = (double)( x - c.x );
            double d2 = dx * dx + dy * dy;
            if ( d2 > outerR2 || d2 < innerR2 ) continue;
            double dist = sqrt( d2 );
            float  t    = (float)( ( dist - innerR ) / ( T > 0 ? T : 1 ) );   // 0 inner .. 1 edge
            if ( t < 0 ) t = 0; if ( t > 1 ) t = 1;
            float thr = ( kBayer8[y & 7][x & 7] + 0.5f ) / 64.0f;
            if ( t < thr ) continue;   // dither: sparser (more map showing) toward the inside
            int k = (int)( t * ( NSHADE - 1 ) + 0.5f );
            if ( k < 0 ) k = 0; if ( k >= NSHADE ) k = NSHADE - 1;
            memcpy( bits + pdib->GetOffset( x, y ), &shade[k], iBpp );
        }
    }

    // A crisp 1px edge ring at the exact range anchors the soft glow so it reads as a
    // deliberate boundary rather than noise. Kept medium (not full) brightness.
    {
        DWORD edge = thePal.GetColorValue( PALETTERGB( 175, 32, 32 ), iBpp * 8 );
        auto put = [&]( int px, int py ) {
            if ( px < 0 || px >= W || py < 0 || py >= H ) return;
            memcpy( bits + pdib->GetOffset( px, py ), &edge, iBpp );
        };
        // #61/G1: draw the crisp edge as a midpoint ELLIPSE (x-radius rad, y-radius radY) so it
        // matches the 2:1 hex aspect of the soft glow band above. The old code here was a plain
        // Bresenham CIRCLE (radius rad in BOTH axes), so the ring sat at full `rad` vertically
        // (~2x the real range) = the operator's "thin outer circle that's mostly wrong" sitting
        // outside the correct (compressed) glow ellipse. An ellipse at radY coincides with the
        // glow's outer edge ((x)^2 + (2y)^2 = rad^2), leaving a single correct boundary.
        long rx2 = (long)rad * rad, ry2 = (long)radY * radY;
        long ex = 0, ey = radY;
        long dx = 0, dy = 2 * rx2 * ey;
        auto put4 = [&]( long ix, long iy ) {
            put( c.x + (int)ix, c.y + (int)iy ); put( c.x - (int)ix, c.y + (int)iy );
            put( c.x + (int)ix, c.y - (int)iy ); put( c.x - (int)ix, c.y - (int)iy );
        };
        double p1 = (double)ry2 - (double)rx2 * radY + 0.25 * rx2;   // region 1 (|slope| < 1)
        while ( dx < dy ) {
            put4( ex, ey );
            ex++; dx += 2 * ry2;
            if ( p1 < 0 ) p1 += ry2 + dx;
            else { ey--; dy -= 2 * rx2; p1 += ry2 + dx - dy; }
        }
        double p2 = (double)ry2 * ( ex + 0.5 ) * ( ex + 0.5 )
                    + (double)rx2 * ( ey - 1 ) * ( ey - 1 ) - (double)rx2 * ry2;   // region 2
        while ( ey >= 0 ) {
            put4( ex, ey );
            ey--; dy -= 2 * rx2;
            if ( p2 > 0 ) p2 += rx2 - dy;
            else { ex++; dx += 2 * ry2; p2 += rx2 - dy + dx; }
        }
    }

    if ( !bGpu )
        m_bUpdateAll = TRUE;   // software path: erase this frame's circle next frame
}

void CWndArea::DrawSelectionRect( )
{

    if ( m_pSelUnder == NULL )
    {
        TRAP( );
        return;
    }

    CDIB* pdib = m_aa.m_dibwnd.GetDIB( );

    // show the select rect
    int     iBytesPerPixel = pdib->GetBytesPerPixel( );
    CDIBits dibits         = pdib->GetBits( );

    BYTE* pbyDst         = dibits + pdib->GetOffset( m_selRect.left, m_selRect.top );
    int   iWid           = m_selRect.Width( );
    int   iWidBytes      = iWid * iBytesPerPixel;
    int   iSelWidthBytes = SEL_WIDTH * iBytesPerPixel;
    int   iHt            = m_selRect.Height( );
    int   iAdd           = pdib->GetDirPitch( );

    // grab underlying pixels as we go
    BYTE* pUnder        = m_pSelUnder;
    int   iAddLineUnder = iWid * iBytesPerPixel;

    BYTE const* pbyColorBuf = m_colorbuffer.GetBuffer( Max( iWid, SEL_WIDTH ) );

    int iNum = SEL_WIDTH;

    while ( ( iNum > 0 ) && ( iHt > 0 ) )
    {
        memcpy( pUnder, pbyDst, iAddLineUnder );
        pUnder += iAddLineUnder;

        memcpy( pbyDst, pbyColorBuf, iWidBytes );
        pbyDst += iAdd;
        iNum--;
        iHt--;
    }

    int iAddColUnderLeft  = iBytesPerPixel * Min( iWid, SEL_WIDTH );
    int iAddColUnderRight = iBytesPerPixel * Min( iWid - SEL_WIDTH, SEL_WIDTH );
    int iOff              = iWidBytes - iAddColUnderRight;

    while ( iHt-- > SEL_WIDTH )
    {
        memcpy( pUnder, pbyDst, iAddColUnderLeft );
        pUnder += iAddColUnderLeft;
        memcpy( pbyDst, pbyColorBuf, iAddColUnderLeft );

        if ( iAddColUnderRight > 0 )
        {
            memcpy( pUnder, pbyDst + iOff, iAddColUnderRight );
            pUnder += iAddColUnderRight;

            memcpy( pbyDst + iOff, pbyColorBuf, iAddColUnderRight );
        }

        pbyDst += iAdd;
    }

    while ( iHt-- >= 0 )
    {
        memcpy( pUnder, pbyDst, iAddLineUnder );
        pUnder += iAddLineUnder;

        memcpy( pbyDst, pbyColorBuf, iWidBytes );
        pbyDst += iAdd;
    }
}

// restore the pixels under the selection rectangle
void CWndArea::RestoreSelectionRect( )
{
    if ( m_pSelUnder == NULL )
    {
        TRAP( );
        return;
    }

    CDIB* pdib = m_aa.m_dibwnd.GetDIB( );

    int     iBytesPerPixel = pdib->GetBytesPerPixel( );
    CDIBits dibits         = pdib->GetBits( );

    BYTE* pbyDst         = dibits + pdib->GetOffset( m_selRect.left, m_selRect.top );
    int   iWid           = m_selRect.Width( );
    int   iHt            = m_selRect.Height( );
    int   iWidBytes      = iWid * iBytesPerPixel;
    int   iSelWidthBytes = SEL_WIDTH * iBytesPerPixel;
    int   iAdd           = pdib->GetDirPitch( );

    // restore the underlying pixels
    BYTE* pUnder        = m_pSelUnder;
    int   iAddLineUnder = iWidBytes;

    int iNum = SEL_WIDTH;
    while ( ( iNum > 0 ) && ( iHt > 0 ) )
    {
        memcpy( pbyDst, pUnder, iAddLineUnder );
        pUnder += iAddLineUnder;

        pbyDst += iAdd;
        iNum--;
        iHt--;
    }

    int iAddColUnderLeft  = iBytesPerPixel * Min( iWid, SEL_WIDTH );
    int iAddColUnderRight = iBytesPerPixel * Min( iWid - SEL_WIDTH, SEL_WIDTH );
    int iOff              = iWidBytes - iAddColUnderRight;

    while ( iHt-- > SEL_WIDTH )
    {
        memcpy( pbyDst, pUnder, iAddColUnderLeft );
        pUnder += iAddColUnderLeft;

        if ( iAddColUnderRight > 0 )
        {
            memcpy( pbyDst + iOff, pUnder, iAddColUnderRight );
            pUnder += iAddColUnderRight;
        }

        pbyDst += iAdd;
    }

    while ( iHt-- >= 0 )
    {
        memcpy( pbyDst, pUnder, iAddLineUnder );
        pUnder += iAddLineUnder;
        pbyDst += iAdd;
    }
}

void CWndArea::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{

    int X = 0;

    switch ( nSBCode )
    {
    // line - move 1 hex
    case SB_LINELEFT:
        X--;
        break;
    case SB_LINERIGHT:
        X++;
        break;

    // page - move 1/2 window
    case SB_PAGELEFT: {
        CRect rect;
        GetClientRect( &rect );
        X = -( ( rect.Width( ) + CGameMap::HexWid( m_aa.m_iZoom ) / 2 ) / CGameMap::HexWid( m_aa.m_iZoom ) + 1 ) / 2;
        if ( X > -2 )
            X = -2;
        break;
    }
    case SB_PAGERIGHT: {
        CRect rect;
        GetClientRect( &rect );
        X = ( ( rect.Width( ) + CGameMap::HexWid( m_aa.m_iZoom ) / 2 ) / CGameMap::HexWid( m_aa.m_iZoom ) + 1 ) / 2;
        if ( X < 2 )
            X = 2;
        break;
    }

        // BUGBUG		case SB_THUMBTRACK :
    case SB_THUMBPOSITION: {
        CSize szSize = theMap.GetSize( );
        X            = -(int)( ( szSize.cx - 1 ) / 2 - nPos );
        break;
    }

    // move to the end - go to the exact oppisate end of the map
    case SB_LEFT:
    case SB_RIGHT: {
        CSize szSize = theMap.GetSize( );
        X            = szSize.cx / 2;
        break;
    }
    }

    if ( X )
    {
        m_aa.MoveCenterViewHexes( X, 0 );
        // GGTESTING		InvalidateWindow ();
        theApp.m_wndWorld.NewLocation( );
    }

    CWndBase::OnHScroll( nSBCode, nPos, pScrollBar );
}

void CWndArea::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{

    int Y = 0;

    switch ( nSBCode )
    {
    // line - move 1 hex
    case SB_LINEDOWN:
        Y--;
        break;
    case SB_LINEUP:
        Y++;
        break;

    // page - move 1/2 window
    case SB_PAGEDOWN: {
        CRect rect;
        GetClientRect( &rect );
        Y = -( ( rect.Height( ) + CGameMap::HexHt( m_aa.m_iZoom ) / 2 ) / CGameMap::HexHt( m_aa.m_iZoom ) + 1 ) / 2;
        if ( Y > -2 )
            Y = -2;
        break;
    }
    case SB_PAGEUP: {
        CRect rect;
        GetClientRect( &rect );
        Y = ( ( rect.Height( ) + CGameMap::HexHt( m_aa.m_iZoom ) / 2 ) / CGameMap::HexHt( m_aa.m_iZoom ) + 1 ) / 2;
        if ( Y < 2 )
            Y = 2;
        break;
    }

        // BUGBUG		case SB_THUMBTRACK :
    case SB_THUMBPOSITION: {
        CSize szSize = theMap.GetSize( );
        Y            = (int)( ( szSize.cy - 1 ) / 2 - nPos );
        break;
    }

    // move to the end - go to the exact oppisate end of the map
    case SB_TOP:
    case SB_BOTTOM: {
        CSize szSize = theMap.GetSize( );
        Y            = szSize.cy / 2;
        break;
    }
    }

    if ( Y )
    {
        m_aa.MoveCenterViewHexes( 0, -Y );
        // GGTESTING		InvalidateWindow ();
        theApp.m_wndWorld.NewLocation( );
    }

    CWndBase::OnVScroll( nSBCode, nPos, pScrollBar );
}


int CWndArea::OnCreate( LPCREATESTRUCT lpCreateStruct )
{

    CWndAnim::OnCreate( lpCreateStruct );

    m_bScrollBars = EnGetProfileInt( "Advanced", "Scroll", 0 );

    // NOTE: this crashes on load game because? something? wasn't initialized
    // if first window AND have placement info - use it
    BOOL bPlaceIt = ( ( theAreaList.GetCount( ) == 0 ) 
        && ( theGame.m_wpArea.length != 0 ) );

#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "CWndArea::OnCreate: %d\n", bPlaceIt );
    OutputDebugStringA( buf );
#endif
    // override because its crashing on load:
   // bPlaceIt = FALSE;

    tShowStat.Init( );
    uShowStat.Init( );

    // accelerators for this window
    m_hAccel = ::LoadAccelerators( theApp.m_hInstance, MAKEINTRESOURCE( IDR_AREA ) );

    CRect rect;
    CWndAnim::GetClientRect( &rect );
    // CWndStub::Create bypasses MFC's PreCreate flow, so invoke explicitly
    // before reading PreCreate-computed members (m_iYmin / m_iXmin / button
    // layout positions). Otherwise m_iYmin is uninitialized garbage.
    m_WndStatic.PreCreate();
    rect.top = rect.bottom - m_WndStatic.m_iYmin;
    LPCTSTR sWndCls = CConquerApp::EnRegisterWndClass( "EnAreaStaticWnd",
                          CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, m_hCurLoad[0] );
    m_WndStatic.Create( sWndCls, NULL, dwStatusWndStyle, rect, this, 0, NULL );

    // we had to start with the build icon to get a different class
    ::SetClassLongPtr( m_hWnd, GCLP_HCURSOR, NULL );

    // set up a WinG DC for the client area
    GetClientRect( &rect );
    m_cx = rect.Width( );
    m_cy = rect.Height( );

    ASSERT( ptrthebltformat.Value( ) );

    // this is creating the CDIB for the main area
    // so like, the background for the gameplay window and mini map methinks!)
    // or maybe the entire window thing? only 1 of these exist?
  //  int rWidth = rect.Width( );
  //  int rHeight = rect.Height( );
#ifdef LOGGINGON
    {
        /*
        char buf[256];
        sprintf_s( buf,
                   "Main Area CDIB: Format=%d, Type=%d, Direction=%d, "
                   "Width(local)=%d, Height(local)=%d, Width(rect)=%d, Height(rect)=%d\n",
                   ptrthebltformat->GetColorFormat( ), ptrthebltformat->GetType( ), ptrthebltformat->GetDirection( ),
                   m_cx, m_cy, rect.Width( ), rect.Height( ) );
        OutputDebugStringA( buf );
        */
    }
#endif
    
    m_aa.m_dibwnd.Init(
        this->m_hWnd,
        new CDIB( ptrthebltformat->GetColorFormat( ), ptrthebltformat->GetType( ),
            ptrthebltformat->GetDirection( ) ),
        rect.Width( ), rect.Height( ) );

    // Create SDL2 panel for this area window
    {
        char b[160];
        sprintf_s( b, "[REN] CWndArea::OnCreate panel-create: gameWindow=%p compositor=%p existingPanel=%p\n",
                   (void*)theApp.m_gameWindow.get( ),
                   theApp.m_gameWindow ? (void*)theApp.m_gameWindow->GetCompositor( ) : nullptr,
                   (void*)m_aa.m_sdlPanel );
        OutputDebugStringA( b );
        FILE* _f = fopen( "SDL2Panel.log", "a" ); if ( _f ) { fputs( b, _f ); fclose( _f ); }
    }
    if ( theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        // Get screen position of this window's client area
        CRect screenRect;
        GetWindowRect( &screenRect );

        // Panel name includes the area index for debugging
        char panelName[64];
        sprintf_s( panelName, "area_%d", theAreaList.GetCount() );

        int panelX = screenRect.left;
        int panelY = screenRect.top;
        // Leave room for resize borders and title bar at screen edges
        if (panelX < SDL2Panel::RESIZE_BORDER)
            panelX = SDL2Panel::RESIZE_BORDER;
        int minY = SDL2Panel::TITLE_BAR_HT + SDL2Panel::RESIZE_BORDER;
        if (panelY < minY)
            panelY = minY;
        m_aa.m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(
            panelName, panelX, panelY,
            rect.Width(), rect.Height(), 10 );  // z=10 for area views
        m_aa.m_sdlPanel->SetMovable(true);
        m_aa.m_sdlPanel->SetResizable(true);
        m_aa.m_sdlPanel->SetTitle("Area Map");
        m_aa.m_sdlPanel->SetTerrainAnimAtr(&m_aa);  // T2: GPU terrain mesh source

        // Route SDL events to CWndArea's MFC handler methods
        CWndArea* pThis = this;
        m_aa.m_sdlPanel->SetEventCallback(
            [pThis](SDL_Event& event, int localX, int localY) -> bool {
                // Route events in the area-bar region (bottom of area panel)
                // to the bar handler rather than the map handler.
                if ( pThis->m_WndStatic.m_sdl2Bar && pThis->m_WndStatic.m_sdlPanel )
                {
                    int barH   = pThis->m_WndStatic.m_sdlPanel->GetHeight();
                    int areaH  = pThis->m_aa.m_sdlPanel->GetHeight();
                    int barOffY = areaH - barH;
                    if ( localY >= barOffY )
                        return pThis->m_WndStatic.m_sdl2Bar->HandleEvent(
                            event, localX, localY - barOffY );
                }

                UINT flags = SDLModToMFC();
                CPoint pt(localX, localY);

                switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    // clicking an area window makes it the focused/top one (GetTop)
                    theAreaList.SetTopArea(pThis);
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (event.button.clicks >= 2)
                            pThis->OnLButtonDblClk(flags, pt);
                        else
                            pThis->OnLButtonDown(flags, pt);
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        if (event.button.clicks >= 2)
                            pThis->OnRButtonDblClk(flags, pt);
                        else
                            pThis->OnRButtonDown(flags, pt);
                    } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                        // MMB held = pan (modern binding; RMB is the command button)
                        pThis->OnMButtonDown(flags, pt);
                    }
                    return true;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        pThis->OnLButtonUp(flags, pt);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        pThis->OnRButtonUp(flags, pt);
                    else if (event.button.button == SDL_BUTTON_MIDDLE)
                        pThis->OnMButtonUp(flags, pt);
                    return true;

                case SDL_MOUSEMOTION:
#ifndef _WIN32
                    // Feed the real client mouse pos so SetMouseState's
                    // GetCursorPos()+ScreenToClient() finds the hovered unit/hex
                    // (the stubs returned 0,0 → select/goto/target cursors never showed).
                    en_SetCursorPos(pt.x, pt.y);
#endif
                    pThis->OnMouseMove(flags, pt);
                    pThis->SetMouseState();  // Update cursor & m_uMouseMode (replaces WM_SETCURSOR)
                    // movie.cpp's intro playback calls Win32 ShowCursor(FALSE);
                    // drive the display counter back to 0 so SetMouseState's
                    // AreaApplyCursor() actually shows the game cursor.
                    if (theApp.m_gameWindow)
                        theApp.m_gameWindow->EnsureCursorVisible();
                    return true;

                case SDL_MOUSEWHEEL:
                    pThis->OnMouseWheel(flags, (short)(event.wheel.y * WHEEL_DELTA), pt);
                    return true;

                case SDL_KEYDOWN: {
                    SDL_Scancode sc = event.key.keysym.scancode;
                    // Zoom hotkeys (new binding — the original zoomed via the wheel only):
                    // + / = zooms in, - zooms out, on both the main row and the numpad.
                    if (sc == SDL_SCANCODE_EQUALS || sc == SDL_SCANCODE_KP_PLUS)  { pThis->ZoomIn();  return true; }
                    if (sc == SDL_SCANCODE_MINUS  || sc == SDL_SCANCODE_KP_MINUS) { pThis->ZoomOut(); return true; }

                    // View rotation (new binding — was button-only): , / < turns the
                    // view counter-clockwise, . / > turns it clockwise.
                    if (sc == SDL_SCANCODE_COMMA)  { pThis->TurnCounter(); return true; }
                    if (sc == SDL_SCANCODE_PERIOD) { pThis->TurnClock();   return true; }

                    // Building-placement rotation (new binding — alongside Ctrl+RMB):
                    // [ rotates the footprint CCW, ] rotates it CW. Only acts while
                    // planning a placement; RotateBuildDir no-ops otherwise.
                    if (sc == SDL_SCANCODE_LEFTBRACKET)  { pThis->RotateBuildDir(-1); return true; }
                    if (sc == SDL_SCANCODE_RIGHTBRACKET) { pThis->RotateBuildDir(+1); return true; }

#ifdef _WIN32
                    // Harness (Windows transport): F9 dumps the local player's units via
                    // HarnessDumpUnits (en_harness.h) to OutputDebugString, which dbgcatch
                    // captures — giving the PostMessage/.ps1 harness deterministic unit
                    // screen-xy + crane ID instead of blind pixel-sweeping. Linux/mac use the
                    // control_socket `units` command (non-MSVC) over the SAME shared fn; this
                    // is just the Windows transport. BEGIN/END markers bracket the dump so it
                    // is trivially grep-able in the dbgcatch log.
                    if (sc == SDL_SCANCODE_F9) {
                        std::string dump;
                        HarnessDumpUnits( dump );
                        OutputDebugStringA( "[HUNITS-BEGIN]\n" );
                        OutputDebugStringA( dump.c_str( ) );
                        OutputDebugStringA( "[HUNITS-END]\n" );
                        return true;
                    }
                    // F10: center the area view on my first OWNED crane so it sits at
                    // view-center (clickable) — F9's reported xy is sprite-offset/wrapped,
                    // so the crane-dblclick needs the unit centered first. Arg-less mirror
                    // of control_socket's `center <id>`. Logs id+result to OutputDebugString.
                    if (sc == SDL_SCANCODE_F10) {
                        unsigned long id = 0;
                        POSITION pos = theVehicleMap.GetStartPosition( );
                        while ( pos != NULL ) {
                            DWORD dwID = 0; CVehicle* pVeh = NULL;
                            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                            if ( pVeh && pVeh->GetOwner( ) && pVeh->GetOwner( )->IsMe( )
                                 && pVeh->GetData( ) && pVeh->GetData( )->IsCrane( ) ) { id = dwID; break; }
                        }
                        char msg[96];
                        if ( id ) {
                            bool ok = HarnessCenterUnit( id );
                            snprintf( msg, sizeof( msg ), "[HCENTER] crane id=%lu centered=%d\n", id, ok ? 1 : 0 );
                        } else {
                            snprintf( msg, sizeof( msg ), "[HCENTER] no owned crane found\n" );
                        }
                        OutputDebugStringA( msg );
                        return true;
                    }
#ifdef _CHEAT
                    // F12: DEV cheat — discover ALL research for the local player so the
                    // research-gated tail (AltOutput toggles, fort/seaport/shipyard/embassy,
                    // edicts) is verifiable instantly (no multi-hour grind). Safe-by-construction
                    // per the cheat convention: _CHEAT-gated (Debug/Sanitize only, compiled OUT of
                    // Release) + opt-in via [Cheat]\GrantResearch (EnGetProfileInt default 0) +
                    // SP-only (GetNetNum()==0 — a local research mutation would desync a net game).
                    if (sc == SDL_SCANCODE_F12) {
                        CPlayer* me = theGame.GetMe( );
                        if ( me != NULL && me->GetNetNum( ) == 0
                             && EnGetProfileInt( "Cheat", "GrantResearch", 0 ) ) {
                            me->DebugDiscoverAllResearch( );
                            OutputDebugStringA( "[HRESEARCH] discovered ALL research (SP, [Cheat]GrantResearch=1)\n" );
                        } else {
                            OutputDebugStringA( "[HRESEARCH] skipped (needs _CHEAT build + [Cheat]GrantResearch=1 + SP)\n" );
                        }
                        return true;
                    }
#endif
#endif

                    UINT vk = SDLKeyToVK(sc);
                    if (!vk) return false;

                    // Simulate MFC accelerator commands that arrow keys
                    // and letter keys would normally trigger
                    switch (vk) {
                    case VK_LEFT:   pThis->CurLeft(); return true;
                    case VK_RIGHT:  pThis->CurRight(); return true;
                    case VK_UP:     pThis->CurUp(); return true;
                    case VK_DOWN:   pThis->CurDown(); return true;
                    case VK_ESCAPE: pThis->OnDeselect(); return true;
                    case VK_HOME:   pThis->CenterUnit(); return true;   // IDA_CENTER: center on selection, or rocket if none
                    case VK_DELETE: pThis->DestroyUnit(); return true;
                    case VK_INSERT: pThis->StopDestroyUnit(); return true;
                    case 'B':       pThis->BuildUnit(); return true;
                    case 'O':       pThis->OppoUnit(); return true;
                    case 'R':       pThis->RoadOrRoute(); return true;  // crane: build road; else: route
                    case 'U':       pThis->UnloadUnit(); return true;
                    case 'X':       pThis->RetreatUnit(); return true;
                    default:
                        pThis->OnKeyDown(vk, 1, 0);
                        return true;
                    }
                }
                case SDL_KEYUP: {
                    UINT vk = SDLKeyToVK(event.key.keysym.scancode);
                    if (vk) pThis->OnKeyUp(vk, 1, 0);
                    return vk != 0;
                }
                }
                return false;
            });

        // When panel is resized by user, update the game's rendering buffers
        m_aa.m_sdlPanel->SetResizeCallback(
            [pThis](int newW, int newH) {
                // Resize the CAnimAtr's DIB to match the new panel size
                pThis->m_aa.m_dibwnd.Size( MAKELPARAM(newW, newH) );
                pThis->m_cx = newW;
                pThis->m_cy = newH;
                pThis->m_bUpdateAll = TRUE;
                pThis->m_aa.Resized();

                // Reposition the static button bar at the bottom of the area
                if ( pThis->m_WndStatic.m_sdlPanel ) {
                    SDL2Panel* areaPanel = pThis->m_aa.m_sdlPanel;
                    int staticH = pThis->m_WndStatic.m_iYmin;
                    pThis->m_WndStatic.m_sdlPanel->SetPosition(
                        areaPanel->GetX(),
                        areaPanel->GetY() + newH - staticH );
                    pThis->m_WndStatic.m_sdlPanel->SetSize( newW, staticH );
                }

                // Update the selection buffer
                CDIB* pdib = pThis->m_aa.m_dibwnd.GetDIB();
                if ( pdib ) {
                    int iBytesPerPixel = pdib->GetBytesPerPixel();
                    delete[] pThis->m_pSelUnder;
                    pThis->m_pSelUnder = new BYTE[
                        pdib->GetWidth() * iBytesPerPixel * SEL_WIDTH * 2 +
                        pdib->GetHeight() * iBytesPerPixel * SEL_WIDTH * 2 +
                        iBytesPerPixel * SEL_WIDTH * 8];
                }
            });

        // Keep the hidden MFC stub window glued under the visible SDL panel.
        // Selection / build-placement / hover code reads the cursor through
        // ::GetCursorPos() + ScreenToClient() against THIS HWND, so if the
        // panel is dragged but the MFC window stays put, those reads desync.
        // Tracking the MFC window to the panel's on-screen content rect keeps
        // them aligned wherever the panel is moved (incl. other monitors).
        {
            HWND hMfc = m_hWnd;
            SDL2Panel* pPanel = m_aa.m_sdlPanel;
            pPanel->SetMoveCallback(
                [hMfc, pPanel](int x, int y, int w, int h) {
                    int sx, sy;
                    if ( pPanel->IsDetached() && pPanel->GetOwnWindow() ) {
                        // Own borderless OS window: content sits below our custom
                        // title bar. Derive content screen origin from the window.
                        int wx = 0, wy = 0;
                        SDL_GetWindowPosition( pPanel->GetOwnWindow(), &wx, &wy );
                        sx = wx;
                        sy = wy + pPanel->GetTitleBarHeight();
                    } else {
                        sx = x; sy = y;
                        if ( theApp.m_gameWindow && theApp.m_gameWindow->GetWindow() ) {
                            int wx = 0, wy = 0;
                            SDL_GetWindowPosition( theApp.m_gameWindow->GetWindow(), &wx, &wy );
                            sx += wx; sy += wy;
                        }
                    }
                    // The MFC window has a caption + frame (non-client area).
                    // Selection reads coords via ScreenToClient against its
                    // CLIENT rect, so align the CLIENT (not the window rect) to
                    // the panel's content origin — otherwise clicks are offset
                    // by the border/caption thickness (~8px x, ~31px y).
                    RECT wr, cr; POINT tl = { 0, 0 };
                    ::GetWindowRect( hMfc, &wr );
                    ::GetClientRect( hMfc, &cr );
                    ::ClientToScreen( hMfc, &tl );
                    int ncL = tl.x - wr.left;
                    int ncT = tl.y - wr.top;
                    int ncW = ( wr.right - wr.left ) - ( cr.right - cr.left );
                    int ncH = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );
                    ::SetWindowPos( hMfc, NULL, sx - ncL, sy - ncT, w + ncW, h + ncH,
                                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
                });
            // Sync the MFC window to the panel's initial position right away.
            pPanel->InvokeMoveCallback();
        }

        // SDL2-only renderer now: the MFC stub HWND has no visible role. Hide
        // it from the desktop so the compositor-managed SDL panel (with its
        // own green title bar) is the only visible window. WS_EX_TRANSPARENT
        // keeps any stray hit-testing click-through.
        ::SetWindowLong( m_hWnd, GWL_EXSTYLE,
            ::GetWindowLong( m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED | WS_EX_TRANSPARENT );
        ::SetLayeredWindowAttributes( m_hWnd, 0, 0, LWA_ALPHA );
        ::ShowWindow( m_hWnd, SW_HIDE );

        // Create the area button bar as a compositor panel that lives inside
        // the area map panel (blitted into m_aa.m_sdlPanel's surface). It's
        // hidden from the compositor's own render pass — the area panel's
        // Render handler does the blit.
        if ( m_WndStatic.m_hWnd ) {
            int staticH = m_WndStatic.m_iYmin;
            if (staticH < 20) staticH = 36;  // ensure minimum height

            int barX = m_aa.m_sdlPanel->GetX();
            int barY = m_aa.m_sdlPanel->GetY() + m_aa.m_sdlPanel->GetHeight() - staticH;
            int barW = m_aa.m_sdlPanel->GetWidth();
            int barZ = m_aa.m_sdlPanel->GetZOrder() + 1;

            m_WndStatic.m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(
                "area_bar", barX, barY, barW, staticH, barZ );
            m_WndStatic.m_sdlPanel->SetVisible(false);

            delete m_WndStatic.m_sdl2Bar;
            m_WndStatic.m_sdl2Bar = new SDL2AreaBar();
            m_WndStatic.m_sdl2Bar->Init( m_WndStatic.m_sdlPanel, this, m_WndStatic.m_hWnd );

            SDL2AreaBar* pBar = m_WndStatic.m_sdl2Bar;
            m_WndStatic.m_sdlPanel->SetEventCallback(
                [pBar](SDL_Event& event, int localX, int localY) -> bool {
                    return pBar->HandleEvent(event, localX, localY);
                });

            // The area window presents through its own GPU-terrain renderer, whose
            // overlay is built from the sprite layer only (not m_surface). Register
            // the bar panel so RenderDetached composites it as bottom chrome —
            // otherwise the bar disappears on the GPU path.
            m_aa.m_sdlPanel->SetBottomChromePanel( m_WndStatic.m_sdlPanel );
        }

        // Give the area map its own borderless OS window (purple chrome drawn
        // by SDL2Panel) so it can be dragged onto any monitor. The move-callback
        // keeps the hidden MFC window aligned for selection. The area button bar
        // is blitted into this panel's surface, so it travels with the window.
        m_aa.m_sdlPanel->Detach( theApp.m_gameWindow.get() );

        // The close [X] hides the window; the Map icon in the status bar
        // (CWndBar::GotoArea) brings it back.
        m_aa.m_sdlPanel->SetClosable(true);
    }

    // Re-run SetButtonState now that m_sdlPanel is set,
    // so ShowButton keeps buttons visible (just disabled) for SDL rendering
    SetButtonState();

    m_bUpdateAll = TRUE;

    if ( bPlaceIt ) // only done in loading? or maybe bringing it back up after closing it?
    {
#ifdef LOGGINGON
        OutputDebugStringA( "bPlaceIt!");
#endif
        bPlaceIt = TRUE;
        // For a detached SDL window the SDL panel owns the on-screen geometry
        // and the hidden MFC window is kept aligned to it by the move-callback.
        // Restoring the saved WINDOWPLACEMENT onto the MFC window would resize
        // its client out from under the SDL content and scale-desync selection,
        // so skip it in that case (the SDL window was already clamped to screen).
        if ( !m_aa.m_sdlPanel || !m_aa.m_sdlPanel->IsDetached( ) )
            SetWindowPlacement( &( theGame.m_wpArea ) );
        Center( theGame.m_hexAreaCenter );

        // bring the world window back too
        if ( theApp.m_wndWorld.m_hWnd == NULL )
        {
            theApp.m_wndWorld.Create( );
            theApp.m_wndWorld.NewAreaMap( this );
        }
    }
    else
    {
        theGame.m_wpArea.length = sizeof( WINDOWPLACEMENT );
        GetWindowPlacement( &( theGame.m_wpArea ) );
    }

    return ( 0 );
}

void CWndArea::OnSize( UINT nType, int cx, int cy )
{

    CWndAnim::OnSize( nType, cx, cy );

    // move/re-size the status window
    m_WndStatic.SetWindowPos( NULL, 0, cy - m_WndStatic.m_iYmin, cx, m_WndStatic.m_iYmin,
                              SWP_NOACTIVATE | SWP_NOZORDER );

    CRect rect;
    GetClientRect( &rect );
    m_cx = rect.Width( );
    m_cy = rect.Height( );

    // When an SDL panel backs this window it owns the *visible* size. The hidden
    // MFC client can transiently differ (restored WINDOWPLACEMENT, non-client
    // tracking), and the render DIB + hit-testing (WindowToHex uses m_cx/m_cy)
    // must match exactly what's on screen — otherwise clicks scale-diverge from
    // the cursor toward the bottom. Use the panel's content size as the truth.
    if ( m_aa.m_sdlPanel )
    {
        m_cx = m_aa.m_sdlPanel->GetWidth( );
        m_cy = m_aa.m_sdlPanel->GetHeight( );
    }

    // create the bitmap for the new size
    m_aa.m_dibwnd.Size( MAKELPARAM( m_cx, m_cy ) );

    // Update SDL2 panel size/position to match — but only if the panel
    // isn't user-resizable (when it is, the panel owns its position/size).
    if ( m_aa.m_sdlPanel && !m_aa.m_sdlPanel->IsResizable() )
    {
        CRect screenRect;
        GetWindowRect( &screenRect );
        m_aa.m_sdlPanel->SetRect( screenRect.left, screenRect.top, m_cx, m_cy );
    }

    // cursor under buffer
    CDIB* pdib           = m_aa.m_dibwnd.GetDIB( );
    int   iBytesPerPixel = pdib->GetBytesPerPixel( );
    delete[] m_pSelUnder;
    m_pSelUnder = new BYTE[pdib->GetWidth( ) * iBytesPerPixel * SEL_WIDTH * 2 +
                           pdib->GetHeight( ) * iBytesPerPixel * SEL_WIDTH * 2 + iBytesPerPixel * SEL_WIDTH * 8];

    // re-center it
    m_aa.Resized( );

    // GGTESTING	m_aa.SetCenter( m_aa.GetCenter() );

    theGame.m_wpArea.length = sizeof( WINDOWPLACEMENT );
    GetWindowPlacement( &( theGame.m_wpArea ) );

    if ( m_bScrollBars )
    {
        if ( m_scrollbarH.m_hWnd )
            m_scrollbarH.MoveWindow( 0, rect.Height( ), rect.Width( ), GetSystemMetrics( SM_CYHSCROLL ) );

        if ( m_scrollbarV.m_hWnd )
            m_scrollbarV.MoveWindow( rect.Width( ), 0, GetSystemMetrics( SM_CXVSCROLL ), rect.Height( ) );
    }

    // redraw the map
    // GGTESTING InvalidateWindow ();
}

void CWndArea::OnMove( int x, int y )
{
    CWndAnim::OnMove( x, y );

    // Don't sync MFC → panel when panel is movable (user controls position)
    // Only sync when panel isn't user-managed (e.g. during initial placement)
    if ( m_aa.m_sdlPanel && !m_aa.m_sdlPanel->IsMovable() )
    {
        CRect screenRect;
        GetWindowRect( &screenRect );
        m_aa.m_sdlPanel->SetPosition( screenRect.left, screenRect.top );
    }
}

void CWndArea::OnGetMinMaxInfo( MINMAXINFO FAR* lpMMI )
{

    // we limit how small it can be
    if ( lpMMI->ptMinTrackSize.x < m_iXmin )
        lpMMI->ptMinTrackSize.x = m_iXmin;
    if ( lpMMI->ptMinTrackSize.y < m_iYmin )
        lpMMI->ptMinTrackSize.y = m_iYmin;

    if ( theApp.m_wndBar.IsCreated() )
    {
        CRect rect;
        theApp.m_wndBar.GetWindowRect( &rect );
        lpMMI->ptMaxTrackSize.y = __min( lpMMI->ptMaxTrackSize.y, rect.top );
        lpMMI->ptMaxSize.y      = __min( lpMMI->ptMaxSize.y, rect.top );
    }

    CWndAnim::OnGetMinMaxInfo( lpMMI );
}

void CWndArea::OnSysCommand( UINT nID, LPARAM lParam )
{

    // for minimize hide it - only if last
    if ( theAreaList.GetCount( ) <= 1 )
        if ( ( nID == SC_MINIMIZE ) || ( nID == SC_CLOSE ) )
        {
            ShowWindow( SW_HIDE );
            return;
        }

    CWndAnim::OnSysCommand( nID, lParam );
}

void CWndArea::OnDestroy( )
{

    // if we're the status line kill it
    if ( theApp.m_wndBar.GetStatusLineData( 1 ) != NULL )
    {
        CPoint pt;
        ::GetCursorPos( &pt );
        theApp.m_wndBar.ClearStatusFunc( 1 );
    }

    // give the world map a new area map
    CWndArea* pNewArea = theAreaList.GetTop( );
    if ( pNewArea == this )
    {
        POSITION pos;
        for ( pos = theAreaList.GetHeadPosition( ); pos != NULL; )
        {
            pNewArea = theAreaList.GetNext( pos );
            ASSERT_STRICT_VALID( pWndArea );
            if ( pNewArea != this )
                break;
        }
    }
    if ( pNewArea == this )
    {
        ASSERT( FALSE );
        pNewArea = NULL;
    }
    theApp.m_wndWorld.NewAreaMap( pNewArea );

    // free everything up
    ReleaseMouse( );
    ::ClipCursor( NULL );

    // save position
    theGame.m_hexAreaCenter = m_aa.GetCenter( );

    m_aa.m_dibwnd.Exit( );

    CWndAnim::OnDestroy( );

    if ( pNewArea == NULL )
    {
        uShowStat.Close( );
        tShowStat.Close( );
    }
}

void CWndArea::ZoomIn( )
{

    ASSERT_STRICT_VALID( this );

    if ( m_aa.m_iZoom > theApp.GetZoomData( )->GetFirstZoom( ) )
    {
        m_aa.Zoom( CAnimAtr::ZOOM_IN );

        ASSERT_STRICT_VALID( &theMap );
        
        InvalidateWindow ();
        InvalidateSound( );
    }

    CheckZoomBtns( );
}

void CWndArea::ZoomOut( )
{

    ASSERT_STRICT_VALID( this );

    if ( m_aa.m_iZoom < NUM_ZOOM_LEVELS - 1 )
    {
        m_aa.Zoom( CAnimAtr::ZOOM_OUT );

        ASSERT_STRICT_VALID( &theMap );
        
        InvalidateWindow ();
        InvalidateSound( );
    }

    CheckZoomBtns( );
}

void CWndArea::CheckZoomBtns( )
{

    if ( m_aa.m_iZoom <= theApp.GetZoomData( )->GetFirstZoom( ) )
        EnableButton( IDC_AREA_ZOOM_IN, FALSE );
    else
        EnableButton( IDC_AREA_ZOOM_IN, TRUE );
    if ( m_aa.m_iZoom >= NUM_ZOOM_LEVELS - 1 )
        EnableButton( IDC_AREA_ZOOM_OUT, FALSE );
    else
        EnableButton( IDC_AREA_ZOOM_OUT, TRUE );
}

void CWndArea::TurnClock( )
{
    ASSERT_STRICT_VALID( this );

    m_aa.Turn( CAnimAtr::TURN_CLOCKWISE );

    ASSERT_STRICT_VALID( &theMap );
    theApp.m_wndWorld.NewDir( );
    InvalidateWindow ();
    InvalidateSound( );
}

void CWndArea::TurnCounter( )
{
    ASSERT_STRICT_VALID( this );

    m_aa.Turn( CAnimAtr::TURN_COUNTERCLOCKWISE );

    ASSERT_STRICT_VALID( &theMap );
    theApp.m_wndWorld.NewDir( );
    InvalidateWindow ();
    InvalidateSound( );
}

void CWndArea::RotateBuildDir( int iStep )
{
    // Only meaningful while planning a building/rocket placement.
    if ( ( m_iMode != build_ready ) && ( m_iMode != rocket_ready ) )
        return;

    m_iBuildDir = ( m_iBuildDir + iStep ) & 0x03;

    // Refresh the placement preview at the current cursor (mirrors the Ctrl+RMB
    // path in OnRButtonDown, which has the click point in hand).
    SetMouseState( );
    CPoint pt;
    ::GetCursorPos( &pt );
    ScreenToClient( &pt );
    OnMouseMove( 0, pt );
}

void CWndArea::ResClicked( )
{
    static int aiRes[] = { -1, -1, 3, 3, -1, -1, 2, -1, 0, 1 };

    m_bShowRes = !m_bShowRes;

    // Resource-view toggle swaps hex sprites to/from the minerals overlay directly
    // (m_psprite, bypassing SetVisibleType). A raw ++g_enTerrainEditGen does NOT
    // invalidate the per-hex GPU tile memo, so the rebuild re-used the cached
    // pre-overlay tiles and nothing appeared. Record each mineral hex via
    // g_enEditHex (below) so its tile is re-baked from the new sprite.
    extern void g_enEditHex( int x, int y );

    BOOL bCopper = theGame.GetMe( )->CanCopper( );

    // walk through each mineral setting the CHex
    POSITION pos = theMinerals.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dw;
        CMinerals* pMn;
        theMinerals.GetNextAssoc( pos, dw, pMn );

        CHexCoord _hex( dw >> 16, dw & 0xFFFF );
        CHex*     pHex = theMap._GetHex( _hex );
        ASSERT_VALID( pHex );

        _hex.SetInvalidated( );

        // give it the resource terrain tile
        if ( bCopper || ( pMn->GetType( ) != CMaterialTypes::copper ) )
        {
            if ( m_bShowRes )
                pHex->m_psprite = theTerrain.GetSprite( CHex::resources, aiRes[pMn->GetType( )] );
            else
            {
                switch ( pHex->GetVisibleType( ) )
                {
                case CHex::road:
                    pHex->ChangeToRoad( _hex );
                    break;
                case CHex::city: {
                    int iIndex;
                    if ( pHex->GetUnits( ) & CHex::bldg )
                        iIndex = CITY_BUILD_OFF + RandNum( CITY_BUILD_NUM - 1 );
                    else
                        iIndex = CITY_DESTROYED_OFF + RandNum( CITY_DESTROYED_NUM - 1 );
                    pHex->m_psprite = theTerrain.GetSprite( CHex::city, iIndex );
                    break;
                }
                default:
                    pHex->m_psprite = theTerrain.GetSprite(
                        pHex->GetVisibleType( ), RandNum( theTerrain.GetCount( pHex->GetVisibleType( ) ) - 1 ) );
                    break;
                }
            }

            // re-bake this hex's GPU tile from the just-changed sprite (records the
            // hex so the per-hex tile memo is invalidated, not just the gen)
            g_enEditHex( _hex.X( ), _hex.Y( ) );
        }
    }

    InvalidateWindow( );
}

void CWndArea::OnLButtonDown( UINT nFlags, CPoint point )
{

    ASSERT_STRICT_VALID( this );

    // if dir != 0 -> switch to proper x, y
    CHexCoord hexcoord = m_aa.WindowToHex( point );

    switch ( m_iMode )
    {
    case normal:
        // pressing LMB during an RMB drag cancels the pending command/line-move
        if ( m_bRmbCmdDown )
        {
            m_bRmbCmdDown = FALSE;
            m_bLineMove   = FALSE;
            s_linePath.clear( );
        }

        m_ptLMB = point;
        m_iMode = normal_select;

        CaptureMouse( );
        m_selOrig = point;
        m_selRect.SetRectEmpty( );
        break;

    // set these up so we know the down came from here
    case build_ready:
        m_iMode = build_loc;
        break;
    case rocket_ready:
        m_iMode = rocket_pos;
        break;

    // set start of a road
    case road_begin:
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        CaptureMouse( );
        m_hexRoadStart = hexcoord;
        m_iMode        = road_set;
        ClrRoadIcons( );
        AreaApplyCursor( m_hCurRoadSet[m_aa.m_iZoom] );
        return;
    }

    // nothing to do
    CWndAnim::OnLButtonDown( nFlags, point );
}

// FIXIT: convert

void CWndArea::GetPanAndVol( CUnit const* pUnit, int& iPan, int& iVol )
{

    if ( ( m_cx <= 0 ) || ( m_cy <= 0 ) )
    {
        TRAP( );
        iPan = iVol = 0;
        return;
    }

    CPoint ptUnit = m_aa.WrapWorldToWindow( CMapLoc3D( pUnit->GetWorldPixels( ) ) );

    iVol = 100;

    // off the top?
    if ( ptUnit.y < 0 )
        // note - this is a subtraction because ptUnit.y < 0
        iVol = 100 + ( ptUnit.y * 100 ) / m_cy;

    // off the bottom
    if ( ptUnit.y >= m_cy )
        iVol = 100 - ( ( ptUnit.y - m_cy ) * 100 ) / m_cy;

    // off to the left?
    if ( ptUnit.x < 0 )
    {
        iPan = 0;
        // note - this is a subtraction because ptUnit.x < 0
        iVol = __min( iVol, 100 + ( ptUnit.x * 100 ) / m_cx );
        return;
    }

    // off to the right?
    if ( ptUnit.x >= m_cx )
    {
        iPan = 127;
        iVol = __min( iVol, 100 - ( ( ptUnit.x - m_cx ) * 100 ) / m_cx );
        return;
    }

    iPan = 128 - ( ( m_cx - ptUnit.x ) * 128 ) / m_cx;
}

void CWndArea::SetDestAndSfx( CVehicle* pVeh, CHexCoord const& hex )
{

    if ( ( theMusicPlayer.SoundsPlaying( ) ) && ( pVeh->GetRouteMode( ) == CVehicle::stop ) )
    {
        int iPan, iVol;
        GetPanAndVol( pVeh, iPan, iVol );
        theMusicPlayer.PlayForegroundSound( pVeh->GetData( )->GetSoundGo( ), iPan, iVol );
    }

    pVeh->SetDest( hex );
}

void CWndArea::SetDestAndSfx( CVehicle* pVeh, CSubHex const& sub )
{

    if ( ( theMusicPlayer.SoundsPlaying( ) ) && ( pVeh->GetRouteMode( ) == CVehicle::stop ) )
    {
        int iPan, iVol;
        GetPanAndVol( pVeh, iPan, iVol );
        theMusicPlayer.PlayForegroundSound( pVeh->GetData( )->GetSoundGo( ), iPan, iVol );
    }

    pVeh->SetDest( sub );
}

// F2: append a movement waypoint to a vehicle's route instead of replacing its order.
// The first Shift-click on an unrouted vehicle starts a fresh ONE-SHOT (non-looping) queue;
// subsequent Shift-clicks append to the tail. Mirrors SDL2RouteWindow's Start sequence so
// the queue actually runs (route event + HP control + kick the first leg).
void CWndArea::ShiftQueueMove( CVehicle* pVeh, CSubHex const& sub )
{
    ASSERT_STRICT_VALID( pVeh );

    pVeh->ResumeUnit( );
    pVeh->TempTargetOff( );
    pVeh->_SetTarget( NULL );

    BOOL bFresh = ( pVeh->GetEvent( ) != CVehicle::route );

    // Shift-click on an EXISTING queued waypoint DELETES it (toggle-off) instead of
    // appending a duplicate. (operator feature.) Only when already routing; match by hex.
    if ( !bFresh )
    {
        auto&    rl = pVeh->GetRouteList( );
        POSITION p  = rl.GetHeadPosition( );
        while ( p != NULL )
        {
            POSITION cur = p;
            CRoute*  pR  = rl.GetNext( p );   // advances p past cur to the next node
            if ( pR != NULL && pR->GetCoord( ) == CHexCoord( sub ) )
            {
                bool     bWasCurrent = ( cur == pVeh->GetRoutePos( ) );
                POSITION pNext       = p;     // node AFTER the deleted one (NULL if tail)
                delete pR;
                rl.RemoveAt( cur );
                if ( rl.GetCount( ) == 0 )
                {
                    StopRoute( pVeh );        // removed the last waypoint -> stop here
                }
                else if ( bWasCurrent )
                {
                    // deleted the leg we were driving -> retarget the next remaining waypoint
                    POSITION np = ( pNext != NULL ) ? pNext : rl.GetHeadPosition( );
                    pVeh->SetRoutePos( np );
                    CRoute* pN = rl.GetAt( np );
                    if ( pN != NULL ) pVeh->SetDest( pN->GetCoord( ) );
                }
                // (deleted a NON-current waypoint -> route pos unchanged, keep driving)
                if ( pVeh->m_pSdlRoute != NULL )
                    pVeh->m_pSdlRoute->RefreshRoute( );
                return;
            }
        }
    }

    if ( bFresh )
    {
        // start a fresh one-shot queue — clear any stale route first
        POSITION pos = pVeh->GetRouteList( ).GetHeadPosition( );
        while ( pos != NULL )
            delete pVeh->GetRouteList( ).GetNext( pos );
        pVeh->GetRouteList( ).RemoveAll( );
        pVeh->SetRoutePos( NULL );
        pVeh->SetRouteLoop( FALSE );   // Shift-queued routes are one-shot

        // #42: if the vehicle is already moving to a DIRECT destination (a prior non-shift
        // right-click move), SEED the new route with that destination as waypoint #1 — so the
        // first Shift-click APPENDS (forming a 2-stop route) instead of OVERWRITING the
        // existing move. (Was: the fresh-queue clear dropped the in-flight dest entirely.)
        if ( pVeh->GetRouteMode( ) == CVehicle::moving )
        {
            CHexCoord hexCur = pVeh->GetHexDest( );
            pVeh->SetLocation( hexCur, pVeh->GetRouteList( ).GetTailPosition( ), CRoute::waypoint );
        }
    }

    // append the waypoint at the tail (SetLocation resolves building entrances + wrap)
    CHexCoord hexWp( sub );
    POSITION posTail = pVeh->GetRouteList( ).GetTailPosition( );
    pVeh->SetLocation( hexWp, posTail, CRoute::waypoint );

    if ( bFresh )
    {
        pVeh->SetEvent( CVehicle::route );
        theGame.m_pHpRtr->MsgTakeVeh( pVeh );
        pVeh->HpControlOn( );
        POSITION rp = pVeh->GetRoutePos( );
        if ( rp != NULL )
        {
            CRoute* pR = pVeh->GetRouteList( ).GetAt( rp );
            if ( pR != NULL )
                pVeh->SetDest( pR->GetCoord( ) );
        }
    }

    if ( pVeh->m_pSdlRoute != NULL )
        pVeh->m_pSdlRoute->RefreshRoute( );
}

// A manual move command (normal click OR line-move) STOPS / overrides ANY active route the
// vehicle is on — including LOOP/haul routes (operator 2026-06-27, BUGS.md #6: "normal and
// line should stop/override routing"). Previously loop routes were deliberately preserved;
// the operator wants a move to interrupt them. If the vehicle is auto-router-controlled (a
// loop/haul route) release it from the router first, then clear the route list and reset the
// loop flag to the default.
void CWndArea::StopRoute( CVehicle* pVeh )
{
    if ( pVeh->IsHpControl( ) )          // on an auto-router (loop/haul) route — release it
    {
        pVeh->HpControlOff( );
        theGame.m_pHpRtr->MsgGiveVeh( pVeh );
    }
    POSITION p = pVeh->GetRouteList( ).GetHeadPosition( );
    while ( p != NULL )
        delete pVeh->GetRouteList( ).GetNext( p );
    pVeh->GetRouteList( ).RemoveAll( );
    pVeh->SetRoutePos( NULL );
    pVeh->SetRouteLoop( TRUE );
    if ( pVeh->m_pSdlRoute != NULL )
        pVeh->m_pSdlRoute->RefreshRoute( );
}

void CWndArea::OnLButtonUp( UINT nFlags, CPoint point )
{

    ASSERT_STRICT_VALID( this );

    ReleaseMouse( );

    // in case from a road
    ClrRoadIcons( );

    CSubHex _sub = m_aa.WindowToSubHex( point );
    _sub.Wrap( );
    CHexCoord hex( _sub );

    switch ( m_iMode )
    {
    // route goto?
    case veh_route: {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        ASSERT_STRICT_VALID( m_pUnit );
        ASSERT_STRICT( m_pUnit->GetUnitType( ) == CUnit::vehicle );
        ( (CVehicle*)m_pUnit )->SetLocation( hex, m_posRoute, m_iRouteType );
        if ( ( (CVehicle*)m_pUnit )->m_pWndRoute != NULL )
            ( ( (CVehicle*)m_pUnit )->m_pWndRoute )->NewRoute( (CVehicle*)m_pUnit );
        // Refresh SDL2 route window if open
        if ( ( (CVehicle*)m_pUnit )->m_pSdlRoute != NULL )
            ( (CVehicle*)m_pUnit )->m_pSdlRoute->RefreshRoute();

        // may now allow resume
        SetButtonState( );
        return;
    }

    // build
    case build_loc:
    case rocket_pos: {
        ASSERT_STRICT( ( 0 < m_iBuild ) && ( m_iBuild <= theStructures.GetNumBuildings( ) ) );
        if ( ( nFlags & MK_SHIFT ) && ( m_pUnit != NULL ) && ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) )
        {
            TRAP( );  // BUGBUG - check what this does
            ( (CVehicle*)m_pUnit )->SetEvent( CVehicle::none );
            m_iMode = build_ready;
            return;
        }

        hex = ToBuildUL( hex );

        // make sure not on water or another city
        if ( m_iFound < 0 )
        {
        bad_loc:
            theGame.Event( m_iMode == rocket_pos ? EVENT_ROCKET_CANT : EVENT_CONST_CANT, EVENT_BAD, m_iBuild );
            m_iMode = ( m_iMode == rocket_pos ) ? rocket_ready : build_ready;
            return;
        }

        if ( m_iMode == rocket_pos )
        {
            // we test to make sure that all vehicles can get out of the rocket
            if ( !CStructureData::CanBuild( hex, GetBuildDir( ), CStructureData::rocket, FALSE, TRUE ) )
                goto bad_loc;

            // it must all be visible and no vehicle under
            BOOL                  bOk   = TRUE;
            CStructureData const* pData = theStructures.GetData( m_iBuild );
            theMap.EnumHexes( hex, GetBuildDir( ) & 1 ? pData->GetCY( ) : pData->GetCX( ),
                              GetBuildDir( ) & 1 ? pData->GetCX( ) : pData->GetCY( ), fnEnumIsVisNoVeh, &bOk );
            if ( !bOk )
                goto bad_loc;

            theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::rocket_landing ), SFXPRIORITY::selected_pri );

            theGame.Event( EVENT_ROCKET_CANT, EVENT_OFF );
            std::string sMsg = EnLoadStdString( IDS_MSG_ROCKET_WAIT );
            SetStatusText( sMsg.c_str( ) );
            CMsgPlaceBldg msg( hex, GetBuildDir( ), CStructureData::rocket );
            msg.m_iPlyrNum = theGame.GetMe( )->GetPlyrNum( );
            msg.m_bShow    = theApp.IsShareware( );
            theGame.PostToServer( &msg, sizeof( msg ) );
            BldgCurOff( );
            m_iMode = rocket_wait;
            AreaApplyCursor( m_hCurStart );
            return;
        }

        // had GPF of no crane
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) ||
             ( ( (CVehicle*)m_pUnit )->GetData( )->GetType( ) != CTransportData::construction ) )
        {
            TRAP( );
            SetButtonState( );
            BldgCurOff( );
            AreaApplyCursor( m_hCurStart );
            SelectOff( );
            return;
        }

        // send the vehicle there
        // find the closest surrounding hex
        theGame.Event( EVENT_CONST_CANT, EVENT_OFF );

        CHexCoord hexDest( hex );
        BuildBldgDest( (CVehicle*)m_pUnit, m_iBuild, GetBuildDir( ), hexDest );

        // lets build here
        m_pUnit->ResumeUnit( );
        ( (CVehicle*)m_pUnit )->SetBuilding( hex, m_iBuild, GetBuildDir( ) );
        ( (CVehicle*)m_pUnit )->SetEvent( CVehicle::build );
        ( (CVehicle*)m_pUnit )->SetDestAndMode( hexDest, CVehicle::full );

        // we loose selection of the crane so we don't change the orders
        m_lstUnits.RemoveAllUnits( TRUE );
        m_pUnit = NULL;
        SetButtonState( );
        BldgCurOff( );
        AreaApplyCursor( m_hCurStart );
        SelectOff( );
        return;
    }

    // selecting unit(s) — the LMB is selection ONLY now: the command half
    // (move/attack/load/unload/repair) lives on the right button (DoCommandAt).
    case normal_select: {
        m_iMode = normal;

        // The box-select marquee (GPU/split path) is drawn into m_dibSprite while
        // m_iMode==normal_select. Now that the drag has ended it won't be redrawn,
        // so force a full sprite-overlay wipe next frame — otherwise, if no pan/zoom
        // follows, the marquee bands linger as a stale striped diamond (the overlay
        // is only fully cleared on a detected view change). Inert on the Windows path
        // (m_bOverlayDirty is honored only under IsGpuFull()).
        m_aa.m_bOverlayDirty = TRUE;

        // Crane over a damaged / under-construction building (or unfinished bridge):
        // a LEFT click should COMMAND the repair/build, same as the right button —
        // NOT deselect the crane. SetMouseState keeps m_uMouseMode == lmb_repair_bldg
        // current from the hover (it's also what drives the repair cursor preview),
        // and that mode is only set for crane + repairable/constructing target
        // (see SetMouseState steps -2/-1). Gate on a click, not a drag-select.
        {
            BOOL bDragSel = ( abs( point.x - m_ptLMB.x ) >= theMap.HexWid( m_aa.m_iZoom ) / 2 ) ||
                            ( abs( point.y - m_ptLMB.y ) >= theMap.HexHt( m_aa.m_iZoom ) / 2 );
            if ( ( m_uMouseMode == lmb_repair_bldg ) && !bDragSel )
            {
                DoCommandAt( nFlags, point );   // dispatches the crane repair/build
                SetButtonState( );
                InvalidateStatus( );
                InvalidateSound( );
                return;
            }
        }

        CHitInfo hitinfo = m_aa.GetHit( point );
        CUnit*   pUnitOn = hitinfo.GetUnit( );
        // if not visible then it's not there
        if ( pUnitOn != NULL )
            if ( !pUnitOn->IsVisible( ) )
                pUnitOn = NULL;

        ASSERT_STRICT_VALID_OR_NULL( pUnitOn );
        BOOL bSelected = FALSE;

        // step 1 - no ctrl/shift -> the click starts a fresh selection
        if ( ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) == 0 )
        {
            m_lstUnits.RemoveAllUnits( TRUE );
            m_pUnit = NULL;
        }

        // step 2 - if we dragged then we are selecting
        if ( ( abs( point.x - m_ptLMB.x ) >= theMap.HexWid( m_aa.m_iZoom ) / 2 ) ||
             ( abs( point.y - m_ptLMB.y ) >= theMap.HexHt( m_aa.m_iZoom ) / 2 ) )
        {
            if ( ( nFlags & MK_SHIFT ) == 0 )
            {
                m_lstUnits.RemoveAllUnits( TRUE );
                m_pUnit = NULL;
            }
            bSelected = TRUE;

            // get all units in the box
            POSITION pos;
            pos                 = theVehicleMap.GetStartPosition( );
            int       iNumInBox = 0;
            CVehicle* pVehLastInBox;
            while ( pos != NULL )
            {
                DWORD     dwID;
                CVehicle* pVeh;
                theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                ASSERT_STRICT_VALID( pVeh );
                if ( pVeh->GetOwner( )->IsMe( ) )
                {
                    CPoint pt = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( pVeh->GetWorldPixels( ) ) );
                    if ( ( m_selRect.PtInRect( pt ) ) && ( pVeh->GetTransport( ) == NULL ) )
                    {
                        iNumInBox++;
                        pVehLastInBox = pVeh;
                        // if not ctrl then only combat vehicles
                        if ( ( ( nFlags & MK_CONTROL ) != 0 ) ||
                             ( !( pVeh->GetData( )->GetVehFlags( ) & CTransportData::FLcivilian ) ) )
                            m_lstUnits.AddUnit( pVeh, TRUE );
                    }
                }
#ifdef _CHEAT
                else if ( _bClickAny )
                {
                    CPoint pt = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( pVeh->GetWorldPixels( ) ) );
                    if ( m_selRect.PtInRect( pt ) )
                        m_lstUnits.AddUnit( pVeh, TRUE );
                }
#endif
            }

            // if we got none and there was only 1 - then we select it (drag to get a lone crane)
            if ( ( iNumInBox == 1 ) && ( m_lstUnits.GetCount( ) == 0 ) )
                m_lstUnits.AddUnit( pVehLastInBox, TRUE );

            // if we got any then ignore what we are over
            if ( m_lstUnits.GetCount( ) >= 0 )
                pUnitOn = NULL;
        }

        // step 3 - clicked one of ours -> toggle its selection (shift/ctrl extend;
        // a plain click already started fresh in step 1). The carrier/repair
        // special actions that used to live here are RMB commands now.
        if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->IsMe( ) ) )
        {
            bSelected = TRUE;
            if ( pUnitOn->GetFlags( ) & CUnit::selected )
                m_lstUnits.RemoveUnit( pUnitOn );
            else
                m_lstUnits.AddUnit( pUnitOn, TRUE );
        }

        // if just one select it
        // set up the status bar
        if ( m_lstUnits.GetCount( ) == 1 )
            m_pUnit = m_lstUnits.GetHead( );
        else
            m_pUnit = NULL;

        // set button states
        SetButtonState( );

        // repaint it
        InvalidateStatus( );
        InvalidateSound( );

        // voices? say ok
        if ( ( bSelected ) && ( m_lstUnits.GetCount( ) > 0 ) )
        {
            if ( m_uFlags & crane )
                theGame.MulEvent( MEVENT_SELECT_CRANE, m_pUnit );
            else if ( m_uFlags & veh )
                theGame.MulEvent( MEVENT_SELECT_COMBAT, m_pUnit );
            if ( m_uFlags & fac )
                theGame.MulEvent( MEVENT_SELECT_FACTORY, m_pUnit );

            // building trigger
            if ( ( theMusicPlayer.SoundsPlaying( ) ) && ( m_pUnit != NULL ) &&
                 ( m_pUnit->GetUnitType( ) == CUnit::building ) && ( !( (CBuilding*)m_pUnit )->IsConstructing( ) ) )
            {
                int iSound = m_pUnit->IsFlag( ( CUnit::UNIT_FLAGS )( CUnit::stopped | CUnit::event |
                                                                     CUnit::repair_stop | CUnit::abandoned ) )
                                 ? m_pUnit->GetData( )->GetSoundIdle( )
                                 : m_pUnit->GetData( )->GetSoundRun( );
                int iPan, iVol;
                GetPanAndVol( m_pUnit, iPan, iVol );
                theMusicPlayer.PlayForegroundSound( iSound, iPan, iVol );
            }
        }

        break;
    }

    case road_set: {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        SelectOff( );
        if ( nFlags & MK_SHIFT )
        {
            SetButtonState( );
            return;
        }

        CHexCoord hex = m_aa.WindowToHex( point );

        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            SetDestAndSfx( (CVehicle*)pUnit, m_hexRoadStart );
            ( (CVehicle*)pUnit )->SetRoad( m_hexRoadStart, hex );
        }

        // deselect all
        m_lstUnits.RemoveAllUnits( TRUE );
        m_pUnit = NULL;
        SetButtonState( );
        InvalidateSound( );
        break;
    }

    case repair_bldg: {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        SelectOff( );

        CHexCoord hex = m_aa.WindowToHex( point );
        hex.Wrap( );
        CBuilding* pBldg = theBuildingHex._GetBuilding( hex );

        if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) )
        {
            POSITION pos;
            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                if ( ( (CVehicle*)pUnit )->GetData( )->GetType( ) == CTransportData::construction )
                {
                    pUnit->ResumeUnit( );
                    ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_bldg );
                    SetDestAndSfx( (CVehicle*)pUnit, hex );
                }
            }
        }

        // deselect all
        m_lstUnits.RemoveAllUnits( TRUE );
        m_pUnit = NULL;
        SetButtonState( );
        InvalidateSound( );
        break;
    }

    default:
        //			CWndAnim::OnLButtonUp(nFlags, point);
        break;
    }
}

typedef struct tagCLOSEST_HEX
{
    CTransportData const* pTd;
    int                   iTime;
    int                   iWheelType;
    int                   eX;
    int                   eY;
    CHexCoord             hexAt;
    CHexCoord             hexBldg;
    CHexCoord             hexClosest;
} CLOSEST_HEX;

static int fnEnumClosestHex( CHex* pHex, CHexCoord hex, void* pData )
{

    CLOSEST_HEX* pCh = (CLOSEST_HEX*)pData;
    if ( !pCh->pTd->CanTravelHex( pHex ) )
        return ( FALSE );

    int iTime = theMap.GetRangeDistance( pCh->hexAt, hex ) + theMap.GetTerrainCost( hex, hex, 0, pCh->iWheelType );

    if ( iTime < pCh->iTime )
    {
        // we only allow hexes on the edge
        if ( ( hex.X( ) == pCh->hexBldg.X( ) ) || ( hex.Y( ) == pCh->hexBldg.Y( ) ) ||
             ( hex.X( ) == pCh->hexBldg.X( ) + pCh->eX ) || ( hex.Y( ) == pCh->hexBldg.Y( ) + pCh->eY ) )
        {
            pCh->hexClosest = hex;
            pCh->iTime      = iTime;
            if ( pCh->hexAt == hex )
                return ( TRUE );
        }
    }

    return ( FALSE );
}

void BuildBldgDest( CVehicle* pVeh, int iBldg, int iDir, CHexCoord& hex )
{

    ASSERT_STRICT_VALID( pVeh );
    CLOSEST_HEX ch;
    ch.iTime      = INT_MAX;
    ch.iWheelType = pVeh->GetData( )->GetWheelType( );
    ch.hexAt      = pVeh->GetHexHead( );
    ch.eX         = iDir & 1 ? theStructures.GetData( iBldg )->GetCY( ) : theStructures.GetData( iBldg )->GetCX( );
    ch.eY         = iDir & 1 ? theStructures.GetData( iBldg )->GetCX( ) : theStructures.GetData( iBldg )->GetCY( );
    ch.hexBldg = ch.hexClosest = CHexCoord( hex.X( ), hex.Y( ) );
    ch.hexBldg.Wrap( );
    ch.pTd = pVeh->GetData( );

    theMap.EnumHexes( CHexCoord( hex.X( ), hex.Y( ) ), ch.eX, ch.eY, fnEnumClosestHex, &ch );
    hex = ch.hexClosest;
}

void CWndArea::SetupStart( )
{

    ASSERT_STRICT_VALID( this );

    // [rocket] diag: does the CLIENT arm manual rocket placement? (operator: MP
    // clients get an AUTO-placed rocket + no deploy; host/SP place manually OK).
    { static int on=-1; if(on<0) on=getenv("EN_ROCKET_LOG")?1:0;
      if(on) fprintf(stderr,"[rocket] SetupStart -> rocket_ready ARMED (AmServer=%d IsNetGame=%d myNet=%d)\n",
                     theGame.AmServer()?1:0, theGame.IsNetGame()?1:0, (int)theGame.GetMyNetNum()); }

    m_iMode     = rocket_ready;
    m_iBuild    = CStructureData::rocket;
    m_iBuildDir = ( theStructures.GetData( CStructureData::rocket )->GetExitDir( ) - 2 ) & 0x03;

    AreaApplyCursor( NULL );
    std::string sMsg = EnLoadStdString( IDS_MSG_ROCKET_START );
    SetStatusText( sMsg.c_str( ) );

    // start with resources showing
    ResClicked( );

    // CDlgFile removed (Phase 2d) — SDL2FileDialog rebuilds state on each open.
}

void CWndArea::SetupDone( )
{

    ASSERT_STRICT_VALID( this );

    BldgCurOff( );
    std::string sMsg = EnLoadStdString( IDS_MSG_ROCKET_DONE );
    SetStatusText( sMsg.c_str( ) );
    InvalidateStatus( );

    // CDlgFile removed (Phase 2d) — SDL2FileDialog rebuilds state on each open.
}

void CWndArea::SelectOff( )
{

    ASSERT_STRICT_VALID( this );

    BldgCurOff( );

    m_uFlags     = 0;
    m_uMouseMode = lmb_nothing;
    m_iMode      = normal;
    m_iBuild     = 0;

    SetStatusText( "" );

    theGame.Event( EVENT_CONST_LOC, EVENT_OFF );

    InvalidateStatus( );
}

void CWndArea::BuildOn( int iIndex )
{

    if ( m_pUnit == NULL )
    {
        ASSERT( FALSE );
        return;
    }

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );
    ASSERT_STRICT( ( 0 <= iIndex ) && ( iIndex < theStructures.GetNumBuildings( ) ) );

    // for events
    ( (CVehicle*)m_pUnit )->SetEvent( CVehicle::none );
    ( (CVehicle*)m_pUnit )->SetBldgType( iIndex );

    CStructureData const* pData = theStructures.GetData( iIndex );

    std::string sText = m_pUnit->GetData( )->GetDesc( ) + " - [" +
                        pData->GetDesc( ) + "]";
    SetWindowText( sText.c_str( ) );

    theGame.Event( EVENT_CONST_LOC, EVENT_NOTIFY, m_pUnit );

    m_iMode = build_ready;
    AreaApplyCursor( NULL );
    m_iBuild    = iIndex;
    m_iBuildDir = ( pData->GetExitDir( ) - 2 ) & 0x03;
    SetButtonState( );

    CPoint point;
    ::GetCursorPos( &point );
    ScreenToClient( &point );
    OnMouseMove( 0, point );
}

void CWndArea::GotoOn( CVehicle* pUnit, int iMode, int iRouteType, POSITION posRoute )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( m_pUnit == pUnit );
    ASSERT_STRICT_VALID( pUnit );
    ASSERT_STRICT( iMode == veh_route );

    std::string sMsg = strPrintf( EnLoadStdString( IDS_MSG_UNIT_GOTO ).c_str(),
                                  pUnit->GetData( )->GetDesc( ).c_str() );
    SetStatusText( sMsg.c_str() );

    m_lstUnits.RemoveAllUnits( TRUE );
    m_lstUnits.AddUnit( pUnit, TRUE );
    m_pUnit = pUnit;

    m_iMode      = iMode;
    m_iRouteType = iRouteType;
    m_posRoute   = posRoute;

    SetButtonState( );

    AreaApplyCursor( m_hCurGoto[m_aa.m_iZoom] );
}

void CWndArea::Center( CMapLoc maploc )
{
    m_aa.SetCenter( maploc, CAnimAtr::SET_CENTER_SCROLL );

    ASSERT_STRICT_VALID( this );

    m_bUpdateAll = TRUE;
    // GGTESTING InvalidateWindow ();

    theApp.m_wndWorld.NewLocation( );
}

void CWndArea::Center( CUnit* pUnit )
{
    Center( pUnit->GetWorldPixels( ) );

    // GGTESTING	m_aa.SetCenter( pUnit->GetWorldPixels () );

    ASSERT_STRICT_VALID( this );

    // GGTESTING	InvalidateWindow ();
    theApp.m_wndWorld.NewLocation( );
}

void CWndArea::LastCombat( )
{

    Center( theAreaList.GetLastAttack( ) );
}

void CWndArea::CenterUnit( )
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // if no units center on rocket
    if ( m_lstUnits.GetCount( ) <= 0 )
    {
        CBuilding* pBldg = theBuildingMap.GetBldg( theGame.GetMe( )->m_dwIDRocket );
        if ( pBldg != NULL )
            Center( pBldg->GetWorldPixels( ) );
        return;
    }

    // average all selected
    int      x = 0, y = 0;
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        x += pUnit->GetWorldPixels( ).x;
        y += pUnit->GetWorldPixels( ).y;
    }

    Center( CMapLoc( x / m_lstUnits.GetCount( ), y / m_lstUnits.GetCount( ) ) );

    // GGTESTING	InvalidateWindow ();
}

void CWndArea::DestroyUnit( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // destroy all selected
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        pUnit->SetDestroyUnit( );
    }
}

void CWndArea::StopDestroyUnit( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // destroy all selected
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        pUnit->StopDestroyUnit( );
    }
}

void CWndArea::OppoUnit( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // set all selected to oppo fire
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        pUnit->_SetTarget( NULL );
        pUnit->SetOppo( NULL );
    }
}

void CWndArea::OnRButtonDown( UINT nFlags, CPoint point )
{

    // if its CTRL & a building we change it's facing
    // (Ctrl+Shift falls through: that's the force-attack modifier — SetMouseState
    // sets lmb_attack for it — so it must reach the command dispatch below)
    if ( ( nFlags & MK_CONTROL ) && !( nFlags & MK_SHIFT ) )
    {
        if ( ( m_iMode == build_ready ) || ( m_iMode == rocket_ready ) )
        {
            m_iBuildDir = ( m_iBuildDir - 1 ) & 0x03;
            SetMouseState( );
            OnMouseMove( nFlags, point );
        }
        return;
    }

    if ( ( nFlags & MK_SHIFT ) && !( nFlags & MK_CONTROL ) )
    {
        CHitInfo hitinfo = m_aa.GetHit( point );
        CUnit*   pUnitOn = hitinfo.GetUnit( );
        ASSERT_STRICT_VALID_OR_NULL( pUnitOn );
        // F2: Shift+RMB on a UNIT shows the info tooltip (existing). On EMPTY GROUND it
        // queues a movement waypoint immediately (handled right after this block).
        if ( pUnitOn != NULL )
        {

#ifdef _CHEAT
        if ( !_bClickAny )
#endif
            // not us and not alliance
            if ( ( !pUnitOn->GetOwner( )->IsMe( ) ) &&
                 ( pUnitOn->GetOwner( )->GetTheirRelations( ) != RELATIONS_ALLIANCE ) )
                return;

        // the info panel dismisses when the cursor moves >4px from here (OnMouseMove)
        m_ptRMDN = point;

        // SDL2 unit info tooltip
        if ( theApp.m_gameWindow ) {
            if ( m_pSdlInfo == NULL )
                m_pSdlInfo = new SDL2UnitInfoPanel();

            // Convert client point to screen coordinates for panel positioning
            CPoint ptScreen = point;
            ClientToScreen( &ptScreen );
            // Convert to SDL window coords (relative to game window)
            RECT sdlRect = {};
#ifdef _WIN32
            // Position the tooltip relative to the native HWND. On Linux the
            // SDL_SysWMinfo union member differs (x11/wayland) and the shim's
            // GetWindowRect is a stub, so leave sdlRect at {0} (top-left origin).
            SDL_SysWMinfo wmInfo;
            SDL_VERSION( &wmInfo.version );
            if ( SDL_GetWindowWMInfo( theApp.m_gameWindow->GetWindow(), &wmInfo ) )
                ::GetWindowRect( wmInfo.info.win.window, &sdlRect );
#endif
            int sx = ptScreen.x - sdlRect.left + 16;
            int sy = ptScreen.y - sdlRect.top + 16;
            m_pSdlInfo->Show( pUnitOn, sx, sy );
            return;
        }

        // MFC fallback
        if ( m_pWndInfo == NULL )
            m_pWndInfo = new CWndInfo( );

        if ( m_pWndInfo->m_hWnd == NULL )
            m_pWndInfo->Create( point, pUnitOn, this );

        m_pWndInfo->ShowWindow( SW_SHOW );
        m_pWndInfo->UpdateWindow( );
        return;
        }   // end if ( pUnitOn != NULL )

        // empty ground + Shift: CAPTURE Shift at press (it may be released before button-up)
        // and ARM a command/line-move drag — exactly like the non-Shift path below, so a
        // Shift+DRAG becomes a QUEUED line-move and a Shift+click (no drag) queues a single
        // waypoint. Dispatch is on OnRButtonUp using the captured m_bRmbShift. (Was: fired a
        // single-point queue on the PRESS, which made Shift+line-movement impossible — the
        // operator wants to queue line moves with Shift held.)
        if ( m_iMode == normal )
        {
            m_bRmbShift   = TRUE;
            m_bRmbCmdDown = TRUE;
            m_ptRMDN      = point;
            m_bLineMove   = FALSE;
            s_linePath.clear( );
            m_lineEnd     = point;
            CaptureMouse( );
        }
        return;
    }

    // Modern RTS: a plain right-click during a pending placement cancels it.
    // Rocket placement is mandatory (Escape can't cancel it either — see
    // OnDeselect), so only the normal build placement cancels.
    if ( ( m_iMode == build_ready ) || ( m_iMode == build_loc ) )
    {
        CancelBuildUnit( );  // same path as the area-bar Cancel Build button
        return;
    }
    // ...and cancels a pending road / building-repair targeting mode.
    if ( ( m_iMode == road_begin ) || ( m_iMode == road_set ) )
    {
        CancelRoadUnit( );
        return;
    }
    if ( m_iMode == repair_bldg )
    {
        CancelRepairUnit( );
        return;
    }

    // RMB = command button: arm a click-command / line-move drag. The decision
    // happens in OnMouseMove (drag with 2+ units = line move) and the dispatch
    // on OnRButtonUp (DoCommandAt).
    if ( m_iMode != normal )
        return;

    m_bRmbShift   = FALSE;   // plain (non-Shift) RMB command/line-move
    m_bRmbCmdDown = TRUE;
    m_ptRMDN      = point;
    m_bLineMove   = FALSE;
    s_linePath.clear( );
    m_lineEnd = point;
    CaptureMouse( );
}

void CWndArea::OnRButtonUp( UINT nFlags, CPoint point )
{

    if ( !m_bRmbCmdDown )
        return;
    m_bRmbCmdDown = FALSE;
    ReleaseMouse( );

    // Re-inject the Shift captured at PRESS so the queue-vs-replace decision survives a Shift
    // release during the drag (DoCommandAt reads MK_SHIFT; DoLineMove reads m_bRmbShift).
    if ( m_bRmbShift )
        nFlags |= MK_SHIFT;

    // drag was a line move: distribute the selected units along the drawn line
    if ( m_bLineMove )
    {
        DoLineMove( point );        // Shift-at-press -> QUEUES the line move (reads m_bRmbShift)
        m_bLineMove = FALSE;
        m_bRmbShift = FALSE;

        // "moving" acknowledgement + UI refresh, mirroring a normal goto
        if ( m_uFlags & crane )
            theGame.MulEvent( MEVENT_GO_CRANE, m_pUnit );
        else if ( m_uFlags & veh )
            theGame.MulEvent( MEVENT_GO_COMBAT, m_pUnit );

        SetButtonState( );
        InvalidateStatus( );
        InvalidateSound( );
        return;
    }

    // plain right-click (no drag): issue the context command. With Shift (captured at press,
    // re-injected above) DoCommandAt QUEUES a single waypoint; without it, replaces.
    m_bRmbShift = FALSE;
    if ( m_iMode == normal )
        DoCommandAt( nFlags, point );
}

// RMB double-click: with units selected the clicks are commands (handled by
// OnRButtonDown/Up); with nothing selected keep the original center-on-location.
void CWndArea::OnRButtonDblClk( UINT nFlags, CPoint pt )
{

    // if we're modifying we don't do this
    if ( nFlags & ( MK_CONTROL | MK_SHIFT ) )
    {
        OnRButtonDown( nFlags, pt );
        return;
    }

    if ( m_lstUnits.GetCount( ) > 0 )
    {
        OnRButtonDown( nFlags, pt );
        return;
    }

    m_bRmbCmdDown = FALSE;
    ReleaseMouse( );

    CHexCoord hexcoord = m_aa.WindowToHex( pt );

    Center( CMapLoc( hexcoord ) );

    ASSERT_STRICT_VALID( &theMap );
    // GGTESTING	InvalidateWindow ();

    m_bNewPos = TRUE;
}

// MMB held = pan: drag-pan in OnMouseMove plus the original continuous
// edge-band scroll in ReRender (this was the RMB behavior pre-2026).
void CWndArea::OnMButtonDown( UINT /*nFlags*/, CPoint point )
{

    m_bPanBtnDown = TRUE;
    m_ptRMB       = point;

    CaptureMouse( );
    theApp.m_wndBar.SetStatusText( 1, m_sHelpRMB.c_str( ) );

    CRect rect;
    GetClientRect( &rect );

    // figure out the cursor
    CPoint pt;
    ::GetCursorPos( &pt );
    ScreenToClient( &pt );

    int x, y;
    int iWid = rect.Width( );
    if ( pt.x < iWid / 8 )
        x = -1;
    else
    {
        int iTmp = iWid - iWid / 8;
        if ( pt.x > iTmp )
            x = 1;
        else
            x = 0;
    }
    int iHt = rect.Height( );
    if ( pt.y < iHt / 8 )
        y = -1;
    else
    {
        int iTmp = iHt - iHt / 8;
        if ( pt.y > iTmp )
            y = 1;
        else
            y = 0;
    }

    // which cursor?
    switch ( ( x + 2 ) | ( ( y + 2 ) << 2 ) )
    {
    case 0x06:  // up
        m_iMoveCur = 0;
        break;
    case 0x07:  // UR
        m_iMoveCur = 1;
        break;
    case 0x0B:  // right
        m_iMoveCur = 2;
        break;
    case 0x0F:  // LR
        m_iMoveCur = 3;
        break;
    case 0x0E:  // down
        m_iMoveCur = 4;
        break;
    case 0x0D:  // LL
        m_iMoveCur = 5;
        break;
    case 0x09:  // left
        m_iMoveCur = 6;
        break;
    case 0x05:  // UL
        m_iMoveCur = 7;
        break;
    default:
        m_iMoveCur = 8;
        break;
    }

    AreaApplyCursor( m_hCurMove[m_iMoveCur] );

    // Only clip cursor to window when using MFC input (not SDL)
    if ( !m_aa.m_sdlPanel )
    {
        ClientToScreen( &rect );
        ::ClipCursor( &rect );
    }
}

void CWndArea::OnMButtonUp( UINT, CPoint )
{

    m_bPanBtnDown = FALSE;
    ::ClipCursor( NULL );
    ReleaseMouse( );
    theApp.m_wndWorld.NewLocation( );

    if ( m_bNewPos )
        InvalidateSound( );

    theApp.m_wndBar.SetStatusText( 1, m_sHelp.c_str( ) );
}

//---------------------------------------------------------------------------
// CWndArea::DoCommandAt
// Issue the context command for the current selection at `point` — the action
// half of the original (1996) OnLButtonUp normal_select case, now driven by
// the right button. Dispatches on m_uMouseMode, which SetMouseState keeps
// current from the hover position (it also drives the cursor, so the cursor
// always previews what this will do).
//---------------------------------------------------------------------------
void CWndArea::DoCommandAt( UINT nFlags, CPoint point )
{

    ASSERT_STRICT_VALID( this );

    // no selection = nothing to command
    if ( m_lstUnits.GetCount( ) == 0 )
        return;

    const BOOL bShift = ( nFlags & MK_SHIFT ) != 0;   // F2: queue instead of replace

    // A normal (non-Shift) command overrides a one-shot Shift-queue — clear it so the
    // vehicle doesn't keep running queued waypoints. (Loop routes are preserved.)
    if ( !bShift )
        for ( POSITION pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                StopRoute( (CVehicle*)pUnit );
        }

    CSubHex _sub = m_aa.WindowToSubHex( point );
    _sub.Wrap( );

    CHitInfo     hitinfo = m_aa.GetHit( point );
    CUnit*       pUnitOn = hitinfo.GetUnit( );
    CBridgeUnit* pBu     = hitinfo.GetBridge( );
    // if not visible then it's not there
    if ( pUnitOn != NULL )
        if ( !pUnitOn->IsVisible( ) )
            pUnitOn = NULL;
    ASSERT_STRICT_VALID_OR_NULL( pUnitOn );

    BOOL bMoveAck = FALSE;  // play the "moving" voice at the end

    switch ( m_uMouseMode )
    {
    // send the selected crane(s) to repair the building / bridge under the cursor
    case lmb_repair_bldg: {
        if ( ( pBu == NULL ) && ( ( pUnitOn == NULL ) || ( pUnitOn->GetUnitType( ) != CUnit::building ) ) )
            return;

        CHexCoord _hexDest;
        if ( pBu != NULL )
            _hexDest = pBu->GetHex( );
        else
            _hexDest = ( (CBuilding*)pUnitOn )->GetExitHex( );
        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            POSITION prev_pos = pos;
            CUnit*   pUnit    = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            if ( ( pUnit->GetUnitType( ) == CUnit::vehicle ) &&
                 ( ( (CVehicle*)pUnit )->GetData( )->IsCrane( ) ) )
            {
                pUnit->ResumeUnit( );
                ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_bldg );
                SetDestAndSfx( (CVehicle*)pUnit, _hexDest );

                // deselect it if it's going to be repaired
                pUnit->SetUnselected( TRUE );
                m_lstUnits.RemoveAt( prev_pos );
            }
        }
        break;
    }

    // send the selected damaged unit(s) to the repair facility under the cursor
    case lmb_repair_self: {
        if ( ( pUnitOn == NULL ) || ( pUnitOn->GetUnitType( ) != CUnit::building ) )
            return;

        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            POSITION prev_pos = pos;
            CUnit*   pUnit    = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            if ( ( pUnit->GetUnitType( ) == CUnit::vehicle ) &&
                 ( ( (CVehicle*)pUnit )->GetData( )->IsRepairable( ) ) )
            {
                pUnit->ResumeUnit( );
                ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_self );
                if ( ( (CVehicle*)pUnit )->GetData( )->IsBoat( ) )
                    SetDestAndSfx( (CVehicle*)pUnit, ( (CBuilding*)pUnitOn )->GetShipHex( ) );
                else
                    SetDestAndSfx( (CVehicle*)pUnit, ( (CBuilding*)pUnitOn )->GetExitHex( ) );

                // deselect it if it's going to be repaired
                pUnit->SetUnselected( TRUE );
                m_lstUnits.RemoveAt( prev_pos );
            }
        }
        break;
    }

    // load the selected carryable unit(s) onto the carrier under the cursor
    case lmb_load: {
        if ( ( pUnitOn == NULL ) || ( pUnitOn->GetUnitType( ) != CUnit::vehicle ) ||
             ( !( (CVehicle*)pUnitOn )->GetData( )->IsCarrier( ) ) )
            return;

        CSubHex _subLoad;
        if ( ( (CVehicle*)pUnitOn )->GetData( )->GetVehFlags( ) & CTransportData::FLload_front )
            _subLoad = ( (CVehicle*)pUnitOn )->GetPtHead( );
        else
            _subLoad = ( (CVehicle*)pUnitOn )->GetPtTail( );

        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            if ( ( ( pUnit->GetUnitType( ) == CUnit::vehicle ) &&
                   ( ( (CVehicle*)pUnit )->GetData( )->IsCarryable( ) ) ) ||
                 ( ( ( (CVehicle*)pUnitOn )->GetData( )->IsBoat( ) ) &&
                   ( ( (CVehicle*)pUnit )->GetData( )->IsLcCarryable( ) ) ) )
            {
                pUnit->ResumeUnit( );
                ( (CVehicle*)pUnit )->SetEvent( CVehicle::load );
                SetDestAndSfx( (CVehicle*)pUnit, _subLoad );
                ( (CVehicle*)pUnit )->SetLoadOn( (CVehicle*)pUnitOn );

                // deselect it
                m_lstUnits.RemoveUnit( pUnit );
            }
        }
        break;
    }

    // unload the selected carrier (it is the unit under the cursor)
    case lmb_unload: {
        if ( ( pUnitOn == NULL ) || ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) ||
             ( ( (CVehicle*)m_pUnit )->GetCargoCount( ) <= 0 ) )
            return;

        CMsgUnloadCarrier _msg( (CVehicle*)pUnitOn );
        theGame.PostToClient( theGame.GetMe( ), &_msg, sizeof( _msg ) );
        break;
    }

    // attack the unit under the cursor
    case lmb_attack: {
        if ( pUnitOn == NULL )
            return;
        bMoveAck = TRUE;

        // we want to spread out the vehicles dest if there are a lot of them
        //   (unless we're going to a unit)
        int     iDestRand = 0;
        CSubHex _subDest( _sub );
        // see if going to a bridge
        if ( pBu != NULL )
            _subDest = pBu->GetHex( );

        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            pUnit->ResumeUnit( );

            // if it can attack we set it to attack. Otherwise we set it to go there
            if ( pUnit->GetData( )->_GetFireRate( ) > 0 )
            {
                // get it going if too far away or dest not visible
                if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                {
                    CVehicle* pVeh = ( (CVehicle*)pUnit );
                    pVeh->TempTargetOff( );
                    CSubHex _subAtk;
                    // get closest point
                    if ( pUnitOn->GetUnitType( ) == CUnit::vehicle )
                        _subAtk = ( (CVehicle*)pUnitOn )->GetPtHead( );
                    else if ( pUnitOn->GetUnitType( ) == CUnit::building )
                    {
                        CHexCoord _hex;
                        _hex = pVeh->GetPtHead( );

                        CBuilding* pBldg = (CBuilding*)pUnitOn;
                        if ( _hex.X( ) < pBldg->GetHex( ).X( ) )
                            _hex.X( ) = pBldg->GetHex( ).X( ) - 1;
                        else if ( _hex.X( ) > pBldg->GetHex( ).X( ) + pBldg->GetCX( ) )
                            _hex.X( ) = pBldg->GetHex( ).X( ) + pBldg->GetCX( );
                        if ( _hex.Y( ) < pBldg->GetHex( ).Y( ) )
                            _hex.Y( ) = pBldg->GetHex( ).Y( ) - 1;
                        else if ( _hex.Y( ) > pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) )
                            _hex.Y( ) = pBldg->GetHex( ).Y( ) + pBldg->GetCY( );
                        _hex.Wrap( );
                        _subAtk = _hex;
                    }

                    // if not visible - go toward it
                    if ( theMap._GetHex( _subAtk )->GetVisible( ) == 0 )
                        SetDestAndSfx( pVeh, _subAtk );
                    else
                    {
                        // too far away - go for it
                        int iLOS = theMap.LineOfSight( pVeh, pUnitOn );
                        if ( ( ( iLOS < 0 ) &&
                               ( pVeh->GetData( )->GetBaseType( ) != CTransportData::artillery ) ) ||
                             ( abs( iLOS ) > pVeh->GetRange( ) - 1 ) )
                            SetDestAndSfx( pVeh, _subAtk );
                    }
                }

                pUnit->MsgSetTarget( pUnitOn );
                NewRelations( pUnitOn->GetOwner( ), RELATIONS_WAR );
            }

            else if ( pUnit->GetUnitType( ) == CUnit::vehicle )
            {
                CVehicle* pVeh = ( (CVehicle*)pUnit );
                pVeh->SetEvent( CVehicle::none );
                CSubHex _subVeh( _subDest.x + RandNum( iDestRand ), _subDest.y + RandNum( iDestRand ) );
                _subVeh.Wrap( );
                int iCost = theMap.GetTerrainCost( _subVeh, _subVeh, 0, pVeh->GetData( )->GetWheelType( ) );
                if ( ( iCost == 0 ) ||
                     ( iCost > theMap.GetTerrainCost( _sub, _sub, 0, pVeh->GetData( )->GetWheelType( ) ) * 2 ) )
                    _subVeh = _sub;
                SetDestAndSfx( pVeh, _subVeh );
            }
        }
        break;
    }

    // a goto. lmb_select = right-clicked one of our own units: treat it as a
    // move-to as well (the selection role of that hover lives on the LMB now).
    case lmb_select:
    case lmb_goto: {
        bMoveAck = TRUE;

        // F2: Shift-click queues a movement waypoint (one-shot route) on every selected
        // vehicle instead of replacing its order. Bypasses the formation-move path below.
        if ( bShift )
        {
            for ( POSITION pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                    ShiftQueueMove( (CVehicle*)pUnit, _sub );
            }
            break;
        }

        // if we have 1 unit or are going to a building - send them direct
        BOOL bDestIsBldg = theBuildingHex._GetBuilding( _sub ) != NULL;
        if ( ( m_lstUnits.GetCount( ) <= 1 ) || bDestIsBldg )
        {
            POSITION pos;
            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                {
                    CVehicle* pVeh = ( (CVehicle*)pUnit );

                    // send it
                    pVeh->TempTargetOff( );
                    pVeh->SetEvent( CVehicle::none );
                    pVeh->ResumeUnit( );
                    SetDestAndSfx( pVeh, _sub );
                    pVeh->_SetTarget( NULL );

                    // goto building to pick up goods
                    if ( ( pVeh->GetData( )->IsTransport( ) ) && bDestIsBldg )
                    {
                        if ( !pVeh->IsHpControl( ) )
                        {
                            theGame.m_pHpRtr->MsgTakeVeh( (CVehicle*)pUnit );
                            pVeh->HpControlOn( );
                        }
                    }
                }
            }
        }
        else

        // we want to hold formation but maybe bring it in and add some randomnesses
        {
            // so first we find the center of where we are
            int      x = 0, y = 0;
            POSITION pos;
            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                x += pUnit->GetWorldPixels( ).x;
                y += pUnit->GetWorldPixels( ).y;
            }
            CSubHex _subSrc( CMapLoc( x / m_lstUnits.GetCount( ), y / m_lstUnits.GetCount( ) ) );

            // now we find the furthest away from that center (for proportional dist at dest)
            int xMaxDist = 1, yMaxDist = 1;
            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                {
                    CVehicle* pVeh  = ( (CVehicle*)pUnit );
                    int       iDist = abs( _subSrc.x - pVeh->GetPtHead( ).x );
                    xMaxDist        = __max( xMaxDist, iDist );
                    iDist           = abs( _subSrc.y - pVeh->GetPtHead( ).y );
                    yMaxDist        = __max( yMaxDist, iDist );
                }
            }

            // and the furthest we want them apart is
            int iDestDist = (int)sqrt( (float)m_lstUnits.GetCount( ) ) + 1;
            iDestDist += iDestDist / 2;
            int iRandDist = iDestDist / 4;

            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                {
                    CVehicle* pVeh = ( (CVehicle*)pUnit );

                    CSubHex _subVeh;
                    _subVeh.x = _sub.x +
                                ( ( pVeh->GetPtHead( ).x - _subSrc.x ) * iDestDist + xMaxDist / 2 ) / xMaxDist +
                                RandNum( iRandDist ) - iRandDist / 2;
                    _subVeh.y = _sub.y +
                                ( ( pVeh->GetPtHead( ).y - _subSrc.y ) * iDestDist + yMaxDist / 2 ) / yMaxDist +
                                RandNum( iRandDist ) - iRandDist / 2;
                    _subVeh.Wrap( );
                    int iCost    = theMap.GetTerrainCost( _subVeh, _subVeh, 0, pVeh->GetData( )->GetWheelType( ) );
                    int iMaxCost = 3 * theMap.GetTerrainCost( _sub, _sub, 0, pVeh->GetData( )->GetWheelType( ) );
                    if ( ( iCost == 0 ) || ( iCost > iMaxCost ) )
                        _subVeh = _sub;

                    // send it
                    pVeh->TempTargetOff( );
                    pVeh->SetEvent( CVehicle::none );
                    pVeh->ResumeUnit( );
                    SetDestAndSfx( pVeh, _subVeh );
                    pVeh->_SetTarget( NULL );

                    // goto building to pick up goods
                    if ( ( pVeh->GetData( )->IsTransport( ) ) && bDestIsBldg )
                    {
                        if ( !pVeh->IsHpControl( ) )
                        {
                            theGame.m_pHpRtr->MsgTakeVeh( pVeh );
                            pVeh->HpControlOn( );
                        }
                    }
                }
            }
        }
        break;
    }

    default:
        // lmb_nothing (or a mode with no command meaning here)
        return;
    }

    // selection bookkeeping (the repair/load commands deselect dispatched units)
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;

    // set button states
    SetButtonState( );

    // repaint it
    InvalidateStatus( );
    InvalidateSound( );

    // voices? say ok
    if ( bMoveAck )
    {
        if ( m_uFlags & crane )
            theGame.MulEvent( MEVENT_GO_CRANE, m_pUnit );
        else if ( m_uFlags & veh )
            theGame.MulEvent( MEVENT_GO_COMBAT, m_pUnit );
    }
}

void CWndArea::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
{

    // on loosing activation we give up the cursor and any pending drag gesture
    m_bPanBtnDown = FALSE;
    m_bRmbCmdDown = FALSE;
    m_bLineMove   = FALSE;
    ReleaseMouse( );
    ::ClipCursor( NULL );

    theApp.m_wndBldgs.m_ListBox.SetRedraw( FALSE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( FALSE );

    // if we loose activation we undo our selections
    if ( nState == WA_INACTIVE )
    {
        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            pUnit->SetUnselected( TRUE );
        }
    }

    // if we gain activation we set our selections
    else
    {
        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            pUnit->SetSelected( TRUE );
        }
        theApp.m_wndWorld.NewAreaMap( this );
    }

    theApp.m_wndBldgs.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL, FALSE );
    theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL, FALSE );

    CWndAnim::OnActivate( nState, pWndOther, bMinimized );
}

void CWndArea::OnLButtonDblClk( UINT nFlags, CPoint point )
{

    ReleaseMouse( );

    CHitInfo hitinfo = m_aa.GetHit( point );
    CUnit*   punit   = hitinfo.GetUnit( );
    // if not visible then it's not there
    if ( punit != NULL )
        if ( !punit->IsVisible( ) )
            punit = NULL;

    ASSERT_STRICT_VALID_OR_NULL( punit );

    // if ctrl is down we bring up a new window
    if ( nFlags & MK_CONTROL )
    {
        // Radio research unlocks additional area-map windows (original behavior).
        // The SDL2 panel for an area window is built by CWndArea::OnCreate, which
        // already indexes panels ("area_N") and supports multiple instances — the
        // same new-CWndArea-then-Create path CWndBar::GotoArea uses. The old
        // !m_gameWindow guard disabled this entirely in SDL2 mode; drop it.
        if ( theGame.GetMe( )->CanMultiArea( ) )
        {
            CWndArea* pWndArea = new CWndArea( );
            pWndArea->Create( hitinfo._GetHexCoord( ), punit, FALSE );
        }
        return;
    }

    if ( punit == NULL )
        return;

    // if it's not ours we return
    if ( !punit->GetOwner( )->IsMe( ) )
        return;

    // this has to be our selected unit to work
    if ( punit != m_pUnit )
        return;

    // we bring up the control window for this unit
    //   it's a building
    if ( punit->GetUnitType( ) == CUnit::building )
    {
        if ( ( (CBuilding*)punit )->IsConstructing( ) )
            return;

        switch ( ( (CBuilding*)punit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UTshipyard:
            ( (CVehicleBuilding*)punit )->GetDlgBuild( );
            return;
        case CStructureData::UTresearch:
            theApp.m_wndBar._GotoScience( );
            return;
        case CStructureData::UTembassy:
            theApp.m_wndBar.GotoRelations( );
            return;
        // Warehouse / rocket (storage), power plants, housing, and resource producers
        // (mines / farms / smelters / refineries) open the read-only building-info
        // window (the rocket shows storage + power + housing + turret).
        case CStructureData::UTwarehouse:
        case CStructureData::UTpower:
        case CStructureData::UThousing:
        case CStructureData::UTmaterials:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTfort:        // pillboxes / bunkers / forts (weapon widget)
        case CStructureData::UTcommand:     // command center (military summary + turret)
        case CStructureData::UTrepair:      // repair building (live repair queue)
            // Note 26 (edict (i) tooltip behind the map) is handled at the
            // SDL2BuildingWindow ctor level (keep-on-top = bOnTop || secEdicts,
            // @3cdca984) so edict-host windows float above the map regardless of
            // open path; non-edict windows stay tuckable-by-design — so this
            // area-map open stays bOnTop=false (default).
            ( (CBuilding*)punit )->ShowInfoWindow( );
            return;
        }

        // if a truck is in there (not on auto) bring up the load dialog
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( !pVeh->GetHexOwnership( ) ) &&
                 ( pVeh->GetData( )->IsTransport( ) ) && ( pVeh->IsHpControl( ) ) &&
                 ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == punit ) )
            {
                pVeh->ShowLoadDialog( );
                break;
            }
        }

        return;
    }

    if ( punit->GetUnitType( ) != CUnit::vehicle )
        return;

    // it's a vehicle - we do crane & other (route)
    if ( ( (CVehicle*)punit )->GetData( )->IsCrane( ) )
    {
        ( (CVehicle*)punit )->GetDlgBuild( );
        return;
    }

    // if someone wants to route a non-truck - fine
    CVehicle* pVeh = (CVehicle*)punit;

    if (!pVeh->m_pSdlRoute) {
        pVeh->m_pSdlRoute = new SDL2RouteWindow(theApp.m_gameWindow.get(), pVeh, m_aa.m_sdlPanel);
        pVeh->m_pSdlRoute->Show();
    }
    SetButtonState();
}

void CWndArea::InvalidateWindow( RECT* )
{

    m_bUpdateAll = TRUE;

    m_aa.GetDirtyRects( )->AddRect( NULL );

    if ( !m_bPanBtnDown )
        InvalidateSound( );

    theApp.m_wndWorld.NewLocation( );
}

void CWndArea::CancelBuildUnit( )
{

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        pUnit->CancelUnit( );
    }

    SelectOff( );
    SetButtonState( );
}

void CWndArea::StopUnit( )
{

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        pUnit->StopUnit( );
    }
    SetButtonState( );
}

void CWndArea::ResumeUnit( )
{

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        pUnit->ResumeUnit( );
    }
    SetButtonState( );
}

void CWndArea::BuildUnit( )
{

    ASSERT_STRICT_VALID( this );
    if ( m_pUnit == NULL )
    {
        ASSERT_STRICT( FALSE );
        return;
    }
    ASSERT_STRICT_VALID( m_pUnit );

    if ( m_pUnit->GetUnitType( ) == CUnit::building )
    {
        // can't do this if still under construction
        if ( ( (CBuilding*)m_pUnit )->IsConstructing( ) )
            return;

        switch ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UTshipyard:
            ( (CVehicleBuilding*)m_pUnit )->GetDlgBuild( );
            return;

        case CStructureData::UTresearch:
            TRAP( );
            theApp.m_wndBar._GotoScience( );
            return;

#ifdef _DEBUG
        default:
            ASSERT_STRICT( FALSE );
            return;
#endif
        }
    }

    ASSERT_STRICT( m_pUnit->GetUnitType( ) == CUnit::vehicle );

    // have to be a construction vehicle
    if ( !( ( (CVehicle*)m_pUnit )->GetData( )->IsCrane( ) ) )
    {
        ASSERT_STRICT( FALSE );
        return;
    }
    ( (CVehicle*)m_pUnit )->GetDlgBuild( );
}

void CWndArea::RouteUnit( )
{

    ASSERT_STRICT_VALID( this );

    if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
        return;
    if ( m_pUnit->GetFlags( ) & CUnit::dying )
        return;
    CVehicle* pVeh = (CVehicle*)m_pUnit;
    ASSERT_STRICT_VALID( pVeh );

    // Use SDL2 route window
    if (!pVeh->m_pSdlRoute) {
        pVeh->m_pSdlRoute = new SDL2RouteWindow(theApp.m_gameWindow.get(), pVeh, m_aa.m_sdlPanel);
        pVeh->m_pSdlRoute->Show();
    }
    SetButtonState();
}

// 'R' hotkey dispatcher. A crane builds a road; any other vehicle sets a route.
// The two are mutually exclusive per unit (a crane can't have a route, a route-
// capable transport can't lay road), so this is purely additive — pressing R did
// nothing for a selected crane before. The area-bar shows "(R)" on whichever of
// the Road / Route buttons is visible for the current selection.
void CWndArea::RoadOrRoute( )
{

    if ( ( m_pUnit != NULL ) && ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) &&
         ( ( (CVehicle*)m_pUnit )->GetData( )->IsCrane( ) ) )
        RoadUnit( );
    else
        RouteUnit( );
}

void CWndArea::RoadUnit( )
{

    ASSERT_VALID( this );

    if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
        return;

    m_iMode = road_begin;

    AreaApplyCursor( m_hCurRoadBgn[m_aa.m_iZoom] );
    SetButtonState( );
}

void CWndArea::CancelRoadUnit( )
{

    ReleaseMouse( );
    SelectOff( );
    SetButtonState( );
}

void CWndArea::RepairUnit( )
{

    ASSERT_VALID( this );

    if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
        return;

    m_iMode = repair_bldg;

    AreaApplyCursor( m_hCurNoRepair );
    SetButtonState( );
}

void CWndArea::CancelRepairUnit( )
{

    ReleaseMouse( );
    SelectOff( );
    SetButtonState( );
}

void CWndArea::UnloadUnit( )
{

    ASSERT_VALID( this );

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_VALID( pUnit );
        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
            if ( ( (CVehicle*)pUnit )->GetCargoCount( ) > 0 )
            {
                CMsgUnloadCarrier _msg( (CVehicle*)pUnit );
                theGame.PostToClient( theGame.GetMe( ), &_msg, sizeof( _msg ) );
            }
    }

    SetButtonState( );
}

void CWndArea::RetreatUnit( )
{

    ASSERT_VALID( this );

    // find where to send them
    // 1: repair facility
    // 2: away
    CBuilding* pDest = NULL;
    POSITION   pos   = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );
        if ( pBldg->GetOwner( )->IsMe( ) )
            if ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
            {
                pDest = pBldg;
                break;
            }
    }

    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_VALID( pUnit );
        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
        {
            pUnit->ResumeUnit( );
            if ( ( pDest != NULL ) && ( ( (CVehicle*)pUnit )->GetDamagePer( ) != 100 ) )
            {
                ( (CVehicle*)pUnit )->SetDest( pDest->GetHex( ) );
                ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_self );
            }
            else
            {
                TRAP( );
                CHexCoord _hex( ( (CVehicle*)pUnit )->GetHexHead( ) );
                _hex.X( ) += RandNum( 20 ) - 10;
                _hex.Y( ) += RandNum( 20 ) - 10;
                _hex.Wrap( );
                ( (CVehicle*)pUnit )->SetDest( _hex );
            }
        }
    }

    SetButtonState( );
}

CUnit* CWndArea::GetUnitOn( CSubHex& _sub )
{

    _sub.Wrap( );
    CUnit* pUnit = theBuildingHex._GetBuilding( _sub );
    if ( pUnit != NULL )
        return ( pUnit );

    return ( theVehicleHex._GetVehicle( _sub ) );
}

// set button states
void CWndArea::SetButtonState( )
{

    // set the title
    if ( m_pUnit == NULL )
    {
        std::string sTitle;
        if ( m_lstUnits.GetCount( ) <= 0 )
            sTitle = EnLoadStdString( IDS_TITLE_AREA_MAP );
        else
            sTitle = strPrintf( EnLoadStdString( IDS_TITLE_AREA_MAP_MULTI ).c_str(),
                                IntToStr( m_lstUnits.GetCount( ) ).c_str() );
        SetWindowText( sTitle.c_str() );
    }
    else
    {
        // special for cranes
        if ( ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) &&
             ( ( (CVehicle*)m_pUnit )->GetData( )->GetType( ) == CTransportData::construction ) && ( m_iBuild > 0 ) )
        {
            std::string sText = m_pUnit->GetData( )->GetDesc( ) + " - [" +
                                theStructures.GetData( m_iBuild )->GetDesc( ) + "]";
            SetWindowText( sText.c_str( ) );
        }
        else
            SetWindowText( m_pUnit->GetData( )->GetDesc( ).c_str() );
    }

    m_uFlags = 0;

    // find out what we have
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );

        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
        {
            m_uFlags |= ( veh | can_stop );
            if ( ( (CVehicle*)pUnit )->GetData( )->IsBoat( ) )
                m_uFlags |= boat;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsCrane( ) )
                m_uFlags |= crane;
            else
                m_uFlags |= non_crane;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsTransport( ) )
                m_uFlags |= truck;
            else
                m_uFlags |= non_truck;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsCarrier( ) )
            {
                m_uFlags |= carrier;
                if ( ( (CVehicle*)pUnit )->GetCargoSize( ) > 0 )
                    m_uFlags |= loaded;
                if ( pUnit->GetDamagePer( ) != 100 )
                    m_uFlags |= veh_hurt;
            }
            else
                m_uFlags |= non_carrier;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsCarryable( ) )
                m_uFlags |= carryable;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsLcCarryable( ) )
                m_uFlags |= lc_carryable;
            if ( ( ( (CVehicle*)pUnit )->GetData( )->IsRepairable( ) ) &&
                 ( pUnit->GetDamagePoints( ) < pUnit->GetData( )->GetDamagePoints( ) ) )
            {
                if ( ( (CVehicle*)pUnit )->GetData( )->IsBoat( ) )
                    m_uFlags |= sea_repairable;
                else
                    m_uFlags |= land_repairable;
            }
        }

        else
        {
            ASSERT_STRICT( pUnit->GetUnitType( ) == CUnit::building );
            m_uFlags |= ( bldg | non_crane | non_truck | non_carrier );
            if ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTvehicle )
                m_uFlags |= ( fac | can_stop );
            if ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTshipyard )
                m_uFlags |= ( fac | repair | can_stop );
            if ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
                m_uFlags |= ( repair | can_stop );
            if ( ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTmaterials ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTrepair ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTpower ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTresearch ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTfarm ) )
                m_uFlags |= can_stop;
        }

        if ( pUnit->GetData( )->_GetFireRate( ) > 0 )
            m_uFlags |= attk;
        if ( pUnit->GetFlags( ) & CUnit::destroying )
            m_uFlags |= destroying;
    }

    int const* pID = asNoneID;
    if ( m_uFlags != 0 )
    {
        // crane(s) only
        if ( ( m_uFlags & ( bldg | non_crane ) ) == 0 )
            pID = asCraneID;
        else
            // truck(s) only
            if ( ( m_uFlags & ( bldg | non_truck ) ) == 0 )
            pID = asTruckID;
        else
            // unloadable vehicles - or all carriers
            if ( ( m_uFlags & loaded ) || ( ( m_uFlags & ( bldg | non_carrier ) ) == 0 ) )
            pID = asUnloadID;
        else
            // vehicles only
            if ( !( m_uFlags & bldg ) )
            pID = asVehID;
        else
            // factory
            if ( ( m_uFlags & fac ) && ( m_lstUnits.GetCount( ) == 1 ) )
            pID = asFacID;
        else
            // bldg only
            if ( !( m_uFlags & veh ) )
            pID = asBldgID;
        else
            // something
            if ( m_lstUnits.GetCount( ) > 0 )
            pID = asUnitID;
    }

    // handle static text width
    int iStatusStart;
    if ( pID == asCraneID )
        iStatusStart = m_WndStatic.m_iStatusCraneStrt;
    else
        iStatusStart = m_WndStatic.m_iStatusNoCraneStrt;
    if ( m_WndStatic.m_iStatusStrt != iStatusStart )
    {
        m_WndStatic.m_iStatusStrt = iStatusStart;
        m_WndStatic.SizeStatus( );
    }

    // set the buttons in pID enabled, other disabled
    int const* pIDon = pID;
    for ( int iOn = ORD_OFFSET; iOn < ORD_OFFSET + NUM_ORD_BUTTONS; iOn++, pIDon++ )
        if ( ( abID[iOn] != IDC_UNIT_RESUME ) && ( abID[iOn] != IDC_UNIT_CANCEL_BUILD ) &&
             ( abID[iOn] != IDC_UNIT_CANCEL_REPAIR ) && ( abID[iOn] != IDC_UNIT_CANCEL_ROAD ) )
        {
            // if reg turned off then so is cancel
            if ( *pIDon != enableID )
            {
                if ( abID[iOn] == IDC_UNIT_STOP )
                {
                    EnableButton( IDC_UNIT_RESUME, FALSE );
                    ShowButton( IDC_UNIT_RESUME, FALSE );
                }
                if ( abID[iOn] == IDC_UNIT_BUILD )
                {
                    EnableButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                }
                if ( abID[iOn] == IDC_UNIT_REPAIR )
                {
                    EnableButton( IDC_UNIT_CANCEL_REPAIR, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_REPAIR, FALSE );
                }
                if ( abID[iOn] == IDC_UNIT_ROAD )
                {
                    EnableButton( IDC_UNIT_CANCEL_ROAD, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_ROAD, FALSE );
                }
            }

            if ( *pIDon == disableID )
            {
                EnableButton( abID[iOn], FALSE );
                ShowButton( abID[iOn], TRUE );
            }
            else if ( *pIDon != enableID )
            {
                EnableButton( abID[iOn], FALSE );
                ShowButton( abID[iOn], FALSE );
            }
            else
                // disable unload
                if ( abID[iOn] == IDC_UNIT_UNLOAD )
            {
                EnableButton( IDC_UNIT_UNLOAD, ( m_uFlags & loaded ) );
                ShowButton( IDC_UNIT_UNLOAD, TRUE );
            }
            else
                // retreat (repair)
                if ( abID[iOn] == IDC_UNIT_RETREAT )
            {
                EnableButton( IDC_UNIT_RETREAT, ( m_uFlags & veh_hurt ) );
                ShowButton( IDC_UNIT_RETREAT, TRUE );
            }
            else
                // cancel road
                if ( abID[iOn] == IDC_UNIT_ROAD )
            {
                BOOL bBuilding = ( ( m_iMode == road_begin ) || ( m_iMode == road_set ) );
                EnableButton( IDC_UNIT_CANCEL_ROAD, bBuilding );
                ShowButton( IDC_UNIT_CANCEL_ROAD, bBuilding );
                EnableButton( IDC_UNIT_ROAD, !bBuilding );
                ShowButton( IDC_UNIT_ROAD, !bBuilding );
            }
            else
                // cancel repair
                if ( abID[iOn] == IDC_UNIT_REPAIR )
            {
                EnableButton( IDC_UNIT_REPAIR, m_iMode != repair_bldg );
                ShowButton( IDC_UNIT_REPAIR, m_iMode != repair_bldg );
                EnableButton( IDC_UNIT_CANCEL_REPAIR, m_iMode == repair_bldg );
                ShowButton( IDC_UNIT_CANCEL_REPAIR, m_iMode == repair_bldg );
            }
            else
                // only 1 crane
                if ( ( abID[iOn] == IDC_UNIT_BUILD ) && ( m_uFlags & crane ) )
            {
                if ( m_lstUnits.GetCount( ) != 1 )
                {
                    EnableButton( IDC_UNIT_BUILD, FALSE );
                    ShowButton( IDC_UNIT_BUILD, TRUE );
                    EnableButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                }
                else
                {
                    EnableButton( IDC_UNIT_BUILD, m_iMode != build_ready );
                    ShowButton( IDC_UNIT_BUILD, m_iMode != build_ready );
                    EnableButton( IDC_UNIT_CANCEL_BUILD, m_iMode == build_ready );
                    ShowButton( IDC_UNIT_CANCEL_BUILD, m_iMode == build_ready );
                }
            }
            else
                // build -> cancel
                if ( abID[iOn] == IDC_UNIT_BUILD )
            {
                BOOL bBuilding;
                if ( ( m_iMode == build_ready ) ||
                     ( ( m_pUnit ) && ( m_pUnit->GetUnitType( ) == CUnit::building ) &&
                       ( !( (CBuilding*)m_pUnit )->IsConstructing( ) ) &&
                       ( ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTvehicle ) ||
                         ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTshipyard ) ) &&
                       ( ( (CVehicleBuilding*)m_pUnit )->GetBldUnt( ) != NULL ) ) )
                    bBuilding = TRUE;
                else
                    bBuilding = FALSE;
                EnableButton( IDC_UNIT_BUILD, !bBuilding );
                ShowButton( IDC_UNIT_BUILD, !bBuilding );
                EnableButton( IDC_UNIT_CANCEL_BUILD, bBuilding );
                ShowButton( IDC_UNIT_CANCEL_BUILD, bBuilding );
            }
            else
                // stop/resume
                if ( abID[iOn] == IDC_UNIT_STOP )
            {
                // if nothing can be stopped - hide it
                if ( !( m_uFlags & can_stop ) )
                {
                    EnableButton( IDC_UNIT_STOP, FALSE );
                    ShowButton( IDC_UNIT_STOP, FALSE );
                    EnableButton( IDC_UNIT_RESUME, FALSE );
                    ShowButton( IDC_UNIT_RESUME, FALSE );
                }
                else
                {
                    POSITION pos;
                    BOOL     bStop = TRUE;
                    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                    {
                        CUnit* pUnit = m_lstUnits.GetNext( pos );
                        ASSERT_STRICT_VALID( pUnit );
                        if ( pUnit->IsPaused( ) )
                        {
                            bStop = FALSE;
                            break;
                        }
                    }
                    EnableButton( IDC_UNIT_STOP, bStop );
                    ShowButton( IDC_UNIT_STOP, bStop );
                    EnableButton( IDC_UNIT_RESUME, !bStop );
                    ShowButton( IDC_UNIT_RESUME, !bStop );
                }
            }
            // any other button
            else
            {
                EnableButton( abID[iOn], TRUE );
                ShowButton( abID[iOn], TRUE );
            }
        }

    StatUnit( m_pUnit );
}

void CWndArea::SetMouseState( )
{

    static int iNum = 0;
    if ( iNum != 0 )
    {
        AreaApplyCursor( m_hCurReg );
        return;
    }

    m_uMouseMode = lmb_nothing;

    // if the pan button (MMB) is down we are scrolling
    if ( m_bPanBtnDown )
    {
        AreaApplyCursor( m_hCurMove[m_iMoveCur] );
        return;
    }

    // for these cases its not location dependent
    switch ( m_iMode )
    {
    case rocket_ready:
    case rocket_pos:
    case build_ready:
    case build_loc:
        AreaApplyCursor( NULL );
        return;

    case road_begin:
        AreaApplyCursor( m_hCurRoadBgn[m_aa.m_iZoom] );
        return;
    case road_set:
        AreaApplyCursor( m_hCurRoadSet[m_aa.m_iZoom] );
        return;
    case veh_route:
        AreaApplyCursor( m_hCurRoute );
        return;

    case repair_bldg: {
        CPoint pt;
        ::GetCursorPos( &pt );
        ScreenToClient( &pt );
        CHexCoord hex = m_aa.WindowToHex( pt );
        hex.Wrap( );
        CBuilding* pBldg = theBuildingHex._GetBuilding( hex );
        if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) &&
             ( ( pBldg->GetDamagePer( ) < 100 ) || ( pBldg->IsConstructing( ) ) ) )
            AreaApplyCursor( m_hCurRepair );
        else
            AreaApplyCursor( m_hCurNoRepair );
        return;
    }

    case normal:
    case normal_select:
        break;

    default:
        AreaApplyCursor( m_hCurReg );
        return;
    }

    // get the mouse position
    CPoint pt;
    ::GetCursorPos( &pt );
    ScreenToClient( &pt );

    // if we are selecting - that has absolute pri
    if ( m_iMode == normal_select )
        if ( ( abs( pt.x - m_ptLMB.x ) >= theMap.HexWid( m_aa.m_iZoom ) / 2 ) ||
             ( abs( pt.y - m_ptLMB.y ) >= theMap.HexHt( m_aa.m_iZoom ) / 2 ) )
        {
            AreaApplyCursor( m_hCurReg );
            m_uMouseMode = lmb_select;
            return;
        }

    // see if flags force it: Ctrl+Shift = force attack (dispatched by RMB).
    // (The old Ctrl-alone force-goto is gone — RMB on an own unit is already a
    // goto, so Ctrl no longer changes what the command button does.)
    if ( m_lstUnits.GetCount( ) > 0 )
    {
        int iShift = GetKeyState( VK_SHIFT ) & ~1;
        int iCtrl  = GetKeyState( VK_CONTROL ) & ~1;
        if ( iShift & iCtrl )
        {
            if ( m_uFlags & attk )
            {
                AreaApplyCursor( m_hCurTarget[m_aa.m_iZoom] );
                m_uMouseMode = lmb_attack;
            }
            else
            {
                AreaApplyCursor( m_hCurReg );
                m_uMouseMode = lmb_nothing;
            }
            return;
        }
    }

    CHitInfo  hitinfo = m_aa.GetHit( pt );
    CHexCoord hexcoord( hitinfo._GetHexCoord( ) );
    hexcoord.Wrap( );
    CUnit*       pUnitOn = hitinfo.GetUnit( );
    CBridgeUnit* pBu     = hitinfo.GetBridge( );

    ASSERT_STRICT_VALID_OR_NULL( pUnitOn );

    // if not visible then it's not there
    if ( pUnitOn != NULL )
        if ( !pUnitOn->IsVisible( ) )
            pUnitOn = NULL;
    if ( pUnitOn != NULL )
        if ( pUnitOn->GetUnitType( ) == CUnit::vehicle )
            if ( !theMap.GetHex( hexcoord )->GetVisibility( ) )
                pUnitOn = NULL;

    // if nothing is selected its select or nothing
    if ( m_lstUnits.GetCount( ) == 0 )
    {
        if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->IsMe( ) ) )
        {
            AreaApplyCursor( m_hCurSelect[m_aa.m_iZoom] );
            m_uMouseMode = lmb_select;
        }
        else
        {
            AreaApplyCursor( m_hCurReg );
            m_uMouseMode = lmb_nothing;
        }
        return;
    }

    // step -2 - finish a bridge
    if ( pBu != NULL )
        if ( ( !( m_uFlags & non_crane ) ) && ( m_uFlags & crane ) && ( !pBu->GetParent( )->IsBuilt( ) ) &&
             ( pBu->IsExit( ) ) )
        {
            AreaApplyCursor( m_hCurRepair );
            m_uMouseMode = lmb_repair_bldg;
            return;
        }

    // if its mine we can select it
    if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->IsMe( ) ) )
    {
        // step -1 - if we are a crane and this building needs help => repair it
        if ( ( !( m_uFlags & non_crane ) ) && ( m_uFlags & crane ) )
            if ( ( pUnitOn->GetUnitType( ) == CUnit::building ) &&
                 ( ( pUnitOn->GetDamagePer( ) < 100 ) || ( ( (CBuilding*)pUnitOn )->IsConstructing( ) ) ) )
            {
                AreaApplyCursor( m_hCurRepair );
                m_uMouseMode = lmb_repair_bldg;
                return;
            }

        // step -1.5 - if we are a truck and over one of our buildings - AND NOT A repair center - it's a goto
        if ( ( !( m_uFlags & non_truck ) ) && ( m_uFlags & truck ) && ( pUnitOn->GetUnitType( ) == CUnit::building ) &&
             ( ( (CBuilding*)pUnitOn )->GetData( )->GetType( ) != CStructureData::repair ) )
        {
            AreaApplyCursor( m_hCurGoto[m_aa.m_iZoom] );
            m_uMouseMode = lmb_goto;
            return;
        }

        // step 0 - find out if we are on a carrier or repair facility
        BOOL bCarrier = FALSE, bLcCarrier = FALSE, bLandRepair = FALSE, bSeaRepair = FALSE;
        if ( ( pUnitOn->GetUnitType( ) == CUnit::building ) && ( !( (CBuilding*)pUnitOn )->IsConstructing( ) ) )
        {
            if ( ( (CBuilding*)pUnitOn )->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
                bLandRepair = TRUE;
            else if ( ( (CBuilding*)pUnitOn )->GetData( )->GetUnionType( ) == CStructureData::UTshipyard )
                bSeaRepair = TRUE;
        }
        else if ( ( pUnitOn->GetUnitType( ) == CUnit::vehicle ) &&
                  ( ( (CVehicle*)pUnitOn )->GetData( )->IsCarrier( ) ) &&
                  ( !( (CVehicle*)pUnitOn )->GetData( )->IsTransport( ) ) )
        {
            bCarrier = TRUE;
            if ( ( (CVehicle*)pUnitOn )->GetData( )->IsBoat( ) )
                bLcCarrier = TRUE;
        }

        // if repairable - that's it
        if ( ( ( ( m_uFlags & land_repairable ) != 0 ) && ( bLandRepair ) ) ||
             ( ( ( m_uFlags & sea_repairable ) != 0 ) && ( bSeaRepair ) ) )
        {
            AreaApplyCursor( m_hCurRepair );
            m_uMouseMode = lmb_repair_self;
            return;
        }

        // if it can be loaded on
        if ( ( bCarrier ) &&
             ( ( (CVehicle*)pUnitOn )->GetCargoSize( ) < ( (CVehicle*)pUnitOn )->GetEffPeopleCarry( ) ) )
            if ( ( m_uFlags & carryable ) || ( ( bLcCarrier ) && ( m_uFlags & lc_carryable ) ) )
            {
                AreaApplyCursor( m_hCurLoad[m_aa.m_iZoom] );
                m_uMouseMode = lmb_load;
                return;
            }

        // if it can be unloaded AND is selected (must be on land)
        if ( ( bCarrier ) && ( ( (CVehicle*)pUnitOn )->GetCargoCount( ) > 0 ) &&
             ( ( (CVehicle*)pUnitOn )->IsSelected( ) ) )
            if ( ( !theMap._GetHex( ( (CVehicle*)pUnitOn )->GetPtHead( ) )->IsWater( ) ) ||
                 ( !theMap._GetHex( ( (CVehicle*)pUnitOn )->GetPtTail( ) )->IsWater( ) ) )
            {
                AreaApplyCursor( m_hCurUnload[m_aa.m_iZoom] );
                m_uMouseMode = lmb_unload;
                return;
            }

        AreaApplyCursor( m_hCurSelect[m_aa.m_iZoom] );
        m_uMouseMode = lmb_select;
        return;
    }

    // if alliance & truck - it's a goto
    if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->GetTheirRelations( ) == RELATIONS_ALLIANCE ) )
        if ( ( !( m_uFlags & non_truck ) ) && ( m_uFlags & truck ) && ( pUnitOn->GetUnitType( ) == CUnit::building ) )
        {
            AreaApplyCursor( m_hCurGoto[m_aa.m_iZoom] );
            m_uMouseMode = lmb_goto;
            return;
        }

    // if nothing under or friendly its goto - unless we are buildings only
    if ( ( pUnitOn == NULL ) || ( pUnitOn->GetOwner( )->GetRelations( ) <= RELATIONS_PEACE ) )
    {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) )
            if ( ( pUnitOn == NULL ) || ( pUnitOn->GetUnitType( ) != CUnit::building ) ||
                 ( ( m_pUnit != NULL ) && ( ( (CVehicle*)m_pUnit )->CanEnterBldg( (CBuilding*)pUnitOn ) ) ) )
            {
                m_uMouseMode = lmb_goto;
                AreaApplyCursor( m_hCurGoto[m_aa.m_iZoom] );
                return;
            }

        m_uMouseMode = lmb_nothing;
        AreaApplyCursor( m_hCurReg );
        return;
    }

    // ok - its a target (and in range for a building)
    if ( m_uFlags & attk )
    {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::building ) )
        {
            AreaApplyCursor( m_hCurTarget[m_aa.m_iZoom] );
            m_uMouseMode = lmb_attack;
            return;
        }

        // building must be in range
        int iLOS = theMap.LineOfSight( m_pUnit, pUnitOn );
        if ( abs( iLOS ) <= m_pUnit->GetRange( ) )
        {
            AreaApplyCursor( m_hCurTarget[m_aa.m_iZoom] );
            m_uMouseMode = lmb_attack;
            return;
        }
    }

    m_uMouseMode = lmb_nothing;
    AreaApplyCursor( m_hCurReg );
}

BOOL CWndArea::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
    // accumulate wheel deltas; require two notches (2 * WHEEL_DELTA) to trigger a zoom
    s_areaWheelAccum += (int)zDelta;

    const int needed = 2 * WHEEL_DELTA;

    if ( s_areaWheelAccum >= needed )
    {
        // scroll in by 2 clicks => zoom in
        s_areaWheelAccum = 0;
        ZoomIn( );
        return TRUE;  // handled
    }
    else if ( s_areaWheelAccum <= -needed )
    {
        // scroll out by 2 clicks => zoom out
        s_areaWheelAccum = 0;
        ZoomOut( );
        return TRUE;  // handled
    }

    // not enough accumulation yet; let base class handle any default behavior
    return CWndAnim::OnMouseWheel( nFlags, zDelta, pt );
}

void CWndArea::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{

    // handle changes in CTRL & SHIFT which changes the cursor
    if ( ( nChar == VK_CONTROL ) || ( nChar == VK_SHIFT ) )
        SetMouseState( );

    CWndAnim::OnKeyUp( nChar, nRepCnt, nFlags );
}

void CWndArea::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{

    // handle changes in CTRL & SHIFT which changes the cursor
    if ( ( nChar == VK_CONTROL ) || ( nChar == VK_SHIFT ) )
        SetMouseState( );

    // if not a number we don't care about it
    if ( ( nChar < '0' ) || ( '9' < nChar ) )
    {
        CWndAnim::OnKeyDown( nChar, nRepCnt, nFlags );
        return;
    }

    CArray<DWORD, DWORD>* paSave = &( m_aSaveSel[nChar - '0'] );

    // see if it's a save
    if ( GetKeyState( VK_CONTROL ) & ~1 )
    {
        if ( !( GetKeyState( VK_SHIFT ) & ~1 ) )
            paSave->RemoveAll( );
        else
            TRAP( );
        int iOn = paSave->GetSize( );
        paSave->SetSize( m_lstUnits.GetCount( ) + iOn );
        POSITION pos = m_lstUnits.GetHeadPosition( );
        for ( ; pos != NULL; iOn++ )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            paSave->SetAtGrow( iOn, pUnit->GetID( ) );
        }
        return;
    }

    // it's a restore
    // do we kill the old?
    if ( !( GetKeyState( VK_SHIFT ) & ~1 ) )
        m_lstUnits.RemoveAllUnits( TRUE );

    theApp.m_wndBldgs.m_ListBox.SetRedraw( FALSE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( FALSE );

    for ( int iOn = 0; iOn < paSave->GetSize( ); iOn++ )
    {
        CUnit* pUnit = ::GetUnit( paSave->GetAt( iOn ) );
        if ( pUnit != NULL )
            m_lstUnits.AddUnit( pUnit, TRUE );
    }

    theApp.m_wndBldgs.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL, FALSE );
    theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL, FALSE );

    // update the screen
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;

    // set button states
    SetButtonState( );

    // repaint it
    InvalidateStatus( );
    InvalidateSound( );
}

void CWndArea::ReCenter( )
{

    TRAP( );
}

CWnd* CWndArea::GetExpand( )
{

    TRAP( );
    return ( NULL );
}

// called when a unit is dtor'ed
void CWndArea::UnitDying( CUnit* pUnit )
{

    BOOL bRedraw = FALSE;

    // remove from selected units list
    POSITION pos = m_lstUnits.Find( pUnit );
    if ( pos != NULL )
    {
        m_lstUnits.RemoveAt( pos );
        bRedraw = TRUE;
    }

    // remove as selected unit
    if ( m_pUnit == pUnit )
    {
        if ( m_iMode == build_loc )
            SelectOff( );
        m_pUnit = NULL;
        m_iMode = normal;
        bRedraw = TRUE;
    }

    // remove it's info window
    if ( m_pSdlInfo && m_pSdlInfo->IsVisible() && m_pSdlInfo->GetUnit() == pUnit )
    {
        m_pSdlInfo->Hide();
        bRedraw = TRUE;
    }
    if ( ( m_pWndInfo != NULL ) && ( m_pWndInfo->m_hWnd != NULL ) && ( m_pWndInfo->GetUnit( ) == pUnit ) )
    {
        m_pWndInfo->DestroyWindow( );
        bRedraw = TRUE;
    }

    // remove status windows
    if ( GetStaticUnit( ) == pUnit )
    {
        StatUnit( NULL );
        bRedraw = TRUE;
    }

    // redraw if changed
    if ( bRedraw )
    {
        InvalidateStatus( );
        SetButtonState( );
    }
}

void CWndArea::UpdateSound( )
{

    m_bNewPos = FALSE;
    if ( !theMusicPlayer.SoundsPlaying( ) )
        return;

    theMusicPlayer.ClrBackgroundSounds( );

    CRect rect;
    GetClientRect( &rect );

    int iOldDir = xiDir;
    xiDir       = m_aa.m_iDir;

    // terrain sounds
    if ( m_aa.m_iZoom <= 1 )
    {
        CSize     size  = m_aa.m_dibwnd.GetWinSize( );
        CHexCoord hexTL = m_aa._WindowToHex( CPoint( 0, 0 ) );
        CHexCoord hexBL = m_aa._WindowToHex( CPoint( 0, size.cy - 1 ) );

        CViewHexCoord viewhexTL( hexTL );
        CViewHexCoord viewhexBL( hexBL );

        int iW = ( size.cx + MAX_HEX_HT - 1 ) >> ( HEX_HT_PWR + 1 - m_aa.m_iZoom );
        int iH = viewhexBL.y - viewhexTL.y + 1;

        int aMul[4];
        theMap.DirMult( m_aa.m_iDir, aMul );

        int       iTotal = 0, iTrees = 0, iSwamp = 0, iRiver = 0, iOcean = 0;
        CHexCoord hexLeft( hexTL );
        hexLeft.Wrap( );

        for ( int x = 0; x < iW; x++ )
        {
            if ( x & 1 )
                hexLeft.Y( hexLeft.Y( ) + aMul[3] );
            else
                hexLeft.X( hexLeft.X( ) + aMul[2] );
            CHexCoord hexOn( hexLeft );
            hexOn.Wrap( );

            for ( int y = 0; y < iH; y++ )
            {
                iTotal++;
                switch ( theMap._GetHex( hexOn )->GetType( ) )
                {
                case CHex::forest:
                    iTrees++;
                    break;
                case CHex::swamp:
                    iSwamp++;
                    break;
                case CHex::river:
                    iRiver++;
                    break;
                case CHex::ocean:
                    iOcean++;
                    break;
                }

                hexOn.X( ) += aMul[0];
                hexOn.Y( ) += aMul[1];
                hexOn.Wrap( );
            }
        }

        // ok, play them if loud enough
        // BUGBUG - do we need to pan?
        if ( iTotal > 0 ) { // only run this calculation if iTotal isn't 0
            int iTreeVol = (iTrees * 100) / iTotal;
            if (iTreeVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::trees, SFXPRIORITY::terrain_pri, 64, iTreeVol);
            int iSwampVol = (iSwamp * 100) / iTotal;
            if (iSwampVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::swamp, SFXPRIORITY::terrain_pri, 64, iSwampVol);
            int iRiverVol = (iRiver * 100) / iTotal;
            if (iRiverVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::river, SFXPRIORITY::terrain_pri, 64, iRiverVol);
            int iOceanVol = (iOcean * 100) / iTotal;
            if (iOceanVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::ocean, SFXPRIORITY::terrain_pri, 64, iOceanVol);
        }
    }

    // walk the buildings for construction, burning, selected
    POSITION pos;
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );

        // must be in the window and visible
        //   OR ours & damaged
        if ( ( pBldg->GetOwner( )->IsMe( ) ) || ( theMap._GetHex( pBldg->GetHex( ) )->GetVisible( ) ) )
        {
            // use iVol to see if it's in the window for construction
            int iPan, iVol;
            GetPanAndVol( pBldg, iPan, iVol );

            // on fire - visible or us
            if ( ( iVol >= 100 ) || ( pBldg->GetOwner( )->IsMe( ) ) )
            {
                if ( !pBldg->GetOwner( )->IsMe( ) )
                    iVol = 60;

                // Own buildings pass the gate above at ANY position, but an
                // off-screen one yields a sub-audible (even negative) iVol from
                // GetPanAndVol. QueueBackgroundSound requires iVol >= 10 and
                // (debug) TRAPs an inaudible call. Skip it here — matching the
                // original release build, where the queue simply returned and
                // the off-screen sound was dropped.
                if ( iVol >= 10 )
                {
                    if ( pBldg->GetDamage( ) == 3 )
                        theMusicPlayer.QueueBackgroundSound( SOUNDS::damage3, SFXPRIORITY::damage_pri, iPan, iVol );
                    else if ( pBldg->GetDamage( ) == 4 )
                    {
                        TRAP( );
                        theMusicPlayer.QueueBackgroundSound( SOUNDS::damage4, SFXPRIORITY::damage_pri, iPan, iVol );
                    }
                }
            }

            // construction - must be visible, quieter if not ours
            if ( ( iVol >= 100 ) && ( pBldg->IsConstructing( ) ) )
            {
                if ( !pBldg->GetOwner( )->IsMe( ) )
                    iVol = 60;

                if ( !pBldg->IsFoundationComplete( ) )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::const1, SFXPRIORITY::const_pri, iPan, iVol );
                else if ( !pBldg->IsSkltnComplete( ) )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::const2, SFXPRIORITY::const_pri, iPan, iVol );
                else
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::const3, SFXPRIORITY::const_pri, iPan, iVol );
            }
        }
    }

    // walk the vehicles for burning, selected
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );

        // must be in the window and visible
        if ( ( pVeh->GetOwner( )->IsMe( ) ) || ( theMap._GetHex( pVeh->GetHexHead( ) )->GetVisible( ) ) )
        {
            int iPan, iVol;
            GetPanAndVol( pVeh, iPan, iVol );

            if ( iVol >= 100 )
            {
                if ( !pVeh->GetOwner( )->IsMe( ) )
                    iVol = 60;

                // on fire
                if ( pVeh->GetDamage( ) == 3 )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::damage3, SFXPRIORITY::damage_pri, iPan, iVol );
                else if ( pVeh->GetDamage( ) == 4 )
                {
                    TRAP( );
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::damage4, SFXPRIORITY::damage_pri, iPan, iVol );
                }

                // if it's selected
                if ( pVeh->IsSelected( ) )
                {
                    if ( ( pVeh->GetRouteMode( ) == CVehicle::stop ) ||
                         ( pVeh->GetRouteMode( ) == CVehicle::blocked ) ||
                         ( pVeh->GetRouteMode( ) == CVehicle::traffic ) )
                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundIdle( ),
                                                             SFXPRIORITY::selected_pri, iPan, iVol );
                    else
                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundRun( ),
                                                             SFXPRIORITY::selected_pri, iPan, iVol );
                }
                else

                    // if it's moving - ours are louder
                    if ( pVeh->GetRouteMode( ) == CVehicle::moving )
                {
                    if ( pVeh->GetOwner( )->IsMe( ) )
                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundRun( ),
                                                             SFXPRIORITY::selected_pri, iPan, 60 );
                    else

                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundRun( ),
                                                             SFXPRIORITY::selected_pri, iPan, 40 );
                }
            }
        }
    }

    xiDir = iOldDir;

    theMusicPlayer.UpdateBackgroundSounds( );
}


/////////////////////////////////////////////////////////////////////////////
// CWndInfo

CWndInfo::CWndInfo( )
{

    m_pUnit = NULL;
    m_pdib  = NULL;
}

CWndInfo::~CWndInfo( )
{

    delete m_pdib;
}


BEGIN_MESSAGE_MAP( CWndInfo, CWndBase )
//{{AFX_MSG_MAP(CWndInfo)
ON_WM_PAINT( )
ON_WM_SIZE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CWndInfo message handlers

const DWORD dwStyle = WS_CHILD;

CWndInfo* CWndInfo::Create( CPoint& pt, CUnit* pUnit, CWndArea* pPar )
{
#ifdef LOGGINGON
    OutputDebugStringA( "CWndInfo::Create" );
#endif

    ASSERT_STRICT_VALID( pUnit );

    m_pUnit = pUnit;

    // figure the size
    CRect rect;
    rect.left = rect.top = 0;
    rect.right           = __max( (int)m_pUnit->GetData( )->GetDesc( ).length( ) + 2, 20 ) * theApp.TextWid( );
    rect.bottom          = FigureHt( );

    // stop clipping to the right/below (above/left impossible)
    CRect rectWin( pt, CSize( rect.Width( ), rect.Height( ) ) );
    pPar->GetClientRect( rect );
    if ( rectWin.right > rect.right )
        rectWin.OffsetRect( -( rectWin.right - rect.right ), 0 );
    if ( rectWin.bottom > rect.bottom )
        rectWin.OffsetRect( 0, -( rectWin.bottom - rect.bottom ) );

    if ( m_pdib != NULL )
        m_pdib->Resize( rectWin.Width( ), rectWin.Height( ) );
    else
        m_pdib = new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY,
                           ptrthebltformat->GetMemDirection( ), rectWin.Width( ), rectWin.Height( ) );

    return ( (CWndInfo*)CWndBase::Create( NULL, pUnit->GetData( )->GetDesc( ).c_str(), dwStyle, rectWin, pPar, 100, NULL ) );
}

static void _DrawText( CDC* pDc, CRect& rect, char const* sText, BOOL bRed = FALSE );

static void _DrawText( CDC* pDc, CRect& rect, char const* sText, BOOL bRed )
{

    rect.OffsetRect( 1, 1 );
    pDc->SetTextColor( RGB( 128, 128, 128 ) );
    pDc->DrawText( sText, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER );

    rect.OffsetRect( -1, -1 );
    if ( bRed )
        pDc->SetTextColor( RGB( 255, 50, 27 ) );
    else
        pDc->SetTextColor( RGB( 0, 0, 0 ) );
    pDc->DrawText( sText, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER );
}

void CWndInfo::OnPaint( )
{

    ASSERT_STRICT_VALID( this );
    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    // set it up
    CRect rect;
    CWndBase::GetClientRect( &rect );

    // draw the background
    PaintBorder( m_pdib, rect, TRUE );

    // set up the DC for text
    CDC* pDc = CDC::FromHandle( m_pdib->GetDC( ) );
    if ( pDc == NULL )
    {
        thePal.EndPaint( dc.m_hDC );
        return;
    }
    thePal.Paint( pDc->m_hDC );
    pDc->SetBkMode( TRANSPARENT );
    HGDIOBJ pOldFont = ::SelectObject( pDc->m_hDC, theApp.TextFont( ) );  // Phase 4c prep

    // draw the name
    CDIB* pdibHorz = theBitmaps.GetByIndex( DIB_BORDER_HORZ );
    CDIB* pdibVert = theBitmaps.GetByIndex( DIB_BORDER_VERT );
    rect.top       = pdibHorz->GetHeight( ) + theApp.FlatDimen( );
    rect.left      = pdibVert->GetWidth( ) + theApp.FlatDimen( );
    rect.right -= theApp.FlatDimen( );
    rect.bottom = rect.top + theApp.TextHt( );
    _DrawText( pDc, rect, m_pUnit->GetData( )->GetDesc( ).c_str() );

    // draw the state
    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
    {
        CVehicle* pVeh = (CVehicle*)m_pUnit;
        if ( pVeh->GetData( )->IsTransport( ) )
        {
            std::string sText;
            if ( !pVeh->IsHpControl( ) )
                sText = CTransportData::m_sAuto;
            else if ( pVeh->GetEvent( ) == CVehicle::route )
                sText = CTransportData::m_sRoute;

            CBuilding* pBldg = theBuildingHex.GetBuilding( pVeh->GetPtHead( ) );
            if ( ( pBldg == NULL ) || ( pVeh->GetHexOwnership( ) ) )
                pBldg = theBuildingHex.GetBuilding( pVeh->GetHexDest( ) );
            if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) )
                sText += std::string( "[" ) + pBldg->GetData( )->GetDesc( ).c_str() + "]";
            else if ( pVeh->GetData( )->IsTransport( ) )
            {
                if ( pVeh->GetRouteMode( ) == CVehicle::stop )
                    sText += CTransportData::m_sIdle;
                else
                    sText += CTransportData::m_sTravel;
            }

            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom = rect.top + theApp.TextHt( );
            _DrawText( pDc, rect, sText.c_str( ) );
        }
    }

    // draw the damage
    rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
    rect.bottom = rect.top + theApp.TextHt( );
    std::string sStatus = strPrintf( EnLoadStdString( IDS_INFO_DAMAGE ).c_str(),
                                     IntToStr( __min( 99, 100 - m_pUnit->GetDamagePer( ) ) ).c_str( ) );
    _DrawText( pDc, rect, sStatus.c_str( ), m_pUnit->GetDamagePer( ) < 50 );

    // if building draw it's status
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
        switch ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UThousing:
        case CStructureData::UTpower:
        case CStructureData::UTresearch:
        case CStructureData::UTrepair:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTshipyard:
            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom = rect.top + theApp.TextHt( );
            ( (CBuilding*)m_pUnit )->ShowStatusText( sStatus );
            _DrawText( pDc, rect, sStatus.c_str( ), m_pUnit->GetDamagePer( ) < 50 );
            break;
        }

    // draw the materials
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
    {
        int iNeed = 0;
        if ( ( iOn < CMaterialTypes::GetNumBuildTypes( ) ) && ( m_pUnit->GetUnitType( ) == CUnit::building ) )
            iNeed = ( (CBuilding*)m_pUnit )->GetBldgResReq( iOn, FALSE );
        if ( ( m_pUnit->GetStore( iOn ) != 0 ) || ( iNeed > 0 ) )
        {
            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom   = rect.top + theApp.TextHt( );
            std::string sText = CMaterialTypes::GetDesc( iOn ) + ": " +
                                IntToStr( m_pUnit->GetStore( iOn ), 10, true );
            _DrawText( pDc, rect, sText.c_str( ) );

            if ( iNeed > 0 )
            {
                CRect rectNum( rect );
                pDc->DrawText( sText.c_str( ), -1, &rectNum, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER );
                rectNum.left  = rectNum.right + theApp.FlatDimen( );
                rectNum.right = rect.right;
                sText         = "(" + IntToStr( iNeed, 10, true ) + ")";
                _DrawText( pDc, rectNum, sText.c_str( ), TRUE );
            }
        }
    }

    // if a carrier list units onboard
    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
    {
        POSITION pos = ( (CVehicle*)m_pUnit )->GetCargoHeadPosition( );
        while ( pos != NULL )
        {
            CVehicle* pVeh = ( (CVehicle*)m_pUnit )->GetCargoNext( pos );
            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom = rect.top + theApp.TextHt( );
            _DrawText( pDc, rect, pVeh->GetData( )->GetDesc( ).c_str() );
        }
    }

    // vehicles inside building
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
    {
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( !pVeh->GetHexOwnership( ) ) &&
                 ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == m_pUnit ) )
            {
                rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
                rect.bottom = rect.top + theApp.TextHt( );
                _DrawText( pDc, rect, pVeh->GetData( )->GetDesc( ).c_str() );
            }
        }
    }

    // paint it
    m_pdib->BitBlt( dc.m_hDC, m_pdib->GetRect( ), CPoint( 0, 0 ) );

    ::SelectObject( pDc->m_hDC, pOldFont );  // Phase 4c prep
    thePal.EndPaint( pDc->m_hDC );
    if ( m_pdib->IsBitmapSelected( ) )
        m_pdib->ReleaseDC( );

    thePal.EndPaint( dc.m_hDC );
    // Do not call CWndBase::OnPaint() for painting messages
}

void CWndInfo::OnSize( UINT nType, int cx, int cy )
{

    CWndBase::OnSize( nType, cx, cy );

    m_pdib->Resize( cx, cy );
}

int CWndInfo::FigureHt( )
{

    CRect rect;
    rect.left = rect.top = 0;
    rect.right           = __max( (int)m_pUnit->GetData( )->GetDesc( ).length( ) + 2, 20 ) * theApp.TextWid( );
    rect.bottom          = 2;

    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
        if ( ( (CVehicle*)m_pUnit )->GetData( )->IsTransport( ) )
            rect.bottom++;

    // if building draw it's status
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
        switch ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UThousing:
        case CStructureData::UTpower:
        case CStructureData::UTresearch:
        case CStructureData::UTrepair:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTshipyard:
            rect.bottom++;
            break;
        }

    if ( ( m_pUnit->GetUnitType( ) == CUnit::building ) ||
         ( ( (CVehicle*)m_pUnit )->GetData( )->GetMaxMaterials( ) != 0 ) )
        for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
        {
            int iNeed = 0;
            if ( ( iOn < CMaterialTypes::GetNumBuildTypes( ) ) && ( m_pUnit->GetUnitType( ) == CUnit::building ) )
                iNeed = ( (CBuilding*)m_pUnit )->GetBldgResReq( iOn, FALSE );
            if ( ( m_pUnit->GetStore( iOn ) != 0 ) || ( iNeed > 0 ) )
                rect.bottom++;
        }

    // vehicles we're carrying
    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
        rect.bottom += ( (CVehicle*)m_pUnit )->GetCargoCount( );

    // vehicles inside us
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
    {
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( !pVeh->GetHexOwnership( ) ) &&
                 ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == m_pUnit ) )
                rect.bottom++;
        }
    }

    rect.bottom = 2 * theApp.FlatDimen( ) + rect.bottom * ( theApp.TextHt( ) + theApp.FlatDimen( ) );

    // border
    rect.bottom += 2 * __min( 6, theBitmaps.GetByIndex( DIB_BORDER_HORZ )->GetHeight( ) );

    AdjustWindowRect( &rect, dwStyle, FALSE );

    return ( rect.Height( ) );
}

void CWndInfo::Refigure( )
{

    CRect rect;
    GetWindowRect( &rect );

    int iHt = FigureHt( );
    if ( iHt == rect.Height( ) )
    {
        InvalidateRect( NULL );
        return;
    }

    SetWindowPos( NULL, 0, 0, rect.Width( ), iHt, SWP_NOMOVE | SWP_NOZORDER );

    InvalidateRect( NULL );
}

BOOL CWndArea::OnCommand( WPARAM wParam, LPARAM lParam )
{

    // With CWndStub the BEGIN_MESSAGE_MAP no-ops, so ON_BN_CLICKED / ON_COMMAND
    // entries are dead. Dispatch them manually here.
    switch ( LOWORD( wParam ) )
    {
    case IDA_SAVE:
        GetParent( )->SendMessage( WM_COMMAND, wParam, lParam );
        return ( TRUE );

    // Area toolbar buttons (SDL2AreaBar) — match the ON_BN_CLICKED table above
    case IDC_AREA_COMBAT:       LastCombat();         return TRUE;
    case IDC_AREA_ZOOM_IN:      ZoomIn();             return TRUE;
    case IDC_AREA_ZOOM_OUT:     ZoomOut();            return TRUE;
    case IDC_AREA_CLOCK:        TurnClock();          return TRUE;
    case IDC_AREA_COUNTER:      TurnCounter();        return TRUE;
    case IDC_AREA_RES:          ResClicked();         return TRUE;
    case IDC_UNIT_STOP:         StopUnit();           return TRUE;
    case IDC_UNIT_RESUME:       ResumeUnit();         return TRUE;
    case IDC_UNIT_ROAD:         RoadUnit();           return TRUE;
    case IDC_UNIT_CANCEL_ROAD:  CancelRoadUnit();     return TRUE;
    case IDC_UNIT_BUILD:        BuildUnit();          return TRUE;
    case IDC_UNIT_CANCEL_BUILD: CancelBuildUnit();    return TRUE;
    case IDC_UNIT_ROUTE:        RouteUnit();          return TRUE;
    case IDC_UNIT_UNLOAD:       UnloadUnit();         return TRUE;
    case IDC_UNIT_RETREAT:      RetreatUnit();        return TRUE;
    case IDC_UNIT_REPAIR:       RepairUnit();         return TRUE;
    case IDC_UNIT_CANCEL_REPAIR: CancelRepairUnit();  return TRUE;

    // Accelerator commands
    case IDA_CENTER:            CenterUnit();         return TRUE;
    case IDA_DESTROY:           DestroyUnit();        return TRUE;
    case IDA_STOP_DESTROY:      StopDestroyUnit();    return TRUE;
    case IDA_CUR_UP:            CurUp();              return TRUE;
    case IDA_CUR_RIGHT:         CurRight();           return TRUE;
    case IDA_CUR_DOWN:          CurDown();            return TRUE;
    case IDA_CUR_LEFT:          CurLeft();            return TRUE;
    case IDA_OPPO:              OppoUnit();           return TRUE;
    case IDA_CLOSE_WIN:         OnCloseWin();         return TRUE;
    case IDA_DESELECT:          OnDeselect();         return TRUE;
    case IDA_BUILD:             BuildUnit();          return TRUE;
    case IDA_RETREAT:           RetreatUnit();        return TRUE;
    case IDA_ROUTE:             RouteUnit();          return TRUE;
    case IDA_UNLOAD:            UnloadUnit();         return TRUE;
    }

    return ( CWndAnim::OnCommand( wParam, lParam ) );
}

void CWndArea::OnDeselect( )
{

    if ( ( m_iMode == rocket_ready ) || ( m_iMode == rocket_pos ) || ( m_iMode == rocket_wait ) )
        return;

    BldgCurOff( );

    m_lstUnits.RemoveAllUnits( TRUE );
    m_pUnit      = NULL;
    m_uFlags     = 0;
    m_uMouseMode = lmb_nothing;

    SetButtonState( );
    InvalidateStatus( );
    InvalidateSound( );
}

void CWndArea::OnCloseWin( )
{

    ShowWindow( SW_HIDE );
}

void CWndArea::BldgCurOff( )
{

    theMap.ClrBldgCur( );
    m_iBuild = -1;
    m_iMode  = normal;
    AreaApplyCursor( m_hCurReg );
}

// add this unit to the list of selected units
void CWndArea::AddSelectUnit( CUnit* pUnit )
{

    m_lstUnits.AddUnit( pUnit, TRUE );
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;
    MaterialChange( pUnit );
}

// add this unit to the list of selected units
void CWndArea::SubSelectUnit( CUnit* pUnit )
{

    m_lstUnits.RemoveUnit( pUnit );
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;
    MaterialChange( pUnit );
}

// make this the only selected unit
void CWndArea::OnlySelectUnit( CUnit* pUnit )
{

    m_lstUnits.RemoveAllUnits( TRUE );
    m_lstUnits.AddUnit( pUnit, TRUE );
    m_pUnit = pUnit;
    MaterialChange( pUnit );
}

HBRUSH CWndArea::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
    HBRUSH hbr = CWndAnim::OnCtlColor( pDC, pWnd, nCtlColor );

    // TODO: Change any attributes of the DC here

    // TODO: Return a different brush if the default is not desired
    return hbr;
}

void CWndArea::ClrRoadIcons( )
{
    // No active road drag-preview → nothing to restore, and DO NOT touch the GPU terrain-edit
    // gen. ClrRoadIcons is called at the top of OnLButtonUp on EVERY map click, so the old
    // unconditional ++g_enTerrainEditGen made every click bump the gen without recording a hex
    // (gendelta ran one ahead of the recorded list) -> reb.editmiss -> a full ~1.6s terrain
    // re-mesh on every click (the zoomed-out stutter when selecting units).
    if ( m_iNumRoadHex <= 0 )
        return;

    extern void g_enEditHex( int, int );

    // reset sprites — and RECORD each restored hex so the GPU terrain cache PATCHES just those
    // few road-path hexes (cheap) instead of forcing a full rebuild. g_enEditHex bumps the gen
    // AND records the hex, keeping gendelta == list size so the edit-patch path applies.
    CHexCoord* pHexOn     = m_phexRoadPath;
    CSprite**  ppSpriteOn = m_ppUnderSprite;
    while ( m_iNumRoadHex-- )
    {
        pHexOn->SetInvalidated( );
        theMap._GetHex( *pHexOn )->m_psprite = *ppSpriteOn++;
        g_enEditHex( pHexOn->X( ), pHexOn->Y( ) );
        ++pHexOn;
    }

    // free it up
    delete[] m_phexRoadPath;
    delete[] m_ppUnderSprite;
    m_phexRoadPath  = NULL;
    m_ppUnderSprite = NULL;
    m_iNumRoadHex   = 0;
}

void CWndArea::SetRoadIcons( CHexCoord hexEnd )
{

    ClrRoadIcons( );

    // no path or starting on water
    if ( ( m_hexRoadStart == hexEnd ) || ( !theMap._GetHex( m_hexRoadStart )->CanRoad( ) ) )
        return;

    // alloc the space
    delete[] m_phexRoadPath;
    delete[] m_ppUnderSprite;
    m_iNumRoadHex   = 0;
    int x           = abs( CHexCoord::Diff( hexEnd.X( ) - m_hexRoadStart.X( ) ) );
    int y           = abs( CHexCoord::Diff( hexEnd.Y( ) - m_hexRoadStart.Y( ) ) );
    int iSize       = __max( x, y ) + 3 + MAX_SPAN_ULT;
    m_phexRoadPath  = new CHexCoord[iSize];
    m_ppUnderSprite = new CSprite*[iSize];
    iSize--;

    CSprite*   pSprRoad = theTerrain.GetSprite( CHex::road, CHex::r_path );
    CHexCoord  _hexOn( m_hexRoadStart );
    CHexCoord* pHexOn     = m_phexRoadPath;
    CSprite**  ppSpriteOn = m_ppUnderSprite;
    BOOL       bOnWater   = FALSE;
    int        iSpan;

    while ( iSize-- > 0 )
    {
        CHex* pHex = theMap._GetHex( _hexOn );

        // hit a bridge - stop
        if ( pHex->GetUnits( ) & CHex::bridge )
            return;

        // check for water
        if ( ( !bOnWater ) && ( !pHex->CanRoad( ) ) )
        {
            // if can't bridge - done
            if ( !theGame.GetMe( )->CanBridge( ) )
                return;

            // ok - over the water
            x = CHexCoord::Diff( hexEnd.X( ) - _hexOn.X( ) );
            y = CHexCoord::Diff( hexEnd.Y( ) - _hexOn.Y( ) );
            if ( abs( x ) >= abs( y ) )
            {
                x = __minmax( -1, 1, x );
                y = 0;
            }
            else
            {
                x = 0;
                y = __minmax( -1, 1, y );
            }
            bOnWater = TRUE;
            iSpan    = 0;
        }

        if ( pHex->m_psprite != pSprRoad )
        {
            *pHexOn++     = _hexOn;
            *ppSpriteOn++ = pHex->m_psprite;
            m_iNumRoadHex++;
            pHex->m_psprite = pSprRoad;
            _hexOn.SetInvalidated( );
            // Record the previewed hex so the GPU terrain cache PATCHES this road tile in
            // (cheap) rather than the old full re-mesh; paired with ClrRoadIcons' restore.
            extern void g_enEditHex( int, int );
            g_enEditHex( _hexOn.X( ), _hexOn.Y( ) );
        }

        // check out span
        if ( bOnWater )
        {
            if ( pHex->IsWater( ) )
            {
                iSpan++;
                if ( iSpan > theGame.GetMe( )->GetMaxSpan( ) )
                    return;
            }
            else
            {
                iSpan = 0;
                if ( pHex->CanRoad( ) )
                    bOnWater = FALSE;
            }
        }

        if ( _hexOn == hexEnd )
            break;

        // move closer on the longest one (so we go diaganol)
        if ( !bOnWater )
        {
            x = CHexCoord::Diff( hexEnd.X( ) - _hexOn.X( ) );
            y = CHexCoord::Diff( hexEnd.Y( ) - _hexOn.Y( ) );
        }

        // to the next hex
        if ( abs( x ) >= abs( y ) )
        {
            if ( x > 0 )
                _hexOn.Xinc( );
            else
                _hexOn.Xdec( );
        }
        else
        {
            if ( y > 0 )
                _hexOn.Yinc( );
            else
                _hexOn.Ydec( );
        }
    }
}

void CWndArea::GiveSelectedUnits( CPlayer* pPlr )
{

    // count for message
    int      iBldgs = 0, iVehs = 0;
    POSITION pos = m_lstUnits.GetHeadPosition( );
    while ( pos != NULL )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );

        if ( pUnit->GetUnitType( ) == CUnit::building )
        {
            CBuilding* pBldg = (CBuilding*)pUnit;
            if ( ( !pBldg->IsConstructing( ) ) && ( pBldg->GetData( )->GetType( ) != CStructureData::rocket ) )
            {
                iBldgs++;
                // add any vehicles inside
                POSITION pos = theVehicleMap.GetStartPosition( );
                while ( pos != NULL )
                {
                    DWORD     dwID;
                    CVehicle* pVeh;
                    theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                    ASSERT_STRICT_VALID( pVeh );
                    if ( ( !pVeh->GetHexOwnership( ) ) && ( pVeh->GetOwner( ) == pPlr ) )
                    {
                        int xDif = CHexCoord::Diff( pVeh->GetHexHead( ).X( ) - pBldg->GetHex( ).X( ) );
                        if ( ( 0 <= xDif ) && ( xDif < m_cx ) )
                        {
                            int yDif = CHexCoord::Diff( pVeh->GetHexHead( ).Y( ) - pBldg->GetHex( ).Y( ) );
                            if ( ( 0 <= yDif ) && ( yDif < m_cy ) )
                                iVehs++;
                        }
                    }
                }
            }
        }

        else
            iVehs += 1 + ( (CVehicle*)pUnit )->GetCargoCount( );
    }

    // create prompt
    std::string sNumB = IntToStr( iBldgs, 10, true );
    std::string sNumV = IntToStr( iVehs, 10, true );
    std::string sSure = strPrintf( EnLoadStdString( IDS_GIVE_UNITS ).c_str(),
                                   sNumB.c_str(), sNumV.c_str(), pPlr->GetName() );
    if ( EnMessageBox( sSure.c_str(), MB_YESNO | MB_ICONQUESTION ) != IDYES )
    {
        TRAP( );
        return;
    }

    // ok hand them over
    pos = m_lstUnits.GetHeadPosition( );
    while ( pos != NULL )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        if ( pUnit->GetDamagePer( ) <= 50 )
            continue;

        if ( pUnit->GetUnitType( ) == CUnit::building )
        {
            CBuilding* pBldg = (CBuilding*)pUnit;
            if ( ( !pBldg->IsConstructing( ) ) && ( pBldg->GetData( )->GetType( ) != CStructureData::rocket ) )
            {
                CMsgGiveUnit msg( pUnit, pPlr );
                theGame.PostToAll( &msg, sizeof( msg ) );
                pUnit->_SetOwner( pPlr );

                // add any vehicles inside
                POSITION pos = theVehicleMap.GetStartPosition( );
                while ( pos != NULL )
                {
                    DWORD     dwID;
                    CVehicle* pVeh;
                    theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                    ASSERT_STRICT_VALID( pVeh );
                    if ( ( !pVeh->GetHexOwnership( ) ) && ( pVeh->GetOwner( ) == pPlr ) )
                    {
                        int xDif = CHexCoord::Diff( pVeh->GetHexHead( ).X( ) - pBldg->GetHex( ).X( ) );
                        if ( ( 0 <= xDif ) && ( xDif < m_cx ) )
                        {
                            int yDif = CHexCoord::Diff( pVeh->GetHexHead( ).Y( ) - pBldg->GetHex( ).Y( ) );
                            if ( ( 0 <= yDif ) && ( yDif < m_cy ) )
                            {
                                TRAP( );
                                CMsgGiveUnit msg( pVeh, pPlr );
                                theGame.PostToAll( &msg, sizeof( msg ) );
                                pVeh->_SetOwner( pPlr );
                            }
                        }
                    }
                }
            }
        }

        else
        {
            CMsgGiveUnit msg( pUnit, pPlr );
            theGame.PostToAll( &msg, sizeof( msg ) );
            pUnit->_SetOwner( pPlr );

            CVehicle* pVeh = (CVehicle*)pUnit;
            POSITION  pos  = pVeh->GetCargoHeadPosition( );
            while ( pos != NULL )
            {
                TRAP( );
                CVehicle*    pVehOn = pVeh->GetCargoNext( pos );
                CMsgGiveUnit msg( pVehOn, pPlr );
                theGame.PostToAll( &msg, sizeof( msg ) );
                pVehOn->_SetOwner( pPlr );
            }
        }
    }

    // we have nothing selected
    SelectNone( );
}

int CWndArea::NumGiveable( ) const
{

    int      iCount = 0;
    POSITION pos    = m_lstUnits.GetHeadPosition( );
    while ( pos != NULL )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        if ( pUnit->GetDamagePer( ) > 50 )
        {
            if ( pUnit->GetUnitType( ) == CUnit::building )
            {
                CBuilding* pBldg = (CBuilding*)pUnit;
                if ( ( !pBldg->IsConstructing( ) ) && ( pBldg->GetData( )->GetType( ) != CStructureData::rocket ) )
                    iCount++;
            }
            else
                iCount += 1 + ( (CVehicle*)pUnit )->GetCargoCount( );
        }
    }

    return ( iCount );
}

//---------------------------------------------------------------------------
// HarnessHexToWindow — project a hex tile to its CENTER pixel in the area
// window via the LIVE view transform. MapToWindowHex returns the hex's 4 corner
// points in window px (its BOOL return is a front/back-facing cull we ignore);
// the centroid is the unit's ground/select point — i.e. exactly the pixel you
// click to select the unit. This replaces the older
// WrapWorldToWindow(WorldToCenterWorld(GetWorldPixels())) path, which was offset
// from the rendered sprite by ~the cluster spacing and made click-targeting miss.
//---------------------------------------------------------------------------
static CPoint HarnessHexToWindow( CAnimAtr& aa, const CHexCoord& hex )
{
    CPoint p[4];
    aa.MapToWindowHex( hex, p );
    return CPoint( ( p[0].x + p[1].x + p[2].x + p[3].x ) / 4,
                   ( p[0].y + p[1].y + p[2].y + p[3].y ) / 4 );
}

//---------------------------------------------------------------------------
// HarnessDumpUnits — enumerate the LOCAL PLAYER's units (vehicles + buildings)
// and project each to its area-window pixel, so a headless driver can locate &
// click the crane (or any unit) deterministically instead of blind-sweeping.
// Declared in en_harness.h. Called on the game/render thread (reads live state).
// One line per unit:
//   vehicle:  "<id> <screenX> <screenY> <kind> <me|other>\n"
//   building: "<id> <screenX> <screenY> building <me|other> <constructing|operational>\n"
// The building build-state is an appended 6th field (backward-compatible — older
// parsers read fields 1-5); poll it for "operational" to know the info window
// will open (a foundation/constructing building's info window stays closed).
//---------------------------------------------------------------------------
void HarnessDumpUnits( std::string& out )
{
    out.clear( );

    CWndArea* a = theAreaList.GetTop( );
    if ( a == NULL )
    {
        out = "err no-area-window\n";
        return;
    }
    CAnimAtr& aa = a->GetAnimAtr( );
    char line[160];
    int  iVehTotal = 0, iVehMine = 0, iBldgTotal = 0, iBldgMine = 0;
    std::string body;

    // Vehicles (the crane lives here). Owner==me OR no-owner-filter fallback:
    // some headless SP states leave m_bMe momentarily unset, so if NObody is
    // flagged "me" we fall back to listing all vehicles (still useful to locate).
    POSITION pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID = 0;
        CVehicle* pVeh = NULL;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        if ( pVeh == NULL )
            continue;
        ++iVehTotal;
        bool bMine = ( pVeh->GetOwner( ) != NULL && pVeh->GetOwner( )->IsMe( ) );
        if ( bMine )
            ++iVehMine;

        CPoint pt = HarnessHexToWindow( aa, pVeh->GetHexHead( ) );

        const char*           kind  = "vehicle";
        CTransportData const* pData = pVeh->GetData( );
        if ( pData != NULL )
        {
            if ( pData->IsCrane( ) )          kind = "crane";
            else if ( pData->IsTransport( ) ) kind = "transport";
            else if ( pData->IsCarrier( ) )   kind = "carrier";
            else if ( pData->IsPeople( ) )    kind = "infantry";
        }

        snprintf( line, sizeof( line ), "%lu %d %d %s %s\n",
                  (unsigned long) dwID, (int) pt.x, (int) pt.y, kind,
                  bMine ? "me" : "other" );
        body += line;
    }

    // Buildings (for the building-info screenshot sweep).
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID  = 0;
        CBuilding* pBldg = NULL;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        if ( pBldg == NULL )
            continue;
        ++iBldgTotal;
        bool bMine = ( pBldg->GetOwner( ) != NULL && pBldg->GetOwner( )->IsMe( ) );
        if ( bMine )
            ++iBldgMine;

        CPoint pt = HarnessHexToWindow( aa, pBldg->GetHex( ) );
        // Build-state (appended, backward-compatible 6th field): lets a headless
        // driver poll "is it done?" deterministically instead of guessing the
        // construction wait — a freshly-placed building is a foundation and its
        // info window won't open until the crane finishes it. "operational" = the
        // info window will open; "constructing" = still a foundation/being built.
        const char* bstate = pBldg->IsConstructing( ) ? "constructing" : "operational";
        snprintf( line, sizeof( line ), "%lu %d %d building %s %s\n",
                  (unsigned long) dwID, (int) pt.x, (int) pt.y, bMine ? "me" : "other", bstate );
        body += line;
    }

    snprintf( line, sizeof( line ), "# veh %d/%d mine, bldg %d/%d mine\n",
              iVehMine, iVehTotal, iBldgMine, iBldgTotal );
    out  = line;
    out += body;
}

//---------------------------------------------------------------------------
// HarnessDumpBldgState — enumerate ALL buildings (mine AND AI/other) with their
// combat/construction state, so a headless driver can verify fire-control fixes
// (e.g. #60 option-(a): a FINISHED+stopped armed camp must compute fireRate>0)
// without driving live combat. Reads only public accessors; called on the game/
// render thread. One line per building:
//   "<id> t<type> <me|other> cd<constDone> stop<0|1> bfr<baseFR> fr<fireRate>\n"
// where type=BLDG_TYPE (0=city 1=rocket 16=barracks_2/camp), constDone=-1 means
// finished, baseFR=_GetFireRate() (>0 ⇒ armed), fr=GetFireRate() (>0 ⇒ will fire).
// Declared in en_harness.h.
//---------------------------------------------------------------------------
void HarnessDumpBldgState( std::string& out )
{
    out.clear( );
    int iTotal = 0, iArmed = 0, iArmedSilent = 0;
    std::string body;
    char line[160];

    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID  = 0;
        CBuilding* pBldg = NULL;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        if ( pBldg == NULL )
            continue;
        ++iTotal;
        bool bMine    = ( pBldg->GetOwner( ) != NULL && pBldg->GetOwner( )->IsMe( ) );
        CStructureData const* pData = pBldg->GetData( );
        int  iType    = ( pData != NULL ) ? (int) pData->GetType( )      : -1;
        int  iConst   = pBldg->GetConstDone( );
        int  iStopped = ( pBldg->GetFlags( ) & CUnit::stopped ) ? 1 : 0;
        int  iBaseFR  = ( pData != NULL ) ? pData->_GetFireRate( )       : 0;  // >0 ⇒ armed (the :1429 gate)
        int  iFireRt  = pBldg->GetFireRate( );       // live computed (>0 ⇒ fires)
        if ( iBaseFR > 0 )
        {
            ++iArmed;
            if ( iFireRt == 0 )
                ++iArmedSilent;                       // armed but won't fire (#60 symptom)
        }
        snprintf( line, sizeof( line ), "%lu t%d %s cd%d stop%d bfr%d fr%d\n",
                  (unsigned long) dwID, iType, bMine ? "me" : "other",
                  iConst, iStopped, iBaseFR, iFireRt );
        body += line;
    }

    snprintf( line, sizeof( line ), "# bldg %d total, armed %d, armed-but-silent %d\n",
              iTotal, iArmed, iArmedSilent );
    out  = line;
    out += body;
}

//---------------------------------------------------------------------------
// HarnessCenterUnit — center the focused area view on the unit with `id`
// (vehicle or building), so a headless driver can then click view-center to
// select it. Declared in en_harness.h. Called on the game/render thread.
//---------------------------------------------------------------------------
bool HarnessCenterUnit( unsigned long id )
{
    CWndArea* a = theAreaList.GetTop( );
    if ( a == NULL )
        return false;

    DWORD dwID = (DWORD) id;

    CVehicle* pVeh = NULL;
    if ( theVehicleMap.Lookup( dwID, pVeh ) && pVeh != NULL )
    {
        a->Center( (CUnit*) pVeh );
        return true;
    }

    CBuilding* pBldg = NULL;
    if ( theBuildingMap.Lookup( dwID, pBldg ) && pBldg != NULL )
    {
        a->Center( (CUnit*) pBldg );
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
// HarnessHexInfo — report the map hex under an area-window client pixel: its hex
// coords, altitude, visibility, and whether a unit occupies it. READ-ONLY (no game
// or view mutation). Lets a headless driver find a FLAT buildable spot (scan a few
// points, pick where alts match with no unit) or a SLOPE (adjacent hexes whose alt
// differs) deterministically, instead of eyeballing the placement cursor's OK/
// no-build sprite (iCurType, ~area.cpp:1964) in a downscaled screenshot. Mirrors the
// screen->hex path in CWndArea::OnMouseMove (GetAA().GetHit -> _GetHexCoord ->
// theMap._GetHex). Pass the same area-window client px you'd give clickid/dblclickid.
// Call on the render thread (reads the view + map). Backs the `hexinfo <areaWin> <x>
// <y>` control_socket command. Declared in en_harness.h.
//---------------------------------------------------------------------------
void HarnessHexInfo( int x, int y, std::string& out )
{
    CWndArea* a = theAreaList.GetTop( );
    if ( a == NULL ) { out = "err no-area\n"; return; }

    CPoint    point( x, y );
    CHitInfo  hitinfo = a->GetAA( ).GetHit( point );
    CHexCoord hexcoord( hitinfo._GetHexCoord( ) );
    hexcoord.Wrap( );
    CHex* pHex = theMap._GetHex( hexcoord );
    if ( pHex == NULL ) { out = "err no-hex\n"; return; }

    out  = "hex " + IntToStr( hexcoord.X( ) ) + " " + IntToStr( hexcoord.Y( ) );
    out += " alt " + IntToStr( pHex->GetAlt( ) );
    out += " vis " + std::string( pHex->GetVisible( ) ? "1" : "0" );
    out += " unit " + std::string( hitinfo.GetUnit( ) != NULL ? "1" : "0" );
    // water + tree block building but are NOT "units" — without these a flat, clear-of-
    // units, visible hex still fails FoundationCost (trees/water), which is exactly what
    // bit the placement captures. A buildable spot = water 0, tree 0, flat (matching
    // neighbour alts), vis 1, unit 0.
    out += " water " + std::string( pHex->IsWater( ) ? "1" : "0" );
    out += " tree " + IntToStr( pHex->GetTree( ) );
    // Terrain sprite type id + variant index. Lets a headless QA driver LOCATE
    // terrain-blend boundaries deterministically — e.g. a river hex (terr=CHex::river)
    // adjacent to a lake hex (terr=CHex::lake), which are BOTH water=1 and so were
    // previously indistinguishable here. Needed to repro the #8 river<->lake blend
    // (and the badlands<->ocean / swamp<->rough blend family).
    CTerrainSprite* spr = pHex->GetSprite( );
    out += " terr " + IntToStr( spr ? spr->GetID( ) : -1 );
    out += " tidx " + IntToStr( spr ? spr->GetIndex( ) : -1 );
    out += "\n";
}

//---------------------------------------------------------------------------
// HarnessFindTerrain — scan the whole map for the first hex of terrain type `id`
// (CHex terrain enum: lake=3, ocean=6, river=8, swamp=11, ...); if adjId>=0 also
// require a 4-neighbour of type adjId (e.g. river next to lake = the #8 river<->lake
// blend boundary). On a hit, center the focused area view on that hex so a headless
// driver can `shotid 5` the area window right at the boundary — the missing piece
// for terrain-blend repro (the harness `center` only centers on a UNIT, and devsaves
// open on the all-land base). READ-ONLY of the map; mutates only the view. Render
// thread only. Declared in en_harness.h.
//---------------------------------------------------------------------------
void HarnessFindTerrain( int id, int adjId, std::string& out )
{
    CWndArea* a = theAreaList.GetTop( );
    if ( a == NULL ) { out = "err no-area\n"; return; }

    CSize sz = theMap.GetSize( );
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    for ( int y = 0; y < sz.cy; ++y )
    {
        for ( int x = 0; x < sz.cx; ++x )
        {
            CHex* h = theMap.GetHex( x, y );
            if ( h == NULL ) continue;
            CTerrainSprite* s = h->GetSprite( );
            if ( s == NULL || s->GetID( ) != id ) continue;

            int ax = -1, ay = -1;
            if ( adjId >= 0 )
            {
                bool adj = false;
                for ( int e = 0; e < 4; ++e )
                {
                    CHexCoord nc( x + dx[e], y + dy[e] ); nc.Wrap( );
                    CHex* n = theMap.GetHex( nc );
                    if ( n && n->GetSprite( ) && n->GetSprite( )->GetID( ) == adjId )
                    { adj = true; ax = nc.X( ); ay = nc.Y( ); break; }
                }
                if ( !adj ) continue;
            }

            CHexCoord hc( x, y );
            CMapLoc   ml( hc );
            a->Center( ml );   // move the view to the found hex (then caller shotid 5)

            out  = "found " + IntToStr( x ) + " " + IntToStr( y );
            out += " terr " + IntToStr( id );
            if ( adjId >= 0 ) out += " adj " + IntToStr( adjId ) + " at " + IntToStr( ax ) + " " + IntToStr( ay );
            out += " (view centered)\n";
            return;
        }
    }
    out = "notfound terr " + IntToStr( id );
    if ( adjId >= 0 ) out += " adj " + IntToStr( adjId );
    out += "\n";
}

//---------------------------------------------------------------------------
// HarnessDumpEdicts — list every civ-wide edict and whether it's currently active
// for the local player (CPlayer::IsEdictActive). Lets a headless QA driver verify an
// edict TOGGLE: read state, clickid the checkbox, read state again. Backs `edicts`.
// Render/game thread only (reads live player state). Declared in en_harness.h.
//---------------------------------------------------------------------------
void HarnessDumpEdicts( std::string& out )
{
    CPlayer* me = theGame.GetMe( );
    if ( me == NULL ) { out = "err no-player (not in-game?)\n"; return; }
    for ( int id = 0; id < EDICT_COUNT; ++id )
    {
        const EdictDef& e = g_aEdicts[id];
        out += "edict " + IntToStr( id ) + " "
             + std::string( e.name ? e.name : "?" )
             + " active " + std::string( me->IsEdictActive( id ) ? "1" : "0" ) + "\n";
    }
}

//---------------------------------------------------------------------------
// HarnessDumpAltBuildings — list every AltOutput-capable building OWNED BY the local
// player (one line: id, alt_oil on/off, mode, label) so a headless QA driver can FIND a
// coal plant / BioFuel / charcoal / fracking building by id without a 500-window showinfo
// scan (each showinfo opens a new stacking window — impractical). Mirrors the building loop
// in HarnessDumpUnits + the AltOutput::Available filter (the exact predicate secAltOutput
// uses). Backs `altbldgs`. Render/game thread only. Declared in en_harness.h.
//---------------------------------------------------------------------------
void HarnessDumpAltBuildings( std::string& out )
{
    if ( theAreaList.GetTop( ) == NULL ) { out = "err not in-game\n"; return; }
    char line[256];
    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID  = 0;
        CBuilding* pBldg = NULL;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        if ( pBldg == NULL )
            continue;
        if ( pBldg->GetOwner( ) == NULL || !pBldg->GetOwner( )->IsMe( ) )
            continue;
        const AltOutput::AltOutputDef* pDef = AltOutput::Available( pBldg );
        if ( pDef == NULL )
            continue;
        snprintf( line, sizeof( line ), "altbldg %lu on %d mode %d label \"%s\"\n",
                  (unsigned long) dwID,
                  pBldg->IsFlag( CUnit::alt_oil ) ? 1 : 0,
                  (int) pDef->m_eMode,
                  pDef->m_szLabel ? pDef->m_szLabel : "?" );
        out += line;
    }
    if ( out.empty( ) )
        out = "# no AltOutput-capable buildings owned by me\n";
}

//---------------------------------------------------------------------------
// HarnessDumpPlayerStats — exact colony stats for the local player, so a QA driver can read
// a precise before/after delta (the in-game Office/Apartment readouts clip the 4th digit
// behind the history graph). Backs `pstats`. Render/game thread only.
//---------------------------------------------------------------------------
void HarnessDumpPlayerStats( std::string& out )
{
    CPlayer* me = theGame.GetMe( );
    if ( me == NULL ) { out = "err no-player (not in-game?)\n"; return; }
    out += "pplneedbldg " + IntToStr( me->GetPplNeedBldg( ) ) + "\n";   // workforce NEED (edict + #2 workforce hooks land here)
    out += "pplbldg "     + IntToStr( me->GetPplBldg( ) )     + "\n";   // workforce ON HAND
    out += "pwrneed "     + IntToStr( me->GetPwrNeed( ) )     + "\n";   // power NEED (Mining-Subsidy upkeep lands here)
    out += "pwrhave "     + IntToStr( me->GetPwrHave( ) )     + "\n";
    out += "food "        + IntToStr( me->GetFood( ) )        + "\n";
    out += "foodneed "    + IntToStr( me->GetFoodNeed( ) )    + "\n";
}

//---------------------------------------------------------------------------
// HarnessShowInfoWindow — open a building's read-only info window by unit id (same
// window a map double-click opens via CBuilding::ShowInfoWindow). Lets a QA driver
// deterministically open a SPECIFIC building (e.g. an edict-host office/command/
// apartment) without screen-coord clicking, then clickid its edict checkbox. Render
// thread only (creates a non-modal dialog). Backs `showinfo <bldgid>`. en_harness.h.
//---------------------------------------------------------------------------
bool HarnessShowInfoWindow( unsigned long id )
{
    if ( theAreaList.GetTop( ) == NULL ) return false;   // not in-game
    CBuilding* pBldg = theBuildingMap.GetBldg( (DWORD) id );
    if ( pBldg == NULL ) return false;
    pBldg->ShowInfoWindow( );
    return true;
}

//---------------------------------------------------------------------------
// HarnessSetAltOil — set/clear a building's alt_oil (AltOutput) flag directly by unit id,
// bypassing the info-window checkbox. QA-only: lets a driver verify AltOutput effects
// (BioFuel/Coal-Liq/Charcoal/Fracking + the #2 kiln workforce cost) via pstats without
// fighting checkbox click-coords. Mirrors the BuildAltOutput onChange (SetFlag/ClrFlag).
// Render/game thread only. Backs `setalt`.
//---------------------------------------------------------------------------
bool HarnessSetAltOil( unsigned long id, bool on )
{
    if ( theAreaList.GetTop( ) == NULL ) return false;   // not in-game
    CBuilding* pBldg = theBuildingMap.GetBldg( (DWORD) id );
    if ( pBldg == NULL ) return false;
    if ( on ) pBldg->SetFlag( CUnit::alt_oil );
    else      pBldg->ClrFlag( CUnit::alt_oil );
    return true;
}

//---------------------------------------------------------------------------
// HarnessSetEdict — set/clear a civ-wide edict for the local player directly by edict id,
// bypassing the info-window checkbox. QA-only: lets a driver verify edict DOWNSIDE deltas
// (workforce/power/food upkeep) via pstats without hunting per-window checkbox click-coords
// or ambiguity over which row flipped. Routes through ToggleEdictNet — the exact path the
// BuildEdicts checkbox onChange takes (ToggleEdict + RecomputeEdictMults; broadcasts only in
// a net game) — so the effect is identical to a real click. Render/game thread only. Backs
// `setedict`. Declared in en_harness.h.
//---------------------------------------------------------------------------
bool HarnessSetEdict( int edictId, bool on )
{
    if ( theAreaList.GetTop( ) == NULL ) return false;   // not in-game
    if ( edictId < 0 || edictId >= EDICT_COUNT ) return false;
    CPlayer* me = theGame.GetMe( );
    if ( me == NULL ) return false;
    me->ToggleEdictNet( edictId, on );
    return true;
}

//---------------------------------------------------------------------------
// HarnessSaveGame — save the current game to <path> headlessly so a developed/
// researched game can be snapshotted and SHARED (one such save unblocks all the
// research-gated work team-wide: gated buildings, AltOutput in-game verify, late-
// game feature tests). CGame::SaveGame skips its file-browser modal when
// m_gameWindow is set AND m_sFileName is pre-filled, so we just set the path and
// call it. Must run on the render/main thread (touches UI + game state) — call
// from EnHarness_Service, never the socket thread. Returns true on a written save.
// Declared in en_harness.h.
//---------------------------------------------------------------------------
bool HarnessSaveGame( const char* path )
{
    if ( path == NULL || path[0] == '\0' )
        return false;
    // SaveGame returns IDCANCEL unless we're in-game (area window live, rocket
    // already placed) — guard so a menu-time save just reports failure.
    if ( theAreaList.GetTop( ) == NULL )
        return false;
    theGame.m_sFileName = path;            // pre-fill => SaveGame skips the browser
    return ( theGame.SaveGame( (CWnd*) NULL ) == IDOK );
}

//---------------------------------------------------------------------------
// HarnessLoadGame — load a .en save headlessly from the MAIN MENU. Runs the normal
// SP load flow (SDL2_RunLoadSinglePlayerFlow) but, while g_harnessLoadPath is set,
// CGame::LoadGame skips the file-browser (uses the path) and
// SDL2_RunLoadSinglePlayerFlow skips the pick-player modal (auto-selects _GetMe()).
// Lets a headless driver consume a shared developed save (the POSIX menu file-
// browser isn't harness-drivable). Must run from the main loop (the flow re-pumps
// events). Returns true on a loaded+started game. Declared in en_harness.h.
//---------------------------------------------------------------------------
static std::string g_harnessLoadPath;   // non-empty only during a headless load

const char* HarnessPendingLoadPath( void )
{
    return g_harnessLoadPath.empty( ) ? NULL : g_harnessLoadPath.c_str( );
}

bool HarnessLoadGame( const char* path )
{
    if ( path == NULL || path[0] == '\0' )
        return false;
    // Only valid from the menu (no game in progress). m_pCreateGame catches an
    // in-flight create/load flow, but it is NULL during normal play — so on its
    // own it does NOT reject an in-game `load`, which then tears the world down
    // before the menu-only flow fails, leaving a zombie (chrome window only, no
    // area map). AmInGame() is the "a game is already running" signal; reject
    // cleanly here, before any teardown. [linux2 regression find 2026-06-29]
    if ( theApp.m_pCreateGame != NULL || theApp.AmInGame( ) )
        return false;
    g_harnessLoadPath = path;                          // arms the headless skips
    bool bOk = false;
    try { bOk = SDL2_RunLoadSinglePlayerFlow( theApp.m_gameWindow.get( ) ); }
    catch ( ... ) { bOk = false; }
    g_harnessLoadPath.clear( );                        // disarm (also on failure)
    return bOk;
}

//---------------------------------------------------------------------------
// HarnessGrantResearch — POSIX analogue of win's Windows F12 hotkey: discover ALL
// research for the local human instantly so the research-gated tail is reachable
// without the multi-hour grind. Cheat-gated to the game's convention (win @e79a75ae):
//   - #ifdef _CHEAT  → compiled OUT of Release (DebugDiscoverAllResearch is _CHEAT-only)
//   - [Cheat]/GrantResearch registry flag (default 0) → opt-in even in Debug
//   - GetNetNum()==0 → single-player only (MP would desync on a local mutation)
// Returns false (no-op) in Release, or if not opted-in / not in-game-SP.
// Call on the game/render thread. Declared in en_harness.h.
//---------------------------------------------------------------------------
bool HarnessGrantResearch( void )
{
#ifdef _CHEAT
    if ( !EnGetProfileInt( "Cheat", "GrantResearch", 0 ) )
        return false;                                   // opt-in only
    CPlayer* me = theGame.GetMe( );
    if ( me == NULL || me->GetNetNum( ) != 0 )
        return false;                                   // in-game + single-player only
    me->DebugDiscoverAllResearch( );
    return true;
#else
    return false;                                       // cheat compiled out of Release
#endif
}
