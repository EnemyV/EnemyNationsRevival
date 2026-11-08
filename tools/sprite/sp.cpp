#include "stdafx.h"
#include "sprite.h"

#include <assert.h>

#include "sp.h"

#ifdef NDEBUG
#pragma warning( disable: 4100 )	//  Get this warning if I just assert on inputs to a function
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp	theApp;

Sprite::Sprite()
{
	viewAllocCnt = 0;
	viewCnt = 0;
	ppViews = NULL;
	pCurrentViewInfo = NULL;
	width = 0;
	height = 0;

	//  Set invalid type.  Derived classes should change this
	//  in their constructors.
	type = INVALID_SPRITE;
}

Sprite::~Sprite()
{
	int viewIndex;

	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
		delete ppViews[ viewIndex ];

	delete[] ppViews;
}

BOOL 
Sprite::Save( 
	const char *pPath, 
	BOOL			bShareware,
	BOOL			bReportErrors )
{
	return SaveOldSPR( pPath, bShareware ) && 
			 SaveNewSPR( pPath, bShareware, bReportErrors );
}

BOOL Sprite::AddView( View *pView )
{
 	assert( pView != NULL );

	//  Allocate more pointers if necessary.
	if ( viewAllocCnt <= viewCnt )
	{
		//  Make sure we allocate some on the first pass through.
		if ( viewAllocCnt == 0 )
			viewAllocCnt = 25;

		//  Allocate twice as many as we already have.
		viewAllocCnt = viewAllocCnt * 2;
		View **ppNewViews = new View * [ viewAllocCnt ];
		if ( ppNewViews == NULL )
			return FALSE;
		( void )memset( ppNewViews, 0, viewAllocCnt * sizeof( View * ) );

		//  Copy the old ones into the new ones.
		int viewIndex;
		for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
			ppNewViews[ viewIndex ] = ppViews[ viewIndex ];

		//  Delete the old ones and update the pointer to point
		//  to the new ones.
		delete[] ppViews;
		ppViews = ppNewViews;
	}

	//  Update width and height if necessary.
	if ( width < pView->GetWidth() )
		width = pView->GetWidth();
	if ( height < pView->GetHeight() )
		height = pView->GetHeight();

	//  Add the pointer to the table.
	ppViews[ viewCnt++ ] = pView;

	//  Flush the message queue.
	if ( theApp.FlushMessageQueue() == FALSE )
		return FALSE;

	return TRUE;
}

BOOL Sprite::CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal )
{
	ASSERT( pDirectory != NULL );
	ASSERT( bitDepth == 8 ||
		bitDepth == 15 ||
		bitDepth == 16 ||
		bitDepth == 24 ||
		bitDepth == 32 );

	_bitDepth = bitDepth;

	if ( _bitDepth == 8 )
	{
		ASSERT( pPal != NULL );
		memcpy( aPalette, pPal, sizeof( RGBColor ) * 256 );
	}

	return TRUE;
} 

void Sprite::HandleTreeSelChange( void *pNewTreeViewInfo )
{
	//  If there is view info for the newly selected 
	//  tree node, save it.  Otherwise, keep the old
	//  info.
	if ( pNewTreeViewInfo )
		pCurrentViewInfo = ( TREEVIEWINFO * )pNewTreeViewInfo;
}

void Sprite::Animate( int frames )
{
	//  If there is a current view, tell it to animate.
	if ( pCurrentViewInfo )
		pCurrentViewInfo->pView->Animate( frames );
}

void Sprite::Render( DrawSurface *pDrawSurf )
{
	//  if no current view, do nothing
	//  Otherwise, call render for that view
	//  with the zoom and index.
	if ( pCurrentViewInfo )
		pCurrentViewInfo->pView->Render( 
			pCurrentViewInfo->zoom,
			pCurrentViewInfo->index,
			pCurrentViewInfo->pUser,
			pDrawSurf );
}

/******************************************
 *
 *  BuildingSprite
 *
 ******************************************/

char *BuildingSprite::constName[ MAX_CONST ] = 
{
	"Finished",
	"Finished - Foreground",
	"Finished - Background",
	"Foundation",
	"Foundation - Foreground",
	"Foundation - Background",
	"Skeleton",
	"Skeleton - Foreground",
	"Skeleton - Background",
	"Construction",
	"Construction - Foreground",
	"Construction - Background",
};

char *BuildingSprite::damName[ MAX_DAMAGE ] =
{
	"No Damage",
	"Mild Damage",
	"Heavy Damage",
	"Engulfed In Flames"
};

