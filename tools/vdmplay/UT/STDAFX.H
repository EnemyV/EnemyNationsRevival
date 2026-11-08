#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma warning (disable : 4201)

#define WIN32_LEAN_AND_MEAN
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#ifndef CONST
#define CONST const
#endif 
#define IN
#define OUT
#define INOUT

#ifndef WIN32
# define USHORT unsigned short
# define APIENTRY FAR PASCAL __export
# define GWL_USERDATA 0
# define MAKEWORD(a, b)   ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif


#if 0
#include <mmsystem.h>
#include <nb30.h>
#include <afx.h>
#include <afxcoll.h>
#endif 

#include <generic.h>
#include <assert.h>
#define ASSERT assert 

#include "vdmplay.h"
#include "vputil.h"
#include "vplist.h"
#include "vpnet.h"

#ifdef WIN32
#define _fmemcpy memcpy
#endif 



#pragma warning (disable : 4704)

#define AFX_API_EXPORT  __declspec (dllexport)

#endif
