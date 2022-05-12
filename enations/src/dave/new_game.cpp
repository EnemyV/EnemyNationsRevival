//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// new_game.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "new_game.h"
#include "error.h"
#include "bitmaps.h"
#include "help.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



// find the index who's data ptr matches
int ListBoxFindDataPtr (CListBox const & lst, void const * pData)
{

	int iMax = lst.GetCount ();
	for (int iInd=0; iInd<iMax; iInd++)
		if (lst.GetItemDataPtr (iInd) == pData)
			return (iInd);

	return (-1);
}	


LRESULT CALLBACK PerBarProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
// scratch DIB for painting (this works well because we only use this in 1 place)
static CDIB *pdibBar = NULL;
static CStatInst si;
static CStatData sd;

	switch (uMsg)
		{
		case WM_CREATE : {
			if ( pdibBar != NULL )
				break;
			CMmio *pMmio = theDataFile.OpenAsMMIO ("misc");
			pMmio->DescendRiff ('M', 'I', 'S', 'C');

			// get the proper bitmap
			char szBPS[3];
			if ( 8 == ptrthebltformat->GetBitsPerPixel ())
				strcpy ( szBPS, "08" );
			else
				strcpy ( szBPS, "24" );
			pMmio->DescendList  ( 'C', 'R', szBPS[0], szBPS[1] );

			// load it
			sd.Init ( pMmio );
			si.m_pStatData = &sd;
			si.SetBack ( theBitmaps.GetByIndex ( DIB_GOLD ), CPoint (0, 0) );

			delete pMmio;
			break; }

		case WM_ERASEBKGND :
			return (1);
			
		case WM_PAINT : {
			// check pDib
			CRect rect;
			CWnd *pWnd = CWnd::FromHandle (hWnd);
			pWnd->GetClientRect ( &rect );
			si.SetSize ( rect );

			// set the percentage
			si.SetPer ( ::GetWindowWord (hWnd, 0) );

			// draw the frame
			CPaintDC dc (pWnd);
			thePal.Paint (dc.m_hDC);
			si.DrawIcon ( &dc );
			return (0); }
		}

	return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}


/////////////////////////////////////////////////////////////////////////////
// CCreateBase

CCreateBase::CCreateBase (int iTyp) 
{ 

	ASSERT ((0 <= iTyp) && (iTyp < num_types)); 
	m_iTyp = iTyp; 
	m_iNet = -1;
	m_iJoinUntil = 0;
	m_iNumPlayers = 0;
	m_pdlgStatus = NULL;
	m_pAdvNet = NULL;

	memset (&m_ID, 0, sizeof (m_ID));

	// read in the races
	ptheRaces = CRaceDef::Init (ptheRaces);
}

void CCreateBase::CloseAll ()
{

	ASSERT_VALID (this);

	ClosePick ();

	if (m_pdlgStatus != NULL)
		{
		CDlgCreateStatus *	pDlg = m_pdlgStatus;
		m_pdlgStatus = NULL;

		if (pDlg->m_hWnd != NULL)
			pDlg->DestroyWindow ();
		else
			delete pDlg;
		}
}

void CCreateBase::ShowDlgStatus ()
{ 

	if (m_pdlgStatus == NULL)
		m_pdlgStatus = new CDlgCreateStatus (&theApp.m_wndMain);

	if (m_pdlgStatus->m_hWnd == NULL)
		m_pdlgStatus->Create ();

	m_pdlgStatus->ShowWindow (SW_SHOW);
}

void CCreateBase::CreateDlgStatus ()
{

	if (m_pdlgStatus == NULL)
		m_pdlgStatus = new CDlgCreateStatus (&theApp.m_wndMain);

	if (m_pdlgStatus->m_hWnd == NULL)
		m_pdlgStatus->Create ();
}

void CCreateBase::HideDlgStatus ()
{ 

	if (m_pdlgStatus == NULL)
		return;

	if (m_pdlgStatus->m_hWnd == NULL)
		return;

	m_pdlgStatus->ShowWindow (SW_HIDE);
}

