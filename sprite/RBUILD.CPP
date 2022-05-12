#include <io.h>

#include "stdafx.h"
#include "sprite.h"
#include "sptypes.h"
#include "sp.h"
#include "terrain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

int CountLeafDirs( const char *pBaseDir )
{
	int		cnt = 0;
	char	aDirectory[ MAX_PATH ];

	strcpy( aDirectory, pBaseDir );
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
			cnt += CountLeafDirs( aSubDir );
		}

	FindNext :
		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	//  If there were no subdirectories, this must be a 
	//  leaf directory, so set the count to 1 ( us ).
	if ( cnt == 0 )
		cnt = 1;

	return cnt;
}

bool RecursiveBuild( TSpriteType type, const char *pBaseDir, BOOL bShareware )
{
	ASSERT( pBaseDir != NULL );
	ASSERT( type == BUILDING_SPRITE || type == TERRAIN_SPRITE || type == VEHICLE_SPRITE );

	Sprite *pSprite8 = NULL;
#ifdef CREATE15
	Sprite *pSprite15 = NULL;
#endif
#ifdef CREATE16
	Sprite *pSprite16 = NULL;
#endif
	Sprite *pSprite24 = NULL;
#ifdef CREATE32
	Sprite *pSprite32 = NULL;
#endif

	char	aDirectory[ MAX_PATH ];
	BOOL	bLeafDir = TRUE;

	strcpy( aDirectory, pBaseDir );
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

	bool bRtn = true;

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
			if ( RecursiveBuild( type, aSubDir, bShareware ) == false )
				bRtn = false;
			bLeafDir = FALSE;
		}

	FindNext :
		//  Find the next file.
		if ( _findnext( hFile, &findData ) == -1 )
			bAllFilesRead = TRUE;
	}
	_findclose( hFile );

	//  If this is a leaf dir, then we should create sprites in this
	//  dir.
	if ( bLeafDir == TRUE )
	{
		//  Allocate the sprites, depending on which type of
		//  sprite it is.
		if ( type == BUILDING_SPRITE )
		{
			pSprite8 = new BuildingSprite;
			if ( pSprite8 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 8-bit sprite");
				goto HandleError;
				}

#ifdef CREATE15
			pSprite15 = new BuildingSprite;
			if ( pSprite15 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 15-bit sprite");
				goto HandleError;
				}
#endif

#ifdef CREATE16
			pSprite16 = new BuildingSprite;
			if ( pSprite16 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 16-bit sprite");
				goto HandleError;
				}
#endif

			pSprite24 = new BuildingSprite;
			if ( pSprite24 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 24-bit sprite");
				goto HandleError;
				}

#ifdef CREATE32
			pSprite32 = new BuildingSprite;
			if ( pSprite32 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 32-bit sprite");
				goto HandleError;
				}
#endif
		}
		else if ( type == TERRAIN_SPRITE )
		{
			pSprite8 = new TerrainSprite;
			if ( pSprite8 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 8-bit terrain sprite");
				goto HandleError;
				}

#ifdef CREATE15
			pSprite15 = new TerrainSprite;
			if ( pSprite15 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 15-bit terrain sprite");
				goto HandleError;
				}
#endif

#ifdef CREATE16
			pSprite16 = new TerrainSprite;
			if ( pSprite16 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 16-bit terrain sprite");
				goto HandleError;
				}
#endif

			pSprite24 = new TerrainSprite;
			if ( pSprite24 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 24-bit terrain sprite");
				goto HandleError;
				}

#ifdef CREATE32
			pSprite32 = new TerrainSprite;
			if ( pSprite32 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 32-bit terrain sprite");
				goto HandleError;
				}
#endif
		}
		else if ( type == VEHICLE_SPRITE )
		{
			pSprite8 = new VehicleSprite;
			if ( pSprite8 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 8-bit vehicle sprite");
				goto HandleError;
				}

#ifdef CREATE15
			pSprite15 = new VehicleSprite;
			if ( pSprite15 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 15-bit vehicle sprite");
				goto HandleError;
				}
#endif

#ifdef CREATE16
			pSprite16 = new VehicleSprite;
			if ( pSprite16 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 16-bit vehicle sprite");
				goto HandleError;
				}
#endif

			pSprite24 = new VehicleSprite;
			if ( pSprite24 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 24-bit vehicle sprite");
				goto HandleError;
				}

#ifdef CREATE32
			pSprite32 = new VehicleSprite;
			if ( pSprite32 == NULL )
				{
				bRtn = false;
				theApp.AddErrorDialog (CString (pBaseDir) + " - could not create 32-bit vehicle sprite");
				goto HandleError;
				}
#endif
		}

		//  Update the multi-progress dialog
		theApp.UpdateMultiProgress( pBaseDir );

		//  Create the sprites.
		if ( pSprite8->CreateSprite( pBaseDir, 8, theApp.aPalette ) == FALSE
#ifdef CREATE15
			|| pSprite15->CreateSprite( pBaseDir, 15 ) == FALSE
#endif
#ifdef CREATE16
			|| pSprite16->CreateSprite( pBaseDir, 16 ) == FALSE
#endif
			|| pSprite24->CreateSprite( pBaseDir, 24 ) == FALSE
#ifdef CREATE32
			|| pSprite32->CreateSprite( pBaseDir, 32 ) == FALSE
#endif
			)
			{
			bRtn = false;
			theApp.AddErrorDialog (CString (pBaseDir) + " - could not create sprite");
			goto HandleError;
			}

		//  Get the name for the sprites, which is the name of the
		//  directory.
		char aName[ MAX_PATH ];
		( void )strcpy( aName, pBaseDir );
		if ( aName[ strlen( aName ) - 1 ] != '\\' )
			( void )strcat( aName, "\\" );
		char aBase[ MAX_PATH ];
		( void )strcpy( aBase, pBaseDir );
		if ( aBase[ strlen( aBase ) - 1 ] == '\\' )
			aBase[ strlen( aBase ) - 1 ] = 0;
		const char *pBaseName = strrchr( aBase, '\\' );
		if ( pBaseName == NULL )
			( void )strcat( aName, aBase );
		else
		{
			pBaseName++;	//  Point just past the \.
			( void )strcat( aName, pBaseName );
		}

		//  Save the sprites.
		//  Figure out where in the string the extension goes.  Note that
		//  we need to make sure that we don't clobber an extension that
		//  actually belongs to a directory.
		char *pExt = strrchr( aName, '.' );
		char *pDir = strrchr( aName, '\\' );
		if ( pExt == NULL || pExt < pDir )
			pExt = &aName[ strlen( aName ) ];

		//  Save the 8-bit sprite.
		if ( bShareware )
			( void )strcpy( pExt, ".s8" );
		else
			( void )strcpy( pExt, ".8" );
		if ( pSprite8->Save( aName, bShareware, FALSE ) == FALSE )
			{
			bRtn = false;
			theApp.AddErrorDialog (CString (pBaseDir) + " - could not save 8-bit sprite");
			goto HandleError;
			}

#ifdef CREATE15
		//  Save the 15-bit sprite.
		if ( bShareware )
			( void )strcpy( pExt, ".s15" );
		else
			( void )strcpy( pExt, ".15" );
		if ( pSprite15->Save( aName, bShareware ) == FALSE )
			{
			bRtn = false;
			theApp.AddErrorDialog (CString (pBaseDir) + " - could not save 15-bit sprite");
			goto HandleError;
			}
#endif

#ifdef CREATE16
		//  Save the 16-bit sprite.
		if ( bShareware )
			( void )strcpy( pExt, ".s16" );
		else
			( void )strcpy( pExt, ".16" );
		if ( pSprite16->Save( aName, bShareware ) == FALSE )
			{
			bRtn = false;
			theApp.AddErrorDialog (CString (pBaseDir) + " - could not save 16-bit sprite");
			goto HandleError;
			}
#endif

		//  Save the 24-bit sprite.
		if ( bShareware )
			( void )strcpy( pExt, ".s24" );
		else
			( void )strcpy( pExt, ".24" );
		if ( pSprite24->Save( aName, bShareware, TRUE ) == FALSE )
			{
			bRtn = false;
			theApp.AddErrorDialog (CString (pBaseDir) + " - could not save 24-bit sprite");
			goto HandleError;
			}

#ifdef CREATE32
		//  Save the 32-bit sprite.
		if ( bShareware )
			( void )strcpy( pExt, ".s32" );
		else
			( void )strcpy( pExt, ".32" );
		if ( pSprite32->Save( aName, bShareware ) == FALSE )
			{
			bRtn = false;
			theApp.AddErrorDialog (CString (pBaseDir) + " - could not save 32-bit sprite");
			goto HandleError;
			}
#endif

		//  Delete the sprites, since we're done with them now.
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
	}

	return bRtn;

HandleError:;

	delete pSprite8;
#ifdef CREATE15
	delete pSprite15;
#endif
#ifdef CREATE15
	delete pSprite16;
#endif
	delete pSprite24;
#ifdef CREATE32
	delete pSprite32;
#endif

	return false;
}