# Phase 3: CWndArea Map Viewport as SDL2Panel

## Goal
The main gameplay map renders into an SDL2 panel with full mouse/keyboard input, replacing the MFC CWndArea window.

## Depends On
- Phase 1 (compositor and panel system)

## What to Build

### SDL2AreaPanel (extends SDL2Panel)
- Receives CAnimAtr DIB output via RenderingAdapter
- Positioned where CWndArea would be (using theApp.m_iCol1, m_iRow3 layout values)
- CWndArea still exists as a data/logic object (mode state, unit selection list, CAnimAtr) but does NOT create an OS window

### Input Translation (12 Mouse Modes)
CWndArea has these modes, each with completely different mouse behavior:
- normal — click to select units
- normal_select — drag-select rectangle
- build_ready — cursor shows building placement ghost
- build_loc — click to place building
- ask_tile — pick a hex tile
- rocket_ready / rocket_pos / rocket_wait — initial rocket placement
- veh_route — vehicle routing mode
- road_begin / road_set — road construction
- repair_bldg — click to repair

For each mode, translate SDL_Event to the equivalent CWndArea method call:
- SDL_MOUSEBUTTONDOWN → CWndArea::OnLButtonDown(flags, point) / OnRButtonDown(flags, point)
- SDL_MOUSEMOTION → CWndArea::OnMouseMove(flags, point)
- SDL_KEYDOWN → CWndArea::OnKeyDown(nChar, nRepCnt, nFlags)
- SDL_MOUSEWHEEL → zoom in/out

Coordinate conversion: SDL screen coords → panel-local coords → game hex coords via existing CAnimAtr::WindowToWorld() / WorldToHex().

### Cursor Management
CWndArea sets different cursors per mode. Map to SDL2:
- Default/select → SDL_SYSTEM_CURSOR_ARROW
- Goto → custom cursor from sprite data
- Attack → custom cursor from sprite data
- Build → custom cursor from sprite data
- Wait → SDL_SYSTEM_CURSOR_WAIT
Use SDL_CreateCursor() or SDL_CreateColorCursor() from existing cursor CDIBs.

### Edge Scrolling
When mouse is near screen edges, scroll the map. Replicate CWndArea's existing edge scroll logic.

### Right-Click Drag
Right-click-drag scrolls/pans the map. Replicate existing RMB scroll behavior.

### CAnimAtr::m_pwnd Bypass
CAnimAtr has a CWnd* m_pwnd used for GetDC() / RectVisible() culling. With no MFC window, either:
- Set m_pwnd to NULL and skip RectVisible() (the dirty rect system handles culling anyway)
- Create a dummy CWnd that always returns TRUE for RectVisible()

### Prevent MFC Window Creation
In CWndArea::Create(), guard with a flag so CreateEx() is skipped. The CWndArea object still exists for game logic but has no OS window.

## Key Risks
- The 12 mouse modes are deeply embedded in CWndArea's message handlers; need to call them directly with correct parameters
- CAnimAtr's global state (xiZoom, xiDir, xpdibwnd) must be properly save/restored per viewport
- GetDC() bypass could affect rendering if RectVisible() culling was load-bearing (test carefully)

## Reuse
- ALL of CAnimAtr, terrain rendering, sprite rendering, coordinate math — untouched
- CWndArea's mode state machine and command logic — untouched
- Only the output destination and input source change

## Testable Milestone
Game map renders in SDL2 window at correct position. Click to select units. Right-drag to scroll. Zoom in/out works. Enter build mode and place a building (building placement ghost visible). Edge scrolling works.

## Complexity: LARGE