void CCreateBase::ToWorld ()
{

	ASSERT_VALID (this);

	if (m_pdlgStatus == NULL)
		m_pdlgStatus = new CDlgCreateStatus (&(theApp.m_wndMain));
	if (m_pdlgStatus->m_hWnd == NULL)
		m_pdlgStatus->Create ();

	m_pdlgStatus->ShowWindow (SW_SHOW);
	ClosePick ();
}

#ifdef _DEBUG
void CCreateBase::AssertValid() const
{

	CObject::AssertValid ();

	ASSERT_VALID_OR_NULL (m_pdlgStatus);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CMultiBase

void CMultiBase::CreatePlyrList (CCreateBase * pCb)
{

	m_wndPlyrList.Create (pCb, IDD_PLAYER_LIST, &theApp.m_wndMain);
}

void CMultiBase::CloseAll ()
{

	if (m_wndPlyrList.m_hWnd != NULL)
		m_wndPlyrList.DestroyWindow ();

	if (m_wndChat.m_hWnd != NULL)
		m_wndChat.DestroyWindow ();

	CCreateBase::CloseAll ();
}

#ifdef _DEBUG
void CMultiBase::AssertValid() const
{

	CCreateBase::AssertValid ();

	ASSERT_VALID (&m_wndChat);
	ASSERT_VALID (&m_wndPlyrList);
};
#endif


/////////////////////////////////////////////////////////////////////////////
// CCreateNewBase

void CCreateNewBase::ClosePick ()
{

	if (m_dlgPickRace.m_hWnd != NULL)
		m_dlgPickRace.DestroyWindow ();
}


/////////////////////////////////////////////////////////////////////////////
// CCreateLoadBase

void CCreateLoadBase::ClosePick ()
{

	if (m_dlgPickPlayer.m_hWnd != NULL)
		m_dlgPickPlayer.DestroyWindow ();
}


/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerList dialog


CDlgPlayerList::CDlgPlayerList(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPlayerList::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgPlayerList)
	//}}AFX_DATA_INIT

	m_bServer = FALSE;
}


