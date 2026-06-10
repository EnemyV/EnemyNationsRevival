// test_ai_staging.cpp
//
// Standalone unit tests for the AI staging unit-classification logic.
// Build & run via run-ai-tests.ps1. Compiles in isolation -- links no game code,
// touches no game build. Safe to run while the main game is being developed.

#include "ai_staging_logic.h"
#include "ai_staging_sim.h"
#include "microtest.h"

using namespace aistaging;

// ---------------------------------------------------------------------------
// Amphibious cargo set is med_tank + rangers ONLY.
// ---------------------------------------------------------------------------
static void test_amphib_membership() {
    CHECK(IsAmphibLandUnit(veh::med_tank));
    CHECK(IsAmphibLandUnit(veh::rangers));

    // Regression guard: these were wrongly amphib-eligible before the
    // canonicalization (routed to a sea invade but unable to board a lander).
    CHECK(!IsAmphibLandUnit(veh::light_tank));
    CHECK(!IsAmphibLandUnit(veh::light_art));
    CHECK(!IsAmphibLandUnit(veh::med_art));
    CHECK(!IsAmphibLandUnit(veh::infantry));
    CHECK(!IsAmphibLandUnit(veh::heavy_tank));
    CHECK(!IsAmphibLandUnit(veh::gun_boat));
}

// THE invariant the original light_tank bug broke: every unit the AI ROUTES to
// an amphibious assault must be able to BOARD a landing craft (assignment subset
// of loadable). Walk the whole enum so a future edit to either set is caught.
static void test_amphib_load_invariant() {
    for (int t = 0; t < veh::num_types; ++t)
        if (IsAmphibLandUnit(t))
            CHECK(IsLoadableSeaInvade(t));
}

// ---------------------------------------------------------------------------
// Per-goal staging bucket table. The 4 production switches must equal this.
// ---------------------------------------------------------------------------
static void test_seainvade_buckets() {
    CHECK_EQ(SeaInvadeBucket(veh::med_tank),      0);
    CHECK_EQ(SeaInvadeBucket(veh::landing_craft), 1);
    CHECK_EQ(SeaInvadeBucket(veh::gun_boat),      2);
    CHECK_EQ(SeaInvadeBucket(veh::rangers),       3);
    CHECK_EQ(SeaInvadeBucket(veh::light_tank), -1);
    CHECK_EQ(SeaInvadeBucket(veh::med_art),    -1);
    CHECK_EQ(SeaInvadeBucket(veh::infantry),   -1);
}

static void test_landwar_buckets() {
    CHECK_EQ(LandwarBucket(veh::light_tank),       0);
    CHECK_EQ(LandwarBucket(veh::med_tank),         0);
    CHECK_EQ(LandwarBucket(veh::heavy_tank),       0);
    CHECK_EQ(LandwarBucket(veh::heavy_scout),      0);
    CHECK_EQ(LandwarBucket(veh::infantry_carrier), 1);
    CHECK_EQ(LandwarBucket(veh::light_art),        2);
    CHECK_EQ(LandwarBucket(veh::med_art),          2);
    CHECK_EQ(LandwarBucket(veh::heavy_art),        2);
    CHECK_EQ(LandwarBucket(veh::infantry),         3);
    CHECK_EQ(LandwarBucket(veh::rangers),          3);
    CHECK_EQ(LandwarBucket(veh::construction),    -1);
    CHECK_EQ(LandwarBucket(veh::med_scout),       -1);  // "med_scouts patrol only"
}

static void test_pirate_seawar_buckets() {
    // PIRATE and SEAWAR share the cruiser/destroyer table.
    CHECK_EQ(StagingBucket(goal::PIRATE, veh::cruiser),   0);
    CHECK_EQ(StagingBucket(goal::PIRATE, veh::destroyer), 1);
    CHECK_EQ(StagingBucket(goal::SEAWAR, veh::cruiser),   0);
    CHECK_EQ(StagingBucket(goal::SEAWAR, veh::destroyer), 1);
    CHECK_EQ(StagingBucket(goal::PIRATE, veh::gun_boat), -1);
}

// LANDWAR and ADVDEFENSE must use an identical table (same production branch).
static void test_landwar_advdefense_identical() {
    for (int t = 0; t < veh::num_types; ++t)
        CHECK_EQ(StagingBucket(goal::LANDWAR, t), StagingBucket(goal::ADVDEFENSE, t));
}

