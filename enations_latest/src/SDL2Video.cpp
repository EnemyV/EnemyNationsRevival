#include "stdafx.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"
#include "SDL2Video.h"
#include "GameWindow.h"
#include "lastplnt.h"
#include "sfx.h"

#include <SDL.h>
#include <SDL_mixer.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#undef min
#undef max
#include <algorithm>
#include <cstring>

// ============================================================================
// Simple audio ring buffer: pl_mpeg decodes float stereo at 44100,
// we convert to S16 stereo and feed to SDL_mixer via Mix_HookMusic.
// ============================================================================
static const int AUDIO_BUF_SAMPLES = 128 * 1024;  // in Sint16 units

struct VideoAudio {
    Sint16 buf[AUDIO_BUF_SAMPLES];
    volatile int writePos;
    volatile int readPos;
};

static VideoAudio s_va;

static void VideoAudioHook(void* /*udata*/, Uint8* stream, int len) {
    int samplesNeeded = len / (int)sizeof(Sint16);
    Sint16* out = (Sint16*)stream;
    for (int i = 0; i < samplesNeeded; i++) {
        if (s_va.readPos != s_va.writePos) {
            out[i] = s_va.buf[s_va.readPos];
            s_va.readPos = (s_va.readPos + 1) % AUDIO_BUF_SAMPLES;
        } else {
            out[i] = 0;
        }
    }
}

static void VideoAudioFeed(plm_samples_t* samples, int plmSampleRate) {
    // pl_mpeg outputs float stereo interleaved at plmSampleRate (44100).
    // SDL_mixer is at DEVICE_FREQ (22050). Downsample 2:1 by averaging pairs.
    int ratio = plmSampleRate / DEVICE_FREQ;
    if (ratio < 1) ratio = 1;

    int count = samples->count;  // number of stereo sample pairs
    for (int i = 0; i + ratio - 1 < count; i += ratio) {
        for (int ch = 0; ch < 2; ch++) {
            // Average 'ratio' samples for simple low-pass filtering
            float sum = 0;
            for (int j = 0; j < ratio; j++)
                sum += samples->interleaved[(i + j) * 2 + ch];
            float s = sum / ratio;
            if (s > 1.0f) s = 1.0f;
            if (s < -1.0f) s = -1.0f;
            int next = (s_va.writePos + 1) % AUDIO_BUF_SAMPLES;
            if (next != s_va.readPos) {
                s_va.buf[s_va.writePos] = (Sint16)(s * 32767.0f);
                s_va.writePos = next;
            }
        }
    }
}

// ============================================================================
// SDL2VideoPlayer
// ============================================================================