char *BuildingSprite::dirName[ MAX_DIR ] = 
{
	"NorthWest",
	"NorthEast",
	"SouthEast",
	"SouthWest",
};

BuildingSprite::BuildingSprite( void )
{
	//  Set correct type
	type = BUILDING_SPRITE;

	//  Make sure all views are initially NULL.
	memset( aViews, 0, MAX_CONST * MAX_DAMAGE * MAX_DIR * sizeof( View * ) );

	pBuildViewInfo = NULL;
}

BuildingSprite::~BuildingSprite()
{
	while( pBuildViewInfo != NULL )
	{
		TBuildViewInfo *pDelBVI = pBuildViewInfo;
		pBuildViewInfo = pBuildViewInfo->pNextViewInfo;
		delete pDelBVI;
	}
}

BOOL BuildingSprite::CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal )
{
	int constIndex, damIndex, dirIndex;

	if ( Sprite::CreateSprite( pDirectory, bitDepth, pPal ) == FALSE )
		return FALSE;

	for( constIndex = 0; constIndex < MAX_CONST; constIndex++ )
	{
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				char aPattern[ 8 ];
				aPattern[ 0 ] = ( char )( 'A' + constIndex );
				aPattern[ 1 ] = ( char )( 'A' + dirIndex * 2 );
				aPattern[ 2 ] = ( char )( '0' + damIndex );
				aPattern[ 3 ] = '\0';

				BuildingView *pView = new BuildingView;
				if ( pView == NULL )
					return FALSE;

				if ( pView->CreateView( this, pDirectory, aPattern, _bitDepth ) == FALSE )
				{
					if ( GetLastError() != USER_ERROR_NO_FILES )
					{
						delete pView;
						return FALSE;
					}
	
					delete pView;

/*
					int prevDamIndex, prevDirIndex, prevConstIndex;
					for( prevDirIndex = dirIndex; prevDirIndex >= 0; prevDirIndex-- )
					{
						for( prevConstIndex = constIndex; prevConstIndex >= 0; prevConstIndex-- )
						{
							for( prevDamIndex = damIndex; prevDamIndex >= 0; prevDamIndex-- )
							{
								if ( aViews[ prevConstIndex ][ prevDirIndex ][ prevDamIndex ] )
								{
									aViews[ constIndex ][ dirIndex ][ damIndex ] = 
										aViews[ prevConstIndex ][ prevDirIndex ][ prevDamIndex ];
									goto gotView;
								}
							}
						}
					}
gotView:;
*/

/*
					int tempDamIndex = damIndex - 1;
					while( tempDamIndex >= 0 )
					{
						if ( aViews[ constIndex ][ dirIndex ][ tempDamIndex ] )
							aViews[ constIndex ][ dirIndex ][ damIndex ] = 
								aViews[ constIndex ][ dirIndex ][ tempDamIndex ];
						tempDamIndex--;
					}
*/
				}
				else
				{
					//  Check overlay conditions.
					if ( pView->overlayCnt )
					{
						if ( pView->baseCnt )
						{
							theApp.AddErrorDialog(CString (pDirectory) + " - cannot have base and overlay for same damage level - Check filenames." );
							return FALSE;
						}
						if ( damIndex == 0 )
						{
							theApp.AddErrorDialog(CString (pDirectory) + " - cannot have overlay for damage level 0" );
							return FALSE;
						}
						if ( aViews[ constIndex ][ dirIndex ][ 0 ] == NULL ||
							aViews[ constIndex ][ dirIndex ][ 0 ]->baseCnt == 0 )
						{
							theApp.AddErrorDialog(CString (pDirectory) + " - must have base bitmaps for damage level 0 in order to use overlay bitmaps." );
							return FALSE;
						}
					}

					// Copy from damage level 0 if necessary and applicable.
					//	GG Say what? if ( damIndex > 0 && damIndex < 2 )

					BOOL	bCopyAnims = 1 == damIndex;

					if ( damIndex > 0 )
						pView->BorrowFromView( aViews[ constIndex ][ dirIndex ][ 0 ], bCopyAnims );

					if ( AddView( pView ) == FALSE )
					{
						delete pView;
						return FALSE;
					}

					aViews[ constIndex ][ dirIndex ][ damIndex ] = pView;
				}

				theApp.UpdateProgressDialog();
			}
		}
	}

	//  If there are no views ( which means no .fb's were found )
	//  warn the user.
	if ( viewCnt == 0 )
	{
		theApp.AddErrorDialog (CString (pDirectory) + " - No files found!");
		return FALSE;
	}

	return TRUE;
}

