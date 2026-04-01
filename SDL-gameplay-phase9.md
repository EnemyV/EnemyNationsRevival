# Phase 9: Multiple Map Windows and Vehicle Routing

## Goal
Support multiple simultaneous map viewports (unlocked via research) and the vehicle routing interface.

## Depends On
- Phase 3 (single area panel working)
- Phase 6 (action buttons)

## What to Build

### Multiple SDL2AreaPanels
- CAreaList already manages multiple CWndArea* instances
- Each panel has its own CAnimAtr with independent position, zoom, direction
- The compositor manages multiple game-rendering panels
- Global state (xiZoom, xiDir, xpdibwnd) must be save/restored around each panel's render pass — the original code already does this per CWndArea

### CWndRoute Panel
Vehicle routing overlay:
- 7 buttons: Waypoint, Unload, Load, Goto, Delete, Start, Auto
- Scrollable waypoint list
- Appears within/alongside the area panel when routing a vehicle
- Waypoints render as markers on the map

### Window Management
- Create/close secondary area panels
- Tab/click to switch active panel
- Each panel has its own minimap viewport rectangle
- CWndWorld tracks which area panel it's linked to (m_pWndArea)

## Key Risks
- Global rendering state must be properly scoped per viewport; race conditions possible if not careful
- Performance impact of rendering multiple viewports (each is a full DIB render pass)

## Testable Milestone
Open a second map view showing a different location. Route a vehicle with waypoints. Switch between viewports.

## Complexity: MEDIUM-LARGE