bool SDL2VideoPlayer::PlayVideo(GameWindow* gameWindow, const std::string& filePath) {
    if (!gameWindow || !gameWindow->GetWindow()) return false;

    // Resolve path
    std::string resolvedPath = filePath;
    FILE* f = fopen(resolvedPath.c_str(), "rb");
    if (!f) {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir(exePath);
        size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            exeDir = exeDir.substr(0, lastSlash + 1);
        resolvedPath = exeDir + filePath;
        f = fopen(resolvedPath.c_str(), "rb");
    }
    if (!f) return false;
    fclose(f);

    SDL_Window* window = gameWindow->GetWindow();
    int winW = gameWindow->GetWidth();
    int winH = gameWindow->GetHeight();

    // The legacy Win32 main window (EnemyNationsMainWindow stub, created
    // at lastplnt.cpp:619) is still alive and can steal Z-order when the
    // user clicks during video 1 — video 2 then starts rendering into the
    // SDL window which is now behind the Win32 stub. Raise + reclaim
    // focus on every PlayVideo entry so the video is always visible.
    SDL_RaiseWindow( window );

    // Load file into memory
    FILE* mpgFile = fopen(resolvedPath.c_str(), "rb");
    if (!mpgFile) return false;
    fseek(mpgFile, 0, SEEK_END);
    long fileSize = ftell(mpgFile);
    fseek(mpgFile, 0, SEEK_SET);
    uint8_t* fileData = (uint8_t*)malloc(fileSize);
    fread(fileData, 1, fileSize, mpgFile);
    fclose(mpgFile);

    plm_t* plm = plm_create_with_memory(fileData, fileSize, TRUE);
    if (!plm) { free(fileData); return false; }

    int videoW = plm_get_width(plm);
    int videoH = plm_get_height(plm);
    double fps = plm_get_framerate(plm);

    // Stop game music
    theMusicPlayer.SoundsOff();

    // Set up video audio hook AFTER SoundsOff (which clears Mix_HookMusic)
    memset(&s_va, 0, sizeof(s_va));
    Mix_HookMusic(VideoAudioHook, nullptr);

    plm_set_audio_enabled(plm, TRUE);
    plm_set_audio_lead_time(plm, 2048.0 / 22050.0);

    // RGB buffer
    uint8_t* rgbBuffer = (uint8_t*)malloc(videoW * videoH * 3);
    if (!rgbBuffer) {
        Mix_HookMusic(NULL, NULL);
        plm_destroy(plm);
        return false;
    }

    bool completed = true;
    Uint32 lastFrameTime = SDL_GetTicks();

    while (!plm_has_ended(plm)) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { completed = false; goto cleanup; }
            if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
                completed = false;
                goto cleanup;
            }
        }

        plm_frame_t* frame = plm_decode_video(plm);

        // Decode audio only when the ring buffer is running low.
        // Keep ~4000 samples buffered (about 180ms at 22050 stereo).
        {
            int avail = (s_va.writePos - s_va.readPos + AUDIO_BUF_SAMPLES) % AUDIO_BUF_SAMPLES;
            while (avail < 4000) {
                plm_samples_t* samples = plm_decode_audio(plm);
                if (!samples) break;
                VideoAudioFeed(samples, plm_get_samplerate(plm));
                avail = (s_va.writePos - s_va.readPos + AUDIO_BUF_SAMPLES) % AUDIO_BUF_SAMPLES;
            }
        }

        if (frame) {
            plm_frame_to_rgb(frame, rgbBuffer, videoW * 3);

            SDL_Surface* frameSurf = SDL_CreateRGBSurfaceFrom(
                rgbBuffer, videoW, videoH, 24, videoW * 3,
                0x000000FF, 0x0000FF00, 0x00FF0000, 0);

            if (frameSurf) {
                SDL_Surface* winSurface = gameWindow->GetPresentSurface();  // T0: software surface or renderer back-buffer
                if (winSurface) {
                    float scaleX = (float)winW / videoW;
                    float scaleY = (float)winH / videoH;
                    float scale = std::min(scaleX, scaleY);
                    int dstW = (int)(videoW * scale);
                    int dstH = (int)(videoH * scale);
                    int dstX = (winW - dstW) / 2;
                    int dstY = (winH - dstH) / 2;

                    SDL_FillRect(winSurface, nullptr, SDL_MapRGB(winSurface->format, 0, 0, 0));
                    SDL_Rect dstRect = { dstX, dstY, dstW, dstH };
                    SDL_BlitScaled(frameSurf, nullptr, winSurface, &dstRect);
                    gameWindow->PresentSurface();
                }
                SDL_FreeSurface(frameSurf);
            }

            // Frame timing
            Uint32 now = SDL_GetTicks();
            Uint32 elapsed = now - lastFrameTime;
            Uint32 target = (Uint32)(1000.0 / fps);
            if (elapsed < target)
                SDL_Delay(target - elapsed);
            lastFrameTime = SDL_GetTicks();
        } else {
            SDL_Delay(1);
        }
    }

cleanup:
    Mix_HookMusic(NULL, NULL);
    free(rgbBuffer);
    plm_destroy(plm);

    // Restart main menu music
    theMusicPlayer.PlayExclusiveMusic(MUSIC::GetID(MUSIC::main_screen));

    return completed;
}

bool SDL2VideoPlayer::PlayVideos(GameWindow* gameWindow, const std::vector<std::string>& filePaths) {
    for (auto& path : filePaths) {
        if (!PlayVideo(gameWindow, path))
            return false;
    }
    return true;
}
