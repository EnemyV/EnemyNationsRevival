#ifndef __THIELEN_H__
#define __THIELEN_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
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
#include "strstrea.h"
#endif
#ifndef __AFXCOLL_H__
#include "afxcoll.h"
#endif


// put this for all pure virtual functions
void PureFunc ();
#ifdef CHECK_PURE
#define PURE_FUNC { PureFunc (); return NULL; }
#else
#define PURE_FUNC		= 0;
#endif


BOOL GetDllVersion ( char const * pFile, DWORD & dwMS, DWORD & dwLS );


// use QUOTE for numbers in strings - "Hi " QUOTE(5) " there";
#define		QUOTE2(s)		#s
#define		QUOTE(s)		QUOTE2(s)


inline int __roll (int _low, int _hi, int _val)
{ 
	return (_val < _low ? _val + _hi : (_val >= _hi ? _val - _hi : _val));
}

inline int __minmax (int _low, int _hi, int _val)
{ 
	return (_val < _low ? _low : (_val > _hi ? _hi : _val));
}

template <class T>
T Max( T const & t1, T const & t2 )	// Requires T to define an operator <
{
	if ( t1 < t2 )		// Not ?: - can cause construction of a temporary
		return t2;

	return t1;
}

template <class T>
T Min( T const & t1, T const & t2 )	// Requires T to define an operator <
{
	if ( t1 < t2 )		// Not ?: - can cause construction of a temporary
		return t1;

	return t2;
}

#ifdef _DEBUG

inline void TRAP (BOOL f=TRUE)
{
	if (f)
		_asm int 3;
}

void AssertString (CString const *pStr);

#define ASSERT_VALID_OR_NULL(pOb)  ((pOb) == NULL ? 0 : ASSERT_VALID (pOb))
#define ASSERT_VALID_CSTRING(pSt)  (AssertString (pSt))
#define ASSERT_VALID_STRUCT(pSt)  (::AfxIsValidAddress(pSt, sizeof (*(pSt))) ? (pSt)->AssertValid () : NULL)
#define ASSERT_VALID_STRUCT_OR_NULL(pSt)  ((pSt) == NULL ? 0 : ASSERT_VALID_STRUCT (pSt))
#define ASSERT_VALID_STRUCT_RO(pSt)  (::AfxAssertValidStruct(pSt, sizeof (*(pSt)), FALSE, THIS_FILE, __LINE__))
#define ASSERT_VALID_STR_OR_NULL(pStr)  ((pStr) == NULL ? 0 : ASSERT (AfxIsValidString (pStr)))
#define ASSERT_VALID_ADDR_OR_NULL(pMem,iLen)  ((pMem) == NULL ? 0 : ASSERT (AfxIsValidAddress (pMem, iLen)))

#else

#pragma warning (disable : 4100 )
#ifdef _TRAP
inline void TRAP (BOOL f=TRUE)
{
	if (f)
		_asm int 3;
}
#else
inline void TRAP (BOOL f = TRUE) { }
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
#define		ASSERT_STRICT_VALID_STRUCT(f)		ASSERT_VALID_STRUCT (f)
#define		ASSERT_STRICT_VALID_OR_NULL(f)	ASSERT_VALID_OR_NULL (f)
#define		ASSERT_STRICT_VALID(f)					ASSERT_VALID (f)
#define		ASSERT_STRICT(f)								ASSERT (f)
#else
#define		ASSERT_STRICT_VALID_STRUCT(f)
#define		ASSERT_STRICT_VALID_OR_NULL(f)
#define		ASSERT_STRICT_VALID(f)
#define		ASSERT_STRICT(f)
#endif


/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

	class CDir;												// dir stuff
	class CFileName;									// a CFile that tracks the file name
	class	CGlobal;										// wraps a GlobalAlloc
	class	COStrStream;								// ostrstream that free's its memory on dtor


/////////////////////////////////////////////////////////////////////////////
// Functions

void CStringDelete (CString & src, int iInd, int iLen);
void CStringInsert (CString & src, int iInd, const char *pStr);
CString IntToCString (int iNum, int iRadix = 10, BOOL bComma = FALSE);
CString LongToCString (long lNum, int iRadix = 10, BOOL bComma = FALSE);
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
		CGlobal (CGlobal const & src);
		CGlobal operator= (CGlobal const & src);

		void __far *	Alloc (DWORD dwSize);
		void					Free ();
		void __far *	GetPtr (DWORD dwOff = 0) const;		// 4/26/96 - BobP
		DWORD					Size () const;

