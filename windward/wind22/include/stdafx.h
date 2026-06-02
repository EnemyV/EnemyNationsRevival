//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently
//

// Rely on windows.h + mfc_compat.h to provide all the MFC types we still
// use. ASSERT/VERIFY/TRACE come from <cassert>.
#include <windows.h>
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
// Override MFC's ASSERT_VALID to work with both CObject-derived and
// non-CObject classes. MFC's version calls AfxAssertValidObject() which
// requires CObject*. This version calls AssertValid() directly, which
// any class can provide — same debug value, no CObject dependency.
#ifdef ASSERT_VALID
#undef ASSERT_VALID
#endif
#ifdef _DEBUG
// Non-fatal + null-safe: a null pOb logs instead of dereferencing into an AV,
// and AssertValid()'s own ASSERTs are now non-fatal too.
#define ASSERT_VALID(pOb) ( (pOb) ? (void)(pOb)->AssertValid() : EnAssertFire( "ASSERT_VALID: null", __FILE__, __LINE__ ) )
#else
#define ASSERT_VALID(pOb) ((void)0)
#endif

#include "mfc_compat.h"

#include <mmsystem.h>
#include <mmreg.h>
#include <MSAcm.h>

#include <cassert>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <strstream>
#include <io.h>
//#include <ctl3d.h>

#include <dsound.h>
//#include <dplay.h>
//#include <wing.h>

// MSS32 removed - using SDL2 + SDL_mixer for audio

