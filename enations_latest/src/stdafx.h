//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma warning( disable : 4711 )

// Pull windows.h + cassert for ASSERT/VERIFY/TRACE; rely on mfc_compat.h
// (included by windward.h's wind22 stdafx.h chain) for CString / CFile /
// CArchive / CMap / CDialog / CObject / CDC / CFont / CBrush / CPen /
// CBitmap / CPalette / CList / CWinThread.
#include <windows.h>
#include <cassert>
#ifndef ASSERT
#define ASSERT(expr)        assert(expr)
#endif
#ifndef VERIFY
#define VERIFY(expr)        assert(expr)
#endif
#ifndef ENSURE
#define ENSURE(expr)        assert(expr)
#endif
#ifndef TRACE
#define TRACE(...)          ((void)0)
#endif
#ifndef TRACE0
#define TRACE0(s)           ((void)0)
#endif
#ifndef TRACE1
#define TRACE1(s, p1)       ((void)0)
#endif
#ifndef TRACE2
#define TRACE2(s, p1, p2)   ((void)0)
#endif
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
#include <dsound.h>
#include <eh.h>
//#include <dplay.h>
// MSS32 removed - using SDL2 + SDL_mixer for audio
#include <wing/INCLUDE/wing.h>

//#define MEM_DEBUG	1
#include "vdmplay.h"
#include "base.h"
#include "windward.h"
#include "EnSettings.h"


// Smartheap's lib was compiled against a very out of date c runtime. There's no need for optimizing allocation
// at this level anymore, given the speed of modern computers and the relatively low load this game presents.
//#include <smartheap/smrtheap.hpp>

#pragma warning( disable : 4244 )  // I don't like this!!!

#ifdef _DEBUG
#define _CHEAT 1
#endif


#endif
