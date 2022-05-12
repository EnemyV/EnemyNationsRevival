//  Sprite tree control implementation.
#include "stdafx.h"
#include "holdtime.h"
#include "sprite.h"
#include "spprop.h"
#include "sptrctrl.h"
#include "v.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SpriteTreeCtrl

SpriteTreeCtrl::SpriteTreeCtrl()
{
}

SpriteTreeCtrl::~SpriteTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(SpriteTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(SpriteTreeCtrl)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_COMMAND( ID_SPTREE_PROP, OnSPTreeProp )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SpriteTreeCtrl message handlers

void SetHoldTime( TV_ITEM tvItem, void *pUserData )
{
	TREEVIEWINFO *ptvi = ( TREEVIEWINFO * )tvItem.lParam;

	//  If this isn't a single bitmap, return.
	if ( ptvi == NULL ||
		ptvi->pView == NULL || 
		ptvi->index == -1 )
		return;

	//  Get a pointer to the actual image associated with this
	//  tree item, and set it's hold time.
	Image *pImage = ptvi->pView->GetImage( ptvi->index, ptvi->zoom );
	ASSERT( pImage != NULL );
	pImage->_holdTime = ( int )pUserData;
}

void SpriteTreeCtrl::DoForSubtree( HTREEITEM hItem, TTreeActFunctionPtr pAction, void *pUserData )
{
	ASSERT( hItem != NULL );

	HTREEITEM	hChild, hSibling;

	//  Perform the operation on myself.  This requires
	//  getting a TV_ITEM for myself, since the handle
	//  doesn't really mean anything without the tree.
	TV_ITEM	tvItem;
	char aItemName[ 256 ];
	tvItem.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;
	tvItem.hItem = hItem;
	tvItem.pszText = aItemName;
	tvItem.cchTextMax = sizeof( aItemName );
	if ( GetItem( &tvItem ) == FALSE )
		return;
	(*pAction)( tvItem, pUserData );

	//  If I have no children, I'm done.
	if ( ItemHasChildren( hItem ) == FALSE )
		return;

	//  Otherwise, get my child item.
	hChild = GetChildItem( hItem );

	//  Recurse through first child and all of it's
	//  children.
	DoForSubtree( hChild, pAction, pUserData );

	//  Now, get all of the siblings of the child item
	for( hSibling = GetNextSiblingItem( hChild ); hSibling != NULL; hSibling = GetNextSiblingItem( hSibling ) )
	{
		//  Recurse through this sibling and all of
		//  it's children.
		DoForSubtree( hSibling, pAction, pUserData );
	}
}

void SpriteTreeCtrl::OnRButtonDown(UINT /*nFlags*/, CPoint point) 
{
	HTREEITEM	hHitItem;
	UINT		flags;

	//  Determine whether the right click hit anything in the tree.
	hHitItem = HitTest( point, &flags );
	if ( hHitItem == NULL ||
		( flags & TVHT_ONITEM ) == 0 )
		return;

	//  Get a pointer to the user data for this tree item.
	TV_ITEM	item;
	item.mask = TVIF_PARAM;
	item.hItem = hHitItem;
	if ( GetItem( &item ) == FALSE )
		return;
	TREEVIEWINFO *ptvi = ( TREEVIEWINFO * )item.lParam;

	//  If this isn't a single bitmap, set the hold time
	//  for the subtree.
	if ( ptvi == NULL ||
		ptvi->pView == NULL || 
		ptvi->index == -1 )
	{
		CHoldTime	holdTime;
		holdTime.m_holdTime = 1;
		if ( holdTime.DoModal() == IDCANCEL )
			return;

		int newHoldTime = holdTime.m_holdTime;
		DoForSubtree( hHitItem, SetHoldTime, ( void * )newHoldTime );
		return;
	}

	//  It's a single bitmap.  Select it ( so we can determine if the menu is
	//  selected which bitmap to set the properties for ) and throw up a menu 
	//  which allows editing of properties.
	SelectItem( hHitItem );
	EnsureVisible( hHitItem );
	CMenu	menu;
	menu.CreatePopupMenu();
	menu.AppendMenu( MF_STRING | MF_ENABLED, ID_SPTREE_PROP, "Properties..." );

	ClientToScreen( &point );
	menu.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, this, NULL );
}

void SpriteTreeCtrl::OnSPTreeProp()
{
	CPropertySheet	propSheet;
	CSPProp			propPage;
	
	//  Get a pointer to the actual Image that has been selected.
	HTREEITEM hSelectedItem = GetSelectedItem();
	ASSERT( hSelectedItem != NULL );	//  Since previous function selected the item.
	TV_ITEM	item;
	item.mask = TVIF_PARAM;
	item.hItem = hSelectedItem;
	if ( GetItem( &item ) == FALSE )
		return;
	TREEVIEWINFO *ptvi = ( TREEVIEWINFO * )item.lParam;
	ASSERT( ptvi != NULL );
	ASSERT( ptvi->pView != NULL );
	ASSERT( ptvi->index != -1 );
	Image *pImage = ptvi->pView->GetImage( ptvi->index, ptvi->zoom );
	ASSERT( pImage != NULL );
	
	propPage.m_width = pImage->_width;
	propPage.m_height = pImage->_height;
	propPage.m_xoff = pImage->_xoff;
	propPage.m_yoff = pImage->_yoff;
	propPage.m_holdTime = pImage->_holdTime;

	propSheet.AddPage( &propPage );
	if ( propSheet.DoModal() == IDOK )
	{
		pImage->_xoff = propPage.m_xoff;
		pImage->_yoff = propPage.m_yoff;
		pImage->_holdTime = propPage.m_holdTime;
	}
}

HTREEITEM SpriteTreeCtrl::AddNewItem( HTREEITEM hParent, const char *pszItemText, void *pUserData, HTREEITEM hInsertAfter ) 
{
	//  Set up the tree view item structure.
	TV_ITEM	tvItem;
	tvItem.mask 		= TVIF_TEXT | TVIF_PARAM;
	tvItem.pszText 		= ( char * )pszItemText;	//  Does MS understand const?
	tvItem.cchTextMax	= strlen( pszItemText );
	tvItem.lParam		= ( DWORD )pUserData;

	//  Set up the tree view insert structure.
	TV_INSERTSTRUCT	tvInsertStruct;
	tvInsertStruct.item				= tvItem;
	tvInsertStruct.hInsertAfter 	= hInsertAfter;
	tvInsertStruct.hParent 			= hParent;

	//  Insert the item into the tree control.
	HTREEITEM hItem = InsertItem( &tvInsertStruct );

	//  return the item.
	return hItem;
}