BOOL BuildingSprite::AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView )
{
	assert( pTreeCtrl != NULL );

	//  Create the root.
	CString	str;
	str.Format( "%d bit", _bitDepth );
	HTREEITEM hTreeRoot = pTreeCtrl->AddNewItem( hParent, str, 0, ( HTREEITEM )TVI_LAST );

	HTREEITEM hConst = NULL, hDam = NULL, hDir = NULL;

	BOOL	bFirstView = bSelectThisView;

	int constIndex, damIndex, dirIndex;
	for( constIndex = 0; constIndex < MAX_CONST; constIndex++ )
	{
		hConst = NULL;
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			hDir = NULL;
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				if ( aViews[ constIndex ][ dirIndex ][ damIndex ] )
				{
					//  Add this construction level if necessary.
					if ( hConst == NULL )
						hConst = pTreeCtrl->AddNewItem( hTreeRoot, constName[ constIndex ], 0, ( HTREEITEM )TVI_LAST );

					//  Add this direction if necessary.
					if ( hDir == NULL )
					{
						assert( hConst != NULL );
						hDir = pTreeCtrl->AddNewItem( hConst, dirName[ dirIndex ], 0, ( HTREEITEM )TVI_LAST );
					}

					//  Add this damage
					hDam = pTreeCtrl->AddNewItem( hDir, damName[ damIndex ], 0, ( HTREEITEM )TVI_LAST );

					//  If necessary, create a node that will tell the building render to
					//  render another view simultaneously with this one.
					TBuildViewInfo *pViewInfo = NULL;
					if ( constIndex == 1 || constIndex == 4 || constIndex == 7 || constIndex == 10 )
					{
						pViewInfo = new TBuildViewInfo;
						if ( pViewInfo == NULL )
							return FALSE;
						pViewInfo->bForeground = false;
						pViewInfo->pOtherView = aViews[ constIndex + 1 ][ dirIndex ][ damIndex ];
						//  If the other view is NULL, I borrow from damage level 0.
						if ( pViewInfo->pOtherView == NULL )
							pViewInfo->pOtherView = aViews[ constIndex + 1 ][ dirIndex ][ 0 ];
						pViewInfo->pNextViewInfo = pBuildViewInfo;
						pBuildViewInfo = pViewInfo;
					}
					else if ( constIndex == 2 || constIndex == 5 || constIndex == 8 || constIndex == 11 )
					{
						pViewInfo = new TBuildViewInfo;
						if ( pViewInfo == NULL )
							return FALSE;
						pViewInfo->bForeground = false;
						pViewInfo->pOtherView = aViews[ constIndex - 1 ][ dirIndex ][ damIndex ];
						//  If the other view is NULL, I borrow from damage level 0.
						if ( pViewInfo->pOtherView == NULL )
							pViewInfo->pOtherView = aViews[ constIndex - 1 ][ dirIndex ][ damIndex ];
						pViewInfo->pNextViewInfo = pBuildViewInfo;
						pBuildViewInfo = pViewInfo;
					}

					//  Add the view.
					aViews[ constIndex ][ dirIndex ][ damIndex ]->AddToTree( pTreeCtrl, hDam, bFirstView, pViewInfo );
					bFirstView = FALSE;
				}
			}
		}
	}

	return TRUE;
}

BOOL BuildingSprite::SaveOldSPR( const char *pPath, BOOL bShareware )
{
	assert( pPath != NULL );

	FILE *pFile = fopen( pPath, "wb" );
	if ( pFile == NULL )
		return FALSE;

	if ( fwrite( &type, 1, sizeof( type ), pFile ) != sizeof( type ) ||
		fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fwrite( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return FALSE;

	//  Save the view count
	if ( fwrite( &viewCnt, 1, sizeof( viewCnt ), pFile ) != sizeof( viewCnt ) )
		return FALSE;

	//  Save the views.
	int viewIndex;
	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
		if ( ppViews[ viewIndex ]->Save( pFile, bShareware ) == FALSE )
			return FALSE;

	//  Save the pointers to the views.
	int constIndex, dirIndex, damIndex;
	for( constIndex = 0; constIndex < MAX_CONST; constIndex++ )
	{
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				if ( aViews[ constIndex ][ dirIndex ][ damIndex ] == NULL )
				{
					int null = -1;
					if ( fwrite( &null, 1, sizeof( null ), pFile ) != sizeof( null ) )
						return FALSE;
				}
				else
				{
					for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
					{
						if ( ppViews[ viewIndex ] == aViews[ constIndex ][ dirIndex ][ damIndex ] )
						{
							if ( fwrite( &viewIndex, 1, sizeof( viewIndex ), pFile ) != sizeof( viewIndex ) )
								return FALSE;
							break;
						}
					}
					assert( viewIndex < viewCnt );
				}
			}
		}
	}

	fflush( pFile );
	fclose( pFile );

	return TRUE;
}

