#ifndef SP_H
#define SP_H

#include "sptypes.h"
#include "v.h"

#include "common.h"
#include "drawsurf.h"

class Sprite
{
	protected :

	TSpriteType		type;
	int 			viewCnt, viewAllocCnt;
	View			**ppViews;
	TREEVIEWINFO	*pCurrentViewInfo;
	int				width, height;
	int				_bitDepth;
	RGBColor		aPalette[ 256 ];

	public :

	Sprite( void );
	virtual ~Sprite( void );

	//  Report the current width or height of the 
	//  sprite, which is the width or height of the 
	//  largest view.
	virtual int GetWidth( void )  { return width; }
	virtual int GetHeight( void ) { return height; }
	virtual int GetBitDepth( void ) { return _bitDepth; }
	
	//  Add a view to the table of views.
	virtual BOOL AddView( View *pView );

	//  Create a sprite.  Must be overridden.
	virtual BOOL CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal = NULL ) = 0;

	//  Create the tree view structure that will allow 
	//  access to each view of the sprite.  Must be
	//  overridden.
	virtual BOOL AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView ) = 0;

	//  If the current tree selection changes, handle
	//  the change.  May be overridden for sprites
	//  which want special behavior when the selection
	//  changes.
	virtual void HandleTreeSelChange( void *pNewTreeViewInfo );
	
	//  Increment the animation state of the current
	//  view by the given number of frames.  May be
	//  overridden for sprites which want special
	//  animation behavior.
	virtual void Animate( int frames );

	//  Render the current view into the given DIB.
	virtual void Render( DrawSurface *pDrawSurf );

	//  Save the sprite.  Must be overridden.
	virtual BOOL SaveOldSPR( const char *pPath, BOOL bShareware ) = 0;

	static BOOL SaveNewSPR( char const * pPath, BOOL bShareware, BOOL bReportErrors );

	// Save the sprite in old and new format files
	BOOL Save( const char *pPath, BOOL bShareware, BOOL bReportErrors );

	//  Load the sprite.  Must be overridden.
	virtual BOOL Load( const char *pPath ) = 0;

	//  Mostly for progress indicators.
	virtual int	GetViewCount() const { return 0; }
};

class BuildingSprite : public Sprite
{
	enum
	{
		MAX_CONST = 12, 
		MAX_DIR = 4,
		MAX_DAMAGE = 4,
	};

	static char *constName[ MAX_CONST ];
	static char *damName[ MAX_DAMAGE ];
	static char *dirName[ MAX_DIR ];

	View *aViews[ MAX_CONST ][ MAX_DIR ][ MAX_DAMAGE ];

	TBuildViewInfo *pBuildViewInfo;

	public :

	BuildingSprite();
	virtual ~BuildingSprite();

	virtual BOOL CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal = NULL );

	virtual BOOL AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView );

	virtual BOOL SaveOldSPR( const char *pPath, BOOL bShareware );

	virtual BOOL Load( const char *pPath );

	//  Mostly for progress indicators.
	int	GetViewCount() const { return MAX_CONST * MAX_DIR * MAX_DAMAGE; }
};

class VehicleSprite : public Sprite
{
	enum
	{
		MAX_TILT = 3,
		MAX_DIR = 8,
		MAX_DAMAGE = 4,
	};

	static char *tiltName[ MAX_TILT ];
	static char *damName[ MAX_DAMAGE ];
	static char *dirName[ MAX_DIR ];

	View *aViews[ MAX_TILT ][ MAX_DIR ][ MAX_DAMAGE ];

	public :

	VehicleSprite();
	virtual ~VehicleSprite() { };

	virtual BOOL CreateSprite( const char *pDirectory, int bitDepth, const RGBColor *pPal = NULL );

	virtual BOOL AddToTreeView( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView );

	virtual BOOL SaveOldSPR( const char *pPath, BOOL bShareware );

	virtual BOOL Load( const char *pPath );

	//  Mostly for progress indicators.
	int	GetViewCount() const { return MAX_TILT * MAX_DIR * MAX_DAMAGE; }
};

#endif	//  SP_H
