// ai_staging_logic.h
//
// Pure, dependency-free model of Enemy Nations' AI staging unit-classification
// logic. Header-only; includes NO game headers, so it can be compiled and tested
// in complete isolation. (The real CTransportData enum lives in vehicle.h, which
// drags in unit.h / unit_wnd.h / netcmd.h and thus the whole game/window/net
// layer -- not linkable in a tiny standalone test.)
//
// This is an executable SPEC of the canonical unit sets used by AI staging. It
// mirrors -- but does NOT change or link -- the decision tables in:
//   - caitmgr.cpp  CAITaskMgr::AssignTask        (navy vs land-combat routing)
//   - caitask.cpp  CAITaskList::GetNavyTask      (amphibious eligibility, bAmphib)
//   - caitmgr.cpp  CAITaskMgr::IsStagingCompete  (staging buckets: SEAINVADE+LANDWAR)
//   - caitmgr.cpp  CAITaskMgr::ContinueStaging   (staging buckets)
//   - caigmgr.cpp  CAIGoalMgr::GetStagingArea    (taskforce sizing)
//   - caitmgr.cpp  CAITaskMgr::LoadCargo / LoadTroops (who can board a landing craft)
//
// It runs standalone and has zero effect on game behavior or the game build.
// SYNC: if the production decision tables change, update this header to match;
// the static_assert blocks below pin the load-bearing values so an accidental
// edit here fails the test compile.

#ifndef AI_STAGING_LOGIC_H
#define AI_STAGING_LOGIC_H

