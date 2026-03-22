# MSS32 to SDL Audio Migration Plan

## Overview

Enemy Nations uses RAD Game Tools' **Miles Sound System (MSS) v3.6B** (1997) for all audio. MSS32 is a closed-source 32-bit DLL with no 64-bit version, making it a blocker for x64 builds. This plan replaces MSS32 with **SDL2 Audio** (via `SDL_mixer` for mixing/multi-channel support).

---

## Current Architecture

### Files Involved

| File | Role |
|------|------|
| `windward/wind22/src/music.cpp` | **Primary implementation** -- all AIL_ API calls live here (~93 explicit calls + ~13 via `MEM_alloc_lock`/`MEM_free_lock` macros) |
| `windward/wind22/include/music.h` | `CMusicPlayer`, `CRawChannel`, `CRawData` class definitions |
| `windward/wind22/src/acmutil.cpp:295` | Uses `MEM_alloc_lock()` (an MSS32 macro) for ADPCM decode buffer allocation |
| `enations_latest/src/stdafx.h:33` | `#include <mss/mssw.h>` |
| `windward/wind22/include/stdafx.h:42` | `#include <mss/mssw.h>` |
| `enations_latest/src/lastplnt.cpp:1493-1507,2874-2886` | 4 AIL_ calls + 8 MSS constant refs (`DIG_F_*`) across 2 diagnostic functions |
| `enations_latest/src/CMakeLists.txt:168` | Links `mss32.lib` |
| `enations_latest/src/CMakeLists.txt:218` | Copies `MSS32.DLL` to output |
| `tools/mss/` | MSS headers (`mssw.h`, `mss.h`, `wail.h`) and binaries (`MSS32.DLL`, `MSS32.LIB`) |

> **Note:** `enations/src/` (non-`_latest`) is a legacy copy not in the active CMake build. It has its own
> `STDAFX.H:40` including MSS and `lastplnt.cpp` with AIL_ calls. These are not part of the migration
> but should be cleaned up or deleted afterward.

### Abstraction Layer

The game code **almost never calls MSS32 directly**. All game audio goes through the single global `CMusicPlayer theMusicPlayer` instance. The only exceptions are 2 diagnostic calls to `AIL_digital_configuration()` in `lastplnt.cpp` (via the `_GetHDig()` accessor) and MSS's `MEM_alloc_lock` macro used in `acmutil.cpp`. The public API of `CMusicPlayer` is:

**Lifecycle:**
- `InitData(mode, group)` -- load audio data from RIFF files
- `Open(musicVol, soundVol, mode, group)` -- initialize audio subsystem
- `Close()` -- shut down audio
- `OnActivate(active)` -- handle app focus gain/loss
- `ShutUp()` / `SoundsOff()` -- silence everything

**Music (MIDI):**
- `StartMidiMusic()` / `StopMidiMusic()` -- XMIDI sequence playback

**Music (Digital):**
- `PlayExclusiveMusic(soundID)` -- play a WAV music track (pauses MIDI)
- `EndExclusiveMusic()` -- stop exclusive music, resume MIDI
- `PlayMusicGroup(firstID, count)` -- rotate through a set of music tracks

**Sound Effects:**
- `PlayForegroundSound(id, priority, pan, vol)` -- one-shot SFX
- `KillForegroundSound(id)` -- stop a foreground sound
- `ClrBackgroundSounds()` / `QueueBackgroundSound()` / `UpdateBackgroundSounds()` -- ambient loop system
- `IncBackgroundSound()` / `DecBackgroundSound()` -- adjust background sound counts

**Volume:**
- `GetMusicVolume()` / `SetMusicVolume(vol)` -- 0-100
- `GetSoundVolume()` / `SetSoundVolume(vol)` -- 0-100
- `SetMusicSoundVolume(musVol, sfxVol)` -- **declared but never implemented or called (dead code)**

**Query:**
- `SoundsPlaying()` / `MusicPlaying()` / `IsRunning()` / `MidiOk()` / `WavOk()`
- `GetVersion()` / `UseDirectSound()` / `GetMode()`
- `IsGroupLoaded(grp)` / `LoadGroup(grp)` / `UnloadGroup(grp)` / `UnloadCache(time)` / `FreeOldSounds(sec)`

**Internal (exposed but should remain internal):**
- `YieldPlayer()` -- calls `AIL_serve()`
- `_GetHDig()` / `_GetHMidi()` -- raw MSS handles (used by `lastplnt.cpp` diagnostics)

### MSS32 API Usage Summary

All in `music.cpp` unless noted:

| MSS32 Function | SDL Equivalent | Count | Purpose |
|---|---|---|---|
| `AIL_startup()` / `AIL_shutdown()` | `SDL_Init(SDL_INIT_AUDIO)` / `Mix_CloseAudio()` + `SDL_QuitSubSystem()` | 5 | Init/teardown |
| `AIL_waveOutOpen()` / `AIL_waveOutClose()` | `Mix_OpenAudio()` / `Mix_CloseAudio()` | 6 | Open/close digital device |
| `AIL_midiOutOpen()` / `AIL_midiOutClose()` | SDL_mixer MIDI or drop MIDI entirely | 4 | Open/close MIDI device |
| `AIL_allocate_sample_handle()` / `AIL_release_sample_handle()` | `Mix_Chunk` + channel management | 2 | Allocate mixing channel |
| `AIL_init_sample()` | Reset channel state | ~5 | Reset sample for reuse |
| `AIL_set_sample_type()` | Audio format in `SDL_AudioSpec` | ~2 | Set PCM format |
| `AIL_set_sample_address()` | `Mix_QuickLoad_RAW()` or manual `Mix_Chunk` | ~2 | Point to PCM buffer |
| `AIL_load_sample_buffer()` | Custom stream callback or `Mix_Chunk` | ~4 | Double-buffer loading |
| `AIL_start_sample()` | `Mix_PlayChannel()` | ~2 | Begin playback |
| `AIL_end_sample()` | `Mix_HaltChannel()` | ~15 | Stop playback |
| `AIL_sample_status()` | `Mix_Playing()` + channel state tracking | ~12 | Query if playing/done |
| `AIL_sample_buffer_ready()` | Custom stream callback | ~3 | Double-buffer status |
| `AIL_set_sample_volume()` | `Mix_Volume()` | ~5 | Per-channel volume |
| `AIL_set_sample_pan()` | `Mix_SetPanning()` | ~2 | Per-channel pan |
| `AIL_set_sample_playback_rate()` | Pre-convert or SDL resampling | ~2 | Set playback rate |
| `AIL_set_sample_loop_count()` | `Mix_PlayChannel(..., loops)` — note: MSS `0` = infinite loop, SDL_mixer `-1` = infinite loop, `0` = play once | ~4 | Loop control |
| `AIL_set_sample_user_data()` / `AIL_sample_user_data()` | Manual map (channel -> CRawChannel*) | ~3 | Associate user pointer |
| `AIL_register_EOB_callback()` / `AIL_register_EOS_callback()` | `Mix_ChannelFinished()` + custom stream | ~4 | End-of-buffer/sample callbacks |
| `AIL_set_digital_master_volume()` / `AIL_digital_master_volume()` | `Mix_MasterVolume()` (SDL_mixer 2.6+) or `Mix_Volume(-1, vol)` | ~3 | Master digital volume |
| `AIL_allocate_sequence_handle()` / `AIL_release_sequence_handle()` | `Mix_LoadMUS()` from memory | 2 | MIDI sequence handle |
| `AIL_init_sequence()` / `AIL_start_sequence()` / `AIL_end_sequence()` | `Mix_PlayMusic()` / `Mix_HaltMusic()` | ~8 | MIDI playback |
| `AIL_sequence_status()` | `Mix_PlayingMusic()` | ~3 | MIDI status query |
| `AIL_register_sequence_callback()` | `Mix_HookMusicFinished()` | ~3 | MIDI end callback |
| `AIL_set_XMIDI_master_volume()` / `AIL_XMIDI_master_volume()` | `Mix_VolumeMusic()` | ~3 | MIDI volume |
| `AIL_digital_handle_release()` / `AIL_digital_handle_reacquire()` | Pause/resume via `Mix_Pause(-1)` / `Mix_Resume(-1)` | 2 | App focus handling |
| `AIL_MIDI_handle_release()` / `AIL_MIDI_handle_reacquire()` | `Mix_PauseMusic()` / `Mix_ResumeMusic()` | 2 | MIDI focus handling |
| `AIL_serve()` | Process deferred callback work (see threading model change) | ~25 | Pump audio driver; becomes deferred-work dispatch point |
| `AIL_lock()` / `AIL_unlock()` | `SDL_LockAudioDevice()` / `SDL_UnlockAudioDevice()` | 2 | Thread safety |
| `AIL_set_preference()` | N/A (SDL handles device selection) | ~3 | Driver preferences |
| `AIL_DLL_version()` | `SDL_GetVersion()` / `Mix_Linked_Version()` | 2 | Version string |
| `AIL_digital_configuration()` | Query `SDL_AudioSpec` from opened device | 2 | In `lastplnt.cpp` diagnostics |
| `MEM_alloc_lock()` (macro for `AIL_mem_alloc_lock`) | `malloc()` or `SDL_malloc()` | ~10 | Page-locked alloc (unnecessary on modern Windows) |
| `MEM_free_lock()` (macro for `AIL_mem_free_lock`) | `free()` or `SDL_free()` | ~6 | Free page-locked memory |

### Audio Formats Used

