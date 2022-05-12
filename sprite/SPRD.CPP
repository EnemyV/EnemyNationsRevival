// sprd.cpp : implementation file
//

#include "stdafx.h"
#include <mmsystem.h>
#include "cdirdlg.h"
#include "sprite.h"
#include "sprd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSprite

IMPLEMENT_DYNCREATE(CSprite, CDocument)

CSprite::CSprite()
{
	pSprite8 = NULL;

#ifdef CREATE15
	pSprite15 = NULL;
#endif

#ifdef CREATE16
	pSprite16 = NULL;
#endif

	pSprite24 = NULL;

#ifdef CREATE32
	pSprite32 = NULL;
#endif

	pCurrentSprite = NULL;
}

CSprite::~CSprite()
{
	delete pSprite8;

#ifdef CREATE15
	delete pSprite15;
#endif

#ifdef CREATE16
	delete pSprite16;
#endif

	delete pSprite24;

#ifdef CREATE32
	delete pSprite32;
#endif

	//  NOTE : pCurrentSprite points to one of the above sprites,
	//  so no need to delete.
}

BEGIN_MESSAGE_MAP(CSprite, CDocument)
	//{{AFX_MSG_MAP(CSprite)
	ON_COMMAND(ID_FILE_SAVESPRITE, OnFileSavesprite)
	//}}AFX_MSG_MAP
	ON_COMMAND( ID_ANIMATE_SPRITE, OnAnimateSprite )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprite diagnostics

#ifdef _DEBUG
void CSprite::AssertValid() const
{
	CDocument::AssertValid();
}

void CSprite::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprite serialization

void CSprite::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// NOTE: add storing code here
	}
	else
	{
		// NOTE: add loading code here
	}
}

