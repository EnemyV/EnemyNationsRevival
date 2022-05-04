// PalDoc.cpp : implementation file
//

#include <io.h>

#include "stdafx.h"

#include "boxsplit.h"
#include "bmpfile.h"
#include "cdirdlg.h"
#include "medcut.h"
#include "varcut.h"
#include "PalDoc.h"
#include "sprite.h"
#include "sptypes.h"
#include "terrain.h"
#include "tgafile.h"

IMPLEMENT_DYNCREATE(CPalDoc, CDocument)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPalDoc


CPalDoc::CPalDoc()
{
}

CPalDoc::~CPalDoc()
{
}


BEGIN_MESSAGE_MAP(CPalDoc, CDocument)
	//{{AFX_MSG_MAP(CPalDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPalDoc diagnostics

#ifdef _DEBUG
void CPalDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPalDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPalDoc serialization

void CPalDoc::Serialize(CArchive& ar)
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

/////////////////////////////////////////////////////////////////////////////
// CPalDoc commands

int CPalDoc::CountFiles( const char *pCurrentPath )
{
	int		cnt = 0;
	char	aDirectory[ MAX_PATH ];

	strcpy( aDirectory, pCurrentPath );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );

	char aPattern[ MAX_PATH ];
	strcpy( aPattern, aDirectory );
	strcat( aPattern, "*.*" );

	struct _finddata_t	findData;
	BOOL bAllFilesRead = FALSE;
	long hFile = _findfirst( aPattern, &findData );
	if ( hFile == -1L )
		bAllFilesRead = TRUE;
	while( bAllFilesRead == FALSE )
	{
		if ( findData.attrib & _A_SUBDIR )
		{
			//  Make sure we don't try to recursively search
			//  the current or parent directories.
			if ( _stricmp( findData.name, "." ) == 0 ||
				_stricmp( findData.name, ".." ) == 0 )
				goto FindNext;

			//  Search the subdirectory.
			char aSubDir[ MAX_PATH ];

			strcpy( aSubDir, aDirectory );
			strcat( aSubDir, findData.name );
			cnt += CountFiles( aSubDir );
		}
		else
		{
			//  See if it's a file type we recognize.
			char *pExt;
			pExt = strrchr( findData.name, '.' );
			if ( pExt != NULL &&
				( _stricmp( pExt, ".bmp" ) == 0 ||
				  _stricmp( pExt, ".dib" ) == 0 ||
				  _stricmp( pExt, ".tga" ) == 0 ) )
				cnt++;
		}

	FindNext :
		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
		return false;

	return cnt;
}

