// NamngDlg.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "NamngDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNamingDlg dialog

CNamingDlg::CNamingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNamingDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNamingDlg)
	//}}AFX_DATA_INIT

	_stage = 0;
	_dir = 0;
	_tilt = 0;
	_damage = 0;
	_overlay = false;
	_base = true;
	_anim1 = false;
	_anim2 = false;
	_underAnim1 = false;
	_underAnim2 = false;
	_baseindex = 1;
	_anim1index = 1;
	_anim2index = 1;
	_underAnim1Index = 1;
	_underAnim2Index = 1;
	_bEditing = false;
}


void CNamingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNamingDlg)
	DDX_Control(pDX, IDC_UNDERANIM2CNT, m_underAnim2Cnt);
	DDX_Control(pDX, IDC_UNDERANIM1CNT, m_underAnim1Cnt);
	DDX_Control(pDX, IDC_ANIM2CNT, m_anim2cnt);
	DDX_Control(pDX, IDC_BASECNT, m_basecnt);
	DDX_Control(pDX, IDC_ANIM1CNT, m_anim1cnt);
	DDX_Control(pDX, IDC_FILENAME, m_filename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNamingDlg, CDialog)
	//{{AFX_MSG_MAP(CNamingDlg)
	ON_BN_CLICKED(IDC_CONSTRUCTION, OnConstruction)
	ON_BN_CLICKED(IDC_DONE, OnDone)
	ON_BN_CLICKED(IDC_FOUNDATION, OnFoundation)
	ON_BN_CLICKED(IDC_SKELETON, OnSkeleton)
	ON_BN_CLICKED(IDC_EAST, OnEast)
	ON_BN_CLICKED(IDC_NORTH, OnNorth)
	ON_BN_CLICKED(IDC_NORTHEAST, OnNortheast)
	ON_BN_CLICKED(IDC_NORTHWEST, OnNorthwest)
	ON_BN_CLICKED(IDC_SOUTH, OnSouth)
	ON_BN_CLICKED(IDC_SOUTHEAST, OnSoutheast)
	ON_BN_CLICKED(IDC_SOUTHWEST, OnSouthwest)
	ON_BN_CLICKED(IDC_WEST, OnWest)
	ON_BN_CLICKED(IDC_DESTROYED, OnDestroyed)
	ON_BN_CLICKED(IDC_HEAVYDAMAGE, OnHeavydamage)
	ON_BN_CLICKED(IDC_MINORDAMAGE, OnMinordamage)
	ON_BN_CLICKED(IDC_NODAMAGE, OnNodamage)
	ON_BN_CLICKED(IDC_ANIM1, OnAnim1)
	ON_BN_CLICKED(IDC_ANIM2, OnAnim2)
	ON_BN_CLICKED(IDC_BASE, OnBase)
	ON_EN_CHANGE(IDC_ANIM1CNT, OnChangeAnim1cnt)
	ON_EN_SETFOCUS(IDC_ANIM1CNT, OnSetfocusAnim1cnt)
	ON_EN_CHANGE(IDC_BASECNT, OnChangeBasecnt)
	ON_EN_SETFOCUS(IDC_BASECNT, OnSetfocusBasecnt)
	ON_EN_CHANGE(IDC_ANIM2CNT, OnChangeAnim2cnt)
	ON_EN_SETFOCUS(IDC_ANIM2CNT, OnSetfocusAnim2cnt)
	ON_EN_CHANGE(IDC_FILENAME, OnChangeFilename)
	ON_EN_SETFOCUS(IDC_FILENAME, OnSetfocusFilename)
	ON_EN_KILLFOCUS(IDC_FILENAME, OnKillfocusFilename)
	ON_BN_CLICKED(IDC_FLAT, OnFlat)
	ON_BN_CLICKED(IDC_TILTAWAY, OnTiltaway)
	ON_BN_CLICKED(IDC_TILTTOWARD, OnTilttoward)
	ON_BN_CLICKED(IDC_ONEPIECE, OnOnepiece)
	ON_BN_CLICKED(IDC_TWOPIECEFORE, OnTwopiecefore)
	ON_BN_CLICKED(IDC_TWOPIECEBACK, OnTwopieceback)
	ON_BN_CLICKED(IDC_OVERLAY, OnOverlay)
	ON_BN_CLICKED(IDC_UNDERANIM1, OnUnderanim1)
	ON_BN_CLICKED(IDC_UNDERANIM2, OnUnderanim2)
	ON_EN_CHANGE(IDC_UNDERANIM1CNT, OnChangeUnderanim1cnt)
	ON_EN_SETFOCUS(IDC_UNDERANIM1CNT, OnSetfocusUnderanim1cnt)
	ON_EN_CHANGE(IDC_UNDERANIM2CNT, OnChangeUnderanim2cnt)
	ON_EN_SETFOCUS(IDC_UNDERANIM2CNT, OnSetfocusUnderanim2cnt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNamingDlg message handlers

void CNamingDlg::OnConstruction() 
{
	_stage = 3;
	UpdateFilename();
}

void CNamingDlg::OnDone() 
{
	_stage = 0;
	UpdateFilename();
}

void CNamingDlg::OnFoundation() 
{
	_stage = 1;
	UpdateFilename();
}

void CNamingDlg::OnSkeleton() 
{
	_stage = 2;
	UpdateFilename();
}

void CNamingDlg::UpdateFilename()
{
	char view = ( char )( _stage * 3 + _buildType + 'A' );
	char dir = ( char )( _dir + _tilt * 8 + 'A' );

	CString	newName;
	if ( _overlay )
		newName.Format( "%c%c%01d00000", view, dir, _damage );
	else if ( _base )
		newName.Format( "%c%c%01d%01d0000", view, dir, _damage, _baseindex );
	else if ( _anim1 )
		newName.Format( "%c%c%01d0%c000", view, dir, _damage, _anim1index + 'A' - 1 );
	else if ( _anim2 )
		newName.Format( "%c%c%01d00%c00", view, dir, _damage, _anim2index + 'A' - 1 );
	else if ( _underAnim1 )
		newName.Format( "%c%c%01d000%c0", view, dir, _damage, _underAnim1Index + 'A' - 1 );
	else if ( _underAnim2 )
		newName.Format( "%c%c%01d0000%c", view, dir, _damage, _underAnim2Index + 'A' - 1 );

	m_filename.SetWindowText( newName );
}

void CNamingDlg::OnEast() 
{
	_dir = 3;
	UpdateFilename();
}

void CNamingDlg::OnNorth() 
{
	_dir = 1;
	UpdateFilename();
}

void CNamingDlg::OnNortheast() 
{
	_dir = 2;
	UpdateFilename();
}

void CNamingDlg::OnNorthwest() 
{
	_dir = 0;
	UpdateFilename();
}

void CNamingDlg::OnSouth() 
{
	_dir = 5;
	UpdateFilename();
}

void CNamingDlg::OnSoutheast() 
{
	_dir = 4;
	UpdateFilename();
}

void CNamingDlg::OnSouthwest() 
{
	_dir = 6;
	UpdateFilename();
}

void CNamingDlg::OnWest() 
{
	_dir = 7;
	UpdateFilename();
}

void CNamingDlg::OnDestroyed() 
{
	_damage = 3;
	UpdateFilename();
}

void CNamingDlg::OnHeavydamage() 
{
	_damage = 2;
	UpdateFilename();
}

void CNamingDlg::OnMinordamage() 
{
	_damage = 1;
	UpdateFilename();
}

void CNamingDlg::OnNodamage() 
{
	_damage = 0;
	UpdateFilename();
}

void CNamingDlg::ClearEdits() 
{
	_overlay = false;
	_base = false;
	_anim1 = false;
	_anim2 = false;
	_underAnim1 = false;
	_underAnim2 = false;

	_baseindex = 1;
	_anim1index = 1;
	_anim2index = 1;
	_underAnim1Index = 1;
	_underAnim2Index = 1;

	m_basecnt.SetWindowText( "" );
	m_anim1cnt.SetWindowText( "" );
	m_anim2cnt.SetWindowText( "" );
	m_underAnim1Cnt.SetWindowText( "" );
	m_underAnim2Cnt.SetWindowText( "" );
}

void CNamingDlg::OnAnim1() 
{
	if ( !_anim1 )
		ClearEdits();

	_anim1 = true;
	m_anim1cnt.GetFocus();
	CString str;
	str.Format( "%d", _anim1index );
	m_anim1cnt.SetWindowText( str );
	UpdateFilename();
}

void CNamingDlg::OnAnim2() 
{
	if ( !_anim2 )
		ClearEdits();

	_anim2 = true;
	m_anim2cnt.GetFocus();
	CString str;
	str.Format( "%d", _anim2index );
	m_anim2cnt.SetWindowText( str );
	UpdateFilename();
}

void CNamingDlg::OnBase() 
{
	if ( !_base )
		ClearEdits();

	_base = true;
	m_basecnt.GetFocus();
	CString str;
	str.Format( "%d", _baseindex );
	m_basecnt.SetWindowText( str );
	UpdateFilename();
}

void CNamingDlg::OnUnderanim1() 
{
	if ( !_underAnim1 )
		ClearEdits();

	_underAnim1 = true;
	m_underAnim1Cnt.GetFocus();
	CString str;
	str.Format( "%d", _underAnim1Index );
	m_underAnim1Cnt.SetWindowText( str );
	UpdateFilename();
}

void CNamingDlg::OnUnderanim2() 
{
	if ( !_underAnim2 )
		ClearEdits();

	_underAnim2 = true;
	m_underAnim2Cnt.GetFocus();
	CString str;
	str.Format( "%d", _underAnim2Index );
	m_underAnim2Cnt.SetWindowText( str );
	UpdateFilename();
}

void CNamingDlg::OnChangeAnim1cnt() 
{
	if ( _anim1 )
	{
		CString	res;
		m_anim1cnt.GetWindowText( res );

		int newindex = atoi( res );
		if ( newindex <= 0 || 26 < newindex )
		{
			MessageBeep( 0xFFFFFFFF );
			res.Format( "%d", _anim1index );
			m_anim1cnt.SetWindowText( res );
		}
		else
		{
			_anim1index = newindex;
			UpdateFilename();
		}
	}
}

void CNamingDlg::OnSetfocusAnim1cnt() 
{
	//  Set the current selection to the anim1 control.
	if ( _anim1 == false )
	{
		ClearEdits();

		_anim1 = true;

		SendDlgItemMessage( IDC_ANIM1, BM_CLICK, 0, 0 );
		SendDlgItemMessage( IDC_ANIM1CNT, EN_SETFOCUS, 0, 0 );

		//  Set my text to the default.
		m_anim1cnt.SetWindowText( "1" );
	}
}

void CNamingDlg::OnChangeBasecnt() 
{
	if ( _base )
	{
		CString	res;
		m_basecnt.GetWindowText( res );

		int newindex = atoi( res );
		if ( newindex <= 0 || 9 < newindex )
		{
			MessageBeep( 0xFFFFFFFF );
			res.Format( "%d", _baseindex );
			m_basecnt.SetWindowText( res );
		}
		else
		{
			_baseindex = newindex;
			UpdateFilename();
		}
	}
}

void CNamingDlg::OnSetfocusBasecnt() 
{
	//  Set the current selection to the anim1 control.
	if ( _base == false )
	{
		ClearEdits();

		_base = true;

		SendDlgItemMessage( IDC_BASE, BM_CLICK, 0, 0 );
		m_basecnt.GetFocus();
		m_basecnt.SetWindowText( "1" );
	}
}

void CNamingDlg::OnChangeAnim2cnt() 
{
	if ( _anim2 )
	{
		CString	res;
		m_anim2cnt.GetWindowText( res );

		int newindex = atoi( res );
		if ( newindex <= 0 || 26 < newindex )
		{
			MessageBeep( 0xFFFFFFFF );
			res.Format( "%d", _anim2index );
			m_anim2cnt.SetWindowText( res );
		}
		else
		{
			_anim2index = newindex;
			UpdateFilename();
		}
	}
}

void CNamingDlg::OnSetfocusAnim2cnt() 
{
	//  Set the current selection to the anim1 control.
	if ( _anim2 == false )
	{
		ClearEdits();

		_anim2 = true;

		SendDlgItemMessage( IDC_ANIM2, BM_CLICK, 0, 0 );
		m_anim2cnt.GetFocus();
		m_anim2cnt.SetWindowText( "1" );
	}
}

BOOL CNamingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	_stage = 0;
	_dir = 0;
	_damage = 0;
	_tilt = 0;
	_overlay = false;
	_base = true;
	_anim1 = false;
	_anim2 = false;
	_underAnim1 = false;
	_underAnim2 = false;
	_baseindex = 1;
	_anim1index = 1;
	_anim2index = 1;
	_underAnim1Index = 1;
	_underAnim2Index = 1;
	SendDlgItemMessage( IDC_DONE, BM_CLICK, 0, 0 );
	SendDlgItemMessage( IDC_ONEPIECE, BM_CLICK, 0, 0 );
	SendDlgItemMessage( IDC_NORTHWEST, BM_CLICK, 0, 0 );
	SendDlgItemMessage( IDC_FLAT, BM_CLICK, 0, 0 );
	SendDlgItemMessage( IDC_NODAMAGE, BM_CLICK, 0, 0 );
	SendDlgItemMessage( IDC_BASE, BM_CLICK, 0, 0 );
	m_basecnt.SetWindowText( "1" );
	UpdateFilename();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNamingDlg::ParseFilename()
{
	CString	str;
	m_filename.GetWindowText( str );
	char aName[ 9 ] = "AA010000";

	if ( str.GetLength() > 8 )
	{
		AfxMessageBox( "Illegal filename - must be eight characters" );
		return;
	}
	strncpy( aName, ( const char * )str, str.GetLength() );

	int newBuildType = aName[ 0 ] - 'A';
	int newStage = 0;
	if ( newBuildType >= 9 )
	{
		newBuildType -= 9;
		newStage = 3;
	}
	else if ( newBuildType >= 6 )
	{
		newBuildType -= 6;
		newStage = 2;
	}
	else if ( newBuildType >= 3 )
	{
		newBuildType -= 3;
		newStage = 1;
	}
	if ( newBuildType < 0 || 3 <= newBuildType ||
		newStage < 0 || 4 <= newStage )
	{
		AfxMessageBox( "Illegal filename - try making first char between A and L" );
		return;
	}

	int newDir = aName[ 1 ] - 'A';
	int newTilt = 0;
	if ( newDir >= 16 )
	{
		newTilt = 2;
		newDir -= 16;
	}
	else if ( newDir >= 8 )
	{
		newTilt = 1;
		newDir -= 8;
	}
	if ( newDir < 0 || 8 <= newDir ||
		newTilt < 0 || 3 <= newTilt )
	{
		AfxMessageBox( "Illegal filename - try making second char between A and X" );
		return;
	}

	int newDam = aName[ 2 ] - '0';
	if ( newDam < 0 || 4 <= newDam )
	{
		AfxMessageBox( "Illegal filename - try making third char between 0 and 3" );
		return;
	}

	bool newOverlay = false;
	int newBase = 0;
	int newAnim1 = 0;
	int newAnim2 = 0;
	int newUnderAnim1 = 0;
	int newUnderAnim2 = 0;
	if ( aName[ 3 ] == '0' && aName[ 4 ] == '0' && aName[ 5 ] == '0' && aName[ 6 ] == '0' && aName[ 7 ] == '0' )
	{
		newOverlay = true;
	}
	if ( aName[ 3 ] != '0' )
	{
		if ( aName[ 4 ] != '0' || aName[ 5 ] != '0' || aName[ 6 ] != '0' || aName[ 7 ] != '0' )
		{
			AfxMessageBox( "Illegal filename - must be base OR anim1 OR anim2 OR under anim1 OR under anim2\n"
						   "Only one of last five chars can be non-zero" );
			return;
		}
		newBase = aName[ 3 ] - '0';
		if ( newBase < 0 || 10 <= newBase )
		{
			AfxMessageBox( "Illegal filename - try making fourth char between 0 and 9" );
			return;
		}
	}
	if ( aName[ 4 ] != '0' )
	{
		if ( aName[ 3 ] != '0' || aName[ 5 ] != '0' || aName[ 6 ] != '0' || aName[ 7 ] != '0' )
		{
			AfxMessageBox( "Illegal filename - must be base OR anim1 OR anim2 OR under anim1 OR under anim2\n"
						   "Only one of last five chars can be non-zero" );
			return;
		}
		newAnim1 = aName[ 4 ] - 'A' + 1;
		if ( newAnim1 < 0 || 27 <= newAnim1 )
		{
			AfxMessageBox( "Illegal filename - try making fifth char between A and Z" );
			return;
		}
	}
	if ( aName[ 5 ] != '0' )
	{
		if ( aName[ 3 ] != '0' || aName[ 4 ] != '0' || aName[ 6 ] != '0' || aName[ 7 ] != '0' )
		{
			AfxMessageBox( "Illegal filename - must be base OR anim1 OR anim2 OR under anim1 OR under anim2\n"
						   "Only one of last five chars can be non-zero" );
			return;
		}
		newAnim2 = aName[ 5 ] - 'A' + 1;
		if ( newAnim2 < 0 || 27 <= newAnim2 )
		{
			AfxMessageBox( "Illegal filename - try making sixth char between A and Z" );
			return;
		}
	}
	if ( aName[ 6 ] != '0' )
	{
		if ( aName[ 3 ] != '0' || aName[ 4 ] != '0' || aName[ 5 ] != '0' || aName[ 7 ] != '0' )
		{
			AfxMessageBox( "Illegal filename - must be base OR anim1 OR anim2 OR under anim1 OR under anim2\n"
						   "Only one of last five chars can be non-zero" );
			return;
		}
		newUnderAnim1 = aName[ 6 ] - 'A' + 1;
		if ( newUnderAnim1 < 0 || 27 <= newUnderAnim1 )
		{
			AfxMessageBox( "Illegal filename - try making seventh char between A and Z" );
			return;
		}
	}
	if ( aName[ 7 ] != '0' )
	{
		if ( aName[ 3 ] != '0' || aName[ 4 ] != '0' || aName[ 5 ] != '0' || aName[ 6 ] != '0' )
		{
			AfxMessageBox( "Illegal filename - must be base OR anim1 OR anim2 OR under anim1 OR under anim2\n"
						   "Only one of last five chars can be non-zero" );
			return;
		}
		newUnderAnim2 = aName[ 7 ] - 'A' + 1;
		if ( newUnderAnim2 < 0 || 27 <= newUnderAnim2 )
		{
			AfxMessageBox( "Illegal filename - try making eighth char between A and Z" );
			return;
		}
	}

	_stage = newStage;
	SendDlgItemMessage( IDC_DONE + _stage, BM_CLICK, 0, 0 );
	_buildType = newBuildType;
	SendDlgItemMessage( IDC_ONEPIECE + _buildType, BM_CLICK, 0, 0 );
	_dir = newDir;
	SendDlgItemMessage( IDC_NORTHWEST + _dir, BM_CLICK, 0, 0 );
	_tilt = newTilt;
	SendDlgItemMessage( IDC_FLAT + _tilt, BM_CLICK, 0, 0 );
	_damage = newDam;
	SendDlgItemMessage( IDC_NODAMAGE + _damage, BM_CLICK, 0, 0 );
	ClearEdits();
	if ( newOverlay )
	{
		_overlay = true;
		SendDlgItemMessage( IDC_OVERLAY, BM_CLICK, 0, 0 );
	}
	else if ( newBase )
	{
		_base = true;
		_baseindex = newBase;
		SendDlgItemMessage( IDC_BASE, BM_CLICK, 0, 0 );
		CString str;
		str.Format( "%d", _baseindex );
		m_basecnt.SetWindowText( str );
	}
	else if ( newAnim1 )
	{
		_anim1 = true;
		_anim1index = newAnim1;
		SendDlgItemMessage( IDC_ANIM1, BM_CLICK, 0, 0 );
		CString str;
		str.Format( "%d", _anim1index );
		m_anim1cnt.SetWindowText( str );
	}
	else if ( newAnim2 )
	{
		_anim2 = true;
		_anim2index = newAnim2;
		SendDlgItemMessage( IDC_ANIM2, BM_CLICK, 0, 0 );
		CString str;
		str.Format( "%d", _anim2index );
		m_anim2cnt.SetWindowText( str );
	}
	else if ( newUnderAnim1 )
	{
		_underAnim1 = true;
		_underAnim1Index = newUnderAnim1;
		SendDlgItemMessage( IDC_UNDERANIM1, BM_CLICK, 0, 0 );
		CString str;
		str.Format( "%d", _underAnim1Index );
		m_underAnim1Cnt.SetWindowText( str );
	}
	else if ( newUnderAnim2 )
	{
		_underAnim2 = true;
		_underAnim2Index = newUnderAnim2;
		SendDlgItemMessage( IDC_UNDERANIM2, BM_CLICK, 0, 0 );
		CString str;
		str.Format( "%d", _underAnim2Index );
		m_underAnim2Cnt.SetWindowText( str );
	}
}

void CNamingDlg::OnChangeFilename() 
{
	if ( _bEditing )
		ParseFilename();
}

void CNamingDlg::OnSetfocusFilename() 
{
	_bEditing = true;
}

void CNamingDlg::OnKillfocusFilename() 
{
	_bEditing = false;
}

void CNamingDlg::OnFlat() 
{
	_tilt = 0;
	UpdateFilename();
}

void CNamingDlg::OnTiltaway() 
{
	_tilt = 2;
	UpdateFilename();
}

void CNamingDlg::OnTilttoward() 
{
	_tilt = 1;
	UpdateFilename();
}


void CNamingDlg::OnOnepiece() 
{
	_buildType = 0;
	UpdateFilename();
}

void CNamingDlg::OnTwopiecefore() 
{
	_buildType = 1;
	UpdateFilename();
}

void CNamingDlg::OnTwopieceback() 
{
	_buildType = 2;
	UpdateFilename();
}

void CNamingDlg::OnOverlay() 
{
	ClearEdits();

	_overlay = true;
	UpdateFilename();
}


void CNamingDlg::OnChangeUnderanim1cnt() 
{
	if ( _underAnim1 )
	{
		CString	res;
		m_underAnim1Cnt.GetWindowText( res );

		int newindex = atoi( res );
		if ( newindex <= 0 || 26 < newindex )
		{
			MessageBeep( 0xFFFFFFFF );
			res.Format( "%d", _underAnim1Index );
			m_underAnim1Cnt.SetWindowText( res );
		}
		else
		{
			_underAnim1Index = newindex;
			UpdateFilename();
		}
	}
}

void CNamingDlg::OnSetfocusUnderanim1cnt() 
{
	//  Set the current selection to the anim1 control.
	if ( _underAnim1 == false )
	{
		ClearEdits();

		_underAnim1 = true;

		SendDlgItemMessage( IDC_UNDERANIM1, BM_CLICK, 0, 0 );
		SendDlgItemMessage( IDC_UNDERANIM1CNT, EN_SETFOCUS, 0, 0 );

		//  Set my text to the default.
		m_underAnim1Cnt.SetWindowText( "1" );
	}
}

void CNamingDlg::OnChangeUnderanim2cnt() 
{
	if ( _underAnim2 )
	{
		CString	res;
		m_underAnim2Cnt.GetWindowText( res );

		int newindex = atoi( res );
		if ( newindex <= 0 || 26 < newindex )
		{
			MessageBeep( 0xFFFFFFFF );
			res.Format( "%d", _underAnim2Index );
			m_underAnim2Cnt.SetWindowText( res );
		}
		else
		{
			_underAnim2Index = newindex;
			UpdateFilename();
		}
	}
}

void CNamingDlg::OnSetfocusUnderanim2cnt() 
{
	//  Set the current selection to the anim2 control.
	if ( _underAnim2 == false )
	{
		ClearEdits();

		_underAnim2 = true;

		SendDlgItemMessage( IDC_UNDERANIM2, BM_CLICK, 0, 0 );
		SendDlgItemMessage( IDC_UNDERANIM2CNT, EN_SETFOCUS, 0, 0 );

		//  Set my text to the default.
		m_underAnim2Cnt.SetWindowText( "1" );
	}
}

