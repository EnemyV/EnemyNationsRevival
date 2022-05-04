// sprite.cpp : Defines the class behaviors for the application.
//

//  MFC includes
#include "stdafx.h"

//  Non-MFC system includes
#include <io.h>
#include <mmsystem.h>

//  MFC-Related project includes
#include "bd.h"
#include "cdirdlg.h"
#include "sprite.h"
#include "mainfrm.h"
#include "namngdlg.h"
#include "paldoc.h"
#include "palview.h"
#include "rbuild.h"
#include "spritvw.h"
#include "splitwnd.h"
#include "td.h"
#include "terrain.h"
#include "treeview.h"
#include "vd.h"
#include "statusdlg.h"

//  Non-MFC related project includes.
#include "bmpfile.h"
#include "tgafile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpriteApp

BEGIN_MESSAGE_MAP(CSpriteApp, CWinApp)
	//{{AFX_MSG_MAP(CSpriteApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_LOADPALETTE, OnFileLoadpalette)
	ON_UPDATE_COMMAND_UI(ID_FILE_VIEWPALETTE, OnUpdateFileViewpalette)
	ON_COMMAND(ID_FILE_CREATESPRITE, OnFileCreatesprite)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATESPRITE, OnUpdateFileCreatesprite)
	ON_COMMAND(ID_FILE_VIEWPALETTE, OnFileViewpalette)
	ON_COMMAND(ID_FILE_LOADSPRITE, OnFileLoadsprite)
	ON_COMMAND(ID_FILE_CREATETERRAIN, OnFileCreateterrain)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATETERRAIN, OnUpdateFileCreateterrain)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOADSPRITE, OnUpdateFileLoadsprite)
	ON_COMMAND(ID_FILE_CREATEVEHICLE, OnFileCreatevehicle)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATEVEHICLE, OnUpdateFileCreatevehicle)
	ON_COMMAND(ID_FILE_CREATEPALETTE, OnFileCreatepalette)
	ON_COMMAND(ID_NAMEDLG, OnNamedlg)
	ON_COMMAND(ID_FILE_CREATESINGLEDIB, OnFileCreateSingleDIB)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATESINGLEDIB, OnUpdateFileCreateSingleDIB)
	ON_COMMAND(ID_FILE_CREATEMULTIPLEBUILDINGS, OnFileCreatemultiplebuildings)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATEMULTIPLEBUILDINGS, OnUpdateFileCreatemultiplebuildings)
	ON_COMMAND(ID_FILE_CREATEMULTIPLETERRAIN, OnFileCreatemultipleterrain)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATEMULTIPLETERRAIN, OnUpdateFileCreatemultipleterrain)
	ON_COMMAND(ID_FILE_CREATEMULTIPLEVEHICLES, OnFileCreatemultiplevehicles)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATEMULTIPLEVEHICLES, OnUpdateFileCreatemultiplevehicles)
	ON_COMMAND(ID_FILE_CREATEMULTIPLEDIBS, OnFileCreatemultipledibs)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATEMULTIPLEDIBS, OnUpdateFileCreatemultipledibs)
	ON_COMMAND(ID_FILE_CREATE_MOVIE, OnFileCreateMovie)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATE_MOVIE, OnUpdateFileCreateMovie)
	ON_COMMAND(ID_FILE_CREATESHAREWAREBUILDINGS, OnFileCreatesharewarebuildings)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATESHAREWAREBUILDINGS, OnUpdateFileCreatesharewarebuildings)
	ON_COMMAND(ID_FILE_CREATESHAREWARETERRAIN, OnFileCreatesharewareterrain)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATESHAREWARETERRAIN, OnUpdateFileCreatesharewareterrain)
	ON_COMMAND(ID_FILE_CREATESHAREWAREVEHICLES, OnFileCreatesharewarevehicles)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATESHAREWAREVEHICLES, OnUpdateFileCreatesharewarevehicles)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpriteApp construction

