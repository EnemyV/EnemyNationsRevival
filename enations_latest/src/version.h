//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifndef __ENATIONS_VERSION_H__
#define __ENATIONS_VERSION_H__


const int				VER_RIFF = 10;

// do NOT use these for display to the user - this is for the dat file only
const char GameDataName[] = "Enemy Nations";
const char GameDataFile[] = "ENations.dat";
const char GameLogFile[] = "ENations.log";


#define         VER_MAJOR       3
#define         VER_MINOR       0
// Release bumped 2 -> 3 so saved games can carry the per-player "last research
// discovered" field (CPlayer::m_iLastDiscovered). The save load check only rejects
// on MAJOR/MINOR mismatch (CGame::Serialize), so release-2 saves still load — the
// new field is read only when the loaded save's release is >= 3.
// Release bumped 3 -> 4 so saves can carry the per-player colony-stat HISTORY ring
// buffers (population / housing / power over time) for the building-info windows'
// graphs. Same rule: older saves still load; the history block is read only when the
// loaded save's release is >= 4.
// Release bumped 4 -> 5 so saves can carry the per-player EDICTS bitmask
// (CPlayer::m_dwEdicts). Same rule: older saves still load; the edicts DWORD is read
// only when the loaded save's release is >= 5, and RecomputeEdictMults() rebuilds the
// derived multipliers/upkeeps after the bit field is restored.
// NOTE: VER_RELEASE is the SAVE-FORMAT counter (written as m_dwVer; load gates the
// optional fields on m_dwVer >= 3 / >= 4 / >= 5 / >= 6 in CPlayer::Serialize). It deliberately
// does NOT reset on the 3.00 major bump: resetting it to 0 would make a 3.00 save
// write those fields but refuse to read them back (0 < 3/4/5/6) -> stream desync. The
// 2.xx -> 3.00 major bump already rejects old saves via the MAJOR/MINOR check.
// Release 6: the workforce-NEED history series (m_aHistPplNeed) is now serialized too
// (was runtime-only + backfilled flat on load, so the workforce graph didn't restore).
// Release 7: CVehicleBuilding::m_iNum (the vehicle-build queue count) is now serialized,
// so a factory's remaining build queue survives save/load (was lost -> queue truncated).
// Release 8: the vehicle route cursor's NO-CURSOR case now writes an out-of-range
// SENTINEL (0xFFFF in the WORD index field) instead of the shipped N-1 (BUGS #99), and
// it is read back as a NULL cursor only when the loaded save's release is >= 8. Older
// saves keep the shipped N-1 rule exactly - a release-7 save storing N-1 still restores
// the last route entry as the cursor.
// Release 8 carries three more things besides that sentinel, all gated the same way and
// all written only once the counter reached 8 (they were authored against a 7 counter, so
// the WRITER is gated too - see the comments at each site in new_unit.cpp):
//   - CVehicle::Serialize: the nine traffic detour/recovery fields (m_bResume,
//     m_subResume, m_iResumeMode, m_bReversing, m_bForwardEscape, m_iHoldFrames,
//     m_iBackUps, m_iJamClear, m_iJamCool), so a vehicle mid-detour or mid-hold resumes
//     its saved job instead of coming back parked; plus m_bRouteLoop, the one-shot/loop
//     flag (BUGS #95) - a pre-8 save never wrote it, so such a route loads LOOPING.
//   - CRoute::Serialize: the order-queue payload (m_iBldgType, m_iDir, m_hexEnd) for
//     BUGS #38, so a queued build/road order survives save/load as an order rather than
//     decaying into a bare movement stop (pre-8 entries are movement stops only).
#define         VER_RELEASE     8

// 3.1.001: display version only. No HEADER change - VER_MAJOR/VER_MINOR stay 3/0, so
// every 3.00.x save still passes the major/minor check and loads. (VER_RELEASE is
// bumped to 8 above by BUGS #99. The release counter is NOT part of that major/minor
// header check - but it is not ignored either: a save whose counter is ABOVE this
// build's is refused outright (BUGS #102), and a counter BELOW it is read with its own
// layout, which is what the m_dwVer >= N field gates exist to do.) Bumping VER_MINOR
// would refuse them all (CGame::Serialize).
#define         VER_STRING                              "3.1.001"
#define         RES_VER_STRING                          "3.1.001\0"

#ifdef _DEBUG
	#define         VER_FLAGS         VS_FF_DEBUG | VS_FF_PRIVATEBUILD | VS_FF_PRERELEASE
#else
  #ifdef _CHEAT
	  #define       VER_FLAGS         VS_FF_PRERELEASE // still testing
	#else
	  #define       VER_FLAGS         0
  #endif
#endif

#endif // __ENATIONS_VERSION_H__

