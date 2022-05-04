#ifndef __THIELEN_INL__
#define __THIELEN_INL__

#ifdef _RELEASE_INLINE

#ifndef _WINDOWS
#ifndef _INC_MALLOC
#include <malloc.h>
#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// CDir

#ifdef BUGBUG
_RELEASE_INLINE CDir::CDir ()
	{ ctor (NULL); }
_RELEASE_INLINE CDir::CDir (char const *pDir)
	{ ctor (pDir); }
_RELEASE_INLINE CDir::CDir (CDir& src)
	{ bIncFile = src.bIncFile; sPathName = src.sPathName; }
_RELEASE_INLINE void CDir::operator= (const CDir& src)
	{ bIncFile = src.bIncFile; sPathName = src.sPathName; }
#endif


/////////////////////////////////////////////////////////////////////////////
// CGlobal

#ifndef _WINDOWS

_RELEASE_INLINE void __far * CGlobal::GetPtr (DWORD dwOff) const	// 4/26/96 - BobP
	{ return ((void __far *) ((char __far *)m_pvMem + dwOff)); }
_RELEASE_INLINE DWORD CGlobal::Size () const
	{ return (m_dwSize); }

_RELEASE_INLINE void __far * CGlobal::Alloc (DWORD dwSize)
	{ m_dwSize = dwSize; return (m_pvMem = _halloc (dwSize, 1)); }
_RELEASE_INLINE void CGlobal::Free ()
	{ _hfree (m_pvMem); m_pvMem = NULL; }
_RELEASE_INLINE void CGlobal::ctor ()
	{ m_pvMem = NULL; m_dwSize = 0; }

#else

_RELEASE_INLINE HGLOBAL CGlobal::Handle () const
	{ return (m_hGlb); }
_RELEASE_INLINE void FAR * CGlobal::GetPtr (DWORD dwOff) const	// 4/26/96 - BobP
	{ return ((void FAR *) ((char FAR *)m_pvMem + dwOff)); }
_RELEASE_INLINE DWORD CGlobal::Size () const
	{ return (m_dwSize); }

_RELEASE_INLINE CResource::CResource ()
	{ ctor (); }
_RELEASE_INLINE void FAR * CResource::Load (HINSTANCE hInst, int iName, int iType)
	{ return (Load (hInst, MAKEINTRESOURCE (iName), MAKEINTRESOURCE (iType)));}
_RELEASE_INLINE void FAR * CResource::GetPtr (DWORD dwOff) const	// 4/26/96 - BobP
	{ return ((void FAR *) ((char FAR *)m_pvMem + dwOff)); }

#endif //_WINDOWS


#endif //_RELEASE_INLINE

#endif

