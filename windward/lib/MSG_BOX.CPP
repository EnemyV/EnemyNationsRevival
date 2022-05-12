//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------




#include "stdafx.h"
#include "_windwrd.h"


BOOL CALLBACK MsgBoxDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HWND hDlgBox = NULL;


#define		MB_DT_FLAGS		(DT_EXPANDTABS | DT_LEFT | DT_NOPREFIX | DT_WORDBREAK)

typedef struct tagMBDATA {
		volatile int	*piRtn;
		long					lHelp;
		WORD					wHelpBtn;
		HICON					hIcon;
		int						iXicon;
		int						iYicon;
} MBDATA;

#pragma pack (1)

typedef struct tagDialogBoxHeader {
    DWORD lStyle;
		DWORD dwExtendedStyle;
		WORD	cdit;
    short x;
    short y;
    short cx;
    short cy;
    char szMenuName[1];
    char szClassName[1];
    char szCaption[1];
} DLGBOXHDR;

typedef struct tagControlData {
    DWORD lStyle;
		DWORD	dwExtendedStyle;
    short x;
    short y;
    short cx;
    short cy;
    WORD  wID;
    BYTE bCtrlClass;
    char szText[1];
		BYTE bEnd;
} DLGCTRLDATA;

#pragma pack ()

static const char sOk[] = "OK\n";
static const char sOC[] = "OK\nCancel\n";
static const char sARI[] = "&Abort\n&Retry\n&Ignore\n";
static const char sYN[] = "&Yes\n&No\n";
static const char sYNC[] = "&Yes\n&No\nCancel\n";
static const char sRC[] = "&Retry\nCancel\n";
static const char sHelp[] = "&Help...";

static const char sEnableProp[] = "DNTenable";
static const char sApplProp[] = "DNTapplEnable";

BOOL CALLBACK EnableProc (HWND hWnd, LPARAM lParam)
{

	// do nothing if its the one we ignore
	if (hWnd == (HWND) LOWORD (lParam))
		return (TRUE);

	// in the property we keep the enabled/disabled state in the low
	//   byte and our recursion in the high byte
	if (HIWORD (lParam) == FALSE)
	  {
		WORD iCount = (WORD) GetProp (hWnd, sEnableProp);
		if (iCount == 0)
			iCount = IsWindowEnabled (hWnd) & 0xFF;
		ASSERT (HIBYTE (iCount) < 255);
		iCount += 0x100;												// inc HIBYTE
		SetProp (hWnd, sEnableProp, (HANDLE) iCount);
		EnableWindow (hWnd, FALSE);
	  }
	else
	  {
		WORD iCount = (WORD) GetProp (hWnd, sEnableProp);
		// will return 0 if this window was created after we disabled all
		if (iCount == 0)
			return (TRUE);
		ASSERT (HIBYTE (iCount) != 0);
		if (HIBYTE (iCount) != 0)
			iCount -= 0x100;
		if (HIBYTE (iCount) != 0)
			SetProp (hWnd, sEnableProp, (HANDLE) iCount);
		else
		  {
		  EnableWindow (hWnd, LOBYTE (iCount));
			RemoveProp (hWnd, sEnableProp);
		  }
	  }

	return (TRUE);
}

// dis/enable all other windows
void EnableAllWindows (HWND hWnd, BOOL bEnable)
{

	EnumThreadWindows (GetCurrentThreadId (), EnableProc, MAKELONG (hWnd, bEnable));
}


int WINAPI ExtMsgBox (const char *psText, UINT uStyle, long lHelp, const char *psButtons, LOOPPROC lpFn)
{

	ASSERT (AfxGetApp () != NULL);

	// the parent is the active window if it is part of our app
	HWND hWndPar = GetActiveWindow ();
	if (hWndPar != NULL)
		if (GetWindowThreadProcessId (hWndPar, NULL) != GetCurrentThreadId ())
			hWndPar = NULL;

	return (ExtMsgBox (hWndPar, psText, AfxGetAppName (), uStyle, lHelp, psButtons, lpFn));
}