CSpriteApp::CSpriteApp() :
	fullPathIniFile( "" )
{
	pCurrentPalette = NULL;
	pWinPalette = NULL;
	pProgressDialog = NULL;
	pStatDlg = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSpriteApp object

CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSpriteApp initialization

BOOL CSpriteApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	//  Figure out where we are.  This assumes that there's no way for 
	//  someone to change my current directory before I get to this point.
	//  If they can, this obviously won't work.
	char aModuleFilename[ MAX_PATH ];
	GetCurrentDirectory( sizeof( aModuleFilename ), aModuleFilename );

	//  Make sure it ends in '\'
	if ( aModuleFilename[ strlen( aModuleFilename ) - 1 ] != '\\' )
		strcat( aModuleFilename, "\\" );
	fullPathIniFile = CString( aModuleFilename ) + "sprite.ini";
	// DNT - get it from win95 dir
	fullPathIniFile = "sprite.ini";

	Enable3dControls();

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	//  Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	//  Note that each of the sprite type templates below shares the same view
	//  code - only the document types are different.

	//  Register the building sprite template.
	pBuildSpriteTemplate = new CMultiDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS( CBD ),
		RUNTIME_CLASS( CSplitWnd ),
		RUNTIME_CLASS( CTView ) );
	AddDocTemplate( pBuildSpriteTemplate );

	//  Register the terrain sprite template.
	pTerrSpriteTemplate = new CMultiDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS( CTD ),
		RUNTIME_CLASS( CSplitWnd ),
		RUNTIME_CLASS( CTView ) );
	AddDocTemplate( pTerrSpriteTemplate );

	//  Register the vehicle sprite template.
	pVehSpriteTemplate = new CMultiDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS( CVD ),
		RUNTIME_CLASS( CSplitWnd ),
		RUNTIME_CLASS( CTView ) );
	AddDocTemplate( pVehSpriteTemplate );

	//  Register the palette template.  This one has it's own
	//  document type and it's own view type.
	pPaletteDocTemplate = new CMultiDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS( CPalDoc ),
		RUNTIME_CLASS( CMDIChildWnd ),
		RUNTIME_CLASS( CPalView ) );
	AddDocTemplate( pPaletteDocTemplate );

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	if (m_lpCmdLine[0] != '\0')
	{
		// NOTE: add command line processing here
	}

	//  TODO : Load the last palette.  For now, come up with no palette.
	pCurrentPalette = NULL;
	pWinPalette = NULL;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSpriteApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSpriteApp commands

void CSpriteApp::OnFileLoadsprite() 
{
	char *pSpriteFilter = "Sprite Files\0*.8;*.15;*.16;*.24;*.32\0\0\0";
	CFileDialog	fileDialog( TRUE );
	fileDialog.m_ofn.lpstrFilter = pSpriteFilter;
	fileDialog.m_ofn.lpstrTitle = "Load Sprite";

	if ( fileDialog.DoModal() == IDOK )
	{
		//  Open the file and read the first item.  This should be a type which
		//  identifies the sprite.
		FILE *pFile = fopen( ( const char * )fileDialog.GetPathName(), "rb" );
		if ( pFile == NULL )
		{
			AfxMessageBox( "Unable to open file!" );
			return;
		}

		TSpriteType type;
		if ( fread( &type, 1, sizeof( type ), pFile ) != sizeof( type ) )
		{
			AfxMessageBox( "Error reading file!" );
			return;
		}
		fclose( pFile );
		
		switch( type )
		{
			default :
				AfxMessageBox( "Unknown sprite type!" );
				return;

			case TERRAIN_SPRITE :
				pTerrSpriteTemplate->OpenDocumentFile( fileDialog.GetPathName() );
				break;

			case BUILDING_SPRITE :
				pBuildSpriteTemplate->OpenDocumentFile( fileDialog.GetPathName() );
				break;

			case VEHICLE_SPRITE :
				pVehSpriteTemplate->OpenDocumentFile( fileDialog.GetPathName() );
				break;
		}
	}
}

