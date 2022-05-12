//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


const int				VER_RIFF = 10;

// do NOT use these for display to the user - this is for the dat file only
const char GameDataName[] = "Enemy Nations";
const char GameDataFile[] = "ENations.dat";
const char GameLogFile[] = "ENations.log";


#define         VER_MAJOR       2
#define         VER_MINOR       0
#define         VER_RELEASE     1

#define         VER_STRING                              "2.00.001"
#define         RES_VER_STRING                          "2.00.001\0"

#ifdef _DEBUG
	#define         VER_FLAGS         VS_FF_DEBUG | VS_FF_PRIVATEBUILD | VS_FF_PRERELEASE
#else
  #ifdef _CHEAT
	  #define       VER_FLAGS         VS_FF_PRERELEASE // still testing
	#else
	  #define       VER_FLAGS         0
  #endif
#endif

