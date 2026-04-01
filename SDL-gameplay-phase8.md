# Phase 8: Building/Vehicle Lists and Chat/Email

## Goal
Side panel windows for unit management and multiplayer communication.

## Depends On
- Phase 1 (compositor)
- Phase 4 (toolbar buttons open these)

## What to Build

### SDL2BuildingListPanel
- Scrollable list of all player buildings
- Each item: sprite icon, name, status
- Click selects building and centers area map on it
- Double-click opens building details
- Real-time updates as buildings are built/destroyed
- Uses SDL2Listbox with custom item rendering (sprite + text)

### SDL2VehicleListPanel
- Same pattern as building list but for vehicles
- Scrollable list with sprite icons
- Click to select and center map

### SDL2ChatPanel (CWndComm replacement)
Most complex panel — has sub-views:
- 9 communication buttons (Read, Send, Reply, Forward, Delete, Refuse, Chat, Global Chat, Player Status)
- Email list (CEMailLB) with columns (Name, Subject)
- Read pane, compose pane, chat pane
- Message notification handling
- Network message processing

### Window Management
These panels need:
- Drag-to-move via title bar
- Resize handles (original MFC windows were WS_THICKFRAME)
- Close button
- Toggle visibility via toolbar buttons

## Reuse
- Game data from theBuildingMap, theVehicleMap
- CWndListUnits::AddToList() / RemoveFromList() logic
- Email/chat message data structures

## Testable Milestone
Building list shows all player buildings. Click one to center map on it. Vehicle list works similarly. In multiplayer, chat window sends/receives messages.

## Complexity: MEDIUM
