# Enemy Nations Rendering System Investigation

## Overview

Enemy Nations uses a multi-layered rendering architecture built on the Windward Studios Wind22 library. The rendering system is primarily software-based, drawing to DIB (Device Independent Bitmap) surfaces and using the WinG API for display.

---

## Rendering Architecture

### High-Level Pipeline

The main rendering flow is controlled by:
- **CConquerApp::Run()** → Main application loop in Mainloop.cpp
- **GraphicsEnginePump()** → Graphics engine pump called each frame
- **CWndArea::ReRender()** → Area map rendering (detailed game area)
- **CWndWorld::ReRender()** → World/radar map rendering (overview)
- **Draw()** → Actual screen blit of rendered buffers

### Key Classes

1. **CAnimAtr** - Animation attributes, manages animation frames and rendering
2. **CDIBWnd** - DIB Window wrapper using WinG
3. **CDrawParms** - Drawing parameters passed to sprite rendering
4. **CSpriteDIB** - Sprite drawing handler
5. **CGameMap** / **CHex** - Map and hex tile system
6. **CWndArea** - Main game area window (where gameplay occurs)
7. **CWndWorld** - World/radar map window (overview map)

---

## Rendering Layers

### Layer 1: Terrain Rendering

**Files:** terrain.cpp, terrain.h, sprite.cpp

**What Gets Rendered:**
- Hexagonal terrain tiles (plains, forest, desert, ocean, mountain, etc.)
- 4 zoom levels (NUM_ZOOM_LEVELS = 4)
- Altitude shading for elevation visualization

**How It Works:**
1. CHex::Draw() renders individual hex tiles
2. CSpriteDIB::TerrainDraw() handles the actual sprite drawing
3. Terrain sprites use **shading** (N_SHADES = 8 levels) based on altitude
4. TerrainDrawQuad() and TerrainDrawQuadVert() handle quad-based terrain drawing with feathering between neighbors

**Sprite Types for Terrain:**
- Stored in CTerrain class (CSpriteStore<CTerrainSprite>)
- Multiple zoom levels for each terrain type
- Shade variants for altitude visualization
- Feathering applied at terrain boundaries

**Issues Found:**
- None specific to terrain rendering in code review

### Layer 2: Buildings (Structures)

**Files:** building.h, unit.cpp, sprite.cpp

**What Gets Rendered:**
- Structures (command centers, factories, barracks, etc.)
- Build stage progression (4 stages: NUM_BLDG_STAGES = 4)
- Damage states (4 levels: NUM_BLDG_DAMAGE = 4)
- Building direction (4 directions: NUM_BLDG_DIRECTIONS = 4)
- Building layers (2 layers: NUM_BLDG_LAYERS = 2)

**How It Works:**
1. CBuilding::Draw() renders a building at a specific hex coordinate
2. CBuilding::DrawFoundation() draws underlying terrain and foundation
3. Buildings stored in CStructureSprite sprites
4. Damage display uses CDamageDisplay class with smoke/flame effects
5. CFlameSpot renders damage smoke and flames

**Sprite Attributes:**
- CSpriteDIB::StructureDraw() handles rendering to window
- GetDrawDir() calculates direction based on current rotation (xiDir)
- Buildings use 4 directional variants

**Issues Found:**
- **BUGBUG at area.cpp:817** - Comment: "is there no way to delete a cursor?" - Cursor cleanup may be incomplete

### Layer 3: Vehicles (Mobile Units)

**Files:** vehicle.h, vehicle.cpp, unit.cpp, sprite.cpp

**What Gets Rendered:**
- Tanks, trucks, scouts, artillery, ships, etc.
- 8 directions (NUM_VEHICLE_DIRECTIONS = 8)
- 3 tilt levels for movement animation (NUM_VEHICLE_TILTS = 3)
- 4 damage states (NUM_VEHICLE_DAMAGE = 4)
- Sprites change orientation dynamically

**How It Works:**
1. CVehicle class stores vehicle state and rendering info
2. Vehicles use CSpriteDIB::VehicleDraw() for rendering
3. **Rotation handled via CQuadDrawParms** - vehicles are drawn as rotated quads
4. VehicleGetWindowVertices() calculates quad corners with rotation
5. 8-directional sprites based on movement heading

**Sprite Transformation:**
- Uses rotation angle (byRotAngle in CQuadDrawParms)
- Sin/Cos lookup tables (g_afixSin[], g_afixCos[256])
- Quad-based transformation for smooth rotation appearance

**Issues Found:**
- None specific to vehicle rendering

### Layer 4: Bridges

**Files:** bridge.h, bridge.cpp

**What Gets Rendered:**
- Bridge segments spanning between hexes
- Build progression (0-100%)
- Direction (Y direction or X direction)

**How It Works:**
1. CBridge manages multiple CBridgeUnit objects (one per hex)
2. CBridgeUnit::Draw() renders each segment
3. Bridges are CStructureSprite type sprites
4. Two-piece bridges (IsTwoPiece()) render differently than single-piece
5. Build percentage affects visual appearance

**Issues Found:**
- None specific to bridge rendering

### Layer 5: Roads

**Files:** terrain.cpp (roads are terrain features), area.cpp

**What Gets Rendered:**
- Roads overlay on terrain hexes
- Road types based on connectivity (straight, T-junctions, crosses, L-corners)
- 12 road facing types (ROAD_FACING enum in terrain.h)

**How It Works:**
1. CHex tracks road connectivity with ROAD_FACING values
2. Road sprites change based on neighboring road connections
3. ChangeToRoad() updates road visuals when new roads are built
4. Roads are terrain sprites with different variants

**Issues Found:**
- None specific to road rendering

### Layer 6: UI Interface Elements

**Files:** area.cpp, world.cpp, toolbar.cpp

**What Gets Rendered:**
- Buttons (4 world buttons, 17 area buttons)
- Status bars and text
- Icons for resources and units
- Damage indicators
- Selection highlighting

**How It Works:**
1. CWndAreaStatic handles button rendering
2. CWndUnitStat renders unit status information
3. Status display at bottom of area window
4. Resources and unit counts shown in toolbar

**Issues Found:**
- **TODO at area.cpp:4780-4782** - Control color attributes not implemented (cosmetic, not critical)

### Layer 7: Special Effects

**Files:** unit.cpp

**What Gets Rendered:**
- Explosions with smoke
- Flame effects for damaged units
- CDamageDisplay manages flame spots

**How It Works:**
1. CDamageDisplay::Draw() renders smoke/flame sprites
2. CFlameSpot handles individual flame/smoke sprite rendering
3. Multiple flame spots per unit based on damage percentage
4. Smoke starts at SMOKE_START_PERCENT = 20% damage
5. Flames start at FLAME_START_PERCENT = 50% damage

**Issues Found:**
- None specific to effects rendering

### Layer 8: World Map / Radar View

**Files:** world.cpp

**What Gets Rendered:**
- Minimap showing entire game world at low resolution
- Resource deposits (colored pixels per type)
- Building locations (player colors)
- Unit positions (radar-only when command center exists)
- Visibility information (when visibility mode enabled)

**How It Works:**
1. CWndWorld::_NewDir() creates base terrain/resources/buildings bitmap
2. Each hex mapped to single pixel or small group of pixels
3. Colors represent terrain type, resources, building ownership
4. Resources shown in blinking animation (m_iResOn cycles 0-7)
5. Vehicles overlay rendered in ReRender() on top of static layers

**Rendering Order in _NewDir():**
1. Check if buildings visible - render building owner color
2. Else check if resources visible - render resource color
3. Else render terrain color

**Rendering Order in ReRender():**
1. Blit base map (pdibBase)
2. Overlay current units/vehicles
3. Overlay visibility if enabled
4. Overlay radar if command center present

**Issues Found:**
- None critical to world map rendering

---

## Rendering Pipeline Details

### Update Cycle (CWndArea)

The area rendering follows a dirty-rect update system:

1. **ReRender()** Called each frame
2. **Render()** processes dirty rectangles in animation list
3. Updates only affected regions for performance

**Key Update Modes:**
- `UPDATE_MODE::INVALIDATE` - Mark regions for redrawing
- `UPDATE_MODE::DRAW` - Actually draw and update ambient sprites

### Sprite Storage

Sprites are stored in CSpriteStore containers:
- **CTerrain** - Terrain sprites (one instance)
- **CTransport** - Vehicle sprites (one instance)
- **CBuilding** (inferred) - Building sprites

Each sprite has:
- Multiple zoom levels
- Multiple directional variants
- Multiple animation frames
- Damage/stage variants

### Coordinate Systems

Multiple coordinate systems are used:

1. **CHexCoord** - Logical hex map coordinates (wraps at map edges)
2. **CSubHex** - Sub-hex coordinates (2x2 per hex for precision)
3. **CMapLoc** / **CMapLoc3D** - 3D map locations (includes altitude)
4. **Window coords** - Screen pixel coordinates
5. **DIB coords** - Bitmap pixel coordinates

### Color and Palette

- 8-bit paletted rendering (256 colors)
- Color format selectable (1, 2, 3, or 4 bytes per pixel)
- SetPixel1/2/3/4 functions handle different bit depths
- Terrain colors mapped from RGB values to palette indices

---

## Rendering Bugs and Issues

### 1. **BUGBUG: Cursor Deletion (area.cpp:817)**

**Severity:** Low
**Location:** CWndArea::OnDestroy()
**Description:** Comment states "is there no way to delete a cursor?" suggesting cursor resources may not be properly cleaned up.
**Impact:** Potential resource leak of cursor handles
**Code:**
```cpp
// BUGBUG - is there no way to delete a cursor?
```

### 2. **BUGBUG: Black Noise Handling (world.cpp:408)**

**Severity:** Low
**Location:** CWndWorld rendering
**Description:** Comment about black color and noise not being changed together
**Impact:** Cosmetic rendering issue with specific colors

### 3. **NULL Pointer Check for fnSetPixel (world.cpp:1098)**

**Severity:** Medium
**Location:** CWndWorld::_NewDir()
**Description:** Added NULL check for fnSetPixel - appears to be defensive coding to prevent crashes if function pointer isn't set
```cpp
if ( fnSetPixel != nullptr ) {
    ( *fnSetPixel ) ( pDib, dwClr );
}
```
**Potential Issue:** Suggests fnSetPixel could theoretically be null if switch statement defaults incorrectly
**Status:** Appears to be fixed/mitigated with defensive check

### 4. **Uninitialized fnSetPixel in world.cpp**

**Severity:** Medium
**Location:** CWndWorld::_NewDir() - switch statement for setting fnSetPixel
**Issue:** Default case sets fnSetPixel to nullptr with comment "NOTE: added default to ensure fnSetPixel always has a value"
```cpp
default:
    fnSetPixel = nullptr; // NOTE: added default...
```
**Problem:** If iBytesPerPixel is not 1, 2, 3, or 4, rendering pixels will be skipped
**Recommendation:** Should handle unexpected bit depths more robustly or assert

### 5. **World Map Rendering Boundary Conditions**

**Severity:** Low
**Location:** CWndWorld::_NewLocation() - bounds checking
**Description:** Multiple TRAP() assertions around xDib/yDib wrapping:
```cpp
while ( m_xDib >= m_cx ) {
    TRAP();
    m_xDib -= m_cx;
}
```
**Issue:** These TRAPs suggest boundary conditions that shouldn't occur but do
**Impact:** Debug-only markers, doesn't affect release builds

### 6. **TODO: Control Color Implementation (area.cpp:4780-4782)**

**Severity:** Very Low (Cosmetic)
**Location:** CWndArea::OnCtlColor()
**Description:**
```cpp
// TODO: Change any attributes of the DC here
// TODO: Return a different brush if the default is not desired
```
**Impact:** None - default behavior is acceptable

---

## Rendering Performance Considerations

### Optimization Techniques Used

1. **Dirty Rect Updating** - Only redraw changed regions
2. **Multi-Level Caching** - Multiple bitmap buffers:
   - m_pdibGround0 - Full terrain at rotation 0,0
   - m_pdibBase - Visible game area with buildings/resources
   - m_pdibRadar - Radar overlay
   - m_pdibButtons - UI buttons
3. **Frame Rate Control** - Throttled rendering
4. **Pixel-Level Resolution on Mini Map** - Each hex = 1-4 pixels
5. **Blt Operations** - Fast bitmap copy operations
6. **Function Pointers** - SetPixel1/2/3/4 avoid runtime branching

### Potential Performance Issues

