// test_ai_data.cpp
//
// Parses the SHIPPED stdgta.dat (the AI goal->task knowledge base) and asserts
// its integrity plus agreement with the hand-maintained mirrors in
// ai_staging_logic.h. This converts the SEAWAR lesson -- "the AI is
// data-driven; verify against the data file" -- into a permanent guard: if the
// data is regenerated/modded or the mirror drifts, this fails.
//
// File format (CAISavLd::LoadBinaryData, caisavld.cpp):
//   int nGoals
//   nGoals x GoalBuff { int id; int type; int tasks[24]; }          (104 B)
//   int nTasks
//   nTasks x TaskBuff { int id, goal, type, priority, orderId; int params[8]; } (52 B)
//   96 x int  initial goals (24 per difficulty x 4 difficulties)
//
// Usage: ai_data_tests.exe <path-to-stdgta.dat>
// Exit:  0 all pass, 1 failures, 2 usage/cannot-open (treated as skip by runner)

#define _CRT_SECURE_NO_WARNINGS  // fopen in a standalone test tool

#include "ai_staging_logic.h"
#include "microtest.h"

#include <cstdio>
#include <vector>

namespace {
struct GoalRec { int id, type; int tasks[24]; };
struct TaskRec { int id, goal, type, priority, orderId; int params[8]; };

const int IDT_PREPAREWAR_ID = 2325;
const int NUM_INITIAL = 24 * 4;  // NUM_INITIAL_GOALS x NUM_DIFFICUTY_LEVELS
}

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: ai_data_tests <stdgta.dat>\n"); return 2; }
    std::FILE* f = std::fopen(argv[1], "rb");
    if (!f) { std::printf("SKIP: cannot open %s\n", argv[1]); return 2; }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    int nGoals = 0;
    CHECK(std::fread(&nGoals, 4, 1, f) == 1);
    CHECK(nGoals > 0 && nGoals < 1000);
    std::vector<GoalRec> goals(nGoals > 0 ? nGoals : 0);
    for (int g = 0; g < nGoals; ++g)
        CHECK(std::fread(&goals[g], sizeof(GoalRec), 1, f) == 1);

    int nTasks = 0;
    CHECK(std::fread(&nTasks, 4, 1, f) == 1);
    CHECK(nTasks > 0 && nTasks < 1000);
    std::vector<TaskRec> tasks(nTasks > 0 ? nTasks : 0);
    for (int t = 0; t < nTasks; ++t)
        CHECK(std::fread(&tasks[t], sizeof(TaskRec), 1, f) == 1);

    std::vector<int> initial(NUM_INITIAL);
    for (int i = 0; i < NUM_INITIAL; ++i)
        CHECK(std::fread(&initial[i], 4, 1, f) == 1);
    std::fclose(f);

    // ---- format integrity: byte math must account for the whole file ----
    CHECK_EQ((long)(8 + 104LL * nGoals + 52LL * nTasks + 4 * NUM_INITIAL), size);

    // ---- referential integrity ----
    // every task id referenced by a goal must exist in the task list
    // (a missing id means FindTask returns NULL and that behaviour silently
    // never happens -- the failure mode data edits would introduce)
    for (int g = 0; g < nGoals; ++g) {
        for (int j = 0; j < 24; ++j) {
            int tid = goals[g].tasks[j];
            if (!tid) continue;
            bool found = false;
            for (int t = 0; t < nTasks && !found; ++t) found = (tasks[t].id == tid);
            CHECK(found);
        }
        // NOTE: duplicate task ids within a goal are INTENTIONAL -- they encode
        // QUANTITY (each listed id becomes one assignable task instance).
        // Shipped data: IDG_BASICFEED lists IDT_BUILDFARM x3 (= build 3 farms),
        // IDG_CARGOSHIP lists IDT_MAKELCARGOSHIP x3. A no-duplicates assertion
        // here failed on shipped data and was removed; multiplicity is only
        // meaningful for ORDER-type tasks, which is what we check instead.
        for (int a = 0; a < 24; ++a) {
            if (!goals[g].tasks[a]) continue;
            for (int b = a + 1; b < 24; ++b) {
                if (goals[g].tasks[a] != goals[g].tasks[b]) continue;
                // duplicated id: must be an ORDER task (type 1) -- quantity of
                // info/combat/message tasks would be meaningless
                for (int t = 0; t < nTasks; ++t)
                    if (tasks[t].id == goals[g].tasks[a])
                        CHECK_EQ(tasks[t].type, 1);
            }
        }
    }

    // every initial-goal entry must reference a defined goal (or be 0)
    for (int i = 0; i < NUM_INITIAL; ++i) {
        int gid = initial[i];
        if (!gid) continue;
        bool found = false;
        for (int g = 0; g < nGoals && !found; ++g) found = (goals[g].id == gid);
        CHECK(found);
    }

    // ---- value sanity ----
    for (int g = 0; g < nGoals; ++g)
        CHECK(goals[g].type >= 1 && goals[g].type <= 4);   // TRADE..NAVAL_WARFARE
    for (int t = 0; t < nTasks; ++t) {
        CHECK(tasks[t].type >= 1 && tasks[t].type <= 4);   // ORDER..COMBAT
        CHECK(tasks[t].priority >= 0 && tasks[t].priority <= 100);
    }

    // ---- mirror agreement: the staging-goal attachment set ----
    // For EVERY goal in the file, "owns IDT_PREPAREWAR" must agree with the
    // DataAttachesPrepareWar mirror. This is the SEAWAR guard read from truth
    // instead of from a hardcoded one-time parse.
    int nAttached = 0;
    for (int g = 0; g < nGoals; ++g) {
        bool owns = false;
        for (int j = 0; j < 24 && !owns; ++j) owns = (goals[g].tasks[j] == IDT_PREPAREWAR_ID);
        if (owns) ++nAttached;
        CHECK_EQ((int)owns, (int)aistaging::DataAttachesPrepareWar(goals[g].id));
    }
    CHECK_EQ(nAttached, 4);  // LANDWAR, ADVDEFENSE, SEAINVADE, PIRATE

    std::printf("[ai_data] parsed %d goals, %d tasks, %d initial entries from %s\n",
                nGoals, nTasks, NUM_INITIAL, argv[1]);
    return microtest::Summary();
}
