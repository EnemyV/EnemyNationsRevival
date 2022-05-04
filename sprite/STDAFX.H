// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#ifndef STDAFX_H
#define STDAFX_H

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdlgs.h>		//  Common dialogs.
#include <afxcmn.h>			//  Win95 common controls.

#ifdef _DEBUG

inline void TRAP (BOOL f=TRUE)
{
	if (f)
		_asm int 3;
}

#else

inline void TRAP (BOOL=TRUE)
{
}

#endif
#endif
