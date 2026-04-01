# Phase 6: Area Action Buttons and Unit Status

## Goal
17 action buttons and unit status bar overlaid on the area map viewport.

## Depends On
- Phase 3 (area map panel exists)

## What to Build

### Action Button Strip (CWndAreaStatic replacement)
17 buttons rendered as a sub-region of SDL2AreaPanel. Using existing CBmBtnData art.

Button list (from area.h NUM_AREA_BUTTONS = 17):
- Zoom in / Zoom out
- Rotate clockwise / counterclockwise
- Stop unit
- Resume / Continue
- Destroy unit
- Build structure (opens build dialog)
- Build road
- Repair
- Route vehicle
- Load/Unload
- Center on unit
- Next unit
- Group select
- etc.

Each button has enable/disable/show/hide state managed by CWndAreaStatic::EnableButton() / ShowButton().

### Unit Status Bar (CWndUnitStat replacement)
Shows selected unit info:
- Unit name and type
- Health bar (green/yellow/red)
- Status text (idle, moving, attacking, building, etc.)
- Resource production/consumption (for buildings)

Rendered as text + icon overlay within the area panel.

### Connect Commands
Button clicks call existing CWndArea command handlers:
- ZoomIn/ZoomOut
- TurnClock/TurnCounter (rotate view)
- StopUnit, DestroyUnit
- BuildUnit (enters build_ready mode)
- RouteUnit (enters veh_route mode)
- etc.

### Tooltip (CWndInfo)
Hover-over tooltip showing unit details. Renders as a floating overlay on the area panel. Position follows mouse cursor with offset. Content from CUnit::GetStatusText().

## Reuse
- Button art from theBmBtnData
- All game logic for button state (CWndArea::SetButtonState, SetMouseState)
- Unit status formatting code

## Testable Milestone
Buttons appear over the map. Click zoom to zoom. Select a unit and see its status. Click build to enter build mode. Click route to enter routing mode. Tooltips show on hover.

## Complexity: MEDIUM
