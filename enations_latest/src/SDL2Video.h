#pragma once

#include <SDL.h>
#include <string>
#include <vector>

class GameWindow;

// ============================================================================
// SDL2 Video Player - plays MPEG-1 video files using pl_mpeg (single-header
// library, zero external dependencies, cross-platform).
//
// The game ships pre-converted .mpg files (MPEG-1 video + MP2 audio).
// Original Indeo 4 AVI files were decoded using the real Intel VFW codec
// and re-encoded to MPEG-1 for universal playback.
// ============================================================================

class SDL2VideoPlayer {
public:
    // Play a video file. Blocks until video ends or user dismisses (any key/click).
    // Returns true if video played to completion, false if dismissed or error.
    static bool PlayVideo(GameWindow* gameWindow, const std::string& filePath);

    // Play a sequence of videos (e.g., logo then intro).
    static bool PlayVideos(GameWindow* gameWindow, const std::vector<std::string>& filePaths);
};
