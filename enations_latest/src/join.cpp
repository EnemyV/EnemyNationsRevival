//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// join.cpp : implementation file
//

#include "join.h"

#include <algorithm>
#include "base.h"
#include "creatmul.inl"
#include "help.h"
#include "lastplnt.h"
#include "netapi.h"
#include "player.h"
#include "racedata.h"
#include "SDL2CreateStatus.h"
#include "stdafx.h"
#include "version.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

int ListBoxFindDataPtr( CListBox const& lst, void const* pData );


/////////////////////////////////////////////////////////////////////////////
// CGameInfo

#ifdef _DEBUG
void CGameInfo::AssertValid( ) const
{

    CObject::AssertValid( );

    ASSERT( ( 0 <= m_iNumOpponents ) && ( m_iNumOpponents < 257 ) );
    ASSERT( ( 0 <= m_iAIlevel ) && ( m_iAIlevel < 4 ) );
    ASSERT( ( 0 <= m_iWorldSize ) && ( m_iWorldSize < 3 ) );
    // m_sName / m_sDesc converted to std::string (Phase 5a) — no MFC validator.
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CJoinMulti

void CJoinMulti::Init( )
{
    // CDlgJoinPublish removed (Phase 2d). The SDL2 flow constructs CJoinMulti
    // directly in SDL2_RunJoinNetworkFlow(); this MFC entry point only ran
    // from CDlgMain::OnMainJoin (dead MFC main-menu fallback).
    ASSERT_VALID( this );
    theApp.Log( "Join multi-player game (MFC entry — no-op)" );
}

void CJoinMulti::ClosePick( )
{
    ASSERT_VALID( this );

    CCreateNewBase::ClosePick( );
    CCreateLoadBase::ClosePick( );
    CMultiBase::ClosePick( );
}

void CJoinMulti::CloseAll( )
{

    ASSERT_VALID( this );

    ClosePick( );

    CCreateNewBase::CloseAll( );
    CCreateLoadBase::CloseAll( );
    CMultiBase::CloseAll( );
}

void CJoinMulti::OnSessionEnum( LPCVPSESSIONINFO pSi )
{
    const CNetPublish* pPub = (const CNetPublish*)( pSi->sessionName );

    // Version + game-ID guard (mirrors original CDlgJoinGame::OnSessionEnum)
    WORD wTst = 0;
#ifdef _DEBUG
    wTst |= CNetPublish::fdebug;
#endif
    if ( pPub->m_iGameID != TLP_GAME_ID ||
         pPub->m_cVerMajor != VER_MAJOR  ||
         pPub->m_cVerMinor != VER_MINOR  ||
         ( pPub->m_cFlags & ( CNetPublish::fdebug | CNetPublish::fcheat ) ) != wTst )
        return;

    // Skip in-progress games (not joinable from lobby)
    if ( pPub->m_cFlags & CNetPublish::finprogress )
        return;

    const char* pGame = pPub->GetGameName();

    // Update if already listed
    for ( auto& s : m_sessions )
    {
        if ( s.gameName == pGame )
        {
            s.id           = pSi->sessionId;
            s.numOpponents = pPub->m_iNumOpponents;
            s.aiLevel      = pPub->m_iAIlevel;
            s.worldSize    = pPub->m_iWorldSize;
            s.startPos     = pPub->m_iPos;
            s.cFlags       = pPub->m_cFlags;
            return;
        }
    }

    // New entry
    SessionEntry e;
    e.id           = pSi->sessionId;
    e.gameName     = pGame;
    e.numOpponents = pPub->m_iNumOpponents;
    e.aiLevel      = pPub->m_iAIlevel;
    e.worldSize    = pPub->m_iWorldSize;
    e.startPos     = pPub->m_iPos;
    e.cFlags       = pPub->m_cFlags;
    m_sessions.push_back( e );
}

void CJoinMulti::OnSessionClose( LPCVPSESSIONINFO pSi )
{
    if ( pSi == NULL )
    {
        m_sessions.clear();
        return;
    }
    m_sessions.erase(
        std::remove_if( m_sessions.begin(), m_sessions.end(),
            [pSi]( const SessionEntry& e ) {
                return memcmp( &e.id, &pSi->sessionId, sizeof( VPSESSIONID ) ) == 0;
            } ),
        m_sessions.end() );
}

void CJoinMulti::GameLoaded( void* pBuf, int iLen )
{

    // load the game file
    SDL2CreateStatus* pDlg = theApp.m_pCreateGame->GetDlgStatus( );
    if ( pDlg != NULL )
        pDlg->SetPer( 0 );

    CPlayer* pPlyrMe   = theGame.GetMe( );
    CPlayer* pPlyrSrvr = theGame.GetServer( );

    // create the world
    theApp.NewWorld( );

    // decompress the file
    int   iDecompLen;
    void* pDeComp = CoDec::Decompress( pBuf, iLen, iDecompLen );
    free( pBuf );

    // put it in a CMemFile
    CMemFile filMem;
    filMem.Attach( (BYTE*)pDeComp, iDecompLen );

    // serialize it
    CArchive ar( &filMem, CArchive::load );
    theGame.Serialize( ar );
    theGame.SetServer( FALSE );
    ar.Close( );

    filMem.Detach( );
    CoDec::FreeBuf( pDeComp );
    filMem.Close( );

    // now set up players the way they are now
    m_wndPlyrList.RemoveAll( );
    POSITION pos;
    for ( pos = theGame.GetAll( ).GetHeadPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.GetAll( ).GetNext( pos );
        POSITION pos2;
        for ( pos2 = theGame.m_lstLoad.GetTailPosition( ); pos2 != NULL; )
        {
            CPlayer* pPlr2 = theGame.m_lstLoad.GetPrev( pos2 );
            if ( pPlr2->GetPlyrNum( ) == pPlr->GetPlyrNum( ) )
            {
                pPlr->SetNetNum( pPlr2->GetNetNum( ) );
                pPlr2->SetNetNum( 0 );
                pPlr->SetAI( pPlr2->IsAI( ) );
                break;
            }
        }

        pPlr->SetState( CPlayer::ready );
        if ( pPlr->GetPlyrNum( ) == pPlyrMe->GetPlyrNum( ) )
        {
            theGame._SetMe( pPlr );
            pPlr->SetLocal( TRUE );
            pPlr->SetAI( FALSE );
        }
        else
        {
            pPlr->SetLocal( FALSE );
            if ( pPlr->GetPlyrNum( ) == pPlyrSrvr->GetPlyrNum( ) )
            {
                theGame._SetServer( pPlr );
                pPlr->SetAI( FALSE );
            }
        }

        // new list box
        if ( pPlr->GetNetNum( ) != 0 )
            m_wndPlyrList.AddPlayer( pPlr );
    }

    // drop lstLoad
    for ( pos = theGame.m_lstLoad.GetTailPosition( ); pos != NULL; )
    {
        CPlayer* pPlr = theGame.m_lstLoad.GetPrev( pos );
        delete pPlr;
    }
    theGame.m_lstLoad.RemoveAll( );

    CNetInitDone msg( theGame.GetMe( ) );
    theGame.PostToServer( &msg, sizeof( msg ) );

    // done
    pDlg = theApp.m_pCreateGame->GetDlgStatus( );
    if ( pDlg != NULL )
        pDlg->SetPer( 100 );
}

#ifdef _DEBUG
void CJoinMulti::AssertValid( ) const
{
    CMultiBase::AssertValid( );
}
#endif

