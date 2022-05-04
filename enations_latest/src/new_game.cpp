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
#include "player.h"
#include "error.h"
#include "bitmaps.h"
#include "help.h"
#include "sfx.h"

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
static CStatInst si;
static CStatData sd;

	switch (uMsg)
		{
		case WM_CREATE : {
			CMmio *pMmio = theDataFile.OpenAsMMIO ("misc", "MISC" );
			pMmio->DescendRiff ('M', 'I', 'S', 'C');

			// get the proper bitmap
			pMmio->DescendList  ( 'C', 'R', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1] );

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
			thePal.EndPaint (dc.m_hDC);
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

		TRAP (pDlg->m_hWnd == NULL);		// got a GPF on delete pDlg
		pDlg->DestroyWindow ();
		}
}

void CCreateBase::ShowDlgStatus ()
{ 

	if (m_pdlgStatus == NULL)
		m_pdlgStatus = new CDlgCreateStatus (&theApp.m_wndMain);

	if (m_pdlgStatus->m_hWnd == NULL)
		m_pdlgStatus->Create ();

	m_pdlgStatus->EnableWindow ( TRUE );
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

	CCreateBase::CloseAll ();
}

#ifdef _DEBUG
void CMultiBase::AssertValid() const
{

	CCreateBase::AssertValid ();

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

	m_sVer.LoadString ( IDS_EN_VER );
	m_sVer += VER_STRING;
	m_sVpVer.LoadString ( IDS_VP_VER );
	long lVer = CNetApi::GetVersion ();
	m_sVpVer += IntToCString (HIBYTE (HIWORD (lVer))) + "." +
												IntToCString (LOBYTE (HIWORD (lVer))) + "." +
												IntToCString (LOWORD (lVer));

	m_bServer = FALSE;
	m_bTimer = FALSE;
	m_bAddrShowing = FALSE;
}


void CDlgPlayerList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPlayerList)
	DDX_Control(pDX, IDC_CREATE_DELETE_PLAYER, m_btnDelete);
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Control(pDX, IDC_CREATE_NUM_PLAYERS, m_btnNumPlayers);
	DDX_Control(pDX, IDC_CREATE_LIST, m_lstPlayers);
	DDX_Control(pDX, IDC_PLYR_DO_ADDR, m_btnAddr);
	DDX_Text(pDX, IDC_PLYR_ADDR, m_sAddr);
	DDX_Text(pDX, IDC_PLYR_IS_ADDR, m_sIsAddr);
	DDX_Text(pDX, IDC_PLYR_VP_VER, m_sVpVer);
	DDX_Text(pDX, IDC_PLYR_EN_VER, m_sVer);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPlayerList, CDialog)
	//{{AFX_MSG_MAP(CDlgPlayerList)
	ON_LBN_SELCHANGE(IDC_CREATE_LIST, OnSelchangeCreateList)
	ON_WM_DRAWITEM()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_MEASUREITEM()
	ON_BN_CLICKED(IDC_CREATE_DELETE_PLAYER, OnCreateDeletePlayer)
	ON_BN_CLICKED(IDC_PLYR_DO_ADDR, OnShowAddr)
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
	
	UpdateData (TRUE);

	CString sTemp;
	sTemp.LoadString ( IDS_YOUR_ADDR );
	m_sAddr = sTemp + theNet.GetAddress ();

	sTemp.LoadString ( IDS_IS_ADDR );
	m_sIsAddr = theNet.GetIServeAddress ();
	if ( m_sIsAddr.IsEmpty () )
		{
		m_bTimer = TRUE;
		SetTimer ( 109, 2000, NULL );
		m_sIsAddr.LoadString ( IDS_NONE );
		char sName [160];
		GetPrivateProfileString ( "TCP", "RegistrationAddress", "iserve.windward.net",
																					sName, 158, "vdmplay.ini" );
		csPrintf ( &m_sIsAddr, (const char *) sName );
		}
	m_sIsAddr = sTemp + m_sIsAddr;

	UpdateData (FALSE);

	CRect rect, rect2;
	GetWindowRect ( & rect );
	m_iWid = rect.Width ();
	m_iHtAddr = rect.Height ();
	GetDlgItem ( IDC_PLYR_ADDR )->GetWindowRect ( &rect2 );
	m_iHtNoAddr = rect2.top - rect.top;
	SetWindowPos ( NULL, 0, 0, m_iWid, m_iHtNoAddr, SWP_NOMOVE | SWP_NOZORDER );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPlayerList::OnShowAddr ()
{

	m_bAddrShowing = ! m_bAddrShowing;

	SetWindowPos ( NULL, 0, 0, m_iWid, m_bAddrShowing ? m_iHtAddr : m_iHtNoAddr,
											SWP_NOMOVE | SWP_NOZORDER );
	CString sText;
	sText.LoadString ( m_bAddrShowing ? IDS_ADDR_OFF : IDS_ADDR );
	SetDlgItemText ( IDC_PLYR_DO_ADDR, sText );
}

