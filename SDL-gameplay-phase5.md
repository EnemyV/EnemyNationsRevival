# Phase 5: CWndWorld Minimap as SDL2Panel

## Goal
Radar/minimap showing full world with toggle buttons and click-to-navigate.

## Depends On
- Phase 1 (compositor)
- Phase 3 (area map link for click-to-navigate)

## What to Build

### SDL2MinimapPanel (extends SDL2Panel)
Positioned at its saved window placement location.

### Minimap Rendering
CWndWorld::ReRender() already renders to its CDIBWnd buffer (m_dibwnd). Redirect CDIBWnd::Update() to blit to the SDL panel surface instead of the MFC window.

Rendering layers:
- Base terrain map (m_pdibBase)
- Resource overlay (colored pixels per resource type)
- Building markers (player colors)
- Unit positions (radar dots)
- Visibility overlay (fog of war)
- Viewport rectangle (showing area map's visible region)

### 4 Toggle Buttons
Bitmap buttons matching CWndWorld's existing art:
1. Resources display toggle
2. Visible units toggle
3. My units display
4. Other units display

### Click Navigation
- Left-click centers the area map on clicked location
- Right-click-drag to scroll the minimap view
- Coordinate conversion from minimap pixel → world hex coords

### Viewport Rectangle
Draw the area map's visible region as a rectangle overlay on the minimap. Updates as the area map scrolls/zooms.

## Reuse
- All minimap rendering code (CWndWorld::ReRender, _NewDir, _NewMode, _NewLocation)
- Sprite/CDIB rendering untouched
- Only the output target changes

## Testable Milestone
Minimap shows terrain, units, resources. Click to jump area map to location. Toggle buttons work. Viewport rectangle tracks area map position.

## Complexity: MEDIUM