1. **World Map Recreation** - Full redraw on direction/mode changes
2. **No Z-Buffer** - Rendering order critical (can cause artifacts)
3. **Software Rendering** - All rendering to memory, then copy to screen
4. **Full Screen Copies** - ReRender copies entire pdibBase to dibwnd each frame

---

## Rendering System Data Flow

```
Main Loop (CConquerApp::Run)
    ↓
GraphicsEnginePump()
    ↓
CWndArea::ReRender() ─→ Dirty rectangle animation update
    ├─ Update terrain
    ├─ Update buildings
    ├─ Update vehicles
    ├─ Update effects
    └─ Update UI
    ↓
CWndWorld::ReRender() ─→ World map update
    ├─ Blit base map
    ├─ Overlay units (if radar)
    ├─ Overlay visibility
    └─ Overlay radar
    ↓
Draw() → Blit DIB buffers to screen via WinG
```

---

## Sprite System Details

### Sprite Types
```cpp
enum class SPRITE_TYPES {
    STRUCTURE,  // Buildings, bridges
    TERRAIN,    // Hexes, roads, water
    VEHICLE,    // Mobile units, ships
    EFFECT      // Explosions, smoke
};
```

### Zoom Levels
- Level 0: Full size (MAX_HEX_HT * 2 × MAX_HEX_HT)
- Level 1: 50% (MAX_HEX_HT × MAX_HEX_HT / 2)
- Level 2: 25% (MAX_HEX_HT / 2 × MAX_HEX_HT / 4)
- Level 3: 12.5% (MAX_HEX_HT / 4 × MAX_HEX_HT / 8)

### Terrain Shading
- 8 shade levels for altitude visualization
- Shading calculated from altitude of hex quad corners
- Feathering applied at boundaries between terrain types

---

## Known Rendering Elements

### What Is Drawn

✓ Terrain hexes
✓ Buildings (all stages and damage states)
✓ Vehicles (all directions and damage states)
✓ Roads (with proper junction connections)
✓ Bridges (multi-segment with build progress)
✓ Boats/Ships (as vehicles)
✓ Resources (minerals showing on map)
✓ Unit damage (smoke and flames)
✓ Selection highlighting
✓ Cursors (terrain, building placement, targeting)
✓ UI buttons and status
✓ World map / radar
✓ Visibility indicators
✓ Building foundations
✓ Unit status bars

---

## Rendering Configuration

### Global Rendering Variables
```cpp
int xiZoom;           // Current zoom level
int xiDir;            // Current rotation direction
BOOL bShowAmb;        // Show all or only changed sprites
BOOL bForceDraw;      // Force complete redraw
BOOL bInvAmb;         // Invalidate all ambient sprites
CDIBWnd* xpdibwnd;    // Current DIB window being drawn to
CAnimAtr const* xpanimatr; // Animation attributes for current window
```