int WINAPI ExtMsgBox (CWnd *pWnd, const char *psText, int iRes, UINT uStyle, long lHelp, const char *psButtons, LOOPPROC lpFn)
{

	CString sTitle;
	sTitle.LoadString (iRes);
	return (ExtMsgBox (pWnd->m_hWnd, psText, sTitle, uStyle, lHelp, psButtons, lpFn));
}

int WINAPI ExtMsgBox (HWND hWndPar, const char *psText, const char *psTitle,	UINT uStyle, long lHelp, const char *psButtons, LOOPPROC lpFn)
{
volatile int iRtn=-1;
MBDATA mbd;

	ASSERT (AfxGetInstanceHandle () != NULL);

	memset (&mbd, 0, sizeof (mbd));
	mbd.piRtn = &iRtn;

	// Lets build up the button string
	CString sBtn;
	UINT uTmp = uStyle & MB_TYPEMASK;
	if ((uTmp == MB_OK) && (psButtons == NULL))
		sBtn += sOk;
	else
		if (uTmp == MB_OKCANCEL)
			sBtn += sOC;
		else
			if (uTmp == MB_ABORTRETRYIGNORE)
				sBtn += sARI;
			else
				if (uTmp == MB_YESNOCANCEL)
					sBtn += sYNC;
				else
					if (uTmp == MB_YESNO)
						sBtn += sYN;
					else
						if (uTmp == MB_RETRYCANCEL)
							sBtn += sRC;
	if ((psButtons != NULL) && (*psButtons != 0))
		{
		sBtn += psButtons;
		if (sBtn[sBtn.GetLength() - 1] != '\n')
			sBtn += "\n";
		}
	if (lHelp != -1)
		sBtn += sHelp;
	else
		while (sBtn[sBtn.GetLength() - 1] == '\n')
			sBtn.ReleaseBuffer (sBtn.GetLength() - 1);

	// lets get the text size
	//   get the font size
	HDC hDc = GetDC (NULL);
	SelectObject (hDc, GetStockObject (SYSTEM_FONT));
	TEXTMETRIC tm;
	GetTextMetrics (hDc, &tm);
	int iWid = GetSystemMetrics (SM_CXFULLSCREEN);
	CRect rText (0,0,iWid,0), rBtn (0,0,iWid,0);
	DrawText (hDc, psText, -1, &rText, MB_DT_FLAGS | DT_CALCRECT);
	DrawText (hDc, sBtn, -1, &rBtn, DT_LEFT | DT_CALCRECT);
	ReleaseDC (NULL, hDc);

	// Determine the spacing between things
	int iYxtra = tm.tmHeight;
	int iXxtra = tm.tmMaxCharWidth;
	int iBtnHt = tm.tmHeight * 2;
	int iXbtnSpc = tm.tmAveCharWidth * 4;
	int iBtnWid = rBtn.right + tm.tmMaxCharWidth * 2;

	// set rText location
	rText.right += tm.tmMaxCharWidth;
	rText += CPoint (iXxtra, iYxtra);

	// determine the window size
	CRect rWin (rText);
	rWin.left = rWin.top = 0;
	rWin.right += iXxtra;
	rWin.bottom += iYxtra;

	// Add in icons
	if (uStyle & MB_ICONMASK)
	  {
		int iXshft = GetSystemMetrics (SM_CXICON) + iXxtra;
		rText += CPoint (iXshft, 0);
		rWin.right += iXshft;
		rWin.bottom = __max (rWin.bottom,
												GetSystemMetrics (SM_CYICON) + iYxtra * 2);
	  }

	// get the number of buttons
	int iNumBtns=1;
	const char * pBufOn = sBtn.GetBuffer (0);
	while ((pBufOn = strchr (pBufOn+1, '\n')) != NULL)
		iNumBtns++;
	ASSERT (iNumBtns <= 253);
	if (iNumBtns > 253)
		return (-2);

	// add in space for buttons (icon goes above buttons)
	rWin.bottom += iBtnHt + iYxtra;
	int iBtnLen = iNumBtns * (iBtnWid + iXbtnSpc) + iXbtnSpc;
	rWin.right = __max (iBtnLen, rWin.right);

	// create the dlg struct
	CGlobal cgDlg (sizeof (DLGBOXHDR) + strlen (psTitle) + sizeof (DLGCTRLDATA) + strlen (psText),
									GPTR);
	DLGBOXHDR FAR *pDbh = (DLGBOXHDR FAR *) cgDlg.GetPtr ();	// 4/26/96 BobP
	if (pDbh == NULL)
		return (-3);
	pDbh->lStyle = WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE;
	if (uStyle & MB_SYSTEMMODAL)
		pDbh->lStyle |=	DS_SYSMODAL;
	pDbh->cdit = (WORD) (iNumBtns + 1);
	pDbh->x = (WORD) ((rWin.left * 4) / tm.tmAveCharWidth);
	pDbh->y = (WORD) ((rWin.top * 8) / tm.tmHeight);
	pDbh->cx = (WORD) ((rWin.Width () * 4) / tm.tmAveCharWidth);
	pDbh->cy = (WORD) ((rWin.Height () * 8) / tm.tmHeight);
	strcpy (pDbh->szCaption, psTitle);

	// Add the text window
	// center it in its available space (rText.right - rWin.right)
	DLGCTRLDATA FAR *pDcd = (DLGCTRLDATA FAR *) (pDbh->szCaption + strlen (psTitle) + 1);
	rText += CPoint ((rWin.Width () - rText.right) / 2, 0);
	pDcd->x = (WORD) ((rText.left * 4) / tm.tmAveCharWidth);
	pDcd->y = (WORD) ((rText.top * 8) / tm.tmHeight);
	pDcd->cx = (WORD) ((rText.Width () * 4) / tm.tmAveCharWidth);
	pDcd->cy = (WORD) ((rText.Height () * 8) / tm.tmHeight);
	pDcd->wID = (WORD) -1;
	pDcd->lStyle = WS_CHILD | SS_LEFT | SS_NOPREFIX | WS_VISIBLE;
	pDcd->bCtrlClass = 0x82;
	strcpy (pDcd->szText, psText);
	pDcd->szText[strlen (psText) + 1] = 0;		// bEnd

	// add the icon data
	if (uStyle & MB_ICONMASK)
	  {
		LPCSTR id = IDI_ASTERISK;
		switch (uStyle & MB_ICONMASK)
			{
			case MB_ICONHAND :
				id = IDI_HAND;
				break;
			case MB_ICONQUESTION :
				id = IDI_QUESTION;
				break;
			case MB_ICONEXCLAMATION :
				id = IDI_EXCLAMATION;
				break;
			}
		if ((mbd.hIcon = LoadIcon (NULL, id)) != NULL)
			{
			mbd.iXicon = iXxtra;
			mbd.iYicon = iYxtra;
			}
	  }

	// add the Help... stuff
	if ((mbd.lHelp = lHelp) == -1)
		mbd.wHelpBtn = (WORD) -2;
	else
		mbd.wHelpBtn = (WORD) (iNumBtns - 1 + 10);

	// get the default button number
	int iDefButton = 0;
	switch (uStyle & MB_DEFMASK)
		{
		case MB_DEFBUTTON2 :
			iDefButton = 1;
			break;
		case MB_DEFBUTTON3 :
			iDefButton = 2;
			break;
		}

	// lets make the buttons
	pBufOn = sBtn.GetBuffer (0);
	int iBtnOn = 0;
	int iAddBtn =  iBtnWid + iXbtnSpc;
	int iXbtn = (rWin.Width() - iNumBtns * iAddBtn + iXbtnSpc) / 2;
	int iYbtn = rText.bottom + iYxtra;

	while (*pBufOn != 0)
	  {
		const char *pEnd = strchr (pBufOn, '\n');
		if (pEnd == NULL)
			pEnd = pBufOn + strlen (pBufOn);
		int iLen = pEnd - pBufOn;

		// set up the dialog template
		DWORD dwOff = cgDlg.Size ();
		cgDlg.ReAlloc (dwOff + sizeof (DLGCTRLDATA) + iLen);
		if ((pDcd = (DLGCTRLDATA FAR *) cgDlg.GetPtr (dwOff)) == NULL)	// 4/26/96 BobP
			return (-3);

		pDcd->x = (WORD) ((iXbtn * 4) / tm.tmAveCharWidth);
		pDcd->y = (WORD) ((iYbtn * 8) / tm.tmHeight);
		pDcd->cx = (WORD) ((iBtnWid * 4) / tm.tmAveCharWidth);
		pDcd->cy = (WORD) ((iBtnHt * 8) / tm.tmHeight);
		pDcd->lStyle = WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP;
		if (iBtnOn == iDefButton)
			pDcd->lStyle |= BS_DEFPUSHBUTTON;
		pDcd->bCtrlClass = 0x80;
		memcpy (pDcd->szText, pBufOn, iLen);
		pDcd->szText[iLen] = 0;
		pDcd->szText[iLen+1] = 0;		// bEnd

		// get the button return
		pDcd->wID = (WORD) (iBtnOn + 10);
		if (stricmp (pDcd->szText, "ok") == 0)
			pDcd->wID = IDOK;
		else
			if (stricmp (pDcd->szText, "cancel") == 0)
				pDcd->wID = IDCANCEL;
			else
				if (stricmp (pDcd->szText, "&yes") == 0)
					pDcd->wID = IDYES;
				else
					if (stricmp (pDcd->szText, "&no") == 0)
						pDcd->wID = IDNO;
					else
						if (stricmp (pDcd->szText, "&abort") == 0)
							pDcd->wID = IDABORT;
						else
							if (stricmp (pDcd->szText, "&retry") == 0)
								pDcd->wID = IDRETRY;
							else
								if (stricmp (pDcd->szText, "&ignore") == 0)
									pDcd->wID = IDIGNORE;

		iBtnOn++;
		iXbtn += iAddBtn;
		pBufOn = *pEnd == 0 ? pEnd : pEnd + 1;
		}

	// add 32 bytes of 0 (or CreateDialogInd GP faults)
	DWORD dwOff = cgDlg.Size ();
	cgDlg.ReAlloc (dwOff + 32);
	void FAR *pBuf;
	if ((pBuf = cgDlg.GetPtr (dwOff)) != NULL)						// 4/26/96 BobP
		memset (pBuf, 0, 32);

	HWND hDlg = CreateDialogIndirect (AfxGetInstanceHandle (),		// 4/26/96 BobP
					(const DLGTEMPLATE *) cgDlg.GetPtr (), hWndPar, MsgBoxDlgProc);
	if (hDlg == NULL)
		{
		ASSERT (FALSE);
		return (MessageBox (hWndPar, psText, psTitle, uStyle));
		}

	SetWindowLong (hDlg, DWL_USER, (LONG) (void FAR *) (&mbd));
	cgDlg.Free ();

	// disable parent
	if ((uStyle & (MB_SYSTEMMODAL | MB_TASKMODAL)) == 0)
		{
		HWND hParOn = hWndPar;
		while (hParOn != NULL)
			{
			SetProp (hParOn, sApplProp, (HANDLE) IsWindowEnabled (hParOn));
			EnableWindow (hParOn, FALSE);
			hParOn = GetParent (hParOn);
			}
		}

	if (uStyle & MB_TASKMODAL)
		EnableAllWindows (hDlg, FALSE);

	// get this puppy up
  EnableWindow (hDlg, TRUE);
  ShowWindow (hDlg, SW_SHOW);
	SetForegroundWindow (hDlg);

	// if we have an icon we need to invalidate it's area cause &mbd
	//   was set AFTER the WM_PAINT
	if (mbd.hIcon != 0)
	  {
	  CRect rIcon (mbd.iXicon, mbd.iYicon,
	  				mbd.iXicon + GetSystemMetrics (SM_CXICON) + 1,
	  				mbd.iYicon + GetSystemMetrics (SM_CYICON) + 1);
		InvalidateRect (hDlg, &rIcon, FALSE);
	  }

	while (iRtn < 0)
		{
		MSG Msg;
		if (PeekMessage (&Msg, NULL, 0, 0, PM_NOREMOVE))
			{
			if (Msg.message == WM_QUIT)
				{
				iRtn = -1;
				break;
				}

			// process the message
			if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
				if ((hDlgBox == NULL) || (! IsDialogMessage (hDlgBox, &Msg)))
					{
					TranslateMessage (&Msg);
					DispatchMessage (&Msg);
					}
			}

		// call the idle processing
		if ((lpFn != NULL) && (iRtn < 0))
			iRtn = (*lpFn) ();
		}

	// enable parent
	if ((uStyle & (MB_SYSTEMMODAL | MB_TASKMODAL)) == 0)
		{
		HWND hParOn = hWndPar;
		while (hParOn != NULL)
			{
			if ((BOOL) GetProp (hParOn, sApplProp) == TRUE)
				EnableWindow (hParOn, TRUE);
			RemoveProp (hParOn, sApplProp);
			hParOn = GetParent (hParOn);
			}
		}

	if (uStyle & MB_TASKMODAL)
		EnableAllWindows (hDlg, TRUE);

	DestroyWindow (hDlg);
	return (iRtn);
}

