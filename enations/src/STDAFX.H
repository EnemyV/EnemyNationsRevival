//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma warning ( disable : 4711 )

#include <afxwin.h>			// MFC core and standard components
#include <afxext.h>
#include <afxcmn.h>
#include <afxtempl.h>
#include <afxmt.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <strstrea.h>
#include <ctl3d.h>
#include <eh.h>
#include <ctype.h>
#include <locale.h>

#include <ddraw.h>
#include <dsound.h>
//#include <dplay.h>
#include <wing.h>

#include <mssw.h>

//#define MEM_DEBUG	1
#include <smrtheap.hpp>

#include "windward.h"
#include "vdmplay.h"

#pragma warning ( disable : 4244 )	// I don't like this!!!

#ifdef	_DEBUG
#define		_CHEAT	1
#endif


#endif
