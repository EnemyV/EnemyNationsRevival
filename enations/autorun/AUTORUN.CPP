#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include <fcntl.h>
#include "resource.h"

HINSTANCE hInst = NULL;
HBITMAP hBm = NULL;
HPALETTE hPal = NULL;
int iWid = 0, iHt = 0;
BOOL b8bit = TRUE;

#define APP_NAME "Enemy Nations"

BOOL GetUninstall (char * pBuf)
{

	*pBuf = 0;

	HKEY key;
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
						"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" APP_NAME,
						NULL, KEY_READ, &key) != ERROR_SUCCESS)
		return (FALSE);

	unsigned long iLen = 256;
	DWORD dwTyp;
	if (RegQueryValueEx (key, "UninstallString", NULL, &dwTyp, (unsigned char *) pBuf, &iLen) != ERROR_SUCCESS)
		return (FALSE);

	if ((dwTyp != REG_SZ) || (*pBuf == 0))
		return (FALSE);

	return (TRUE);
}

BOOL GetApp (char * pBuf)
{

	*pBuf = 0;

	HKEY key;
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
						"SOFTWARE\\Windward Studios\\Games\\" APP_NAME,
						NULL, KEY_READ, &key) != ERROR_SUCCESS)
		return (FALSE);

	unsigned long iLen = 256;
	DWORD dwTyp;
	if (RegQueryValueEx (key, "", NULL, &dwTyp, (unsigned char *) pBuf, &iLen) != ERROR_SUCCESS)
		return (FALSE);

	if ((dwTyp != REG_SZ) || (*pBuf == 0))
		return (FALSE);

	return (TRUE);
}

