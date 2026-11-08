// cdirdlg.cpp : implementation file
//

#include "stdafx.h"

#include <ctype.h>
#include <io.h>
 
#include "sprite.h"
#include "cdirdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// cdirdlg dialog

cdirdlg::cdirdlg(CWnd* pParent /*=NULL*/)
	: CDialog(cdirdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(cdirdlg)
	//}}AFX_DATA_INIT
}


void cdirdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cdirdlg)
	DDX_Control(pDX, IDC_DRIVELIST, m_driveList);
	DDX_Control(pDX, IDC_DIRLIST, m_dirList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cdirdlg, CDialog)
	//{{AFX_MSG_MAP(cdirdlg)
	ON_LBN_DBLCLK(IDC_DIRLIST, OnDblclkDirlist)
	ON_CBN_SELCHANGE(IDC_DRIVELIST, OnSelchangeDrivelist)
	ON_LBN_SELCHANGE(IDC_DIRLIST, OnSelchangeDirlist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// cdirdlg message handlers

void cdirdlg::OnDblclkDirlist() 
{
	//  Get the name of the directory which was just clicked on.
	char buffer[ MAX_PATH ];
	ASSERT( m_dirList.GetTextLen( m_dirList.GetCurSel() ) < MAX_PATH );
	m_dirList.GetText( m_dirList.GetCurSel(), buffer );

	//  Remove the trailing ']'.
	buffer[ strlen( buffer ) - 1 ] = '\0';

	//  Move to the given subdirectory.  Note that we point to the second
	//  character in the buffer, since the first character is '['.
	SetCurrentDirectory( &buffer[ 1 ] );

	//  Get a new list of the subdirectories available.
	m_dirList.ResetContent();
	m_dirList.Dir( 0x8010, "*.*" );

	//  Select the 0th entry.
	m_dirList.SetCurSel( 0 );
}

BOOL cdirdlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//  Search all of the files in the current directory and add
	//  them to the listbox control.
	//  0x0010 means subdirectories.
	//  0x8000 means only files of the given type, so
	//  0x8010 means only add subdirectories.
	m_dirList.Dir( 0x8010, "*.*" );
	m_dirList.SetCurSel( 0 );
		
	m_driveList.Dir( DDL_EXCLUSIVE | DDL_DRIVES, "*.*" );
	char buffer[ MAX_PATH ], drive[ 6 ];
	GetCurrentDirectory( MAX_PATH, buffer );
	strcpy( drive, "[- -]" );
	drive[ 2 ] = ( char )tolower( buffer[ 0 ] );
	prevDriveSel = m_driveList.SelectString( -1, drive );

	CenterWindow ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void cdirdlg::OnSelchangeDrivelist() 
{
	char buffer[ MAX_PATH ];

	//  Get the selected drive
	ASSERT( m_driveList.GetLBTextLen( m_driveList.GetCurSel() ) < MAX_PATH );
	m_driveList.GetLBText( m_driveList.GetCurSel(), buffer );

	//  replace the trailing -] with a :.
	strcpy( &buffer[ strlen( buffer ) - 2 ], ":" );

	//  Set the current directory.  This can fail if the drive is invalid.
	if ( SetCurrentDirectory( &buffer[ 2 ] ) == FALSE )
	{
		//  What to do?
		CString str;
		str.Format( "Error reading drive %c", buffer[ 2 ] );
		AfxMessageBox( str );
		m_driveList.SetCurSel( prevDriveSel );
	}

	prevDriveSel = m_driveList.GetCurSel();

	//  Get a new list of the subdirectories available.
	m_dirList.ResetContent();
	m_dirList.Dir( 0x8010, "*.*" );

	//  Select the 0th entry.
	m_dirList.SetCurSel( 0 );
}

void cdirdlg::OnSelchangeDirlist() 
{
	m_dirList.GetText( m_dirList.GetCurSel(), aCurrentDirSel );
}
