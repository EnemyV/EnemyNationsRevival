// PalView.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "PalView.h"

IMPLEMENT_DYNCREATE(CPalView, CView)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSpriteApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPalView


CPalView::CPalView()
{
}

CPalView::~CPalView()
{
}


BEGIN_MESSAGE_MAP(CPalView, CView)
	//{{AFX_MSG_MAP(CPalView)
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPalView drawing

void CPalView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	
	ASSERT_VALID( pDoc );
	ASSERT( theApp.pCurrentPalette != NULL );
	ASSERT( theApp.pWinPalette != NULL );
	
	//  Select the current palette.
	CPalette *pOldPalette = pDC->SelectPalette( theApp.pWinPalette, FALSE );
	pDC->RealizePalette();

	CRect	rect;
	GetClientRect( rect );
	int boxWidth = rect.right / 16;
	int boxHeight = rect.bottom / 16;
	
	int row, col;
	for( row = 0; row < 16; row++ )
	{
		for( col = 0; col < 16; col++ )
		{
			BYTE red = theApp.pCurrentPalette[ row * 16 + col ].rgbRed;
			BYTE green = theApp.pCurrentPalette[ row * 16 + col ].rgbGreen;
			BYTE blue = theApp.pCurrentPalette[ row * 16 + col ].rgbBlue;

			CBrush	palBrush;
			if ( palBrush.CreateSolidBrush( PALETTERGB( red, green, blue ) ) == FALSE )
				return;

			CBrush *pOldBrush = pDC->SelectObject( &palBrush );
			
			pDC->Rectangle( col * boxWidth, row * boxHeight, ( col + 1 ) * boxWidth,
				( row + 1 ) * boxHeight );

			( void )pDC->SelectObject( pOldBrush );
		}
	}

	//  Restore the old palette
	pDC->SelectPalette( pOldPalette, FALSE );
}

/////////////////////////////////////////////////////////////////////////////
// CPalView diagnostics

#ifdef _DEBUG
void CPalView::AssertValid() const
{
	CView::AssertValid();
}

void CPalView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPalView message handlers

void CPalView::OnRButtonUp(UINT /*nFlags*/, CPoint point) 
{
	//  Determine which palette index was hit.
	CRect	rect;
	GetClientRect( rect );
	int boxWidth = rect.right / 16;
	int boxHeight = rect.bottom / 16;
	int col = point.x / boxWidth;
	int row = point.y / boxHeight;
	if ( col >= 16 || row >= 16 )
		return;
	int index = row * 16 + col;

	//  Create a popup menu with the RGB values
	//  for that index.
	CString str;
	str.Format( "( %d, %d, %d )", 
		theApp.pCurrentPalette[ index ].rgbRed,
		theApp.pCurrentPalette[ index ].rgbGreen,
		theApp.pCurrentPalette[ index ].rgbBlue );
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu( MF_DISABLED, 0, str );

	ClientToScreen( &point );
	menu.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, this, NULL );
}