bool CPalDoc::ProcessFiles( const char *pCurrentPath, ColorQuant *pQuant, bool bTerrain )
{
	assert( pCurrentPath != NULL );
	assert( pQuant != NULL );

	//  Create a copy of the current path.  Make sure it ends in \*.*.
	char	aDirectory[ MAX_PATH ];
	strcpy( aDirectory, pCurrentPath );
	if ( aDirectory[ strlen( aDirectory ) - 1 ] != '\\' )
		strcat( aDirectory, "\\" );

	char aPattern[ MAX_PATH ];
	strcpy( aPattern, aDirectory );
	strcat( aPattern, "*.*" );

	struct _finddata_t	findData;
	BOOL bAllFilesRead = FALSE;
	long hFile = _findfirst( aPattern, &findData );
	if ( hFile == -1L )
		bAllFilesRead = TRUE;
	while( bAllFilesRead == FALSE )
	{
		if ( findData.attrib & _A_SUBDIR )
		{
			//  Make sure we don't try to recursively search
			//  the current or parent directories.
			if ( _stricmp( findData.name, "." ) == 0 ||
				_stricmp( findData.name, ".." ) == 0 )
				goto FindNext;

			//  Search the subdirectory.
			char aSubDir[ MAX_PATH ];

			strcpy( aSubDir, aDirectory );
			strcat( aSubDir, findData.name );

			//  Determine whether the files in this subdirectory
			//  are terrain files.  They will be terrain files if
			//    a)  The directory this is recursing from contains terrain files
			//    b)  The subdirectory name is "terrain"
			bool bSubdirTerrain = bTerrain;
			if ( _stricmp( findData.name, "terrain" ) == 0 )
				bSubdirTerrain = true;

			if ( ProcessFiles( aSubDir, pQuant, bSubdirTerrain ) == false )
				return false;
		}
		else
		{
			//  See if it's a file type we recognize.
			char *pExt;
			pExt = strrchr( findData.name, '.' );
			if ( pExt == NULL ||
				( _stricmp( pExt, ".bmp" ) != 0 &&
				  _stricmp( pExt, ".dib" ) != 0 &&
				  _stricmp( pExt, ".tga" ) != 0 ) )
				goto FindNext;

			//  Create the full filename.
			char aFullPath[ MAX_PATH ];
			strcpy( aFullPath, aDirectory );
			strcat( aFullPath, findData.name );

			//  Indicate what file is being processed.
			CString sStatus (aFullPath);
			if (sStatus.GetLength () > 35)
				sStatus = "..." + sStatus.Right (35);
			theApp.ChangeProgressName( sStatus );
			theApp.UpdateProgressDialog();
			if ( theApp.FlushMessageQueue() == FALSE )
				return false;

			//  Load the file.
			Image *pImage = NULL;
			if ( _stricmp( pExt, ".dib" ) == 0 ||
				_stricmp( pExt, ".bmp" ) == 0 )
			{
				BMPFile	file;
				pImage = file.Load( aFullPath );
			}
			else if ( _stricmp( pExt, ".tga" ) == 0 )
			{
				TGAFile file;
				pImage = file.Load( aFullPath );
			}

			if ( pImage == NULL )
			{
			char sBuf[300];
			sprintf (sBuf, "Error procesing file:\n%s", aFullPath);
			theApp.AddErrorDialog (sBuf);
			goto next_img;
			}

			//  If I'm processing terrain files, I need to munge the
			//  image some first.  The aspect ratio check is needed because
			//  the Square routine asserts if it isn't present;
			//  theoretically this wouldn't be a problem because all the
			//  art would be correct, but...
			if ( bTerrain == true &&
				pImage->_width == pImage->_height * 2 )
			{
				//  This is not a good practice in general!  This will make
				//  changes to the original image.
				TerrainImage *pTerrainImage = ( TerrainImage * )pImage;
			
				if (! pTerrainImage->Square(aFullPath))
					goto next_img;

				if ( pTerrainImage->Shade() == false )
					{
					char sBuf[300];
					sprintf (sBuf, "Error procesing file:\n%s", aFullPath);
					theApp.AddErrorDialog (sBuf);
					goto next_img;
					}
			}

			//  Add the image to the quantizer histogram.
			if ( pQuant->AddImage( pImage ) == false )
				{
				char sBuf[300];
				sprintf (sBuf, "Error procesing file:\n%s", aFullPath);
				theApp.AddErrorDialog (sBuf);
				}

			//  Delete the image, since we're done with it now.
next_img:
			delete pImage;
		}

	FindNext :
		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	return true;
}

BOOL CPalDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	//  Load the palette.
	BMPFile file;
	RGBColor *pPalette = file.LoadPalette( lpszPathName );
	if ( pPalette == NULL )
		return FALSE;

	if ( theApp.SetPalette( pPalette ) == false )
		return FALSE;

	delete[] pPalette;	//  No longer needed.

	return TRUE;
}

