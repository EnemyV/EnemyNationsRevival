#ifndef V_H
#define V_H

#include "drawsurf.h"
#include "image.h"
#include "sptrctrl.h"
#include "sptypes.h"

//  Forward declaration of View for TREEVIEWINFO.
class View;

//  Forward declaration of Sprite for CreateView.
class Sprite;

//  Structure which is used to add information about
//  a view to a tree control.
struct TREEVIEWINFO
{
	View *	pView;	//  Pointer to the view.
	int		zoom;	//  Zoom level of the view.
	int		index;	//  Index of the bitmap to display
	void *	pUser;	//  Anything a subclass wants to add.
};

class View
{
	//  Sprites have intimate knowledge of their views, so they
	//  should have access to their views.  Unfortunately, making
	//  base classes friends does not make descendents friends, so
	//  I'm making these things public for expediency.

	public :

	enum
	{ 
		MAX_BASE = 9, 
		MAX_ANIM = 26, 
		MAX_OVERLAY = 1,
		MAX_ZOOM = 4
	};

	static char *zoomNames[ MAX_ZOOM ];

	int baseCnt, overlayCnt, anim1Cnt, anim2Cnt;
	int underAnim1Cnt, underAnim2Cnt;
	bool bBorrowedBase, bBorrowedAnim1, bBorrowedAnim2;
	bool bBorrowedUnderAnim1, bBorrowedUnderAnim2;
	Image *apBases[ MAX_BASE ][ MAX_ZOOM ];
	Image *apOverlay[ MAX_OVERLAY ][ MAX_ZOOM ];
	Image *apAnim1[ MAX_ANIM ][ MAX_ZOOM ];
	Image *apAnim2[ MAX_ANIM ][ MAX_ZOOM ];
	Image *apUnderAnim1[ MAX_ANIM ][ MAX_ZOOM ];
	Image *apUnderAnim2[ MAX_ANIM ][ MAX_ZOOM ];

	BOOL		bAnchorPointIsValid;
	THotSpot	anchorPoint;
	int			nAttachedSpriteCnt;
	THotSpot	aAttachedSprites[ MAX_ATTACHED_SPRITES ];

	int				tviCnt, tviAllocCnt;
	TREEVIEWINFO	**pptvi;

	int			anim1Step, anim2Step, underAnim1Step, underAnim2Step;
	int			anim1Frames, anim2Frames, underAnim1Frames, underAnim2Frames;
	int 		width, height;
	int			_bitDepth;
	Sprite *	_pSprite;

	virtual BOOL SaveImage( FILE *pFile, Image *pImage ) = 0;
	virtual Image *LoadImage( FILE *pFile, char const * pszFileName ) = 0;

	BOOL	SaveCompressedImage( FILE *pFile, Image *pImage );

	public :

	View( void );
	virtual ~View( void );

	//  Get the width or height of the view, which is
	//  the width or height of the largest fb in the
	//  view.
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	int GetBitDepth() { return _bitDepth; }

	//  Get a pointer to one of the images in this view.
	Image *GetImage( int index, int zoom );

	//  Return a pointer to a new tree view structure.
	//  This is managed by the view so that all of the
	//  TVI structures will be correctly deleted when the
	//  view is deleted.
	TREEVIEWINFO *GetNewTreeView();

	//  Create a view consisting of all files which 
	//  match the given pattern.  Can be overridden 
	//  for sprite types which don't want the general 
	//  base/anim1/anim2 handling.
	virtual BOOL CreateView( Sprite *pSprite, const char *pDir, const char *pPattern, int bitDepth );

	//  Copies all elements from pCopyView that this view doesn't have.
	//  In other words, if this view has no anim1 bitmaps, copy the ones
	//  from pCopyView, but if this view has it's own anim1 bitmaps ignore
	//  the ones in pCopyView.
	//  Does NOT copy overlays - they are unique to a view.
	virtual void BorrowFromView( const View *pCopyView, BOOL bCopyAnims );

	//  GG: Copy anchor and append hotspots from the image
	virtual void GetHotSpots( Image *pImage, float fMul = 1.0f );

	//  Load a file.  The type indicates whether 
	//  this is a base, an anim1 file, or an anim2 
	//  file.  This function must be overridden.
	//  Note that this function is responsible for
	//  updating the width and height of the view.
	virtual BOOL LoadFile( const char *pFullPath, int index, int type ) = 0;

	//  Adds the necessary information to the tree 
	//  control for this view.  Can be overridden for 
	//  views that want more control over what they 
	//  add to the tree control.
	virtual BOOL AddToTree( SpriteTreeCtrl *pTreeCtrl, HTREEITEM hParent, BOOL bSelectThisView, void *pUserData = NULL );

	//  Increments the current animation step by the 
	//  given number of frames.  Can be overridden for 
	//  views that want special animation behavior.
	virtual void Animate( int frames );
	
	//  Renders the current state of the view, or the
	//  single bitmap represented by the index, into
	//  the DIB.  Can be overridden by views which
	//  have special rendering needs.
	virtual void Render( int zoom, int index, void *pUserData, DrawSurface *pDrawSurf );

	//  Saves the view in the given file.  May be 
	//  overridden.for sprites which need more save 
	//  control ( default saves FB's for all bases and
	//  anims using overidden FB save functions ).
	virtual BOOL Save( FILE *pFile, BOOL bShareware );

	//  Loads the view from the given file.  May be 
	//  overridden.for sprites which need more save 
	//  control ( default saves FB's for all bases and
	//  anims using overidden FB save functions ).
	virtual BOOL Load( FILE *pFile, char const * pszFileName );
};

struct	TBuildViewInfo
{
	bool	bForeground;  //  The view in _this structure_ is the foreground ( drawn last ).
	View *	pOtherView;
	TBuildViewInfo *pNextViewInfo;
};

class BuildingView : public View
{
	protected :

	virtual BOOL SaveImage( FILE *pFile, Image *pImage );
	virtual Image *LoadImage( FILE *pFile, char const * pszFileName );

	public :

	//  Loads the file.  This call loads the file, 
	//  does all of the processing, and creates the
	//  four zooms for this file.
	virtual BOOL LoadFile( const char *pFullPath, int index, int type );

	//  Renders the current state of the view, or the
	//  single bitmap represented by the index, into
	//  the DIB.  Can be overridden by views which
	//  have special rendering needs.
	virtual void Render( int zoom, int index, void *pUserData, DrawSurface *pDrawSurf );
};

class VehicleView : public View
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

#endif	//  V_H

