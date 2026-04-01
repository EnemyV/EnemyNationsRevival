# Phase 4: CWndBar Toolbar as SDL2Panel

## Goal
Bottom toolbar with 8 navigation buttons, 4 resource status bars, clock, and 2 status text lines.

## Depends On
- Phase 1 (compositor)

## What to Build

### SDL2ToolbarPanel (extends SDL2Panel)
Positioned at bottom of screen; height = TOOLBAR_HT (66px = BAR_BTN_HT 38 + BAR_TEXT_HT 28).

### 8 Navigation Buttons
Using existing CBmBtnData art (3-state strips: normal/pressed/disabled). Convert CDIBs to SDL surfaces.

| Button | Command | Action |
|--------|---------|--------|
| Area | GotoArea | Show/focus area map |
| World | GotoWorld | Show/focus minimap |
| Chat | GotoChat | Open chat window |
| Vehicles | GotoVehicles | Open vehicle list |
| Buildings | GotoBuildings | Open building list |
| Relations | GotoRelations | Open diplomacy dialog |
| Science | GotoScience | Open research dialog |
| File | GotoFile | Open save/load dialog |

### 4 Resource Status Bars
Gas, Power, People, Food — each shows:
- Icon sprite (animated for critical levels)
- Production vs consumption bar
- Numeric values
Uses CStatData/CStatInst with existing icon sprites from theBitmaps/theIcons.

### Clock Display
Game elapsed time formatted by CWndBar::UpdateTime(). Render with TTF.

### 2 Status Text Lines
CWndStatLine shows context help text and status messages. Render as SDL2 text.

### Sound/Notification Triggers
Low-resource alarms, button click sounds — preserve existing trigger points from mainloop.cpp update functions (UpdateGas, UpdatePower, etc.).

## Reuse
- Button art from theBitmaps, theBmBtnData
- Status bar update logic from mainloop.cpp (already computes values)
- Sound trigger code (just call theMusicPlayer directly)

## Testable Milestone
Toolbar appears at screen bottom. Resource bars update in real-time during gameplay. Clicking buttons opens corresponding windows/dialogs. Help text shows on hover. Low-resource alarms trigger.

## Complexity: MEDIUM
