//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma warning ( disable : 4711 )

#include <afxwin.h>   // MFC core and standard components
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
#include <strstream>
#include <eh.h>
#include <ctype.h>
#include <locale.h>

#include <ddraw.h>
#include <dsound.h>
//#include <dplay.h>
#include <wing.h>

// NOTE: This import originally simply provided the filename. In order to keep import folders
//            relatively organized, and aid in disambiguation of import names, I chose to set the
//            entire tools folder as an import location, allowing folder names to aid in disambiguating
//            includes.
#include "mss\mssw.h"

//#define MEM_DEBUG 1
#include <smartheap\smrtheap.hpp>

#include "wind22\include\windward.h"
#include "vdmplay.h"

#pragma warning ( disable : 4244 ) // I don't like this!!!

#ifdef _DEBUG
#define  _CHEAT 1
#endif


#endif
