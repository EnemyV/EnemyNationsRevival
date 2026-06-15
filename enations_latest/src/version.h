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


#define         VER_MAJOR       2
#define         VER_MINOR       1
// Release bumped 2 -> 3 so saved games can carry the per-player "last research
// discovered" field (CPlayer::m_iLastDiscovered). The save load check only rejects
// on MAJOR/MINOR mismatch (CGame::Serialize), so release-2 saves still load — the
// new field is read only when the loaded save's release is >= 3.
// Release bumped 3 -> 4 so saves can carry the per-player colony-stat HISTORY ring
// buffers (population / housing / power over time) for the building-info windows'
// graphs. Same rule: older saves still load; the history block is read only when the
// loaded save's release is >= 4.
#define         VER_RELEASE     4

#define         VER_STRING                              "2.01.003"
#define         RES_VER_STRING                          "2.01.003\0"

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

