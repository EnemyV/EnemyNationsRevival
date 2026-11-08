// sprite.h : main header file for the SPRITE application
//
#ifndef SPRITE_H
#define SPRITE_H

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#define	ID_TREEVIEW		12	//  Some arbitrary value.

#include "progdlg.h"
#include "multprog.h"
#include "rgbcolor.h"
#include "stucki.h"
#include "sptypes.h"

class CStatusDlg;


/////////////////////////////////////////////////////////////////////////////
// CSpriteApp:
// See sprite.cpp for the implementation of this class
//

#define	WM_USER_ANIMATE_SPRITE	( WM_USER + 1 )
#define WM_USER_STOP			( WM_USER + 2 )
#define WM_USER_UPDATE_PROGRESS	( WM_USER + 3 )

class CSpriteApp : public CWinApp
{
public:
	CSpriteApp();

	RGBColor		aPalette[ 256 ];
	StuckiDither	ditherer;

	CString			fullPathIniFile;

	//  TODO : Things which should go away sooner or later.
	RGBQUAD	*pCurrentPalette;
	CPalette *pWinPalette;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpriteApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
	//  Pointers to the different document templates, so we can
	//  easily choose which template to use based on menu item choices.
	CMultiDocTemplate *	pBuildSpriteTemplate;
	CMultiDocTemplate *	pTerrSpriteTemplate;
	CMultiDocTemplate *	pVehSpriteTemplate;
	CMultiDocTemplate *	pPaletteDocTemplate;

	//  Pointer to progress dialog.  Some functions will tell
	//  us to create, update, and destroy progress dialogs.
	//  Have two separate types, it's up to us to keep track
	CProgressDlg *		pProgressDialog;
	CMultiProgress *	pMultiProgressDlg;
	CStatusDlg *			pStatDlg;

	//{{AFX_MSG(CSpriteApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileLoadpalette();
	afx_msg void OnUpdateFileViewpalette(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatesprite();
	afx_msg void OnUpdateFileCreatesprite(CCmdUI* pCmdUI);
	afx_msg void OnFileViewpalette();
	afx_msg void OnFileLoadsprite();
	afx_msg void OnFileCreateterrain();
	afx_msg void OnUpdateFileCreateterrain(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileLoadsprite(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatevehicle();
	afx_msg void OnUpdateFileCreatevehicle(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatepalette();
	afx_msg void OnNamedlg();
	afx_msg void OnFileCreateSingleDIB();
	afx_msg void OnUpdateFileCreateSingleDIB(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatemultiplebuildings();
	afx_msg void OnUpdateFileCreatemultiplebuildings(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatemultipleterrain();
	afx_msg void OnUpdateFileCreatemultipleterrain(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatemultiplevehicles();
	afx_msg void OnUpdateFileCreatemultiplevehicles(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatemultipledibs();
	afx_msg void OnUpdateFileCreatemultipledibs(CCmdUI* pCmdUI);
	afx_msg void OnFileCreateMovie();
	afx_msg void OnUpdateFileCreateMovie(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatesharewarebuildings();
	afx_msg void OnUpdateFileCreatesharewarebuildings(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatesharewareterrain();
	afx_msg void OnUpdateFileCreatesharewareterrain(CCmdUI* pCmdUI);
	afx_msg void OnFileCreatesharewarevehicles();
	afx_msg void OnUpdateFileCreatesharewarevehicles(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	public :
		BOOL FlushMessageQueue( void );
		void _OnFileCreatemultipledibs(BOOL b24);

		//  Create and update the dir on a multi-progress dialog.
		//  Call UpdateProgressDialog to update the files.
		BOOL	CreateMultiProgressDialog( int dirCnt, int fileCnt );
		void	UpdateMultiProgress( const char *pDirName );

		BOOL	CreateProgressDialog( int stepCnt, int stepSize );
		void	UpdateProgressDialog();
		void	ChangeProgressName( const char *pStr );
		void	DestroyProgressDialog( void );

		BOOL	CreateErrorDialog();
		void	AddErrorDialog(char const * pErr);
		void	DestroyErrorDialog();

		bool	SetPalette( const RGBColor *pPalette );

		void	MultiBuild( TSpriteType type, BOOL bShareware );

//  TODO :  Should go away.
		CPalette *CreateIdentPal( RGBQUAD *pPalette );
};


extern CSpriteApp theApp;

#endif