void CDlgPlayerList::OnTimer(UINT nIDEvent) 
{

	// see if we've got it
	CString sIserve = theNet.GetIServeAddress ();
	if ( sIserve.IsEmpty () )
		return;

	// we've got it - kill the timer
	m_bTimer = FALSE;
	KillTimer ( nIDEvent );

	UpdateData (TRUE);
	m_sIsAddr.LoadString ( IDS_IS_ADDR );
	m_sIsAddr += sIserve;
	UpdateData (FALSE);
}

void CDlgPlayerList::OnDestroy()
{

	if ( m_bTimer )
		{
		m_bTimer = FALSE;
		KillTimer ( 109 );
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

	// if joining we don't touch this button
	if ( (theApp.m_pCreateGame != NULL ) &&
						( (theApp.m_pCreateGame->m_iTyp == CCreateBase::join_net) ||
							(theApp.m_pCreateGame->m_iTyp == CCreateBase::load_join) ) )
		{
		OnSelchangeCreateList();
		return;
		}

	// if no us, can't do it
	if ( theGame.HaveHP () && ( ! theGame.GetMe () ) )
		{
		m_btnOk.EnableWindow ( FALSE );
		OnSelchangeCreateList();
		return;
		}

	BOOL bStart = ( (theGame.GetState () == CGame::open) ||
												(theGame.GetState () == CGame::wait_start) ) &&
												( (! theGame.HaveHP ()) ||
												(theGame.GetMe()->GetState () == CPlayer::ready) );

	// if loading a game all we care about is total
	if ( (theApp.m_pCreateGame != NULL ) &&
						(theApp.m_pCreateGame->m_iTyp == CCreateBase::load_multi) )
		if ( m_lstPlayers.GetCount () >= 2 )
			{
			m_btnOk.EnableWindow ( TRUE );
			OnSelchangeCreateList();
			return;
			}

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
				if ( ListBoxFindDataPtr (m_lstPlayers, pPlr) >= 0 )
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

	// let them know one added
	theMusicPlayer.PlayForegroundSound ( SOUNDS::GetID ( SOUNDS::player_join ), SFXPRIORITY::selected_pri );

	SetNumPlayers ();

//NO - processes messages which can kill this	UpdateWindow ();
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
		if ( m_pCb->m_iTyp == CCreateBase::load_multi )
			{
			if ( theGame.m_lstLoad.GetCount () > 0 )
				{
				TRAP ();
				if (AfxMessageBox (IDS_ASK_START, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
					return;
				}
			}
		else
			{
			POSITION pos;
			for (pos = theGame.GetAll().GetTailPosition(); pos != NULL; )
				{
				CPlayer *pPlr = theGame.GetAll().GetPrev (pos);
				ASSERT_VALID (pPlr);
				if (pPlr->GetState () != CPlayer::ready)
					{
					if (AfxMessageBox (IDS_ASK_START, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL) != IDYES)
						return;
					break;
					}
				}
			}

		if ( m_pCb->m_iTyp == CCreateBase::load_multi )
			{
			if (theGame.StartGame (FALSE) != IDOK)
				{
				TRAP ();
				theApp.CloseWorld ();
				theApp.CreateMain ();
				}
			}
		else
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
	
	if ( ODT_BUTTON == lpDIS->CtlType )	// GG
	{
		Default();
		return;
	}

	if ((int) lpDIS->itemID < 0)
		return;

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
// CDlgCreateStatus message handlers

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
	if ( (m_iPer >= iPer) && (iPer != 0) )
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
	if ( pWnd != NULL )
		{
		::SetWindowWord (pWnd->m_hWnd, 0, (WORD) iPer);
		pWnd->InvalidateRect (NULL, FALSE);
		pWnd->UpdateWindow ();
		}
	return;
}

