//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// main.cpp : implementation file
//

#include "stdafx.h"
#include "toolbar.h"
#include "bitmaps.h"
#include "lastplnt.h"
#include "relation.h"
#include "event.h"
#include "area.h"

#include "building.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


CString CWndBar::m_sChat1;
CString CWndBar::m_sChat2;
CString CWndBar::m_sScience;
CString CWndBar::m_sRelations;


/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat window

CWndStatLine::CWndStatLine ()
{

	m_fnStatus = NULL;
	m_pFnData = NULL;
}

void CWndStatLine::SetText (char const * pText, CStatInst::IMPORTANCE iImp)
{

	// CWndStatBar::SetText won't Invalidate if it's identical to the old value (usually NULL)
	if ( m_fnStatus != NULL )
		{
		InvalidateRect (NULL);

		m_fnStatus = NULL;
		m_pFnData = NULL;
		}

	CWndStatBar::SetText (pText, iImp);
}

void CWndStatLine::SetStatusFunc (FNSTATUSLINE fnStat, void * pData)
{

	// if the same - exit
	if ((fnStat == m_fnStatus) && (pData == m_pFnData))
		return;

	if ((fnStat == NULL) || (pData == NULL))
		{
		m_fnStatus = NULL;
		m_pFnData = NULL;
		InvalidateRect (NULL);
		return;
		}

	// turn off text
	CWndStatBar::SetText (NULL);

	m_fnStatus = fnStat;
	m_pFnData = pData;
	InvalidateRect (NULL);
}

BEGIN_MESSAGE_MAP(CWndStatLine, CWndStatBar)
	//{{AFX_MSG_MAP(CWndStatLine)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWndStatLine::OnPaint()
{

	if (m_fnStatus == NULL)
		{
		CWndStatBar::OnPaint ();
		return;
		}

	CPaintDC dc (this);
	thePal.Paint (dc.m_hDC);

	CRect rect;
	GetClientRect (&rect);

	// get the offset of the background
	CPoint pt (0, 0);
	MapWindowPoints (GetParent (), &pt, 1);

	(* m_fnStatus) (m_pFnData, &dc, rect, theBitmaps.GetByIndex (DIB_TOOLBAR), pt);

	thePal.EndPaint (dc.m_hDC);
}


/////////////////////////////////////////////////////////////////////////////
// CWndBar

CWndBar::CWndBar ()
{

}

const int CWndBar::aID [NUM_BAR_BTNS] = {
				IDC_BAR_AREA,
				IDC_BAR_WORLD,
				IDC_BAR_CHAT,
				IDC_BAR_ADVISOR,
				IDC_BAR_VEHICLES,
				IDC_BAR_BUILDINGS,
				IDC_BAR_SCIENCE,
				IDC_BAR_FILE };
const int CWndBar::aBtn [NUM_BAR_BTNS] = {
				43,
				17,
				15,
				31,
				19,
				18,
				0,
				27 };
const int CWndBar::aHelp [NUM_BAR_BTNS] = {
				IDH_BAR_AREA,
				IDH_BAR_WORLD,
				IDH_BAR_CHAT,
				IDH_BAR_ADVISOR,
				IDH_BAR_VEHICLES,
				IDH_BAR_BUILDINGS,
				IDH_BAR_SCIENCE,
				IDH_BAR_FILE };

void fnMouseMove (CWnd * pWnd, UINT nFlags, CPoint point)
{

	theApp.m_wndBar.SetStatusText (1, "");
}

void CWndBar::Create ()
{

	// load the strings
	m_sChat1.LoadString (IDS_NO_CHAT1);
	m_sChat2.LoadString (IDS_NO_CHAT2);
	m_sScience.LoadString (IDS_NO_SCIENCE);
	m_sRelations.LoadString (IDS_NO_EMBASSY);

	// we go at the bottom of the main window (in case the Windows toolbar pushes it up/over)
	CRect rect;
	theApp.m_pMainWnd->GetClientRect (&rect);

	theApp.m_iRow3 = rect.Height () - TOOLBAR_HT;

	if (CreateEx (WS_EX_TOPMOST, theApp.m_sWndCls, "", WS_POPUP, 0, theApp.m_iRow3,
											rect.Width (), TOOLBAR_HT, theApp.m_pMainWnd->m_hWnd, NULL, NULL) == 0)
		throw (ERR_RES_CREATE_WND);

	// if not net play - disable the chat window
	if ( ! theGame.IsNetGame () )
		EnableButton (IDC_BAR_CHAT, FALSE);

	CWndBase::SetFnMouseMove (fnMouseMove);
}