private:
		void					ctor ();

		void __far *	m_pvMem;
		DWORD					m_dwSize;
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

		void FAR *	Alloc (DWORD dwSize, UINT uFlags = GHND);
		void					Free ();
		HGLOBAL				Handle () const;
		void FAR *	ReAlloc (DWORD dwSize, UINT uFlags = GHND);
		void FAR *	GetPtr (DWORD dwOff = 0) const;		// 4/26/96 - BobP
		DWORD					Size () const;

private:
		void					ctor ();

		HGLOBAL				m_hGlb;
		void FAR *	m_pvMem;
		DWORD					m_dwSize;
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

		void FAR *	Load (HINSTANCE hInst, int iName, int iType);
		void FAR *	Load (HINSTANCE hInst, LPCSTR lpszName, LPCSTR lpszType);
		void					Free ();
		void FAR *	GetPtr (DWORD dwOff = 0) const;			// 4/26/96 - BobP

private:
		void					ctor ();
		HGLOBAL				m_hGlb;
		void FAR *	m_pvMem;
};
#endif

/////////////////////////////////////////////////////////////////////////////
// an ostrstream that free's its memory on dtor

class COStrStream : public ostrstream {
public:
        COStrStream() : ostrstream () {}
        COStrStream(char *pch, int nLen, int nMode = ios::out) : ostrstream (pch, nLen, nMode) {}

				// free up the memory
        ~COStrStream() {rdbuf()->freeze (0);}
};


/////////////////////////////////////////////////////////////////////////////
// CDir - directory stuff
// BUGBUG - implement the rest of this, trace test

#ifdef BUGBUG
typedef int (* FILEFINDPROC) (CFileStatus & fs, void * pvData);
typedef int (* WALKDIRPROC) (CDir & dir, void * pvData);
class CDir : public CObject
{
// Construction
public:
	CDir	();
	CDir	(char const *pDir);
	CDir	(CDir& src);
	void	operator= (const CDir& src);
private:
	void	ctor (char const *pDir);
	int		NextSub (const char *pDir, WALKDIRPROC fnEnum, void *pvData);

// Data
protected:
	CString					sPathName;
	BOOL						bIncFile;

// Attributes
public:

// Operations
public:
	void					AddFile (char const *pFile);
	unsigned			ChDir (BOOL bChDrive = FALSE);
	BOOL					DoesExist ();
	int						FindAll (FILEFINDPROC fnEnum, void * pvData = NULL, unsigned uAtr = 0x21);
	CString				GetDir (BOOL bIncDrv = TRUE, BOOL bIncEndSlash = TRUE);
	CString				GetDrive (BOOL bIncSemi = TRUE);
	CString				GetExt (BOOL bIncPer = FALSE);
	CString				GetFile ();
	CString				GetFullPath () {return (sPathName);}
	CString				GetRelPath (char const *pDir = NULL);
	BOOL					IsLocal ();
	unsigned			MkDir ();
	int						WalkDirs (WALKDIRPROC fnEnum, void * pvData = NULL);
	void					RmDir (BOOL bDelNode = FALSE);

// Static operations
	static	CString		SetName (char const *pDrv, char const *pDir, char const *pFile);
//	static	CDriveStatus	GetDriveStatus (char const *pDir);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};
#endif


/////////////////////////////////////////////////////////////////////////////
// CFileName

class CFileName : public CFile
{
	DECLARE_DYNAMIC(CFileName)

public:
	CFileName() {}
	CFileName(int hFile) : CFile (hFile) {}
	CFileName(const char* pszFileName, UINT nOpenFlags) :
							CFile (pszFileName, nOpenFlags),
							m_strFileName (pszFileName) {}

	virtual DWORD GetPosition() const;

	virtual BOOL Open(const char* pszFileName, UINT nOpenFlags,
							CFileException* pError = NULL);

	DWORD SeekToEnd();
	void SeekToBegin();

	DWORD ReadHuge(void FAR* lpBuffer, DWORD dwCount);
	void WriteHuge(const void FAR* lpBuffer, DWORD dwCount);

	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual void SetLength(DWORD dwNewLen);
	virtual DWORD GetLength() const;

	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);

	virtual void Flush();
	virtual void Close();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString		m_strFileName;
};


#endif

