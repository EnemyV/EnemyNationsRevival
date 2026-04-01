# SDL2 Gameplay Migration — Overview

## Execution Order (optimized for fastest playable build)

| Order | Phase | Description | Complexity | Playability |
|-------|-------|-------------|------------|-------------|
| 1 | Phase 1 | Backplate + Compositor framework | LARGE | Foundation only |
| 2 | Phase 2 | Creating World progress dialog | SMALL | Can start a game |
| 3 | Phase 3 | Area map viewport + input (12 mouse modes) | LARGE | Can see and interact with map |
| 4 | Phase 4 | Toolbar (8 buttons, resource bars, clock) | MEDIUM | Can see resources; open dialogs |
| 5 | Phase 5 | Minimap (radar + 4 toggles + click-to-nav) | MEDIUM | Can navigate world |
| 6 | Phase 6 | Area action buttons + unit status | MEDIUM | Can zoom, rotate, use build mode |
| 7 | Phase 7a | Build Structure dialog | MEDIUM | **Can build buildings** |
| 8 | Phase 7b | Build Transport dialog | MEDIUM | **Can build vehicles** |
| 9 | Phase 7c | File dialog (save/load) | SMALL | Can save/load |
| 10 | Phase 7d | Research dialog | MEDIUM | Can research technology |
| 11 | Phase 7e-f | Relations, Pause, other dialogs | SMALL | Full dialog coverage |
| 12 | Phase 8 | Building/Vehicle lists + Chat | MEDIUM | Unit management; multiplayer chat |
| 13 | Phase 9 | Multiple map windows + routing | MEDIUM-LARGE | Advanced features |
| 14 | Phase 10 | MFC removal + state transitions + cleanup | MEDIUM-LARGE | Pure SDL2 |

## First Playable Build
After phases 1-7a (orders 1-7), the game is playable:
- Start a game (menu + world creation)
- See the map, scroll, zoom, rotate
- See resource levels in toolbar
- Navigate via minimap
- Select and move units
- Build structures
- Basic gameplay loop works

## Architecture

### Single SDL2 Window
One borderless fullscreen window. All rendering composited to its surface.

### Panel System
Each game "window" becomes an SDL2Panel — a rectangular region with its own surface buffer. The SDL2Compositor manages z-order and dirty-rect compositing.

### Rendering Bridge
Game logic still renders to CDIBWnd buffers via CAnimAtr. RenderingAdapter copies each CDIBWnd buffer to its panel's SDL surface. No game rendering code changes needed.

### Input Flow
SDL events → SDL2Compositor hit-tests panels → route to active panel → translate to game coordinates → call existing CWndArea/CWndWorld methods directly.

### The "Backplate"
Just like the original CWndMain, the SDL2 window renders a tiled wallpaper texture behind all panels. This persists across all game states (menu, creation, gameplay).

## Key Architectural Decisions
1. Stay with SDL_GetWindowSurface(); never use SDL_Renderer (they conflict)
2. Keep all game logic in existing classes; only redirect input/output
3. CWndArea/CWndWorld remain as data objects; they just don't create OS windows
4. Global rendering state (xiZoom, xiDir) save/restored per viewport render pass
5. Modal dialogs block the game loop (same as MFC); non-modal dialogs start as modal, convert later if needed
