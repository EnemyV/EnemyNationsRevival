// comstatd.cpp : implementation file
//
#ifndef NOMFC
#include "afxwin.h"
#include "afxdlgs.h"
#endif

#include "stdafx.h"
#include <stdio.h>
#include "vdmplay.h"
#include "resource.h"
#include "datalog.h"
#include "comstatd.h"
#include "help.h"

#ifndef NOMFC
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#else
extern HINSTANCE vphInst;
#endif

#if !defined(WIN32) && !defined(EXPORT)
#define EXPORT _export
#else
#define EXPORT
#endif


/////////////////////////////////////////////////////////////////////////////
// CComStatDlg dialog


CComStatDlg::CComStatDlg(LPCSTR port, CDataLogger* log, CWnd* pParent /*=NULL*/)
	: CDialog(CComStatDlg::IDD, pParent)
{
#ifndef NOMFC
	//{{AFX_DATA_INIT(CComStatDlg)
	m_commStat = _T("");
	//}}AFX_DATA_INIT

#endif
	m_port = port;
	m_log = log;

}


void CComStatDlg::DoDataExchange(CDataExchange* pDX)
{
#ifndef NOMFC
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComStatDlg)
	DDX_Control(pDX, IDC_COMM_TRACE, m_commTrace);
	DDX_Text(pDX, IDC_COMM_STATUS, m_commStat);
	//}}AFX_DATA_MAP
#endif
}

#ifndef NOMFC
BEGIN_MESSAGE_MAP(CComStatDlg, CDialog)
	//{{AFX_MSG_MAP(CComStatDlg)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif


/////////////////////////////////////////////////////////////////////////////
// CComStatDlg message handlers

void CComStatDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
 	CDialog::OnCancel();

	if (m_log)
		m_log->OnCancel();
	
}


void CComStatDlg::ShowStatus(UINT id)
{
#ifdef NOMFC
	char fmt[128];
	char buf[128];
	

	LoadString(vphInst, id, fmt, sizeof(fmt));
	wsprintf(buf, fmt, (LPCSTR) m_number);
	SetDlgItemText(m_hWnd, IDC_COMM_STATUS, buf);
#else
	m_commStat.LoadString(id);
	UpdateData(FALSE);
#endif

}

void CComStatDlg::ShowNumber(LPCSTR p)
{
#ifndef NOMFC
	m_commStat = p;
	UpdateData(FALSE);
#else
	m_number = p;
#endif
}



void CComStatDlg::ShowTrace(LPCSTR p, UINT size)
{
#ifdef USE_VPMEM
	char* tmp = (char*) vpAllocMem(size+1);
#else
	char* tmp = (char*) malloc(size+1);
#endif
	
	memcpy(tmp, p, size);
	tmp[size] = 0;
#ifndef NOMFC
	UINT i = m_commTrace.AddString(tmp);
	m_commTrace.SetTopIndex(i);
#else
	HWND hWnd = GetDlgItem(m_hWnd, IDC_COMM_TRACE);
	UINT i = SendMessage(hWnd, LB_ADDSTRING, 0, (LPARAM) (LPCSTR) tmp);
	SendMessage(hWnd, LB_SETTOPINDEX, (WPARAM) i, 0);
#endif
#ifdef USE_VPMEM
	vpFreeMem(tmp, size+1);
#else
	free(tmp);
#endif
}


void CComStatDlg::OnHelp() 
{
	::WinHelp (m_hWnd, "vdmplay.hlp", HELP_CONTEXT, HLP_COMM_STAT);
	// TODO: Add your control notification handler code here
	
}

BOOL CComStatDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

#ifdef NOMFC
	char sTmp [42], sTitle [62];
	LoadString (vphInst, IDS_COMM_STAT_TITLE, sTmp, 40);
	sprintf (sTitle, sTmp, (LPCSTR) m_port);
	SetWindowText (m_hWnd, sTitle);
#endif	

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



#ifdef NOMFC

BOOL CALLBACK EXPORT MyDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
		case WM_INITDIALOG :
		{

			CDialog* pDlg = (CDialog*) lParam;

			
			SetWindowLong (hDlg, DWL_USER, (LONG) lParam);
			pDlg->m_hWnd = hDlg;

			return pDlg->OnInitDialog();
		 }
		case WM_COMMAND :
		{
			CDialog* pDl = (CDialog*) GetWindowLong(hDlg, DWL_USER);
			if (HIWORD (wParam) == BN_CLICKED)
			{
				switch (LOWORD (wParam))
				{
			 
					case IDCANCEL : 
						pDl->OnCancel ();
						return (TRUE); 
			
					case IDX_HELP :
						pDl->OnHelp();
						return TRUE;
					case IDOK:
						pDl->OnOK();
						return TRUE;
				}
			}
		}
	}


	return (FALSE);
}




BOOL CDialog::Create(UINT id, CWnd* pWnd)
{

	m_hWnd = CreateDialogParam (vphInst, MAKEINTRESOURCE (id), NULL, (DLGPROC) MyDlgProc, (LPARAM) this);

	ShowWindow (m_hWnd, SW_SHOW);
	UpdateWindow (m_hWnd);
	return m_hWnd != NULL;

}


void CDialog::OnCancel()
{
	DestroyWindow();
	m_hWnd = NULL;
}

void CDialog::OnOK()
{
	DestroyWindow();
	m_hWnd = NULL;
}

#endif

