#include "stdafx.h"
#include "framecap.h"

#include <SDL.h>
#include <cstdlib>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define FC_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define FC_MKDIR(p) mkdir((p), 0755)
#endif

namespace {
    bool        s_init    = false;
    bool        s_enabled = false;
    int         s_seq     = 0;
    int         s_max     = 600;          // ~10s @60fps; guardrail so it can't fill the disk
    std::string s_dir     = "framecap";

    void EnsureDir() { FC_MKDIR( s_dir.c_str() ); }   // ignore "already exists"

    void EnsureInit() {
        if ( s_init ) return;
        s_init = true;
        const char* e = getenv( "EN_FRAMECAP" );
        s_enabled = ( e && e[0] && e[0] != '0' );
        const char* m = getenv( "EN_FRAMECAP_MAX" );
        if ( m && atoi( m ) > 0 ) s_max = atoi( m );
        const char* d = getenv( "EN_FRAMECAP_DIR" );
        if ( d && d[0] ) s_dir = d;
        if ( s_enabled ) { EnsureDir(); SDL_Log( "FRAMECAP: ON (env) dir=%s max=%d", s_dir.c_str(), s_max ); }
    }
}

void FrameCap::Capture( SDL_Renderer* r, const char* tag ) {
    EnsureInit();
    if ( !s_enabled || !r ) return;
    if ( s_seq >= s_max ) {                    // hit the budget -> stop, don't fill the disk
        SDL_Log( "FRAMECAP: hit cap %d frames, stopping", s_max );
        s_enabled = false;
        return;
    }
    int w = 0, h = 0;
    if ( SDL_GetRendererOutputSize( r, &w, &h ) != 0 || w <= 0 || h <= 0 ) return;
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat( 0, w, h, 32, SDL_PIXELFORMAT_ARGB8888 );
    if ( !s ) return;
    // Read the CURRENT render target (call this AFTER the frame is composited but BEFORE
    // SDL_RenderPresent — the back buffer is undefined after present).
    if ( SDL_RenderReadPixels( r, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch ) == 0 ) {
        char path[600];
        snprintf( path, sizeof( path ), "%s/%s_%05d.bmp", s_dir.c_str(), tag ? tag : "frame", s_seq );
        SDL_SaveBMP( s, path );
        s_seq++;
    }
    SDL_FreeSurface( s );
}

void FrameCap::Toggle() {
    EnsureInit();
    s_enabled = !s_enabled;
    if ( s_enabled ) { s_seq = 0; EnsureDir(); SDL_Log( "FRAMECAP: ON (toggle) dir=%s max=%d", s_dir.c_str(), s_max ); }
    else             { SDL_Log( "FRAMECAP: OFF (toggle) after %d frames", s_seq ); }
}

bool FrameCap::Enabled() { EnsureInit(); return s_enabled; }
