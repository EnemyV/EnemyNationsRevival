#ifndef __RENDER_BACKEND_H__
#define __RENDER_BACKEND_H__

//---------------------------------------------------------------------------
// RenderBackend — single source of truth for the selectable rendering backend.
//
// [Advanced] RenderBackend:  0 = Software, 1 = SDL2 (GPU), 2 = OpenGL.
//
// Centralizes what used to be ad-hoc `GetProfileInt("Advanced","Renderer") != 0`
// reads (GameWindow, SDL2Panel, ...) so the 3-way choice doesn't leak the old
// binary assumption. OpenGL is not implemented yet → it falls back to SDL2 here
// and is greyed out in the options UI until the backend is finished (flip
// RenderBackendOpenGLAvailable() to true at that point — see
// plans/opengl-rendering-backend.md).
//
// Registry store is HKCU\Software\Second Chance\Second Chance\Advanced\RenderBackend.
// NOTE: the entry was DELIBERATELY renamed from the old "Renderer" so that stale
// per-user values (many testers had Renderer=0 = software left in HKCU) are
// ignored — everyone gets the SDL2 default until they re-choose in the options
// dialog. Do NOT read the old "Renderer" entry anywhere; it is dead on purpose.
//---------------------------------------------------------------------------

#include <cstdlib>
#include "w22_settings.h"

enum class RenderBackend { Software = 0, SDL2 = 1, OpenGL = 2 };

// Whether the OpenGL backend is finished and selectable. Until then the options
// dialog greys out the OpenGL choice and GetRenderBackend() maps 2 → SDL2.
inline bool RenderBackendOpenGLAvailable() { return false; }

inline RenderBackend GetRenderBackend()
{
#if !defined(_WIN32)
    // SDL2 (GPU) is the DEFAULT backend on Linux: it has the 4-zoom GPU terrain and
    // the flicker-free full-walk sprite layer (the software path's incremental
    // dirty-rect model drops moving vehicles). EN_SOFTWARE forces the legacy
    // software path for debugging; an explicit [Advanced]RenderBackend=0 also honors it.
    // A value of "0" or empty means "not set" (so EN_SOFTWARE=0 doesn't force it).
    if ( const char* e = getenv( "EN_SOFTWARE" ) )
        if ( e[0] && e[0] != '0' )
            return RenderBackend::Software;
    int v = w22::GetProfileInt( "Advanced", "RenderBackend", 1 );  // default SDL2 on Linux
    if ( v == 0 )
        return RenderBackend::Software;
    if ( v == 2 && RenderBackendOpenGLAvailable() )
        return RenderBackend::OpenGL;
    return RenderBackend::SDL2;
#else
    // SDL2 (GPU) is the DEFAULT on Windows too — a fresh install used to land on the
    // legacy software present path (tester logs: gpu=0, no "T0 renderer present path
    // active" line). Software stays selectable via [Advanced]RenderBackend=0.
    int v = w22::GetProfileInt( "Advanced", "RenderBackend", 1 );
    if ( v <= 0 )
        return RenderBackend::Software;
    if ( v == 2 && RenderBackendOpenGLAvailable() )
        return RenderBackend::OpenGL;
    return RenderBackend::SDL2;   // 1, or 2 before the GL backend lands → SDL2
#endif
}

// True when a GPU present path is active (SDL2 today, OpenGL later) — i.e. NOT the
// original software window-surface present. Replaces the old `Renderer != 0` test.
inline bool RenderBackendIsGpu() { return GetRenderBackend() != RenderBackend::Software; }

#endif // __RENDER_BACKEND_H__