namespace aistaging {

// --- Mirror of CTransportData::TRANS_TYPE (enations_latest/src/vehicle.h:100) ---
// Positional enum; values must match vehicle.h exactly.
namespace veh {
enum Type {
    construction = 0, med_truck, heavy_truck, light_scout, med_scout, heavy_scout,
    infantry_carrier, light_tank, med_tank, heavy_tank, light_art, med_art,
    heavy_art, light_cargo, gun_boat, destroyer, cruiser, landing_craft,
    infantry, rangers, marines, num_types
};
} // namespace veh

static_assert(veh::infantry_carrier == 6 && veh::med_tank == 8 &&
              veh::gun_boat == 14 && veh::landing_craft == 17 &&
              veh::infantry == 18 && veh::rangers == 19 && veh::num_types == 21,
              "aistaging::veh drifted from CTransportData::TRANS_TYPE (vehicle.h)");

// --- Staging taskforce param-slot constants (mirror of cai.h / caigmgr.hpp) ---
// Buckets are 0..3; a task stores the desired count of each bucket at param slot
// (bucket + STAGING_UNITTYPES).
enum { STAGING_UNITTYPES = 4 };

// SEAINVADE slot semantics (cai.h): ARMOR=4, LANDING=5, SHIPS=6, MARINES=7
enum { TF_ARMOR = 4, TF_LANDING = 5, TF_SHIPS = 6, TF_MARINES = 7 };
// LANDWAR slot semantics (cai.h): TANKS=4, IFVS=5, ARTILLERY=6, INFANTRY=7
enum { TF_TANKS = 4, TF_IFVS = 5, TF_ARTILLERY = 6, TF_INFANTRY = 7 };

static_assert(TF_ARMOR == STAGING_UNITTYPES + 0 && TF_LANDING == STAGING_UNITTYPES + 1 &&
              TF_SHIPS == STAGING_UNITTYPES + 2 && TF_MARINES == STAGING_UNITTYPES + 3,
              "CAI_TF_* SEAINVADE slots must be contiguous from STAGING_UNITTYPES");

// Bucket index -> task param slot.
inline int StagingSlot(int bucket) { return bucket + STAGING_UNITTYPES; }

// ----------------------------------------------------------------------------
// Amphibious assault (IDG_SEAINVADE): only med_tank + rangers can embark on a
// landing craft (LoadCargo / LoadTroops gate), so only they are eligible to be
// routed to the amphibious assault task (AssignTask -> AssignNavy; GetNavyTask
// bAmphib set). This is the invariant the original light_tank/light_art bug
// violated: those were assignable to a sea invade but could not board.
// ----------------------------------------------------------------------------
inline bool IsAmphibLandUnit(int t) {
    return t == veh::med_tank || t == veh::rangers;
}

// Who can actually board a landing craft for a sea invade
// (LoadCargo gate, LoadTroops carrier match, FindTroopToLoad filter).
inline bool IsLoadableSeaInvade(int t) {
    return t == veh::med_tank || t == veh::rangers;
}

// SEAINVADE staging bucket for a unit type, or -1 if it is not a sea-invade
// staging unit. (IsStagingCompete / ContinueStaging SEAINVADE switch.)
inline int SeaInvadeBucket(int t) {
    switch (t) {
        case veh::med_tank:      return 0; // -> TF_ARMOR
        case veh::landing_craft: return 1; // -> TF_LANDING
        case veh::gun_boat:      return 2; // -> TF_SHIPS
        case veh::rangers:       return 3; // -> TF_MARINES
        default:                 return -1;
    }
}

// LANDWAR / ADVDEFENSE staging bucket, or -1.
// (IsStagingCompete / ContinueStaging LANDWAR switch.)
inline int LandwarBucket(int t) {
    switch (t) {
        case veh::heavy_scout:
        case veh::light_tank:
        case veh::med_tank:
        case veh::heavy_tank:       return 0; // -> TF_TANKS
        case veh::infantry_carrier: return 1; // -> TF_IFVS
        case veh::light_art:
        case veh::med_art:
        case veh::heavy_art:        return 2; // -> TF_ARTILLERY
        case veh::infantry:
        case veh::rangers:          return 3; // -> TF_INFANTRY
        default:                    return -1;
    }
}

inline bool IsSeaInvadeStagingType(int t) { return SeaInvadeBucket(t) >= 0; }
inline bool IsLandwarStagingType(int t)   { return LandwarBucket(t)   >= 0; }

// --- Mirror of the IDG_* staging goals (cai.h) ---
namespace goal {
enum Id {
    LANDWAR    = 1018,  // IDG_LANDWAR
    SEAWAR     = 1019,  // IDG_SEAWAR
    REPELL     = 1031,  // IDG_REPELL
    SHORES     = 1032,  // IDG_SHORES
    SEAINVADE  = 1033,  // IDG_SEAINVADE
    PIRATE     = 1034,  // IDG_PIRATE
    ADVDEFENSE = 1022   // IDG_ADVDEFENSE
};
} // namespace goal

// Which staging goals GetStagingArea (the taskforce SIZER) assigns unit counts
// for. Faithful to caigmgr.cpp CAIGoalMgr::GetStagingArea branches:
//   if (IDG_PIRATE)                      (caigmgr.cpp:7679)
//   if (IDG_SEAINVADE)                   (caigmgr.cpp:7716)
//   if (IDG_ADVDEFENSE || IDG_LANDWAR)   (caigmgr.cpp:7801)
inline bool SizerHandlesGoal(int g) {
    return g == goal::PIRATE || g == goal::SEAINVADE ||
           g == goal::ADVDEFENSE || g == goal::LANDWAR;
}

// Which staging goals the COMPLETION counters classify units for. Faithful to
// the (identical) goal branches in IsStagingCompete and ContinueStaging:
//   if (IDG_ADVDEFENSE || IDG_LANDWAR)
//   else if (IDG_PIRATE || IDG_SEAWAR)
//   else if (IDG_SEAINVADE)
inline bool CounterHandlesGoal(int g) {
    return g == goal::ADVDEFENSE || g == goal::LANDWAR ||
           g == goal::PIRATE || g == goal::SEAWAR || g == goal::SEAINVADE;
}

// Which goals the SHIPPED DATA (stdgta.dat) attaches IDT_PREPAREWAR (2325) to.
// Verified 2026-06-09 by parsing the binary (40 goals / 75 tasks): exactly
// {LANDWAR 1018, ADVDEFENSE 1022, SEAINVADE 1033, PIRATE 1034}. Notably,
// IDG_SEAWAR (1019) does NOT own a staging task -- its list is make-ships /
// seek-at-sea / patrol / escort -- so the counters' SEAWAR grouping is
// dead-defensive code, unreachable with shipped data, and GetStagingArea's
// PIRATE-only ocean sizing matches both the data and the "only 2 ocean based
// staging tasks" comment (caigmgr.cpp:7529). SYNC: re-verify if stdgta.dat is
// ever regenerated/modded.
inline bool DataAttachesPrepareWar(int g) {
    return g == goal::LANDWAR || g == goal::ADVDEFENSE ||
           g == goal::SEAINVADE || g == goal::PIRATE;
}

// Canonical staging bucket for (goal, unit type), or -1 if not a staging unit of
// that goal. This single table is what ALL FOUR production switches must equal:
//   caitmgr.cpp IsStagingCompete count switch (~3873)
//   caitmgr.cpp IsStagingCompete iType switch (~3944)
//   caitmgr.cpp ContinueStaging  type  switch (~4087)
//   caitmgr.cpp ContinueStaging  count switch (~4198)
inline int StagingBucket(int g, int t) {
    if (g == goal::ADVDEFENSE || g == goal::LANDWAR) return LandwarBucket(t);
    if (g == goal::PIRATE || g == goal::SEAWAR) {
        if (t == veh::cruiser)   return 0;  // -> CAI_TF_CRUISERS (slot 4)
        if (t == veh::destroyer) return 1;  // -> CAI_TF_DESTROYERS (slot 5)
        return -1;
    }
    if (g == goal::SEAINVADE) return SeaInvadeBucket(t);
    return -1;
}

// --- Material slots ---
// Mirror of CMaterialTypes (documented in cai.h:166) and the CAI_* material slot
// constants (cai.h:183). The original code had ASSERT(CMaterialTypes::x==CAI_x)
// for each material, but commented out (cai.h:150-161); this models that contract.
namespace mat {
enum Type { lumber = 0, steel, copper, moly, goods, food, oil, gas, coal, iron, num_types };
} // namespace mat
enum {
    CAI_LUMBER = 0, CAI_STEEL = 1, CAI_COPPER = 2, CAI_MOLY = 3, CAI_GOODS = 4,
    CAI_FOOD = 5, CAI_OIL = 6, CAI_GAS = 7, CAI_COAL = 8, CAI_IRON = 9
};

} // namespace aistaging

#endif // AI_STAGING_LOGIC_H