// ---------------------------------------------------------------------------
// Bucket -> CAI_TF_* param-slot alignment, for every goal family.
// ---------------------------------------------------------------------------
static void test_slot_alignment() {
    CHECK_EQ(StagingSlot(SeaInvadeBucket(veh::med_tank)),      TF_ARMOR);
    CHECK_EQ(StagingSlot(SeaInvadeBucket(veh::landing_craft)), TF_LANDING);
    CHECK_EQ(StagingSlot(SeaInvadeBucket(veh::gun_boat)),      TF_SHIPS);
    CHECK_EQ(StagingSlot(SeaInvadeBucket(veh::rangers)),       TF_MARINES);

    CHECK_EQ(StagingSlot(LandwarBucket(veh::med_tank)),         TF_TANKS);
    CHECK_EQ(StagingSlot(LandwarBucket(veh::infantry_carrier)), TF_IFVS);
    CHECK_EQ(StagingSlot(LandwarBucket(veh::med_art)),          TF_ARTILLERY);
    CHECK_EQ(StagingSlot(LandwarBucket(veh::infantry)),         TF_INFANTRY);

    // PIRATE/SEAWAR: cruiser->CAI_TF_CRUISERS(4), destroyer->CAI_TF_DESTROYERS(5)
    CHECK_EQ(StagingSlot(StagingBucket(goal::PIRATE, veh::cruiser)),   4);
    CHECK_EQ(StagingSlot(StagingBucket(goal::PIRATE, veh::destroyer)), 5);
}

// ---------------------------------------------------------------------------
// Buckets must be in range [0, STAGING_UNITTYPES) for every classified unit, in
// every goal -- a bucket >= STAGING_UNITTYPES would index past the count array.
// ---------------------------------------------------------------------------
static void test_bucket_bounds() {
    const int goals[] = { goal::LANDWAR, goal::ADVDEFENSE, goal::PIRATE,
                          goal::SEAWAR, goal::SEAINVADE };
    for (int gi = 0; gi < (int)(sizeof(goals) / sizeof(goals[0])); ++gi) {
        int g = goals[gi];
        for (int t = 0; t < veh::num_types; ++t) {
            int b = StagingBucket(g, t);
            CHECK(b == -1 || (b >= 0 && b < STAGING_UNITTYPES));
        }
    }
}

// ---------------------------------------------------------------------------
// Material slots must equal the CMaterialTypes enum order (cai.h contract that
// shipped as commented-out ASSERTs).
// ---------------------------------------------------------------------------
static void test_material_slots() {
    CHECK_EQ(CAI_LUMBER, mat::lumber);
    CHECK_EQ(CAI_STEEL,  mat::steel);
    CHECK_EQ(CAI_COPPER, mat::copper);
    CHECK_EQ(CAI_MOLY,   mat::moly);
    CHECK_EQ(CAI_GOODS,  mat::goods);
    CHECK_EQ(CAI_FOOD,   mat::food);
    CHECK_EQ(CAI_OIL,    mat::oil);
    CHECK_EQ(CAI_GAS,    mat::gas);
    CHECK_EQ(CAI_COAL,   mat::coal);
    CHECK_EQ(CAI_IRON,   mat::iron);
    CHECK_EQ((int)mat::num_types, 10);
}

// ---------------------------------------------------------------------------
// *** Latent-bug hunt ***
// Every staging goal the COMPLETION counters classify units for must also be
// SIZED by GetStagingArea. Otherwise that goal's taskforce gets a staging area
// but zero unit requirements, and IsStagingCompete's iTaskCnt==0 early-out
// returns "complete" => an empty assault launches.
// ---------------------------------------------------------------------------
static void test_sizer_counter_goalset() {
    const int goals[] = { goal::ADVDEFENSE, goal::LANDWAR, goal::PIRATE,
                          goal::SEAWAR, goal::SEAINVADE };
    for (int gi = 0; gi < (int)(sizeof(goals) / sizeof(goals[0])); ++gi) {
        int g = goals[gi];
        if (!CounterHandlesGoal(g))
            continue;

        if (g == goal::SEAWAR) {
            // LATENT BUG (found by this test): IsStagingCompete and
            // ContinueStaging both classify units for IDG_SEAWAR (grouped with
            // IDG_PIRATE: cruiser/destroyer buckets), but GetStagingArea's naval
            // branch is `== IDG_PIRATE` only (caigmgr.cpp:7679) -- it never sizes
            // IDG_SEAWAR. A SEAWAR IDT_PREPAREWAR task therefore gets a staging
            // area but zero unit requirements; IsStagingCompete's iTaskCnt==0
            // early-out then returns "complete" and an EMPTY sea-war assault
            // launches. Reachability is data-gated (whether stdgta.dat attaches
            // IDT_PREPAREWAR to IDG_SEAWAR), but the code inconsistency is real.
            // FIX: make caigmgr.cpp:7679 `== IDG_PIRATE || == IDG_SEAWAR` (the
            // cruiser/destroyer sizing already fits SEAWAR). Then this promotes
            // to a CHECK automatically.
            KNOWN_BUG(SizerHandlesGoal(g),
                      "GetStagingArea omits IDG_SEAWAR (caigmgr.cpp:7679) -> empty SEAWAR assault");
        } else {
            CHECK(SizerHandlesGoal(g));
        }
    }
}

