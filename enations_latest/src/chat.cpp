//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// chat.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "chat.h"
#include "player.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgChatAll dialog


CDlgChatAll::CDlgChatAll(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgChatAll::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgChatAll)
	m_strRcv = _T("");
	m_strSend = _T("");
	//}}AFX_DATA_INIT
}


void CDlgChatAll::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgChatAll)
	DDX_Control(pDX, IDC_CHAT_SEND, m_edtSend);
	DDX_Control(pDX, IDC_CHAT_RCV, m_edtRcv);
	DDX_Text(pDX, IDC_CHAT_RCV, m_strRcv);
	DDX_Text(pDX, IDC_CHAT_SEND, m_strSend);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgChatAll, CDialog)
	//{{AFX_MSG_MAP(CDlgChatAll)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDOK, OnReturn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgChatAll message handlers

void CDlgChatAll::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{

	// make it readable
	if (lpMMI->ptMinTrackSize.x < theApp.TextWid () * 20)
		lpMMI->ptMinTrackSize.x = theApp.TextWid () * 20;
	if (lpMMI->ptMinTrackSize.y < theApp.TextHt () * 6)
		lpMMI->ptMinTrackSize.y = theApp.TextHt () * 6;
	
	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CDlgChatAll::OnSize(UINT nType, int cx, int cy) 
{

	CDialog::OnSize(nType, cx, cy);
	
	CRect rect;
	GetClientRect (&rect);

	if (m_edtSend.m_hWnd != NULL)
		m_edtSend.SetWindowPos (NULL, 0, rect.bottom - theApp.TextHt () * 2, 
											rect.Width (), theApp.TextHt () * 2, SWP_NOZORDER);

	if (m_edtRcv.m_hWnd != NULL)
		m_edtRcv.SetWindowPos (NULL, 0, 0, rect.Width (), 
						rect.Height () - theApp.TextHt () * 2 - 6, SWP_NOZORDER | SWP_NOMOVE);
}

void CDlgChatAll::NewMessage (const char *pMsg)
{

	UpdateData (TRUE);
	m_strRcv += pMsg;
	UpdateData (FALSE);
	int iPos = m_strRcv.GetLength ();
	m_edtRcv.SetSel ( iPos, iPos, FALSE );
}

void CDlgChatAll::OnReturn()
{

	UpdateData (TRUE);

	CString sMsg;
	if ( theGame.HaveHP () )
		sMsg = theGame.GetMe()->GetName ();
	else
		sMsg.LoadString ( IDS_SERVER );
	sMsg += ": " + m_strSend + "\r\n";

	CNetChat * pMsg = CNetChat::Alloc (theGame.GetMe (), sMsg);
	theGame.PostToAll (pMsg, pMsg->m_iLen, FALSE);
	delete [] ((char *) pMsg);

	m_strSend = "";
	UpdateData (FALSE);
}

BOOL CDlgChatAll::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	// set location
	SetWindowPos (NULL, 0, theApp.m_iScrnY/2, theApp.m_iScrnX/3, theApp.m_iScrnY/3, SWP_NOZORDER);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDlgChatAll::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	
	return CDialog::OnCommand(wParam, lParam);
}

BOOL CDlgChatAll::PreTranslateMessage(MSG* pMsg) 
{
	
	// look for CTRL-P
	if ((WM_KEYFIRST <= pMsg->message) && (pMsg->message <= WM_KEYLAST))
		if (theGame.GetState () == CGame::play)
			if ( TranslateAccelerator ( m_hWnd, theApp.GetAccel (), pMsg ) )
				return 0;

	return CDialog::PreTranslateMessage(pMsg);
}
