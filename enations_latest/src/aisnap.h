////////////////////////////////////////////////////////////////////////////
//
//  aisnap.h : per-tick world snapshot for lock-free AI reads (Tier B).
//
//  The AI threads' dominant world access is CAIUnit::GetCopyData, which
//  historically took the global game lock `cs` for EVERY refresh — so AI
//  "thinking" serialized against the sim tick (which holds `cs` for the whole
//  operate pass). This module publishes an immutable copy of the few dynamic
//  fields the AI actually reads (see CAIUnit::Update(CBuilding*/CVehicle*)),
//  once per sim tick, double-buffered. AI threads then serve GetCopyData from
//  the snapshot without ever touching `cs`.
//
//  Concurrency model:
//   - Publish() runs on the MAIN thread only. It fills the back buffer while
//     holding `cs` (so the world is consistent), then swaps the front-buffer
//     index under a dedicated SRWLOCK held exclusively for the swap only.
//   - Readers (AI threads) take the SRWLOCK shared just long enough to find
//     the entry and memcpy it out. They never hold it across game calls, and
//     nothing else is ever taken inside it (no lock-order hazard). They block
//     only against the microsecond pointer swap — never against the sim tick.
//   - Staleness is bounded by the publish cadence (~one sim tick, <=~42ms),
//     which matches what the AI effectively saw before (it could only read
//     between ticks anyway), and GetCopyData already tolerated stale copies.
//
//  Gate: EN_AISNAP (default ON; set EN_AISNAP=0 to fall back to the legacy
//  locked path — useful for A/B perf comparison and as a safety valve).
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AISNAP_H__
#define __AISNAP_H__

#include <windows.h>

class CBuildUnit;

// material slots: sized to CAI_DATA_SLOTS (cai.h); only
// CMaterialTypes::num_types entries are filled.
#define AISNAP_MAT_SLOTS 16

// Dynamic per-building fields read by CAIUnit::Update(CBuilding*) plus the
// current-production/repair static-table records for the CBuildUnit copy path.
struct AiBldgSnap
{
    int iOwner;                     // GetOwner()->GetPlyrNum() (-1 if no owner);
                                    // lets readers replicate the legacy
                                    // owner-validation in GetBuildingData
    int aiStore[AISNAP_MAT_SLOTS];  // CBuilding::GetStore(i)
    int iExitX, iExitY;             // GetExitHex()
    int bConstructing;              // IsConstructing()
    int bPaused;                    // IsPaused()
    int bWaiting;                   // IsWaiting()
    int iDamagePer;                 // GetDamagePer()
    int iType;                      // GetData()->GetType()      (CStructureData id)
    int iUnionType;                 // GetData()->GetUnionType() (CAI_PRODUCES)
    int iBldgType;                  // GetData()->GetBldgType()  (CAI_BUILDTYPE)

    int bAbandoned;                 // IsFlag( CUnit::abandoned ) (depleted mine)

    // Static-table records (stable pointers into CStructureData-owned data,
    // valid for the lifetime of the loaded game data). NULL when idle.
    CBuildUnit const* pProducing;   // vehicle factory: current build record
    CBuildUnit const* pRepairing;   // repair shop: current repair record
};

// Dynamic per-vehicle fields read by CAIUnit::Update(CVehicle*) plus the
// extra fields the AI's direct theVehicleMap sites read (movement/cargo).
struct AiVehSnap
{
    int iOwner;                     // GetOwner()->GetPlyrNum() (-1 if no owner)
    int aiStore[AISNAP_MAT_SLOTS];  // CVehicle::GetStore(i)
    int iHeadX, iHeadY;             // GetHexHead()
    int iDestX, iDestY;             // GetHexDest()
    int iDamagePer;                 // GetDamagePer()
    int iType;                      // GetData()->GetType()  (CTransportData id)

    int iPtHeadX, iPtHeadY;         // GetPtHead() (subhex)
    int iPtTailX, iPtTailY;         // GetPtTail() (subhex)
    int iCargoCount;                // GetCargoCount()
    int bCarried;                   // GetTransport() != NULL
    int iSpotting;                  // GetSpottingRange()
    int iRouteMode;                 // GetRouteMode()
};

namespace AiSnap
{
// TRUE when the snapshot path is active (EN_AISNAP != 0, checked once).
BOOL Enabled( void );

// Main thread only. Copies the world into the back buffer (takes `cs`
// internally) and swaps it to front. Self-throttled to ~one sim tick, so it is
// safe and cheap to call once per main-loop iteration.
void Publish( void );

// AI threads: copy the snapshot entry for a unit id into `out`.
// FALSE = no entry (unit dead, or created since the last publish, or snapshot
// not yet published) — caller must fall back to the legacy locked path.
BOOL CopyBldg( DWORD dwID, AiBldgSnap& out );
BOOL CopyVeh( DWORD dwID, AiVehSnap& out );

// AI threads: snapshot-first read with the legacy locked fallback built in —
// on a snapshot miss this takes `cs` and fills `out` from the live unit (the
// exact data the old direct theVehicleMap/theBuildingMap sites read).
// FALSE = the unit does not exist at all (matches the legacy NULL check).
BOOL ReadVeh( DWORD dwID, AiVehSnap& out );
BOOL ReadBldg( DWORD dwID, AiBldgSnap& out );

// --- minerals presence cache --------------------------------------------
// theMinerals is near-static (placed at worldgen; entries are removed only on
// depletion, at a single site in new_unit.cpp). CAIMap::UpdateMap's full-map
// rescan did a guarded hash Lookup PER HEX per AI per idle rotation (65k+
// lookups/pass — 7 AI threads were profiled inside it simultaneously). The
// bitmap is built once per game by Publish (main thread, under cs) and read
// lock-free by the AI threads; depletion clears the bit.
BOOL MineralsReady( void );              // TRUE once the bitmap is valid
BOOL MineralsHas( int iX, int iY );      // only meaningful when MineralsReady
void MineralsRemoved( int iX, int iY );  // main thread (the depletion site)

// Clear both buffers (game teardown / world unload — static-table pointers in
// the snapshot must not outlive the game data they point into).
void Reset( void );
}  // namespace AiSnap

#endif  // __AISNAP_H__
