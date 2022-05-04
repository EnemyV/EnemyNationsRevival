//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "_windwrd.h"
#include "_res.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "init.h"

int iWinType = WNT;	// the default
CWinApp * ptheApp = NULL;

// for asserts
int __iAssertPriority = ASSERT_PRI_CRITICAL;
int __iAssertSection = -1;

void PureFunc ()
{

	AfxMessageBox ("Pure Virtual Function - report the addresses in the next MessageBox",
																									MB_OK | MB_ICONSTOP | MB_TASKMODAL);
	// force a GPF
	char * pBuf = 0;
	char ch = * pBuf;
}

BOOL GetDllVersion ( char const * pFile, DWORD & dwMS, DWORD & dwLS )
{

	dwMS = dwLS = 0;

	char sDir[140];
	GetSystemDirectory (sDir, 128);
	strcat (sDir, "\\");
	strcat (sDir, pFile);
	DWORD dwHdl = NULL;
	int iSize = (int) GetFileVersionInfoSize (sDir, &dwHdl);
	if (iSize == 0)
		return (FALSE);
		
	void *pBuf = malloc (iSize);
	GetFileVersionInfo (sDir, dwHdl, iSize, pBuf);
	void FAR * pData;
	UINT uiSize = sizeof (VS_FIXEDFILEINFO);
	VerQueryValue ( pBuf, "\\", &pData, &uiSize );

	VS_FIXEDFILEINFO FAR * pVffi = (VS_FIXEDFILEINFO FAR *) pData;
	dwMS = pVffi->dwFileVersionMS;
	dwLS = pVffi->dwFileVersionLS;

	free ( pBuf );
	return ( TRUE );
}	

// this sets up an app for us
// returns TRUE if can run
void InitWindwardLib1 (CWinApp const * pWa)
{

	ptheApp = (CWinApp *) pWa;
}

BOOL InitWindwardLib2 ()
{

	// make sure we have 256+ colors
	if (ptheApp->GetProfileInt ("Compatibility", "NumColors", 0) != 1)
		{
		HDC hdc = GetDC (NULL);
		// note: 1<<BITSPIXEL overflows for 32-bit
		long lBits = GetDeviceCaps (hdc, BITSPIXEL);
		long lPlanes = GetDeviceCaps (hdc, PLANES);
		ReleaseDC (NULL, hdc);

		if ((lBits < 8) && (lPlanes < 256) &&
			((1L << GetDeviceCaps (hdc, BITSPIXEL)) * (long) GetDeviceCaps (hdc, PLANES) < 256))
			{
			AfxMessageBox (IDS_256_MIN, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
			return (FALSE);
			}
		}

	// make sure 3.1+ (protected mode guaranteed in Win32s)
	OSVERSIONINFO ovi;
	memset (&ovi, 0, sizeof (ovi));
	ovi.dwOSVersionInfoSize = sizeof (ovi);
	if (! GetVersionEx (&ovi))
		{
		AfxMessageBox (IDS_NO_WIN_VER, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
		return (FALSE);
		}

	switch (ovi.dwPlatformId)
	  {
		case VER_PLATFORM_WIN32s:
			iWinType = W32s;
			break;
		case 1 : // BUGBUG - VER_PLATFORM_WIN32_WINDOWS :
			iWinType = W95;
			break;
		default: // if its new hopefully its NT compatible
			iWinType = WNT;
			break;
	  }

	if (ptheApp->GetProfileInt ("Compatibility", "Version", 0) != 1)
		switch (ovi.dwPlatformId)
  		{
			case VER_PLATFORM_WIN32s: {
				if ((ovi.dwMajorVersion == 1) && (ovi.dwMinorVersion < 3))
					{
					AfxMessageBox (IDS_NEED_32S, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
					return (FALSE);
					}
				WORD wVer = LOWORD (GetVersion ());
				if ((HIBYTE (wVer) == 3) && (LOBYTE (wVer) < 10))
					{
					AfxMessageBox (IDS_NEED_31, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
					return (FALSE);
					}
				break; }
			default: // if its new hopefully its NT compatible
				if ((ovi.dwMajorVersion == 3) && (ovi.dwMinorVersion < 51))
					{
					AfxMessageBox (IDS_NEED_351, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
					return (FALSE);
					}
				break;
			}

	// check DirectX version
	DWORD dwMS, dwLS;
	if ( GetDllVersion ( "ddraw.dll", dwMS, dwLS ) )
		if ( (dwMS < 0x40000) || ( (dwMS == 0x40000) && (dwLS < 0x55E0001) ) )
			{
			CString sText, sNum1, sNum2, sNum3, sNum4;
			sText.LoadString ( IDS_DDRAW_VER );
			sNum1 = IntToCString ( HIWORD ( dwMS ) );
			sNum2 = IntToCString ( LOWORD ( dwMS ) );
			sNum3 = IntToCString ( HIWORD ( dwLS ) );
			sNum4 = IntToCString ( LOWORD ( dwLS ) );
			csPrintf ( &sText, (char const *) sNum1, (char const *) sNum2, (char const *) sNum3, (char const *) sNum4);
			CDlgMsg dlg;
			if ( dlg.MsgBox (sText, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "DirectDraw" ) != IDYES)
				return (FALSE);
			}
	if ( GetDllVersion ( "dsound.dll", dwMS, dwLS ) )
		if ( (dwMS < 0x40000) || ( (dwMS == 0x40000) && (dwLS < 0x55B0001) ) )
			{
			CString sText, sNum1, sNum2, sNum3, sNum4;
			sText.LoadString ( IDS_DSOUND_VER );
			sNum1 = IntToCString ( HIWORD ( dwMS ) );
			sNum2 = IntToCString ( LOWORD ( dwMS ) );
			sNum3 = IntToCString ( HIWORD ( dwLS ) );
			sNum4 = IntToCString ( LOWORD ( dwLS ) );
			csPrintf ( &sText, (char const *) sNum1, (char const *) sNum2, (char const *) sNum3, (char const *) sNum4);
			CDlgMsg dlg;
			if ( dlg.MsgBox (sText, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "DirectSound" ) != IDYES)
				return (FALSE);
			}

#ifdef _DEBUG
	// get assert flags here
	__iAssertPriority = ptheApp->GetProfileInt ("Logging", "Priority", ASSERT_PRI_CRITICAL);
	__iAssertSection = ptheApp->GetProfileInt ("Logging", "Section", -1);
#endif

	return (TRUE);
}	