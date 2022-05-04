//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// relation.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "relation.h"
#include "area.h"

#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef _CHEAT
CDlgStats * pDlgStats = NULL;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgRelations dialog


CDlgRelations::CDlgRelations(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgRelations::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgRelations)
	m_btnRelations = -1;
	//}}AFX_DATA_INIT
}


void CDlgRelations::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgRelations)
	DDX_Control(pDX, IDC_PLYR_GIVE, m_btnGive);
	DDX_Control(pDX, IDC_REL_LIST, m_lstPlayers);
	DDX_Radio(pDX, IDC_PLYR_ALLIANCE, m_btnRelations);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgRelations, CDialog)
	//{{AFX_MSG_MAP(CDlgRelations)
	ON_LBN_DBLCLK(IDC_REL_LIST, OnDblclkRelList)
	ON_LBN_SELCHANGE(IDC_REL_LIST, OnSelchangeRelList)
	ON_BN_CLICKED(IDC_PLYR_ALLIANCE, OnRadio)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_WM_SYSCOMMAND()
	ON_BN_CLICKED(IDC_PLYR_HOSTILE, OnRadio)
	ON_BN_CLICKED(IDC_PLYR_NEUTRAL, OnRadio)
	ON_BN_CLICKED(IDC_PLYR_PEACE, OnRadio)
	ON_BN_CLICKED(IDC_PLYR_WAR, OnRadio)
	ON_BN_CLICKED(IDC_PLYR_GIVE, OnPlyrGive)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgRelations message handlers

BOOL CDlgRelations::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	m_btnGive.EnableWindow ( FALSE );

	// put players in the list box
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_VALID (pPlr);
		if (! pPlr->IsMe ())
			{
			int iInd = m_lstPlayers.AddString (pPlr->GetName ());
			m_lstPlayers.SetItemDataPtr (iInd, pPlr);
			}
		}

	// if rebuilding use old pos
	if ( theGame.m_wpRelations.length != 0 )
		SetWindowPlacement ( &(theGame.m_wpRelations) );
	else
		{
		// down by e-mail
		SetWindowPos (NULL, 0, theApp.m_iRow2, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

		// save position
		theGame.m_wpRelations.length = sizeof (WINDOWPLACEMENT);
		GetWindowPlacement ( &(theGame.m_wpRelations) );
		}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgRelations::RemovePlayer ( CPlayer * pPlr )
{

	int iNum = m_lstPlayers.GetCount ();
	for (int iInd=0; iInd<iNum; iInd++)
		if ( m_lstPlayers.GetItemDataPtr ( iInd ) == (void *) pPlr )
			{
			m_lstPlayers.DeleteString ( iInd );
			return;
			}
}

void CDlgRelations::ChangedIfAi ()
{

	// redo all players
	m_lstPlayers.ResetContent ();

	// put players in the list box
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_VALID (pPlr);
		if (! pPlr->IsMe ())
			{
			int iInd = m_lstPlayers.AddString (pPlr->GetName ());
			m_lstPlayers.SetItemDataPtr (iInd, pPlr);
			}
		}
}

void CDlgRelations::OnOK() 
{
	
	ShowWindow (SW_HIDE);
}

void CDlgRelations::OnDblclkRelList() 
{

#ifdef _CHEAT
	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		return;
	CPlayer * pPlr = (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd);

	if (pDlgStats != NULL)
		{
		pDlgStats->m_iPlyrNum = pPlr->GetPlyrNum ();
		pDlgStats->UpdateData (TRUE);
		pDlgStats->m_strName = pPlr->GetName ();
		pDlgStats->UpdateData (FALSE);
		pDlgStats->Update ();
		return;
		}

	pDlgStats = new CDlgStats (&theApp.m_wndMain);
	pDlgStats->m_iPlyrNum = pPlr->GetPlyrNum ();
	pDlgStats->m_strName = pPlr->GetName ();
	pDlgStats->Create (IDD_PLYR_STAT, &theApp.m_wndMain);
	pDlgStats->Update ();
	
#endif
}

void CDlgRelations::OnRadio() 
{
	
	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		return;
		
	UpdateData (TRUE);

	NewRelations ((CPlayer *) m_lstPlayers.GetItemDataPtr (iInd), m_btnRelations);
}

