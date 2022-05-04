// splitwnd.cpp : implementation file
//

#include "stdafx.h"
#include "sprite.h"
#include "splitwnd.h"
#include "spritvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitWnd

IMPLEMENT_DYNCREATE(CSplitWnd, CMDIChildWnd)

CSplitWnd::CSplitWnd()
{
}

CSplitWnd::~CSplitWnd()
{
}

BOOL CSplitWnd::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
	//  Create a static splitter window with one row and two columns.
	if ( !m_wndSplitter.CreateStatic(this,	1, 2) )
	{
		TRACE0( "Failed to create splitter window.\n" );
		return FALSE;
	}

	//  Add the first splitter pane, which will have the default view
	//  ( which is the tree view ).
	if ( !m_wndSplitter.CreateView( 0, 0,
		pContext->m_pNewViewClass, CSize( 150, 150 ),
		pContext ) )
	{
		TRACE0( "Failed to create tree view.\n" );
		return FALSE;
	}

	//  Add the second splitter pane, which will be a sprite view.
	if ( !m_wndSplitter.CreateView( 0, 1, 
		RUNTIME_CLASS( CSpriteView ), CSize( 150, 150 ),
		pContext ) )
	{
		TRACE0( "Failed to create sprite view.\n" );
		return FALSE;
	}

	//  Activate the first view.
	SetActiveView( ( CView * )m_wndSplitter.GetPane( 0, 1 ) );

	//  The window and both panes were successfully
	//  created.
	return TRUE;
}


BEGIN_MESSAGE_MAP(CSplitWnd, CMDIChildWnd)
	//{{AFX_MSG_MAP(CSplitWnd)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSplitWnd message handlers
