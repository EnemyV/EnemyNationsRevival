//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __ERROR_H__
#define __ERROR_H__


enum 
{
	ERR_FIRST = ERR_APP_START,

	ERR_TLP_QUIT,
	ERR_TLP_BAD_SPRITE_TYPE,
	ERR_TLP_BAD_PLAYER_NET_NUM,
	ERR_TLP_BAD_PLAYER_NUM,
	ERR_TLP_LOAD_DIFF_VER,
	ERR_TLP_BAD_DATA,

	ERR_TLP_BAD_SPRITE_DIB,
	ERR_TLP_BAD_SPRITE_VIEW,
	ERR_TLP_BAD_SPRITE_ID,

	ERR_CAI_BAD_NEW,
	ERR_CAI_BAD_FILE,
	ERR_CAI_TM_QUIT,

	ERR_TLP_BAD_RCE_FILE
};


#endif
