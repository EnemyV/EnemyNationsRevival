// Temporary AI diagnostics switches. Flip to 0 (or strip the #if blocks) for release.
#ifndef __ENPROBES_H__
#define __ENPROBES_H__

#define EN_AI_PROBES_WAR  0  // war: staging, launches, bridges, bunker, recall
#define EN_AI_PROBES_ECON 0  // economy/logistics/census: trucks, cranes, roads, factories
#define EN_PATH_PROBES    1  // CPathMgr movement-A* exit-mix counters (mpath.* in perf.log; needs EN_PERF=1 at runtime)

#endif
