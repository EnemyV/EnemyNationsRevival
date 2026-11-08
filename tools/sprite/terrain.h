#ifndef TERRAIN_H
#define TERRAIN_H

#include "image.h"
#include "v.h"
#include "sp.h"

class TerrainImage : public Image
{
	public :

/*
	TerrainImage();
	TerrainImage( const TerrainImage & );
	~TerrainImage();
*/
	TerrainImage() {}
	TerrainImage( const TerrainImage &image ) :
		Image( image ) {}
	TerrainImage( int width, int height, int bitDepth, RGBColor *pPal = NULL ) :
		Image( width, height, bitDepth, pPal ) {}

	//  Shade the tile to 8 levels, creating a new
	//  image which is 8 times taller but the same width.
	bool Shade();

	//  Copy opposite corners into each other, creating a
	//  tile which has no magenta in it ( assuming that the
	//  tile is correctly formed ).
	BOOL Square(char const * pFile);
	void UnSquare();

	//  Pack the tile into a (  height + 2 ) x height square,
	//  by removing all of the magenta pixels.
	bool Pack();
	bool UnPack();
};

//  Uncomment this line to check terrain tiles for
//  magenta pixels within the diamond.
#define CHECK_TILES
class TerrainView : public View
{
	protected :

	virtual BOOL SaveImage( FILE *pFile, Image *pImage );
	virtual Image *LoadImage( FILE *pFile, char const * pszFileName );

	public :

	//  Loads the file.  This call loads the file, 
	//  does all of the processing, and creates the
	//  four zooms for this file.
	virtual BOOL LoadFile( const char *pFullPath, int index, int type );
};

class TerrainSprite : public Sprite
{
	enum
	{
		MAX_DIR = 4,
		MAX_DAMAGE = 4,
	};

	static char *damName[ MAX_DAMAGE ];
	static char *dirName[ MAX_DIR ];

	View *aViews[ MAX_DIR ][ MAX_DAMAGE ];

	public :

	TerrainSprite();
	virtual ~TerrainSprite() { };

	virtual int GetHeight() { return height * 8; }

	virtual BOOL CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal = NULL );

	virtual BOOL AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView );

	virtual BOOL SaveOldSPR( const char *pPath, BOOL bShareware );

	virtual BOOL Load( const char *pPath );

	//  Mostly for progress indicators.
	int	GetViewCount() const { return MAX_DIR * MAX_DAMAGE; }
};

#endif
