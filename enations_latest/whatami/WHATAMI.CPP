#include <windows.h>
#include <ver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource.h"

HINSTANCE hInst = NULL;

BOOL TestLib (char *pFile)
{

	UINT uMode = SetErrorMode (SEM_NOOPENFILEERRORBOX);
	HINSTANCE hLib = LoadLibrary (pFile);
	SetErrorMode (uMode);

#ifdef WIN32
	if ( hLib == NULL )
		return (FALSE);
#else
	if ((hLib < HINSTANCE_ERROR) && (hLib != 21))
		return (FALSE);
#endif

	FreeLibrary (hLib);
	return (TRUE);
}

BOOL TestLibList (char * * ppFile, int iNum)
{

	while (iNum--)
		{
		if (! TestLib (*ppFile))
			return (FALSE);
		ppFile++;
		}

	return (TRUE);
}

//const int NUM_VC_LIBS = 3;
//char * vcLibs[NUM_VC_LIBS] = {"msvcrt40.dll", "mfc40.dll", "ctl3d32.dll"};
const int NUM_DX_LIBS = 2;
char * dxLibs[NUM_DX_LIBS] = {"ddraw.dll", "dsound.dll"};
const int NUM_WG_LIBS = 3;
char * wgLibs[NUM_WG_LIBS] = {"wing.dll", "wing32.dll", "wingde.dll"};
const int NUM_32_LIBS = 1;
char * w32Libs[NUM_WG_LIBS] = {"w32sys.dll"};

