//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// Written by David Thielen
//   Copyright 1994, All Rights Reserved


#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


void CGlobal::ctor ()
{

	m_hGlb = NULL;
	m_pvMem = NULL;
	m_dwSize = 0;
}

CGlobal::CGlobal (DWORD dwSize, UINT uFlags)
{

	ctor ();
	Alloc (dwSize, uFlags);
}

CGlobal::~CGlobal ()
{

	Free ();
}

void FAR * CGlobal::Alloc (DWORD dwSize, UINT uFlags)
{

	ASSERT (m_hGlb == NULL);

	if ((m_hGlb = GlobalAlloc (uFlags, dwSize)) != NULL)
		m_pvMem = GlobalLock (m_hGlb);

	m_dwSize = m_pvMem != NULL ? dwSize : 0;

	return (m_pvMem);
}

void CGlobal::Free ()
{

	if (m_pvMem != NULL)
		GlobalUnlock (m_hGlb);
	if (m_hGlb != NULL)
		GlobalFree (m_hGlb);

	ctor ();
}

void FAR * CGlobal::ReAlloc (DWORD dwSize, UINT uFlags)
{

	if (m_hGlb == NULL)
		return (Alloc (dwSize, uFlags));

	if (m_pvMem != NULL)
	  {
	  GlobalUnlock (m_hGlb);
		m_pvMem = NULL;
	  }

	if ((m_hGlb = GlobalReAlloc (m_hGlb, dwSize, uFlags)) == NULL)
		return (NULL);

	m_pvMem = GlobalLock (m_hGlb);
	m_dwSize = m_pvMem != NULL ? dwSize : 0;

	return (m_pvMem);
}

void CResource::ctor ()
{

	m_hGlb = NULL;
	m_pvMem = NULL;
}

CResource::CResource (HINSTANCE hInst, LPCSTR lpszName, LPCSTR lpszType)
{

	ctor ();
	Load (hInst, lpszName, lpszType);
}

CResource::~CResource ()
{

	Free ();
}

void FAR * CResource::Load (HINSTANCE hInst, LPCSTR lpszName, LPCSTR lpszType)
{

	ASSERT (m_hGlb == NULL);

	HRSRC hRsrc;
	if ((hRsrc = FindResource (hInst, lpszName, lpszType)) == NULL)
		return (NULL);

	if ((m_hGlb = LoadResource (hInst, hRsrc)) == NULL)
		return (NULL);

	return (m_pvMem = LockResource (m_hGlb));
}

void CResource::Free ()
{

	if (m_pvMem != NULL)
	  UnlockResource (m_hGlb);
	if (m_hGlb != NULL)
	  FreeResource (m_hGlb);

	ctor ();
}