void CDlgRelations::NewRelations (CPlayer * pPlyr, int iLevel)
{

	// if no change nothing to do
	if (pPlyr->GetRelations () == iLevel)
		return;

	if (pPlyr->IsMe ())
		return;

	pPlyr->SetRelations (iLevel);

	CMsgSetRelations msg (theGame.GetMe (), pPlyr, iLevel);
	theGame.PostToClient (pPlyr, &msg, sizeof (msg));

	// things may have changed (we may want to stop/start/switch targets) - redo oppo fire
	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);

		if (pBldg->GetOwner()->IsMe ())
			{
			pBldg->SetOppo (NULL);

			// tell them the contents of our buildings if alliance
			if ( iLevel == RELATIONS_ALLIANCE )
				pBldg->UpdateStore (TRUE);
			}
		}

	pos = theVehicleMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CVehicle *pVeh;
		theVehicleMap.GetNextAssoc (pos, dwID, pVeh);
		ASSERT_STRICT_VALID (pVeh);
		if (pVeh->GetOwner()->IsMe ())
			pVeh->SetOppo (NULL);
		}

	// update the window
	if ( theApp.m_pdlgRelations != NULL )
		{
		int iInd = theApp.m_pdlgRelations->m_lstPlayers.GetCurSel ();
		if (iInd >= 0)
			{
			CPlayer * pPlr = (CPlayer *) theApp.m_pdlgRelations->m_lstPlayers.GetItemDataPtr (iInd);
			theApp.m_pdlgRelations->UpdateData (TRUE);
			theApp.m_pdlgRelations->m_btnRelations = pPlr->GetRelations ();
			theApp.m_pdlgRelations->UpdateData (FALSE);
			}
		theApp.m_pdlgRelations->InvalidateRect ( NULL );
		}
}

void CDlgRelations::OnSelchangeRelList() 
{
	
	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		{
		m_btnGive.EnableWindow ( FALSE );
		return;
		}

	CWndArea * pWndArea = theAreaList.GetTop ();
	if ( pWndArea == NULL )
		{
		TRAP ();
		m_btnGive.EnableWindow ( FALSE );
		}
	else
		m_btnGive.EnableWindow ( pWndArea->NumGiveable () > 0 );
		
	CPlayer * pPlr = (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd);
	ASSERT_VALID (pPlr);

	// cannot alliance with an AI player
	GetDlgItem (IDC_PLYR_ALLIANCE)->EnableWindow ( ! pPlr->IsAI () );

	UpdateData (TRUE);
	m_btnRelations = pPlr->GetRelations ();
	UpdateData (FALSE);
}

void CDlgRelations::OnActivate (UINT nState, CWnd *, BOOL)
{

	if ( nState == WA_INACTIVE )
		{
		m_btnGive.EnableWindow ( FALSE );
		return;
		}

	if ( m_lstPlayers.GetCurSel () < 0 )
		{
		m_btnGive.EnableWindow ( FALSE );
		return;
		}

	// are there area map units selected
	CWndArea * pWndArea = theAreaList.GetTop ();
	if ( pWndArea == NULL )
		{
		TRAP ();
		m_btnGive.EnableWindow ( FALSE );
		return;
		}

	m_btnGive.EnableWindow ( pWndArea->NumGiveable () > 0 );
}

void CDlgRelations::OnPlyrGive() 
{

	// is there anything?
	CWndArea * pWndArea = theAreaList.GetTop ();
	if ( ( pWndArea == NULL ) || ( pWndArea->NumGiveable () <= 0 ) )
		{
		TRAP ();
		m_btnGive.EnableWindow ( FALSE );
		return;
		}

	int iInd = m_lstPlayers.GetCurSel ();
	if (iInd < 0)
		{
		TRAP ();
		m_btnGive.EnableWindow ( FALSE );
		return;
		}
	
	// hand them over	
	pWndArea->GiveSelectedUnits ( (CPlayer *) m_lstPlayers.GetItemDataPtr (iInd) );

	// we have nothing selected
	m_btnGive.EnableWindow ( FALSE );
}

