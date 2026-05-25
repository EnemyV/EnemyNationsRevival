// DlgMsg.cpp : implementation file
//
// CDlgModelessMsg — SDL2-rendered notification popup. Replaces the
// earlier Win32 popup; matches the in-game dialog aesthetic.

#include "stdafx.h"
#include "lastplnt.h"
#include "DlgMsg.h"
#include "GameWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg

CDlgModelessMsg::CDlgModelessMsg( CWnd* /*pParent*/ )
    : SDL2Dialog( theApp.m_gameWindow ? theApp.m_gameWindow.get() : nullptr,
                  "Enemy Nations", 360, 140 )
{
}

CDlgModelessMsg::~CDlgModelessMsg() {}

void CDlgModelessMsg::OnInit()
{
    // Message label fills most of the interior, leaving room for an
    // OK button along the bottom.
    int lx = m_x + 12;
    int ly = m_y + 30;
    int lw = m_width - 24;
    int lh = m_height - 30 - 38;
    auto* lbl = AddWidget<SDL2Label>( lx, ly, lw, lh, m_sMsg );
    lbl->SetWrapped( true );
    lbl->SetTopAligned( true );

    // OK button — centered along the bottom
    const int bw = 70, bh = 24;
    AddWidget<SDL2Button>( m_x + ( m_width - bw ) / 2,
                           m_y + m_height - bh - 8,
                           bw, bh, "OK",
                           [this]() { EndDialog( 1 ); } );
}

void CDlgModelessMsg::Create( const char* pMsg )
{
    m_sMsg = pMsg ? pMsg : "";
    ShowNonModal();  // GameWindow deletes us after the user dismisses
}
