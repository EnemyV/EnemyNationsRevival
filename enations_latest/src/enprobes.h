// Temporary AI diagnostics switches. Flip to 0 (or strip the #if blocks) for release.
#ifndef __ENPROBES_H__
#define __ENPROBES_H__

#define EN_AI_PROBES_WAR  0  // war: staging, launches, bridges, bunker, recall
#define EN_AI_PROBES_ECON 0  // economy/logistics/census: trucks, cranes, roads, factories
#define EN_PERF_PROBES    1  // performance: MAINSTALL/SLOWMSG/SLOWBLDG/SLOWPATH/SLOWROAD stall reporters

#define EN_SEEK_SINGLEPASS 1 // SeekOpfor targeting: 1 = single-pass multi-class scan (GetOpForUnitScan), 0 = vanilla per-rung GetOpForUnit ladder

#endif