BOOL CALLBACK MsgBoxDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
MBDATA * pMbd;

	switch (uMsg)
		{
		//  center in parent but on screen if hWndPar != NULL
		case WM_INITDIALOG : {
			CRect rWin;
			GetClientRect (hDlg, &rWin);
			ASSERT ((rWin.left == 0) && (rWin.top == 0));
			HWND hWndPar = GetParent (hDlg);

			if (hWndPar == NULL)
				rWin += CPoint ((GetSystemMetrics (SM_CXSCREEN) - rWin.Width ()) / 2,
										(GetSystemMetrics (SM_CYSCREEN) - rWin.Height ()) / 2);
			else
			  {
			  CRect rPar;
				GetWindowRect (hWndPar, &rPar);
				rWin += CPoint (rPar.left + (rPar.Width () - rWin.Width ()) / 2,
										rPar.top + (rPar.Height () - rWin.Height ()) / 2);
				if (rWin.left < 0)
					rWin += CPoint (- rWin.left, 0);
				else
					if (rWin.right > GetSystemMetrics (SM_CXSCREEN))
						rWin -= CPoint (rWin.right - GetSystemMetrics (SM_CXSCREEN), 0);
				if (rWin.top < 0)
					rWin += CPoint (0, - rWin.top);
				else
					if (rWin.bottom > GetSystemMetrics (SM_CYSCREEN))
						rWin -= CPoint (0, rWin.bottom - GetSystemMetrics (SM_CYSCREEN));
			  }

			SetWindowPos (hDlg, NULL, rWin.left, rWin.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			} // NO BREAK - falls through

		case WM_ACTIVATE :
		case WM_ACTIVATEAPP :
			hDlgBox = hDlg;
			return (TRUE);
		case WM_DESTROY :
			hDlgBox = NULL;
			break;

		case WM_PAINT :
			pMbd = (MBDATA *) GetWindowLong (hDlg, DWL_USER);
			if ((pMbd == NULL) || (pMbd->hIcon == NULL))
				break;
			PAINTSTRUCT ps;
			BeginPaint (hDlg, &ps);
			DrawIcon (ps.hdc, pMbd->iXicon, pMbd->iYicon, pMbd->hIcon);
			EndPaint (hDlg, &ps);
			return (TRUE);

		case WM_COMMAND :
			if ((LOWORD (lParam) == 0) || (HIWORD (lParam) != BN_CLICKED))
				break;
			if ((pMbd = (MBDATA *) GetWindowLong (hDlg, DWL_USER)) == NULL)
				break;

			// check for help
			if (wParam == pMbd->wHelpBtn)
			  {
				const char *pHelpFile;
				char sName[130];
				CWinApp *pApp = AfxGetApp ();
				if (pApp != NULL)
					pHelpFile =	pApp->m_pszHelpFilePath;
				else
				  {
				  GetModuleFileName (AfxGetInstanceHandle (), sName, 128);
					int iLen = strlen (sName);
					if (iLen < 3)
						break;
					sName[iLen-3] = 0;
					strcat (sName, "HLP");
					pHelpFile = sName;
				  }

				if (pMbd->lHelp == 0)
					WinHelp (hDlg, pHelpFile,	HELP_CONTENTS, 0);
				else
					WinHelp (hDlg, pHelpFile,	HELP_CONTEXT, pMbd->lHelp);
			  return (TRUE);
			  }

			*(pMbd->piRtn) = wParam;
			return (TRUE);
		}

	return (FALSE);
}