BOOL CALLBACK MainDlgProc (HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			// center it
			RECT rect;
			GetWindowRect (hDlg, &rect);
			int x = (GetSystemMetrics (SM_CXSCREEN) - (rect.right - rect.left)) / 2;
			int y = (GetSystemMetrics (SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
			SetWindowPos (hDlg, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

			int iOS;
			enum { W31, W95, WNT };
			char sBuf[40];

			// get the O/S
#ifdef WIN32
			OSVERSIONINFO ovi;
			memset ( &ovi, 0, sizeof (ovi) );
			ovi.dwOSVersionInfoSize = sizeof (ovi);
			GetVersionEx ( &ovi );
			switch ( ovi.dwPlatformId )
			  {
				case VER_PLATFORM_WIN32s : {
					LoadString (hInst, IDS_STRING3, sBuf, 38);
					char sBuf2[60];
					sprintf (sBuf2, "%s %d.%d.%d", sBuf, ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);
					SetDlgItemText (hDlg, IDC_OS, sBuf2);
					iOS = W31;
					break; }
				case VER_PLATFORM_WIN32_WINDOWS : {
					iOS = W95;
					LoadString (hInst, IDS_STRING2, sBuf, 38);
					char sBuf2[60];
					sprintf (sBuf2, "%s %d.%d.%d", sBuf, ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);
					SetDlgItemText (hDlg, IDC_OS, sBuf2);
					break; }
				default : {
					iOS = WNT;
					LoadString (hInst, IDS_STRING1, sBuf, 38);
					char sBuf2[60];
					sprintf (sBuf2, "%s %d.%d.%d", sBuf, ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);
					SetDlgItemText (hDlg, IDC_OS, sBuf2);
					break; }
			  }

#else
			DWORD dwVer = GetVersion ();
			WORD wVer = LOWORD ( dwVer );
			WORD wDos = HIWORD ( dwVer );
			if ((LOBYTE (wDos) == 0) || (LOBYTE (wVer) > 3))
				{
				iOS = WNT;
				LoadString (hInst, IDS_STRING1, sBuf, 38);
				SetDlgItemText (hDlg, IDC_OS, sBuf);
				}
			else
				if (HIBYTE (wVer) >= 95)
					{
					iOS = W95;
					LoadString (hInst, IDS_STRING2, sBuf, 38);
					SetDlgItemText (hDlg, IDC_OS, sBuf);
					}
				else
					{
					iOS = W31;
					LoadString (hInst, IDS_STRING3, sBuf, 38);
					char sBuf2[60];
					sprintf (sBuf2, "%s %d.%d", sBuf, LOBYTE (wVer), HIBYTE (wVer));
					SetDlgItemText (hDlg, IDC_OS, sBuf2);
					}
#endif

			char sInst[20], sNot[20], sNA[20];
			LoadString (hInst, IDS_STRING4, sNA, 18);
			LoadString (hInst, IDS_STRING5, sInst, 18);
			LoadString (hInst, IDS_STRING6, sNot, 18);

			SetDlgItemText (hDlg, IDC_VC, sNA );
#ifdef BUGBUG
			// get the VC++ DLLs
			BOOL bVC = TestLibList (vcLibs, NUM_VC_LIBS);
			SetDlgItemText (hDlg, IDC_VC, bVC ? sInst : sNot);
#endif
			BOOL bVC = TRUE;

			// get DirectX
			BOOL bDX = FALSE;
			if (iOS != W95)
				SetDlgItemText (hDlg, IDC_DX, sNA);
			else
				{
				bDX = TestLibList (dxLibs, NUM_DX_LIBS);
				if ( ! bDX )
					SetDlgItemText (hDlg, IDC_DX, sNot );
				else
				  {

#ifndef WIN32
					SetDlgItemText (hDlg, IDC_DX, sInst );
#else

					char sBuf2[60], sDir[140];
					GetWindowsDirectory (sDir, 128);
					strcat (sDir, "\\system\\ddraw.dll");
					DWORD dwHdl = NULL;
					int iSize = (int) GetFileVersionInfoSize (sDir, &dwHdl);
					if (iSize == 0)
						bDX = FALSE;
					else
						{
						void *pBuf = malloc (iSize);
						GetFileVersionInfo (sDir, dwHdl, iSize, pBuf);
						void FAR * pData;
						UINT uiSize = sizeof (VS_FIXEDFILEINFO);
						VerQueryValue ( pBuf, "\\", &pData, &uiSize );
						VS_FIXEDFILEINFO FAR * pVffi = (VS_FIXEDFILEINFO FAR *) pData;
						sprintf (sBuf2, "DirectX  Version %d.%d.%d.%d",  HIWORD (pVffi->dwFileVersionMS), 
																				LOWORD (pVffi->dwFileVersionMS), 
																				HIWORD (pVffi->dwFileVersionLS), 
																				LOWORD (pVffi->dwFileVersionLS) );
						SetDlgItemText (hDlg, IDC_DX, sBuf2 );
						if ( (pVffi->dwFileVersionMS < 0x40004) || 
									( (pVffi->dwFileVersionMS == 0x40004) && (pVffi->dwFileVersionLS < 0x44) ) )
							bDX = FALSE;
						else
							bDX = TRUE;
				  	}
#endif
				  }
				}

			// get Win32s
			BOOL b32S = FALSE;
			if (iOS != W31)
				SetDlgItemText (hDlg, IDC_32S, sNA);
			else
				{
				b32S = TestLibList (w32Libs, NUM_32_LIBS);
				if (! b32S)
					SetDlgItemText (hDlg, IDC_32S, sNot);
				else
					{
					char sVer[20], sBuf2[60], sDir[140];
					GetWindowsDirectory (sDir, 128);
					strcat (sDir, "\\system\\win32s.ini");
					GetPrivateProfileString ("Win32s", "Version", "0", sVer, 18, sDir);
					LoadString (hInst, IDS_STRING7, sBuf, 18);
					sprintf (sBuf2, "%s %s", sBuf, sVer);
					SetDlgItemText (hDlg, IDC_32S, sBuf2);

					// break ver down to see if ok
					b32S = TRUE;
					if (atoi (sVer) < 1)
						b32S = FALSE;
					else
						{
						if (atoi (sVer+2) < 30)
							b32S = FALSE;
						else
							if (atoi (sVer+5) < 172)
								b32S = FALSE;
						}
					}
				}

			// get WinG
			BOOL bWG = FALSE;
			if (iOS != W31)
				SetDlgItemText (hDlg, IDC_WING, sNA);
			else
				{
				bWG = TestLibList (wgLibs, NUM_WG_LIBS);
				SetDlgItemText (hDlg, IDC_WING, bWG ? sInst : sNot);
				}

			// what do they need to install
			sBuf[0] = 0;
			switch (iOS)
			  {
				case W31 :
					if ((! bVC) || (! b32S) || (! bWG))
						strcpy (sBuf, "setup31.exe\r\n");
					break;
				case W95 :
					if ((! bVC) || (! bDX))
						strcpy (sBuf, "setup95.exe\r\n");
					break;
				case WNT :
					if (! bVC)
						strcpy (sBuf, "setupnt.exe\r\n");
					break;
			  }
			strcat (sBuf, "setupnet.exe");
			SetDlgItemText (hDlg, IDC_DOWNLOAD, sBuf);

			break; }

		case WM_COMMAND :
			switch (LOWORD (wParam))
			  {
				case IDOK :
				case IDCANCEL :
					EndDialog (hDlg, IDCANCEL);
					return (TRUE);
				}
			break;
	  }

	return (FALSE);
}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{

	hInst = hInstance;
	int iRtn = DialogBox (hInst, MAKEINTRESOURCE (IDD_MAIN), NULL, (DLGPROC) MainDlgProc);

	return (iRtn);
}
