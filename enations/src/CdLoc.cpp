// CdLoc.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "CdLoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// checks for the CD
BOOL CheckForCD ()
{

#ifdef _CHEAT
	return (TRUE);
#endif

	if ( ! theApp.RequireCD () )
		return (TRUE);

	// default is the .DAT file location
	theApp.m_sCdFile = theApp.GetProfileString ("Game", "DataFile", "");
	theApp.m_sCdFile = theApp.GetProfileString ("Game", "CDLocation", theApp.m_sCdFile );

	// force the drive location
	theApp.m_sCdFile = CString ( theApp.m_sCdFile [0] ) + ":\\";

	// if not a CD - prompt
	BOOL bPrompt = FALSE;
	UINT uTyp = GetDriveType ( theApp.m_sCdFile );
	if ( uTyp != DRIVE_CDROM )
		bPrompt = TRUE;
	else

		// see if it's there
		{
		theApp.m_sCdFile += GameDataFile;
		CFileStatus fs;
		if ( CFile::GetStatus ( theApp.m_sCdFile, fs ) == 0 )
			bPrompt = TRUE;
		else
			if ( fs.m_size < 1000 )
				bPrompt = TRUE;
		}

	// leave if ok
	if ( ! bPrompt )
		return (TRUE);

	CDlgCdLoc dlg;

	// disable all of our windows
	EnableAllWindows ( NULL, FALSE );

	UINT iRtn = dlg.DoModal ();

	// disable all of our windows
	EnableAllWindows ( NULL, TRUE );

	if ( iRtn != IDOK )
		return (FALSE);
	return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgCdLoc dialog


CDlgCdLoc::CDlgCdLoc(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCdLoc::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCdLoc)
	m_strStat = _T("");
	//}}AFX_DATA_INIT
}


void CDlgCdLoc::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgCdLoc)
	DDX_Control(pDX, IDC_CD_DRIVES, m_cbDrives);
	DDX_Text(pDX, IDC_CD_STAT, m_strStat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgCdLoc, CDialog)
	//{{AFX_MSG_MAP(CDlgCdLoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCdLoc message handlers

void CDlgCdLoc::OnOK() 
{

	UpdateData (TRUE);
	m_strStat.LoadString ( IDS_CD_CHECKING );
	SetDlgItemText ( IDC_CD_STAT, m_strStat );
	UpdateData (FALSE);
	UpdateWindow ();

	// this is my attempt to get the system to read the CD.
	theApp.m_sCdFile = CString ( theApp.m_sCdFile [0] ) + ":\\";
	GetDriveType ( theApp.m_sCdFile );
	DWORD dw1, dw2, dw3, dw4;
	GetDiskFreeSpace ( theApp.m_sCdFile, &dw1, &dw2, &dw3, &dw4 );
	::Sleep ( 100 );

	// do we have it?	
	int iInd = m_cbDrives.GetCurSel ();
	if (iInd < 0)
		{
		m_strStat.LoadString ( IDS_CD_NOT );
		UpdateData (FALSE);
		UpdateWindow ();
		return;
		}
	char drv = (char) m_cbDrives.GetItemData ( iInd );

	// is it readable yet?
	CString sDrv = CString ( drv ) + ":\\";
	if ( ! GetVolumeInformation ( sDrv, NULL, 0, &dw1, &dw2, &dw3, NULL, 0 ) )
		{
		m_strStat.LoadString ( IDS_CD_NOT_READY );
		UpdateData (FALSE);
		UpdateWindow ();
		return;
		}
  
	theApp.m_sCdFile = CString ( drv ) + ":\\" + GameDataFile;
	CFileStatus fs;
	if ( (CFile::GetStatus ( theApp.m_sCdFile, fs ) == 0) || (fs.m_size < 1000) )
		{
		m_strStat.LoadString ( IDS_CD_NO_FILE );
		UpdateData (FALSE);
		UpdateWindow ();
		return;
		}

	m_strStat.LoadString ( IDS_CD_OK );
	UpdateData (FALSE);
	SetDlgItemText ( IDC_CD_STAT, m_strStat );
	UpdateWindow ();

	theApp.WriteProfileString ("Game", "CDLocation", theApp.m_sCdFile );

	CDialog::OnOK();
}

void CDlgCdLoc::OnCancel() 
{

	// we leave the app
	::PostQuitMessage ( 0 );
		
	CDialog::OnCancel();
}

BOOL CDlgCdLoc::OnInitDialog() 
{

	CDialog::OnInitDialog();
	
	// put drive letters in combo-box
	DWORD dwDrives = GetLogicalDrives ();
	char drv = 'A';
	char sDrv [5] = "A:\\";
	char sShow [4] = "A:";
	while ( dwDrives != 0 )
		{
		if ( dwDrives & 0x01 )
			{
			sDrv [0] = drv;
			UINT uTyp = GetDriveType ( sDrv );
			if ( uTyp == DRIVE_CDROM )
				{
				sShow [0] = drv;
				int iInd = m_cbDrives.AddString ( sShow );
				m_cbDrives.SetItemData ( iInd, drv );
				}
			}
		dwDrives >>= 1;
		drv ++;
		}

	// select the first one
	if ( m_cbDrives.GetCount () > 0 )
		m_cbDrives.SetCurSel (0);

	CenterWindow ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}