void CDlgRelations::OnMeasureItem (int, LPMEASUREITEMSTRUCT lpMIS)
{

	ASSERT_VALID (this);

	lpMIS->itemHeight = theApp.TextHt () + 1;
}

void CDlgRelations::OnDrawItem (int, LPDRAWITEMSTRUCT lpDIS)
{

	if ( ODT_BUTTON == lpDIS->CtlType ) // GG
	{
		Default();
		return;
	}

	CPlayer * pPlr = (CPlayer *) lpDIS->itemData;
	if (pPlr == NULL)
		return;

	CDC *pDc = CDC::FromHandle (lpDIS->hDC);
	if (pDc == NULL)
		return;
	thePal.Paint (pDc->m_hDC);
	CFont * pOldFont = pDc->SelectObject (&theApp.TextFont ());

	CRect rect ( lpDIS->rcItem );

	if (lpDIS->itemState & ODS_SELECTED)
		{
		pDc->FillSolidRect ( &rect, pPlr->GetRGBColor () );
		pDc->SetTextColor ( RGB (255, 255, 255) );
		}
	else
	  {
		pDc->FillSolidRect ( &rect, RGB (255, 255, 255) );
		pDc->SetTextColor ( RGB (0, 0, 0) );
	  }
	pDc->SetBkMode ( TRANSPARENT );

	rect.left += 5;
	rect.top += 1;
	pDc->DrawText ( pPlr->GetName (), -1, &rect, DT_TOP | DT_LEFT | DT_SINGLELINE | DT_NOPREFIX );

	if (lpDIS->itemState & ODS_SELECTED)
		pDc->SetTextColor (RGB (0, 0, 0));
	else
		pDc->SetTextColor (pPlr->GetRGBColor ());

	rect.left --;
	rect.top --;
	pDc->DrawText ( pPlr->GetName (), -1, &rect, DT_TOP | DT_LEFT | DT_SINGLELINE | DT_NOPREFIX );

	pDc->SelectObject ( pOldFont );
	thePal.EndPaint (pDc->m_hDC);
}

void CDlgRelations::OnCancel() 
{

	DestroyWindow ();
}

void CDlgRelations::OnDestroy() 
{

	theApp.m_pdlgRelations = NULL;

	CDialog::OnDestroy();
}


#ifdef _CHEAT
/////////////////////////////////////////////////////////////////////////////
// CDlgStats dialog


CDlgStats::CDlgStats(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgStats::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgStats)
	m_strFood = _T("");
	m_strGas = _T("");
	m_strName = _T("");
	m_strPeople = _T("");
	m_strPower = _T("");
	//}}AFX_DATA_INIT
}


void CDlgStats::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgStats)
	DDX_Text(pDX, IDC_STAT_FOOD, m_strFood);
	DDX_Text(pDX, IDC_STAT_GAS, m_strGas);
	DDX_Text(pDX, IDC_STAT_NAME, m_strName);
	DDX_Text(pDX, IDC_STAT_PEOPLE, m_strPeople);
	DDX_Text(pDX, IDC_STAT_POWER, m_strPower);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgStats, CDialog)
	//{{AFX_MSG_MAP(CDlgStats)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgStats message handlers

BOOL CDlgStats::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgStats::OnOK() 
{
	
	pDlgStats = NULL;

	CDialog::OnOK();
}

void CDlgStats::Update ()
{

	UpdateData (TRUE);

	CPlayer * pPlyr = theGame.GetPlayerByPlyr (m_iPlyrNum);	
	m_strGas = IntToCString (pPlyr->GetGasHave ()) + "/" + IntToCString (pPlyr->GetGasNeed ());
	m_strPower = IntToCString (pPlyr->GetPwrHave ()) + "/" + IntToCString (pPlyr->GetPwrNeed ());
	m_strPeople = IntToCString (pPlyr->GetPplVeh ()) + ":" + IntToCString (pPlyr->GetPplBldg ()) 
																			+ "/" + IntToCString (pPlyr->GetPplNeedBldg ());
	m_strFood = IntToCString (pPlyr->GetFood ()) + "/" + IntToCString (pPlyr->GetFoodNeed ());

	UpdateData (FALSE);
}
#endif

