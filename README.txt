Enemy Nations (Windward Studios, 1996), ported to SDL2 for Windows, Linux and macOS.

Full build, data and packaging instructions are in BUILD.md.

Quick build (Windows stages ../ENATIONS.DAT automatically; add -DEN_LOOSE_DATA_DIR=<dir containing data/manifest.txt> to stage loose game data instead):

  Windows (VS 2022):  cmake -S . -B cmakeBuild-x64 -A x64
                      ./build.ps1 -Release -x64
  Linux:              sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
                      cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
                      cmake --build build --target enations
  macOS:              brew install sdl2 sdl2_ttf sdl2_mixer
                      cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
                      cmake --build build-mac --target enations

Tests: configure with -DBUILD_TESTING=ON, then run ctest in the build directory (see BUILD.md).
