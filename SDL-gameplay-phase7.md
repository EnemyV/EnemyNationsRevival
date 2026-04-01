# Phase 7: In-Game Dialogs Migration

## Goal
All gameplay dialogs converted to SDL2. Required for a playable build.

## Depends On
- Phase 1 (compositor)
- Phase 4 (toolbar buttons open these)
- Phase 6 (build button enters build mode)

## Sub-Phases

### 7a: CDlgBuildStructure (building construction) — MEDIUM; PRIORITY
Required for gameplay — without this, player can't build anything.

- 6 category buttons (civilian, military, industrial, etc.)
- 6 building buttons per category with sprite preview thumbnails
- Building description text
- Cost display: have vs need for each resource
- Build button (enters build_loc mode on area map)
- Need to render building sprite previews from CDIB data using CreateSurfaceFromDIB()

### 7b: CDlgBuildTransport (vehicle factory) — MEDIUM; PRIORITY
Required to build vehicles.

- Vehicle selection buttons with sprite previews
- Quantity spinner
- Build time and resource requirements
- Build button

### 7c: CDlgFile (save/load/options) — SMALL
- Music volume slider
- Sound volume slider
- Game speed slider
- Save, Load, Help, Exit, Minimize buttons
- Version and License info buttons

### 7d: CDlgResearch (tech tree) — MEDIUM
- Scrollable list of research items
- Prerequisites display
- Start/stop research button
- Progress indicator
- CDlgDiscover notification popup when research completes

### 7e: CDlgRelations (diplomacy) — SMALL
- Player list
- Relationship radio buttons (War/Neutral/Peace/Alliance)
- Give resources button
- Player status info

### 7f: Other Dialogs — SMALL each
- CDlgPause — pause notification with status text
- CDlgLoadTruck — cargo loading interface
- CDlgChatAll — multiplayer chat window
- CDlgSaveMsg — save progress notification

## Non-Modal Dialog Handling
CDlgResearch, CDlgRelations, CDlgFile are MODELESS in the original — they stay open while gameplay continues. The current SDL2Dialog::DoModal() blocks the game loop.

Options:
1. Make them modal (simplest; slight UX change from original)
2. Integrate as persistent SDL2Panels that render each frame alongside gameplay (more work but faithful to original)

Recommendation: Start with modal for all; convert research and relations to non-modal panels later if needed.

## Reuse
- SDL2Dialog, SDL2Widget hierarchy
- CreateSurfaceFromDIB() for sprite rendering in dialogs
- All game logic (building data, research data, player relations) untouched

## Testable Milestone (per sub-phase)
Each dialog opens, displays correct data, and actions work:
- Build Structure: select a building, place it on map
- Build Transport: queue vehicle production
- Research: start researching a technology
- Save game: game actually saves to file
- Relations: change diplomacy status with AI

## Complexity: LARGE (total); individual sub-phases are SMALL-MEDIUM
