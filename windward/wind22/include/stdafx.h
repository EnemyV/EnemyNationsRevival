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

#include <afxwin.h>   // MFC core and standard components
#include <afxext.h>   // MFC extensions (including VB)
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>   // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h> // This might cause problems?
#include <afxmt.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <MSAcm.h>

#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <strstream>
#include <io.h>
//#include <ctl3d.h>

#include <ddraw.h>
#include <dsound.h>
//#include <dplay.h>
//#include <wing.h>

// MSS32 removed - using SDL2 + SDL_mixer for audio

