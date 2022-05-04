// spritvw.cpp : implementation of the CSpriteView class
//

#include "stdafx.h"
#include "sprite.h"

#include "sprd.h"
#include "spritvw.h"

#include "drawdib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSpriteView

IMPLEMENT_DYNCREATE(CSpriteView, CScrollView)

BEGIN_MESSAGE_MAP(CSpriteView, CScrollView)
	//{{AFX_MSG_MAP(CSpriteView)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpriteView construction/destruction

CSpriteView::CSpriteView() : drawsurf()
{
}

CSpriteView::~CSpriteView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSpriteView drawing

void CSpriteView::OnDraw(CDC* pDC)
{
	ASSERT( pDC != NULL );
	CSprite *pDoc = GetDocument();
	ASSERT_VALID( pDoc );
	ASSERT( pDoc != NULL );
	ASSERT( pDoc->pCurrentSprite != NULL );

	memset( drawsurf.GetPixels(), 255, drawsurf.GetStride() * drawsurf.GetHeight() );
	pDoc->pCurrentSprite->Render( &drawsurf );
	drawsurf.Blit( pDC->GetSafeHdc() );
}

/////////////////////////////////////////////////////////////////////////////
// CSpriteView printing

BOOL CSpriteView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSpriteView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// NOTE: add extra initialization before printing
}

void CSpriteView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// NOTE: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CSpriteView diagnostics

#ifdef _DEBUG
void CSpriteView::AssertValid() const
{
	CView::AssertValid();
}

void CSpriteView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSprite* CSpriteView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSprite)));
	return (CSprite*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpriteView message handlers

void CSpriteView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/) 
{

	//  If I wanted to get fancy I could actually use the hints, but
	//  the important thing is to make sure the background doesn't get
	//  erased every time.
	if ( lHint == 1 )
		Invalidate( FALSE );
	else
	{
		CSprite *pDoc = GetDocument();
		ASSERT_VALID( pDoc );
		ASSERT( pDoc->pCurrentSprite != NULL );

		//  Re-create the drawing surface in the correct bit depth.
		drawsurf.Create( pDoc->pCurrentSprite->GetWidth(), pDoc->pCurrentSprite->GetHeight(), pDoc->pCurrentSprite->GetBitDepth(), theApp.aPalette );
		Invalidate( TRUE );	
	}
}

void CSpriteView::OnInitialUpdate() 
{
	CSprite *pDoc = GetDocument();
	ASSERT_VALID( pDoc );
	ASSERT( pDoc->pCurrentSprite != NULL );

	//  Create the drawing surface for this view.
	drawsurf.Create( pDoc->pCurrentSprite->GetWidth(), pDoc->pCurrentSprite->GetHeight(), pDoc->pCurrentSprite->GetBitDepth(), theApp.aPalette );
	
	//  Set the size of the view for scrolling.
	SetScrollSizes( MM_TEXT, CSize( pDoc->pCurrentSprite->GetWidth(), pDoc->pCurrentSprite->GetHeight() ) );

	CScrollView::OnInitialUpdate();
}
