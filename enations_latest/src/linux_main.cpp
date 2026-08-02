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

#include <cstdlib>   // getenv (EN_MAC_USABLE_FULLSCREEN opt-in)
#include <cstring>   // strrchr (exe-dir cwd anchor)
#include <climits>   // PATH_MAX
#include <unistd.h>  // chdir
#include <string>
#include "en_logpath.h"   // EnLogCaptureLaunchDir - must run before the exe-dir chdir

extern "C" int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                LPSTR lpCmdLine, int nCmdShow);

int main(int argc, char** argv) {
    // We own main(); tell SDL not to expect its own entry shim.
    SDL_SetMainReady();

    // Capture the launch cwd BEFORE the chdir below so debug logs land where the
    // game was launched from rather than beside the exe. See en_logpath.h.
    EnLogCaptureLaunchDir();

    // Anchor the working directory to the executable's own directory so the game
    // finds its co-located ENations.dat / data/ / res/ no matter how it was launched.
    // The data pipeline (CDataFile) resolves paths relative to cwd, but Finder
    // double-clicks and absolute-path launches start with cwd = "/" or $HOME, which
    // threw ERR_DATAFILE_NO_ENTRY. GetModuleFileNameA already resolves the exe path
    // via _NSGetExecutablePath (mac) / /proc/self/exe (Linux). No-op when cwd is
    // already the package dir (the previous working "cd in first" launch path).
    {
        char exePath[PATH_MAX];
        if ( GetModuleFileNameA( NULL, exePath, sizeof(exePath) ) > 0 ) {
            char* slash = strrchr( exePath, '/' );
            if ( slash ) { *slash = '\0'; (void)chdir( exePath ); }
        }
    }

    // Pick the launch resolution. The engine only renders correctly at its
    // designed fixed sizes (≤ 1280x1024) — running at an arbitrary desktop size
    // (e.g. 1741x1081) breaks the terrain rasterizer (black base + scattered
    // sprites). So use the desktop size only when it's within the supported
    // envelope; otherwise stay at 1280x1024. (True fullscreen on a larger-only
    // display needs a 1280x1024→desktop scaling layer — a separate task.)
    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) == 0 ) {
        SDL_DisplayMode dm;
        if ( SDL_GetDesktopDisplayMode( 0, &dm ) == 0 && dm.w > 0 && dm.h > 0 ) {
            int scrW = dm.w, scrH = dm.h;
#ifdef __APPLE__
            // #47 symptom-5 (opt-in, default OFF): when EN_MAC_USABLE_FULLSCREEN is set,
            // render to the display's USABLE bounds (excl. Dock + menu bar) so the matching
            // window (GameWindow.cpp, same flag) sits in the usable area instead of under
            // the Dock. Usable bounds are <= the desktop size already used here, so there is
            // no new terrain-rasterizer envelope risk. Default OFF = full desktop (unchanged).
            const char* usableFs = getenv( "EN_MAC_USABLE_FULLSCREEN" );
            SDL_Rect usable;
            if ( usableFs && usableFs[0] && usableFs[0] != '0'
                 && SDL_GetDisplayUsableBounds( 0, &usable ) == 0
                 && usable.w > 0 && usable.h > 0 ) {
                scrW = usable.w;
                scrH = usable.h;
            }
#endif
            en_SetScreenMetrics( scrW, scrH );
        }
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
