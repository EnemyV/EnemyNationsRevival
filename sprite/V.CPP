#include "stdafx.h"
#include "sprite.h"

#include <io.h>

#include "bmpfile.h"
#include "tgafile.h"
#include "v.h"

extern CSpriteApp theApp;
extern BOOL IsTrans ( BYTE red, BYTE green, BYTE blue );

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char *View::zoomNames[ MAX_ZOOM ] =
{
	"Zoom 0",
	"Zoom 1",
	"Zoom 2",
	"Zoom 3"
};

View::View()
{
	baseCnt = 0;
	overlayCnt = 0;
	anim1Cnt = 0;
	anim2Cnt = 0;
	anim1Step = 0;
	anim2Step = 0;
	bBorrowedBase = false;
	bBorrowedAnim1 = false;
	bBorrowedAnim2 = false;
	bBorrowedUnderAnim1 = false;
	bBorrowedUnderAnim2 = false;
	underAnim1Cnt = 0;
	underAnim2Cnt = 0;
	underAnim1Step = 0;
	underAnim2Step = 0;
	anim1Frames = 0;
	anim2Frames = 0;
	underAnim1Frames = 0;
	underAnim2Frames = 0;
	width = 0;
	height = 0;
	tviCnt = 0;
	tviAllocCnt = 0;
	pptvi = NULL;
	_pSprite = NULL;
	bAnchorPointIsValid = FALSE;
	nAttachedSpriteCnt = 0;
	( void )memset( &anchorPoint, 0, sizeof( anchorPoint ) );
	( void )memset( &aAttachedSprites, 0, sizeof( aAttachedSprites ) );

	memset( &apBases, 0, sizeof( apBases ) );
	memset( &apOverlay, 0, sizeof( apOverlay ) );
	memset( &apAnim1, 0, sizeof( apAnim1 ) );
	memset( &apAnim2, 0, sizeof( apAnim2 ) );
	memset( &apUnderAnim1, 0, sizeof( apUnderAnim1 ) );
	memset( &apUnderAnim2, 0, sizeof( apUnderAnim2 ) );
}

View::~View()
{
	int fbIndex;

	//  Delete the base bitmaps, unless they were borrowed from some other view.
	if ( bBorrowedBase == false )
	{
		for	( fbIndex = 0; fbIndex < baseCnt; fbIndex++ )
		{
			delete apBases[ fbIndex ][ 0 ];
			delete apBases[ fbIndex ][ 1 ];
			delete apBases[ fbIndex ][ 2 ];
			delete apBases[ fbIndex ][ 3 ];
		}
	}

	//  Delete overlay bitmaps.  Since these can't be borrowed, there's no check.
	for	( fbIndex = 0; fbIndex < overlayCnt; fbIndex++ )
	{
		delete apOverlay[ fbIndex ][ 0 ];
		delete apOverlay[ fbIndex ][ 1 ];
		delete apOverlay[ fbIndex ][ 2 ];
		delete apOverlay[ fbIndex ][ 3 ];
	}

	//  Delete anim1 bitmaps, unless they were borrowed from some other view.
	if ( bBorrowedAnim1 == false )
	{
		for	( fbIndex = 0; fbIndex < anim1Cnt; fbIndex++ )
		{
			delete apAnim1[ fbIndex ][ 0 ];
			delete apAnim1[ fbIndex ][ 1 ];
			delete apAnim1[ fbIndex ][ 2 ];
			delete apAnim1[ fbIndex ][ 3 ];
		}
	}

	//  Delete anim2 bitmaps, unless they were borrowed from some other view.
	if ( bBorrowedAnim2 == false )
	{
		for	( fbIndex = 0; fbIndex < anim2Cnt; fbIndex++ )
		{
			delete apAnim2[ fbIndex ][ 0 ];
			delete apAnim2[ fbIndex ][ 1 ];
			delete apAnim2[ fbIndex ][ 2 ];
			delete apAnim2[ fbIndex ][ 3 ];
		}
	}

	//  Delete under anim1 bitmaps, unless they were borrowed from some other view.
	if ( bBorrowedUnderAnim1 == false )
	{
		for	( fbIndex = 0; fbIndex < underAnim1Cnt; fbIndex++ )
		{
			delete apUnderAnim1[ fbIndex ][ 0 ];
			delete apUnderAnim1[ fbIndex ][ 1 ];
			delete apUnderAnim1[ fbIndex ][ 2 ];
			delete apUnderAnim1[ fbIndex ][ 3 ];
		}
	}

	//  Delete under anim2 bitmaps, unless they were borrowed from some other view.
	if ( bBorrowedUnderAnim2 == false )
	{
		for	( fbIndex = 0; fbIndex < underAnim2Cnt; fbIndex++ )
		{
			delete apUnderAnim2[ fbIndex ][ 0 ];
			delete apUnderAnim2[ fbIndex ][ 1 ];
			delete apUnderAnim2[ fbIndex ][ 2 ];
			delete apUnderAnim2[ fbIndex ][ 3 ];
		}
	}

	//  Delete TREEVIEWINFO structures.
	int tviIndex;
	for ( tviIndex = 0; tviIndex < tviCnt; tviIndex++ )
		delete pptvi[ tviIndex ];

	delete[] pptvi;
}

	//  Get a pointer to one of the images in this view.
Image *View::GetImage( int index, int zoom )
{
	assert( 0 <= index && index < underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt + anim1Cnt + anim2Cnt );
	assert( 0 <= zoom && zoom < MAX_ZOOM );

	if ( underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt + anim1Cnt <= index )
		//  It's an anim2 bitmap.
		return apAnim2[ index - anim1Cnt - overlayCnt - baseCnt - underAnim2Cnt - underAnim1Cnt ][ zoom ];
	else if ( underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt <= index )
		//  It's an anim1 bitmap.
		return apAnim1[ index - overlayCnt - baseCnt - underAnim2Cnt - underAnim1Cnt ][ zoom ];
	else if ( underAnim1Cnt + underAnim2Cnt + baseCnt <= index )
		//  It's an overlay bitmap
		return apOverlay[ index - baseCnt - underAnim2Cnt - underAnim1Cnt ][ zoom ];
	else if ( underAnim1Cnt + underAnim2Cnt <= index )
		//  It's a base bitmap
		return apBases[ index - underAnim2Cnt - underAnim1Cnt ][ zoom ];
	else if ( underAnim1Cnt <= index )
		//  It's an under anim2 bitmap
		return apUnderAnim2[ index - underAnim1Cnt ][ zoom ];

	//  It's an under anim1 bitmap.
	return apUnderAnim1[ index ][ zoom ];
}

TREEVIEWINFO *View::GetNewTreeView( void )
{
	if ( tviAllocCnt <= tviCnt )
	{
		if ( tviAllocCnt == 0 )
			tviAllocCnt = 25;

		tviAllocCnt = tviAllocCnt * 2;
		TREEVIEWINFO **ppNewtvi = new TREEVIEWINFO * [ tviAllocCnt ];
		if ( ppNewtvi == NULL )
			return NULL;

		int tviIndex;
		for( tviIndex = 0; tviIndex < tviCnt; tviIndex++ )
			ppNewtvi[ tviIndex ] = pptvi[ tviIndex ];

		delete[] pptvi;
		pptvi = ppNewtvi;
	}

	TREEVIEWINFO *ptvi = new TREEVIEWINFO;
	if ( ptvi == NULL )
		return NULL;

	pptvi[ tviCnt++ ] = ptvi;

	return ptvi;
}

int __cdecl QsortStrcmp( const void *pStr0, const void *pStr1 )
{
	return strcmp( ( const char * )pStr0, ( const char * )pStr1 );
}

