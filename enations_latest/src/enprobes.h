// Temporary AI diagnostics switches. Flip to 0 (or strip the #if blocks) for release.
#ifndef __ENPROBES_H__
#define __ENPROBES_H__

#define EN_AI_PROBES_WAR  1  // war: staging, launches, bridges, bunker, recall
#define EN_AI_PROBES_ECON 1  // economy/logistics/census: trucks, cranes, roads, factories
#define EN_PERF_PROBES    1  // performance: MAINSTALL/SLOWMSG/SLOWBLDG/SLOWPATH/SLOWROAD stall reporters
#define EN_SAVE_PROBES    0  // save/load mineral flag round-trip ([SAVE store]/[LOAD] + worlddbg.log in the run dir)
#define EN_PATH_PROBES    0  // CPathMgr movement-A* exit-mix counters (mpath.* in perf.log; needs EN_PERF=1 at runtime)
// Per-gameplay-event chatter that used to ship in Release: [REN] panel/renderer
// lifecycle, [mp-plyr] rocket/player events, [BRIDGESRV] bridge accepts, [GDI-AUDIT]
// first-hit-per-callsite. Off for release (operator: "they slow/lagging things,
// they're messy"). NOT gated by this flag, deliberately kept in release builds:
// [WORLDGEN] (2 lines per create - names the map in a bug report), [CREATEEX]
// (fires only on a window-create FAILURE) and [TRAP-REMOVED] (reports a condition
// that used to be fatal).
#define EN_GAMEPLAY_PROBES 1

#define EN_SEEK_SINGLEPASS 1 // SeekOpfor targeting: 1 = single-pass multi-class scan (GetOpForUnitScan), 0 = vanilla per-rung GetOpForUnit ladder

#endif
