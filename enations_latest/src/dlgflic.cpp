// dlgflic.cpp : implementation file
//
//--------------------------------------------------------------------------
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "lastplnt.h"
#include "dlgflic.h"
#include "msgs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const int IDC_FLIC 	  = 100;

static RGBQUAD Gargbquad[ 256 ];

//------------------------------- C F l i c ---------------------------------

//---------------------------------------------------------------------------
// CFlic::CFlic
//---------------------------------------------------------------------------
CFlic::CFlic(
	char const *pszName )
	:
		m_strName( pszName )
{
	m_ptrflcinfo = new CFlcInfo;
}


/////////////////////////////////////////////////////////////////////////////
// CDlgFlic dialog

//---------------------------------------------------------------------------
// CDlgFlic::SavePalette
//---------------------------------------------------------------------------
void
CDlgFlic::SavePalette()
{
	thePal.GetColors( Gargbquad, 0, 256 );
}

//---------------------------------------------------------------------------
// CDlgFlic::RestorePalette
//---------------------------------------------------------------------------
void
CDlgFlic::RestorePalette()
{
	thePal.Init();
	thePal.SetColors( Gargbquad, 0, 256 );
}

//---------------------------------------------------------------------------
// CDlgFlic::Play - Play a flics
//---------------------------------------------------------------------------
void
CDlgFlic::Play(
	CFlic	* pflic )
{
	Play( &pflic );
}

//---------------------------------------------------------------------------
// CDlgFlic::Play - Play a sequence of flics
//---------------------------------------------------------------------------
void
CDlgFlic::Play(
	CFlic	** ppflics,
	int	   nFlics )
{
	SavePalette();
	ShowCursor( FALSE );

	CDlgFlic	dlgflic( ppflics, nFlics );

	try		// FIXIT: Check how to handle exceptions with dialog boxes
	{
		dlgflic.DoModal();
	}

	catch ( ... )
	{
		RestorePalette();
		ShowCursor( TRUE );

		throw;
	}

	RestorePalette();

	ShowCursor( TRUE );
}

//---------------------------------------------------------------------------
// CDlgFlic::CDlgFlic
//---------------------------------------------------------------------------
CDlgFlic::CDlgFlic(
	CFlic	** ppflic,
	int		nFlics )
	:
		CDialog   ( IDD, &theApp.m_wndMain ),
		m_ppflic  ( ppflic  ),
		m_nFlics	 ( nFlics ),
		m_iFlic	 ( 0      ),
		m_bStarted( FALSE  )
{
	//{{AFX_DATA_INIT(CDlgFlic)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


BEGIN_MESSAGE_MAP(CDlgFlic, CDialog)
	//{{AFX_MSG_MAP(CDlgFlic)
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_MESSAGE( WM_CTLCOLORDLG, OnCtlColorDlg )
	ON_MESSAGE( WM_PLAY_FLIC,   OnPlay )
	ON_WM_PAINT()
	ON_CONTROL( FLC_STOP, IDC_FLIC, OnFlicStop )
	ON_CONTROL( FLC_KEY,  IDC_FLIC, OnFlicKey  )
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgFlic message handlers

//--------------------------------------------------------------------------
// CDlgFlic::OnFlicStop
//--------------------------------------------------------------------------
void
CDlgFlic::OnFlicStop()
{
	m_iFlic++;

	if ( m_iFlic >= m_nFlics )
		EndDialog( 0 );
	else
		PostMessage( WM_PLAY_FLIC );
}

//--------------------------------------------------------------------------
// CDlgFlic::OnFlicKey
//--------------------------------------------------------------------------
void
CDlgFlic::OnFlicKey()
{
	Stop();
}

//--------------------------------------------------------------------------
// CDlgFlic::OnInitDialog
//--------------------------------------------------------------------------
BOOL
CDlgFlic::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect	rect( 0, 0, theApp.m_iScrnX, theApp.m_iScrnY );

	SetWindowPos( &wndTopMost, 0, 0, rect.Width(), rect.Height(), SWP_SHOWWINDOW );

	CPoint	ptCenter   = CPoint( rect.Width() / 2, rect.Height() / 2 );
	CRect		rectCenter = CRect ( ptCenter.x, ptCenter.y, ptCenter.x + 1, ptCenter.y + 1 );

	BOOL	bOK = m_flcctrl.Create( WS_CHILD | WS_VISIBLE, rectCenter, this, IDC_FLIC );

	if ( !bOK )
		EndDialog( 1 );	// FIXIT: exception?

	return TRUE;
}

//--------------------------------------------------------------------------
// CDlgFlic::OnPlay
//--------------------------------------------------------------------------
LRESULT
CDlgFlic::OnPlay(
	WPARAM,
	LPARAM )
{
	Ptr< CFile >	ptrfile = theDataFile.OpenAsFile( m_ppflic[ m_iFlic ]->GetName() );

	m_flcctrl.Play( ptrfile, m_ppflic[ m_iFlic ]->GetInfo() );

	return 0;
}

//--------------------------------------------------------------------------
// CDlgFlic::Stop
//--------------------------------------------------------------------------
void
CDlgFlic::Stop()
{
	if ( m_flcctrl.IsPlaying() )
		m_flcctrl.Stop();
}

//--------------------------------------------------------------------------
// CDlgFlic::OnOK
//--------------------------------------------------------------------------
void
CDlgFlic::OnOK()
{
	Stop();
}

//--------------------------------------------------------------------------
// CDlgFlic::OnCancel
//--------------------------------------------------------------------------
void
CDlgFlic::OnCancel()
{
	Stop();
}

//--------------------------------------------------------------------------
// CDlgFlic::OnCtlColorDlg
//--------------------------------------------------------------------------
LRESULT
CDlgFlic::OnCtlColorDlg(
	WPARAM,
	LPARAM )
{
	return ( LRESULT )( HBRUSH )::GetStockObject( BLACK_BRUSH );
}

//--------------------------------------------------------------------------
// CDlgFlic::OnPaint
//--------------------------------------------------------------------------
void CDlgFlic::OnPaint() 
{
	CDialog::OnPaint();

	if ( !m_bStarted )
	{
		m_bStarted = TRUE;

		PostMessage( WM_PLAY_FLIC );
	}
}

//--------------------------------------------------------------------------
// CDlgFlic::OnLButtonDown
//--------------------------------------------------------------------------
void CDlgFlic::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CDialog::OnLButtonDown(nFlags, point);

	Stop();
}

//--------------------------------------------------------------------------
// CDlgFlic::OnMButtonDown
//--------------------------------------------------------------------------
void CDlgFlic::OnMButtonDown(UINT nFlags, CPoint point) 
{
	CDialog::OnMButtonDown(nFlags, point);

	Stop();
}

//--------------------------------------------------------------------------
// CDlgFlic::OnRButtonDown
//--------------------------------------------------------------------------
void CDlgFlic::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CDialog::OnRButtonDown(nFlags, point);

	Stop();
}