BOOL GetFiles( const char *pPattern, int bufEntries, int bufLen, char *pBuf, int *pFilesFound )
{
	ASSERT( 0 < bufLen );
	ASSERT( 0 < bufEntries );
	ASSERT( pBuf );
	ASSERT( pFilesFound );

	int		fileCnt = 0;
	BOOL	bAllFilesRead = FALSE;
	long	hFile;
	struct _finddata_t	findData;

	//  Find all files matching the given pattern, and bitmap.
	hFile = _findfirst( ( char * )pPattern, &findData );
	if ( hFile == -1L )
		bAllFilesRead = TRUE;
	while( bAllFilesRead == FALSE )
	{
		if ( bufLen <= ( int )strlen( findData.name ) )
			return FALSE;

		if ( bufEntries <= fileCnt )
			return FALSE;

		( void )strcpy( pBuf + fileCnt * bufLen, findData.name );
		( void )strupr( pBuf + fileCnt * bufLen );
		
		fileCnt++;

		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	*pFilesFound = fileCnt;

	//  Now sort the array.
	qsort( pBuf, *pFilesFound, bufLen, QsortStrcmp );

	return TRUE;
}

BOOL View::CreateView( Sprite *pSprite, const char *pDir, const char *pPattern, int bitDepth )
{
	ASSERT( pSprite != NULL );
	ASSERT( pDir != NULL );
	ASSERT( pPattern != NULL );
	ASSERT( bitDepth == 8 || bitDepth == 15 || bitDepth == 16 || bitDepth == 24 || bitDepth == 32 );

	char	aFileList[ MAX_ANIM * MAX_PATH ];
	char	aPattern[ MAX_PATH ];
	char	aFullPattern[ MAX_PATH ];
	int		fileIndex;

	//  Save the sprite pointer and the bit depth.
	_pSprite = pSprite;
	_bitDepth = bitDepth;

	//  Copy the pattern into an editable buffer.
	strcpy( aPattern, pPattern );

	//  Adjust the pattern to search for an overlay bitmap.
	aPattern[ 3 ] = '\0';
	strcat( aPattern, "00000.*" );
	strcpy( aFullPattern, pDir );
	if ( aFullPattern[ strlen( aFullPattern ) - 1 ] != '\\' )
		strcat( aFullPattern, "\\" );
	strcat( aFullPattern, aPattern );

	//  Get an alphabetical list of all of the overlay files.
	if ( GetFiles( aFullPattern, MAX_ANIM, MAX_PATH, aFileList, &overlayCnt ) == FALSE )
		return FALSE;
	ASSERT( overlayCnt <= MAX_OVERLAY );

	for( fileIndex = 0; fileIndex < overlayCnt; fileIndex++ )
	{
		char aFullPath[ MAX_PATH ];

		//  Update the progress dialog.
		theApp.ChangeProgressName( &aFileList[ fileIndex * MAX_PATH ] );

		//  Create the full filename.
		strcpy( aFullPath, pDir );
		if ( aFullPath[ strlen( aFullPath ) - 1 ] != '\\' )
			strcat( aFullPath, "\\" );
		strcat( aFullPath, &aFileList[ fileIndex * MAX_PATH ] );

		//  Load the file.
		if ( LoadFile( aFullPath, fileIndex, 3 ) == FALSE )
		{
			CString str;
			str.Format( "Load of file %s failed!", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Flush the message queue.
		if ( theApp.FlushMessageQueue() == FALSE )
			return FALSE;
	}

	//  Adjust the pattern to search for base bitmaps.
	aPattern[ 3 ] = '\0';
	strcat( aPattern, "?0000.*" );
	strcpy( aFullPattern, pDir );
	if ( aFullPattern[ strlen( aFullPattern ) - 1 ] != '\\' )
		strcat( aFullPattern, "\\" );
	strcat( aFullPattern, aPattern );

	//  Get an alphabetical list of all of the base bitmaps.
	if ( GetFiles( aFullPattern, MAX_ANIM, MAX_PATH, aFileList, &baseCnt ) == FALSE )
		return FALSE;
	ASSERT( baseCnt < MAX_BASE );

	BOOL bMatchedOverlay = FALSE;
	for( fileIndex = 0; fileIndex < baseCnt; fileIndex++ )
	{
		char aFullPath[ MAX_PATH ];

		//  If the match is on '0', ignore this file.
		if ( aFileList[ fileIndex * MAX_PATH + 3 ] == '0' )
		{
			bMatchedOverlay = TRUE;
			continue;
		}

		//  If the match isn't valid ( '1'-'9' ), this is
		//  an error.
		if ( aFileList[ fileIndex * MAX_PATH + 3 ] < '1' || '9' < aFileList[ fileIndex * MAX_PATH + 3 ] )
		{
			CString str;
			str.Format( "Error : %s is an invalid filename - fourth char must be between 1 and 9", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Update the progress dialog.
		theApp.ChangeProgressName( &aFileList[ fileIndex * MAX_PATH ] );

		//  Create the full filename.
		strcpy( aFullPath, pDir );
		if ( aFullPath[ strlen( aFullPath ) - 1 ] != '\\' )
			strcat( aFullPath, "\\" );
		strcat( aFullPath, &aFileList[ fileIndex * MAX_PATH ] );

		//  Load the file.
		if ( LoadFile( aFullPath, fileIndex, 0 ) == FALSE )
		{
			CString str;
			str.Format( "Load of file %s failed!", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Flush the message queue.
		if ( theApp.FlushMessageQueue() == FALSE )
			return FALSE;
	}
	if ( bMatchedOverlay )
		baseCnt--;

	//  Adjust the pattern to find all anim1 bitmaps.
	aPattern[ 3 ] = '\0';
	strcat( aPattern, "0?000.*" );
	strcpy( aFullPattern, pDir );
	if ( aFullPattern[ strlen( aFullPattern ) - 1 ] != '\\' )
		strcat( aFullPattern, "\\" );
	strcat( aFullPattern, aPattern );

	//  Get an alphabetical list of all of the anim1 bitmaps.
	if ( GetFiles( aFullPattern, MAX_ANIM, MAX_PATH, aFileList, &anim1Cnt ) == FALSE )
		return FALSE;
	ASSERT( anim1Cnt < MAX_ANIM );

	bMatchedOverlay = FALSE;
	for( fileIndex = 0; fileIndex < anim1Cnt; fileIndex++ )
	{
		char aFullPath[ MAX_PATH ];

		//  If the match is on '0', ignore this file.
		if ( aFileList[ fileIndex * MAX_PATH + 4 ] == '0' )
		{
			bMatchedOverlay = TRUE;
			continue;
		}

		//  If the match isn't valid ( 'A'-'Z' ), this is
		//  an error.
		if ( aFileList[ fileIndex * MAX_PATH + 4 ] < 'A' || 'Z' < aFileList[ fileIndex * MAX_PATH + 4 ] )
		{
			CString str;
			str.Format( "Error : %s is an invalid filename - fifth char must be between A and Z.", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Update the progress dialog.
		theApp.ChangeProgressName( &aFileList[ fileIndex * MAX_PATH ] );

		//  Create the full filename.
		strcpy( aFullPath, pDir );
		if ( aFullPath[ strlen( aFullPath ) - 1 ] != '\\' )
			strcat( aFullPath, "\\" );
		strcat( aFullPath, &aFileList[ fileIndex * MAX_PATH ] );

		//  Load the file.
		if ( LoadFile( aFullPath, fileIndex, 1 ) == FALSE )
		{
			CString str;
			str.Format( "Load of file %s failed!", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Flush the message queue.
		if ( theApp.FlushMessageQueue() == FALSE )
			return FALSE;
	}
	if ( bMatchedOverlay )
		anim1Cnt--;

	//  Adjust the pattern to find all anim2 bitmaps.
	aPattern[ 3 ] = '\0';
	strcat( aPattern, "00?00.*" );
	strcpy( aFullPattern, pDir );
	if ( aFullPattern[ strlen( aFullPattern ) - 1 ] != '\\' )
		strcat( aFullPattern, "\\" );
	strcat( aFullPattern, aPattern );

	//  Get an alphabetical list of all of the anim2 bitmaps.
	if ( GetFiles( aFullPattern, MAX_ANIM, MAX_PATH, aFileList, &anim2Cnt ) == FALSE )
		return FALSE;
	ASSERT( anim2Cnt < MAX_ANIM );

	bMatchedOverlay = FALSE;
	for( fileIndex = 0; fileIndex < anim2Cnt; fileIndex++ )
	{
		char aFullPath[ MAX_PATH ];

		//  If the match is on '0', ignore this file.
		if ( aFileList[ fileIndex * MAX_PATH + 5 ] == '0' )
		{
			bMatchedOverlay = TRUE;
			continue;
		}

		//  If the match isn't valid ( 'A'-'Z' ), this is
		//  an error.
		if ( aFileList[ fileIndex * MAX_PATH + 5 ] < 'A' || 'Z' < aFileList[ fileIndex * MAX_PATH + 5 ] )
		{
			CString str;
			str.Format( "Error : %s is an invalid filename - sixth char must be between A and Z.", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Update the progress dialog.
		theApp.ChangeProgressName( &aFileList[ fileIndex * MAX_PATH ] );

		//  Create the full filename.
		strcpy( aFullPath, pDir );
		if ( aFullPath[ strlen( aFullPath ) - 1 ] != '\\' )
			strcat( aFullPath, "\\" );
		strcat( aFullPath, &aFileList[ fileIndex * MAX_PATH ] );

		//  Load the file.
		if ( LoadFile( aFullPath, fileIndex, 2 ) == FALSE )
		{
			CString str;
			str.Format( "Load of file %s failed!", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Flush the message queue.
		if ( theApp.FlushMessageQueue() == FALSE )
			return FALSE;
	}
	if ( bMatchedOverlay )
		anim2Cnt--;

	//  Adjust the pattern to find all under anim1 bitmaps.
	aPattern[ 3 ] = '\0';
	strcat( aPattern, "000?0.*" );
	strcpy( aFullPattern, pDir );
	if ( aFullPattern[ strlen( aFullPattern ) - 1 ] != '\\' )
		strcat( aFullPattern, "\\" );
	strcat( aFullPattern, aPattern );

	//  Get an alphabetical list of all of the under anim1 bitmaps.
	if ( GetFiles( aFullPattern, MAX_ANIM, MAX_PATH, aFileList, &underAnim1Cnt ) == FALSE )
		return FALSE;
	ASSERT( underAnim1Cnt < MAX_ANIM );

	bMatchedOverlay = FALSE;
	for( fileIndex = 0; fileIndex < underAnim1Cnt; fileIndex++ )
	{
		char aFullPath[ MAX_PATH ];

		//  If the match is on '0', ignore this file.
		if ( aFileList[ fileIndex * MAX_PATH + 6 ] == '0' )
		{
			bMatchedOverlay = TRUE;
			continue;
		}

		//  If the match isn't valid ( 'A'-'Z' ), this is
		//  an error.
		if ( aFileList[ fileIndex * MAX_PATH + 6 ] < 'A' || 'Z' < aFileList[ fileIndex * MAX_PATH + 6 ] )
		{
			CString str;
			str.Format( "Error : %s is an invalid filename - seventh char must be between A and Z.", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Update the progress dialog.
		theApp.ChangeProgressName( &aFileList[ fileIndex * MAX_PATH ] );

		//  Create the full filename.
		strcpy( aFullPath, pDir );
		if ( aFullPath[ strlen( aFullPath ) - 1 ] != '\\' )
			strcat( aFullPath, "\\" );
		strcat( aFullPath, &aFileList[ fileIndex * MAX_PATH ] );

		//  Load the file.
		if ( LoadFile( aFullPath, fileIndex, 4 ) == FALSE )
		{
			CString str;
			str.Format( "Load of file %s failed!", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Flush the message queue.
		if ( theApp.FlushMessageQueue() == FALSE )
			return FALSE;
	}
	if ( bMatchedOverlay )
		underAnim1Cnt--;

	//  Adjust the pattern to find all under anim2 bitmaps.
	aPattern[ 3 ] = '\0';
	strcat( aPattern, "0000?.*" );
	strcpy( aFullPattern, pDir );
	if ( aFullPattern[ strlen( aFullPattern ) - 1 ] != '\\' )
		strcat( aFullPattern, "\\" );
	strcat( aFullPattern, aPattern );

	//  Get an alphabetical list of all of the under anim2 bitmaps.
	if ( GetFiles( aFullPattern, MAX_ANIM, MAX_PATH, aFileList, &underAnim2Cnt ) == FALSE )
		return FALSE;
	ASSERT( underAnim2Cnt < MAX_ANIM );
	
	bMatchedOverlay = FALSE;
	for( fileIndex = 0; fileIndex < underAnim2Cnt; fileIndex++ )
	{
		char aFullPath[ MAX_PATH ];

		//  If the match is on '0', ignore this file.
		if ( aFileList[ fileIndex * MAX_PATH + 7 ] == '0' )
		{
			bMatchedOverlay = TRUE;
			continue;
		}

		//  If the match isn't valid ( 'A'-'Z' ), this is
		//  an error.
		if ( aFileList[ fileIndex * MAX_PATH + 7 ] < 'A' || 'Z' < aFileList[ fileIndex * MAX_PATH + 7 ] )
		{
			CString str;
			str.Format( "Error : %s is an invalid filename - seventh char must be between A and Z.", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Update the progress dialog.
		theApp.ChangeProgressName( &aFileList[ fileIndex * MAX_PATH ] );

		//  Create the full filename.
		strcpy( aFullPath, pDir );
		if ( aFullPath[ strlen( aFullPath ) - 1 ] != '\\' )
			strcat( aFullPath, "\\" );
		strcat( aFullPath, &aFileList[ fileIndex * MAX_PATH ] );

		//  Load the file.
		if ( LoadFile( aFullPath, fileIndex, 5 ) == FALSE )
		{
			CString str;
			str.Format( "Load of file %s failed!", &aFileList[ fileIndex * MAX_PATH ] );
			AfxMessageBox( str );
			SetLastError( USER_ERROR_BAD_FILE );
			return FALSE;
		}

		//  Flush the message queue.
		if ( theApp.FlushMessageQueue() == FALSE )
			return FALSE;
	}
	if ( bMatchedOverlay )
		underAnim2Cnt--;

	//  If no files were read, there's no view.
	if ( baseCnt + overlayCnt + anim1Cnt + anim2Cnt + underAnim1Cnt + underAnim2Cnt == 0 )
	{
		SetLastError( USER_ERROR_NO_FILES );
		return FALSE;
	}

	return TRUE;
}

//  Copies all elements from pCopyView that pView doesn't have.
//  In other words, if pView has no anim1 bitmaps, copy the ones
//  from pCopyView, but if pView has it's own anim1 bitmaps ignore
//  the ones in pCopyView.
//  Does NOT copy overlays - they are unique to a view.
void View::BorrowFromView( const View *pCopyView, BOOL bCopyAnims )
{
	if ( pCopyView == NULL )
		return;

	if ( baseCnt == 0 )
	{
		bBorrowedBase = true;
		
		baseCnt = pCopyView->baseCnt;
		int baseIndex;
		for( baseIndex = 0; baseIndex < baseCnt; baseIndex++ )
		{
			apBases[ baseIndex ][ 0 ] = pCopyView->apBases[ baseIndex ][ 0 ];
			apBases[ baseIndex ][ 1 ] = pCopyView->apBases[ baseIndex ][ 1 ];
			apBases[ baseIndex ][ 2 ] = pCopyView->apBases[ baseIndex ][ 2 ];
			apBases[ baseIndex ][ 3 ] = pCopyView->apBases[ baseIndex ][ 3 ];

			GetHotSpots( apBases[ baseIndex ][ 0 ] );	// GG
		}
	}

	// GG
	if ( ! bCopyAnims )
		return;

	if ( anim1Cnt == 0 )
	{
		bBorrowedAnim1 = true;

		anim1Cnt = pCopyView->anim1Cnt;
		int anim1Index;
		for( anim1Index = 0; anim1Index < anim1Cnt; anim1Index++ )
		{
			apAnim1[ anim1Index ][ 0 ] = pCopyView->apAnim1[ anim1Index ][ 0 ];
			apAnim1[ anim1Index ][ 1 ] = pCopyView->apAnim1[ anim1Index ][ 1 ];
			apAnim1[ anim1Index ][ 2 ] = pCopyView->apAnim1[ anim1Index ][ 2 ];
			apAnim1[ anim1Index ][ 3 ] = pCopyView->apAnim1[ anim1Index ][ 3 ];

			GetHotSpots( apAnim1[ anim1Index ][ 0 ] );	// GG
		}
	}

	if ( anim2Cnt == 0 )
	{
		bBorrowedAnim2 = true;

		anim2Cnt = pCopyView->anim2Cnt;
		int anim2Index;
		for( anim2Index = 0; anim2Index < anim2Cnt; anim2Index++ )
		{
			apAnim2[ anim2Index ][ 0 ] = pCopyView->apAnim2[ anim2Index ][ 0 ];
			apAnim2[ anim2Index ][ 1 ] = pCopyView->apAnim2[ anim2Index ][ 1 ];
			apAnim2[ anim2Index ][ 2 ] = pCopyView->apAnim2[ anim2Index ][ 2 ];
			apAnim2[ anim2Index ][ 3 ] = pCopyView->apAnim2[ anim2Index ][ 3 ];

			GetHotSpots( apAnim2[ anim2Index ][ 0 ] );	// GG
		}
	}

	if ( underAnim1Cnt == 0 )
	{
		bBorrowedUnderAnim1 = true;

		underAnim1Cnt = pCopyView->underAnim1Cnt;
		int underAnim1Index;
		for( underAnim1Index = 0; underAnim1Index < underAnim1Cnt; underAnim1Index++ )
		{
			apUnderAnim1[ underAnim1Index ][ 0 ] = pCopyView->apUnderAnim1[ underAnim1Index ][ 0 ];
			apUnderAnim1[ underAnim1Index ][ 1 ] = pCopyView->apUnderAnim1[ underAnim1Index ][ 1 ];
			apUnderAnim1[ underAnim1Index ][ 2 ] = pCopyView->apUnderAnim1[ underAnim1Index ][ 2 ];
			apUnderAnim1[ underAnim1Index ][ 3 ] = pCopyView->apUnderAnim1[ underAnim1Index ][ 3 ];

			GetHotSpots( apUnderAnim1[ underAnim1Index ][ 0 ] );	// GG
		}
	}

	if ( underAnim2Cnt == 0 )
	{
		bBorrowedUnderAnim2 = true;

		underAnim2Cnt = pCopyView->underAnim2Cnt;
		int underAnim2Index;
		for( underAnim2Index = 0; underAnim2Index < underAnim2Cnt; underAnim2Index++ )
		{
			apUnderAnim2[ underAnim2Index ][ 0 ] = pCopyView->apUnderAnim2[ underAnim2Index ][ 0 ];
			apUnderAnim2[ underAnim2Index ][ 1 ] = pCopyView->apUnderAnim2[ underAnim2Index ][ 1 ];
			apUnderAnim2[ underAnim2Index ][ 2 ] = pCopyView->apUnderAnim2[ underAnim2Index ][ 2 ];
			apUnderAnim2[ underAnim2Index ][ 3 ] = pCopyView->apUnderAnim2[ underAnim2Index ][ 3 ];

			GetHotSpots( apUnderAnim2[ underAnim2Index ][ 0 ] );	// GG
		}
	}
}

BOOL View::AddToTree( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView, void *pUserData )
{
	int 			zoomIndex;
	TREEVIEWINFO	*ptvi;
	HTREEITEM		hZoom;

	ASSERT( pTreeCtrl != NULL );

	for( zoomIndex = 0; zoomIndex < MAX_ZOOM; zoomIndex++ )
	{
		ptvi = GetNewTreeView();
		if ( ptvi == NULL )
			return FALSE;

		ptvi->pView = this;
		ptvi->index = -1;
		ptvi->zoom = zoomIndex;
		ptvi->pUser = pUserData;
		hZoom = pTreeCtrl->AddNewItem( hParent, zoomNames[ zoomIndex ], ptvi, ( HTREEITEM )TVI_LAST ); 

		if ( bSelectThisView && zoomIndex == 0 )
		{
			pTreeCtrl->SelectItem( hZoom );
			pTreeCtrl->EnsureVisible( hZoom );
		}

		int fbIndex;
		for( fbIndex = 0; fbIndex < underAnim1Cnt; fbIndex++ )
		{
			ptvi = GetNewTreeView();
			if ( ptvi == NULL )
				return FALSE;

			ptvi->pView = this;
			ptvi->index = fbIndex;
			ptvi->zoom = zoomIndex;
			ptvi->pUser = NULL;
			pTreeCtrl->AddNewItem( hZoom, "under anim1 bitmap", ptvi, ( HTREEITEM )TVI_LAST ); 
		}

		for( fbIndex = 0; fbIndex < underAnim2Cnt; fbIndex++ )
		{
			ptvi = GetNewTreeView();
			if ( ptvi == NULL )
				return FALSE;

			ptvi->pView = this;
			ptvi->index = underAnim1Cnt + fbIndex;
			ptvi->zoom = zoomIndex;
			ptvi->pUser = NULL;
			pTreeCtrl->AddNewItem( hZoom, "under anim2 bitmap", ptvi, ( HTREEITEM )TVI_LAST ); 
		}

		for( fbIndex = 0; fbIndex < baseCnt; fbIndex++ )
		{
			ptvi = GetNewTreeView();
			if ( ptvi == NULL )
				return FALSE;

			ptvi->pView = this;
			ptvi->index = underAnim1Cnt + underAnim2Cnt + fbIndex;
			ptvi->zoom = zoomIndex;
			ptvi->pUser = NULL;
			pTreeCtrl->AddNewItem( hZoom, "base bitmap", ptvi, ( HTREEITEM )TVI_LAST ); 
		}

		for( fbIndex = 0; fbIndex < overlayCnt; fbIndex++ )
		{
			ptvi = GetNewTreeView();
			if ( ptvi == NULL )
				return FALSE;

			ptvi->pView = this;
			ptvi->index = underAnim1Cnt + underAnim2Cnt + baseCnt + fbIndex;
			ptvi->zoom = zoomIndex;
			ptvi->pUser = NULL;
			pTreeCtrl->AddNewItem( hZoom, "overlay bitmap", ptvi, ( HTREEITEM )TVI_LAST ); 
		}

		for( fbIndex = 0; fbIndex < anim1Cnt; fbIndex++ )
		{
			ptvi = GetNewTreeView();
			if ( ptvi == NULL )
				return FALSE;

			ptvi->pView = this;
			ptvi->index = underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt + fbIndex;
			ptvi->zoom = zoomIndex;
			ptvi->pUser = NULL;
			pTreeCtrl->AddNewItem( hZoom, "anim1 bitmap", ptvi, ( HTREEITEM )TVI_LAST ); 
		}

		for( fbIndex = 0; fbIndex < anim2Cnt; fbIndex++ )
		{
			ptvi = GetNewTreeView();
			if ( ptvi == NULL )
				return FALSE;

			ptvi->pView = this;
			ptvi->index = underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt + anim1Cnt + fbIndex;
			ptvi->zoom = zoomIndex;
			ptvi->pUser = NULL;
			pTreeCtrl->AddNewItem( hZoom, "anim2 bitmap", ptvi, ( HTREEITEM )TVI_LAST ); 
		}
	}

	return TRUE;
}

void View::Animate( int frames )
{
	ASSERT( 0 < frames );

	if ( underAnim1Cnt )
	{
		underAnim1Frames += frames;
		while( underAnim1Frames >= apUnderAnim1[ underAnim1Step ][ 0 ]->_holdTime )
		{
			underAnim1Step++;
			if ( underAnim1Step >= underAnim1Cnt )
				underAnim1Step = 0;

			underAnim1Frames -= apUnderAnim1[ underAnim1Step ][ 0 ]->_holdTime;
		}
	}

	if ( underAnim2Cnt )
	{
		underAnim2Frames += frames;
		while( underAnim2Frames >= apUnderAnim2[ underAnim2Step ][ 0 ]->_holdTime )
		{
			underAnim2Step++;
			if ( underAnim2Step >= underAnim2Cnt )
				underAnim2Step = 0;

			underAnim2Frames -= apUnderAnim2[ underAnim2Step ][ 0 ]->_holdTime;
		}
	}

	if ( anim1Cnt )
	{
		anim1Frames += frames;
		while( anim1Frames >= apAnim1[ anim1Step ][ 0 ]->_holdTime )
		{
			anim1Step++;
			if ( anim1Step >= anim1Cnt )
				anim1Step = 0;

			anim1Frames -= apAnim1[ anim1Step ][ 0 ]->_holdTime;
		}
	}

	if ( anim2Cnt )
	{
		anim2Frames += frames;
		while( anim2Frames >= apAnim2[ anim2Step ][ 0 ]->_holdTime )
		{
			anim2Step++;
			if ( anim2Step >= anim2Cnt )
				anim2Step = 0;

			anim2Frames -= apAnim2[ anim2Step ][ 0 ]->_holdTime;
		}
	}
}

void View::Render( int zoom, int index, void * /*pUserData*/, DrawSurface *pDrawSurf )
{
	ASSERT( pDrawSurf != NULL );

	if ( index == -1 )
	{
		//  Render whole view.
		
		//  If there's an under anim1, render that.
		if ( underAnim1Cnt )
			apUnderAnim1[ underAnim1Step ][ zoom ]->Render( 0, 0, pDrawSurf );
		
		//  If there's an under anim2, render that.
		if ( underAnim2Cnt )
			apUnderAnim2[ underAnim2Step ][ zoom ]->Render( 0, 0, pDrawSurf );

		//  Render all of the bases.
		int baseIndex;
		for( baseIndex = 0; baseIndex < baseCnt; baseIndex++ )
			apBases[ baseIndex ][ zoom ]->Render( 0, 0, pDrawSurf );

		//  If there's an overlay, render the overlay.  Note that 
		//  the sprite will have set my apBases to that of the
		//  no damage level if it detected an overlay.
		int overlayIndex;
		for( overlayIndex = 0; overlayIndex < overlayCnt; overlayIndex++ )
			apOverlay[ overlayIndex ][ zoom ]->Render( 0, 0, pDrawSurf );

		//  If there's an anim1, render that.
		if ( anim1Cnt )
			apAnim1[ anim1Step ][ zoom ]->Render( 0, 0, pDrawSurf );
		
		//  If there's an anim2, render that.
		if ( anim2Cnt )
			apAnim2[ anim2Step ][ zoom ]->Render( 0, 0, pDrawSurf );
	}
	else
	{
		//  Render single bitmap.
		if ( underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt + anim1Cnt <= index )
			//  It's an anim2 bitmap.
			apAnim2[ index - anim1Cnt - overlayCnt - baseCnt - underAnim2Cnt - underAnim1Cnt ][ zoom ]->Render( 0, 0, pDrawSurf );
		else if ( underAnim1Cnt + underAnim2Cnt + baseCnt + overlayCnt <= index )
			//  It's an anim1 bitmap.
			apAnim1[ index - overlayCnt - baseCnt - underAnim2Cnt - underAnim1Cnt ][ zoom ]->Render( 0, 0, pDrawSurf );
		else if ( underAnim1Cnt + underAnim2Cnt + baseCnt <= index )
			//  It's a base bitmap.
			apOverlay[ index - baseCnt - underAnim2Cnt - underAnim1Cnt ][ zoom ]->Render( 0, 0, pDrawSurf );
		else if ( underAnim1Cnt + underAnim2Cnt <= index )
			//  It's a base bitmap.
			apBases[ index - underAnim2Cnt - underAnim1Cnt ][ zoom ]->Render( 0, 0, pDrawSurf );
		else if ( underAnim1Cnt <= index )
			//  It's an under anim2 bitmap
			apUnderAnim2[ index - underAnim1Cnt ][ zoom ]->Render( 0, 0, pDrawSurf );
		else
			//  It's an under anim1 bitmap
			apUnderAnim1[ index ][ zoom ]->Render( 0, 0, pDrawSurf );
	}

	return;
}

BOOL View::Save( FILE *pFile, BOOL bShareware )
{
	ASSERT( pFile != NULL );

	//  Information which probably doesn't need to be in the file
	//  but makes life easier for me.
	if ( fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fwrite( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return FALSE;

	//  Anchor point.  May not be valid, but always present
	//  in view.
	if ( fwrite( &anchorPoint.iX, 1, sizeof( anchorPoint.iX ),  pFile ) != sizeof( anchorPoint.iX ) ||
		fwrite( &anchorPoint.iY, 1, sizeof( anchorPoint.iY ),  pFile ) != sizeof( anchorPoint.iY ) )
		return FALSE;

	//  Other hotspots.
	if ( fwrite( &nAttachedSpriteCnt, 1, sizeof( nAttachedSpriteCnt ), pFile ) != sizeof( nAttachedSpriteCnt ) )
		return FALSE;

	int hotspotIndex;
	THotSpot  *pHotSpot;
	for( hotspotIndex = 0, pHotSpot = aAttachedSprites; 
		hotspotIndex < nAttachedSpriteCnt; 
		hotspotIndex++, pHotSpot++ )
	{
		if ( fwrite( &pHotSpot->iType, 1, sizeof( pHotSpot->iType ), pFile ) != sizeof( pHotSpot->iType ) ||
			fwrite( &pHotSpot->iX, 1, sizeof( pHotSpot->iX ), pFile ) != sizeof( pHotSpot->iX ) ||
			fwrite( &pHotSpot->iY, 1, sizeof( pHotSpot->iY ), pFile ) != sizeof( pHotSpot->iY ) ||
			fwrite( &pHotSpot->iIndex, 1, sizeof( pHotSpot->iIndex ), pFile ) != sizeof( pHotSpot->iIndex ) )
			return FALSE;
	}

	int fbIndex;
	int zero = 0;
	if ( bBorrowedBase )
	{
		//  The base bitmaps were borrowed from a previous damage level,
		//  so write 0 for the base count and don't write the base images.
		if ( fwrite( &zero, 1, sizeof( zero ), pFile ) != sizeof( zero ) )
			return FALSE;
	}
	else
	{
		//  If I have base bitmaps of my own, there can't be an overlay.
		ASSERT( overlayCnt == 0 );

		//  Write out the number of base fb's.
		if ( fwrite( &baseCnt, 1, sizeof( baseCnt ), pFile ) != sizeof( baseCnt ) )
			return FALSE;

		//  Save the base fb's.
		for( fbIndex = 0; fbIndex < baseCnt; fbIndex++ )
		{
			if ( bShareware == FALSE )
			{
				if ( SaveImage( pFile, apBases[ fbIndex ][ 0 ] ) == FALSE ||
					SaveImage( pFile, apBases[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apBases[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apBases[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
			else
			{
				if ( SaveImage( pFile, apBases[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apBases[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apBases[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
		}
	}

	if ( overlayCnt )
	{
		//  If I have an overlay, bBorrowedBase better be true.
		ASSERT( bBorrowedBase );
	}

	//  Write out the number of overlay fb's.
	if ( fwrite( &overlayCnt, 1, sizeof( overlayCnt ), pFile ) != sizeof( overlayCnt ) )
		return FALSE;

	//  Save the overlay fb's.
	for( fbIndex = 0; fbIndex < overlayCnt; fbIndex++ )
	{
		if ( bShareware == FALSE )
		{
			if ( SaveImage( pFile, apOverlay[ fbIndex ][ 0 ] ) == FALSE ||
				SaveImage( pFile, apOverlay[ fbIndex ][ 1 ] ) == FALSE ||
				SaveImage( pFile, apOverlay[ fbIndex ][ 2 ] ) == FALSE ||
				SaveImage( pFile, apOverlay[ fbIndex ][ 3 ] ) == FALSE )
				return FALSE;
		}
		else
		{
			if ( SaveImage( pFile, apOverlay[ fbIndex ][ 1 ] ) == FALSE ||
				SaveImage( pFile, apOverlay[ fbIndex ][ 2 ] ) == FALSE ||
				SaveImage( pFile, apOverlay[ fbIndex ][ 3 ] ) == FALSE )
				return FALSE;
		}
	}

	if ( bBorrowedAnim1 )
	{
		//  The anim1 bitmaps were borrowed from a previous damage level,
		//  so write 0 for the anim1 count and don't write the anim1 images.
		if ( fwrite( &zero, 1, sizeof( zero ), pFile ) != sizeof( zero ) )
			return FALSE;
	}
	else
	{
		//  Write out the number of anim1 fb's.
		if ( fwrite( &anim1Cnt, 1, sizeof( anim1Cnt ), pFile ) != sizeof( anim1Cnt ) )
			return FALSE;

		//  Save the anim1 fb's.
		for( fbIndex = 0; fbIndex < anim1Cnt; fbIndex++ )
		{
			if ( bShareware == FALSE )
			{
				if ( SaveImage( pFile, apAnim1[ fbIndex ][ 0 ] ) == FALSE ||
					SaveImage( pFile, apAnim1[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apAnim1[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apAnim1[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
			else
			{
				if ( SaveImage( pFile, apAnim1[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apAnim1[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apAnim1[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
		}
	}

	if ( bBorrowedAnim2 )
	{
		//  The anim2 bitmaps were borrowed from a previous damage level,
		//  so write 0 for the anim2 count and don't write the anim2 images.
		if ( fwrite( &zero, 1, sizeof( zero ), pFile ) != sizeof( zero ) )
			return FALSE;
	}
	else
	{
		//  Write out the number of anim2 fb's.
		if ( fwrite( &anim2Cnt, 1, sizeof( anim2Cnt ), pFile ) != sizeof( anim2Cnt ) )
			return FALSE;

		//  Save the anim2 fb's.
		for( fbIndex = 0; fbIndex < anim2Cnt; fbIndex++ )
		{
			if ( bShareware == FALSE )
			{
				if ( SaveImage( pFile, apAnim2[ fbIndex ][ 0 ] ) == FALSE ||
					SaveImage( pFile, apAnim2[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apAnim2[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apAnim2[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
			else
			{
				if ( SaveImage( pFile, apAnim2[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apAnim2[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apAnim2[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
		}
	}

	if ( bBorrowedUnderAnim1 )
	{
		//  The under anim1 bitmaps were borrowed from a previous damage level,
		//  so write 0 for the under anim1 count and don't write the under anim1 images.
		if ( fwrite( &zero, 1, sizeof( zero ), pFile ) != sizeof( zero ) )
			return FALSE;
	}
	else
	{
		//  Write out the number of under anim1 fb's.
		if ( fwrite( &underAnim1Cnt, 1, sizeof( underAnim1Cnt ), pFile ) != sizeof( underAnim1Cnt ) )
			return FALSE;

		//  Save the under anim1 fb's.
		for( fbIndex = 0; fbIndex < underAnim1Cnt; fbIndex++ )
		{
			if ( bShareware == FALSE )
			{
				if ( SaveImage( pFile, apUnderAnim1[ fbIndex ][ 0 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim1[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim1[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim1[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
			else
			{
				if ( SaveImage( pFile, apUnderAnim1[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim1[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim1[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
		}
	}

	if ( bBorrowedUnderAnim2 )
	{
		//  The under anim2 bitmaps were borrowed from a previous damage level,
		//  so write 0 for the under anim2 count and don't write the under anim2 images.
		if ( fwrite( &zero, 1, sizeof( zero ), pFile ) != sizeof( zero ) )
			return FALSE;
	}
	else
	{
		//  Write out the number of under anim2 fb's.
		if ( fwrite( &underAnim2Cnt, 1, sizeof( underAnim2Cnt ), pFile ) != sizeof( underAnim2Cnt ) )
			return FALSE;
		
		//  Save the under anim2 fb's.
		for( fbIndex = 0; fbIndex < underAnim2Cnt; fbIndex++ )
		{
			if ( bShareware == FALSE )
			{
				if ( SaveImage( pFile, apUnderAnim2[ fbIndex ][ 0 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim2[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim2[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim2[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
			else
			{
				if ( SaveImage( pFile, apUnderAnim2[ fbIndex ][ 1 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim2[ fbIndex ][ 2 ] ) == FALSE ||
					SaveImage( pFile, apUnderAnim2[ fbIndex ][ 3 ] ) == FALSE )
					return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL View::Load( FILE *pFile, char const * pszFileName )
{
	ASSERT( pFile != NULL );

	//  Information which probably doesn't need to be in the file
	//  but makes life easier for me.
	if ( fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return FALSE;

	//  Anchor point.  May not be valid but always present.
	if ( fread( &anchorPoint.iX, 1, sizeof( anchorPoint.iX ), pFile ) != sizeof( anchorPoint.iX ) ||
		fread( &anchorPoint.iY, 1, sizeof( anchorPoint.iY ), pFile ) != sizeof( anchorPoint.iY ) )
		return FALSE;

	//  Other hotspots
	if ( fread( &nAttachedSpriteCnt, 1, sizeof( nAttachedSpriteCnt ), pFile ) != sizeof( nAttachedSpriteCnt ) )
		return FALSE;

	int hotspotIndex;
	THotSpot *pHotSpot;
	for( hotspotIndex = 0, pHotSpot = aAttachedSprites;
		hotspotIndex < nAttachedSpriteCnt;
		hotspotIndex++, pHotSpot++ )
	{
		if ( fread( &pHotSpot->iType, 1, sizeof( pHotSpot->iType ), pFile ) != sizeof( pHotSpot->iType ) ||
			fread( &pHotSpot->iX, 1, sizeof( pHotSpot->iX ), pFile ) != sizeof( pHotSpot->iX ) ||
			fread( &pHotSpot->iY, 1, sizeof( pHotSpot->iY ), pFile ) != sizeof( pHotSpot->iY ) ||
			fread( &pHotSpot->iIndex, 1, sizeof( pHotSpot->iIndex ), pFile ) != sizeof( pHotSpot->iIndex ) )
			return FALSE;
	}

	//  Read the number of base fb's.
	if ( fread( &baseCnt, 1, sizeof( baseCnt ), pFile ) != sizeof( baseCnt ) )
		return FALSE;

	//  Make sure it's in range.
	if ( baseCnt < 0 || MAX_BASE < baseCnt )
		return FALSE;

	//  Read the base fb's.
	int fbIndex;
	for( fbIndex = 0; fbIndex < baseCnt; fbIndex++ )
	{
		apBases[ fbIndex ][ 0 ] = LoadImage( pFile, pszFileName );
		apBases[ fbIndex ][ 1 ] = LoadImage( pFile, pszFileName );
		apBases[ fbIndex ][ 2 ] = LoadImage( pFile, pszFileName );
		apBases[ fbIndex ][ 3 ] = LoadImage( pFile, pszFileName );
		if ( apBases[ fbIndex ][ 0 ] == NULL || apBases[ fbIndex ][ 1 ] == NULL ||
			apBases[ fbIndex ][ 2 ] == NULL || apBases[ fbIndex ][ 3 ] == NULL )
			return FALSE;
	}

	//  Read the number of overlay fb's.
	if ( fread( &overlayCnt, 1, sizeof( overlayCnt ), pFile ) != sizeof( overlayCnt ) )
		return FALSE;

	//  Should not have both base bitmaps and overlay bitmaps, although it
	//  is possible to have neither.
	if ( ( baseCnt != 0 && overlayCnt != 0 ) ||
		( overlayCnt < 0 || MAX_OVERLAY < overlayCnt ) )
		return FALSE;

	//  Read the overlay fb's.
	for( fbIndex = 0; fbIndex < overlayCnt; fbIndex++ )
	{
		apOverlay[ fbIndex ][ 0 ] = LoadImage( pFile, pszFileName );
		apOverlay[ fbIndex ][ 1 ] = LoadImage( pFile, pszFileName );
		apOverlay[ fbIndex ][ 2 ] = LoadImage( pFile, pszFileName );
		apOverlay[ fbIndex ][ 3 ] = LoadImage( pFile, pszFileName );
		if ( apOverlay[ fbIndex ][ 0 ] == NULL || apOverlay[ fbIndex ][ 1 ] == NULL ||
			apOverlay[ fbIndex ][ 2 ] == NULL || apOverlay[ fbIndex ][ 3 ] == NULL )
			return FALSE;
	}

	//  Read the number of anim1 fb's.
	if ( fread( &anim1Cnt, 1, sizeof( anim1Cnt ), pFile ) != sizeof( anim1Cnt ) )
		return FALSE;

	//  Make sure it's in a valid range.
	if ( anim1Cnt < 0 || MAX_ANIM < anim1Cnt )
		return FALSE;

	//  Read the anim1 fb's.
	for( fbIndex = 0; fbIndex < anim1Cnt; fbIndex++ )
	{
		apAnim1[ fbIndex ][ 0 ] = LoadImage( pFile, pszFileName );
		apAnim1[ fbIndex ][ 1 ] = LoadImage( pFile, pszFileName );
		apAnim1[ fbIndex ][ 2 ] = LoadImage( pFile, pszFileName );
		apAnim1[ fbIndex ][ 3 ] = LoadImage( pFile, pszFileName );
		if ( apAnim1[ fbIndex ][ 0 ] == NULL || apAnim1[ fbIndex ][ 1 ] == NULL ||
			apAnim1[ fbIndex ][ 2 ] == NULL || apAnim1[ fbIndex ][ 3 ] == NULL )
			return FALSE;
	}

	//  Read the number of anim2 fb's.
	if ( fread( &anim2Cnt, 1, sizeof( anim2Cnt ), pFile ) != sizeof( anim2Cnt ) )
		return FALSE;

	//  Make sure it's in a valid range.
	if ( anim2Cnt < 0 || MAX_ANIM < anim2Cnt )
		return FALSE;

	//  Read the anim2 fb's.
	for( fbIndex = 0; fbIndex < anim2Cnt; fbIndex++ )
	{
		apAnim2[ fbIndex ][ 0 ] = LoadImage( pFile, pszFileName );
		apAnim2[ fbIndex ][ 1 ] = LoadImage( pFile, pszFileName );
		apAnim2[ fbIndex ][ 2 ] = LoadImage( pFile, pszFileName );
		apAnim2[ fbIndex ][ 3 ] = LoadImage( pFile, pszFileName );
		if ( apAnim2[ fbIndex ][ 0 ] == NULL || apAnim2[ fbIndex ][ 1 ] == NULL ||
			apAnim2[ fbIndex ][ 2 ] == NULL || apAnim2[ fbIndex ][ 3 ] == NULL )
			return FALSE;
	}

	//  Read the number of under anim1 fb's.
	if ( fread( &underAnim1Cnt, 1, sizeof( underAnim1Cnt ), pFile ) != sizeof( underAnim1Cnt ) )
		return FALSE;

	//  Make sure it's in a valid range.
	if ( underAnim1Cnt < 0 || MAX_ANIM < underAnim1Cnt )
		return FALSE;

	//  Read the anim1 fb's.
	for( fbIndex = 0; fbIndex < underAnim1Cnt; fbIndex++ )
	{
		apUnderAnim1[ fbIndex ][ 0 ] = LoadImage( pFile, pszFileName );
		apUnderAnim1[ fbIndex ][ 1 ] = LoadImage( pFile, pszFileName );
		apUnderAnim1[ fbIndex ][ 2 ] = LoadImage( pFile, pszFileName );
		apUnderAnim1[ fbIndex ][ 3 ] = LoadImage( pFile, pszFileName );
		if ( apUnderAnim1[ fbIndex ][ 0 ] == NULL || apUnderAnim1[ fbIndex ][ 1 ] == NULL ||
			apUnderAnim1[ fbIndex ][ 2 ] == NULL || apUnderAnim1[ fbIndex ][ 3 ] == NULL )
			return FALSE;
	}

	//  Read the number of anim2 fb's.
	if ( fread( &underAnim2Cnt, 1, sizeof( underAnim2Cnt ), pFile ) != sizeof( underAnim2Cnt ) )
		return FALSE;

	//  Make sure it's in a valid range.
	if ( underAnim2Cnt < 0 || MAX_ANIM < underAnim2Cnt )
		return FALSE;

	//  Read the anim2 fb's.
	for( fbIndex = 0; fbIndex < underAnim2Cnt; fbIndex++ )
	{
		apUnderAnim2[ fbIndex ][ 0 ] = LoadImage( pFile, pszFileName );
		apUnderAnim2[ fbIndex ][ 1 ] = LoadImage( pFile, pszFileName );
		apUnderAnim2[ fbIndex ][ 2 ] = LoadImage( pFile, pszFileName );
		apUnderAnim2[ fbIndex ][ 3 ] = LoadImage( pFile, pszFileName );
		if ( apUnderAnim2[ fbIndex ][ 0 ] == NULL || apUnderAnim2[ fbIndex ][ 1 ] == NULL ||
			apUnderAnim2[ fbIndex ][ 2 ] == NULL || apUnderAnim2[ fbIndex ][ 3 ] == NULL )
			return FALSE;
	}

	return TRUE;
}

/*****************************************************
 *
 *  Building View
 *
 *****************************************************/

BOOL BuildingView::LoadFile( const char *pFullPath, int index, int type )
{
	Image *pImage = NULL, *pImage1_2 = NULL, *pImage1_4 = NULL, *pImage1_8 = NULL;
	Image *pDitheredImage = NULL;
	char *pExt = NULL;

	//  Determine which type of file to load by the
	//  extension.  If we cannot determine this, we
	//  cannot load the file.
	pExt = strrchr( pFullPath, '.' );
	if ( pExt == NULL ||
		( _stricmp( pExt, ".dib" ) != 0 &&
		  _stricmp( pExt, ".bmp" ) != 0 &&
		  _stricmp( pExt, ".tga" ) != 0 ) )
		return FALSE;

	//  Load the file.
	if ( _stricmp( pExt, ".dib" ) == 0 ||
		_stricmp( pExt, ".bmp" ) == 0 )
	{
		BMPFile	file;
		pImage = file.Load( pFullPath );
	}
	else if ( _stricmp( pExt, ".tga" ) == 0 )
	{
		TGAFile file;
		pImage = file.Load( pFullPath );
	}

	//  If the file failed to load, return FALSE.
	if ( pImage == NULL )
		return FALSE;

	//  Update the width and height if necessary.
	if ( width < pImage->_width )
		width = pImage->_width;
	if ( height < pImage->_height )
		height = pImage->_height;

	//  Check to see if the user wants to cancel the
	//  operation.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

#ifdef BUGBUG
	//  Crop the file.  If this fails, handle the error
	//  and return.
	if ( pImage->Crop() == false )
		goto HandleErrorOrCancel;

	//  Check again to see if the user wants to cancel
	//  the operation.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;
#endif

	//  Get any hotspot information out of the file.
	GetHotSpots( pImage );

	//  Create copies of the image for scaling.
	pImage1_2 = new Image( *pImage );
	pImage1_4 = new Image( *pImage );
	pImage1_8 = new Image( *pImage );

	//  If any of the copies failed to allocate,
	//  handle the error and return.
	if ( pImage1_2 == NULL ||
		pImage1_4 == NULL ||
		pImage1_8 == NULL )
		goto HandleErrorOrCancel;

	//  Check again to see if the user wants to cancel.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

	//  Scale the copies.  If any of the scaling operations
	//  fails, handle the error and return.
	if ( pImage1_2->ScaleDown( 2 ) == false ||
		pImage1_4->ScaleDown( 4 ) == false ||
		pImage1_8->ScaleDown( 8 ) == false )
		goto HandleErrorOrCancel;

	// crop all copies
	if ( pImage->Crop() == false )
		goto HandleErrorOrCancel;
	if ( pImage1_2->Crop() == false )
		goto HandleErrorOrCancel;
	if ( pImage1_4->Crop() == false )
		goto HandleErrorOrCancel;
	if ( pImage1_8->Crop() == false )
		goto HandleErrorOrCancel;

	//  Check again to see if the user wants to cancel.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

	//  Determine if the images need to be color reduced.
	if ( _bitDepth == 8 )
	{
		//  We need to dither images down to 8 bits.

		//  Dither the first image.  If the dithering fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Get rid of the current image, and save the dithered
		//  image as the current image.
		delete pImage;
		pImage = pDitheredImage;

		//  Dither the 1/2 scale image.  If the dithering fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage1_2 );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Get rid of the 1/2 scale image, and save the dithered
		//  image as the 1/2 scale image.
		delete pImage1_2;
		pImage1_2 = pDitheredImage;

		//  Dither the 1/4 scale image.  If the dithering fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage1_4 );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Get rid of the 1/4 scale image, and save the dithered image
		//  as the 1/4 scale image.
		delete pImage1_4;
		pImage1_4 = pDitheredImage;

		//  Dither the 1/8 scale image.  If the dithering fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage1_8 );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Get rid of the 1/8 scale image, and save the dithered
		//  image as the 1/8 scale image.
		delete pImage1_8;
		pImage1_8 = pDitheredImage;
	}
	else if ( _bitDepth == 15 )
	{
		//  Need to reduce to 15-bit image.
		
		//  Reduce all images to 15-bit.  If any of the
		//  reductions fails, handle the error and exit.
		if ( pImage->ReduceTo15() == false ||
			pImage1_2->ReduceTo15() == false ||
			pImage1_4->ReduceTo15() == false ||
			pImage1_8->ReduceTo15() == false )
			goto HandleErrorOrCancel;
	}
	else if ( _bitDepth == 16 )
	{
		//  Need to reduce to 16-bit image.

		//  Reduce all images to 16-bit.  If any of the 
		//  reductions fails, handle the error and exit.
		if ( pImage->ReduceTo16() == false ||
			pImage1_2->ReduceTo16() == false ||
			pImage1_4->ReduceTo16() == false ||
			pImage1_8->ReduceTo16() == false )
			goto HandleErrorOrCancel;
	}
	else if ( _bitDepth == 32 )
	{
		//  Need to increase to 32-bit image.

		//  Increase all images to 32-bit.  If any of the
		//  increases fails, handle the error and exit.
		if ( pImage->IncreaseTo32() == false ||
			pImage1_2->IncreaseTo32() == false ||
			pImage1_4->IncreaseTo32() == false ||
			pImage1_8->IncreaseTo32() == false )
			goto HandleErrorOrCancel;
	}

	//  See if the user wants to cancel.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

	//  Save the image pointers.
	if ( type == 0 )
	{
		apBases[ index ][ 0 ] = pImage;
		apBases[ index ][ 1 ] = pImage1_2;
		apBases[ index ][ 2 ] = pImage1_4;
		apBases[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 1 )
	{
		apAnim1[ index ][ 0 ] = pImage;
		apAnim1[ index ][ 1 ] = pImage1_2;
		apAnim1[ index ][ 2 ] = pImage1_4;
		apAnim1[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 2 )
	{
		apAnim2[ index ][ 0 ] = pImage;
		apAnim2[ index ][ 1 ] = pImage1_2;
		apAnim2[ index ][ 2 ] = pImage1_4;
		apAnim2[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 3 )
	{
		apOverlay[ index ][ 0 ] = pImage;
		apOverlay[ index ][ 1 ] = pImage1_2;
		apOverlay[ index ][ 2 ] = pImage1_4;
		apOverlay[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 4 )
	{
		apUnderAnim1[ index ][ 0 ] = pImage;
		apUnderAnim1[ index ][ 1 ] = pImage1_2;
		apUnderAnim1[ index ][ 2 ] = pImage1_4;
		apUnderAnim1[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 5 )
	{
		apUnderAnim2[ index ][ 0 ] = pImage;
		apUnderAnim2[ index ][ 1 ] = pImage1_2;
		apUnderAnim2[ index ][ 2 ] = pImage1_4;
		apUnderAnim2[ index ][ 3 ] = pImage1_8;
	}
	else
		ASSERT( false );

	return TRUE;

HandleErrorOrCancel:;
	delete pImage;
	delete pImage1_2;
	delete pImage1_4;
	delete pImage1_8;
	delete pDitheredImage;

	return FALSE;
}

BOOL
View::SaveCompressedImage( FILE *pFile, Image *pImage )
{
	ASSERT( pFile != NULL );
	ASSERT( pImage != NULL );

	char 	id[ 2 ] = { '0', '0' };
	short	width = ( short )pImage->_width;
	short	height = ( short )pImage->_height;
	short	xoff = ( short )pImage->_xoff;
	short	yoff = ( short )pImage->_yoff;
	short	holdTime = ( short )pImage->_holdTime;

	//  Write everything in the FB except the data.
	if ( fwrite( &id, 1, sizeof( id ), pFile ) != sizeof( id ) ||
		fwrite( &holdTime, 1, sizeof( holdTime ), pFile ) != sizeof( holdTime ) ||
		fwrite( &xoff, 1, sizeof( xoff ), pFile ) != sizeof( xoff ) ||
		fwrite( &yoff, 1, sizeof( yoff ), pFile ) != sizeof( yoff ) ||
		fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) )
 		return FALSE;

	//  Write the data, in compressed form.  
	
	// GG: Rewrite
	//     New format is:
	//
	//		int 	iCompressedLength
	//		int	iPixelOffset (offset of start of compressed pixels from here
	//		int	aiRowStartByteOffsets[ <image_height> ];
	//		<image_height compressed rows>
	//
	// 	where a compressed row starts with # bytes of transparent pixels to 
	//		skip and ends when # bytes non-transparent pixels is 0. 
	//
	//		A compressed row is a sequence of: 
	//
	//			# bytes of transparent pixels
	//			# bytes of non-transparent pixels
	//			non-transparent pixels
	//			etc.

	int			 iBytesPerPixel = pImage->_bytesPerPixel;
	byte const * pData 		  = pImage->_pPixels;
	int		  * piLineOffset = new int [ height ];	
	int			 iCompressedSize = 0;
	long			 lStartPos = ftell( pFile );

	//  Save space for the header
	if ( fseek( pFile, 2 * sizeof( int ) + height * sizeof( *piLineOffset ) + sizeof( iCompressedSize ), SEEK_CUR ))
	{
		delete piLineOffset;
		return FALSE;
	}

	// Compress each row

	for ( int iY = 0; iY < height; ++iY )
	{
		piLineOffset[ iY ] = iCompressedSize;

		assert( pData == pImage->_pPixels + iY * width * iBytesPerPixel );

		byte const * pEnd = pData + width * iBytesPerPixel;

		assert( pEnd == pImage->_pPixels + ( iY + 1 ) * width * iBytesPerPixel );

		for ( ;; )
		{
			byte const * pStart = pData;

			while ( pData < pEnd && pImage->IsTransparent( pData ))
				pData += iBytesPerPixel;

			int	iSkipBytes = pData - pStart;

			iCompressedSize += sizeof( iSkipBytes );

			if ( 1 != fwrite( & iSkipBytes, sizeof( iSkipBytes ), 1, pFile ))
			{
				delete piLineOffset;

				return FALSE;
			}

			pStart = pData;

			// Write # transparent bytes

			while ( pData < pEnd && ! pImage->IsTransparent( pData ))
				pData += iBytesPerPixel;

			int	iDataBytes = pData - pStart;

			iCompressedSize += sizeof( iDataBytes );
			iCompressedSize += iDataBytes;

			// Write # non-transparent bytes

			if ( 1 != fwrite( & iDataBytes, sizeof( iDataBytes ), 1, pFile ))
			{
				delete piLineOffset;
				return FALSE;
			}

			if ( 0 == iDataBytes )
				break;

			// Write the pixels

			if ( 1 != fwrite( pStart, iDataBytes, 1, pFile ))
			{
				delete piLineOffset;
				return FALSE;
			}
		}
	}

	// Write the compressed size

	long	lEndPos = ftell( pFile );

	if ( fseek( pFile, lStartPos, SEEK_SET ))
	{
		delete piLineOffset;
		return FALSE;
	}

	iCompressedSize += 2 * sizeof( int ) + height * sizeof( * piLineOffset );

	if ( 1 != fwrite( & iCompressedSize, sizeof( iCompressedSize ), 1, pFile ))
		return FALSE;

	// Write the width/height

	int	iHeight = height;
	int	iWidth  = width;

	if ( 1 != fwrite( & iWidth,  sizeof( iWidth  ), 1, pFile ) ||
	     1 != fwrite( & iHeight, sizeof( iHeight ), 1, pFile ))
	{
		delete piLineOffset;
		return FALSE;
	}

	// Write the row start byte offsets

	if ( height != ( short )fwrite( piLineOffset, sizeof( *piLineOffset ), height, pFile ))
	{
		delete piLineOffset;
		return FALSE;
	}

	delete piLineOffset;

	if ( fseek( pFile, lEndPos, SEEK_SET ))
		return FALSE;

	return TRUE;
}

BOOL BuildingView::SaveImage( FILE *pFile, Image *pImage )
{
	return SaveCompressedImage( pFile, pImage );
}

Image *BuildingView::LoadImage( FILE *pFile, const char * pszFileName )
{
	char 	id[ 2 ];
	short	holdTime;
	short	xoff, yoff;
	short	width, height;
	int		size;

	if ( fread( &id, 1, sizeof( id ), pFile ) != sizeof( id ) ||
		fread( &holdTime, 1, sizeof( holdTime ), pFile ) != sizeof( holdTime ) ||
		fread( &xoff, 1, sizeof( xoff ), pFile ) != sizeof( xoff ) ||
		fread( &yoff, 1, sizeof( yoff ), pFile ) != sizeof( yoff ) ||
		fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &size, 1, sizeof( size ), pFile ) != sizeof( size ) )
 		return FALSE;

	Image *pImage = new Image( width, height, _bitDepth, theApp.aPalette );
	if ( pImage == NULL )
		return FALSE;

	pImage->strFileName = pszFileName;

	pImage->SetSkip( RGBColor( 255, 0, 255 ) );

	pImage->_xoff = xoff;
	pImage->_yoff = yoff;
	pImage->_holdTime = holdTime;

	//  Find a transparent color.
	dword skip;
	if ( _bitDepth == 8 )
	{
		for( skip = 0; skip < 256; skip++ )
			if ( theApp.pCurrentPalette[ skip ].rgbRed == 255 &&
				theApp.pCurrentPalette[ skip ].rgbGreen == 0 &&
				theApp.pCurrentPalette[ skip ].rgbBlue == 255 )
				break;
		if ( 256 <= skip )
		{
			AfxMessageBox( "No skip color in current palette!" );
			skip = 0;
		}
	}
	else if ( _bitDepth == 15 )
	{
		RGBColor	s( 255, 0, 255 );
		skip = s.To15();
	}
	else if ( _bitDepth == 16 )
	{
		RGBColor	s( 255, 0, 255 );
		skip = s.To16();
	}
	else if ( _bitDepth == 32 )
	{
		skip = 0x00FF00FF;
	}
	//  Note 24 handled specially down below.

	int totalSize = width * height * pImage->_bytesPerPixel;
	int bytesDecompressed = 0;
	byte *pDest = pImage->_pPixels;
//	while( bytesDecompressed < totalSize )
	while( 1 )	// GG
	{
		int skipCnt;
		if ( fread( &skipCnt, 1, sizeof( skipCnt ), pFile ) != sizeof( skipCnt ) )
			return NULL;
		bytesDecompressed += skipCnt;

		skipCnt /= pImage->_bytesPerPixel;

		if ( _bitDepth == 8 )
		{
			while( skipCnt-- )
				*pDest++ = ( byte )skip;
		}
		else if ( _bitDepth == 15 || _bitDepth == 16 )
		{
			while( skipCnt-- )
			{
				*( word * )pDest= ( word )skip;
				pDest += 2;
			}
		}
		else if ( _bitDepth == 24 )
		{
			while( skipCnt-- )
			{
				*pDest++ = 255;
				*pDest++ = 0;
				*pDest++ = 255;
			}
		}
		else if ( _bitDepth == 32 )
		{
			while( skipCnt-- )
			{
				*( dword * )pDest = skip;
				pDest += 4;
			}
		}

// GG		if ( bytesDecompressed >= totalSize )
//			break;

		int run;
		if ( fread( &run, 1, sizeof( run ), pFile ) != sizeof( run ) ||
			 fread( pDest, 1, run, pFile ) != ( size_t )run )
			return NULL;

		if ( !run )	// GG
			break;

		bytesDecompressed += run;
		pDest += run;
	}
// GG
//	int dummy;
//	fread( &dummy, 1, sizeof( dummy ), pFile );
//	fread( &dummy, 1, sizeof( dummy ), pFile );

	return pImage;
}

//  Renders the current state of the view, or the
//  single bitmap represented by the index, into
//  the DIB.  Can be overridden by views which
//  have special rendering needs.
void BuildingView::Render( int zoom, int index, void *pUserData, DrawSurface *pDrawSurf )
{
	if ( pUserData == NULL )
		//  If pUserData is NULL, there is only view of this building.
		View::Render( zoom, index, pUserData, pDrawSurf );
	else
	{
		//  Otherwise, there are two views - foreground and background.
		//  If the bool in the structure is true, the View in the structure
		//  is the foreground and should be drawn last.  Otherwise, the
		//  view in the structure is background and should be drawn first.
		TBuildViewInfo *pbvi = ( TBuildViewInfo * )pUserData;

		if ( pbvi->bForeground )
		{
			View::Render( zoom, index, NULL, pDrawSurf );
			if ( pbvi->pOtherView )
				pbvi->pOtherView->Render( zoom, index, NULL, pDrawSurf );
		}
		else
		{
			if ( pbvi->pOtherView )
				pbvi->pOtherView->Render( zoom, index, NULL, pDrawSurf );
			View::Render( zoom, index, NULL, pDrawSurf );
		}
	}
}

/*****************************************************
 *
 *  Vehicle View
 *
 *****************************************************/

BOOL VehicleView::LoadFile( const char *pFullPath, int index, int type )
{
	Image *pImage = NULL, *pImage1_2 = NULL, *pImage1_4 = NULL, *pImage1_8 = NULL;
	Image *pDitheredImage = NULL;
	char *pExt = NULL;

	//  Determine which type of file to load by the
	//  extension.  If we cannot determine this, we
	//  cannot load the file.
	pExt = strrchr( pFullPath, '.' );
	if ( pExt == NULL ||
		( _stricmp( pExt, ".dib" ) != 0 &&
		  _stricmp( pExt, ".bmp" ) != 0 &&
		  _stricmp( pExt, ".tga" ) != 0 ) )
		return FALSE;

	//  Load the file.
	if ( _stricmp( pExt, ".dib" ) == 0 ||
		_stricmp( pExt, ".bmp" ) == 0 )
	{
		BMPFile	file;
		pImage = file.Load( pFullPath );
	}
	else if ( _stricmp( pExt, ".tga" ) == 0 )
	{
		TGAFile file;
		pImage = file.Load( pFullPath );
	}

	//  If the file failed to load, return false.
	if ( pImage == NULL )
		return FALSE;

	//  Update the width and height if necessary.
	if ( width < pImage->_width )
		width = pImage->_width;
	if ( height < pImage->_height )
		height = pImage->_height;

	//  Check to see if the user wants to cancel.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

	pImage->ScaleUp ( 1.5f );

	//  Get any hotspot information out of the file.
	GetHotSpots( pImage, 1.5f );

	//  Create copies of the image for scaling.
	pImage1_2 = new Image( *pImage );
	pImage1_4 = new Image( *pImage );
	pImage1_8 = new Image( *pImage );

	//  If any of the copies failed, handle the error
	//  and return.
	if ( pImage1_2 == NULL || pImage1_4 == NULL || pImage1_8 == NULL )
		goto HandleErrorOrCancel;

	//  Check to see if the user wants to cancel.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

	//  Scale the copies.  If any of the scaling operations
	//  fails, handle the error and return.
	if ( pImage1_2->ScaleDown( 2 ) == false ||
		pImage1_4->ScaleDown( 4 ) == false ||
		pImage1_8->ScaleDown( 8 ) == false )
		goto HandleErrorOrCancel;

	//  Crop the file.  If this fails, handle the error
	//  and return.
	if ( pImage->Crop() == false )
		goto HandleErrorOrCancel;
	if ( pImage1_2->Crop() == false )
		goto HandleErrorOrCancel;
	if ( pImage1_4->Crop() == false )
		goto HandleErrorOrCancel;
	if ( pImage1_8->Crop() == false )
		goto HandleErrorOrCancel;

	//  Check to see if we need to convert the image
	//  to a different bit depth.
	if ( _bitDepth == 8 )
	{
		//  We need to dither the images down to 8 bits.

		//  Dither the original image.  If this fails,
		//  handle the error and return.
		Image *pDitheredImage = theApp.ditherer.DitherImage( pImage );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Get rid of the old image and save the dithered
		//  image as the full-size image.
		delete pImage;
		pImage = pDitheredImage;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Dither the 1/2 scale image.  If this fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage1_2 );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Get rid of the old 1/2 scale image and save the dithered
		//  image as the 1/2 scale image.
		delete pImage1_2;
		pImage1_2 = pDitheredImage;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Dither the 1/4 scale image.  If this fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage1_4 );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Get rid of the old 1/4 scale image and save the dithered
		//  image as the 1/4 scale image.
		delete pImage1_4;
		pImage1_4 = pDitheredImage;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;

		//  Dither the 1/8 scale image.  If this fails,
		//  handle the error and return.
		pDitheredImage = theApp.ditherer.DitherImage( pImage1_8 );
		if ( pDitheredImage == NULL )
			goto HandleErrorOrCancel;

		//  Get rid of the old 1/8 scale image and save the dithered
		//  image as the 1/8 scale image.
		delete pImage1_8;
		pImage1_8 = pDitheredImage;

		//  Check to see if the user wants to cancel.
		if ( theApp.FlushMessageQueue() == FALSE )
			goto HandleErrorOrCancel;
	}
	else if ( _bitDepth == 15 )
	{
		//  We need to reduce the images down to 15 bit.

		//  Reduce the images to 15-bit.  If any of the
		//  reductions fails, handle the error and return.
		if ( pImage->ReduceTo15() == false ||
			pImage1_2->ReduceTo15() == false ||
			pImage1_4->ReduceTo15() == false ||
			pImage1_8->ReduceTo15() == false )
			goto HandleErrorOrCancel;
	}
	else if ( _bitDepth == 16 )
	{
		//  We need to reduce the images down to 16 bit.

		//  Reduce the image to 16-bit.  If any of the 
		//  reductions fails, handle the error and return.
		if ( pImage->ReduceTo16() == false ||
			pImage1_2->ReduceTo16() == false ||
			pImage1_4->ReduceTo16() == false ||
			pImage1_8->ReduceTo16() == false )
			goto HandleErrorOrCancel;
	}
	else if ( _bitDepth == 32 )
	{
		//  We need to increase the images to 32-bit.

		//  Increase the images to 32-bit.  If any of the
		//  increases fails, handle the error and return.
		if ( pImage->IncreaseTo32() == false ||
			pImage1_2->IncreaseTo32() == false ||
			pImage1_4->IncreaseTo32() == false ||
			pImage1_8->IncreaseTo32() == false )
			goto HandleErrorOrCancel;
	}

	//  Check to see if the user wants to cancel.
	if ( theApp.FlushMessageQueue() == FALSE )
		goto HandleErrorOrCancel;

	//  Save the image pointers.
	if ( type == 0 )
	{
		apBases[ index ][ 0 ] = pImage;
		apBases[ index ][ 1 ] = pImage1_2;
		apBases[ index ][ 2 ] = pImage1_4;
		apBases[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 1 )
	{
		apAnim1[ index ][ 0 ] = pImage;
		apAnim1[ index ][ 1 ] = pImage1_2;
		apAnim1[ index ][ 2 ] = pImage1_4;
		apAnim1[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 2 )
	{
		apAnim2[ index ][ 0 ] = pImage;
		apAnim2[ index ][ 1 ] = pImage1_2;
		apAnim2[ index ][ 2 ] = pImage1_4;
		apAnim2[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 3 )
	{
		apOverlay[ index ][ 0 ] = pImage;
		apOverlay[ index ][ 1 ] = pImage1_2;
		apOverlay[ index ][ 2 ] = pImage1_4;
		apOverlay[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 4 )
	{
		apUnderAnim1[ index ][ 0 ] = pImage;
		apUnderAnim1[ index ][ 1 ] = pImage1_2;
		apUnderAnim1[ index ][ 2 ] = pImage1_4;
		apUnderAnim1[ index ][ 3 ] = pImage1_8;
	}
	else if ( type == 5 )
	{
		apUnderAnim2[ index ][ 0 ] = pImage;
		apUnderAnim2[ index ][ 1 ] = pImage1_2;
		apUnderAnim2[ index ][ 2 ] = pImage1_4;
		apUnderAnim2[ index ][ 3 ] = pImage1_8;
	}
	else
		ASSERT( false );

	return TRUE;

HandleErrorOrCancel:;
	delete pImage;
	delete pImage1_2;
	delete pImage1_4;
	delete pImage1_8;
	delete pDitheredImage;

	return FALSE;
}

BOOL VehicleView::SaveImage( FILE *pFile, Image *pImage )
{
	ASSERT( pFile != NULL );
	ASSERT( pImage != NULL );

	char 	id[ 2 ] = { '0', '0' };
	short	width = ( short )pImage->_width;
	short	height = ( short )pImage->_height;
	short	xoff = ( short )pImage->_xoff;
	short	yoff = ( short )pImage->_yoff;
	short	holdTime = ( short )pImage->_holdTime;

	int dataSize = width * height * pImage->_bytesPerPixel;

	//  Write everything in the FB except the data.
	if ( fwrite( &id, 1, sizeof( id ), pFile ) != sizeof( id ) ||
		fwrite( &holdTime, 1, sizeof( holdTime ), pFile ) != sizeof( holdTime ) ||
		fwrite( &xoff, 1, sizeof( xoff ), pFile ) != sizeof( xoff ) ||
		fwrite( &yoff, 1, sizeof( yoff ), pFile ) != sizeof( yoff ) ||
		fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fwrite( &dataSize, 1, sizeof( dataSize ), pFile ) != sizeof( dataSize ) )
		return FALSE;

	//  Save the cropped bits directly ( no compression ).
	if ( fwrite( pImage->_pPixels, 1, ( size_t )dataSize, pFile ) != ( size_t )dataSize )
		return FALSE;

	return TRUE;
}

Image *VehicleView::LoadImage( FILE *pFile, char const * pszFileName )
{
	char 	id[ 2 ];
	short	holdTime;
	short	xoff, yoff;
	short	width, height;
	int		size;

	if ( fread( &id, 1, sizeof( id ), pFile ) != sizeof( id ) ||
		fread( &holdTime, 1, sizeof( holdTime ), pFile ) != sizeof( holdTime ) ||
		fread( &xoff, 1, sizeof( xoff ), pFile ) != sizeof( xoff ) ||
		fread( &yoff, 1, sizeof( yoff ), pFile ) != sizeof( yoff ) ||
		fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &size, 1, sizeof( size ), pFile ) != sizeof( size ) )
 		return NULL;

	Image *pImage = new Image( width, height, _bitDepth, theApp.aPalette );
	if ( pImage == NULL )
		return NULL;

	pImage->strFileName = pszFileName;

	pImage->SetSkip( RGBColor( 255, 0, 255 ) );

	pImage->_xoff = xoff;
	pImage->_yoff = yoff;
	pImage->_holdTime = holdTime;

	if ( fread( pImage->_pPixels, 1, ( size_t )size, pFile ) != ( size_t )size )
		return NULL;

	return pImage;
}

// GG:
void View::GetHotSpots( Image *pImage, float fMul )
{
	assert( pImage != NULL );

	BOOL	bReportErrors = 24 == _bitDepth;	// Avoid repeating error messages
	pImage->ScanForHotSpots( bReportErrors, fMul );

	if ( bAnchorPointIsValid )
	{
		if ( pImage->bAnchorPointIsValid && bReportErrors )
		{
			CString str;

			str.Format( "Multiple anchor points defined! - at ( %d, %d )\nFile %s\n",
							pImage->anchorPoint.iX,
							pImage->anchorPoint.iY,
							( LPCTSTR )pImage->strFileName );

			theApp.AddErrorDialog( str );	// GG: 8/25/96
		}
	}
	else if ( pImage->bAnchorPointIsValid )
	{
		bAnchorPointIsValid = TRUE;

		anchorPoint = pImage->anchorPoint;
	}

	for ( int i = 0; i < pImage->nAttachedSpriteCnt; ++i )
	{
		assert( nAttachedSpriteCnt < MAX_ATTACHED_SPRITES );

		int	iType  = pImage->aAttachedSprites[ i ].iType;
		int	iIndex = pImage->aAttachedSprites[ i ].iIndex;
		BOOL	bDuplicate = FALSE;

		for ( int j = 0; j < nAttachedSpriteCnt; j++ )
		{
			if ( aAttachedSprites[ j ].iType  == iType &&
				  aAttachedSprites[ j ].iIndex == iIndex )
			{
				if ( bReportErrors )
				{
					CString str;

					str.Format( "Duplicate %s at ( %d, %d ), index %d\nFile %s\n",
									g_apszHotSpotName[ iType ],
									pImage->aAttachedSprites[ i ].iX,
									pImage->aAttachedSprites[ i ].iY,
									pImage->aAttachedSprites[ i ].iIndex,
									( LPCTSTR )pImage->strFileName );

					theApp.AddErrorDialog( str );	// GG 8/25/96
				}

				bDuplicate = TRUE;

				break;
			}
		}

		if ( !bDuplicate )
		{
			aAttachedSprites[ nAttachedSpriteCnt ] = pImage->aAttachedSprites[ i ];

			nAttachedSpriteCnt++;
		}
	}
}
