// multprog.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "multprog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMultiProgress dialog


CMultiProgress::CMultiProgress(CWnd* pParent /*=NULL*/)
	: CDialog(CMultiProgress::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMultiProgress)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	Create( IDD_PROGRESS2 );
	CenterWindow ();
}

CMultiProgress::CMultiProgress( CWnd *pParent, int dirCnt, int fileCnt ) :
	CDialog( CMultiProgress::IDD, pParent )
{
	//{{AFX_DATA_INIT(CMultiProgress)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	Create( IDD_PROGRESS2 );

	m_dirProgress.SetRange( 0, dirCnt );
	m_dirProgress.SetStep( 1 );
	m_dirProgress.SetPos( 0 );
	m_currentDir.SetWindowText( "" );	

	m_fileProgress.SetRange( 0, fileCnt );
	m_fileProgress.SetStep( 1 );
	m_fileProgress.SetPos( 0 );
	m_currentFile.SetWindowText( "" );
}


void CMultiProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMultiProgress)
	DDX_Control(pDX, IDC_FILEPROGRESS, m_fileProgress);
	DDX_Control(pDX, IDC_DIRPROGRESS, m_dirProgress);
	DDX_Control(pDX, IDC_CURRENTFILE, m_currentFile);
	DDX_Control(pDX, IDC_CURRENTDIR, m_currentDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMultiProgress, CDialog)
	//{{AFX_MSG_MAP(CMultiProgress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultiProgress message handlers

void CMultiProgress::OnCancel() 
{
	::PostMessage( theApp.m_pMainWnd->GetSafeHwnd(), WM_USER_STOP, 0, 0L );

	CDialog::OnCancel();
}

void CMultiProgress::UpdateDir( const char *pDirName )
{
	m_dirProgress.StepIt();
	int iLen = strlen (pDirName);
	if (iLen < 23)
		m_currentDir.SetWindowText( pDirName );
	else
		{
		CString sText ("...");
		sText += (pDirName + iLen - 21);
		m_currentDir.SetWindowText( sText );
		}
}

void CMultiProgress::StepFile()
{
	m_fileProgress.StepIt();
}

void CMultiProgress::UpdateFile( const char *pFilename )
{
	int iLen = strlen (pFilename);
	if (iLen < 23)
		m_currentFile.SetWindowText( pFilename );
	else
		{
		CString sText ("...");
		sText += (pFilename + iLen - 21);
		m_currentFile.SetWindowText( sText );
		}
}

void CMultiProgress::ResetFile()
{
	m_fileProgress.SetPos( 0 );
	m_currentFile.SetWindowText( "" );
}