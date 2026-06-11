# GPU radar/world-map plan (SDL2 renderer API only — no direct GL)

Goal: radar steady-state render cost **<1ms/frame** (user: "5ms is a long time"),
replacing the per-window-pixel CPU walk entirely. Current state after the
input-gate fix (`3370697`): ~57ms/s under load, ~0 idle, walk still O(window
pixels) whenever fog/buildings/centre change.

## Architecture

One **map-resolution texture** (eX × eY texels, 1024² on Large = 4MB ARGB,
`SDL_TEXTUREACCESS_STREAMING` or static + UpdateTexture) mirrored by a CPU
shadow buffer (`std::vector<uint32_t>`). One texel = one hex's radar color.

Per-texel color precedence (exact port of the walk, world.cpp ~1403-1472):
1. building on hex && visible && mode-allowed → owner `GetPalColor()`
   (hit-flash: `m_clrHit` while `pBldg->m_iFrameHit != 0`)
2. minerals on hex && resources mode → `m_clrResources[type]` (copper gated by
   `CanCopper()`; highlight cycle `m_iResOn` brightens one class —
   `m_clrResHigh[]` in the radar variant, world.cpp ~1989-2004)
3. else terrain: `m_clrTerrain[pHex->GetVisibleType()]` (radar palette) or
   `m_clrTerrainPaper[...]` (world map parchment palette)
4. fog-of-war: hex not visible → darken the texel ~50% (the CPU walk's
   checkerboard at 1024-texels-into-505px ≈ 0.5px/texel aliases to a 50% dim
   anyway — encode the dim directly, visually equivalent)

### Incremental updates (the whole point — O(changes), never O(pixels))
- **Fog**: hook the visible↔invisible transition choke points
  `CHex::IncVisible/DecVisible` (terrain.inl:210-212 — they already bump
  `g_enFogVisGen` on exactly the right transitions). Pattern:
  `extern bool g_enRadarFogOn; extern void EnRadarFogDirtyHex(const CHex*);`
  one predicted branch when off. Hex position from pointer arithmetic vs
  theMap's hex array base (CHex doesn't know its coords). Push into a mutexed
  vector (copy the g_enEditedHexes/g_enEditMutex pattern in SDL2Terrain.cpp).
- **Terrain edits**: piggyback the existing choke point SDL2Terrain.cpp:89
  (RecordHexEdit / ++g_enTerrainEditGen — has coords already).
- **Buildings**: NO hooks — re-stamp from theBuildingMap (650 entries ×
  footprint texels) on a ~250ms cadence; cheap, covers add/remove/ownership/
  hit-flash decay.
- **Minerals**: stamp once at load + full re-stamp on `m_iResOn` cycle change
  (15k deposits, rare event).
- **Initial build**: one full map scan at load (1M hexes, ~10-30ms, hidden in
  load time like the terrain underlay burst).

Per frame: apply dirty texels to shadow → one `SDL_UpdateTexture` of the dirty
bounding rect → draws below. Zero per-frame CPU when nothing changed.

### Drawing (radar panel's own renderer)
Hook point: `SDL2Panel::PresentOwn` (SDL2Panel.cpp:998) — the radar panel takes
the simple branch at :1052 (DIB upload + copy + present). New branch (like the
`m_terrainAA` terrain branch above it):
1. DIB copy first (ornate frame art + corner buttons stay CPU-drawn, unchanged)
2. `SDL_RenderCopyEx(mapTex, …, angle, center, flip)` — the rotated square IS
   the diamond; it exactly covers the stale diamond region of the DIB, corners
   outside the rotated quad keep showing the frame. Angle = f(camera dir);
   start with 45° + dir·90° and calibrate against the CPU walk's aInc tables
   (world.cpp ~1809-1831) — there may be a flip; verify by screenshot diff.
3. Unit dots: world.cpp's existing live-dot pass (~2214, already O(units) with
   window coords + owner colors) redirected into `SDL2Radar::AddDot()`; drawn
   as ONE `SDL_RenderFillRects` batch. The white current-view rect overlay:
   same treatment (find its draw site near NewLocation/m_rcLoc).
4. Present.

### Wiring
- New files SDL2Radar.h/.cpp + CMakeLists entry.
- Opt-in `EN_GPURADAR=1` until screenshot-verified at all 4 camera dirs, all
  radar modes (units/resources cycle), world-map variant, then default ON.
- CWndWorld (world.cpp ReRender, radar branch): when active, skip the walk AND
  the CPU dot writes entirely; feed dots to SDL2Radar instead.
- Device-lost: register with the same SDL_RENDER_TARGETS_RESET path as
  SDL2Terrain (NotifyTargetsLost — re-upload shadow).

### Expected cost
Steady state: 1 small UpdateTexture + RenderCopyEx + FillRects ≈ **0.1-0.3ms
per frame** (~2-5ms/s at 20fps), fog-transition bookkeeping O(transitions).
Worst case (full restamp): one-time ~10-30ms, load-hidden.

### Window resize (user note: radar is resizable in-game)
The texture is MAP-space (eX × eY, fixed per map) — window size is irrelevant
to it. A resize only changes the RenderCopyEx dstrect: the GPU rescales the
same texture for free, dots re-project per frame anyway. This beats the CPU
walk, which is O(window pixels) and re-allocates/re-walks per size (NewSize →
m_pdibGround0 etc. — those stay for the world map only). Pixelated look is
expected and accepted ("it DOES render things as pixels") — use
SDL_ScaleModeNearest to keep the crisp pixel aesthetic rather than bilinear
mush; flip to Linear only if the user prefers smoothing.

### Risks / gotchas
- The walk samples SUB-hex (CSubHex, ×2) — one texel per hex halves radar
  sampling resolution; at 505px window over 1024 hexes the window is ALREADY
  undersampled 2:1, no visible loss expected. If edge shimmer appears, make
  the texture 2048² (16MB) — same architecture.
- Radar click→map coords (OnLButtonUp) and the view rect math are separate
  window-space code — unaffected.
- The world-map (non-radar) variant has no live dots and a different palette —
  same module, different palette pointer + no dot pass.
- m_pdibGround0/m_pdibBase/NewDir CPU machinery stays for the world map until
  the world map is migrated too; delete only after both are GPU.
