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
#ifdef _WIN32
// NOMINMAX BEFORE <windows.h> so it does not define max/min as textual macros.
// Also set as a compile definition in CMakeLists.txt; defining it here too makes the
// header self-sufficient (stdafx.h is the force-included PCH, so it is seen first).
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// min/max on MSVC: with NOMINMAX above, <windows.h> does NOT define max/min as
// textual macros. The cross-platform port calls ::max(...)/::min(...) and legacy
// code calls unqualified max(a,b)/min(a,b); the shared header below provides the
// global function templates both bind to (same source the POSIX build uses via
// win32_compat.h). MUST come after NOMINMAX + <windows.h>, which it does here.
#include "en_minmax.h"
#else
#include "win32_compat.h"   // Win32-on-POSIX shim (Linux build)
#endif
#include <cassert>
#include "en_assert.h"   // non-fatal, logged ASSERT (mirrors original MFC "Ignore")
#ifndef ASSERT
#define ASSERT(expr)        EN_ASSERT_NONFATAL(expr)
#endif
#ifndef VERIFY
#define VERIFY(expr)        EN_ASSERT_NONFATAL(expr)
#endif
#ifndef ENSURE
#define ENSURE(expr)        EN_ASSERT_NONFATAL(expr)
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
#ifdef __APPLE__
#include <stdlib.h>     // macOS has no <malloc.h>; malloc/alloca live here
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include <strstream>
//#include <ctl3d.h> // Unnecessary as of modern windows...
#include <cctype>
#include <clocale>
// <dsound.h> removed — audio is SDL_mixer, not DirectSound (no DS API used).
#ifdef _WIN32
// Windows multimedia + WinG + SEH headers. On Linux these have no equivalent:
// audio is SDL_mixer, the legacy WinG blit path is excluded, and there is no
// SEH in the tree. The few types still referenced (WAVEFORMATEX etc.) live in
// excluded TUs or are provided by the shim.
#include <mmreg.h>
#include <mmsystem.h>
#include <msacm.h>
#include <eh.h>
#include <wing/INCLUDE/wing.h>
#endif

//#define MEM_DEBUG	1
#include "vdmplay.h"
#include "base.h"
#include "windward.h"
#include "EnSettings.h"


// Smartheap's lib was compiled against a very out of date c runtime. There's no need for optimizing allocation
// at this level anymore, given the speed of modern computers and the relatively low load this game presents.
//#include <smartheap/smrtheap.hpp>

#pragma warning( disable : 4244 )  // I don't like this!!!

// Dev cheat surface (_CHEAT): map reveal, KnowItAll/GrantResearch, Max*, the AI
// unit grants, the End-key spectator, the debug overlays. This used to be implied
// by _DEBUG, so EVERY debug build was a cheat build and a stale [Cheat] registry
// value (e.g. SeeAll=1) silently revealed the map in it. Now an explicit opt-in,
// default OFF: flip to 1 here or build with -DEN_CHEAT=1.
// NOTE: spectator mode (End during rocket placement) is in-dev and lives behind
// this gate, so it needs a cheat build - deliberate, it does not ship.
// The CMake build is the primary gate (option EN_CHEAT, default OFF, which
// defines _CHEAT directly); this covers any non-CMake path. Guarded so the two
// can never collide into a macro redefinition warning.
#ifndef EN_CHEAT
#define EN_CHEAT 0
#endif
#if EN_CHEAT && !defined( _CHEAT )
#define _CHEAT 1
#endif


#endif