int CWndBar::OnCreate (LPCREATESTRUCT lpCS) 
{

	if (CWndAnim::OnCreate (lpCS) == -1)
		return -1;

	// create the buttons
	CRect rect (BAR_BTN_X_SKIP, BAR_BTN_Y_START,
						BAR_BTN_X_SKIP + theBmBtnData.Width (), BAR_BTN_Y_START + theBmBtnData.Height ());

	CBmButton * pBtn = m_BmBtns;
	for (int iOn=0; iOn<NUM_BAR_BTNS; iOn++, pBtn++)
		{
		pBtn->Create (aBtn[iOn], aHelp[iOn], &theBmBtnData, rect, theBitmaps.GetByIndex (DIB_TOOLBAR), this, aID[iOn]);
		rect.OffsetRect (theBmBtnData.Width () + BAR_BTN_X_SKIP, 0);
		}
	rect.left += BAR_BTN_X_SKIP;

	// status bars
	// time goes on the right
	CStatData * pSb = theIcons.GetByIndex (ICON_CLOCK);
	CRect rTime;
	GetClientRect (&rTime);
	rTime.right -= BAR_BTN_X_SKIP;
	rTime.top = rect.top;
	rTime.bottom = rect.bottom;

	// we size this based on fitting the text in
	CClientDC dc ( this );
	CFont * pOld = NULL;
	if ( pSb->m_pFnt != NULL )
		pOld = dc.SelectObject ( pSb->m_pFnt );
	CRect rText ( rTime );
	dc.DrawText ( "999:99:99", -1, rText, DT_CALCRECT | DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
	rTime.left = rTime.right - rText.Width () - pSb->m_leftOff - pSb->m_rightOff;

	m_wndTime.Create (&theIcons, ICON_CLOCK, rTime, this, theBitmaps.GetByIndex (DIB_TOOLBAR));
	m_wndTime.SetText ("0:00:00");

	// rect now goes from last btn + skip to time (4 * status + skip)
	int aiStat[4] = { ICON_GAS, ICON_POWER, ICON_PEOPLE, ICON_FOOD };
	rect.right = rTime.left;
	int iWid = (rect.Width () / 4) - BAR_BTN_X_SKIP;
	rect.right = rect.left + iWid;
	for (int iInd=0; iInd<4; iInd++)
		{
		m_wndStat[iInd].Create (&theIcons, aiStat[iInd], rect, this, theBitmaps.GetByIndex (DIB_TOOLBAR));
		rect.OffsetRect (iWid + BAR_BTN_X_SKIP, 0);
		}

	// two text windows
	pSb = theIcons.GetByIndex (ICON_BAR_TEXT);
	GetClientRect (&rect);
	iWid = (rect.Width () - 3 * BAR_BTN_X_SKIP) / 2;
	rect.top += BAR_BTN_HT + (BAR_TEXT_HT - pSb->m_cyBack) / 2;
	rect.bottom = rect.top + pSb->m_cyBack;
	rect.left += BAR_BTN_X_SKIP;
	rect.right = rect.left + iWid;

	m_wndText[0].Create (&theIcons, ICON_BAR_TEXT, rect, this, theBitmaps.GetByIndex (DIB_TOOLBAR));
	rect.OffsetRect (iWid + BAR_BTN_X_SKIP, 0);
	m_wndText[1].Create (&theIcons, ICON_BAR_TEXT, rect, this, theBitmaps.GetByIndex (DIB_TOOLBAR));

	CheckButtons ();
	if ( pOld != NULL )
		dc.SelectObject ( pOld );

	return 0;
}

void CWndBar::OnClose() 
{
	// don't do anything
}

void CWndBar::EnableButton (int ID, BOOL bEnable)
{

	CBmButton *pBtn = (CBmButton *) GetDlgItem (ID);
	ASSERT_VALID (pBtn);
	if (pBtn)
		pBtn->EnableWindow (bEnable);
}

void CWndBar::OnPaint() 
{

	CPaintDC dc(this); // device context for painting
	thePal.Paint (dc.m_hDC);
	
	CRect rect;
	CWndBase::GetClientRect (&rect);
	int iWid = rect.Width ();

	CDIB * pDib = theBitmaps.GetByIndex (DIB_TOOLBAR);

	rect.bottom = rect.top + pDib->GetHeight ();

	for (int x=0; x<iWid; x+=pDib->GetWidth ())
		{
		rect.left = x;
		rect.right = __min ( x + pDib->GetWidth (), iWid );
		pDib->BitBlt ( dc, rect, CPoint ( 0, 0 ) );
		}

	// second row
	rect.top += pDib->GetHeight ();
	rect.bottom += pDib->GetHeight ();
	for (x=0; x<iWid; x+=pDib->GetWidth ())
		{
		rect.left = x;
		rect.right = __min ( x + pDib->GetWidth (), iWid );
		pDib->BitBlt ( dc, rect, CPoint ( 0, 3 ) );
		}

	thePal.EndPaint (dc.m_hDC);
	// Do not call CWndAnim::OnPaint() for painting messages
}

void CWndBar::SetStatusText (int iLine, const char * psText, CStatInst::IMPORTANCE iImp)
{

	if (psText == NULL)
		psText = "";

	ASSERT_VALID (this);
	ASSERT ((0 <= iLine) && (iLine <= 1));
	ASSERT (AfxIsValidString (psText));

	m_wndText[iLine].SetText (psText, iImp);
}

void CWndBar::SetStatusFunc (int iLine, FNSTATUSLINE fnStat, void * pData)
{

	ASSERT_VALID (this);
	ASSERT ((0 <= iLine) && (iLine <= 1));

	m_wndText[iLine].SetStatusFunc (fnStat, pData);
}

#ifdef _CHEAT
void CWndBar::SetDebugText (int iLine, const char * psText)
{

	// draw it
	CClientDC dc (&(m_wndText[iLine]));
	CFont * pOld = dc.SelectObject (&theApp.TextFont ());
	dc.SetBkColor (RGB (0, 0, 0));
	dc.SetTextColor (RGB (255, 255, 255));
	dc.SetBkMode (OPAQUE);
	CRect rect;
	m_wndText[iLine].GetClientRect (rect);
	rect.right -= 6;

	dc.DrawText (psText, -1, &rect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
	dc.SelectObject ( pOld );
}
#endif

void CWndBar::CheckButtons ()
{

	if (m_hWnd != NULL)
		{
		EnableButton (IDC_BAR_SCIENCE, theGame.GetMe()->GetExists(CStructureData::research));
		EnableButton (IDC_BAR_ADVISOR, theGame.GetMe()->GetExists(CStructureData::embassy));
		}
}

#ifdef BUGBUG
void CWndBar::OnActivateApp(BOOL bActive, HTASK hTask) 
{

	CWndBtnStatusBar::OnActivateApp(bActive, hTask);
	
	if (bActive)
		SetWindowPos (&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	else
		SetWindowPos (&CWnd::wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
#endif

BEGIN_MESSAGE_MAP(CWndBar, CWndAnim)
	//{{AFX_MSG_MAP(CWndBar)
	ON_BN_CLICKED (IDC_BAR_AREA, GotoArea)
	ON_BN_CLICKED (IDC_BAR_WORLD, GotoWorld)
	ON_BN_CLICKED (IDC_BAR_CHAT, GotoChat)
	ON_BN_CLICKED (IDC_BAR_VEHICLES, GotoVehicles)
	ON_BN_CLICKED (IDC_BAR_BUILDINGS, GotoBuildings)
	ON_BN_CLICKED (IDC_BAR_ADVISOR, GotoRelations)
	ON_BN_CLICKED (IDC_BAR_SCIENCE, GotoScience)
	ON_BN_CLICKED (IDC_BAR_FILE, GotoFile)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_MESSAGE (WM_BUTTONMOUSEMOVE, OnButtonMouseMove)
	ON_MESSAGE (WM_ICONMOUSEMOVE, OnStatusMouseMove)
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndBar message handlers

void CWndBar::OnSize(UINT nType, int cx, int cy)
{

	CWndAnim::OnSize ( nType, cx, cy );

	// if no time window we haven't been built yet
	if (m_wndTime.m_hWnd == NULL)
		return;

	// create the buttons
	int iAdd = NUM_BAR_BTNS * ( theBmBtnData.Width () + BAR_BTN_X_SKIP );
	CRect rect ( 2 * BAR_BTN_X_SKIP + iAdd, BAR_BTN_Y_START,
												BAR_BTN_X_SKIP + theBmBtnData.Width () + iAdd, 
												BAR_BTN_Y_START + theBmBtnData.Height ());

	// status bars
	// time goes on the right
	CStatData * pSb = theIcons.GetByIndex (ICON_CLOCK);
	CRect rTime;
	GetClientRect (&rTime);
	rTime.right -= BAR_BTN_X_SKIP;
	rTime.top = rect.top;
	rTime.bottom = rect.bottom;

	// we size this based on fitting the text in
	CClientDC dc ( this );
	CFont * pOld;
	if ( pSb->m_pFnt != NULL )
		pOld = dc.SelectObject ( pSb->m_pFnt );
	CRect rText ( rTime );
	dc.DrawText ( "999:99:99", -1, rText, DT_CALCRECT | DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
	rTime.left = rTime.right - rText.Width () - pSb->m_leftOff - pSb->m_rightOff;

	m_wndTime.SetWindowPos (NULL, rTime.left, rTime.top, rTime.Width (), rTime.Height (), SWP_NOZORDER);

	// rect now goes from last btn + skip to time (4 * status + skip)
	int aiStat[4] = { ICON_GAS, ICON_POWER, ICON_PEOPLE, ICON_FOOD };
	rect.right = rTime.left;
	int iWid = (rect.Width () / 4) - BAR_BTN_X_SKIP;
	rect.right = rect.left + iWid;
	for (int iInd=0; iInd<4; iInd++)
		{
		m_wndStat[iInd].SetWindowPos (NULL, rect.left, rect.top, rect.Width (), rect.Height (), SWP_NOZORDER);
		rect.OffsetRect (iWid + BAR_BTN_X_SKIP, 0);
		}

	// two text windows
	pSb = theIcons.GetByIndex (ICON_BAR_TEXT);
	GetClientRect (&rect);
	iWid = (rect.Width () - 3 * BAR_BTN_X_SKIP) / 2;
	rect.top += BAR_BTN_HT + (BAR_TEXT_HT - pSb->m_cyBack) / 2;
	rect.bottom = rect.top + pSb->m_cyBack;
	rect.left += BAR_BTN_X_SKIP;
	rect.right = rect.left + iWid;

	m_wndText[0].SetWindowPos (NULL, rect.left, rect.top, rect.Width (), rect.Height (), SWP_NOZORDER);
	rect.OffsetRect (iWid + BAR_BTN_X_SKIP, 0);
	m_wndText[1].SetWindowPos (NULL, rect.left, rect.top, rect.Width (), rect.Height (), SWP_NOZORDER);

	if ( pSb->m_pFnt != NULL )
		dc.SelectObject ( pOld );
}

LRESULT CWndBar::OnButtonMouseMove (WPARAM, LPARAM lParam)
{

	m_wndText[1].SetText ((char *) lParam);
	return (0);
}

LRESULT CWndBar::OnStatusMouseMove (WPARAM, LPARAM lParam)
{

	CString sText;	// default to blank
	if ((CWnd *) lParam == &m_wndStat[gas])
		{
		sText.LoadString (IDH_STAT_GAS);
		CString sNum1, sNum2;
		sNum1 = IntToCString (theGame.GetMe()->GetGasNeed ());
		sNum2 = IntToCString (theGame.GetMe()->GetGasHave ());
		csPrintf (&sText, (char const *) sNum1, (char const *) sNum2);
		}
	else
		if ((CWnd *) lParam == &m_wndStat[power])
			{
			sText.LoadString (IDH_STAT_POWER);
			CString sNum1, sNum2;
			sNum1 = IntToCString (theGame.GetMe()->GetPwrNeed ());
			sNum2 = IntToCString (theGame.GetMe()->GetPwrHave ());
			csPrintf (&sText, (char const *) sNum1, (char const *) sNum2);
			}
		else
			if ((CWnd *) lParam == &m_wndStat[people])
				{
				CString sNum1, sNum3;
				sNum1 = IntToCString ( theGame.GetMe()->GetPplTotal ());
				sNum3 = IntToCString ( theGame.GetMe()->GetPplVeh ());
				if ( theGame.GetMe()->GetPplBldg () >= theGame.GetMe()->GetPplNeedBldg () )
					{
					sText.LoadString (IDH_STAT_PEOPLE);
					CString sNum2, sNum4;
					sNum2 = IntToCString ( theGame.GetMe()->GetPplNeedBldg ());
					sNum4 = IntToCString ( theGame.GetMe()->GetPplBldg () - theGame.GetMe()->GetPplNeedBldg () );
					csPrintf (&sText, (char const *) sNum1, (char const *) sNum2, (char const *) sNum3, (char const *) sNum4);
					}
				else
					{
					sText.LoadString (IDH_STAT_PEOPLE2);
					CString sNum2, sNum4;
					sNum2 = IntToCString ( theGame.GetMe()->GetPplBldg () );
					sNum4 = IntToCString ( theGame.GetMe()->GetPplNeedBldg () - theGame.GetMe()->GetPplBldg () );
					csPrintf (&sText, (char const *) sNum1, (char const *) sNum2, (char const *) sNum3, (char const *) sNum4);
					}
				}
			else
				if ((CWnd *) lParam == &m_wndStat[food])
					{
					sText.LoadString (IDH_STAT_FOOD);
					CString sNum1, sNum2;
					sNum1 = IntToCString ( theGame.GetMe()->GetFoodNeed () );
					sNum2 = IntToCString ( theGame.GetMe()->GetFood () );
					csPrintf (&sText, (char const *) sNum1, (char const *) sNum2);
					}
				else
					if ((CWnd *) lParam == &m_wndTime)
						sText.LoadString (IDH_STAT_CLOCK);

	m_wndText[1].SetText (sText);
	return (0);
}

void CWndBar::GotoArea ()
{

	if (theAreaList.BringToTop () != NULL)
		return;

	// need to create one
	CWndArea * pWndArea = new CWndArea ();
	pWndArea->Create (theGame.GetMe()->m_hexMapStart, NULL, FALSE);
}

void CWndBar::GotoWorld ()
{

	if (theApp.m_wndWorld.m_hWnd == NULL)
		theApp.m_wndWorld.Create ();		// world must come after area

	theApp.m_wndWorld.ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_wndWorld.SetFocus ();
}

void CWndBar::GotoChat ()
{

	if ( ! theGame.IsNetGame () )
		return;
		
	if (theApp.m_wndChat.m_hWnd == NULL)
		{
		TRAP ();
		theApp.m_wndChat.Create ();		// world must come after area
		}

	theApp.m_wndChat.ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_wndChat.SetFocus ();
}

void CWndBar::GotoVehicles ()
{

	if (theApp.m_wndVehicles.m_hWnd == NULL)
		theApp.m_wndVehicles.Create ();		// world must come after area

	theApp.m_wndVehicles.ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_wndVehicles.SetFocus ();
}

void CWndBar::GotoBuildings ()
{

	if (theApp.m_wndBldgs.m_hWnd == NULL)
		theApp.m_wndBldgs.Create ();		// world must come after area

	theApp.m_wndBldgs.ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_wndBldgs.SetFocus ();
}

void CWndBar::GotoRelations ()
{

	// when built it's called so we enable here
	if (theGame.GetMe ()->GetExists (CStructureData::embassy))
		EnableButton (IDC_BAR_ADVISOR, TRUE);
	else
		return;

	if (theApp.m_pdlgRelations == NULL)
		theApp.m_pdlgRelations = new CDlgRelations (&theApp.m_wndMain);

	if (theApp.m_pdlgRelations->m_hWnd == NULL)
		theApp.m_pdlgRelations->Create (IDD_RELATIONS, &theApp.m_wndMain);

	theApp.m_pdlgRelations->ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_pdlgRelations->SetFocus ();
}

void CWndBar::GotoScience ()
{

	_GotoScience ();
}

void CWndBar::_GotoScience ()
{

	// when built it's called so we enable here
	if (theGame.GetMe ()->GetExists (CStructureData::research))
		EnableButton (IDC_BAR_SCIENCE, TRUE);
	else
		return;

	if (theApp.m_pdlgRsrch == NULL)
		theApp.m_pdlgRsrch = new CDlgResearch (&theApp.m_wndMain);

	if (theApp.m_pdlgRsrch->m_hWnd == NULL)
		theApp.m_pdlgRsrch->Create (IDD_RESEARCH, &theApp.m_wndMain);

	theApp.m_pdlgRsrch->ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_pdlgRsrch->SetFocus ();
}

void CWndBar::GotoFile ()
{

	if (theApp.m_pdlgFile == NULL)
		theApp.m_pdlgFile = new CDlgFile (&theApp.m_wndMain);

	if (theApp.m_pdlgFile->m_hWnd == NULL)
		theApp.m_pdlgFile->Create (iWinType == W32s ? IDD_FILE1 : IDD_FILE, &theApp.m_wndMain);

	theApp.m_pdlgFile->ShowWindow (theApp.m_wndMain.IsIconic () ? SW_SHOW : SW_RESTORE);
	theApp.m_pdlgFile->SetFocus ();
}

// if the curosr is over us - update it
void CWndBar::UpdateHelp (CWnd * pWnd)
{

	CPoint pt;
	::GetCursorPos (&pt);
	if (CWnd::WindowFromPoint (pt) == pWnd)
		SendMessage (WM_ICONMOUSEMOVE, 0, (LPARAM) pWnd );
}

void CWndBar::UpdateGas () 
{ 
static int iLastStat = 2;

	if (theGame.GetMe()->GetGasHave () < theGame.GetMe()->GetGasNeed () / 2)
		{
		if (theGame.GetMe()->GetGasHave () <= 0)
			{
			if (iLastStat != 0)
				{
				theGame.Event (EVENT_GAS_OUT, EVENT_BAD);
				iLastStat = 0;
				}
			}
		else
			if (iLastStat != 1)
				{
				theGame.Event (EVENT_GAS_LOW, EVENT_BAD);
				iLastStat = 1;
				}
		}
	else
		if (theGame.GetMe()->GetGasHave () < (theGame.GetMe()->GetGasNeed () * 3) / 4)
			{
			if (iLastStat != 1)
				{
				theGame.Event (EVENT_GAS_LOW, EVENT_BAD);
				iLastStat = 1;
				}
			else
				if (iLastStat == 0)
					theGame.Event (EVENT_GAS_OUT, EVENT_OFF);
			}
		else
			{
			if (iLastStat != 2)
				{
				theGame.Event (EVENT_GAS_OUT, EVENT_OFF);
				theGame.Event (EVENT_GAS_LOW, EVENT_OFF);
				iLastStat = 2;
				}
			}

	m_wndStat[gas].SetHaveNeed (theGame.GetMe()->GetGasHave (), __max (1, theGame.GetMe()->GetGasNeed ()));

	// if the cursor is over us - update it
	UpdateHelp ( &m_wndStat[gas]);
}

void CWndBar::UpdatePower () 
{
static int iLastStat = 0;	// we start with no power

	ASSERT_VALID (this);

	// not enough capacity
	if ( theGame.GetMe()->GetPwrNeed () > 0 )
		{
		if (theGame.GetMe()->GetPwrNeed () > theGame.GetMe()->GetPwrHave ())
			{
			if (iLastStat != 0)
				{
				theGame.Event (EVENT_POWER_LOW, EVENT_BAD);
				iLastStat = 0;
				}
			}
		else

			// ok capacity
	  	{
			if (iLastStat != 1)
				{
				theGame.Event (EVENT_POWER_LOW, EVENT_OFF);
				iLastStat = 1;
				}
		  }
		}

	m_wndStat[power].SetHaveNeed (theGame.GetMe()->GetPwrHave (), theGame.GetMe()->GetPwrNeed ());

	// if the curosr is over us - update it
	UpdateHelp ( &m_wndStat[power]);
}

void CWndBar::UpdatePeople () 
{ 
static int iLastStat = 1;

	ASSERT_STRICT_VALID (this);

	// not enough capacity
	if (theGame.GetMe()->GetPplNeedBldg () >= theGame.GetMe()->GetPplBldg ())
		{
		if (iLastStat != 0)
			{
			theGame.Event (EVENT_POP_LOW, EVENT_BAD);
			iLastStat = 0;
			}
		}
	else

		// ok
	  {
		if (iLastStat != 1)
			if (theGame.GetMe()->GetPplBldg () >= 
							theGame.GetMe()->GetPplNeedBldg () + theGame.GetMe()->GetPplNeedBldg () / 10 )
				{
				theGame.Event (EVENT_POP_LOW, EVENT_OFF);
				iLastStat = 1;
				}
	  }

	m_wndStat[people].SetHaveNeed (theGame.GetMe()->GetPplBldg (), theGame.GetMe()->GetPplNeedBldg ());

	// if the curosr is over us - update it
	UpdateHelp ( &m_wndStat[people]);
}

void CWndBar::UpdateFood () 
{ 
static int iLastStat = 2;

	ASSERT_VALID (this);

	if ( theGame.GetMe()->GetFood () <= theGame.GetMe()->GetFoodNeed () / 8 )
		{
		if ((theGame.GetMe()->GetFood () <= 0) && (iLastStat != 0))
			{
			theGame.Event (EVENT_FOOD_OUT, EVENT_BAD);
			iLastStat = 0;
			}
		}
	else
		if ( theGame.GetMe()->GetFood () < theGame.GetMe()->GetFoodNeed () / 2 )
			{
			if (iLastStat != 1)
				{
				theGame.Event (EVENT_FOOD_LOW, EVENT_BAD);
				iLastStat = 1;
				}
			else
				if (iLastStat == 0)
					theGame.Event (EVENT_FOOD_OUT, EVENT_OFF);
			}
		else
			{
			if (iLastStat != 2)
				{
				theGame.Event (EVENT_FOOD_OUT, EVENT_OFF);
				theGame.Event (EVENT_FOOD_LOW, EVENT_OFF);
				iLastStat = 2;
				}
			}

	m_wndStat[food].SetHaveNeed ( theGame.GetMe()->GetFood (), theGame.GetMe()->GetFoodNeed () );

	// if the curosr is over us - update it
	UpdateHelp ( &m_wndStat[food]);
}

void CWndBar::UpdateTime () 
{ 

	int iTime = theGame.GetElapsedSeconds ();
	char sTime [24];

	int iHour = iTime / (60 * 60);
	iTime = iTime % (60 * 60);
	itoa (iHour, sTime, 10);
	int iLen = strlen (sTime);

	int iMinute = iTime / 60;
	iTime = iTime % 60;
	if (iMinute <= 9)
		{
		strcpy (&sTime[iLen], ":0");
		sTime [iLen+2] = '0' + iMinute;
		}
	else
		{
		strcpy (&sTime[iLen], ":");
		itoa (iMinute, &sTime[iLen+1], 10);
		}

	if (iTime <= 9)
		{
		strcpy (&sTime[iLen+3], ":0");
		sTime [iLen+5] = '0' + iTime;
		}
	else
		{
		strcpy (&sTime[iLen+3], ":");
		itoa (iTime, &sTime[iLen+4], 10);
		}
	sTime [iLen+6] = 0;

	m_wndTime.SetText (sTime);

#ifdef _LOGOUT
static int iLastMinute = -1;
	if (iLastMinute != iMinute)
		{
		iLastMinute = iMinute;
		logPrintf (LOG_PRI_USEFUL, LOG_TIME, "time: %d:%d:%d", iHour, iMinute, iTime);
		}
#endif
}

void CWndBar::OnDestroy() 
{

	CWndBase::SetFnMouseMove (NULL);

	CWndAnim::OnDestroy();
}

void CWndBar::OnMouseMove(UINT nFlags, CPoint point) 
{
	
	// are we on the Comm button?
	CWnd * pChild = ChildWindowFromPoint (point);
	if (pChild == GetDlgItem (IDC_BAR_CHAT))
		{
		if ((theGame.GetMe () != NULL) && (theGame.GetMe ()->GetNetNum () != 0))
			m_wndText[1].SetText (m_sChat2);
		else
			m_wndText[1].SetText (m_sChat1);
		}
	else
		if (pChild == GetDlgItem (IDC_BAR_SCIENCE))
			m_wndText[1].SetText (m_sScience);
		else
			if (pChild == GetDlgItem (IDC_BAR_ADVISOR))
				m_wndText[1].SetText (m_sRelations);
			else
				m_wndText[1].SetText ("");
}

void CWndBar::InvalidateStatus ( void * pData )
{

	for (int iInd=0; iInd<2; iInd++)
		if ( m_wndText [iInd].GetStatusData () == pData )
			m_wndText [iInd].InvalidateRect ( NULL );
}
