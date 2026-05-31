#ifndef __THIELEN_H__
#define __THIELEN_H__


//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef _WINDEF_
#include "windef.h"
#endif

#ifndef __cplusplus
#error require C++ compilation (use a .cpp suffix)
#endif


#ifndef _INC_DOS
#include "dos.h"
#endif
#ifndef _INC_STRSTREAM
#include <ios>
#include <strstream>
#endif
// CString comes from mfc_compat.h (via stdafx.h); the IntToCString /
// CStringDelete helpers are dead in live code (Phase 5 replaced them
// with IntToStr / std::string).
class CWnd;


// put this for all pure virtual functions
void PureFunc ();
#ifdef CHECK_PURE
#define PURE_FUNC { PureFunc (); return NULL; }
#else
#define PURE_FUNC  = 0;
#endif


BOOL GetDllVersion ( char const * pFile, DWORD & dwMS, DWORD & dwLS );


// use QUOTE for numbers in strings - "Hi " QUOTE(5) " there";
#define  QUOTE2(s)  #s
#define  QUOTE(s)  QUOTE2(s)


inline int __roll (int _low, int _hi, int _val)
{ 
    return (_val < _low ? _val + _hi : (_val >= _hi ? _val - _hi : _val));
}

inline int __minmax (int _low, int _hi, int _val)
{ 
    return (_val < _low ? _low : (_val > _hi ? _hi : _val));
}

template <class T>
T Max( T const & t1, T const & t2 ) // Requires T to define an operator <
{
    if ( t1 < t2 )  // Not ?: - can cause construction of a temporary
        return t2;

    return t1;
}

template <class T>
T Min( T const & t1, T const & t2 ) // Requires T to define an operator <
{
    if ( t1 < t2 )  // Not ?: - can cause construction of a temporary
        return t1;

    return t2;
}

#ifdef _DEBUG

inline void TRAP (bool f=true)
{
    if (f)
        __debugbreak();   // was `_asm int 3` — x64 has no inline asm; this intrinsic is portable
}

void AssertString (CString const *pStr);

#define ASSERT_VALID_OR_NULL(pOb)  ((pOb) == NULL ? (void)0 : ASSERT_VALID (pOb))
#define ASSERT_VALID_CSTRING(pSt)  (AssertString (pSt))
#define ASSERT_VALID_STRUCT(pSt)  (::AfxIsValidAddress(pSt, sizeof (*(pSt))) ? (pSt)->AssertValid () : (void)NULL)
#define ASSERT_VALID_STRUCT_OR_NULL(pSt)  ((pSt) == NULL ? (void)0 : ASSERT_VALID_STRUCT (pSt))
#define ASSERT_VALID_STRUCT_RO(pSt)  (::AfxAssertValidStruct(pSt, sizeof (*(pSt)), false, THIS_FILE, __LINE__))
#define ASSERT_VALID_STR_OR_NULL(pStr)  ((pStr) == NULL ? (void)0 : ASSERT (AfxIsValidString (pStr)))
#define ASSERT_VALID_ADDR_OR_NULL(pMem,iLen)  ((pMem) == NULL ? (void)0 : ASSERT (AfxIsValidAddress (pMem, iLen)))

#else

#pragma warning (disable : 4100 )
#ifdef _TRAP
inline void TRAP (bool f=true)
{
    if (f)
        __debugbreak();   // was `_asm int 3` — x64 has no inline asm; this intrinsic is portable
}
#else

//#pragma clang diagnostic push
// ignore Unused Parameter warning here
//#pragma ide diagnostic ignored "UnusedParameter"
inline void TRAP (bool f = true) { }
//#pragma clang diagnostic pop

#endif
#pragma warning (default : 4100 )

#define ASSERT_VALID_OR_NULL(pOb)
#define ASSERT_VALID_CSTRING(pOb)
#define ASSERT_VALID_STRUCT(pSt)
#define ASSERT_VALID_STRUCT_OR_NULL(pSt)
#define ASSERT_VALID_STRUCT_RO(pSt)
#define ASSERT_VALID_STR_OR_NULL(pStr)
#define ASSERT_VALID_ADDR_OR_NULL(pMem,iLen)
#endif

