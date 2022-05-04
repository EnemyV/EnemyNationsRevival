//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// options.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "options.h"
#include "player.h"
#include "help.h"
#include "license.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgOptions dialog


CDlgOptions::CDlgOptions(CWnd* pParent /*=NULL*/)
	: CDialog(iWinType == W32s ? IDD_OPTIONS1 : IDD_OPTIONS, pParent)
{
	//{{AFX_DATA_INIT(CDlgOptions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgOptions)
	if ( iWinType == W32s )
		{
		DDX_Control(pDX, IDC_FILE_SPEED, m_scrSpeed);
		DDX_Control(pDX, IDC_FILE_SOUND, m_scrSound);
		DDX_Control(pDX, IDC_FILE_MUSIC, m_scrMusic);
		}
	else
		{
		DDX_Control(pDX, IDC_FILE_SPEED, m_sldSpeed);
		DDX_Control(pDX, IDC_FILE_SOUND, m_sldSound);
		DDX_Control(pDX, IDC_FILE_MUSIC, m_sldMusic);
		}
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgOptions, CDialog)
	//{{AFX_MSG_MAP(CDlgOptions)
	ON_BN_CLICKED(IDC_FILE_ADV, OnFileAdv)
	ON_BN_CLICKED(IDC_FILE_HELP, OnFileHelp)
	ON_BN_CLICKED(IDC_FILE_VERSION, OnFileVersion)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgOptions message handlers

void CDlgOptions::OnFileAdv() 
{
	
	CDlgAdvOptions dlgAdv (this);
	dlgAdv.DoModal ();
}

void CDlgOptions::OnFileHelp() 
{
	
	theApp.WinHelp (0, HELP_CONTENTS);
}

void CDlgOptions::OnFileVersion() 
{
	
	CDlgVer dlgVer (this);
	dlgVer.DoModal ();
}

void CDlgOptions::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{

	int iNum = pScrollBar->GetScrollPos ();
	BOOL bSpeed = pScrollBar == &m_scrSpeed;

	switch (nSBCode)
		{
		case SB_LINELEFT :
			iNum--;
			break;
		case SB_LINERIGHT :
			iNum++;
			break;

		// page - move 1/2 window
		case SB_PAGELEFT :
			if ( bSpeed )
				iNum -= 4;
			else
				iNum -= 10;
			break;
		case SB_PAGERIGHT :
			if ( bSpeed )
				iNum += 4;
			else
				iNum += 10;
			break;

		// move to the end - go to the exact oppisate end of the map
		case SB_LEFT :
			iNum = 0;
			break;
		case SB_RIGHT :
			iNum = 100;
			break;

		case SB_THUMBPOSITION :
			iNum = nPos;
			break;

		default:
			CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
			return;
		}

	iNum = __max ( iNum, 0 );
	if ( bSpeed )
		iNum = __min ( iNum, NUM_SPEEDS - 1 );
	else
		iNum = __min ( iNum, 100 );
	pScrollBar->SetScrollPos ( iNum );

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CDlgOptions::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	CenterWindow ();

	if ( iWinType == W32s )
		{
		m_scrSpeed.SetScrollRange (0, NUM_SPEEDS - 1);
		m_scrSpeed.SetScrollPos (theApp.GetProfileInt ("Game", "Speed", NUM_SPEEDS/2));
		m_scrSound.SetScrollRange (0, 100);
		m_scrSound.SetScrollPos (theApp.GetProfileInt ("Game", "Sound", 100));
		m_scrMusic.SetScrollRange (0, 100);
		m_scrMusic.SetScrollPos (theApp.GetProfileInt ("Game", "Music", 100));
		}

	else
		{
		m_sldSpeed.SetLineSize (1);
		m_sldSpeed.SetPageSize (1);
		m_sldSpeed.SetRange (0, NUM_SPEEDS - 1);
		m_sldSpeed.SetSelection (0, NUM_SPEEDS - 1);
		m_sldSpeed.SetPos (theApp.GetProfileInt ("Game", "Speed", NUM_SPEEDS/2));
		m_sldSpeed.SetTicFreq (2);

		m_sldSound.SetLineSize (1);
		m_sldSound.SetPageSize (1);
		m_sldSound.SetRange (0, 100);
		m_sldSound.SetSelection (0, 100);
		m_sldSound.SetPos (theApp.GetProfileInt ("Game", "Sound", 100));
		m_sldSound.SetTicFreq (10);
	
		m_sldMusic.SetLineSize (1);
		m_sldMusic.SetPageSize (1);
		m_sldMusic.SetRange (0, 100);
		m_sldMusic.SetSelection (0, 100);
		m_sldMusic.SetPos (theApp.GetProfileInt ("Game", "Music", 100));
		m_sldMusic.SetTicFreq (10);
		}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgOptions::OnOK() 
{
	
	int iSpeed, iSound, iMusic;
	if ( iWinType == W32s )
		{
		iSpeed = m_scrSpeed.GetScrollPos ();
		iSound = m_scrSound.GetScrollPos ();
		iMusic = m_scrMusic.GetScrollPos ();
		}
	else
		{
		iSpeed = m_sldSpeed.GetPos ();
		iSound = m_sldSound.GetPos ();
		iMusic = m_sldMusic.GetPos ();
		}

	ASSERT ((0 <= iSpeed) && (iSpeed < NUM_SPEEDS));
	theApp.WriteProfileInt ("Game", "Speed", iSpeed);
	theGame.SetGameMul (iSpeed);

	theApp.WriteProfileInt ("Game", "Sound", iSound);
	theMusicPlayer.SetSoundVolume (iSound);

	theApp.WriteProfileInt ("Game", "Music", iMusic);
	theMusicPlayer.SetMusicVolume (iMusic);

	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CDlgAdvOptions dialog


CDlgAdvOptions::CDlgAdvOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdvOptions::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdvOptions)
	m_iBlt = 0;
	m_iMusic = 0;
	m_iScroll = FALSE;
	m_iPause = TRUE;
	m_iZoom = 0;
	m_iRes = 0;
	m_iIntro = FALSE;
	m_iSysColors = FALSE;
	//}}AFX_DATA_INIT
}


void CDlgAdvOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdvOptions)
	DDX_Control(pDX, IDC_OPT_LANG, m_lstLang);
	DDX_Radio(pDX, IDC_ADV_BLT, m_iBlt);
	DDX_Radio(pDX, IDC_ADV_MUSIC, m_iMusic);
	DDX_Radio(pDX, IDC_ADV_DEPTH_0, m_iDepth);
	DDX_Check(pDX, IDC_OPT_SCROLL, m_iScroll);
	DDX_Check(pDX, IDC_OPT_PAUSE, m_iPause);
	DDX_Radio(pDX, IDC_ADV_ZOOM_1, m_iZoom);
	DDX_Radio(pDX, IDC_ADV_RES_NATIVE, m_iRes);
	DDX_Check(pDX, IDC_ADV_NO_INTRO, m_iIntro);
	DDX_Check(pDX, IDC_ADV_USE_SYS_COLORS, m_iSysColors);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgAdvOptions, CDialog)
	//{{AFX_MSG_MAP(CDlgAdvOptions)
	ON_BN_CLICKED(IDC_FILE_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgAdvOptions message handlers

void CDlgAdvOptions::OnHelp() 
{
	
	theApp.WinHelp (HLP_ADV_OPTIONS, HELP_CONTEXT);
}

void CDlgAdvOptions::OnOK() 
{

	UpdateData (TRUE);

	BOOL bWarn = FALSE;
	if ( (int) theApp.GetProfileInt ("Advanced", "BLT", 0) != m_iBlt )
		{
		theApp.WriteProfileInt ("Advanced", "BLT", m_iBlt);
		bWarn = TRUE;
		}
	if ( (int) theApp.GetProfileInt ("Advanced", "ScreenResolution", 0) != m_iRes )
		{
		theApp.WriteProfileInt ("Advanced", "ScreenResolution", m_iRes);
		bWarn = TRUE;
		}
	if ( (int) theApp.GetProfileInt ("Advanced", "Music", 0) != m_iMusic )
		{
		theApp.WriteProfileInt ("Advanced", "Music", m_iMusic);
		bWarn = TRUE;
		}
	if ( (int) theApp.GetProfileInt ("Advanced", "Scroll", 0) != m_iScroll )
		{
		theApp.WriteProfileInt ("Advanced", "Scroll", m_iScroll);
		bWarn = TRUE;
		}
	if ( (int) theApp.GetProfileInt ("Advanced", "Pause", 1) != m_iPause )
		{
		theApp.WriteProfileInt ("Advanced", "Pause", m_iPause);
		bWarn = TRUE;
		}
	if ( (int) theApp.GetProfileInt ("Game", "NoIntro", 0) != m_iIntro )
		{
		theApp.WriteProfileInt ("Game", "NoIntro", m_iIntro);
		bWarn = TRUE;
		}
	if ( (int) theApp.GetProfileInt ("Advanced", "SetSysColors", 0) != m_iSysColors )
		{
		theApp.WriteProfileInt ("Advanced", "SetSysColors", m_iSysColors);
		bWarn = TRUE;
		}

	if ( (int) theApp.GetProfileInt ("Advanced", "ColorDepth", 1) != m_iDepth )
		{
		theApp.WriteProfileInt ("Advanced", "ColorDepth", m_iDepth);
		bWarn = TRUE;
		if ( m_iDepth == 2 )
			AfxMessageBox (IDS_ADV_DEPTH, MB_OK);
		}

	if ( (int) theApp.GetProfileInt ("Advanced", "Zoom", 2) != m_iZoom )
		{
		theApp.WriteProfileInt ("Advanced", "Zoom", m_iZoom);
		bWarn = TRUE;
		if ( m_iZoom == 1 )
			AfxMessageBox (IDS_ADV_ZOOM, MB_OK);
		}

	int iInd = m_lstLang.GetCurSel ();
	if ( iInd >= 0 )
		{
		iInd = m_lstLang.GetItemData ( iInd );
		if ( (int) theApp.GetProfileInt ("Advanced", "Language", theApp.m_iLangCode) != iInd )
			{
			theApp.WriteProfileInt ("Advanced", "Language", iInd );
			bWarn = TRUE;
			}
		}

	if (bWarn)
		AfxMessageBox (IDS_OPTIONS_ADV, MB_OK);

	CDialog::OnOK();
}

BOOL CDlgAdvOptions::OnInitDialog() 
{

	try
		{
		if ( ( theApp.Have24Bit () || theApp.HaveWAV () || (theApp.GetNumDataZooms() > 3) )
								&& (theApp.GetProfileInt ("Warnings", "AdvancedNote", 0) == 0) )
			{
			CDlgLicense dlgLic (6, TRUE);
			dlgLic.DoModal ();
			theApp.WriteProfileInt ( "Warnings", "AdvancedNote", 1 );
			}
		}
	catch (...)
		{
		}

	CDialog::OnInitDialog();
	
	CenterWindow ();

	UpdateData (TRUE);
	
	m_iRes = theApp.GetProfileInt ("Advanced", "ScreenResolution", 0);
	m_iRes = __minmax ( 0, 4, m_iRes );
	m_iBlt = theApp.GetProfileInt ("Advanced", "BLT", 0);
	m_iBlt = __minmax ( 0, 4, m_iBlt );
	m_iMusic = theApp.GetProfileInt ("Advanced", "Music", 0);
	m_iMusic = __minmax ( 0, 2, m_iMusic );
	m_iScroll = theApp.GetProfileInt ("Advanced", "Scroll", 0);
	m_iScroll = __minmax ( 0, 1, m_iScroll );
	m_iDepth = theApp.GetProfileInt ("Advanced", "ColorDepth", 1);
	m_iDepth = __minmax ( 0, 2, m_iDepth );
	m_iZoom = theApp.GetProfileInt ("Advanced", "Zoom", 2);
	m_iZoom = __minmax ( 0, 2, m_iZoom );
	m_iPause = theApp.GetProfileInt ("Advanced", "Pause", 1);
	m_iPause = __minmax ( 0, 1, m_iPause );
	m_iIntro = theApp.GetProfileInt ("Game", "NoIntro", 0);
	m_iIntro = __minmax ( 0, 1, m_iIntro );
	m_iSysColors = theApp.GetProfileInt ("Advanced", "SetSysColors", 0);
	m_iSysColors = __minmax ( 0, 1, m_iSysColors );

	// may not have a choice because of DAT file
	if ( ! theApp.Have24Bit () )
		m_iDepth = 1;
	if ( ! theApp.HaveWAV () )
		m_iMusic = 2;
	if ( iWinType == W32s )
		m_iRes = 0;

	UpdateData (FALSE);

	// no illegal stuff
	if ( iWinType == W32s )
		{
		GetDlgItem (IDC_ADV_RES_SPEED)->EnableWindow (FALSE);
		GetDlgItem (IDC_ADV_RES_640)->EnableWindow (FALSE);
		GetDlgItem (IDC_ADV_RES_800)->EnableWindow (FALSE);
		GetDlgItem (IDC_ADV_RES_1024)->EnableWindow (FALSE);
		GetDlgItem (IDC_RADIO2)->EnableWindow (FALSE);
		GetDlgItem (IDC_RADIO4)->EnableWindow (FALSE);
		}
	if ( theApp.GetFirstZoom () == 1 )
		{
		m_iZoom = 2;
		GetDlgItem (IDC_ADV_ZOOM_1)->EnableWindow (FALSE);
		GetDlgItem (IDC_ADV_ZOOM_2)->EnableWindow (FALSE);
		}

	// may not have a choice because of DAT file
	if ( ! theApp.Have24Bit () )
		{
		GetDlgItem (IDC_ADV_DEPTH_0)->EnableWindow (FALSE);
		GetDlgItem (IDC_ADV_DEPTH_2)->EnableWindow (FALSE);
		}
	if ( ! theApp.HaveWAV () )
		{
		GetDlgItem (IDC_ADV_MUSIC)->EnableWindow (FALSE);
		GetDlgItem (IDC_ADV_MUSIC_1)->EnableWindow (FALSE);
		}

	// language
	for (int iOn=0; iOn<theApp.m_iNumLang; iOn++)
		{
		char sBuf [62];
		GetLocaleInfo ( MAKELCID ( MAKELANGID (theApp.m_piLangAvail [iOn], SUBLANG_DEFAULT), 
																		SORT_DEFAULT ), LOCALE_SNATIVELANGNAME, sBuf, 60 );
		int iIndex = m_lstLang.AddString ( sBuf );
		m_lstLang.SetItemData ( iIndex, theApp.m_piLangAvail [iOn] );
		if ( theApp.m_piLangAvail [iOn] == theApp.m_iLangCode )
			m_lstLang.SetCurSel ( iIndex );
		}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

