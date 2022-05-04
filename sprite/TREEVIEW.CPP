// treeview.cpp : implementation file
//

#include <string.h>

#include "stdafx.h"
#include "sprite.h"
#include "sprd.h"
#include "treeview.h"
#include "v.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTView

IMPLEMENT_DYNCREATE(CTView, CScrollView)

CTView::CTView()
{
}

CTView::~CTView()
{
}


BEGIN_MESSAGE_MAP(CTView, CScrollView)
	//{{AFX_MSG_MAP(CTView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY( TVN_SELCHANGED, ID_TREEVIEW, OnSelChanged )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTView drawing

void CTView::OnDraw(CDC* /*pDC*/)
{
	CDocument* pDoc = GetDocument();
}

/////////////////////////////////////////////////////////////////////////////
// CTView diagnostics

#ifdef _DEBUG
void CTView::AssertValid() const
{
	CView::AssertValid();
}

void CTView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTView message handlers

int CTView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if ( m_treeCtl.Create( 
		WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT,	//  Styles - note mix of control styles and window styles.
		CRect( 0, 0, 0, 0),		//	We set it to the window size in OnSize.
		this, 				//  Parent window
		ID_TREEVIEW) == FALSE )
	{
		TRACE0( "Failed to create tree control!\n" );
		return -1;
	}

	SetScrollSizes( MM_TEXT, CSize( 150, 150 ) );

	return 0;
}

void CTView::OnSize(UINT nType, int cx, int cy) 
{
	m_treeCtl.MoveWindow( 0, 0, cx, cy );

	CScrollView::OnSize(nType, cx, cy);
}

void CTView::OnInitialUpdate() 
{
	CScrollView::OnInitialUpdate();
	
	CSprite *pDoc = ( CSprite * )GetDocument();
	ASSERT_VALID( pDoc );

	ASSERT( pDoc->pSprite8 != NULL );
#ifdef CREATE15
	ASSERT( pDoc->pSprite15 != NULL );
#endif
#ifdef CREATE16
	ASSERT( pDoc->pSprite16 != NULL );
#endif
	ASSERT( pDoc->pSprite24 != NULL );
#ifdef CREATE32
	ASSERT( pDoc->pSprite32 != NULL );
#endif

	HTREEITEM hTreeRoot = m_treeCtl.AddNewItem( ( HTREEITEM )NULL, "Root", NULL, ( HTREEITEM )TVI_LAST );
	pDoc->pSprite8->AddToTreeView( &m_treeCtl, hTreeRoot, true );
#ifdef CREATE15
	pDoc->pSprite15->AddToTreeView( &m_treeCtl, hTreeRoot, false );
#endif
#ifdef CREATE16
	pDoc->pSprite16->AddToTreeView( &m_treeCtl, hTreeRoot, false );
#endif
	pDoc->pSprite24->AddToTreeView( &m_treeCtl, hTreeRoot, false );
#ifdef CREATE32
	pDoc->pSprite32->AddToTreeView( &m_treeCtl, hTreeRoot, false );
#endif
}

void CTView::OnSelChanged( NMHDR *pNotifyStruct, LRESULT * /*pResult*/ )
{
	NM_TREEVIEW *pTreeView = ( NM_TREEVIEW * )pNotifyStruct;

	TV_ITEM	item;
	item.mask = TVIF_PARAM;
	item.hItem = pTreeView->itemNew.hItem;
	if ( m_treeCtl.GetItem( &item ) == FALSE )
		return;

	CSprite *pDoc = ( CSprite * )GetDocument();
	ASSERT_VALID( pDoc );

	//  HACK!  Use the bit depth of the view to determine which 
	//  sprite this is.
	Sprite *pCurrentSprite = pDoc->pCurrentSprite;
	if ( item.lParam )
	{
		TREEVIEWINFO *ptvi = ( TREEVIEWINFO * )item.lParam;
		ASSERT( ptvi->pView );
		switch( ptvi->pView->GetBitDepth() )
		{
			default :
				ASSERT( FALSE );
				//  Fall through on purpose for non-debug.

			case 8 :
				pCurrentSprite = pDoc->pSprite8;
				break;

#ifdef CREATE15
			case 15 :
				pCurrentSprite = pDoc->pSprite15;
				break;
#endif

#ifdef CREATE16
			case 16 :
				pCurrentSprite = pDoc->pSprite16;
				break;
#endif

			case 24 :
				pCurrentSprite = pDoc->pSprite24;
				break;

#ifdef CREATE32
			case 32 :
				pCurrentSprite = pDoc->pSprite32;
				break;
#endif
		}
	}

	ASSERT( pCurrentSprite != NULL );
	LPARAM	lHint = 1;
	if ( pCurrentSprite != pDoc->pCurrentSprite )
	{
		pDoc->pCurrentSprite = pCurrentSprite;
		lHint = 0;		//  Want to force a re-create of the display buffer.
	}

	//  Finally, tell the sprite to handle the selection change, and
	//  update all of the views.
	pDoc->pCurrentSprite->HandleTreeSelChange( ( void * )item.lParam );
	pDoc->UpdateAllViews( NULL );

}

void CTView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if ( lHint != 1 )
		CScrollView::OnUpdate( pSender, lHint, pHint );
}