#ifdef _STRICT_DEBUG
#define  ASSERT_STRICT_VALID_STRUCT(f)  ASSERT_VALID_STRUCT (f)
#define  ASSERT_STRICT_VALID_OR_NULL(f) ASSERT_VALID_OR_NULL (f)
#define  ASSERT_STRICT_VALID(f)     ASSERT_VALID (f)
#define  ASSERT_STRICT(f)        ASSERT (f)
#else
#define  ASSERT_STRICT_VALID_STRUCT(f)
#define  ASSERT_STRICT_VALID_OR_NULL(f)
#define  ASSERT_STRICT_VALID(f)
#define  ASSERT_STRICT(f)
#endif


/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

    class CGlobal;                              // wraps a GlobalAlloc
    class COStrStream;                          // ostrstream that free's its memory on dtor


/////////////////////////////////////////////////////////////////////////////
// Functions

void CStringDelete (CString & src, int iInd, int iLen);
void CStringInsert (CString & src, int iInd, const char *pStr);
CString IntToCString (int iNum, int iRadix = 10, BOOL bComma = false);
CString LongToCString (long lNum, int iRadix = 10, BOOL bComma = false);
void __cdecl csPrintf (CString * pFmt, ...);

#ifdef _WINDOWS
void EnableAllWindows (HWND hWnd, BOOL bEnable);
typedef int (CALLBACK * LOOPPROC) ();
int WINAPI ExtMsgBox (const char *psText, UINT uStyle, long lHelp = -1, const char *psButtons = NULL, LOOPPROC lpFn = NULL);
int WINAPI ExtMsgBox (HWND hWndPar, const char *psText, const char *psTitle, UINT uStyle, long lHelp = -1, const char *psButtons = NULL, LOOPPROC lpFn = NULL);
int WINAPI ExtMsgBox (CWnd *pWnd, const char *psText, int iRes, UINT uStyle, long lHelp = -1, const char *psButtons = NULL, LOOPPROC lpFn = NULL);
#endif


/////////////////////////////////////////////////////////////////////////////
// a wrapper for HGLOBAL

#ifndef _WINDOWS

class CGlobal
{
public:
        CGlobal () { ctor (); }
        CGlobal (DWORD dwSize, UINT uFlags = 0);
        ~CGlobal () { Free (); }
        // below declared but not implemented so defaults are dead
        CGlobal (CGlobal const & src) = delete;
        CGlobal operator= (CGlobal const & src) = delete;

        HGLOBAL Alloc (DWORD dwSize);
        void    Free ();
        HGLOBAL GetPtr (DWORD dwOff = 0) const;  // 4/26/96 - BobP
        DWORD   Size () const;

private:
        void    ctor ();

        HGLOBAL m_pvMem;
        DWORD   m_dwSize;
};

#else

class CGlobal
{
public:
        CGlobal () {ctor ();}
        CGlobal (DWORD dwSize, UINT uFlags = GHND);
        ~CGlobal ();
        // below declared but not implemented so defaults are dead
        CGlobal (CGlobal const & src);
        CGlobal operator= (CGlobal const & src);

        void*   Alloc (DWORD dwSize, UINT uFlags = GHND);
        void    Free ();
        HGLOBAL Handle () const;
        void*   ReAlloc (DWORD dwSize, UINT uFlags = GHND);
        void*   GetPtr (DWORD dwOff = 0) const;  // 4/26/96 - BobP
        DWORD   Size () const;

private:
        void    ctor();

        HGLOBAL m_hGlb;
        void*   m_pvMem;
        DWORD   m_dwSize;
};

class CResource
{
public:
        CResource ();
        CResource (HINSTANCE hInst, LPCSTR lpszName, LPCSTR lpszType);
        ~CResource ();
        // below declared but not implemented so defaults are dead
        CResource (CResource const & src);
        CResource operator= (CResource const & src);

        void* Load (HINSTANCE hInst, int iName, int iType);
        void* Load (HINSTANCE hInst, LPCSTR lpszName, LPCSTR lpszType);
        void  Free ();
        void* GetPtr (DWORD dwOff = 0) const;   // 4/26/96 - BobP

private:
        void    ctor();
        HGLOBAL m_hGlb;
        void*   m_pvMem;
};
#endif

/////////////////////////////////////////////////////////////////////////////
// an ostrstream that free's its memory on dtor

class COStrStream final : public std::ostrstream {
public:
    COStrStream() : ostrstream () {}
    COStrStream(char *pch, int nLen, int nMode = std::ios::out) : ostrstream (pch, nLen, nMode) {}

    // free up the memory
    ~COStrStream() final {
        rdbuf()->freeze(false);
    }
};


/////////////////////////////////////////////////////////////////////////////
// CDir and CFileName classes removed — were unused dead code

#endif

