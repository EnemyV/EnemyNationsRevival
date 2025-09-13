#ifndef __WINDWARD_H__
#define __WINDWARD_H__


//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


#include <thielen.h>

#ifdef _GG
#include "gg.h"
#endif

class CMmio;
class CDIB;
class CDIBWnd;
struct BITMAPINFO256;
struct LOGPALETTE256;

#include <_debug.h>
#include <_error.h>
#include <_help.h>
#include <_msgs.h>
#include <btree.h>
#include <cache.h>
#include <codec.h>
#include <datafile.h>
#include <dlgmsg.h>
#include <fixpoint.h>
#include <games.h>
#include <init.h>
#include <logging.h>
#include <mmio.h>
#include <music.h>
#include <ptr.h>
#include <rand.h>
#include <threads.h>

// so this totally wasn't finished, right??
/* WIP
#include <blt.h>
#include <dib.h>
#include <dibwnd.h>
#include <scanlist.h>
#include <wndbase.h>
*/

extern void InitWindwardLib1( CWinApp const* pWa );
extern BOOL InitWindwardLib2( );

// this brings in the inlines in non-debug mode
#ifndef _DEBUG
#define _RELEASE_INLINE inline
#include <windward.inl>
#endif

#endif