void CSpriteApp::OnUpdateFileLoadsprite(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileViewpalette() 
{
	//  Create a new palette document.  The document doesn't really do
	//  anything, it's just there to create the view.
	pPaletteDocTemplate->OpenDocumentFile( NULL );
}

void CSpriteApp::OnUpdateFileViewpalette(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

bool CSpriteApp::SetPalette( const RGBColor *pPalette )
{
	//  Delete the currently loaded palette.
	delete[] pCurrentPalette;
	delete pWinPalette;
		
	//  Copy the new palette, as an RGBColor, an RGBQUAD, and
	//  an HPALETTE.
	//  TODO :  Maybe _one_ palette?
	( void )memcpy( aPalette, pPalette, 256 * sizeof( RGBColor ) );
	pCurrentPalette = new RGBQUAD[ 256 ];
	if ( pCurrentPalette == NULL )
		return false;
	int palIndex;
	for( palIndex = 0; palIndex < 256; palIndex++ )
	{
		pCurrentPalette[ palIndex ].rgbRed = pPalette[ palIndex ].Red();
		pCurrentPalette[ palIndex ].rgbGreen = pPalette[ palIndex ].Green();
		pCurrentPalette[ palIndex ].rgbBlue = pPalette[ palIndex ].Blue();
	}
	pWinPalette = CreateIdentPal( pCurrentPalette );

	//  Initialize the ditherer with the new palette.
	ditherer.Init( aPalette, 256 );

	//  Update all palette documents currently visible.
	POSITION docPos = pPaletteDocTemplate->GetFirstDocPosition();
	while( docPos != NULL )
	{
		CDocument *pDoc = pPaletteDocTemplate->GetNextDoc( docPos );
		pDoc->UpdateAllViews( NULL );
	}

	return true;
}

void CSpriteApp::OnFileCreatepalette() 
{
	pPaletteDocTemplate->OpenDocumentFile( NULL );	
}

void CSpriteApp::OnFileLoadpalette() 
{
	//  Get the name of the palette to load.  We assume that
	//  the palette is an 8-bit DIB.
	CFileDialog	fileDialog( TRUE );

	fileDialog.m_ofn.lpstrFilter = "Palette files\0*.bmp;*.dib\0\0\0";
	fileDialog.m_ofn.lpstrTitle = "Choose palette file";
	
	if ( fileDialog.DoModal() == IDOK )
	{
		pPaletteDocTemplate->OpenDocumentFile( ( const char * )fileDialog.GetPathName() );
	}	
}

void CSpriteApp::OnFileCreatesprite() 
{
	//  If this returns NULL, the user has already been informed.
	pBuildSpriteTemplate->OpenDocumentFile( NULL );
}

void CSpriteApp::OnFileCreateterrain() 
{
	//  This will cause a new terrain document to be created.
	//  That document's OnNewDocument will then deal with finding
	//  the initial file(s) from which to create the terrain.
	pTerrSpriteTemplate->OpenDocumentFile( NULL );
}

void CSpriteApp::OnFileCreatevehicle() 
{
	pVehSpriteTemplate->OpenDocumentFile( NULL );
}

void CSpriteApp::OnUpdateFileCreatesprite(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnUpdateFileCreateterrain(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnUpdateFileCreatevehicle(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

int CSpriteApp::ExitInstance() 
{
	if ( pCurrentPalette != NULL )
		free( pCurrentPalette );
	delete pWinPalette;
	
	return CWinApp::ExitInstance();
}

BOOL CSpriteApp::OnIdle(LONG lCount) 
{
	static DWORD	timeLastAnimated = 0;
	DWORD			timeNow = 0;

	( void )CWinApp::OnIdle(lCount);

	timeNow = timeGetTime();
	if ( timeNow - timeLastAnimated > 41 )
	{
		//  Send a message to all sprite windows,
		//  telling them to animate.
		::PostMessage( m_pMainWnd->m_hWnd, WM_COMMAND, ID_ANIMATE_SPRITE, 0L );

		timeLastAnimated = timeNow;
	}

	return TRUE;
}

CPalette *CSpriteApp::CreateIdentPal( RGBQUAD *pPalette )
{
	ASSERT( pPalette != NULL );

	int i;
	struct {
		WORD Version;
		WORD NumberOfEntries;
		PALETTEENTRY aEntries[256];
	} Palette =
	{
		0x300,
		256
	};

	//*** Just use the screen DC where we need it
	HDC hdc = GetDC(NULL);

	//*** For SYSPAL_NOSTATIC, just copy the color table into
	//*** a PALETTEENTRY array and replace the first and last entries
	//*** with black and white
	if (GetSystemPaletteUse(hdc) == SYSPAL_NOSTATIC)

	{
		//*** Fill in the palette with the given values, marking each
		//*** as PC_NOCOLLAPSE
		for(i = 0; i < 256; i++)
		{
			Palette.aEntries[i].peRed = pPalette[i].rgbRed;
			Palette.aEntries[i].peGreen = pPalette[i].rgbGreen;
			Palette.aEntries[i].peBlue = pPalette[i].rgbBlue;
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
		}

		//*** Mark any unused entries PC_NOCOLLAPSE
		for (; i < 256; ++i)
		{
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;

		}

		//*** Make sure the last entry is white
		//*** This may replace an entry in the array!
		Palette.aEntries[255].peRed = 255;
		Palette.aEntries[255].peGreen = 255;
		Palette.aEntries[255].peBlue = 255;
		Palette.aEntries[255].peFlags = 0;

		//*** And the first is black
		//*** This may replace an entry in the array!
		Palette.aEntries[0].peRed = 0;
		Palette.aEntries[0].peGreen = 0;
		Palette.aEntries[0].peBlue = 0;
		Palette.aEntries[0].peFlags = 0;

	}
	else
	//*** For SYSPAL_STATIC, get the twenty static colors into
	//*** the array, then fill in the empty spaces with the
	//*** given color table
	{
		int nStaticColors;
		int nUsableColors;

		//*** Get the static colors from the system palette
		nStaticColors = GetDeviceCaps(hdc, NUMCOLORS);
		GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

		//*** Set the peFlags of the lower static colors to zero
		nStaticColors = nStaticColors / 2;

		for (i=0; i<nStaticColors; i++)
			Palette.aEntries[i].peFlags = 0;

		//*** Fill in the entries from the given color table
		nUsableColors = 256 - nStaticColors;
		for (; i<nUsableColors; i++)
		{
			Palette.aEntries[i].peRed = pPalette[i].rgbRed;
			Palette.aEntries[i].peGreen = pPalette[i].rgbGreen;
			Palette.aEntries[i].peBlue = pPalette[i].rgbBlue;
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
		}

		//*** Mark any empty entries as PC_NOCOLLAPSE

		for (; i<256 - nStaticColors; i++)
			Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;

		//*** Set the peFlags of the upper static colors to zero
		for (i = 256 - nStaticColors; i<256; i++)
			Palette.aEntries[i].peFlags = 0;
	}

	//*** Remember to release the DC!
	ReleaseDC(NULL, hdc);

	//*** Return the palette
	CPalette *pIdentPalette = new CPalette;
	pIdentPalette->CreatePalette((LOGPALETTE *)&Palette);
	return pIdentPalette;
}

BOOL CSpriteApp::FlushMessageQueue( void )
{
	MSG	msg;

    while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
    { 
		if ( msg.message == WM_USER_STOP )
			return FALSE;

        if ( !PumpMessage( ) ) 
        { 
            ::PostQuitMessage( 0 ); 
            break; 
        } 
    } 
 
    //  Let MFC do its idle processing  Note that
	//  we are only concerned with letting MFC do it's
	//  idle processing;  we don't care about our idle
	//  processing, so we call the base class OnIdle.
    LONG lIdle = 0;
    while ( CWinApp::OnIdle( lIdle++ ) )
        ;  

	return TRUE;
}

BOOL CSpriteApp::CreateMultiProgressDialog( int dirCnt, int fileCnt )
{
	ASSERT( pProgressDialog == NULL );
	ASSERT( pMultiProgressDlg == NULL );

	pMultiProgressDlg = new CMultiProgress( m_pMainWnd, dirCnt, fileCnt );
	if ( pMultiProgressDlg == NULL )
		return FALSE;

	pMultiProgressDlg->CenterWindow ();

	return TRUE;
}

void CSpriteApp::UpdateMultiProgress( const char *pStr )
{
	ASSERT( pStr != NULL );
	ASSERT( pMultiProgressDlg != NULL );
	ASSERT( pProgressDialog == NULL );

	pMultiProgressDlg->UpdateDir( pStr );
	pMultiProgressDlg->ResetFile();
}

BOOL CSpriteApp::CreateProgressDialog( int stepCnt, int stepSize )
{
	ASSERT( pProgressDialog == NULL );
	ASSERT( pMultiProgressDlg == NULL );

	pProgressDialog = new CProgressDlg( m_pMainWnd, stepCnt, stepSize );
	if ( pProgressDialog == NULL )
		return FALSE;

	pProgressDialog->CenterWindow ();

	return TRUE;
}

BOOL CSpriteApp::CreateErrorDialog()
{

	DestroyErrorDialog ();

	pStatDlg = new CStatusDlg(m_pMainWnd);
	if ( pStatDlg == NULL )
		return FALSE;

	pStatDlg->InvalidateRect (NULL);
	pStatDlg->UpdateWindow ();

	return TRUE;
}

void CSpriteApp::AddErrorDialog(char const * pErr)
{
	char * psz     = new char [ 1 + strlen( pErr ) ];
	char * pszTemp = psz;

	strcpy( psz, pErr );

	for ( char * pszToken; NULL != ( pszToken = strtok( pszTemp, "\n" )); pszTemp = NULL )
		pStatDlg->m_lstErr.AddString( pszToken );

	delete psz;
}

void CSpriteApp::DestroyErrorDialog( void )
{

	delete pStatDlg;
	pStatDlg = NULL;
}

void CSpriteApp::UpdateProgressDialog()
{
	if ( pProgressDialog != NULL )
	{
		ASSERT( pMultiProgressDlg == NULL );
		pProgressDialog->m_progressCtl.StepIt();
	}
	else
	{
		ASSERT( pMultiProgressDlg != NULL );
		pMultiProgressDlg->StepFile();
	}
}

void CSpriteApp::ChangeProgressName( const char *pStr )
{
	if ( pProgressDialog != NULL )
	{
		ASSERT( pMultiProgressDlg == NULL );
		pProgressDialog->SetWindowText( pStr );
	}
	else
	{
		ASSERT( pMultiProgressDlg != NULL );
		pMultiProgressDlg->UpdateFile( pStr );
	}
}

void CSpriteApp::DestroyProgressDialog( void )
{
	if ( pProgressDialog != NULL )
	{
		ASSERT( pMultiProgressDlg == NULL );
		delete pProgressDialog;
		pProgressDialog = NULL;
	}
	else
	{
		ASSERT( pMultiProgressDlg != NULL );
		delete pMultiProgressDlg;
		pMultiProgressDlg = NULL;
	}
}

void CSpriteApp::OnNamedlg() 
{
	CNamingDlg	nameDlg;
	( void )nameDlg.DoModal();
}

void CSpriteApp::OnFileCreateSingleDIB() 
{
	CFileDialog		fileDlg( TRUE );
	fileDlg.m_ofn.lpstrFilter = "Image Files\0*.bmp;*.dib;*.tga\0BMP Files\0*.bmp;*.dib\0TGA Files\0*.tga\0\0";
	if ( fileDlg.DoModal() == IDOK )
	{
		CString filename;
		filename = fileDlg.GetPathName();

		//  I'd much rather deal with a char * than a CString.
		//  Fortunately, MFC provides an easy way for me to do this.
		const char *pFilename = ( const char * )filename;
		const char *pExt = strrchr( pFilename, '.' );
		if ( pExt == NULL ||
			( _stricmp( pExt, ".dib" ) != 0 &&
			  _stricmp( pExt, ".bmp" ) != 0 &&
			  _stricmp( pExt, ".tga" ) != 0 ) )
		{
			CString str;
			str.Format( "Unable to determine type of file: %s", pFilename );
			AfxMessageBox( str );
			return;
		}

		//  Load the file.
		Image *pImage = NULL;
		if ( _stricmp( pExt, ".dib" ) == 0 ||
			_stricmp( pExt, ".bmp" ) == 0 )
		{
			BMPFile	file;
			pImage = file.Load( pFilename );
		}
		else if ( _stricmp( pExt, ".tga" ) == 0 )
		{
			TGAFile file;
			pImage = file.Load( pFilename );
		}

		if ( pImage == NULL )
		{
			CString str;
			str.Format( "Unable to load file: %s", pFilename );
			AfxMessageBox( str );
			return;
		}

		//  Dither the image.
		Image *pDitheredImage = theApp.ditherer.DitherImage( pImage );
		if ( pDitheredImage == NULL )
		{
			CString str;
			str.Format( "Unable to dither file: %s", pFilename );
			AfxMessageBox( str );
			return;
		}

		CFileDialog	saveDlg( FALSE );
		saveDlg.m_ofn.lpstrFilter = "Bitmap Files\0*.bmp;*.dib\0\0";
		saveDlg.m_ofn.lpstrTitle = "Enter dib name ( no extension )";

		if ( saveDlg.DoModal() == IDOK )
		{
			CString savePath;
			savePath = saveDlg.GetPathName();

			char aFilename[ MAX_PATH ];
			( void )strcpy( aFilename, saveDlg.GetPathName() );
			char *pExt = strrchr( aFilename, '.' );
			char *pDir = strrchr( aFilename, '\\' );
			if ( pExt == NULL || pExt < pDir )
				pExt = &aFilename[ strlen( aFilename ) - 1 ];

			BMPFile	file;
			( void )strcpy( pExt, ".d8" );
			if ( file.Save( aFilename, pDitheredImage ) == false )
			{
				CString str;
				str.Format( "Error saving file %s!\n", aFilename );
				AfxMessageBox( str );
			}

			( void )strcpy( pExt, ".d24" );
			if ( file.Save( aFilename, pImage ) == false )
			{
				CString str;
				str.Format( "Error saving file %s!\n", aFilename );
				AfxMessageBox( str );
			}
		}

		delete pImage;
		delete pDitheredImage;
	}
}

void CSpriteApp::OnUpdateFileCreateSingleDIB(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatemultiplebuildings() 
{
/*
	//  Get the directory in which to recursively create the
	//  building sprites.
	cdirdlg		dirDlg;
	if ( dirDlg.DoModal() != IDOK )
		return;

	//  Get the total directory, which is the current
	//  directory plus the selected directory.  Remember
	//  that the selected directory will have a leading '[' 
	//  and trailing ']' that should be removed.
	char	aDirectory[ MAX_PATH ];
	GetCurrentDirectory( MAX_PATH, aDirectory );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );
	strcat( aDirectory, &dirDlg.aCurrentDirSel[ 1 ] );
	aDirectory[ strlen( aDirectory ) - 1 ] = '\0';
	strcat( aDirectory, "\\" );

	//  Determine how many directories I'll be building in.
	//  Note the cheesy way I get the building sprites count.
	//  As opposed to figuring out a way to get a static virtual function.
	int dirCnt = CountLeafDirs( aDirectory );
	BuildingSprite	build;

	int depthCnt = 2;
#ifdef CREATE15
	depthCnt++;
#endif
#ifdef CREATE16
	depthCnt++;
#endif
#ifdef CREATE32
	depthCnt++;
#endif

	if ( CreateMultiProgressDialog( dirCnt, depthCnt * build.GetViewCount() ) == FALSE )
	{
		AfxMessageBox( "Unable To Create Progress Dialog!" );
		return;
	}

	if ( RecursiveBuild( BUILDING_SPRITE, aDirectory ) == false )
	{
		CString str;
		str.Format( "Error creating building sprites!" );
		AfxMessageBox( str );
	}

	theApp.DestroyProgressDialog();
*/
	MultiBuild( BUILDING_SPRITE, FALSE );
}

void CSpriteApp::OnUpdateFileCreatemultiplebuildings(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatemultipleterrain() 
{
/*
	//  Get the directory in which to recursively create the
	//  building sprites.
	cdirdlg		dirDlg;
	if ( dirDlg.DoModal() != IDOK )
		return;

	//  Get the total directory, which is the current
	//  directory plus the selected directory.  Remember
	//  that the selected directory will have a leading '[' 
	//  and trailing ']' that should be removed.
	char	aDirectory[ MAX_PATH ];
	GetCurrentDirectory( MAX_PATH, aDirectory );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );
	strcat( aDirectory, &dirDlg.aCurrentDirSel[ 1 ] );
	aDirectory[ strlen( aDirectory ) - 1 ] = '\0';
	strcat( aDirectory, "\\" );

	//  Determine how many directories I'll be building in.
	int dirCnt = CountLeafDirs( aDirectory );
	TerrainSprite terrain;

	int depthCnt = 2;
#ifdef CREATE15
	depthCnt++;
#endif
#ifdef CREATE16
	depthCnt++;
#endif
#ifdef CREATE32
	depthCnt++;
#endif

	if ( CreateMultiProgressDialog( dirCnt, depthCnt * terrain.GetViewCount() ) == FALSE )
	{
		AfxMessageBox( "Unable To Create Progress Dialog!" );
		return;
	}

	if ( RecursiveBuild( TERRAIN_SPRITE, aDirectory ) == false )
	{
		CString str;
		str.Format( "Error creating terrain sprites!" );
		AfxMessageBox( str );
	}

	theApp.DestroyProgressDialog();
*/
	MultiBuild( TERRAIN_SPRITE, FALSE );
}

void CSpriteApp::OnUpdateFileCreatemultipleterrain(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatemultiplevehicles() 
{
/*
	//  Get the directory in which to recursively create the
	//  building sprites.
	cdirdlg		dirDlg;
	if ( dirDlg.DoModal() != IDOK )
		return;

	//  Get the total directory, which is the current
	//  directory plus the selected directory.  Remember
	//  that the selected directory will have a leading '[' 
	//  and trailing ']' that should be removed.
	char	aDirectory[ MAX_PATH ];
	GetCurrentDirectory( MAX_PATH, aDirectory );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );
	strcat( aDirectory, &dirDlg.aCurrentDirSel[ 1 ] );
	aDirectory[ strlen( aDirectory ) - 1 ] = '\0';
	strcat( aDirectory, "\\" );

	//  Determine how many directories I'll be building in.
	int dirCnt = CountLeafDirs( aDirectory );
	VehicleSprite vehicle;

	int depthCnt = 2;
#ifdef CREATE15
	depthCnt++;
#endif
#ifdef CREATE16
	depthCnt++;
#endif
#ifdef CREATE32
	depthCnt++;
#endif

	if ( CreateMultiProgressDialog( dirCnt, depthCnt * vehicle.GetViewCount() ) == FALSE )
	{
		AfxMessageBox( "Unable To Create Progress Dialog!" );
		return;
	}

	if ( RecursiveBuild( VEHICLE_SPRITE, aDirectory ) == false )
	{
		CString str;
		str.Format( "Error creating vehicle sprites!" );
		AfxMessageBox( str );
	}

	theApp.DestroyProgressDialog();
*/
	MultiBuild( VEHICLE_SPRITE, FALSE );
}

void CSpriteApp::OnUpdateFileCreatemultiplevehicles(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatemultipledibs() 
{

	_OnFileCreatemultipledibs (TRUE);
}

void CSpriteApp::OnFileCreateMovie() 
{

	_OnFileCreatemultipledibs (FALSE);
}

void CSpriteApp::_OnFileCreatemultipledibs(BOOL b24) 
{
	//  Get the directory in which to create the DIBs.
	cdirdlg		dirDlg (m_pMainWnd);
	if ( dirDlg.DoModal() != IDOK )
		return;

	//  Get the total directory, which is the current
	//  directory plus the selected directory.  Remember
	//  that the selected directory will have a leading '[' 
	//  and trailing ']' that should be removed.
	char	aDirectory[ MAX_PATH ];
	GetCurrentDirectory( MAX_PATH, aDirectory );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );
	strcat( aDirectory, &dirDlg.aCurrentDirSel[ 1 ] );
	aDirectory[ strlen( aDirectory ) - 1 ] = '\0';
	strcat( aDirectory, "\\" );

	char aPattern[ MAX_PATH ];
	strcpy( aPattern, aDirectory );
	strcat( aPattern, "*.*" );

	//  Count the number of files matching the file spec, so we can put up
	//  the progress dialog.
	int cnt = 0;
	struct _finddata_t	findData;
	BOOL bAllFilesRead = FALSE;
	long hFile = _findfirst( aPattern, &findData );
	if ( hFile == -1L )
		bAllFilesRead = TRUE;
	while( bAllFilesRead == FALSE )
	{
		//  See if it's a file type we recognize.
		char *pExt;
		pExt = strrchr( findData.name, '.' );
		if (b24)
			{
			if ( pExt != NULL &&
				( _stricmp( pExt, ".bmp" ) == 0 ||
				  _stricmp( pExt, ".dib" ) == 0 ||
				  _stricmp( pExt, ".tga" ) == 0 ) )
				cnt++;
			}
		else
			if ( pExt != NULL &&
				  _stricmp( pExt, ".tga" ) == 0 )
				cnt++;

		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	if ( cnt == 0 )
	{
		AfxMessageBox( "No files found to process" );
		return;
	}

	theApp.CreateErrorDialog();
	theApp.CreateProgressDialog( cnt, 1 );

	//  Now go through and process the files.
	bAllFilesRead = FALSE;
	hFile = _findfirst( aPattern, &findData );
	if ( hFile == -1L )
		bAllFilesRead = TRUE;
	while( bAllFilesRead == FALSE )
	{
		Image *pImage = NULL;
		Image *pDitheredImage = NULL;
		char *pDir = NULL;
		BMPFile file;

		//  See if it's a file type we recognize.
		char *pExt;
		pExt = strrchr( findData.name, '.' );
		if ( pExt == NULL ||
			( _stricmp( pExt, ".bmp" ) != 0 &&
			  _stricmp( pExt, ".dib" ) != 0 &&
			  _stricmp( pExt, ".tga" ) != 0 ) )
			goto $NextFile;
		if ( (! b24) && _stricmp( pExt, ".tga" ) != 0 )
			goto $NextFile;

		char aFilename[ MAX_PATH ];
		( void )strcpy( aFilename, aDirectory );
		( void )strcat( aFilename, findData.name );

		theApp.ChangeProgressName ( aFilename );
		theApp.UpdateProgressDialog ();
		theApp.FlushMessageQueue ( );

		//  Load the file.
		if ( _stricmp( pExt, ".dib" ) == 0 ||
			_stricmp( pExt, ".bmp" ) == 0 )
		{
			BMPFile	file;
			pImage = file.Load( aFilename );
		}
		else if ( _stricmp( pExt, ".tga" ) == 0 )
		{
			TGAFile file;
			pImage = file.Load( aFilename );
		}

		if ( pImage == NULL )
		{
			CString str;
			str.Format( "Unable to load file: %s", aFilename );
			AfxMessageBox( str );
			return;
		}

		//  Dither the image.
		pDitheredImage = theApp.ditherer.DitherImage( pImage );
		if ( pDitheredImage == NULL )
		{
			CString str;
			str.Format( "Unable to dither file: %s", aFilename );
			AfxMessageBox( str );
			return;
		}

		//  Get the full pathname.
		pExt = strrchr( aFilename, '.' );
		pDir = strrchr( aFilename, '\\' );
		if ( pExt == NULL || pExt < pDir )
			pExt = &aFilename[ strlen( aFilename ) - 1 ];
		
		//  Save the dithered image as a .d8
		if (b24)
			strcpy( pExt, ".d8" );
		else
			strcpy( pExt, ".dib" );
		if ( file.Save( aFilename, pDitheredImage ) == false )
		{
			CString str;
			str.Format( "Error saving file %s!\n", aFilename );
			AfxMessageBox( str );
		}

		//  Save the undithered image as a .d24
		if (b24)
		{
			strcpy( pExt, ".d24" );
			if ( file.Save( aFilename, pImage ) == false )
			{
				CString str;
				str.Format( "Error saving file %s!\n", aFilename );
				AfxMessageBox( str );
			}
		}

		delete pImage;
		delete pDitheredImage;

$NextFile:;
		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	theApp.DestroyProgressDialog();
}

void CSpriteApp::OnUpdateFileCreatemultipledibs(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnUpdateFileCreateMovie(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatesharewarebuildings() 
{
	MultiBuild( BUILDING_SPRITE, TRUE );
}

void CSpriteApp::OnUpdateFileCreatesharewarebuildings(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatesharewareterrain() 
{
	MultiBuild( TERRAIN_SPRITE, TRUE );
}

void CSpriteApp::OnUpdateFileCreatesharewareterrain(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::OnFileCreatesharewarevehicles() 
{
	MultiBuild( VEHICLE_SPRITE, TRUE );
}

void CSpriteApp::OnUpdateFileCreatesharewarevehicles(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( pCurrentPalette != NULL );
}

void CSpriteApp::MultiBuild( TSpriteType type, BOOL bShareware )
{
	//  Get the directory in which to recursively create the
	//  building sprites.
	cdirdlg		dirDlg (m_pMainWnd);
	if ( dirDlg.DoModal() != IDOK )
		return;

	//  Get the total directory, which is the current
	//  directory plus the selected directory.  Remember
	//  that the selected directory will have a leading '[' 
	//  and trailing ']' that should be removed.
	char	aDirectory[ MAX_PATH ];
	GetCurrentDirectory( MAX_PATH, aDirectory );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );
	strcat( aDirectory, &dirDlg.aCurrentDirSel[ 1 ] );
	aDirectory[ strlen( aDirectory ) - 1 ] = '\0';
	strcat( aDirectory, "\\" );

	//  Determine how many directories I'll be building in.
	int dirCnt = CountLeafDirs( aDirectory );

	int depthCnt = 2;
#ifdef CREATE15
	depthCnt++;
#endif
#ifdef CREATE16
	depthCnt++;
#endif
#ifdef CREATE32
	depthCnt++;
#endif

	int filesPerView = 0;
	if ( type == BUILDING_SPRITE )
	{
		BuildingSprite building;
		filesPerView = building.GetViewCount();
	}
	else if ( type == TERRAIN_SPRITE )
	{
		TerrainSprite terrain;
		filesPerView = terrain.GetViewCount();
	}
	else if ( type == VEHICLE_SPRITE )
	{
		VehicleSprite vehicle;
		filesPerView = vehicle.GetViewCount();
	}
	else
		ASSERT( FALSE );

	theApp.CreateErrorDialog();
	if ( CreateMultiProgressDialog( dirCnt, depthCnt * filesPerView ) == FALSE )
	{
		AfxMessageBox( "Unable To Create Progress Dialog!" );
		return;
	}

	if ( RecursiveBuild( type, aDirectory, bShareware ) == false )
	{
		CString str;
		str.Format( "Error creating sprites!" );
		AfxMessageBox( str );
	}

	DestroyProgressDialog();
}