// ---------------------------------------------------------------------------
// Behavioral simulation: drive the staging-completion state machine and assert
// the taskforce completes / stalls / launches empty as expected.
// req[] layout per goal: SEAINVADE = {armor, landing, ships, marines};
//                        LANDWAR   = {tanks, ifv, artillery, infantry}.
// ---------------------------------------------------------------------------

// A normally-sized amphibious force converges and launches with real units.
static void test_sim_seainvade_completes() {
    int req[4] = {1, 1, 0, 1};  // 1 med_tank, 1 landing_craft, 1 rangers
    int units[] = {veh::med_tank, veh::landing_craft, veh::rangers, veh::gun_boat};
    StageResult r = RunStaging(goal::SEAINVADE, req, units, 4);
    CHECK(r.completed);
    CHECK(!r.emptyLaunch);
    CHECK_EQ(r.staged, 3);
}

// Over-supply of one bucket still completes (total >= taskCnt, each wanted >=1).
static void test_sim_oversupply_ok() {
    int req[4] = {1, 0, 0, 1};
    int units[] = {veh::med_tank, veh::med_tank, veh::rangers};
    StageResult r = RunStaging(goal::SEAINVADE, req, units, 3);
    CHECK(r.completed);
    CHECK(!r.emptyLaunch);
}

// A required bucket that cannot be filled stalls the whole assault -- the
// generalized form of the "re-enabling infantry adds a required type that may be
// unavailable" risk (and why each wanted type must be supplyable).
static void test_sim_missing_type_stalls() {
    int req[4] = {1, 1, 0, 1};  // needs a marine (rangers)...
    int units[] = {veh::med_tank, veh::landing_craft};  // ...but none present
    StageResult r = RunStaging(goal::SEAINVADE, req, units, 2);
    CHECK(!r.completed);
}

// Same stall, modelled for a land assault that requires infantry it doesn't have.
static void test_sim_landwar_infantry_stall() {
    int req[4] = {2, 0, 0, 1};  // tanks + 1 infantry
    int tanksOnly[] = {veh::med_tank, veh::light_tank, veh::heavy_tank};
    StageResult r = RunStaging(goal::LANDWAR, req, tanksOnly, 3);
    CHECK(!r.completed);  // infantry bucket never satisfied
    // ...and with an infantry unit present, it completes:
    int withInf[] = {veh::med_tank, veh::light_tank, veh::infantry};
    StageResult r2 = RunStaging(goal::LANDWAR, req, withInf, 3);
    CHECK(r2.completed);
}

// Behavioral counterpart of the SEAWAR sizer bug: when GetStagingArea fails to
// size a goal, the taskforce has zero requirements but a staging area exists,
// and IsStagingComplete's iTaskCnt==0 early-out declares it complete with NO
// units -> an empty launch. Documents the runtime consequence of the KNOWN_BUG.
static void test_sim_empty_requirements_launch_empty() {
    int zeroReq[4] = {0, 0, 0, 0};
    int units[] = {veh::cruiser, veh::destroyer};
    StageResult r = RunStaging(goal::SEAWAR, zeroReq, units, 2, /*hasStagingArea=*/true);
    CHECK(r.completed);
    CHECK(r.emptyLaunch);
    CHECK_EQ(r.staged, 0);

    // Sanity: with no staging area yet, zero requirements is NOT "complete".
    int none[4] = {0, 0, 0, 0};
    CHECK(!IsStagingComplete(zeroReq, none, /*hasStagingArea=*/false));
}

int main() {
    test_amphib_membership();
    test_amphib_load_invariant();
    test_seainvade_buckets();
    test_landwar_buckets();
    test_pirate_seawar_buckets();
    test_landwar_advdefense_identical();
    test_slot_alignment();
    test_bucket_bounds();
    test_material_slots();
    test_sizer_counter_goalset();

    test_sim_seainvade_completes();
    test_sim_oversupply_ok();
    test_sim_missing_type_stalls();
    test_sim_landwar_infantry_stall();
    test_sim_empty_requirements_launch_empty();
    return microtest::Summary();
}