BOOL BuildingSprite::Load( const char *pPath )
{
	assert( pPath != NULL );

	FILE *pFile = fopen( pPath, "rb" );
	if ( pFile == NULL )
		return FALSE;

	if ( fread( &type, 1, sizeof( type ), pFile ) != sizeof( type ) ||
		type != BUILDING_SPRITE )
		return FALSE;

	if ( fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return FALSE;

	//  Read the count.
	if ( fread( &viewCnt, 1, sizeof( viewCnt ), pFile ) != sizeof( viewCnt ) )
		return FALSE;

	//  Alloc the view array.
	ppViews = new View * [ viewCnt ];
	if ( ppViews == NULL )
		return FALSE;
	( void )memset( ppViews, 0, viewCnt * sizeof( View * ) );

	//  Read the views.
	int viewIndex;
	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
	{
		ppViews[ viewIndex ] = new BuildingView;
		if ( ppViews[ viewIndex ]->Load( pFile, pPath ) == FALSE )
			return FALSE;
	}

	//  Read the pointers to the views.
	int constIndex, damIndex, dirIndex;
	for( constIndex = 0; constIndex < MAX_CONST; constIndex++ )
	{
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				int index;
				if ( fread( &index, 1, sizeof( index ), pFile ) != sizeof( index ) )
					return FALSE;
				if ( index == -1 )
					aViews[ constIndex ][ dirIndex ][ damIndex ] = NULL;
				else
				{
					//  Check overlay conditions.
					if ( ppViews[ index ]->overlayCnt )
					{
						if ( ppViews[ index ]->baseCnt )
						{
							theApp.AddErrorDialog(CString (pPath) + " - cannot have base and overlay for same damage level - Check filenames." );
							return FALSE;
						}
						if ( damIndex == 0 )
						{
							theApp.AddErrorDialog(CString (pPath) + " - cannot have overlay for damage level 0\n" );
							return FALSE;
						}
						if ( aViews[ constIndex ][ dirIndex ][ 0 ] == NULL ||
							aViews[ constIndex ][ dirIndex ][ 0 ]->baseCnt == 0 )
						{
							theApp.AddErrorDialog(CString (pPath) + " - must have base bitmaps for damage level 0 in order to use overlay bitmaps." );
							return FALSE;
						}
					}

					if ( damIndex != 0 )
						ppViews[ index ]->BorrowFromView( aViews[ constIndex ][ dirIndex ][ 0 ], TRUE );

					aViews[ constIndex ][ dirIndex ][ damIndex ] = ppViews[ index ];
				}
			}
		}
	}

	fflush( pFile );
	fclose( pFile );
	
	return TRUE;
}

/******************************************
 *
 *  VehicleSprite
 *
 ******************************************/

char *VehicleSprite::tiltName[ MAX_TILT ] =
{
	"Flat",
	"Toward viewer",
	"Away from viewer"
};

char *VehicleSprite::damName[ MAX_DAMAGE ] =
{
	"No Damage",
	"Mild Damage",
	"Heavy Damage",
	"Engulfed In Flames"
};

char *VehicleSprite::dirName[ MAX_DIR ] = 
{
	"NorthWest",
	"North",
	"NorthEast",
	"East",
	"SouthEast",
	"South",
	"SouthWest",
	"West",
};

VehicleSprite::VehicleSprite( void )
{
	//  Set type.
	type = VEHICLE_SPRITE;

	//  Make sure all views are initially NULL.
	memset( aViews, 0, MAX_TILT * MAX_DAMAGE * MAX_DIR * sizeof( View * ) );
}

