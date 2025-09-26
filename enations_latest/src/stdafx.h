//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma warning( disable : 4711 )

#include <afxcmn.h>
#include <afxext.h>
#include <afxmt.h>
#include <afxtempl.h>
#include <afxwin.h>  // MFC core and standard components
#include <climits>
#include <cmath>
#include <malloc.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <msacm.h>
#include <strstream>
//#include <ctl3d.h> // Unnecessary as of modern windows...
#include <cctype>
#include <clocale>
#include <ddraw.h>
#include <dsound.h>
#include <eh.h>
//#include <dplay.h>
#include <mss/mssw.h>
#include <wing/INCLUDE/wing.h>

//#define MEM_DEBUG	1
#include "vdmplay.h"
#include "base.h"
#include "windward.h"


// Smartheap's lib was compiled against a very out of date c runtime. There's no need for optimizing allocation
// at this level anymore, given the speed of modern computers and the relatively low load this game presents.
//#include <smartheap/smrtheap.hpp>

#pragma warning( disable : 4244 )  // I don't like this!!!

#ifdef _DEBUG
#define _CHEAT 1
#endif


#endif