BOOL CALLBACK AutorunProc (HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_USER+0x1000 :
			if ((wParam != 1) || (lParam != 2))
				break;

			char sFile [130], sDir [145], sDrive[4];
			GetModuleFileName (NULL, sFile, 128);
			sFile [128] = 0;
			_splitpath (sFile, sDrive, sDir, NULL, NULL);
			strcpy (sFile, sDrive);
			strcat (sFile, sDir);
			strcat (sFile, "autorun.wav");
			sndPlaySound (sFile, SND_ASYNC);
			break;

		case WM_INITDIALOG : {
			// give it the icon
			SendMessage (hDlg, WM_SETICON, (WPARAM)TRUE, 
										(LPARAM) LoadIcon (hInst, MAKEINTRESOURCE (IDI_AUTORUN)));

			// can we uninstall?
			char sCmd [258];
			if (! GetUninstall (sCmd))
				EnableWindow (GetDlgItem (hDlg, IDC_UNINSTALL), FALSE);

			if (! GetApp (sCmd))
				EnableWindow (GetDlgItem (hDlg, IDOK), FALSE);

			// if 8-bit use the 8-bit art. Otherwise the 24-bit
			HDC hdc = GetDC (NULL);
			long lBits = GetDeviceCaps (hdc, BITSPIXEL);
			long lPlanes = GetDeviceCaps (hdc, PLANES);
			ReleaseDC (NULL, hdc);
			if ( (lBits <= 8) && (lPlanes <= 256) && ( (1L <<lBits) * lPlanes <= 256 ) )
				b8bit = TRUE;
			else
				b8bit = FALSE;

			// make it bigger and center it
			RECT rect;
			GetWindowRect (hDlg, &rect);
			int xScreen = GetSystemMetrics (SM_CXSCREEN);
			int yScreen = GetSystemMetrics (SM_CYSCREEN);
			int xWid = rect.right - rect.left;
			int yHt = rect.bottom - rect.top;

			// do we increase it?
			if ( (xWid < (xScreen*9)/10) && (yHt < (yScreen*9)/10) )
				{
				xWid = (xScreen * 9) / 10;
				yHt = (yScreen * 9) / 10;

				// move the buttons
				const int NUM_BTNS = 5;
				static int aiBtns [] = { IDOK, IDC_SETUP, IDXHELP, IDC_UNINSTALL, IDCANCEL };
				int xBtnPos = xWid / 30;
				int xBtnAdd = (xWid * 6) / 30;
				int xBtnWid = (xWid * 4) / 30;
				int yBtnPos = ( yHt * 229 ) / 269;
				int	yBtnHt = 12 + ( yHt * 25 ) / 540;
				for (int iBtn=0; iBtn<NUM_BTNS; iBtn++)
					{
					SetWindowPos ( GetDlgItem (hDlg, aiBtns[iBtn]), NULL, xBtnPos, yBtnPos, xBtnWid, yBtnHt, SWP_NOZORDER );
					xBtnPos += xBtnAdd;
					}
				}

			int x = (xScreen - xWid) / 2;
			int y = (yScreen - yHt) / 2;
			SetWindowPos ( hDlg, NULL, x, y, xWid, yHt, SWP_NOZORDER );

			// read the bitmap for the paint message
			char sFile [130], sDir [145], sDrive[4];
			GetModuleFileName (NULL, sFile, 128);
			sFile [128] = 0;
			_splitpath (sFile, sDrive, sDir, NULL, NULL);
			strcpy (sFile, sDrive);
			strcat (sFile, sDir);
			if (b8bit)
				strcat (sFile, "autorun.d8");
			else
				strcat (sFile, "autorun.d24");

			int hdl = _open (sFile, _O_BINARY | _O_RDONLY | _O_SEQUENTIAL);
			if (hdl >= 0)
				{
				int iLen = _lseek (hdl, 0, SEEK_END);
				_lseek (hdl, 0, SEEK_SET);
				char  * pBmp = (char  *) malloc (iLen);
				if (pBmp)
					{
					if (_read (hdl, pBmp, iLen) == iLen)
						{
						HDC hDc = GetDC (hDlg);
						if (hDc != NULL)
							{

							BITMAPFILEHEADER * pBfh = (BITMAPFILEHEADER *) pBmp;
							BITMAPINFO * pBi = (BITMAPINFO *) (pBfh + 1);
							BITMAPINFOHEADER * pBih = &(pBi->bmiHeader);

							if (((iWid = pBih->biWidth) != 0) && ((iHt = pBih->biHeight) != 0))
								{
								// create our palette
								if (b8bit)
									{
									LOGPALETTE * pPal = (LOGPALETTE *) malloc (sizeof (LOGPALETTE) + 256 * sizeof (PALETTEENTRY));
									if (pPal != NULL)
										{
										// get the palette
									  GetSystemPaletteEntries (hDc, 0, 256, &(pPal->palPalEntry[0]));
										pPal->palVersion = 0x300;
										pPal->palNumEntries = 256;

										// now we are setting it up for our window palette
										PALETTEENTRY * pPeDest = &(pPal->palPalEntry [10]);
										RGBQUAD * pRgb = &(pBi->bmiColors[10]);
										for (int iOn=10; iOn<246; iOn++, pPeDest++, pRgb++)
											{
											pPeDest->peFlags = 0;
											pPeDest->peRed = pRgb->rgbRed;
											pPeDest->peGreen = pRgb->rgbGreen;
											pPeDest->peBlue = pRgb->rgbBlue;
											}

										hPal = CreatePalette (pPal);
										SelectPalette (hDc, hPal, FALSE);
										RealizePalette (hDc);

										free (pPal);
										}
									}

								if ((hBm = CreateCompatibleBitmap (hDc, iWid = pBih->biWidth, 
																						iHt = pBih->biHeight)) != NULL)
									SetDIBits (hDc, hBm, 0,	iHt, pBmp + pBfh->bfOffBits, pBi, 
																				b8bit ? DIB_RGB_COLORS : DIB_PAL_COLORS);
								}

							ReleaseDC (hDlg, hDc);
							}
						}
					free (pBmp);
					}
				_close (hdl);
				}

			// we want to get up FAST so we post for the sound and let it display first
			PostMessage (hDlg, WM_USER+0x1000, 1, 2);
			return (TRUE); }

		case WM_PALETTECHANGED :
			if ( ! b8bit )
				break;

			if ((HWND)wParam == hDlg)
				break;
			// fall through to WM_QUERYNEWPALETTE
		case WM_QUERYNEWPALETTE : {
			if ( ! b8bit )
				break;

			if (hPal == NULL)
				break;

			HDC hDc = GetDC (hDlg);
			SelectPalette (hDc, hPal, FALSE);
			if (RealizePalette (hDc))
				InvalidateRect (hDlg, NULL, FALSE);
			break; }

		case WM_PAINT : {
			if (hBm == NULL)
				break;

			PAINTSTRUCT ps;
			HDC hDc = BeginPaint (hDlg, &ps);

			if ( b8bit )
				{
				SelectPalette (hDc, hPal, FALSE);
				RealizePalette (hDc);
				}

			SetStretchBltMode (hDc, COLORONCOLOR );

			HDC hDcBm = CreateCompatibleDC (hDc);
			HGDIOBJ hOld = SelectObject (hDcBm, hBm);

			RECT rect;
			GetClientRect (hDlg, &rect);

			// scale it to the window
			StretchBlt (hDc, 0, 0, rect.right, rect.bottom, hDcBm, 0, 0, iWid, iHt, SRCCOPY);

			SelectObject (hDcBm, hOld);
			EndPaint (hDlg, &ps);
			return (TRUE); }

		case WM_COMMAND :
			switch (LOWORD (wParam))
			  {
				case IDOK : {
					char sCmd [258];
					if (! GetApp (sCmd))
						{
						EnableWindow (GetDlgItem (hDlg, IDOK), FALSE);
						return (TRUE);
						}

					STARTUPINFO sui;
					memset (&sui, 0, sizeof (sui));
					sui.cb = sizeof (sui);
					PROCESS_INFORMATION pi;
					if (CreateProcess (NULL, sCmd, NULL, NULL, FALSE, 
														NORMAL_PRIORITY_CLASS, NULL, NULL, &sui, &pi))
						{
						EndDialog (hDlg, IDOK);
						return (TRUE);
						}

					// disable if failed
					EnableWindow (GetDlgItem (hDlg, IDOK), FALSE);

					// it couldn't find uninstall - say why
					char * lpMessageError;
					FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
 										NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
									  (LPTSTR) &lpMessageError, 0, NULL);

					char sMsg [132];
					LoadString (hInst, IDS_NO_APP, sMsg, 132);
					char * pArgs[2] = { sCmd, lpMessageError };
					char * lpMessageBuffer;
					FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
 										sMsg, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
									  (LPTSTR) &lpMessageBuffer, 0, pArgs);

					LocalFree (lpMessageError);
					char sTitle [42];
					LoadString (hInst, IDS_TITLE, sTitle, 40);
					MessageBox (hDlg, lpMessageBuffer, sTitle, MB_ICONSTOP | MB_OK);
					LocalFree (lpMessageBuffer);
					return (TRUE); }

				case IDC_SETUP : {
					// lets get the base directory
					char sFile [130], sDir [145], sDrive[4];
					GetModuleFileName (NULL, sFile, 128);
					sFile [128] = 0;
					_splitpath (sFile, sDrive, sDir, NULL, NULL);
					strcpy (sFile, sDrive);
					strcat (sFile, sDir);
					strcat (sFile, "..");
					_fullpath (sDir, sFile, 128);

					strcpy (sDir, sFile);
					strcat (sFile, "\\setup.exe");

					STARTUPINFO sui;
					memset (&sui, 0, sizeof (sui));
					sui.cb = sizeof (sui);
					PROCESS_INFORMATION pi;
					BOOL bOk = CreateProcess (sFile, NULL, NULL, NULL, FALSE, 
														NORMAL_PRIORITY_CLASS, NULL, sDir, &sui, &pi);
					if (bOk)
						{
						EndDialog (hDlg, IDC_SETUP);
						return (TRUE);
						}

					// it couldn't find setup - say why
					char * lpMessageError;
					FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
 										NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
									  (LPTSTR) &lpMessageError, 0, NULL);

					char sMsg [132];
					LoadString (hInst, IDS_NO_SETUP, sMsg, 132);
					char * pArgs[2] = { sFile, lpMessageError };
					char * lpMessageBuffer;
					FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
 										sMsg, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
									  (LPTSTR) &lpMessageBuffer, 0, pArgs);

					LocalFree (lpMessageError);
					char sTitle [42];
					LoadString (hInst, IDS_TITLE, sTitle, 40);
					MessageBox (hDlg, lpMessageBuffer, sTitle, MB_ICONSTOP | MB_OK);
					LocalFree (lpMessageBuffer);
					return (TRUE); }

				case IDC_UNINSTALL : {
					char sTitle [42];
					LoadString (hInst, IDS_TITLE, sTitle, 40);
					char sCmd [258];
					LoadString ( hInst, IDS_UNINSTALL, sCmd, 256 );
					if ( MessageBox (hDlg, sCmd, sTitle, MB_ICONSTOP | MB_YESNO) != IDYES )
						return (TRUE);
					LoadString ( hInst, IDS_UNINSTALL_2, sCmd, 256 );
					if ( MessageBox (hDlg, sCmd, sTitle, MB_ICONSTOP | MB_YESNO) != IDYES )
						return (TRUE);

					if (! GetUninstall (sCmd))
						{
						EnableWindow (GetDlgItem (hDlg, IDC_UNINSTALL), FALSE);
						return (TRUE);
						}

					STARTUPINFO sui;
					memset (&sui, 0, sizeof (sui));
					sui.cb = sizeof (sui);
					PROCESS_INFORMATION pi;
					if (CreateProcess (NULL, sCmd, NULL, NULL, FALSE, 
														NORMAL_PRIORITY_CLASS, NULL, NULL, &sui, &pi))
						{
						EndDialog (hDlg, IDC_UNINSTALL);
						return (TRUE);
						}

					// disable if failed
					EnableWindow (GetDlgItem (hDlg, IDC_UNINSTALL), FALSE);

					// it couldn't find uninstall - say why
					char * lpMessageError;
					FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
 										NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
									  (LPTSTR) &lpMessageError, 0, NULL);

					char sMsg [132];
					LoadString (hInst, IDS_NO_UNINSTALL, sMsg, 132);
					char * pArgs[2] = { sCmd, lpMessageError };
					char * lpMessageBuffer;
					FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
 										sMsg, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
									  (LPTSTR) &lpMessageBuffer, 0, pArgs);

					LocalFree (lpMessageError);
					MessageBox (hDlg, lpMessageBuffer, sTitle, MB_ICONSTOP | MB_OK);
					LocalFree (lpMessageBuffer);
					return (TRUE); }
					
				case IDXHELP :
					// lets get the base directory
					char sFile [130], sDir [145], sDrive[4];
					GetModuleFileName (NULL, sFile, 128);
					sFile [128] = 0;
					_splitpath (sFile, sDrive, sDir, NULL, NULL);
					strcpy (sFile, sDrive);
					strcat (sFile, sDir);
					strcat (sFile, "..\\enations.hlp");

					WinHelp (hDlg, sFile, HELP_CONTENTS, 0);
					return (TRUE);

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

	// don't run if game running
	if ( ::FindWindow ("EnemyNationsMainWindow", "Enemy Nations") != NULL )
		return (FALSE);

	hInst = hInstance;
	int iRtn = DialogBox (hInst, MAKEINTRESOURCE (IDD_AUTORUN), NULL, (DLGPROC) AutorunProc);

	sndPlaySound (NULL, SND_ASYNC);

	return (iRtn);
	
}
