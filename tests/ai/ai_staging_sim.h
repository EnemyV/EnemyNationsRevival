// ai_staging_sim.h
//
// A tiny, pure state-machine model of the AI staging-completion loop, so we can
// test BEHAVIOR (does a taskforce complete / stall / launch empty?) -- not just
// the static classification tables. No game code, no threads; faithful to the
// decision in caitmgr.cpp CAITaskMgr::IsStagingCompete (the iType==0 path):
//
//   - iTaskCnt = sum of the four bucket requirements (param[4..7])     :3769-3775
//   - if iTaskCnt == 0: "complete" iff a staging area exists           :3779-3786
//   - else NOT complete unless every wanted bucket has >=1 staged ...  :4019-4025
//   - ... AND total staged >= iTaskCnt                                 :4029-4035

#ifndef AI_STAGING_SIM_H
#define AI_STAGING_SIM_H

#include "ai_staging_logic.h"

namespace aistaging {

// Faithful model of IsStagingCompete (iType==0). req/staged are per-bucket.
inline bool IsStagingComplete(const int req[STAGING_UNITTYPES],
                              const int staged[STAGING_UNITTYPES],
                              bool hasStagingArea) {
    int taskCnt = 0;
    for (int i = 0; i < STAGING_UNITTYPES; ++i) taskCnt += req[i];

    if (taskCnt == 0) return hasStagingArea;        // empty-requirement early-out

    for (int i = 0; i < STAGING_UNITTYPES; ++i)     // at least 1 of each wanted type
        if (req[i] > 0 && staged[i] == 0) return false;

    int total = 0;
    for (int i = 0; i < STAGING_UNITTYPES; ++i) total += staged[i];
    return total >= taskCnt;
}

struct StageResult {
    bool completed;    // staging reached "complete"
    bool emptyLaunch;  // completed with ZERO units staged (degenerate launch)
    int  staged;       // total units staged when it concluded
};

// Drive a taskforce to its conclusion: classifiable units "arrive" one per tick
// (input order). Stop when complete, or when no un-staged classifiable unit
// remains (stall). `goal` selects the bucket table. Completion is monotonic in
// the staged counts, so arrival order does not change whether it completes.
inline StageResult RunStaging(int goal, const int req[STAGING_UNITTYPES],
                              const int* unitTypes, int nUnits,
                              bool hasStagingArea = true) {
    int  staged[STAGING_UNITTYPES] = {0, 0, 0, 0};
    bool used[64] = {false};
    StageResult r = {false, false, 0};

    if (IsStagingComplete(req, staged, hasStagingArea)) {
        r.completed = true;
        r.emptyLaunch = true;  // declared complete with nothing staged
        return r;
    }

    for (;;) {
        int pick = -1;
        for (int u = 0; u < nUnits && u < 64; ++u) {
            if (used[u]) continue;
            if (StagingBucket(goal, unitTypes[u]) < 0) continue;  // irrelevant unit
            pick = u;
            break;
        }
        if (pick < 0) break;  // stalled: no remaining unit can advance staging

        int b = StagingBucket(goal, unitTypes[pick]);
        used[pick] = true;
        staged[b] += 1;
        if (IsStagingComplete(req, staged, hasStagingArea)) {
            r.completed = true;
            break;
        }
    }

    for (int i = 0; i < STAGING_UNITTYPES; ++i) r.staged += staged[i];
    return r;
}

} // namespace aistaging

#endif // AI_STAGING_SIM_H