### Display List Order
Rendering uses a sorted display list (CTileDrawInfo) to ensure correct depth-sorting:
1. Terrain rendered first
2. Buildings behind units
3. Units sorted by Y coordinate (painter's algorithm)
4. Effects rendered on top

---

---

## Rendering Pipeline Execution Flow (Verified)

```
CConquerApp::Run() main loop
    ↓
GraphicsEnginePump()
    ↓
ProcessAllMessages() - Handle Windows messages
    ↓
Check if time for next frame (1000/FRAME_RATE = ~42ms)
    ↓
_RenderScreens() - Only if enough time has passed
    ├─ Calculate frame timing
    ├─ ReRender() ALL windows [1st loop]
    │   ├─ CWndArea::ReRender()
    │   │   ├─ theMap.Update(m_aa) - Update all sprites
    │   │   └─ Handle scrolling/cursor movement
    │   └─ CWndWorld::ReRender()
    │       ├─ Update minimap with current state
    │       └─ Handle resource blinking
    │
    ├─ Draw() ALL windows [2nd loop]
    │   ├─ CWndArea::Draw()
    │   └─ CWndWorld::Draw()
    │
    └─ CHexCoord::ClearInvalidated() - Clear all hex invalidation flags

Sleep until next frame time
```

**Critical Sequence:**
1. ALL ReRender() must complete before ANY Draw()
2. ClearInvalidated() must happen AFTER all Drawing
3. If sequence violated, rendering will be corrupt

---

## CRITICAL RENDERING SYSTEM GOTCHAS & EDGE CASES

### 1. **Global Variable xiDir Must Be Set Before Sprite Rendering**

**Risk Level:** HIGH
**Location:** area.cpp:4226-4227, sprite.cpp global
**Issue:**
```cpp
int iOldDir = xiDir;
xiDir = m_aa.m_iDir;  // MUST set this before drawing!
// ... rendering happens here ...
xiDir = iOldDir;      // Then restore
```

**Gotcha:** If xiDir is not set to the correct window direction before rendering sprites, building/vehicle sprites will be drawn rotated by the wrong amount. This affects:
- Building direction visualization (GetDrawDir uses xiDir)
- Vehicle rotation calculations
- Coordinate transformations

**Why It Matters:** Different area windows can have different directions (rotated 4 ways). If xiDir doesn't match the current window's m_aa.m_iDir, all rotated sprites will display incorrectly.

---

### 2. **Viewport Pointer (prVp) Can Be NULL and MUST Be Set**

**Risk Level:** CRITICAL
**Location:** sprite.cpp:132, 426, 2433
**Code:**
```cpp
CRect* prVp = NULL;  // Global static - initialized to NULL!

// In StructureDraw():
return StructureDrawToDIB( pdib, ptUL, *prVp );  // DEREFERENCE without null check!

// Gets set in:
prVp = &rect;  // In CSpriteDIB::TerrainDraw() and related functions
```

**The Gotcha:**
- prVp is used for clipping in all sprite rendering
- It's dereferenced WITHOUT null checks in multiple places (line 426)
- If rendering code is called without first setting prVp via CDrawParms::SetClipRect(), it will crash
- No visible guard against this - relies on proper calling sequence

**Likely Bug:** If any rendering is triggered out-of-order or during initialization, crash due to NULL dereference is possible.

**Recommendation:** Should have an assert like `ASSERT(prVp != NULL)` before dereferencing.

---

### 3. **fnSetPixel Function Pointer Can Be NULL (World Map Only)**

**Risk Level:** MEDIUM (mitigated)
**Location:** world.cpp:1098, 1385, 1434, 1456, 1487
**Issue:**
```cpp
SETPIXEL fnSetPixel;
switch ( iBytesPerPixel ) {
case 1: fnSetPixel = SetPixel1; break;
case 2: fnSetPixel = SetPixel2; break;
case 3: fnSetPixel = SetPixel3; break;
case 4: fnSetPixel = SetPixel4; break;
default:
    fnSetPixel = nullptr;  // NOTE: added default...
}

// Later:
if ( fnSetPixel != nullptr ) {  // Must check!
    ( *fnSetPixel ) ( pDib, dwClr );
}
```

**The Gotcha:**
- iBytesPerPixel values other than 1,2,3,4 will leave fnSetPixel as nullptr
- Multiple pixel assignments will silently fail
- Debug builds added defensive checks, but if iBytesPerPixel is unexpected, pixels simply won't render

**When It Happens:** If DIB format changes or unexpected color depth is used

**Status:** Appears to be mitigated with explicit checks, but defensive programming indicates this was a discovered issue

---

### 4. **Rendering Order in _RenderScreens() Is Fixed and Critical**

**Risk Level:** MEDIUM
**Location:** Mainloop.cpp:364-388
**Sequence:**
```cpp
// First: ReRender() all windows (update logic & dirty rects)
for ( each window )
    pWnd->ReRender();

// Second: Draw() all windows (blit to screen)
for ( each window )
    pWnd->Draw();

// Third: CLEAR INVALIDATION FLAGS
CHexCoord::ClearInvalidated();
```

**The Gotcha:**
- Must complete ALL ReRender() calls before ANY Draw() calls
- If a window's Draw() is called before its ReRender(), it will blit stale data
- Invalidation flags are GLOBAL and cleared for ALL windows after drawing
- If invalidation flags are cleared before a window's dirty rects are processed, that window won't update next frame

**Implication:** Cannot easily parallelize rendering across multiple windows or threads without careful synchronization.

---

### 5. **Coordinate Wrapping on World Map Is Manual and Error-Prone**

**Risk Level:** MEDIUM
**Location:** world.cpp:1175-1190, _NewLocation()
**Code:**
```cpp
while ( m_xDib >= m_cx ) {
    TRAP();  // <-- DEBUG TRAP HERE
    m_xDib -= m_cx;
}
while ( m_xDib < 0 ) {
    TRAP();  // <-- DEBUG TRAP HERE
    m_xDib += m_cx;
}
```

**The Gotcha:**
- TRAP() is hit when boundary conditions occur that shouldn't
- Manual wrapping suggests complex coordinate calculations
- Comments indicate this is known to be problematic ("got a GPF below")
- Multiple TRAP() calls suggest repeated debugging of this issue

**When It Happens:** When area map moves near world edges or wraps around

---

### 6. **Global Rendering State Must Be Synchronized Across Windows**

**Risk Level:** HIGH
**Global Variables:**
```cpp
int xiZoom;           // Current zoom level - SET PER WINDOW
int xiDir;            // Current direction - SET PER WINDOW
BOOL bShowAmb;        // Show all sprites or just invalidated
BOOL bForceDraw;      // Force all tiles to redraw
BOOL bInvAmb;         // Invalidate all ambient sprites
CDIBWnd* xpdibwnd;    // CURRENT WINDOW - CHANGES EACH CALL
CAnimAtr const* xpanimatr;  // Animation attributes - CHANGES
```

**The Gotcha:**
- These are GLOBAL variables that must be updated before each sprite rendering call
- Multiple windows rendering concurrently would corrupt these values
- No thread-safe access; single-threaded assumption built in
- Sprite rendering code relies on these being correct WITHOUT validation

**Critical Assumption:** Only ONE window renders at a time. If violated, rendering will be corrupt.

**Example Problem:**
```
Thread A: xiZoom = 0; xiDir = 0;
Thread B: xiZoom = 2; xiDir = 1;  // Corrupts Thread A's state
Thread A: render sprite for zoom 0... but xiZoom is now 2!
```

---

### 7. **Sprite Assignment via m_psprite Directly Modifies Map State**

**Risk Level:** MEDIUM
**Location:** area.cpp:2091, 2104, 2108, 4793, 4861
**Pattern:**
```cpp
pHex->m_psprite = theTerrain.GetSprite( CHex::resources, aiRes[pMn->GetType()] );
pHex->SetInvalidated();  // Mark for redraw
```

**The Gotcha:**
- Sprite assignment directly changes terrain appearance
- ResClicked() function shows resources by SWAPPING terrain sprites
- When toggling resource display, actual terrain sprites get replaced
- Restoration requires knowing what the original sprite was

**Edge Case:** If resource toggling fails mid-operation or is interrupted, terrain sprites could be left in corrupted state showing resources permanently.

---

### 8. **Animation Invalidation Is Frame-Based with Potential Race Conditions**

**Risk Level:** MEDIUM
**Location:** sprite.cpp, terrain.cpp:590, 2630
**Pattern:**
```cpp
CHexCoord( X() + iX, Y() + iY ).SetInvalidated();
hex.SetInvalidated();

// Later...
CHexCoord::ClearInvalidated();  // Clears ALL invalidation flags
```

**The Gotcha:**
- SetInvalidated() marks a hex for redraw next frame
- ClearInvalidated() clears ALL flags at end of _RenderScreens()
- If a hex is invalidated during rendering, it's cleared immediately
- Sprites that should animate may not if invalidation timing is wrong

**Potential Issue:** Rapid animations (explosions, building construction) that invalidate faster than rendering could miss frames.

---

### 9. **Resource Management Issues with Bitmaps and Cursors**

**Risk Level:** MEDIUM
**Location:** area.cpp:817 (BUGBUG), world.cpp:77-121 (Close)
**Issues:**
```cpp
// BUGBUG - is there no way to delete a cursor?
// Multiple cursors allocated:
HCURSOR m_hCurReg;
HCURSOR m_hCurGoto[4];
HCURSOR m_hCurWait;
// ... more cursors ...

// Only deleted in Close():
delete m_pdibGround0;
delete m_pdibBase;
delete m_pdibRadar;
delete[] m_piRadarEdges;
delete m_pdibButtons;
// But NO cursor destruction!
```

**The Gotcha:**
- HCURSOR handles cannot be deleted with `delete`
- Must use DestroyCursor() for proper cleanup
- Comment suggests developer knew but didn't know how to fix
- Potential resource leak if windows are destroyed/recreated

---

### 10. **Multi-Hex Buildings Don't Have Contiguous Memory Layout**

**Risk Level:** MEDIUM (Architectural)
**Issue:** Buildings can span multiple hexes but aren't stored contiguously
- Building drawing uses CHex::Draw() for each hex separately
- Building state is tracked via building pointer, not hex grid
- Foundation drawing must iterate foundations per hex

**Gotcha:** If building deletion or movement doesn't properly update all hex pointers, rendering could show corrupted/phantom buildings.

---

### 11. **No Bounds Checking on Zoom Level Array Access**

**Risk Level:** LOW-MEDIUM
**Location:** world.cpp:53-54 terrain.h
```cpp
const int hHexWid[NUM_ZOOM_LEVELS] = { ... };
const int hHexHt[NUM_ZOOM_LEVELS] = { ... };

// Used via:
static int HexWid( int iZoom ) {
    ASSERT_STRICT( ( iZoom >= 0 ) && ( iZoom < NUM_ZOOM_LEVELS ) );
    return ( hHexWid[iZoom] );
}
```

**Gotcha:** Debug builds have ASSERT, release builds don't. If iZoom gets corrupted or is out of range, array access will read garbage.

---

### 12. **Coordinate Transformation Complex and Error-Prone**

**Risk Level:** MEDIUM (Architectural)
**Multiple Coordinate Systems:**
1. CHexCoord - logical hex map coordinates (wraps)
2. CSubHex - sub-hex coordinates (2x2 per hex)
3. CMapLoc - 3D map locations with altitude
4. Window coordinates - screen pixels
5. DIB coordinates - bitmap pixels
6. Sprite view coordinates - sprite-relative

**Gotcha:**
- Transformations between systems are complex
- Wrapping happens in different ways in different systems
- Off-by-one errors in transformations cause visual glitches
- No type-safety between coordinate systems (all use int)

---

### 13. **Rendering Order Uses Painter's Algorithm with Y-Sort**

**Risk Level:** MEDIUM
**Issue:** Buildings and vehicles sorted by Y-coordinate for depth
- Works for hex-based isometric view
- But can fail if sprites overlap significantly
- No Z-buffer means rendering order MUST be correct

**Gotcha:** If a sprite's Y coordinate calculation is off by 1, it renders in front of or behind the wrong sprite, creating visual artifacts.

---

### 14. **Feathering Values Are Random Per Zoom Level**

**Risk Level:** LOW
**Location:** sprite.cpp:99-118 (FeatherInit)
```cpp
for ( int i = 0; i < FEATHER_DIM; ++i )
    xaaiFeather[0][i] = 6 + RandNum( 11 );  // RANDOM!

for ( int j = 1; j < NUM_ZOOM_LEVELS; ++j )
    for ( int i = 0; i < FEATHER_DIM; ++i )
        xaaiFeather[j][i] = xaaiFeather[0][i] >> j;
```

**Gotcha:** Terrain feathering uses random values, so each run has slightly different terrain blending. This means:
- Replays won't look identical
- Screenshot comparisons will fail
- Deterministic testing of rendering is difficult

---

## Summary

The Enemy Nations rendering system is a sophisticated software renderer using:
- Hexagonal tile-based terrain
- Sprite-based graphics with multiple LOD levels
- Efficient dirty-rect updating
- Windward Wind22 library for graphics primitives
- Multiple rendering layers (terrain, buildings, vehicles, effects, UI)
- Multi-directional sprite variants for visual variety
- Altitude-based shading for 3D effect

**Critical Issues Requiring Attention:**

1. **prVp NULL dereference** - Viewport pointer not validated before use
2. **xiDir/xiZoom global state** - Must be synchronized per-window, single-threaded assumption
3. **Coordinate wrapping edge cases** - Multiple TRAP() calls indicate known issues
4. **Resource cleanup** - Cursors not destroyed, potential memory leak
5. **Thread safety** - No synchronization for global rendering state

**Medium Severity Issues:**

1. fnSetPixel could be null on unexpected bit depths
2. Complex coordinate transformations (error-prone)
3. Animation invalidation timing
4. Building sprite management
5. Rendering order dependencies

**Low Severity / Cosmetic:**

1. Cursor cleanup BUGBUG (known issue, doesn't crash)
2. Random feathering values (non-deterministic)
3. TODO comments for UI enhancements
4. Multiple boundary condition TRAP() calls

---

## Rendering Invalidation System (Deep Dive)

### How It Works

1. **Setting invalidation:**
   ```cpp
   CHexCoord hex(x, y);
   hex.SetInvalidated();  // Mark this hex's sprite needs redraw
   ```

2. **During rendering:**
   ```cpp
   if (bForceDraw)  // Global flag - force all sprites
       render ALL sprites
   else if (hex.IsInvalidated())  // Only render marked ones
       render THIS sprite
   ```

3. **Cleanup after frame:**
   ```cpp
   CHexCoord::ClearInvalidated();  // Clear ALL invalidation flags
   ```

### Discovered Issues

**GGTESTING Comments:** The code contains many commented-out InvalidateWindow() calls with "GGTESTING" prefix:
```cpp
// GGTESTING  InvalidateWindow ();
// GGTESTING InvalidateWindow();
// GGTESTING   InvalidateWindow();
```

**Implication:** Developers were testing whether forced invalidation was necessary. This suggests:
- Invalidation logic may be unreliable
- Some rendering updates might be missed if invalidation fails
- The system was debugged by adding/removing full-screen redraws

### Edge Case: Building Invalidation

When buildings are built/destroyed, they need to:
1. Invalidate their own hex
2. Invalidate adjacent hexes (for visual effects)
3. BUT if this invalidation happens DURING rendering, it gets cleared immediately!

**Risk:** Race condition where building changes don't render until next frame if timing is wrong.

---

## Known Decoder Implementation (enations/src/dave/)

The codebase contains a parallel "dave" implementation directory with:
- `dave/sprite.cpp`
- `dave/terrain.cpp`
- `dave/unit.cpp`
- `dave/vehicle.h`

**This suggests:** A separate graphics decoder or alternative implementation was being developed/maintained. The main rendering code in `enations/src/` may differ from this implementation.

**Gotcha:** Any rendering fixes applied to one implementation might not propagate to the other.

---

## Rendering System Assumptions

1. **Single-threaded execution** - Global xiZoom/xiDir/xpdibwnd not thread-safe
2. **Fixed frame rate** - FRAME_RATE = 24 fps assumed throughout
3. **8-bit palette graphics** - Color depth handling shows palette-based rendering
4. **Hexagonal isometric projection** - All coordinate math assumes this
5. **Painter's algorithm with Y-sort** - No 3D depth buffer
6. **WinG API available** - CDIBWnd uses WinG, won't work on modern Windows
7. **Invalidation works per-frame** - Can't handle invalidation across frames properly

---

---

## BRIDGES - Detailed Rendering System

### Architecture

**CBridge** - Represents complete bridge from start to end hex
- Stores start/end coordinates
- Calculates middle point for display
- Owns list of CBridgeUnit objects (one per hex)
- Stores build percentage (0-100)
- Has direction: m_iDir (0 = Y-direction, 1 = X-direction)
- Stores altitude that bridge spans across

**CBridgeUnit** - Single hex segment of bridge
- Owns the actual sprite (CStructureSprite)
- Has position, direction, exit type
- Tracks build percentage same as parent bridge
- Stores altitude of this segment

### Bridge Creation & Sprite Assignment

**CBridge::Create() Flow:**
```cpp
1. Create CBridge instance
2. Calculate direction from start/end (X vs Y)
3. Calculate span length (distance + 1)
4. Set construction time: span * 40 * CTerrainData::GetBuildRoadTime()
5. Iterate from start to end hex:
   a. Create CBridgeUnit for each hex
   b. Add to m_lstUnits list
   c. For start hex: set m_iExit (which direction leads out)
   d. For end hex: set m_iExit and call AssignSprite()
   e. For middle hexes: call AssignSprite()
6. Add bridge to theBridgeMap
```

**CBridgeUnit::AssignSprite() Logic:**
```cpp
if (m_iExit == -1)  // Middle segment
    if (m_iDir == 0)      iID = bridge_0   // Y-direction bridge
    if (m_iDir == 1)      iID = bridge_1   // X-direction bridge
else                // End segment
    case 0: iID = bridge_end_2   // Exit direction 0
    case 1: iID = bridge_end_3   // Exit direction 1
    case 2: iID = bridge_end_0   // Exit direction 2
    case 3: iID = bridge_end_1   // Exit direction 3

m_psprite = theStructures.GetSprite(iID, 0)
```

### Bridge Rendering

**CBridgeUnit::Draw() Process:**

1. **Get draw corner position:**
   ```cpp
   GetDrawCorner(hexcoord) {
       // Adjust corner based on current rotation (xiDir)
       switch(xiDir) {
           case 0: corner = (hexX, hexY)
           case 1: corner = (hexX+1, hexY)
           case 2: corner = (hexX+1, hexY+1)
           case 3: corner = (hexX, hexY+1)
       }
       // Convert to window coords with FUDGE adjustments
   }
   ```

2. **Get sprite view:**
   - For built bridges: Use COMPLETED_STAGE
   - For under-construction: Use CONSTRUCTION_STAGE

3. **Apply Y-position adjustments:**
   ```cpp
   pt.y += MAX_HEX_HT >> (xiZoom + 1)  // Base height

   if (m_iExit == -1)  // Middle segment
       pt.y += BRIDGE_Y_CENTER_FUDGE >> xiZoom   // +21 pixels
   else if (m_iExit matches xiDir)  // Exit at top
       pt.y += BRIDGE_Y_TOP_END_FUDGE >> xiZoom  // -4 pixels
   ```

   **GOTCHA:** These FUDGE constants suggest artwork is misaligned by 21 and -4 pixels respectively!

4. **Render based on build state:**
   - **Built:** Draw full sprite from GetView()
   - **Under construction:**
     - Draw CONSTRUCTION_STAGE sprite
     - Use DrawClip() with partial rect
     - Calculate visible portion: rect.top += (100 - percent) * height / 100
     - This clips the bottom of sprite to show incomplete build

5. **Two-piece bridges:**
   - Call IsTwoPiece() to check if bridge has two visual layers
   - If yes: Draw FOREGROUND_LAYER first, then BACKGROUND_LAYER
   - Each layer has its own height calculation

### Bridge Coordinate Issues

**Direction calculation (lines 32-33):**
```cpp
m_iDir = (hexStart.X() != hexEnd.X()) ? 1 : 0;
// If X coordinates differ -> X-direction bridge
// Otherwise -> Y-direction bridge
```

**Exit direction encoding (lines 75-86):**
- Start exit: `(xAdd != 0) ? (xAdd + 2) : ((yAdd + 3) & 0x03)`
- End exit: `(xAdd != 0) ? (xAdd & 0x03) : (yAdd + 1)`

**GOTCHA:** This complex encoding converts xAdd/yAdd (-1, 0, 1) to exit directions (0-3). Off-by-one errors here would render wrong bridge ends.

### Bridge Altitude Flattening

When bridge created:
```cpp
hexStart.Flatten(iAlt);  // Set 4 corners of start hex to same altitude
hexEnd.Flatten(iAlt);    // Set 4 corners of end hex to same altitude
```

This ensures smooth terrain transition at bridge endpoints.

### Bridge Update Mechanism

```cpp
CBridge::__SetPer(int iPercent) {
    m_iPerBuilt = iPercent;
    for (each CBridgeUnit pBu) {
        pBu->m_iPerBuilt = iPercent;
        pBu->m_hex.SetInvalidated();  // Mark for redraw
    }
}
```

When construction percentage changes, all bridge segments are invalidated.

### Bridge Gotchas & Issues

**1. GGTODO: Height on sloped endpoints (line 158)**
```cpp
Fix GetSurfaceAlt(CMapLoc) const {
    fix.Value((GetAlt() << 16) + 0x00060000);  // GGTODO: Figure out height on sloped endpoints
    return fix;
}
```
**Issue:** Bridge altitude doesn't properly account for sloped terrain at endpoints. Currently uses fixed offset.

**2. Artwork misalignment fudges (lines 266-272)**
```cpp
const int BRIDGE_Y_CENTER_FUDGE = 21;      // Artwork misaligned
const int BRIDGE_Y_TOP_END_FUDGE = -4;     // Artwork misaligned
```
**Issue:** Bridge sprites have known artwork alignment issues requiring pixel-level adjustments.

**3. Exit direction encoding complexity (lines 74-86)**
The exit direction calculation uses arithmetic on xAdd/yAdd that could be error-prone:
```cpp
pBu->m_iExit = (xAdd != 0) ? (xAdd + 2) : ((yAdd + 3) & 0x03);
```
If xAdd/yAdd wrapping fails, exit directions would be wrong.

**4. Two-piece bridge layer rendering (lines 369-383)**
If IsHitClip() succeeds on FOREGROUND_LAYER, it returns without checking BACKGROUND_LAYER.
**Potential issue:** Click detection might be wrong if foreground doesn't cover the actual visual area.

**5. GetLayer() function used but not shown**
```cpp
GetSprite()->GetView(iDir, GetLayer(), stage, damage)
```
GetLayer() method determines if FOREGROUND or BACKGROUND is used initially. Logic not shown in read code.

---

## ROADS - Detailed Rendering System

### Architecture

Roads are stored as terrain sprites in CHex, not as separate objects like bridges.
- Terrain type = CHex::road
- Sprite determined by connectivity to neighbors

### Road Connectivity Algorithm

**CHex::ChangeToRoad() Process:**

1. **Set hex to road terrain type:**
   ```cpp
   m_bType = CHex::road;
   SetVisibleType(CHex::road);
   SetTree(0);  // Remove trees
   ```

2. **Create 8-bit connectivity mask by checking 4 neighbors:**
   ```
   Bit 0 (value 1): Above neighbor      (-1Y)
   Bit 1 (value 2): Left neighbor       (-1X, +1Y)
   Bit 2 (value 4): Right neighbor      (+2X)
   Bit 3 (value 8): Bottom neighbor     (-1X, +1Y)
   ```

3. **Check each neighbor type:**
   - If neighbor is CHex::road → Set bit
   - If neighbor is building with exit → Set bit
   - Otherwise → Don't set bit

4. **Determine road sprite from mask:**
   ```
   iType = 0x0:  r_path     (isolated road segment)
   iType = 0x1 or 0x8 or 0x9:        r_vert     (vertical road)
   iType = 0x2 or 0x4 or 0x6:        r_horz     (horizontal road)
   iType = 0x3:  r_l_lr     (L-corner: top-right)
   iType = 0x5:  r_l_ll     (L-corner: top-left)
   iType = 0xA:  r_l_ur     (L-corner: bottom-right)
   iType = 0xC:  r_l_ul     (L-corner: bottom-left)
   iType = 0x7:  r_t_dn     (T-junction: down)
   iType = 0xB:  r_t_rt     (T-junction: right)
   iType = 0xD:  r_t_lf     (T-junction: left)
   iType = 0xE:  r_t_up     (T-junction: up)
   iType = 0xF:  r_x        (cross junction)
   default (0):  r_x        (also used for isolated)
   ```

5. **Assign sprite:**
   ```cpp
   m_psprite = theTerrain.GetSprite(CHex::road, iIndex);
   hex.SetInvalidated();
   theApp.m_wndWorld.NewMode();  // Update world map
   ```

6. **Recursive update (if bCallNext = TRUE):**
   ```cpp
   for (each neighbor that changed to road) {
       pHex->ChangeToRoad(_hexOn, FALSE);  // Non-recursive to avoid infinite loop
   }
   ```

### Road Connectivity Details

**Neighbor positions in ChangeToRoad():**

```
Iteration 1: Above
  _hexOn = CHexCoord(hex.X(), hex.Y() - 1)

Iteration 2: Left
  _hexOn.Xdec(); _hexOn.Yinc()  // Move to left diagonal

Iteration 3: Right
  _hexOn.X() += 2;  // Move to right

Iteration 4: Bottom
  _hexOn.Xdec(); _hexOn.Yinc()  // Move to bottom diagonal
```

### Building Exit Integration

Roads can connect to building exits:
```cpp
if (pHex->GetUnits() & CHex::bldg) {
    CBuilding* pBldg = theBuildingHex.GetBuilding(_hexOn);
    if (pBldg != NULL && pBldg->GetExitHex() == _hexOn)
        iType |= bit;  // Treat building exit as connected road
}
```

This allows roads to properly connect to building entrance hexes.

### Road Rendering Flow

1. CHex stores sprite pointer m_psprite
2. When road connectivity changes, sprite is reassigned
3. Next frame, CHex::Draw() renders the assigned sprite
4. Sprite drawn at its hex position like any terrain

### Road Gotchas & Issues

**1. Complex neighbor offset calculations**
```cpp
_hexOn.X() += 2;  // Right neighbor is X+2
_hexOn.Xdec(); _hexOn.Yinc();  // Diagonal movements
```

**GOTCHA:** These offset calculations are hex-grid specific and could fail if coordinate system changed. Off-by-one errors would connect to wrong neighbors.

**2. Connectivity mask is order-dependent**
```
Above = bit 0 (value 1)
Left = bit 1 (value 2)
Right = bit 2 (value 4)
Bottom = bit 3 (value 8)
```

If neighbor checking order changes, connectivity mask values change and wrong sprites render.

**3. Recursive neighbor update can be expensive**
When road placed, all neighbors are checked and updated recursively. A large area of roads could cause cascade updates.

**Pattern observed:**
```cpp
if (bCallNext)
    pHex->ChangeToRoad(_hexOn, FALSE);  // FALSE to stop recursion
```
This prevents infinite recursion but means only direct neighbors are updated, not second-order neighbors.

**4. Building exit assumption**
Code assumes building exit is always a valid road connection point:
```cpp
if (pBldg->GetExitHex() == _hexOn)
    iType |= bit;
```

**GOTCHA:** If building exit hex calculation is wrong elsewhere, roads will connect incorrectly.

**5. Visibility not updated**
```cpp
if ((!GetVisibility()) && (!bForce) && (GetVisibleType() != road))
    return;  // Don't render if not visible
```

Road sprite is NOT assigned if hex is invisible and not forced. This means:
- If road placed on invisible hex, it won't render until visible
- But connectivity IS calculated, so adjacent roads update correctly
- Could cause visual glitches where connection sprite exists but road doesn't render

**6. World map needs explicit update**
```cpp
theApp.m_wndWorld.NewMode();  // Update world minimap
```

After road changes, world map must be explicitly updated. If this call fails or is missed, minimap shows stale road data.

**7. Sprite assignment assumes valid index**
```cpp
m_psprite = theTerrain.GetSprite(CHex::road, iIndex);
```

If iIndex calculation produces invalid value, GetSprite() could return nullptr or wrong sprite. No validation checks before assignment.

### Road vs Bridge Rendering Priority

In display list sorting (CHexCoordColumnIter), both roads and bridges are rendered, but:
- Roads: Terrain layer (rendered early)
- Bridges: Structure layer (rendered later)

This means bridges always appear on top of roads. If they occupy same hex, bridge hides road.

---

## Bridges vs Roads - Rendering Comparison

| Aspect | Bridges | Roads |
|--------|---------|-------|
| **Storage** | Separate CBridge/CBridgeUnit objects | Terrain sprite in CHex |
| **Creation** | CBridge::Create() creates all units | ChangeToRoad() creates nothing, just updates sprite |
| **Rendering** | CStructureSprite (building-like) | CTerrainSprite (terrain-like) |
| **Layers** | 2 layers (foreground/background) | Single layer |
| **Connectivity** | Fixed endpoints, stored as m_iExit | Dynamic 4-neighbor connectivity mask |
| **Construction** | Shows partial build with clip | N/A (instant) |
| **Damage** | Can be damaged, shows damage sprites | N/A (cannot damage) |
| **Altitude** | Flattens 4 corners at endpoints | Uses terrain altitude as-is |
| **Rendering Order** | After terrain, before vehicles | With terrain |
| **Multiple hexes** | Yes (multiple CBridgeUnits) | Single hex |

---

## BUILD MENU - Detailed Rendering System

**Files:** unit_wnd.h (lines 249-314), unit_wnd.cpp (lines 858-1395)

The build menu is a modal dialog (CDlgBuildStructure) that allows players to select buildings to construct. It has a sophisticated rendering system with sprite-based buttons, dynamic filtering, and efficient list updates.

### Dialog Architecture

The dialog is a standard Windows modal with custom rendering:

```
Dialog Size: 465×345 pixels
├── Category buttons (left): 6 buttons at positions (11, 22) with 50px vertical spacing
│   └── Size: 104×49 pixels each
│   └── Show building categories: resource, visible, military, etc.
├── Building buttons (middle): 6 buttons at positions (129, 22) with 50px vertical spacing
│   └── Size: 104×49 pixels each
│   └── Display filteredbuildings from selected category
└── Info panel (right): Text display of selected building details
    ├── Description area: (252, 22) to (450, 144)
    ├── Cost/resources area: (264, 158) to (439, 280)
    ├── OK button: (249, 300) 98×23
    └── Cancel button: (359, 300) 98×23
```

### Building Availability Filtering (CanBuild Logic)

The CanBuild() function (unit_wnd.cpp:923) determines which buildings can be built. It applies multiple filters:

**1. Category Match**
```cpp
if (pSd->GetCat() != iIndex)
    return FALSE;
```
Building must match selected category.

**2. Discovery/Research Check**
```cpp
if (!pSd->IsDiscovered())
    return FALSE;
```
Building must be discovered/researched before availability.

**3. Scenario Level Check**
```cpp
if ((theGame.GetScenario() != -1) && (pSd->GetScenario() > theGame.GetScenario()))
    return FALSE;
```
Building cannot be used if scenario requirements are not met.

**4. Special Apartment Logic**
```cpp
if (pSd->GetBldgType() == CStructureData::apartment) {
    int iMax = theApp.IsShareware() ? 3 : num_apartments;
    // Count discovered apartments
    for (int iOn = apartment_base; iOn < apartment_base + iMax; iOn++)
        if (!theStructures.GetData(iOn)->IsDiscovered())
            iMax--;

    int iBase = theGame.GetMe()->GetPplTotal() / 200;  // Population / 200
    iBase = __min(iBase, iMax - 3);
    int iNum = __min(NUM_CIV_BLDG, iMax - iBase);  // NUM_CIV_BLDG = 3
    int iStrt = apartment_base + iBase;

    // Only show 3 variants based on population
    if ((pSd->GetType() < iStrt) || (pSd->GetType() >= iStrt + iNum))
        return FALSE;
}
```

**Key insight:** Apartment availability is population-based. Players get access to more apartment tiers (variants) as population grows. Only 3 variants shown at a time, with the starting point shifting every 200 population.

**5. Special Office Logic**
Similar to apartments but uses building population (GetPplBldg()) divided by 100 instead.

**6. Factory/Shipyard Vehicle Check**
```cpp
if ((pSd->GetUnionType() != CStructureData::UTvehicle) &&
    (pSd->GetUnionType() != CStructureData::UTshipyard))
    return TRUE;  // Not a factory, return TRUE

// For factories/shipyards, check if at least one vehicle is available
CBuildVehicle const* pBv = pSd->GetBldVehicle();
for (int iOn = 0; iOn < pBv->GetSize(); iOn++) {
    CTransportData const* pTd = theTransports.GetData(pBv->GetUnit(iOn)->m_iVehType);
    if ((pTd->IsDiscovered()) &&
        ((theGame.GetScenario() == -1) || (pTd->GetScenario() <= theGame.GetScenario())))
        return TRUE;  // At least one vehicle available
}
return FALSE;  // No buildable vehicles discovered
```

Factory buildings only show if at least one vehicle type is discovered and available for current scenario.

### Dynamic List Population (OnSelchangeBuildListCat)

When a category is selected, the building list is populated:

1. **Clear current selection:**
   ```cpp
   m_iBldgOn = -1;
   m_pSd = NULL;
   m_strDesc = "";
   InvalidateRect(&rectText, FALSE);
   ```

2. **Iterate all buildings and filter:**
   ```cpp
   int iBtnNum = 0;
   for (int iOn = 0; iOn < theStructures.GetNumBuildings(); iOn++) {
       CStructureData const* pSd = theStructures.GetData(iOn);
       if (CanBuild(m_iCatOn, pSd)) {
           m_btnBldg[iBtnNum].SetNum(iOn);
           m_btnBldg[iBtnNum].m_pData = (void*)pSd;
           m_btnBldg[iBtnNum].SetWindowText(pSd->GetDesc());
           m_btnBldg[iBtnNum].InvalidateRect(NULL);
           m_btnBldg[iBtnNum].SetToggleState(FALSE);
           m_btnBldg[iBtnNum].EnableWindow(TRUE);
           iBtnNum++;
           if (iBtnNum >= 6)  // Max 6 buttons
               break;
       }
   }

   // Clear unused buttons
   for (int iOn = iBtnNum; iOn < 6; iOn++) {
       m_btnBldg[iOn].SetNum(-1);
       m_btnBldg[iOn].m_pData = NULL;
       m_btnBldg[iOn].InvalidateRect(NULL);
       m_btnBldg[iOn].SetWindowText("");
       m_btnBldg[iOn].SetToggleState(FALSE);
       m_btnBldg[iOn].EnableWindow(FALSE);
   }
   ```

**Gotcha:** The list is completely repopulated every time a category is selected, even if no buildings changed. This could be optimized with dirty flags on research events.

### Efficient List Updates (UpdateChoices Optimization)

The UpdateChoices() function (line 1073) is smart about re-population:

```cpp
void CDlgBuildStructure::UpdateChoices() {
    // Check if list needs updating by comparing current buttons
    // against what should be there
    if (m_iCatOn >= 0)
        for (int iOn = 0, iInd = 0; iOn < theStructures.GetNumBuildings(); iOn++) {
            CStructureData const* pSd = theStructures.GetData(iOn);
            if (CanBuild(m_iCatOn, pSd)) {
                if (m_btnBldg[iInd].m_pData != (void*)pSd) {
                    OnSelchangeBuildListCat();  // Only re-populate if changed
                    return;
                }
                iInd++;
            }
        }
}
```

**Optimization:** Only calls expensive OnSelchangeBuildListCat() if building list actually changed. This prevents visual flicker and unnecessary redraws.

### Button Rendering with Sprites (CUnitButton::DrawItem)

Building buttons use custom owner-draw with sprite overlays. The rendering pipeline (lines 719-804):

1. **Button State Art** (3-state sprites):
   ```cpp
   CRect rSrc(m_pBtnDib->GetRect());
   rSrc.right /= 3;  // Sprite strip has 3 variants

   if (pDis->itemState & (ODS_DISABLED | ODS_GRAYED))
       rSrc.OffsetRect(rSrc.Width() * 2, 0);  // Disabled state (rightmost)
   else if ((pDis->itemState & ODS_SELECTED) || (m_cState & 0x01))
       rSrc.OffsetRect(rSrc.Width(), 0);      // Pressed state (middle)
   // else normal state (left) - no offset needed

   m_pBtnDib->StretchBlt(m_pDib, rect, rSrc);  // Draw button background
   ```

2. **Building Sprite Overlay** (if present):
   ```cpp
   if (m_pOvrlyDib != NULL) {
       if (bShift)
           rect.OffsetRect(2, 2);  // Shift overlay if button pressed

       // Sprite strip has 64-pixel tall sprites, stacked vertically
       CRect rSrc(0, 64 * m_iOvrlyNum, m_pOvrlyDib->GetWidth(), 64 * m_iOvrlyNum + 64);

       // Center overlay on button
       CRect rDest(0, rect.top, m_pOvrlyDib->GetWidth(), rect.bottom);
       if (rect.Width() >= m_pOvrlyDib->GetWidth())
           rDest.OffsetRect((rect.Width() - m_pOvrlyDib->GetWidth()) / 2, 0);

       // Vertically center if button is taller
       if (rDest.Height() > rSrc.Height()) {
           rDest.top += (rDest.Height() - rSrc.Height()) / 2;
           rDest.bottom = rDest.top + rSrc.Height();
       }

       m_pOvrlyDib->StretchTranBlt(m_pDib, rDest, rSrc);  // Transparent blit
   }
   ```

3. **Button Text Rendering**:
   ```cpp
   if (!sText.IsEmpty()) {
       CDC* pDc = CDC::FromHandle(m_pDib->GetDC());
       pDc->SetBkMode(TRANSPARENT);

       // Font selection based on overlay presence
       if (m_pOvrlyDib != NULL)
           pDc->SelectObject(&theApp.CostFont());
       else
           pDc->SelectObject(&theApp.TextFont());

       // Calculate text layout
       int iHt = rect.Height();
       int iWid = rect.Width();
       pDc->DrawText(sText, -1, &rect, DT_CALCRECT | DT_CENTER | DT_WORDBREAK);

       // Position text
       if (m_pOvrlyDib != NULL)
           rect.OffsetRect((iWid - rect.Width()) / 2, (iHt - rect.Height()) - 2);  // Below sprite
       else
           rect.OffsetRect((iWid - rect.Width()) / 2, (iHt - rect.Height()) / 2);  // Centered

       // Draw text with outline effect (shadow + highlight)
       pDc->SetTextColor(RGB(0, 0, 0));      // Black shadow
       pDc->DrawText(sText, -1, &rect, DT_CENTER | DT_WORDBREAK);

       rect.OffsetRect(-1, -1);
       pDc->SetTextColor(RGB(255, 255, 255));  // White highlight
       pDc->DrawText(sText, -1, &rect, DT_CENTER | DT_WORDBREAK);
   }

   // Final blit to screen
   m_pDib->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));
   ```

**Key Details:**
- **Sprite overlay size:** 64 pixels tall (building sprite + margin)
- **Button dimensions:** 98×23 pixels (narrow buttons fit 2 columns)
- **DIB pre-rendering:** m_pDib (98×23) is built off-screen then blitted to screen in one operation
- **Text outline:** Double-draw with black (shadow) then white (highlight) for readability

### Dialog Content Rendering (OnPaint)

The main dialog background is rendered once and cached in m_pDibBkgnd (465×345 DIB):

1. **Background stretch-blt:**
   ```cpp
   CDIB* pdib = theBitmaps.GetByIndex(DIB_STRUCTURE_BKGND);
   pdib->StretchBlt(m_pDibBkgnd, rect, pdib->GetRect());
   ```

2. **Text rendering to cached DIB:**
   - Description (green text, large font, rect 252,22-450,144)
   - Build time, materials, people, power requirements (blue text, small font)
   - Dynamic cost calculations with color-coded shortfalls

   **Color scheme:**
   - Green (41, 255, 8): Normal text
   - Blue (71, 71, 225): Operating costs
   - Red (255, 41, 8): Resource shortfalls (negative numbers in parentheses)

3. **Separator lines:**
   ```cpp
   CPen pen(PS_SOLID, 1, PALETTERGB(41, 255, 8));
   pDcTxt->MoveTo(264, rect.top);
   pDcTxt->LineTo(439, rect.top);  // Horizontal line between sections
   pDcTxt->MoveTo(329, 158);
   pDcTxt->LineTo(329, rect.top);  // Vertical divider
   ```

4. **Final blit:**
   ```cpp
   m_pDibBkgnd->BitBlt(dc, m_pDibBkgnd->GetRect(), CPoint(0, 0));
   ```

All text rendering happens into the cached DIB, which is then blitted to screen. This prevents flicker and improves performance.

### Build Menu Rendering Gotchas

**1. Category buttons show when no category selected**
```cpp
if (m_iCatOn < 0) {
    m_btnBuild.EnableWindow(FALSE);
    // Clear all building buttons
    for (int iOn = 0; iOn < 6; iOn++)
        m_btnBldg[iOn].EnableWindow(FALSE);
}
```

**GOTCHA:** If user closes and reopens dialog, m_iCatOn could be -1 from previous state if not reset. Building buttons would appear disabled.

**2. Apartment/Office selection is population-gated**
```cpp
int iBase = theGame.GetMe()->GetPplTotal() / 200;
```

**GOTCHA:** If population is between 0-199, iBase = 0 and only base apartments show. Players might think apartments are locked when they're actually just unavailable at low population. No UI feedback about this.

**3. Factory without available vehicles**
```cpp
if ((pSd->GetUnionType() != CStructureData::UTvehicle) &&
    (pSd->GetUnionType() != CStructureData::UTshipyard))
    return TRUE;  // Return TRUE for non-factories!
```

**GOTCHA:** The logic exits early for non-factory buildings! Factory check only happens if it IS a factory/shipyard. Logic could be misunderstood as "factories only return TRUE if vehicles exist" when it actually means "non-factories always return TRUE here."

**4. Max 6 buildings per category shown**
```cpp
if (iBtnNum >= 6)
    break;
```

**GOTCHA:** If a category has more than 6 available buildings, only first 6 are shown. No scrolling, paging, or indication that buildings are hidden.

**5. Button m_pData pointer could be stale**
```cpp
m_btnBldg[iBtnNum].m_pData = (void*)pSd;
```

**GOTCHA:** If CStructureData objects are ever reallocated (during research update), these raw pointers become invalid. UpdateChoices() helps prevent this by detecting mismatches, but if UpdateChoices() isn't called, stale pointers could cause crashes.

**6. Text rendering happens every paint**
```cpp
void CDlgBuildStructure::OnPaint() {
    FitDrawText(pDcTxt, theApp.DescFont(), m_strDesc, rect);
    // ... multiple DrawText calls...
}
```

**GOTCHA:** All text is recalculated and re-rendered on every WM_PAINT message, even if m_pSd didn't change. Could be optimized with dirty flags to only re-render when building selection changed.

**7. OnEraseBkgnd returns 1 (don't erase)**
```cpp
BOOL CDlgBuildStructure::OnEraseBkgnd(CDC*) {
    return (1);  // Don't erase background
}
```

**GOTCHA:** Custom paint handler assumes full coverage. If InvalidateRect is called with partial rect, non-covered areas won't be erased, causing rendering artifacts.

**8. Text outline technique is expensive**
```cpp
// Draw black shadow
pDc->DrawText(sText, -1, &rect, ...);
// Draw white highlight offset
pDc->DrawText(sText, -1, &rect, ...);
```

**GOTCHA:** Each button text is drawn 2x (shadow + highlight). With 12 buttons on screen, that's 24 text draws per frame. For a modal dialog this is acceptable but wouldn't scale to 100s of buttons.

---

## MAIN MENU - Detailed Rendering System

**Files:** lastplnt.h (lines 100-159), lastplnt.cpp (lines 1900-2440)

The main menu (CDlgMain) is the first screen users see. It has a sophisticated dual-rendering system that supports both custom artwork and fallback tiled backgrounds. The system scales button positions to any screen resolution while maintaining artwork integrity.

### Dual-Mode Architecture

The menu has two completely different rendering paths determined by `m_bTile` flag:

**Mode 1: Custom Artwork (m_bTile = FALSE)**
- Loads background image from data file (reference: 1280×768)
- Loads 11 individual button sprite strips
- Buttons positioned at exact pixel coordinates on artwork
- All rendering composited to off-screen DIB then blitted to screen
- Supports full-screen scaling to any resolution

**Mode 2: Tiled Background (m_bTile = TRUE)**
- Fallback if custom artwork unavailable
- Repeating tiled background pattern
- Buttons auto-positioned in 2-column grid layout
- Uses Windows 3D beveled button styling
- No scaling - buttons sized by system

### Button Data Structure

11 buttons stored in static `_btnData` array. Each entry:

```cpp
class _BTN_DATA {
    UINT ID;              // Button control ID
    int x, y;             // Base position on 1280×768 artwork reference
    UINT fmt;             // Text format (DT_CENTER, DT_LEFT, DT_WORDBREAK)
    CRect rText;          // Text bounding box within button sprite
    CPoint ptDnOff;       // Offset when button pressed/selected
};
```

**Complete Button Layout (Reference 1280×768):**

```
Top Row (Y ≈ 25-138):
├── Load (784, 25) - Single-line CENTER
├── Options (1060, 28) - Single-line CENTER
├── Load Multi (776, 135) - Single-line CENTER
└── Credits (1052, 138) - Single-line CENTER

Middle Row (Y ≈ 407-490):
├── Campaign (100, 489) - Multi-line LEFT - Large button
├── Single Player (293, 490) - Multi-line LEFT - Large button
└── Intro (868, 407) - Multi-line CENTER

Bottom Row (Y ≈ 604-744):
├── Create Game (345, 632) - Multi-line LEFT
├── Join Game (326, 744) - Multi-line LEFT
├── Exit (1021, 604) - Single-line CENTER
└── Minimize (1035, 703) - Single-line CENTER
```

### Resolution-Independent Scaling

Buttons scale to any screen resolution using proportional calculations:

```cpp
// In OnSize() - Custom art mode
button_x = (_btnData.x * windowWidth) / backgroundWidth;
button_y = (_btnData.y * windowHeight) / backgroundHeight;
button_width = (spriteWidth * windowWidth) / (backgroundWidth * 3);
button_height = (spriteHeight * windowHeight) / backgroundHeight;
```

**Example:** On 1920×1440 screen:
- Button at (100, 489) in reference becomes (150, 733)
- Maintains same relative position and proportions

### Rendering Pipeline (Custom Artwork Mode)

**OnPaint() Flow:**

1. **Composite background + buttons:**
   ```cpp
   UpdateBlk();  // Build composite frame in m_pcdibTmp DIB
   m_pcdibTmp->StretchBlt(dc, rect, m_pcdibTmp->GetRect());  // Final blit to screen
   ```

2. **UpdateBlk() - Composite building process:**
   ```cpp
   // Stretch background artwork to temp DIB
   m_pcdibWall->StretchBlt(m_pcdibTmp, windowRect, backgroundRect);

   // For each button, extract and composite button sprite
   for (int i = 0; i < NUM_BTNS; i++) {
       CRect spriteRect = buttonDIB->GetRect();
       int stateWidth = spriteRect.Width() / 3;

       // Select state: disabled (right), pressed (middle), normal (left)
       if (!button->IsWindowEnabled())
           spriteRect.OffsetRect(stateWidth * 2, 0);  // Disabled
       else if (button->GetState() & 0x04)
           spriteRect.OffsetRect(stateWidth, 0);      // Pressed

       // Transparent-blit button into composite
       m_pcdibBtns[i]->StretchTranBlt(m_pcdibTmp, destRect, spriteRect);
   }
   ```

3. **Dynamic Title Rendering:**
   ```cpp
   // Font size auto-scales based on text length
   lf.lfWidth = (3 * (screenWidth / titleLength)) / 4;
   lf.lfHeight = lf.lfWidth * 2;  // Height 2x width
   lf.lfWeight = 800;  // Bold
   strcpy(lf.lfFaceName, "Book Antiqua");

   // Drop shadow effect (3-4 offset draws)
   int shadowShift = lf.lfWidth / 30;
   while (shadowShift--) {
       dc.SetTextColor(PALETTERGB(144, 127, 116));  // Light brown
       dc.DrawText(sTitle, ...);  // Shadow
       rect.top--;
       rect.left -= 2 or 4;  // iJmp based on mode
   }
   dc.SetTextColor(PALETTERGB(90, 74, 57));  // Dark brown
   dc.DrawText(sTitle, ...);  // Final text
   ```

4. **Copyright text placement:**
   ```cpp
   // Bottom-right corner with auto-sizing
   dc.DrawText(copyright, -1, &rect, DT_CALCRECT | DT_CENTER);
   rect.top = screenHeight - textHeight - margin;
   rect.left = screenWidth - textWidth - margin;
   ```

### Button Rendering (OnDrawItem - Custom Artwork Mode)

**3-State Sprite System:**

Button sprites are stored as horizontal strips with 3 consecutive states:

```
[Normal State] [Pressed State] [Disabled State]
   (width/3)       (width/3)        (width/3)
```

**State Selection:**

```cpp
int stateWidth = spriteWidth / 3;

if (!IsWindowEnabled())
    spriteRect.OffsetRect(stateWidth * 2, 0);  // Jump to disabled (right)
else if (GetState() & 0x04)
    spriteRect.OffsetRect(stateWidth, 0);      // Jump to pressed (middle)
// else normal - no offset (left)
```

**Pressed State Text Offset:**

When button pressed, text shifts to indicate 3D depth:

```cpp
if (itemState & ODS_SELECTED) {
    rect.OffsetRect(
        (buttonWidth * ((ptDnOff.x - stateWidth) - textRect.left)) / stateWidth,
        (buttonHeight * (ptDnOff.y - textRect.top)) / spriteHeight
    );
}
```

**Text Fitting Algorithm:**

```cpp
// Initial font size based on button height
LOGFONT lf;
lf.lfHeight = (5 * rect.Height()) / 4;
lf.lfWeight = 400;
strcpy(lf.lfFaceName, "Book Antiqua");

// Test if text fits
pDc->DrawText(sText, -1, &testRect, DT_CALCRECT | fmt);

// If doesn't fit, shrink font incrementally
if (testRect exceeds button) {
    while (lf.lfHeight > 10) {
        lf.lfHeight--;
        pDc->SelectObject(&font);
        pDc->DrawText(..., DT_CALCRECT);
        if (fits) break;
    }
}
```

### Button Rendering (OnDrawItem - Tiled Mode)

For fallback tiled mode, uses Windows 3D beveled buttons:

```cpp
// Create brushes for 3D effect
brBottom = PALETTERGB(38, 46, 49);   // Dark edge
brFace = PALETTERGB(70, 86, 82);     // Center
brTop = PALETTERGB(103, 127, 121);   // Light edge

// Draw with bevel effect
if (selected)
    PaintBevel(dc, rect, 6, brBottom, brTop);  // Inverted (pressed)
else
    PaintBevel(dc, rect, 6, brTop, brBottom);  // Normal (raised)

// Fill center
rect.InflateRect(-6, -6);
pDc->FillRect(&rect, &brFace);
```

### Initialization & Art Loading

**OnCreate() - Load artwork:**

```cpp
CMmio* pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
pMmio->DescendRiff('M', 'I', 'S', 'C');

try {
    // Try to load custom artwork
    pMmio->DescendList('M', 'N', bps[0], bps[1]);  // 'MN' = main menu
    pMmio->DescendChunk('D', 'A', 'T', 'A');
    m_pcdibWall->Load(*pMmio);

    // Load 11 button sprite strips
    for (int i = 0; i < NUM_BTNS; i++) {
        pMmio->DescendChunk('D', 'A', 'T', 'A');
        m_pcdibBtns[i]->Load(*pMmio);
        pMmio->AscendChunk();
    }
    m_bTile = FALSE;
} catch (...) {
    // Artwork not found, use tiled fallback
    m_bTile = TRUE;
    pMmio->DescendList('W', 'L', bps[0], bps[1]);  // 'WL' = wall tile
    pMmio->DescendChunk('D', 'A', 'T', 'A');
    m_pcdibWall->Load(*pMmio);
}
```

**OnInitDialog() - Button states:**

```cpp
// Shareware restrictions
if (theApp.IsShareware() || theApp.IsSecondDisk()) {
    GetDlgItem(IDC_MAIN_LOAD)->EnableWindow(FALSE);
    GetDlgItem(IDC_MAIN_LOAD_MUL)->EnableWindow(FALSE);
}
if (theApp.IsSecondDisk())
    GetDlgItem(IDC_MAIN_CREATE)->EnableWindow(FALSE);

// Disable if intro movie not available
if (!theApp.HaveIntro())
    GetDlgItem(IDC_MAIN_INTRO)->EnableWindow(FALSE);

// Resize buttons for screen if tiled mode
if (m_bTile) {
    for (int i = 0; i < NUM_BTNS; i++) {
        button->SetWindowPos(...,
            oldWidth / 2 + (screenX * oldWidth) / 2560,
            oldHeight / 2 + (screenY * oldHeight) / 2048, ...);
    }
}
```

### Main Menu Rendering Gotchas

**1. Silent fallback to tiled mode if art missing**
```cpp
try {
    pMmio->DescendList('M', 'N', ...);  // Try custom art
} catch (...) {
    m_bTile = TRUE;  // Silent switch to tiled
}
```

**GOTCHA:** If artwork file corrupted or missing, no error shown. Menu appears with basic tiled look but user won't know why artwork didn't load.

**2. Button positions hardcoded for 1280×768 reference**
All button coordinates in `_btnData` assume background artwork is exactly 1280×768. If artwork is different size, button positioning will be off proportionally.

**3. Sprite strip assumption - must be exactly 3 states**
```cpp
int stateWidth = spriteWidth / 3;
```

**GOTCHA:** Code divides sprite width by 3 without validation. If button sprite width not divisible by 3, state selection gives fractional offset and renders wrong sprite section.

**4. Text rect (rText) vs clickable area mismatch**
`rText` specifies where text draws, but Windows handles clickable button area separately. Text could draw outside button bounds.

**5. Pressed state offset calculation**
```cpp
rect.OffsetRect((rPos.Width() * ((ptDnOff.x - iWid) - rText.left)) / iWid, ...);
```

This subtracts `iWid` (which is width/3) from `ptDnOff.x`. If formula incorrect, button text won't shift properly when clicked.

**6. UpdateBlk() called every paint - expensive operation**
```cpp
void OnPaint() {
    UpdateBlk();  // Recalculates and reblits all buttons every frame
}
```

**GOTCHA:** All button states recalculated and redrawn even if nothing changed. Could optimize with dirty flag - only UpdateBlk() when button state changed.

**7. Font sizing algorithm can get stuck**
```cpp
while (lf.lfHeight > 10) {
    lf.lfHeight--;
    // Test if fits
}
```

If text is too long and even size-10 font doesn't fit, loop ends with font at 10 and text renders clipped. No error or warning.

**8. Title font size depends on text length**
```cpp
lf.lfWidth = (3 * (screenWidth / titleLength)) / 4;
```

If title text changes between versions (e.g., localization), font size changes. Long titles could overflow.

**9. EnableWindow() doesn't visually update buttons immediately**
```cpp
GetDlgItem(IDC_MAIN_LOAD)->EnableWindow(FALSE);
```

Button appears disabled only if OnDrawItem called. Until WM_PAINT or button click, old appearance stays visible.

**10. OnSize() doesn't trigger repaint in custom art mode**
OnSize() only repositions buttons, doesn't call InvalidateRect(). If window resized, user must trigger paint for new layout to appear (e.g., minimize/restore).

**11. Tiled mode button sizing uses magic formula**
```cpp
newWidth = oldWidth / 2 + (screenX * oldWidth) / 2560;
```

Constants 2560 and 2048 suggest 2560×2048 or 5×2 aspect ratio. If screen dimensions way outside expected range, buttons could size to 0 or huge.

**12. No bounds checking on button coordinates**
Button positions calculated but never validated to be on-screen. Off-by-one errors in `_btnData` could position button off-screen without warning.

---

## PAUSE MENU - Multiplayer Game Pause Dialog

**Files:** main.cpp (lines 1430-1497), ui.h (lines 290-324)

The pause menu (CDlgPause) is a simple modeless dialog that displays when multiplayer games are paused. It shows different messages depending on whether the player is the host/server or a client.

### Architecture

**Modeless Dialog - Does Not Block Input:**
- Created once, reused via Show()/Hide()
- Stays on top of main window
- No blocking modal behavior

### Rendering System

**Simple Text Display:**
```cpp
class CDlgPause : public CDialog {
    // Dialog Data
    CString m_sText;  // Single text field
    enum { IDD = IDD_PAUSE_MSG };
};
```

**Show() Method - Three modes:**

```cpp
void CDlgPause::Show(int iMode) {
    if (m_hWnd == NULL)
        Create(IDD_PAUSE_MSG, &theApp.m_wndMain);  // Create if needed

    switch (iMode) {
    case server:  // Host paused the game
        m_sText.LoadString(IDS_PAUSE_SERVER);  // "Game paused by host"
        CenterWindow();
        ShowWindow(SW_SHOW);
        SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        theApp.m_wndMain.SetActiveWindow();
        break;

    case client:  // Waiting for host to unpause
        m_sText.LoadString(IDS_PAUSE_CLIENT);  // "Waiting for host..."
        CenterWindow();
        ShowWindow(SW_SHOW);
        SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        theApp.m_wndMain.SetActiveWindow();
        break;

    default:
        ShowWindow(SW_HIDE);  // Hide pause dialog
        break;
    }
}
```

### Positioning & Layout

- **Centered on screen** - CenterWindow() called each time
- **Always on top** - SetWindowPos(&wndTop, ...)
- **Fixed size** - Resource-defined (IDD_PAUSE_MSG)
- **Main window stays active** - SetActiveWindow() ensures focus returns

### Pause Menu Gotchas

**1. Dialog recreated if not created yet**
```cpp
if (m_hWnd == NULL)
    Create(IDD_PAUSE_MSG, &theApp.m_wndMain);
```

**GOTCHA:** First call to Show() has latency creating dialog. Subsequent calls reuse existing window. Could cause noticeable pause on first pause.

**2. PostNcDestroy() calls delete this**
```cpp
virtual void PostNcDestroy() { delete this; }
```

**GOTCHA:** Dialog deletes itself when closed. If Show() called after close, m_hWnd will be invalid and new dialog created. Self-deleting dialogs are error-prone.

**3. CenterWindow() called every Show()**
```cpp
CenterWindow();  // Called each time
```

**GOTCHA:** If window manager or screen resolution changed between Show() calls, dialog centers wrong. Expensive operation called too often.

**4. Main window set active after showing pause**
```cpp
theApp.m_wndMain.SetActiveWindow();
```

**GOTCHA:** This may not be respected if pause dialog is already focused. Race condition between dialog creation/show and SetActiveWindow().

---

## UNIT STATUS WINDOW - Real-Time Unit Information Display

**Files:** area.h (lines 36-59), area.cpp (lines 494-556), new_unit.cpp (lines 324-405)

The unit status window (CWndUnitStat) displays real-time information about selected units in the game world. It shows unit name, damage, materials, and other attributes using a custom icon-based rendering system.

### Architecture

**Window Hierarchy:**
```
CWndAreaStatic (toolbar/status bar area)
└── CWndUnitStat (inherits from CWndStatBar)
    └── Renders unit information using CStatInst objects
```

**Core Classes:**
- **CWndUnitStat** - Window that displays unit information
- **CStatInst** - Individual status icon/bar renderer (used multiple times)
- **CUnit** - The unit being displayed

### Rendering System

**Unit Status Display (_UnitShowStatus):**

```cpp
void _UnitShowStatus(BOOL bText, void* pData, CDC* pDc,
                     CRect const& rDraw, CDIB* pDibBack, CPoint const& ptOff)
{
    CUnit* pUnit = (CUnit*)pData;
    CRect rect(rDraw);

    // Part 1: Unit description/name (if bText = TRUE)
    if (bText) {
        uShowStat.m_siText.SetBack(pDibBack, ptOff);
        int iWid = 14 * 8 + leftOff + rightOff;
        iWid = __min(rect.Width() / 2, iWid);  // Max 50% of window
        rect.right = rect.left + iWid;

        uShowStat.m_siText.SetSize(rect);
        uShowStat.m_siText.SetText(pUnit->GetData()->GetDesc());
        uShowStat.m_siText.DrawIcon(pDc);

        rect.left = rect.right;  // Move right for remaining space
    }

    // Part 2: If enemy unit - name & damage only
    if (!pUnit->GetOwner()->IsMe()) {
        // Owner name (left half of remaining space)
        CStatInst* pSi = &uShowStat.m_si[1][0];
        rect.right = rect.left + (rDraw.right - rect.left) / 2;
        pSi->SetText(pUnit->GetOwner()->GetName());
        pSi->DrawIcon(pDc);

        // Damage bar (right half)
        rect.left = rect.right + 1;
        pSi++;
        pUnit->PaintStatusBars(pSi, 0, pDc);
        return;
    }

    // Part 3: If own unit - show all status bars
    int iNum = pUnit->GetNumStatusBars();  // Usually 3+ bars

    // Evenly divide remaining space
    for (int i = 0; i < iNum; i++) {
        rect.left = rect.right + 1;
        rect.right += (rDraw.right - rect.left) / (iNum - i);

        uShowStat.m_si[i][0].SetSize(rect);
        pUnit->PaintStatusBars(&uShowStat.m_si[i][0], i, pDc);
    }
}
```

### Layout & Positioning

**Three-Part Horizontal Layout:**

```
Unit Description (14 chars max) | Enemy Info     | Own Unit Status Bars
(if bText=TRUE, max 50% width)   | Name | Damage | Health | Materials | ...
                                 | (for enemies) | (for own units - equal widths)
```

**Dynamic Width Calculation:**
- Description: 14 characters × character width (capped at 50% of window)
- Enemy display: 2 equal sections (name, damage)
- Own unit: N equal sections (health, materials, power, etc.)

### Mouse Move Handling

**OnMouseMove - Real-time Status Text Update:**

```cpp
void CWndUnitStat::OnMouseMove(UINT, CPoint) {
    if (m_pUnit == NULL)
        CWndStatBar::SetText(NULL);
    else {
        CString str;
        ::UnitStatusText(m_pUnit, str);  // Generate status text
        theApp.m_wndBar.SetStatusText(1, str);  // Update status bar
    }
}
```

Updates status text in real-time as mouse moves over unit window.

### Unit Status Window Gotchas

**1. CStatInst objects are static globals**
```cpp
uShowStat.m_siText, uShowStat.m_si[iNum][0]
```

**GOTCHA:** Shared global state. If multiple unit windows render simultaneously, they interfere with each other. Not thread-safe.

**2. Dynamic space division can leave gaps**
```cpp
int iWid = 14 * 8 + leftOff + rightOff;
iWid = __min(rect.Width() / 2, iWid);
```

**GOTCHA:** If window is narrow, description gets clamped but remaining space calculation might be off, causing visual artifacts.

**3. Status bar count varies by unit type**
```cpp
int iNum = pUnit->GetNumStatusBars();  // Returns 1, 2, or 3+
```

**GOTCHA:** Different unit types have different numbers of status bars (building vs vehicle). Layout adjusts but could cause jarring visual changes when selecting different unit types.

**4. OnPaint() called for every mouse move on window**
```cpp
void CWndUnitStat::UpdateStat() {
    if (CWnd::WindowFromPoint(pt) == this)
        OnMouseMove(0, pt);  // Triggers paint
}
```

**GOTCHA:** Mouse movement = repaint. High frequency redraws if mouse hovers. Could be expensive if CUnit data retrieval is slow.

**5. Background DIB must be set correctly**
```cpp
uShowStat.m_siText.SetBack(pDibBack, ptOff);
```

**GOTCHA:** If pDibBack or ptOff are incorrect, status icons render with wrong background. Alignment errors hard to debug.

**6. Enemy vs own unit logic changes entire layout**
```cpp
if (!pUnit->GetOwner()->IsMe()) {
    // 2-part layout
} else {
    // N-part layout
}
```

**GOTCHA:** Ownership check determines layout. If ownership changes unexpectedly, display changes dramatically. No transition animation.

---

## NEW GAME SYSTEM - Game Creation & Configuration UI

**Files:** new_game.h (lines 1-251), new_game.cpp, newworld.cpp

The new game system handles creating single-player and multiplayer games. It consists of multiple interconnected dialogs that guide players through scenario selection, difficulty settings, player setup, and game initialization.

### Architecture

**Game Type Enumeration:**
```cpp
enum game_type {
    scenario,       // Campaign scenario
    single,         // Single-player skirmish
    create_net,     // Host multiplayer game
    join_net,       // Join multiplayer game
    load_single,    // Load single-player save
    load_multi,     // Load multiplayer save
    load_join,      // Load and join multiplayer
    num_types
};
```

**Class Hierarchy:**
```
CCreateBase (abstract base)
├── CMultiBase (multiplayer support)
│   └── CJoinMulti (join/host logic)
├── CCreateNewBase (new game dialogs)
│   └── CDlgPickRace (race selection)
└── CCreateLoadBase (load game dialogs)
    └── CDlgPickPlayer (player selection)
```

### Main Dialog Classes

**CDlgCreateStatus** - Progress/Status display during game creation:
- Shows progress bar during world generation/loading
- Displays status messages (IDD_CREATE_STATUS)
- Cancel button to abort creation
- Modeless - doesn't block UI

**CDlgPlayerList** - Multiplayer player management:
```cpp
class CDlgPlayerList : public CDialog {
    CListBox m_lstPlayers;      // List of connected players
    CString m_sAddr;            // Connection address
    CString m_sVpVer;           // Network protocol version
    CWndOD<CButton> m_btnDelete; // Remove player (host only)
    CWndOD<CButton> m_btnOk;    // Start game
    BOOL m_bServer;             // TRUE if host/server
};
```

**Game Info Structure** (for multiplayer join):
```cpp
class CGameInfo : public CObject {
    int m_iNumOpponents;        // Number of AI opponents
    int m_iAIlevel;            // AI difficulty (0-3)
    int m_iWorldSize;          // World size (0-2)
    int m_iPos;                // Starting position
    int m_iNumPlayers;         // Total player positions
    char m_cFlags;             // Game flags
    CString m_sName;           // Game name
    CString m_sDesc;           // Game description
    VPSESSIONID m_ID;          // Network session ID
};
```

### New Game Creation Flow

**Single-Player New Game:**
1. CDlgCreateStatus shows "Creating World..."
2. NewWorld.cpp generates terrain procedurally
3. Game initializes with player, AI opponents
4. Transitions to main game view (CWndArea)

**Multiplayer Host (create_net):**
1. Player configures game (name, difficulty, size)
2. CDlgPlayerList shows waiting for players to join
3. Host can delete players, set "ready"
4. When ready, host clicks OK to start game
5. Game broadcasts start command to all clients

**Multiplayer Join (join_net):**
1. CDlgJoinPublish lists available games
2. Player selects game, joins
3. Appears in host's player list
4. Waits for host to start
5. Receives game state from host

### New Game Rendering Gotchas

**1. CDlgCreateStatus is modeless - doesn't block**
```cpp
class CDlgCreateStatus {
    virtual void PostNcDestroy() { delete this; }
};
```

**GOTCHA:** Dialog self-deletes when closed. If creation fails/cancelled, dialog destroyed but creation thread might still run, orphaning it.

**2. World generation is synchronous in main thread**
During world creation, UI freezes. Large worlds (size 2) can freeze for 10+ seconds.

**GOTCHA:** No background thread or progress granularity. No way to show per-step progress.

**3. Player list rendering depends on network status**
```cpp
void CDlgPlayerList::UpdatePlyrStatus(CPlayer* pPlyr, int iStatus) {
    // Redraw player list with new status
}
```

**GOTCHA:** If network drops, player status updates fail silently. No error indication in list.

**4. Game info (CGameInfo) passed by reference**
Multiple dialogs reference same CGameInfo. Modifications affect shared state.

**GOTCHA:** If info modified unexpectedly (network message), all dialogs show updated info. No dirty flag or re-sync protection.

**5. CDlgPlayerList only shows for multiplayer**
Single-player games skip player list entirely.

**GOTCHA:** UI flow completely different between single/multi. Players might be confused where to click.

---

## MULTIPLAYER JOIN/HOST - Network Game Discovery & Connection

**Files:** join.h (lines 1-250), join.cpp, creatmul.inl (included in multiple files)

The multiplayer join/host system handles network game discovery, hosting, and joining. It supports multiple network protocols (TCP, IPX, NetBIOS, DirectPlay, Modem, TAPI, Serial).

### Architecture

**Protocol Support:**
```cpp
const int NUM_PROTOCOLS = 7;
const int aPr[NUM_PROTOCOLS] = {
    VPT_TCP,      // TCP/IP
    VPT_IPX,      // IPX
    VPT_NETBIOS,  // NetBIOS
    VPT_DP,       // DirectPlay
    VPT_MODEM,    // Modem
    VPT_TAPI,     // TAPI (telephony)
    VPT_COMM      // Serial port
};
```

**Game Publishing (Host):**

**CDlgJoinPublish** - Publish/advertise hosted game:
```cpp
class CDlgJoinPublish : public CDialog {
    CEdit m_StrName;           // Game name text field
    CString m_strPw;           // Password
    int m_NetRadio;            // Selected protocol radio button
    CWndOD<CButton> m_btnAdv;  // Advanced settings button
    CWndOD<CButton> m_btnUnPublish; // Stop hosting button
};
```

**Features:**
- Player enters game name
- Optionally password-protect
- Select network protocol
- Publish to network for others to discover
- Unpublish to stop accepting joins

**Game Discovery (Joiner):**

**CDlgJoinGame** - Browse and join existing games:
```cpp
class CDlgJoinGame : public CDialog {
    CListBox m_lstGames;       // List of discovered games
    CGameInfo m_SelectedGame;  // Info about selected game
    // Shows game name, num players, difficulty, size
};
```

**Features:**
- Displays list of published games
- Shows game details (players, difficulty, map size)
- Player selects game and clicks Join
- Joins multiplayer session

### Network Game Flow

**Host (Publish) Flow:**
```
CDlgJoinPublish Created
    ↓
Enter game name, select protocol
    ↓
Click OK → Publish to network
    ↓
Wait in CDlgPlayerList for players to join
    ↓
When players join:
    - CMultiBase::AddPlayer(CPlayer*)
    - CDlgPlayerList updated with new player
    - Display shows player name, status
    ↓
Host clicks "Start Game" in player list
    ↓
Broadcast game start to all clients
    ↓
Transition to gameplay
```

**Client (Join) Flow:**
```
CDlgJoinGame Created
    ↓
Browse network for published games
    ↓
CJoinMulti::OnSessionEnum(LPCVPSESSIONINFO)
    - Called for each discovered game
    - Adds to available games list
    ↓
Player selects game, clicks Join
    ↓
CJoinMulti::GameLoaded(void* pBuf, int iLen)
    - Receives game state from host
    ↓
Transition to gameplay with loaded state
```

### CJoinMulti Class

**Main multiplayer join controller:**

```cpp
class CJoinMulti {
    void Init();                    // Initialize join system
    void ClosePick();              // Close dialogs
    void CloseAll();               // Clean up all
    void GameLoaded(void* pBuf, int iLen);  // Game data received

    CDlgJoinPublish m_dlgJoinPublish;  // Host dialog
    CDlgJoinGame m_dlgJoinGame;        // Join dialog

    virtual void OnSessionEnum(LPCVPSESSIONINFO);  // New game found
    virtual void OnSessionClose(LPCVPSESSIONINFO); // Game closed/full
};
```

**Key Methods:**
- **Init()** - Set up as client, load data, show publish/join dialogs
- **ClosePick()** - Close either dialog
- **CloseAll()** - Clean up all dialogs and multiplayer state

### Multiplayer Rendering Gotchas

**1. Multiple protocol selection can confuse users**
```cpp
int m_NetRadio;  // Radio button for protocol selection
```

**GOTCHA:** 7 different network protocols available. Non-technical users don't know which to pick. Wrong choice = games can't find each other.

**2. Game list updates asynchronously**
```cpp
virtual void OnSessionEnum(LPCVPSESSIONINFO);
```

**GOTCHA:** Games appear/disappear as discovered. If player is reading list while it updates, visual glitching can occur. No locking mechanism visible.

**3. Password-protected games accepted but silently fail**
Player might join wrong password-protected game without error.

**GOTCHA:** No feedback if join fails due to password. Player left waiting indefinitely.

**4. Game info (CGameInfo) passed around by reference**
```cpp
CGameInfo m_SelectedGame;
```

**GOTCHA:** Multiple dialogs hold references to same info. Updates from network don't refresh all displays. Stale data shown to user.

**5. CDlgJoinPublish and CDlgJoinGame are mutually exclusive**
Only one visible at a time (host OR join, not both).

**GOTCHA:** Complex state management. Switching between host/join requires closing/reopening dialogs.

**6. Unpublish doesn't disconnect existing players**
```cpp
m_btnUnPublish.Click()  // Only stops new joins
```

**GOTCHA:** Existing players still connected but can't be seen in player list. Confusing UI state.

**7. Advanced settings button (_btnAdv) hidden complexity**
```cpp
CWndOD<CButton> m_btnAdv;  // Advanced settings
```

**GOTCHA:** Important settings (bandwidth, timeout, retry count) buried in "Advanced". Most users never find them.

**8. Session enumeration might hang**
Network protocol issues could cause enumeration to block UI.

**GOTCHA:** No timeout on game discovery. User sits with empty list, unclear if searching or stalled.

---

## WATER & BOATS - Ocean & Naval Unit Rendering System

**Files:** terrain.h (lines 85-146), terrain.cpp (lines 2390-2456), vehicle.h (lines 135-189), building.h (lines 263-289), area.cpp (lines 2459-2500)

The water and boat system handles ocean terrain rendering, boat movement, and shipyard-based naval units. It includes sophisticated coastline rendering with feathering, water depth tracking, and boat-specific building exits.

### Water System Architecture

**Altitude System for Water:**

```cpp
enum {
    OCEAN_FLOOR = 1,     // Deepest water
    SEA_LEVEL = 16,      // == 16 → ocean, == 17 → land (threshold)
    AVERAGE = 44,        // Sea level + 28 altitude
    MOUNTAIN_TOP = 96,
    MAX = 104
};
```

**Water Detection:**
```cpp
int GetSeaAlt() const;   // Get water surface altitude
int GetAlt() const;      // Get terrain altitude
BOOL IsWater() const;    // Check if this hex is water
```

**Key Insight:** Water altitude and land altitude are different. Boats move at SEA_LEVEL (16), while land vehicles navigate land altitude. Water depth = (vehicle water capability) vs (SEA_LEVEL - land altitude).

### Coastline System

**13 Coastline Types (COASTLINE enum):**

```cpp
enum COASTLINE {
    // 1/4 land corners (water around 3 sides)
    LAND_UL, LAND_UR, LAND_LR, LAND_LL,

    // 1/2 land edges (water around 2 sides)
    LAND_UP, LAND_RT, LAND_DN, LAND_LF,

    // 1/4 water corners (land around 3 sides)
    WATER_UL, WATER_UR, WATER_LR, WATER_LL,

    // Island (all water neighbors)
    ISLAND
};
```

**Feathering - Smooth Coastal Transitions:**

```cpp
// Coastline feathering prevents hard transitions between water/land
if (coastline == iType && coastline != iTypeNeighbor)
    aeFeather[i] = FEATHER_IN;      // Feather INTO coastline
else if (coastline == iTypeNeighbor && coastline != iType)
    aeFeather[i] = FEATHER_OUT;     // Feather OUT from coastline
else if (coastline != iTypeNeighbor && coastline != iType)
    aeFeather[i] = FEATHER_INOUT;   // Full transition
```

**Feathering Process:**
1. Check each of 4 neighbor hexes
2. If neighbor is coastline but this hex isn't (or vice versa), apply feathering
3. 3 feather types smooth edges: IN (darkening), OUT (lightening), INOUT (full blend)
4. Prevents jarring visual transitions at water/land boundaries

### Boat System

**Boat Classification:**

```cpp
class CTransportData {
    BOOL IsBoat() const;         // Is this a naval unit?
    BOOL IsCarrier() const;      // Can it carry other units?
    BOOL IsLcCarryable() const;  // Can it be carried by landing craft?

    int GetWaterDepth() const;   // How deep can it go (in altitude units)?
    int GetWheelType() const;    // WHEEL_TYPES enum

    enum WHEEL_TYPES {
        WALK = 0,    // Infantry
        WHEEL = 1,   // Wheeled vehicles
        TRACK = 2,   // Tracked vehicles
        HOVER = 3,   // Hovercraft
        WATER = 4    // Boats
    };

    enum TRANS_FLAGS {
        FLboat = 0x02,           // Is a boat
        FLcarrier = 0x04,        // Can carry units
        FLlc_carryable = 0x80    // Landing craft carryable
    };
};

static int m_iMaxDraft;  // Maximum water depth any boat needs
```

### Shipyard & Ship Exits

**Buildings with Water Access:**

```cpp
class CStructureData {
    BOOL HasShipExit() const;        // Can ships enter/exit?
    BOOL IsPartWater() const;        // Must be on both land and water?

    CHexCoord GetShipHex() const;    // Naval unit entry/exit point
    int GetShipDir() const;          // Direction boats approach from

    CBuildShipyard* GetBldShipyard() const;  // If is a shipyard
};
```

**Key Difference:** Land units use GetExitHex(), boats use GetShipHex(). Shipyards have separate water-side entries for boats.

### Boat Movement & Docking

**Boat Behavior in area.cpp:**

```cpp
// Repair dock - boat vs land vehicle
if (((CVehicle*)pUnit)->GetData()->IsBoat())
    SetDestAndSfx((CVehicle*)pUnit, ((CBuilding*)pUnitOn)->GetShipHex());
else
    SetDestAndSfx((CVehicle*)pUnit, ((CBuilding*)pUnitOn)->GetExitHex());

// Load units into carrier - boats have special cargo
if (((CVehicle*)pUnitOn)->GetData()->IsBoat() &&
    ((CVehicle*)pUnit)->GetData()->IsLcCarryable()) {
    pUnit->ResumeUnit();
    ((CVehicle*)pUnit)->SetEvent(CVehicle::load);
    ((CVehicle*)pUnit)->SetLoadOn((CVehicle*)pUnitOn);
}
```

**Boat Features:**
- **Ship Hexes:** Boats dock at separate water-side hexes, not land-side exits
- **Landing Craft:** Boats can carry specialized landing craft units (amphibious operations)
- **Water Depth Checking:** Before movement, system validates water depth at destination

### Water Terrain Rendering

**Terrain Drawing with Coastal Feathering:**

```cpp
// In CHex::Draw()
CTerrainDrawParms::FEATHER_TYPE aeFeather[4] = {
    FEATHER_NONE, FEATHER_NONE, FEATHER_NONE, FEATHER_NONE
};

// For each of 4 neighbors
for (int i = 0; i < 4; ++i) {
    CHex* phexNeighbor = theMap.GetHex(neighborCoord);
    int iTypeNeighbor = phexNeighbor->GetSpriteID();

    // Determine feathering based on coastline neighbors
    if (coastline == iType && coastline != iTypeNeighbor)
        aeFeather[i] = FEATHER_IN;
    else if (coastline == iTypeNeighbor && coastline != iType)
        aeFeather[i] = FEATHER_OUT;
    else if (coastline != iTypeNeighbor && coastline != iType)
        aeFeather[i] = FEATHER_INOUT;
}

CTerrainDrawParms drawparms(*this, hexcoord, bDrawVert, bShade, aeFeather);
GetSprite()->GetView(xiDir, 0)->Draw(drawparms);
```

### Water & Boat Rendering Gotchas

**1. Coastline feathering complexity**
```cpp
if (coastline == iType && coastline != iTypeNeighbor)
    aeFeather[i] = FEATHER_IN;
```

**GOTCHA:** 4 different feathering conditions must be checked. Off-by-one in neighbor checking could cause missing feathering. Hard to debug because feathering is gradual (not obviously wrong).

**2. Water depth calculation is complex**
```cpp
// Boat can travel if:
// GetWaterDepth() > SEA_LEVEL - pHex->GetAlt()
```

**GOTCHA:** Subtraction could underflow if altitude > SEA_LEVEL. No validation shown. Shallow water hexes might be unreachable for deep-draft ships.

**3. Ship exits separate from land exits**
```cpp
if (IsBoat())
    SetDest(..., GetShipHex());
else
    SetDest(..., GetExitHex());
```

**GOTCHA:** If shipyard building has both land and water exits, wrong exit used = boat stuck unable to dock. GetShipHex() vs GetExitHex() confusion could cause mission-breaking bugs.

**4. Landing craft carryable flag must match**
```cpp
if (((CVehicle*)pUnitOn)->GetData()->IsBoat() &&
    ((CVehicle*)pUnit)->GetData()->IsLcCarryable()) {
    // Can load
}
```

**GOTCHA:** Only units with explicit IsLcCarryable() flag can load into boats. Regular carryable units can't. If flag not set correctly, players can't load units into boats.

**5. Feathering only applied if not road/resource**
```cpp
if (road != iType && resources != iType && city != iType && ...) {
    // Apply feathering
}
```

**GOTCHA:** If a road crosses water terrain, feathering disabled. Road/water intersections could look jarring.

**6. Max draft stored as static class variable**
```cpp
static int m_iMaxDraft;  // Shared across all boats
```

**GOTCHA:** Max draft is global, not per-scenario. If mod changes boat designs, old max draft value stays cached.

**7. GetSeaAlt() vs GetAlt() confusion**
Terrain system has both water and land altitude. Easy to use wrong one for water operations.

**GOTCHA:** Calling GetAlt() on water hex returns land altitude (not water surface). Developer must remember to use GetSeaAlt() for water logic.

**8. Coastline type calculation during world generation**
Coastline type determined by 4 neighbors' water status. Procedural generation could create invalid coastline combinations.

**GOTCHA:** Non-adjacent water patches could create isolated coastline hexes that look wrong (e.g., LAND_UL with no actual land corners visible).

**9. Boat rendering layering**
Boats rendered with vehicles layer. If water hex has terrain feathering, boat appears to be "on top" incorrectly.

**GOTCHA:** Visual glitching where boats appear to float above coastline because feathering doesn't apply to boat sprites.

**10. Ship hex must be water**
No validation that GetShipHex() actually points to a water hex.

**GOTCHA:** If scenario designer makes shipyard on all-land, GetShipHex() points to water-less area. Boats can't dock/repair.

---

## Porting/Replacement Considerations (For SDL Replacement)

When replacing with SDL:

1. **Global state xiZoom/xiDir/xpdibwnd must become thread-local or encapsulated**
   - Current code breaks if more than one thread renders

2. **Palette-based rendering is NO LONGER NECESSARY**
   - SDL supports full RGBA
   - But code may rely on palette tricks for performance

3. **WinG BitBlt operations need replacement**
   - Current code uses fast bitmap blitting
   - SDL surface blitting should handle this

4. **Feathering uses fixed random tables**
   - These would need to be replicated or recalculated in SDL

5. **Clipping viewport (prVp) needs validation**
   - Should assert prVp is valid before use in new system

6. **Invalidation system is core to performance**
   - Must be preserved or system will redraw entire screen every frame

7. **Coordinate transformation code can stay mostly as-is**
   - But needs testing for off-by-one errors

8. **Multi-window support with separate DIBs**
   - SDL can handle multiple surfaces/windows

9. **Cursor handling** - Use SDL cursor APIs properly

10. **Frame rate throttling** - SDL provides timing functions