BOOL CPalDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	cdirdlg		dirDlg (theApp.m_pMainWnd);
	if ( dirDlg.DoModal() != IDOK )
		return FALSE;

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

	//  Count the number of files which will ultimately be processed,
	//  so we can put up a progress dialog.
	int totalFiles = CountFiles( aDirectory );
	if ( totalFiles == 0 )
	{
		theApp.AddErrorDialog ("No files found to process!" );
		return FALSE;
	}

	//  Create the progress dialog.  We create the dialog with one more
	//  entry than we have files, since after we process all of the files
	//  we still have the step of creating the palette.
	theApp.CreateErrorDialog();
	if ( theApp.CreateProgressDialog( totalFiles + 1, 1 ) == FALSE )
		return FALSE;

	//  Choose a quantizer.
	ColorQuant *pQuant = NULL;
	CString sTitle ("Sprite - [");
	int quantizer = GetPrivateProfileInt( "sprite", "quantizer", 2, theApp.fullPathIniFile );
	switch ( quantizer )
	{
		case 0 :
			pQuant = new MedianCutQuant;
			sTitle += "Median Cut";
			if ( pQuant == NULL )
				return FALSE;
			break;

		case 1 :
			pQuant = new BoxSplitQuant;
			sTitle += "Box Cut";
			if ( pQuant == NULL )
				return FALSE;
			break;

		default :
		{
			CString str;
			str.Format( "Unknown quantizer %d - using VarQuant\n", quantizer );
			OutputDebugString( str );
			//  Fall through on purpose.
		}

		case 2 :
			pQuant = new VarSplitQuant;
			sTitle += "Variance Cut";
			if ( pQuant == NULL )
				return FALSE;
			break;
	}

	sTitle += "]";
	theApp.m_pMainWnd->SetWindowText( sTitle );
	theApp.AddErrorDialog ( sTitle );

	//  Set up the palette and the quantizer.
	RGBQUAD aPal[ 258 ];
	int palSize = 256;
	memset( aPal, 0, sizeof( aPal ) );
	aPal[ 0 ].rgbReserved = PC_RESERVED;
	aPal[ 1 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 2 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 3 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 4 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 5 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 6 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 7 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 8 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 9 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 246 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 247 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 248 ].rgbReserved = PC_NOCOLLAPSE;
	aPal[ 249 ].rgbRed = 255;
	aPal[ 249 ].rgbReserved = PC_RESERVED;
	aPal[ 250 ].rgbGreen = 255;
	aPal[ 250 ].rgbReserved = PC_RESERVED;
	aPal[ 251 ].rgbRed = 255;
	aPal[ 251 ].rgbGreen = 255;
	aPal[ 251 ].rgbReserved = PC_RESERVED;
	aPal[ 252 ].rgbBlue = 255;
	aPal[ 252 ].rgbReserved = PC_RESERVED;
	aPal[ 253 ].rgbRed = 255;
	aPal[ 253 ].rgbBlue = 255;
	aPal[ 253 ].rgbReserved = PC_RESERVED;
	aPal[ 254 ].rgbGreen = 255;
	aPal[ 254 ].rgbBlue = 255;
	aPal[ 254 ].rgbReserved = PC_RESERVED;
	aPal[ 255 ].rgbRed = 255;
	aPal[ 255 ].rgbGreen = 255;
	aPal[ 255 ].rgbBlue = 255;
	aPal[ 255 ].rgbReserved = PC_RESERVED;
	assert( pQuant );
	if ( pQuant->Init( aPal, palSize ) == false )
	{
		theApp.AddErrorDialog ("Error initializing quantizer" );
		delete pQuant;
		return FALSE;
	}

	//  Process the files.
	bool bTerrain = false;
	if ( _strnicmp( &dirDlg.aCurrentDirSel[ 1 ], "terrain", 7 ) == 0 )
		bTerrain = true;
	if ( ProcessFiles( aDirectory, pQuant, bTerrain ) == FALSE )
	{
		theApp.DestroyProgressDialog();
		theApp.AddErrorDialog("Error processing files!" );
		delete pQuant;
		return FALSE;
	}

	//  Update the progress dialog.
	theApp.ChangeProgressName( "Quantizing..." );
	theApp.UpdateProgressDialog();
	if ( theApp.FlushMessageQueue() == FALSE )
	{
		delete pQuant;
		return FALSE;
	}

	//  Create the palette.
	if ( pQuant->CreatePalette( aPal, &palSize ) == false )
	{
		theApp.DestroyProgressDialog();
		theApp.AddErrorDialog("Error creating palette" );
		delete pQuant;
		return FALSE;
	}

	//  Make the dialog go away.  We never bother to update the
	//  last position on the dialog;  we just want it there so it's
	//  clear that there's one more step.
	theApp.DestroyProgressDialog();

	//  Save the palette.
	CFileDialog saveDialog( FALSE );
	saveDialog.m_ofn.lpstrFilter = "Palette Files\0*.bmp;*.dib\0\0\0";
	saveDialog.m_ofn.lpstrTitle = "Save palette as...";
	if ( saveDialog.DoModal() != IDOK )
	{
		delete pQuant;
		return FALSE;
	}

	char aFilename[ MAX_PATH ];
	( void )strcpy( aFilename, saveDialog.GetPathName() );
	char *pExt = strrchr( aFilename, '.' );
	char *pDir = strrchr( aFilename, '\\' );
	if ( pExt == NULL || pExt < pDir )
		strcat( aFilename, ".dib" );

	RGBColor aRGBColorPal[ 256 ];
	int palIndex;
	for( palIndex = 0; palIndex < 256; palIndex++ )
	{
		aRGBColorPal[ palIndex ].Set( 
			aPal[ palIndex ].rgbRed,
			aPal[ palIndex ].rgbGreen,
			aPal[ palIndex ].rgbBlue );
	}
	BMPFile file;
	if ( file.SavePalette( aRGBColorPal, aFilename ) == false )
	{
		theApp.AddErrorDialog ("Error saving palette" );
		delete pQuant;
		return FALSE;
	}

	delete pQuant;

	//  Tell the app that there's a new palette.
	theApp.SetPalette( aRGBColorPal );

	//  Set the name of the document.
	SetTitle( ( const char * )saveDialog.GetFileTitle() );
	
	return TRUE;
}
