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
#include <stdarg.h>
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

void CStringDelete (CString & src, int iInd, int iLen)
{

//BUGBUG	ASSERT_VALID_CLASS (this);
	ASSERT ((iInd >= 0) && (iLen >= 0));
	ASSERT (iInd+iLen <= src.GetLength ());

	// delete to end?
	if (iInd+iLen >= src.GetLength ())
	  {
		if (iInd < src.GetLength ()-1)
			src.ReleaseBuffer (iInd);
		return;
	  }

	// this moves the rest of the string & \0 at end
	char * pBuf = src.GetBuffer (0);
	memmove (pBuf + iInd, pBuf + iInd + iLen,
						src.GetLength () - iInd - iLen + 1);
	src.ReleaseBuffer (src.GetLength () - iLen);
}

void CStringInsert (CString & src, int iInd, const char *pStr)
{

//BUGBUG	ASSERT_VALID_CLASS (this);
	ASSERT ((0 <= iInd) && (iInd <= src.GetLength ()));

	CString sTmp (src.Left (iInd) + pStr + src.Mid (iInd));
	src = sTmp;

#ifdef BUGBUG
	int iLen = strlen (pStr);
	if (iLen <= 0)
		return;

	int iRest;
	if (src.GetLength () >= iInd)
		iRest = src.GetLength () - iInd + 1;
	else
		{
		iRest = 1;
		iInd = src.GetLength ();
		}

	char *pBuf = src.GetBufferSetLength (src.GetLength () + iLen + 2);
	memmove (pBuf + iInd + iLen, pBuf + iInd, iRest);
	memcpy (pBuf + iInd, pStr, iLen);

	src.ReleaseBuffer (-1);
#endif
}

// replaces the %1, %2, ... in sFmt with the passed in strings
void __cdecl csPrintf (CString * pFmt, ... )
{

	va_list vaOn;
	va_start (vaOn, pFmt);

	int iStrOn = 1;
	BOOL bFound;
	do
	  {
		bFound = FALSE;
	  const char *pBuf = *pFmt;

		while ((pBuf = strchr (pBuf, '%')) != NULL)
			if (atoi (pBuf+1) != iStrOn)
				pBuf++;
			else
			  {
				bFound = TRUE;
			  char *pStr = va_arg (vaOn, char *);
			  int iInd = pBuf - *pFmt;

				int iLen = 1;
				while (isdigit (*(++pBuf)))
				  iLen++;

				CStringDelete (*pFmt, iInd, iLen);
				CStringInsert (*pFmt, iInd, pStr);
	 			pBuf = *pFmt;

				iStrOn++;
				break;
			  }
	  }
	while (bFound);
}

static void InsComma (char *pBuf)
{

	unsigned uLen = strlen (pBuf);
	unsigned uMove = 4;
	pBuf += uLen - 3;

	while (uLen > 3)
		{
		memmove (pBuf+1, pBuf, uMove);
		*pBuf = ',';

		pBuf -= 3;
		uLen -= 3;
		uMove += 4;
		}
}

CString LongToCString (long lNum, int iRadix, BOOL bComma)
{

	char sBuf[sizeof(long)*8 + sizeof(long)/3 + 4];
	_ltoa (lNum, sBuf, iRadix);

	if (bComma)
		InsComma (sBuf);

	CString sRtn (sBuf);
	return (sRtn);
}

CString IntToCString (int iNum, int iRadix, BOOL bComma)
{

	char sBuf[sizeof(int)*8 + sizeof(int)/3 + 4];
	_itoa (iNum, sBuf, iRadix);

	if (bComma)
		InsComma (sBuf);

	CString sRtn (sBuf);
	return (sRtn);
}

#ifdef _DEBUG
void AssertString (CString const *pStr)
{

	ASSERT (AfxIsValidAddress (pStr, sizeof (CString)));

	LPCTSTR pBuf = (LPCTSTR) *pStr;	
	if (pBuf != NULL)
		ASSERT (AfxIsValidAddress (pBuf, pStr->GetAllocLength () + 1));
	ASSERT (0 <= pStr->GetLength ());
	ASSERT (pStr->GetLength () <= pStr->GetAllocLength ());
}
#endif