| Context | Format | Rate | Channels |
|---------|--------|------|----------|
| SFX (MIDI mode) | 8-bit PCM unsigned | 11025 Hz | Mono |
| SFX (Digital mode) | 16-bit PCM signed | 22050 Hz | Stereo |
| Digital Music | 16-bit PCM signed | 22050 Hz | Stereo |
| MIDI Music | XMIDI sequences | N/A | N/A |

### Audio Data Pipeline

1. **Data files**: Audio is stored in custom RIFF containers (`MUSC` chunk) read via `CMmio`
2. **Compression**: ADPCM (type 9) decompressed via Windows ACM, or custom codec (type 8), or uncompressed (-1)
3. **Playback modes**:
   - **Preloaded**: Entire WAV loaded to memory, played directly
   - **Cached**: Loaded on demand, freed after timeout
   - **Buffered**: Streamed via triple-buffered read-ahead thread (music tracks)
4. **MIDI**: XMIDI data loaded to memory, played via `AIL_init_sequence`

---

## Migration Strategy

### Library Choice: SDL2 + SDL_mixer

- **SDL2** (`SDL_audio.h`): Low-level audio device management
- **SDL_mixer** (`SDL_mixer.h`): Multi-channel mixing, music playback, callbacks
- Both support x86 and x64, are actively maintained, and have permissive licenses (zlib)
- SDL_mixer provides channel-based mixing that maps well to MSS32's sample handle model
- SDL_mixer supports MIDI playback (via Timidity or FluidSynth backends) -- though we recommend dropping MIDI support (see Design Decision #6 / Phase 2d)

### Key Design Decisions

1. **Keep the `CMusicPlayer` public API unchanged** -- game code only calls through this interface, so the migration is fully contained within `music.cpp` and `music.h`
2. **Replace MSS types in `music.h`** -- `HDIGDRIVER`, `HMDIDRIVER`, `HSAMPLE`, `HSEQUENCE` become SDL equivalents or custom handles
3. **Threading model changes fundamentally** -- MSS32 callbacks (`RawCallback`, `RawDblBufCallback`, `MidiCallback`) are invoked **synchronously from the game thread** via `AIL_serve()`. SDL_mixer callbacks (`Mix_ChannelFinished`, `Mix_HookMusic`) are invoked from **SDL's audio thread**. This is a critical difference:
   - The current callbacks do file I/O, `::Sleep()`, ADPCM decompression, and call back into `StartRaw()` — all unsafe from an audio thread
   - **Solution:** SDL callbacks must only set flags/enqueue events. The actual work (loading buffers, starting new sounds, file I/O) must be deferred to the game thread, likely in `YieldPlayer()` which is called every game loop tick from `mainloop.cpp:301`
4. **`AIL_serve()` calls become processing points** -- Rather than simple no-ops, the ~25 `AIL_serve()` sites (and `YieldPlayer()`) become the places where deferred work from SDL callbacks is processed on the game thread
5. **Double-buffering thread stays** -- The existing read-ahead thread (`fnMusicReadAhead`) is independent of MSS32; it reads from disk into buffers. We keep it and feed buffers to SDL_mixer instead. Its `SuspendThread`/`ResumeThread` synchronization model is fragile but functional — consider migrating to an event-based model (`CreateEvent`/`WaitForSingleObject`) during the rewrite
6. **MIDI decision**: XMIDI is a proprietary Miles format. Options:
   - (a) Convert XMIDI to standard MIDI at load time, play via SDL_mixer's MIDI backend
   - (b) Pre-convert all XMIDI files to WAV/OGG and treat all music as digital
   - (c) Drop MIDI mode entirely (it was the "low-end" fallback in 1996)
   - **Recommended: Option (c)** -- simplifies migration significantly. The game already has a full digital music mode (`wav_only`). Force this mode and remove MIDI code paths.
   - **Important:** The default constructor sets `m_iMode = MUSIC_MODE::mixed`. This must be changed to `wav_only`. The mode is also persisted to the Windows registry via `WriteProfileInt("Advanced", "MusicModeUsed", ...)` in `Open()` -- existing installs will pick up the old setting. Add migration logic or ignore the registry value.

---

## Phased Implementation Plan

### Phase 0: Prerequisites

- [ ] Add SDL2 and SDL_mixer as project dependencies
  - Download SDL2 and SDL_mixer development libraries (MSVC)
  - Add to `tools/sdl2/` (or use vcpkg/CMake FetchContent)
  - Update `CMakeLists.txt` to find and link SDL2, SDL2_mixer
  - Copy `SDL2.dll` and `SDL2_mixer.dll` to output (replacing MSS32.DLL copy step)
- [ ] Verify SDL2 builds for both x86 and x64

### Phase 1: Remove MSS32 Header Dependencies

**Files to modify:**
- `enations_latest/src/stdafx.h` -- remove `#include <mss/mssw.h>`
- `windward/wind22/include/stdafx.h` -- remove `#include <mss/mssw.h>`
- `windward/wind22/include/music.h` -- add `#include <SDL.h>` and `#include <SDL_mixer.h>`

**Replace `MEM_alloc_lock`/`MEM_free_lock` macros** -- these expand to `AIL_mem_alloc_lock`/`AIL_mem_free_lock` (MSS32 page-locked memory). Used in `music.cpp` (~13 sites) and `acmutil.cpp` (1 site). Replace with `malloc`/`free`. Page-locked memory was needed for ISA DMA in the DOS/Win3.1 era; it's unnecessary on modern Windows.

**Define replacement constants** (in `music.h` or a new `sdl_audio_compat.h`):
```cpp
// Replaces MSS32 DIG_F_* format constants
enum AudioFormat {
    AUDIO_FMT_MONO_8   = 0,  // was DIG_F_MONO_8
    AUDIO_FMT_MONO_16  = 1,  // was DIG_F_MONO_16
    AUDIO_FMT_STEREO_8 = 2,  // was DIG_F_STEREO_8
    AUDIO_FMT_STEREO_16 = 3, // was DIG_F_STEREO_16
};
```

**Replace MSS types in `music.h`:**

| Old (MSS32) | New (SDL) |
|---|---|
| `HDIGDRIVER m_hDig` | `bool m_bDigOpen` (SDL_mixer manages the device) |
| `HMDIDRIVER m_hMdi` | Remove (drop MIDI, or `Mix_Music*` for MIDI) |
| `HSEQUENCE m_hSeq` | Remove (or `Mix_Music* m_pMidiMusic`) |
| `HSAMPLE m_hSmp` (in CRawChannel) | `int m_iChannel` (SDL_mixer channel index, -1 if inactive) |

### Phase 2: Rewrite `music.cpp` Core Functions

This is the bulk of the work. Rewrite each function group:

#### 2a: Initialization & Shutdown

| Function | Changes |
|---|---|
| `CMusicPlayer()` constructor | Initialize SDL state instead of MSS handles |
| `~CMusicPlayer()` | Call `Mix_CloseAudio()`, `SDL_QuitSubSystem(SDL_INIT_AUDIO)` |
| `Open()` | `SDL_Init(SDL_INIT_AUDIO)` + `Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 2048)` + `Mix_AllocateChannels(MAX_SOUND_SAMPLES)` |
| `Close()` | `Mix_HaltChannel(-1)` + `Mix_CloseAudio()` |
| `OpenDigital()` | Simplified -- SDL_mixer handles device; just open once with best format |
| `OpenMidi()` | Remove entirely (or minimal SDL_mixer MIDI init) |

#### 2b: Sample Playback (SFX & Streamed Music)

Replace MSS sample handle model with SDL_mixer channels:

```
MSS model:                      SDL_mixer model:
HSAMPLE = AIL_allocate_sample   channel = Mix_PlayChannel(-1, chunk, loops)
AIL_init_sample(h)              (reset state in CRawChannel)
AIL_set_sample_address(h,buf,l) chunk = Mix_QuickLoad_RAW(buf, len, 0)
AIL_start_sample(h)             Mix_PlayChannel(ch, chunk, loops)
AIL_end_sample(h)               Mix_HaltChannel(ch)
AIL_sample_status(h)            Mix_Playing(ch)
```

**`StartRaw()` rewrite** -- the most critical function:
1. Set up `Mix_Chunk` from `CRawData::m_pBuf` (for preloaded/cached)
2. Set volume via `Mix_Volume(ch, vol)`
3. Set panning via `Mix_SetPanning(ch, left, right)` (convert 0-127 pan to L/R)
4. **Format conversion required**: SDL_mixer's `Mix_QuickLoad_RAW` expects data in the device's opened format. If we open at 22050Hz/16-bit/stereo (the best quality the game uses), then 8-bit 11025Hz mono SFX must be converted before playback. This means either:
   - (a) Convert at load time: when loading a `DIG_F_MONO_8` sound, upsample 11025→22050, convert unsigned 8-bit→signed 16-bit, and duplicate mono→stereo. Simple and cache-friendly but uses ~8x memory per sound
   - (b) Convert on-the-fly using `SDL_AudioStream` (SDL2's resampling API) per-channel. More complex but memory-efficient
   - (c) Open the device at 11025Hz/8-bit when in MIDI mode — but we're dropping MIDI, so always open at 22050/16/stereo and convert at load time (option a)
   - **Recommended: option (a)** — the sounds are small (SFX are preloaded/cached), the conversion is trivial, and it eliminates per-channel format tracking entirely
5. For buffered (streamed) music: use `Mix_HookMusic()` or a custom `Mix_Chunk` with periodic refilling

**Callback mapping:**

| MSS Callback | SDL_mixer Equivalent |
|---|---|
| `AIL_register_EOS_callback(h, fn)` | `Mix_ChannelFinished(fn)` (global -- dispatch via channel->CRawChannel map) |
| `AIL_register_EOB_callback(h, fn)` | No direct equivalent; use `Mix_HookMusic()` for streamed music or periodic polling |
| `AIL_register_sequence_callback(h, fn)` | `Mix_HookMusicFinished(fn)` |

**CRITICAL: Callback threading change.** The 4 current callbacks and their constraints:

| Callback | Signature | Called From | What It Does |
|---|---|---|---|
| `RawCallback` | `void WINAPI (HSAMPLE)` | Game thread via `AIL_serve()` | Spin-waits on read-ahead thread, calls `StartRaw()` (file I/O, buffer setup) |
| `RawDblBufCallback` | `void WINAPI (HSAMPLE)` | Game thread via `AIL_serve()` | Calls `LoadNextDblBuffer()` (file I/O, ADPCM decode, `::Sleep()`) |
| `MidiCallback` | `void WINAPI (HSEQUENCE)` | Game thread via `AIL_serve()` | Calls `AIL_init_sequence()`, `AIL_start_sequence()` |
| `DblBufCallBack` (CRawData) | `void (DWORD)` | N/A (declared but appears unused in active code) | Cache callback stub |

SDL_mixer's `Mix_ChannelFinished` and `Mix_HookMusic` run on **SDL's audio thread**. The current callbacks are NOT safe to run there (file I/O, Sleep, ADPCM, mutex waits). The rewrite must:
1. SDL callbacks: only set a flag (e.g., `m_bChannelFinished[ch] = true`) or enqueue an event
2. Game thread: check flags in `YieldPlayer()` (called every tick from `mainloop.cpp:301`) and do the actual work (start next sound, load next buffer, etc.)

**User data mapping:**
- MSS stores a user pointer per sample via `AIL_set_sample_user_data()`
- SDL_mixer has no per-channel user data
- Solution: maintain a `CRawChannel* g_channelMap[MAX_SOUND_SAMPLES]` array indexed by SDL_mixer channel number

#### 2c: Double-Buffered Music Streaming

The existing triple-buffer read-ahead system reads compressed audio from disk, decompresses via ADPCM, and feeds PCM to MSS via `AIL_load_sample_buffer()`.

**SDL approach:** Use `Mix_HookMusic(callback, userdata)` for the music channel:
- The callback is invoked by SDL_mixer's audio thread requesting PCM data
- Feed data from the existing `m_pDblBuf[]` buffers
- The read-ahead thread (`fnMusicReadAhead`) continues to fill buffers as before
- Replace `AIL_sample_buffer_ready()` checks with a ring-buffer ready flag

Alternatively, use an `SDL_AudioStream` to push decoded PCM and let SDL handle buffering.

**Important caveat:** `Mix_HookMusic()` bypasses SDL_mixer's normal channel system. `Mix_VolumeMusic()` does NOT affect the hook callback -- you must apply volume scaling manually inside the callback when writing PCM data. This is a difference from MSS32 where `AIL_set_digital_master_volume()` affected all samples including double-buffered ones. The alternative (`SDL_AudioStream` feeding a regular `Mix_Chunk` on a reserved channel) avoids this issue at the cost of more buffer management.

#### 2d: MIDI Music

**Recommended: Remove MIDI support entirely.**

- The game has 3 music modes: `midi_only`, `mixed`, `wav_only`
- Force `wav_only` mode -- all music plays as digital audio
- Remove: `OpenMidi()`, `StartMidiMusic()`, `StopMidiMusic()`, `MidiCallback()`, `ToExclMusic()`, `FromExclMusic()`
- Remove: `m_hMdi`, `m_hSeq`, `m_aMidi` array, `m_bKillMidi`, `m_bExclWav`, `m_bNoMidi`
- Simplify: `Open()`, `Close()`, `SetMusicVolume()`, `CheckDigVol()`

If MIDI must be preserved: convert XMIDI to standard MIDI at load time and use `Mix_LoadMUS_RW()`.

#### 2e: Volume Control

| MSS | SDL_mixer |
|---|---|
| `AIL_set_digital_master_volume(hDig, 0-127)` | `Mix_MasterVolume(0-128)` or `Mix_Volume(-1, 0-128)` |
| `AIL_set_sample_volume(hSmp, 0-127)` | `Mix_Volume(ch, 0-128)` |
| `AIL_set_sample_pan(hSmp, 0-127)` | `Mix_SetPanning(ch, 255-pan*2, pan*2)` |
| `AIL_set_XMIDI_master_volume(hMdi, 0-127)` | `Mix_VolumeMusic(0-128)` |

Volume range is nearly identical (MSS: 0-127, SDL_mixer: 0-128). Scale by `vol * 128 / 127` or just use directly.

#### 2f: Focus Handling

| Function | MSS | SDL |
|---|---|---|
| `OnActivate(FALSE)` | `AIL_digital_handle_release()` / `AIL_MIDI_handle_release()` | `Mix_Pause(-1)` / `Mix_PauseMusic()` |
| `OnActivate(TRUE)` | `AIL_digital_handle_reacquire()` / `AIL_MIDI_handle_reacquire()` | `Mix_Resume(-1)` / `Mix_ResumeMusic()` |

Note: `movie.cpp` calls `OnActivate(FALSE)` before FMV playback and `OnActivate(TRUE)` after, to avoid audio device conflicts. With SDL this should work naturally via pause/resume, but test FMV + audio interaction.

#### 2g: Utility & Query Functions

| Function | Changes |
|---|---|
| `YieldPlayer()` | Becomes the **deferred work processor** — checks flags set by SDL audio-thread callbacks and performs the actual work (start next sound, load next buffer, etc.). Called every game tick from `mainloop.cpp:301` |
| `FreeOldSounds()` | No changes needed (doesn't use MSS API, just uses `timeGetTime()`) |
| `LoadGroup()` / `UnloadGroup()` | Remove `AIL_serve()` calls; rest is unchanged |
| `GetVersion()` | Return SDL_mixer version string instead of `AIL_DLL_version()` |
| `UseDirectSound()` | Remove or always return `FALSE`. The MSS32 DirectSound detection logic (`m_hDig->pDS`, `IDirectSound_GetCaps`, `DSCAPS_EMULDRIVER` check) is replaced by SDL's own device management. `lastplnt.cpp` calls this for diagnostics — replace with SDL device info |
| `m_bUseDS` member | Remove. SDL_mixer manages its own backend (DirectSound/WASAPI/etc.) |

#### 2h: Dead Code & Legacy Cleanup

Remove during rewrite:
- All `W32s`/`iWinType` checks — Win32s (Windows 3.1 compatibility layer) hasn't existed since the 1990s. The triple-buffer vs double-buffer decision, thread creation guard, and format fallback logic based on `iWinType == W32s` can all be removed
- `NoDirectSound` registry preference — SDL handles backend selection
- `DIG_USE_WAVEOUT` preference — MSS-specific
- `SetMusicSoundVolume()` declaration in `music.h` — never implemented
- `UnloadCache()` declaration in `music.h` — declared but never implemented or called
- `EndMusicGroup()` — implemented but never called by any game code

### Phase 3: Update External Callers

**`acmutil.cpp:295`** -- uses `MEM_alloc_lock()` (MSS macro):
- Replace with `malloc()` — page-locked memory is unnecessary on modern Windows
- **Important:** This buffer's ownership is transferred to `music.cpp` via the `ResultData()`/`ReleaseBuffer()` pattern. `acmutil.cpp` allocates it with `MEM_alloc_lock`, then `ReleaseBuffer()` detaches it (sets `pbDst = NULL` without freeing). `music.cpp` stores the pointer in `CRawData::m_pBuf` and later frees it with `MEM_free_lock`. Both sides must use matching allocators (`malloc`/`free`)
- This is the **only MSS32 dependency in the ADPCM/ACM code**; the rest uses Windows ACM (`msacm32.dll`) exclusively

**`lastplnt.cpp`** (4 AIL_ call sites + 8 MSS constant references across 2 diagnostic functions):
- Lines 1493+1495, 2864+2874: `AIL_digital_configuration(theMusicPlayer._GetHDig(), &iRate, &iFmt, sBuf)`
- Lines 1498-1507, 2877-2886: `switch(iFmt)` uses MSS constants `DIG_F_MONO_8`, `DIG_F_MONO_16`, `DIG_F_STEREO_8`, `DIG_F_STEREO_16` — these will break when `mssw.h` is removed
- Replace entire diagnostic block with a new `CMusicPlayer` method: `GetDigitalConfig(rate, fmt, name)` that queries the SDL audio device spec and returns a pre-formatted string, eliminating the caller's need for any MSS constants
- Remove `_GetHDig()` and `_GetHMidi()` from public API
- Remove dead declarations from `music.h`: `SetMusicSoundVolume()` (never implemented), `UnloadCache()` (never implemented)

**`tstsnds.cpp`** (test sounds dialog):
- Uses `CMusicPlayer` public API only -- no changes needed

**All game code** -- uses `CMusicPlayer` public API only, no changes needed:
- `area.cpp` -- background sounds (terrain, construction, damage), foreground SFX (unit selection, movement)
- `main.cpp` -- volume sliders, music start, group loading
- `cutscene.cpp` -- voices, win/lose music, scene transitions
- `event.cpp` -- event voice playback
- `bmbutton.cpp` -- button click SFX
- `credits.cpp` -- credits music
- `movie.cpp` -- calls `OnActivate(FALSE/TRUE)` to pause/resume audio during FMV playback
- `newworld.cpp` -- `SoundsOff()`, `PlayExclusiveMusic()`, `PlayMusicGroup()`, `StartMidiMusic()`, `LoadGroup()`, volume
- `new_unit.cpp` -- `KillForegroundSound()`, `IncBackgroundSound()`, `DecBackgroundSound()`
- `projbase.cpp` -- `SoundsPlaying()`, `PlayForegroundSound()`
- `options.cpp` -- `SetMusicVolume()`, `SetSoundVolume()`
- `new_game.cpp` -- `PlayForegroundSound()`
- `mainloop.cpp` -- `YieldPlayer()` (called every game loop tick; becomes deferred-work processor for SDL callbacks)
- `lastplnt.cpp` -- `InitData()`, `Open()`, `GetVersion()`, `GetMode()`, `IsRunning()`, `MidiOk()`, `WavOk()`, `UseDirectSound()`

### Phase 4: Build System Updates

**`enations_latest/src/CMakeLists.txt`:**
- Remove `mss32.lib` from link dependencies (line 168)
- Remove MSS32.DLL copy command (line 218)
- Add SDL2 and SDL_mixer: `find_package(SDL2)`, `find_package(SDL2_mixer)`, link targets
- Add DLL copy for `SDL2.dll`, `SDL2_mixer.dll`

**`enations_latest/src/enations.vcxproj`:**
- Remove `mss32.lib` from `AdditionalDependencies` in all 5 build configurations (lines 150, 192, 235, 277, 320)
- Add SDL2 and SDL_mixer lib references
- Note: if the project exclusively uses CMake (which generates its own build files), this vcxproj may be legacy. Verify which build system is authoritative before editing both.

**`enations/enations.vcxproj`** (legacy, not in active build):
- Has `mss32.lib` in 4 build configurations (lines 117, 141, 159, 182)
- Has `tools\mss\` in LibraryPath for 4 configurations (lines 81, 87, 93, 99)
- Clean up if this legacy project file is retained

**`windward/wind22/CMakeLists.txt`:**
- Add SDL2 include paths (wind22 is a static lib, so just headers needed)

**Include paths:**
- Remove `tools/mss` from include directories
- Add SDL2 include path

### Phase 5: Remove MSS32 Artifacts

- Delete or archive: `tools/mss/` directory (DLLs, LIBs, headers)
- Remove MSS32 references from setup scripts (`patch_mss.wse`, `_setup.WSE`, etc.)
- Remove `MSSDLLNAME` and `MSSBreakPoint()` macro references

### Phase 6: Testing & Validation

- [ ] **Build**: Verify clean compile for x86 (and x64 when other blockers are resolved)
- [ ] **Basic playback**: SFX play when clicking buttons, selecting units, issuing orders
- [ ] **Background sounds**: Ambient terrain sounds (trees, river, ocean) loop correctly
- [ ] **Music**: Digital music tracks play during gameplay, rotate between tracks
- [ ] **Exclusive music**: Credits, win/lose music plays and returns to gameplay music
- [ ] **Volume controls**: Music and SFX sliders work independently
- [ ] **Focus**: Alt-tab pauses audio, returning resumes
- [ ] **Group loading**: Sound groups load/unload without leaks
- [ ] **Cache eviction**: Cached sounds are freed after timeout
- [ ] **Cutscenes**: Voice and music play during scenario intros
- [ ] **Diagnostics**: About dialog shows audio device info correctly
- [ ] **Combat sounds**: Weapon fire and explosion sounds play with correct panning/volume
- [ ] **Voice alerts**: System voices ("out of food", "under attack") trigger correctly from events
- [ ] **Vehicle sounds**: Idle/run/go sounds transition correctly as units move
- [ ] **Construction sounds**: const1/const2/const3 play for correct build stages
- [ ] **Panning**: Sounds pan left/right based on unit position relative to viewport center
- [ ] **Priority system**: High-priority sounds (terrain) properly override low-priority (voices) when channels are full
- [ ] **FMV playback**: Movie.cpp pause/resume cycle doesn't break audio state
- [ ] **No device**: Game starts gracefully with no audio device (error dialog, continues without sound)
- [ ] **ADPCM compressed sounds**: Compressed SFX and streamed music decompress and play correctly

---

## Risk Assessment

| Risk | Severity | Mitigation |
|---|---|---|
| SDL_mixer lacks per-channel format/rate | Medium | `Mix_QuickLoad_RAW` requires data in the device's opened format. Convert all 8-bit/11025Hz/mono sounds to 16-bit/22050Hz/stereo at load time (~8x memory per sound, but sounds are small). Eliminates per-channel format tracking |
| Double-buffer streaming needs redesign | Medium | Use `Mix_HookMusic()` custom callback; existing read-ahead thread provides data |
| SDL_mixer's `Mix_ChannelFinished` is global, not per-channel | Low | Use channel->CRawChannel mapping array to dispatch |
| XMIDI format not supported by SDL | Low | Drop MIDI entirely; force digital music mode |
| ADPCM decompression uses Windows ACM | Low | ACM code is independent of MSS32 **except for one `MEM_alloc_lock()` call in acmutil.cpp:295**. Replace with `malloc()`. Rest of ACM code uses `msacm32.dll` (Windows built-in). (Future: replace with SDL-native decode or bundled codec) |
| `AIL_serve()` sites need repurposing | Medium | These ~25 call sites were synchronous callback dispatch points. They become deferred-work processing points. Some tight-loop patterns (e.g., `AIL_end_sample` + `AIL_serve` + check status) may need restructuring since SDL's `Mix_HaltChannel` takes effect asynchronously |
| **Threading model change** | **High** | MSS32 callbacks run synchronously on game thread (via `AIL_serve()`). SDL_mixer callbacks run on SDL's audio thread. Current callbacks do file I/O, `::Sleep()`, ADPCM decompression, and call `StartRaw()` — all unsafe from an audio thread. Must redesign: SDL callbacks set flags only, game thread processes deferred work in `YieldPlayer()`/`AIL_serve()` sites |
| `music.cpp` accesses MSS internal struct (`m_hDig->pDS`) | Low | This DirectSound emulation-detection code is removed entirely; SDL manages its own backend |
| `waveOutGetNumDevs()` / `midiOutGetNumDevs()` used for device detection | Low | Replace with SDL audio device enumeration (`SDL_GetNumAudioDevices()`) or remove (SDL fails gracefully if no device) |
| `timeGetTime()` used for cache timing | None | `timeGetTime()` is a Windows multimedia API independent of MSS32; keep as-is |
| Registry settings (`NoDirectSound`, `MusicModeUsed`, `MusicThread`) | Low | Remove MSS-specific settings; keep or migrate user-facing ones (volume, music mode) |

---

## Estimated Effort

| Phase | Effort | Notes |
|---|---|---|
| Phase 0: SDL setup | Small | Download libs, update CMake |
| Phase 1: Header cleanup | Small | Mechanical replacement |
| Phase 2: music.cpp rewrite | **Large** | ~1900 lines, ~93 AIL_ calls. Core of the migration |
| Phase 3: External callers | Small | 4 call sites in lastplnt.cpp + 1 in acmutil.cpp |
| Phase 4: Build system | Small | CMake changes |
| Phase 5: Cleanup | Small | Delete old files |
| Phase 6: Testing | Medium | Manual testing of all audio scenarios |

**Phase 2 is the critical path.** The hardest sub-tasks in order: (1) the callback threading redesign (2b) — converting synchronous game-thread callbacks to async flag-setting + deferred processing is an architectural change that touches every playback path; (2) the streaming/double-buffer rewrite (2c) — feeding the `Mix_HookMusic` callback from the existing triple-buffer system while handling volume manually; (3) the `AIL_serve()` repurposing across ~25 sites.

---

## File Change Summary

| File | Action |
|---|---|
| `windward/wind22/src/music.cpp` | **Rewrite** -- replace all AIL_ calls and `MEM_alloc_lock`/`MEM_free_lock` with SDL_mixer equivalents and `malloc`/`free` |
| `windward/wind22/include/music.h` | **Modify** -- replace MSS types, remove MIDI members, remove dead `SetMusicSoundVolume` decl, add SDL includes |
| `windward/wind22/src/acmutil.cpp` | **Modify** -- replace 1 `MEM_alloc_lock()` call (line 295) with `malloc()` |
| `enations_latest/src/stdafx.h` | **Modify** -- remove `#include <mss/mssw.h>` |
| `windward/wind22/include/stdafx.h` | **Modify** -- remove `#include <mss/mssw.h>` |
| `enations_latest/src/lastplnt.cpp` | **Modify** -- replace 4 MSS32 call sites + 8 MSS constant refs (`DIG_F_*` in switch statements) with new `GetDigitalConfig()` method |
| `enations_latest/src/CMakeLists.txt` | **Modify** -- remove mss32.lib, add SDL2/SDL_mixer |
| `enations_latest/src/enations.vcxproj` | **Modify** -- remove mss32.lib from 5 build configurations (if vcxproj is still used) |
| `windward/wind22/CMakeLists.txt` | **Modify** -- add SDL2 include paths |
| `tools/mss/*` | **Delete** (after migration complete) |
| `enations_latest/setup/*.wse` | **Modify** -- remove MSS32.DLL references, add SDL DLLs |
