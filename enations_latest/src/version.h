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
#define         VER_RELEASE     7

// 3.00.009 -> 3.00.010: T-0073 (no text on non-Debian Linux — hardcoded Debian font
// paths) ships here instead of as a re-cut of the published 3.00.009 asset. No
// save-format change, so VER_RELEASE is untouched.
#define         VER_STRING                              "3.00.012"
#define         RES_VER_STRING                          "3.00.012\0"

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