BOOL VehicleSprite::CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal )
{
	int damIndex, dirIndex, tiltIndex;

	if ( Sprite::CreateSprite( pDirectory, bitDepth, pPal ) == FALSE )
		return FALSE;

	for( tiltIndex = 0; tiltIndex < MAX_TILT; tiltIndex++ )
	{
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				char aPattern[ 8 ];
				aPattern[ 0 ] = 'A';
				aPattern[ 1 ] = ( char )( 'A' + tiltIndex * MAX_DIR + dirIndex );
				aPattern[ 2 ] = ( char )( '0' + damIndex );
				aPattern[ 3 ] = '\0';

				VehicleView *pView = new VehicleView;
				if ( pView == NULL )
					return FALSE;

				if ( pView->CreateView( this, pDirectory, aPattern, _bitDepth ) == FALSE )
				{
					if ( GetLastError() != USER_ERROR_NO_FILES )
						return FALSE;

					delete pView;

/*
					int tempDamIndex = damIndex - 1;
					while( tempDamIndex >= 0 )
					{
						if ( aViews[ tiltIndex ][ dirIndex ][ tempDamIndex ] )
							aViews[ tiltIndex ][ dirIndex ][ damIndex ] = aViews[ tiltIndex ][ dirIndex ][ tempDamIndex ];
						tempDamIndex--;
					}
*/
				}
				else
				{
					//  Check overlay conditions.
					if ( pView->overlayCnt )
					{
						if ( pView->baseCnt )
						{
							theApp.AddErrorDialog(CString (pDirectory) + " - cannot have base and overlay for same damage level - Check filenames." );
							return FALSE;
						}
						if ( damIndex == 0 )
						{
							theApp.AddErrorDialog(CString (pDirectory) + " - cannot have overlay for damage level 0" );
							return FALSE;
						}
						if ( aViews[ tiltIndex ][ dirIndex ][ 0 ] == NULL ||
							aViews[ tiltIndex ][ dirIndex ][ 0 ]->baseCnt == 0 )
						{
							theApp.AddErrorDialog(CString (pDirectory) + " - must have base bitmaps for damage level 0 in order to use overlay bitmaps." );
							return FALSE;
						}
					}

					if ( damIndex != 0 )
						pView->BorrowFromView( aViews[ tiltIndex ][ dirIndex ][ 0 ], TRUE );

					if ( AddView( pView ) == FALSE )
						return FALSE;

					aViews[ tiltIndex ][ dirIndex ][ damIndex ] = pView;
				}

				theApp.UpdateProgressDialog();
			}
		}
	}

	//  If no matching files were found, return false.
	if ( viewCnt == 0 )
	{
		theApp.AddErrorDialog (CString (pDirectory) + " - No files found!");
		return FALSE;
	}

	return TRUE;
}

BOOL VehicleSprite::AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView )
{
	assert( pTreeCtrl != NULL );

	//  Create the root.
	CString	str;
	str.Format( "%d bit", _bitDepth );
	HTREEITEM hTreeRoot = pTreeCtrl->AddNewItem( hParent, str, 0, ( HTREEITEM )TVI_LAST );

	HTREEITEM hDam = NULL, hDir = NULL, hTilt = NULL;

	BOOL	bFirstView = bSelectThisView;

	int damIndex, dirIndex, tiltIndex;
	for( tiltIndex = 0; tiltIndex < MAX_TILT; tiltIndex++ )
	{
		hTilt = NULL;
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			hDir = NULL;
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				if ( aViews[ tiltIndex ][ dirIndex ][ damIndex ] )
				{
					//  Add this tilt if necessary.
					if ( hTilt == NULL )
						hTilt = pTreeCtrl->AddNewItem( hTreeRoot, tiltName[ tiltIndex ], 0, ( HTREEITEM )TVI_LAST );

					//  Add this direction if necessary.
					if ( hDir == NULL )
					{
						assert( hTilt != NULL );
						hDir = pTreeCtrl->AddNewItem( hTilt, dirName[ dirIndex ], 0, ( HTREEITEM )TVI_LAST );
					}

					//  Add this damage
					hDam = pTreeCtrl->AddNewItem( hDir, damName[ damIndex ], 0, ( HTREEITEM )TVI_LAST );

					//  Add the view.
					aViews[ tiltIndex ][ dirIndex ][ damIndex ]->AddToTree( pTreeCtrl, hDam, bFirstView );
					bFirstView = FALSE;
				}
			}
		}
	}

	return TRUE;
}

