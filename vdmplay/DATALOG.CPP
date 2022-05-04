
#ifndef NOMFC
#include "afxwin.h"
#include "afxdlgs.h"
#endif




#include "stdafx.h"
#include "stdio.h"

#include "resource.h"
#include "help.h"
#include "datalog.h"
#include "comstatd.h"



#ifdef XXX

extern HINSTANCE vphInst;



CDialogLogger::CDialogLogger ()
{

	m_hDlg = NULL;
}

CDialogLogger::~CDialogLogger ()
{

	if (m_hDlg != NULL)
		CloseWindow (m_hDlg);
}

typedef struct tagDLGDATA {
	char const * 		pPort;
	CDialogLogger *	pDl;
} DLGDATA;

#if !defined(WIN32) && !defined(EXPORT)
#define EXPORT _export
#else
#define EXPORT
#endif

BOOL CALLBACK EXPORT DlgProcLog (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			DLGDATA * pDd = (DLGDATA *) lParam;
			SetWindowLong (hDlg, DWL_USER, (LONG) pDd->pDl);

			char sTmp [42], sTitle [62];
			LoadString (vphInst, IDS_COMM_STAT_TITLE, sTmp, 40);
			sprintf (sTitle, sTmp, pDd->pPort);
			SetWindowText (hDlg, sTitle);
			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {
					case IDCANCEL : {
						CDialogLogger * pDl = (CDialogLogger *) GetWindowLong (hDlg, DWL_USER);
						pDl->OnCancel ();
						return (TRUE); }

					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_COMM_STAT);
						return (TRUE);
					break;
					}
	  }

	return (FALSE);
}

void CDialogLogger::Start (char const * pPort)
{

	DLGDATA dd;
	dd.pPort = pPort;
	dd.pDl = this;

	m_hDlg = CreateDialogParam (vphInst, MAKEINTRESOURCE (IDD_COMM_STAT), NULL, (DLGPROC) DlgProcLog, (LPARAM) &dd);

	ShowWindow (m_hDlg, SW_SHOW);
	UpdateWindow (m_hDlg);
}

void CDialogLogger::Stop()
{

	if (m_hDlg != NULL)
		{
		CloseWindow (m_hDlg);
		m_hDlg = NULL;
		}
}

void CDialogLogger::Status (int iStat)
{
	char sStat [42];
	LoadString (vphInst, iStat, sStat, 40);
	SetDlgItemText (m_hDlg, IDC_COMM_STATUS, sStat);
}

void CDialogLogger::Number (char const *psNum)
{

	char sFmt [42];
	LoadString (vphInst, IDS_COMM_STAT_DIAL, sFmt, 40);

	char * psStat = (char *) malloc (strlen (sFmt) + strlen (psNum) + 6);
	if (psStat == NULL)
		return;

	sprintf (psStat, sFmt, psNum);
	SetDlgItemText (m_hDlg, IDC_COMM_STATUS, psStat);

	free (psStat);
}

void CDialogLogger::OutgoingData(LPCSTR data, unsigned size)
{
#ifdef WIN32
	if (size <= 0)
		return;
    
    HWND hItem = GetDlgItem (m_hDlg, IDC_COMM_DATA);
	SendMessage (hItem, EM_SETSEL, (WPARAM) 0, MAKELPARAM(0, -1));

	// count \r's
	char * psBuf = (char *) data;
	int iLen = size;
	int iNum = 0;
	while (iLen--)
		if (*psBuf++ == '\r')
			iNum++;

	char * psData = (char *) malloc (size + 2 + iNum);
	if (psData == NULL)
		return;

	// copy across & add a \n to \r if it doesn't have one
	psBuf = psData;
	while (size--)
		{
		*psBuf++ = *data++;
		if ((*(data-1) == '\r') && ((size <= 0) || (*data != '\n')))
			*psBuf++ = '\n';
		}
	*psBuf = 0;

	SendMessage (GetDlgItem (m_hDlg, IDC_COMM_DATA), EM_REPLACESEL, FALSE, (LPARAM) psData);
	free (psData);  
#endif
}                 


#else

CDialogLogger::CDialogLogger ()
{

	m_dlg = NULL;
}

CDialogLogger::~CDialogLogger ()
{
	delete m_dlg;
}




void CDialogLogger::Status (int iStat)
{
	if (m_dlg)
		m_dlg->ShowStatus(iStat);
}


void CDialogLogger::Stop()
{

	if (m_dlg)
	{
		m_dlg->DestroyWindow();
		delete m_dlg;
		m_dlg = NULL;
	}
}

void CDialogLogger::Start (char const * pPort)
{

	if (!m_dlg)
	{
		m_dlg = new CComStatDlg(pPort, this);
		m_dlg->Create(m_dlg->IDD);
	}
}


void CDialogLogger::Number (char const *psNum)
{
	m_dlg->ShowNumber(psNum);
}

void CDialogLogger::OutgoingData(LPCSTR data, unsigned size)
{
	m_dlg->ShowTrace(data, size);
}

#endif
