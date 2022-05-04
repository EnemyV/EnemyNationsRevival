//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  chatbar.cpp : generic toolbar for use
//                Divide and Conquer
//               
//  Last update:    08/22/95
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "player.h"
#include "chatbar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

// BUGBUG - only show legit players when drops down (ie don't have windows for

/////////////////////////////////////////////////////////////////////////////
// CChatBar dialog

CChatBar::CChatBar(CWnd* pParent /*=NULL*/)
	: CDialogBar() //(CChatBar::IDD, pParent)
{
	m_pParent = pParent;
}

void CChatBar::InitBar() 
{
	CComboBox *pcbPlayers = (CComboBox *)GetDlgItem( IDC_SELECTPLAYER );
	if( pcbPlayers != NULL )
	{
		int i = 0;
		POSITION pos;
		for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
			CPlayer *pPlr = theGame.GetAll().GetNext (pos);
			ASSERT_VALID (pPlr);
			if ((! pPlr->IsAI ()) && (! pPlr->IsLocal ()))
				{
				pcbPlayers->AddString( pPlr->GetName() );
				pcbPlayers->SetItemDataPtr( i++, (void*) pPlr );
				}
		}
	}

	// must subclass buttons that are in the dlg template
	// and respond to BN_CLICKED in parent
	SubclassDlgItem( IDC_HANGUP, m_pParent);
	SubclassDlgItem( IDC_SELECTPLAYER, m_pParent);
}

BEGIN_MESSAGE_MAP(CChatBar, CDialogBar)
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChatBar message handlers

void CChatBar::OnSize( UINT nType, int cx, int cy )
{
	CDialogBar::OnSize(nType, cx, cy);

	// adjust the size of the select player combo box
	// this needs to be a derived combo box
	CComboBox *m_pPlayers = (CComboBox *)GetDlgItem(IDC_SELECTPLAYER);
	if( m_pPlayers != NULL )
	{
		if( m_pPlayers->m_hWnd != NULL )
		{
			CRect rect;
			GetClientRect (&rect);
			ClientToScreen (&rect);
			CRect rWnd;

			m_pPlayers->GetWindowRect( &rWnd );
			m_pPlayers->SetWindowPos (NULL, 0, 0, 
				rect.right - rWnd.left, rWnd.Height (), 
				SWP_NOMOVE | SWP_NOZORDER);
		}
	}
}

void CChatBar::PostNcDestroy() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CDialogBar::PostNcDestroy();
}

