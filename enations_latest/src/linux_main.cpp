//---------------------------------------------------------------------------
// linux_main.cpp — Linux entry point. The Win32 build enters at WinMain()
// (WinMain.cpp); on Linux we provide a standard main() that synthesizes the
// WinMain arguments and forwards. Linux build only (excluded from MSVC).
//---------------------------------------------------------------------------

#ifdef _WIN32
#error "linux_main.cpp is the Linux entry shim and must not be compiled on Windows"
#endif

#include "win32_compat.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <string>

extern "C" int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                LPSTR lpCmdLine, int nCmdShow);

int main(int argc, char** argv) {
    // We own main(); tell SDL not to expect its own entry shim.
    SDL_SetMainReady();

    // Pick the launch resolution. The engine only renders correctly at its
    // designed fixed sizes (≤ 1280x1024) — running at an arbitrary desktop size
    // (e.g. 1741x1081) breaks the terrain rasterizer (black base + scattered
    // sprites). So use the desktop size only when it's within the supported
    // envelope; otherwise stay at 1280x1024. (True fullscreen on a larger-only
    // display needs a 1280x1024→desktop scaling layer — a separate task.)
    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) == 0 ) {
        SDL_DisplayMode dm;
        if ( SDL_GetDesktopDisplayMode( 0, &dm ) == 0 && dm.w > 0 && dm.h > 0 )
            en_SetScreenMetrics( dm.w, dm.h );
    }

    // Rebuild a single command-line string (argv[1..]) as WinMain expects.
    std::string cmdline;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) cmdline += ' ';
        cmdline += argv[i];
    }

    return WinMain((HINSTANCE)GetModuleHandleA(NULL), (HINSTANCE)NULL,
                   (LPSTR)cmdline.c_str(), SW_SHOWNORMAL);
}