BOOL VehicleSprite::SaveOldSPR( const char *pPath, BOOL bShareware )
{
	assert( pPath != NULL );

	FILE *pFile = fopen( pPath, "wb" );
	if ( pFile == NULL )
		return FALSE;

	if ( fwrite( &type, 1, sizeof( type ), pFile ) != sizeof( type ) ||
		fwrite( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fwrite( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fwrite( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return false;

	//  Save the view count
	if ( fwrite( &viewCnt, 1, sizeof( viewCnt ), pFile ) != sizeof( viewCnt ) )
		return false;

	//  Save the views.
	int viewIndex;
	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
		if ( ppViews[ viewIndex ]->Save( pFile, bShareware ) == FALSE )
			return FALSE;

	//  Save the pointers to the views.
	int dirIndex, damIndex, tiltIndex;
	for( tiltIndex = 0; tiltIndex < MAX_TILT; tiltIndex++ )
	{
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				if ( aViews[ tiltIndex ][ dirIndex ][ damIndex ] == NULL )
				{
					int null = -1;
					if ( fwrite( &null, 1, sizeof( null ), pFile ) != sizeof( null ) )
						return FALSE;
				}
				else
				{
					for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
					{
						if ( ppViews[ viewIndex ] == aViews[ tiltIndex ][ dirIndex ][ damIndex ] )
						{
							if ( fwrite( &viewIndex, 1, sizeof( viewIndex ), pFile ) != sizeof( viewIndex ) )
								return FALSE;
							break;
						}
					}
					assert( viewIndex < viewCnt );
				}
			}
		}
	}

	fflush( pFile );
	fclose( pFile );

	return TRUE;
}

BOOL VehicleSprite::Load( const char *pPath )
{
	assert( pPath != NULL );

	FILE *pFile = fopen( pPath, "rb" );
	if ( pFile == NULL )
		return FALSE;

	if ( fread( &type, 1, sizeof( type ), pFile ) != sizeof( type ) ||
		type != VEHICLE_SPRITE )
		return FALSE;

	if ( fread( &width, 1, sizeof( width ), pFile ) != sizeof( width ) ||
		fread( &height, 1, sizeof( height ), pFile ) != sizeof( height ) ||
		fread( &_bitDepth, 1, sizeof( _bitDepth ), pFile ) != sizeof( _bitDepth ) )
		return FALSE;

	//  Read the count.
	if ( fread( &viewCnt, 1, sizeof( viewCnt ), pFile ) != sizeof( viewCnt ) )
		return FALSE;

	//  Alloc the view array.
	ppViews = new View * [ viewCnt ];
	if ( ppViews == NULL )
		return FALSE;
	( void )memset( ppViews, 0, viewCnt * sizeof( View * ) );

	//  Read the views.
	int viewIndex;
	for( viewIndex = 0; viewIndex < viewCnt; viewIndex++ )
	{
		ppViews[ viewIndex ] = new VehicleView;
		if ( ppViews[ viewIndex ]->Load( pFile, pPath ) == FALSE )
			return FALSE;
	}

	//  Read the pointers to the views.
	int damIndex, dirIndex, tiltIndex;
	for( tiltIndex = 0; tiltIndex < MAX_TILT; tiltIndex++ )
	{
		for( dirIndex = 0; dirIndex < MAX_DIR; dirIndex++ )
		{
			for( damIndex = 0; damIndex < MAX_DAMAGE; damIndex++ )
			{
				int index;
				if ( fread( &index, 1, sizeof( index ), pFile ) != sizeof( index ) )
					return false;
				if ( index == -1 )
					aViews[ tiltIndex ][ dirIndex ][ damIndex ] = NULL;
				else
				{
					//  Check overlay conditions.
					if ( ppViews[ index ]->overlayCnt )
					{
						if ( ppViews[ index ]->baseCnt )
						{
							theApp.AddErrorDialog(CString (pPath) + " - cannot have base and overlay for same damage level - Check filenames." );
							return FALSE;
						}
						if ( damIndex == 0 )
						{
							theApp.AddErrorDialog(CString (pPath) + " - cannot have overlay for damage level 0" );
							return FALSE;
						}
						if ( aViews[ tiltIndex ][ dirIndex ][ 0 ] == NULL ||
							aViews[ tiltIndex ][ dirIndex ][ 0 ]->baseCnt == 0 )
						{
							theApp.AddErrorDialog(CString (pPath) + " - must have base bitmaps for damage level 0 in order to use overlay bitmaps." );
							return FALSE;
						}
					}

					if ( damIndex != 0 )
						ppViews[ index ]->BorrowFromView( aViews[ tiltIndex ][ dirIndex ][ 0 ], TRUE );

					aViews[ tiltIndex ][ dirIndex ][ damIndex ] = ppViews[ index ];
				}
			}
		}
	}

	fflush( pFile );
	fclose( pFile );
	
	return TRUE;
}