void CDlgPlayerList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPlayerList)
	DDX_Control(pDX, IDC_CREATE_DELETE_PLAYER, m_btnDelete);
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Control(pDX, IDC_CREATE_NUM_PLAYERS, m_btnNumPlayers);
	DDX_Control(pDX, IDC_CREATE_LIST, m_lstPlayers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPlayerList, CDialog)
	//{{AFX_MSG_MAP(CDlgPlayerList)
	ON_LBN_SELCHANGE(IDC_CREATE_LIST, OnSelchangeCreateList)
	ON_WM_DRAWITEM()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_NET_ADDRESS, OnNetAddress)
	ON_WM_MEASUREITEM()
	ON_BN_CLICKED(IDC_CREATE_DELETE_PLAYER, OnCreateDeletePlayer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerList message handlers

void CDlgPlayerList::Create (CCreateBase * pCb, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pCb = pCb;
	CDialog::Create (id, pPar);

	// BUGBUG doesn't take in InInit
	SetWindowPos (NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

BOOL CDlgPlayerList::OnInitDialog() 
{

	CDialog::OnInitDialog();

	UpdateBtns ();

	if (! m_bServer)
		{
		m_btnDelete.EnableWindow (FALSE);
		m_btnDelete.ShowWindow (SW_HIDE);
		m_btnOk.EnableWindow (FALSE);
		m_btnOk.ShowWindow (SW_HIDE);
		}
	else
		m_btnOk.EnableWindow (! theGame.HaveHP ());

	// put in upper left corner	
	SetWindowPos (NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPlayerList::OnDestroy()
{

	if ( m_dlgNetAddr.m_hWnd != NULL )
		{
		TRAP ();
		m_dlgNetAddr.EndDialog ( 0 );
		}
}

void CDlgPlayerList::OnSelchangeCreateList() 
{

	ASSERT_VALID (this);

	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		{
		m_btnDelete.EnableWindow (FALSE);
		return;
		}

	CPlayer *pPlr = (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd);
	ASSERT_VALID (pPlr);
	if (pPlr->IsMe ())
		{
		m_btnDelete.EnableWindow (FALSE);
		return;
		}

	m_btnDelete.EnableWindow (theGame.GetState () < CGame::AI_done);
}

void CDlgPlayerList::UpdateBtns ()
{

	BOOL bStart = ( (theGame.GetState () == CGame::open) ||
												(theGame.GetState () == CGame::wait_start) ) &&
												( (! theGame.HaveHP ()) ||
												(theGame.GetMe()->GetState () == CPlayer::ready) );
	// check for at least 2 players & at least 1 HP
	if ( bStart )
		{
		int iNum = 0;
		POSITION pos;
		for (pos = theGame.GetAll().GetTailPosition(); pos != NULL; )
			{
			CPlayer *pPlr = theGame.GetAll().GetPrev (pos);
			ASSERT_VALID (pPlr);
			if ( ( ! pPlr->IsAI ()) && (pPlr->GetState () == CPlayer::ready) )
				iNum++;
			}
		if ( theGame.GetAi().GetCount () > 0 )
			iNum++;
		if (iNum < 2)
			bStart = FALSE;
		}

	m_btnOk.EnableWindow ( bStart );

	OnSelchangeCreateList();
}

void CDlgPlayerList::OnCreateDeletePlayer() 
{
	
	ASSERT_VALID (this);

	// get the selected player
	int iInd = m_lstPlayers.GetCurSel ();
	ASSERT (iInd >= 0);
	CPlayer *pPlr = (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd);
	ASSERT_VALID (pPlr);

	// remove from the listbox
	m_lstPlayers.DeleteString (iInd);

	// if we haven't started just delete them
	if ((theGame.GetState () == CGame::open) ||
												(theGame.GetState () == CGame::wait_start))
		{
		TRAP ();
		theGame.DeletePlayer (pPlr);
		}
	// we turn them into AI players when we are done initializing
	else
	  {
		pPlr->SetState (CPlayer::replace);
		theNet.DeletePlayer (pPlr->GetNetNum ());
		pPlr->SetNetNum (0);
		pPlr->SetAI (TRUE);
		pPlr->SetLocal (TRUE);
	  }

	SetNumPlayers ();
}

void CDlgPlayerList::RemovePlayer (CPlayer *pPlr) 
{

	ASSERT_VALID (this);

	// can be called anytime by OnLeave
	if ((m_hWnd == NULL) || (m_lstPlayers.m_hWnd == NULL))
		return;

	// get the selected player
	int iInd = ListBoxFindDataPtr (m_lstPlayers, pPlr);
	ASSERT (iInd >= 0);
	if (iInd < 0)
		return;

	// remove from the list
	m_lstPlayers.DeleteString (iInd);
	SetNumPlayers ();
}

// set the number of players
void CDlgPlayerList::SetNumPlayers ()
{

	ASSERT_VALID (this);

	CString sText;
	sText.LoadString (IDS_CREATE_NUM_PLAYERS);
	CString sNum = IntToCString (m_lstPlayers.GetCount ());
	csPrintf (&sText, (const char *) sNum);
	m_btnNumPlayers.SetWindowText (sText);

	UpdateBtns ();
}

void CDlgPlayerList::AddPlayer (CPlayer * pPlr)
{

	ASSERT_VALID (this);

	m_lstPlayers.AddString ((const char *) pPlr);

	SetNumPlayers ();
}

void CDlgPlayerList::OnNetAddress() 
{
	
	if ( m_dlgNetAddr.m_hWnd != NULL )
		{
		TRAP ();
		m_dlgNetAddr.SetActiveWindow ();
		return;
		}

	m_dlgNetAddr.DoModal ();
}

void CDlgPlayerList::UpdatePlyrStatus (CPlayer * pPlyr, int iStatus)
{

	ASSERT_VALID (this);

	if ( (m_hWnd == NULL) || (m_lstPlayers.m_hWnd == NULL) )
		return;

	// find the player
	int iInd = ListBoxFindDataPtr (m_lstPlayers, pPlyr);
	if (iInd >= 0)
		{
		pPlyr->m_iPerInit = iStatus;
		m_lstPlayers.InvalidateRect (NULL);
		m_lstPlayers.UpdateWindow ();
		}
}	

void CDlgPlayerList::OnOK() 
{
	
	try
		{
		theGame.IncTry ();
		// if some not ready - check
		POSITION pos;
		for (pos = theGame.GetAll().GetTailPosition(); pos != NULL; )
			{
			CPlayer *pPlr = theGame.GetAll().GetPrev (pos);
			ASSERT_VALID (pPlr);
			if (pPlr->GetState () != CPlayer::ready)
				{
				if (AfxMessageBox (IDS_ASK_START, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
					return;
				goto DoIt;
				}
			}

DoIt:
		theApp.StartCreateWorld ();
		theGame.DecTry ();
		}

	catch (int iNum)
		{
		TRAP ();
		CatchNum (iNum);
		theApp.CloseWorld ();
		return;
		}
	catch ( SE_Exception e )
		{
		TRAP ();
		CatchSE ( e );
		theApp.CloseWorld ();
		return;
		}
	catch (...)
		{
		TRAP ();
		CatchOther ();
		theApp.CloseWorld ();
		return;
		}
}

void CDlgPlayerList::OnDrawItem(int, LPDRAWITEMSTRUCT lpDIS)
{
	
	if ((int) lpDIS->itemID < 0)
		return;

	if ( ODT_BUTTON == lpDIS->CtlType )	// GG
	{
		Default();
		return;
	}

	// make a CDC
	CDC *pDc = CDC::FromHandle (lpDIS->hDC);
	if (pDc == NULL)
		return;
	TEXTMETRIC tm;
	pDc->GetTextMetrics (&tm);

	CPlayer * pPlyr = (CPlayer *) lpDIS->itemData;
	ASSERT_VALID (pPlyr);

	pDc->SetBkMode (OPAQUE);
	if (lpDIS->itemState & ODS_SELECTED)
		{
		pDc->SetBkColor (GetSysColor (COLOR_HIGHLIGHT));
		pDc->SetTextColor (GetSysColor (COLOR_HIGHLIGHTTEXT));
		}
	else
		{
		pDc->SetBkColor (GetSysColor (COLOR_WINDOW));
		pDc->SetTextColor (GetSysColor (COLOR_WINDOWTEXT));
		}

	CRect rect (lpDIS->rcItem);
	rect.right = rect.left + tm.tmAveCharWidth * 4;

	CString sText;
	if (pPlyr == theGame._GetMe ())
		sText = "»";
	else
		sText = " ";
	pDc->ExtTextOut (rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, 
															&rect, sText, sText.GetLength (), NULL);

	rect.left = rect.right;
	rect.right = lpDIS->rcItem.left + (lpDIS->rcItem.right - lpDIS->rcItem.left) / 2;
	pDc->ExtTextOut (rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, 
						&rect, pPlyr->GetName(), strlen (pPlyr->GetName()), NULL);

	sText = " ";
	if (pPlyr->GetState () == CPlayer::load_pick)
		{
		CString sStatus;
		sStatus.LoadString (IDS_PICKING_PLAYER);
		sText += sStatus;
		}
	else
		if (pPlyr->m_iPerInit < 0)
			{
			CString sStatus;
			sStatus.LoadString (IDS_PICKING_RACE);
			sText += sStatus;
			}
		else
			if (pPlyr->m_iPerInit == 0)
				{
				CString sRes;
				sRes.LoadString (IDS_READY_TO_GO);
				sText += sRes;
				}
			else
				if (pPlyr->m_iPerInit < 100)
					sText += IntToCString (pPlyr->m_iPerInit) + "%";
				else
					{
					CString sRes;
					sRes.LoadString (IDS_READY_TO_GO);
					sText += sRes;
					}

	rect.left = rect.right;
	rect.right = lpDIS->rcItem.right;
	pDc->ExtTextOut (rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, 
															&rect, sText, sText.GetLength (), NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgNetAddr dialog


CDlgNetAddr::CDlgNetAddr(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgNetAddr::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgNetAddr)
	m_strAddr = _T("");
	//}}AFX_DATA_INIT
}


void CDlgNetAddr::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgNetAddr)
	DDX_Text(pDX, IDC_NET_ADDR, m_strAddr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgNetAddr, CDialog)
	//{{AFX_MSG_MAP(CDlgNetAddr)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgNetAddr message handlers

BOOL CDlgNetAddr::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// the net address
	m_strAddr = theNet.GetAddress ();
	UpdateData (FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateStatus dialog

CDlgCreateStatus::CDlgCreateStatus (CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateStatus::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreateStatus)
	//}}AFX_DATA_INIT
}

void CDlgCreateStatus::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCreateStatus)
	DDX_Control(pDX, IDC_CREATE_WAIT_TEXT, m_txtMsg);
	DDX_Control(pDX, IDCANCEL, m_btnCancel);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgCreateStatus, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateStatus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgCreateStatus message handlers

BOOL CDlgCreateStatus::OnInitDialog()
{

	CDialog::OnInitDialog();

	CenterWindow (theApp.m_pMainWnd);

	CWnd * pWnd = GetDlgItem ( IDC_PER_BAR );
	CRect rect;
	pWnd->GetClientRect ( &rect );
	CStatData * pSd = theIcons.GetByIndex ( ICON_CLOCK );
	pWnd->SetWindowPos (NULL, 0, 0, rect.Width (), pSd->m_cyBack, SWP_NOMOVE | SWP_NOZORDER);

	m_iPer = 0;
	m_Quit = FALSE;

	ASSERT_VALID (this);

	// set the focus to the text so a <CR> doesn't cancel
	GetDlgItem (IDC_PER_BAR)->SetFocus ();
	return (FALSE);
}

void CDlgCreateStatus::OnOK()
{

	ASSERT_VALID (this);

	MessageBeep (0);
}

void CDlgCreateStatus::OnCancel()
{

	ASSERT_VALID (this);

	if (theGame._GetMe () == NULL)
		{
		if (AfxMessageBox (IDS_SERVER_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
			return;
		}
	else
		if (theGame.AmServer ())
			{
			if ( theGame.IsNetGame () )
				{
				if (AfxMessageBox (IDS_SERVER_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
					return;
				}
			else
				if (AfxMessageBox (IDS_SINGLE_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
					return;
			}
		else
			if (AfxMessageBox (IDS_CLIENT_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
				return;

	m_Quit = TRUE;

	// if we are hanging out we need to kill now
	if (theGame.GetTry ())
		{
		m_Quit = TRUE;
		CDialog::OnCancel ();
		}
	else
		theApp.CloseWorld ();
}

void CDlgCreateStatus::SetStatus ()
{

	SetPer (0, FALSE);
	SetMsg (IDS_CREATE_WORLD);
}

void CDlgCreateStatus::SetMsg (int idRes)
{

	ASSERT_VALID (this);

	CString sText;
	sText.LoadString (idRes);
	SetMsg (sText);
}

void CDlgCreateStatus::SetMsg (char const * pText)
{

	ASSERT_VALID (this);

	m_txtMsg.SetWindowText (pText);
	UpdateWindow ();
	theApp.BaseYield ();
}

void CDlgCreateStatus::SetPer (int iPer, BOOL bYield)
{

	ASSERT_VALID (this);
	ASSERT ((0 <= iPer) && (iPer <= 100));

	if (bYield)
		{
		if (m_Quit)
			ThrowError (ERR_TLP_QUIT);

		// yield for messages
		theApp.BaseYield ();

		// need to handle % changes we got
		theGame.ProcessAllMessages ();

		if (m_Quit)
			ThrowError (ERR_TLP_QUIT);
		}

	iPer = iPer < -1 ? -1 : (iPer > 100 ? 100 : iPer);
	if (m_iPer >= iPer)
		return;

	// tell everyone our new status
	if (theGame.HaveHP ())
		{
		CNetPlyrStatus msg (theGame.GetMe()->GetNetNum (), iPer);
		theGame.PostToAll (&msg, sizeof (msg), FALSE);
		}

	// this is 0 .. 100
	m_iPer = iPer == -1 ? 0 : iPer;

	CWnd *pWnd = GetDlgItem (IDC_PER_BAR);
	::SetWindowWord (pWnd->m_hWnd, 0, (WORD) iPer);
	pWnd->InvalidateRect (NULL, FALSE);
	pWnd->UpdateWindow ();
	return;
}

