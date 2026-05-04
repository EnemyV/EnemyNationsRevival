// PlyrList.cpp : implementation file
//
// CDlgPlyrList (the in-game player list / kick dialog) deleted Phase 2d.
// SDL2PlayerListDialog (in SDL2GameDialogs.cpp) is the sole replacement.
// This file now hosts only CConquerApp::ShowPlayerList which routes to it.

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "GameWindow.h"
#include "SDL2GameDialogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CConquerApp::ShowPlayerList( )
{
    if ( ( !theGame.IsNetGame( ) ) || ( !theGame.AmServer( ) ) )
        return;

    if ( !m_gameWindow )
        return;

    SDL2PlayerListDialog dlg( m_gameWindow.get() );
    dlg.DoModal();
}
