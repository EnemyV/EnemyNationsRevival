#ifndef __WINDWARD_H__
#define __WINDWARD_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include	<thielen.h>

#ifdef _GG
#include	"gg.h"
#endif

class 	CMmio;
class 	CDIB;
class 	CDIBWnd;
struct 	BITMAPINFO256;
struct 	LOGPALETTE256;

#include	<games.h>
#include	<ptr.h>
#include	<cache.h>
#include	<datafile.h>
#include	<dlgmsg.h>
#include	<fixpoint.h>
#include	<init.h>
#include	<mmio.h>
#include	<music.h>
#include	<rand.h>
#include	<threads.h>
#include	<btree.h>
#include	<_msgs.h>
#include	<_error.h>
#include	<_help.h>
#include	<logging.h>
#include	<_debug.h>
#include	<codec.h>

/*	WIP
#include	<wndbase.h>
#include	<blt.h>
#include	<dib.h>
#include	<dibwnd.h>
#include	<scanlist.h>
*/

extern void InitWindwardLib1 (CWinApp const * pWa);
extern BOOL InitWindwardLib2 ();

// this brings in the inlines in non-debug mode
#ifndef _DEBUG
#define _RELEASE_INLINE inline
#include <windward.inl>
#endif

#endif
