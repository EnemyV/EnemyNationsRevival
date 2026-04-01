# Phase 2: Creating World Progress Dialog

## Goal
Replace CDlgCreateStatus (MFC "Creating world..." progress dialog) with an SDL2 version.

## Depends On
- Phase 1 (SDL2 compositor visible during world creation)

## What to Build

### SDL2CreateStatusDialog
Extends SDL2Dialog. Shows:
- Text message ("Creating world...", "Loading terrain...", etc.)
- Progress bar (0-100%)
- Cancel button

### Hook Points
In new_game.cpp, CCreateBase calls:
- CreateDlgStatus() — create the SDL2 dialog
- GetDlgStatus()->SetMsg(IDS_xxx) — update message text
- GetDlgStatus()->SetPer(n) — update progress percentage
- ShowDlgStatus() / HideDlgStatus() — visibility

### Yield Integration
SetPer() calls yield to keep UI responsive during world generation. The SDL2 version must:
- Pump SDL events inside yield
- Re-render the dialog each yield
- Check for cancel button press

## Reuse
- Existing SDL2Dialog modal pattern
- SDL2Label, SDL2Button widgets

## Testable Milestone
Click "Create Single Player Game", pick settings and race; "Creating world..." shows as an SDL2 dialog with updating progress bar. Cancel aborts creation and returns to menu.

## Complexity: SMALL
