# Phase 10: MFC Window Removal, Game State Transitions, and Cleanup

## Goal
Complete removal of MFC windows from gameplay. Handle all game state transitions. Clean build with zero MFC windows during gameplay.

## Depends On
- All previous phases

## What to Build

### MFC Window Removal
- Remove CWndMain OS window creation (SDL2 window becomes the sole display)
- Replace CConquerApp::Run() main loop or adapt it to drive SDL2Compositor
- Remove CreateEx() calls from all gameplay CWnd-derived classes
- Strip MFC message maps (ON_WM_PAINT, ON_WM_LBUTTONDOWN, etc.) from gameplay windows
- Keep the data/logic classes (CWndArea's mode state, CWndBar's button state, CWndWorld's rendering logic)
- Remove CDIBWnd::Update() GDI blitting; all output goes through panel surfaces
- Remove CBT hook workaround in GameWindow::InitializeSDL()

### Game State Transitions
Currently missing from the plan:

1. **Victory/Defeat** — CWndCutScene plays end-game cutscenes, then returns to main menu. Convert to SDL2VideoPlayer or a simple animation overlay.

2. **Return to Menu** — From CDlgFile "Exit" button or after game over. Must tear down all gameplay panels, restore main menu. theApp.CreateMain() / theApp.CloseWorld() flow.

3. **Network Disconnect** — Game shows CDlgPause with network status. Must handle gracefully without MFC.

4. **Scenario Cutscenes** — CCutScene plays before/after scenarios. May use AVI or custom animation.

### Replace theAnimList Iteration
_RenderScreens() calls ReRender()/Draw() on CWndAnim instances via theAnimList. Replace with SDL2Compositor-driven rendering:
1. Call CAnimAtr::ReRender() for each active viewport's animation attributes
2. CAnimAtr::Draw() writes to its CDIBWnd buffer
3. SDL2Compositor reads each CDIBWnd buffer and composites to panels
4. Compositor calls SDL_UpdateWindowSurface() once

### Keyboard Shortcuts / Accelerators
The MFC accelerator table (IDA_CLOSE_APP, IDA_NEXT, IDA_PREV, IDA_PAUSE) must be replicated in SDL2 event handling. Map to SDL_KEYDOWN events in the compositor or game window event handler.

### Sound/Notification Preservation
Verify all sound triggers still fire:
- Unit selection sounds
- Attack/damage sounds
- Building complete notifications
- Low-resource alarms
- Button click sounds
These are called from game logic code (not MFC message handlers) so should work, but verify.

## Testable Milestone
Game runs with zero MFC windows created during gameplay. Full gameplay loop works: start game → build base → produce units → fight → win/lose → return to menu. All via SDL2.

## Complexity: MEDIUM-LARGE
