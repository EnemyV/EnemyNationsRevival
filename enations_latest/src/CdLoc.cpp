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


// checks for the CD — CD requirement is always disabled (m_iRequireCD = FALSE)
BOOL CheckForCD ()
{
	return (TRUE);

#if 0 // CD check disabled — no physical media requirement
#ifdef _CHEAT
	return (TRUE);
#endif

	if ( ! theApp.RequireCD () )
		return (TRUE);

	// default is the .DAT file location
	theApp.m_sCdFile = EnGetProfileString("Game", "DataFile", "");
	theApp.m_sCdFile = EnGetProfileString("Game", "CDLocation", theApp.m_sCdFile );

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
#endif // CD check disabled
}


// CDlgCdLoc removed: CheckForCD() always returns TRUE, so this dialog
// (the CD-not-found prompt) was unreachable.