void CSprite::OnAnimateSprite()
{
	static DWORD	timeAtLastAnim = 0;
	static DWORD	remainderFromLastAnim = 0;
	DWORD	timeNow, elapsedMilliseconds;
	DWORD	frames;

	if ( pCurrentSprite )
	{
		timeNow = timeGetTime();
		elapsedMilliseconds = timeNow - timeAtLastAnim + remainderFromLastAnim;
		frames = elapsedMilliseconds / 41;
		remainderFromLastAnim = elapsedMilliseconds - frames * 41;
		timeAtLastAnim = timeNow;

		//  Total kludge to keep us from hanging 
		//  forever on the first update, when frames
		//  is likely to be several thousand.
		if ( frames > 30 )
			frames = 30;

		pCurrentSprite->Animate( frames );

		//  Update all of the views.  The 1 will tell the views not to
		//  clear to white before drawing.
		UpdateAllViews( NULL, 1 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSprite commands

BOOL CSprite::OnNewDocument()
{
	ASSERT( pSprite8 != NULL );
#ifdef CREATE15
	ASSERT( pSprite15 != NULL );
#endif
#ifdef CREATE16
	ASSERT( pSprite16 != NULL );
#endif
	ASSERT( pSprite24 != NULL );
#ifdef CREATE32
	ASSERT( pSprite32 != NULL );
#endif

	//  Since we're not using serialization, this does
	//  very little, but we'll call it anyway.
	if ( CDocument::OnNewDocument() == FALSE )
		return FALSE;

	cdirdlg	chooseDir (theApp.m_pMainWnd);
	if ( chooseDir.DoModal() != IDOK )
		return FALSE;

	//  Get the total path, which is the current
	//  directory and the selected directory ( minus the 
	//  leading '[' ).
	GetCurrentDirectory( MAX_PATH, aDirectory );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );
	strcat( aDirectory, &chooseDir.aCurrentDirSel[ 1 ] );

	//  Remove trailing ']'
	aDirectory[ strlen( aDirectory ) - 1 ] = '\0';

	//  Figure out how many color depths we'll actually be creating.
	int	depthCnt = 2;	//  8 and 24
#ifdef CREATE15
	depthCnt++;
#endif
#ifdef CREATE16
	depthCnt++;
#endif
#ifdef CREATE32
	depthCnt++;
#endif

	//  Set up the progress dialog.  It will be set up
	//  to have one bar for each view for each sprite.
	theApp.CreateErrorDialog();
	if ( theApp.CreateProgressDialog( depthCnt * pSprite8->GetViewCount(), 1 ) == FALSE )
		return FALSE;

	//  Create the sprites.
	if ( pSprite8->CreateSprite( aDirectory, 8, theApp.aPalette ) == FALSE
#ifdef CREATE15
		|| pSprite15->CreateSprite( aDirectory, 15 ) == FALSE
#endif
#ifdef CREATE16
		|| pSprite16->CreateSprite( aDirectory, 16 ) == FALSE
#endif
		|| pSprite24->CreateSprite( aDirectory, 24 ) == FALSE
#ifdef CREATE32
		|| pSprite32->CreateSprite( aDirectory, 32 ) == FALSE
#endif
		)
	{
		theApp.DestroyProgressDialog();
		return FALSE;
	}

	//  Destroy the progress dialog, since we're done
	//  crunching and are ready to save.
	theApp.DestroyProgressDialog();

	//  Figure out the default name for the sprite.
	char *pDir = strrchr( aDirectory, '\\' );
	if ( pDir == NULL )
		pDir = &aDirectory[ 0 ];
	else
		pDir++;	//  Point just past the \.
	char aName[ MAX_PATH ];
	( void )strcpy( aName, pDir );

	//  Save the sprite.
	char *pSpriteFilter = "Sprite Files\0*.8;*.15;*.16;*.24;*.32\0\0\0";
	CFileDialog saveDialog( FALSE );
	saveDialog.m_ofn.lpstrFilter = pSpriteFilter;
	saveDialog.m_ofn.lpstrTitle = "Enter sprite name ( no extension )";
	saveDialog.m_ofn.lpstrFile = aName;
	saveDialog.m_ofn.lpstrInitialDir = aDirectory;
	if ( saveDialog.DoModal() == IDOK )
	{
		CString path;
		path = saveDialog.GetPathName();

		SetPathName( path );

		if ( CSprite::OnSaveDocument( ( const char * )path ) == FALSE )
			return FALSE;
	}
	else
		return FALSE;
	
	//  Set the current sprite to the 8-bit sprite
	pCurrentSprite = pSprite8;

	return TRUE;
}

BOOL CSprite::OnOpenDocument(LPCTSTR lpszPathName) 
{
	ASSERT( lpszPathName != NULL );
	ASSERT( pSprite8 != NULL );
#ifdef CREATE15
	ASSERT( pSprite15 != NULL );
#endif
#ifdef CREATE16
	ASSERT( pSprite16 != NULL );
#endif
	ASSERT( pSprite24 != NULL );
#ifdef CREATE32
	ASSERT( pSprite32 != NULL );
#endif

	//  Since we're not using serialization, this does
	//  very little, but we'll call it anyway.
	if ( CDocument::OnOpenDocument( lpszPathName ) == FALSE )
		return FALSE;

	//  Make a copy of the string so we can modify it.
	//  Pad it to make sure we have room for our extension.
	char *pPath = ( char * )malloc( strlen( lpszPathName ) + 4 );
	if ( pPath == NULL )
		return FALSE;
	( void )strcpy( pPath, lpszPathName );

	//  Figure out where in the string the extension goes.  Note that
	//  we need to make sure that we don't clobber an extension that
	//  actually belongs to a directory.
	char *pExt = strrchr( pPath, '.' );
	char *pDir = strrchr( pPath, '\\' );
	if ( pExt == NULL || pExt < pDir )
		pExt = &pPath[ strlen( pPath ) ];

	//  Load the 8-bit sprite.
	( void )strcpy( pExt, ".8" );
	if ( pSprite8->Load( pPath ) == FALSE )
		return FALSE;
	
#ifdef CREATE15
	//  Load the 15-bit sprite.
	( void )strcpy( pExt, ".15" );
	if ( pSprite15->Load( pPath ) == FALSE )
		return FALSE;
#endif
	
#ifdef CREATE16
	//  Load the 16-bit sprite.
	( void )strcpy( pExt, ".16" );
	if ( pSprite16->Load( pPath ) == FALSE )
		return FALSE;
#endif
	
	//  Load the 24-bit sprite.
	( void )strcpy( pExt, ".24" );
	if ( pSprite24->Load( pPath ) == FALSE )
		return FALSE;
	
#ifdef CREATE32
	//  Load the 32-bit sprite.
	( void )strcpy( pExt, ".32" );
	if ( pSprite32->Load( pPath ) == FALSE )
		return FALSE;
#endif
	
	//  Free the string
	free( pPath );

	return TRUE;
}

BOOL CSprite::OnSaveDocument(LPCTSTR lpszPathName) 
{
	ASSERT( lpszPathName != NULL );
	ASSERT( pSprite8 != NULL );
#ifdef CREATE15
	ASSERT( pSprite15 != NULL );
#endif
#ifdef CREATE16
	ASSERT( pSprite16 != NULL );
#endif
	ASSERT( pSprite24 != NULL );
#ifdef CREATE32
	ASSERT( pSprite32 != NULL );
#endif

	//  Since we're not using serialization, this does
	//  very little, but we'll call it anyway.
	if ( CDocument::OnSaveDocument( lpszPathName ) == FALSE )
		return FALSE;

	//  Make a copy of the string so we can stick in our
	//  own extension.  Make sure to pad the string so we
	//  know we have enough room.
	char *pPath = ( char * )malloc( strlen( lpszPathName ) + 4 );
	if ( pPath == NULL )
		return FALSE;
	( void )strcpy( pPath, lpszPathName );

	//  Figure out where in the string the extension goes.  Note that
	//  we need to make sure that we don't clobber an extension that
	//  actually belongs to a directory.
	char *pExt = strrchr( pPath, '.' );
	char *pDir = strrchr( pPath, '\\' );
	if ( pExt == NULL || pExt < pDir )
		pExt = &pPath[ strlen( pPath ) ];

	//  Save the 8-bit sprite.
	( void )strcpy( pExt, ".8" );
	if ( pSprite8->Save( pPath, FALSE, TRUE ) == FALSE )
		return FALSE;

#ifdef CREATE15
	//  Save the 15-bit sprite.
	( void )strcpy( pExt, ".15" );
	if ( pSprite15->Save( pPath ) == FALSE )
		return FALSE;
#endif

#ifdef CREATE16
	//  Save the 16-bit sprite.
	( void )strcpy( pExt, ".16" );
	if ( pSprite16->Save( pPath ) == FALSE )
		return FALSE;
#endif

	//  Save the 24-bit sprite.
	( void )strcpy( pExt, ".24" );
	if ( pSprite24->Save( pPath, FALSE, TRUE ) == FALSE )
		return FALSE;

#ifdef CREATE32
	//  Save the 32-bit sprite.
	( void )strcpy( pExt, ".32" );
	if ( pSprite32->Save( pPath ) == FALSE )
		return FALSE;
#endif

	//  Free the string
	free( pPath );

	return TRUE;
}

void CSprite::OnFileSavesprite() 
{
	if ( OnSaveDocument( GetPathName() ) == FALSE )
	{
		CString str;
		str.Format( "Error saving %s\n", GetPathName() );
		AfxMessageBox( str );
	}
}
