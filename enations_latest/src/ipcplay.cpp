//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCPlay.cpp:  CIPCPlayer, CIPCPlayerList, CPlyrMsgStatusDlg
//               
//  Last update:    08/25/95
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lastplnt.h"
#include "player.h"
#include "IPCPlay.h"

IMPLEMENT_SERIAL( CIPCPlayer, CObject, 0 );
IMPLEMENT_SERIAL( CIPCPlayerList, CObList, 0 );

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


extern CIPCPlayerList *plIPCPlayers;
// pPlayerDlg / CPlyrMsgStatusDlg removed (Phase 2d).

//CIPCPlayer::CIPCPlayer( WORD wID )
CIPCPlayer::CIPCPlayer( const char *pszName, WORD wID )
{
	m_wID = wID;
	m_sName = pszName;
	m_wStatus = 0;
	m_pPlyr = NULL;
	m_pwndChat = NULL;

	// now set default status
	m_wStatus |= IPC_ACCEPT_EMAIL;
	m_wStatus |= IPC_ACCEPT_CHAT;
	m_wStatus |= IPC_ACCEPT_VOICE;
}

CIPCPlayer::~CIPCPlayer()
{
}

void CIPCPlayer::Serialize( CArchive& archive )
{
    ASSERT_VALID( this );

    CObject::Serialize( archive );

    if( archive.IsStoring() )
    {
		archive << m_wID;
		archive << m_sName;
		archive << m_wStatus;
		archive << (LONG) m_pPlyr->GetPlyrNum ();
	}
	else
	{
		archive >> m_wID;
		archive >> m_sName;
		archive >> m_wStatus;

		LONG l;
		TRAP ();
		archive >> l;
		m_pPlyr = theGame.GetPlayerByPlyr (l);
	}
}

///////////////////////////////////////////////////////////////////////////

void CIPCPlayerList::InitPlayers( void )
{

	if (GetCount () > 0)
		{
		ASSERT (FALSE);
		DeleteList ();
		}

	CIPCPlayer *pPlayer;
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
	{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_VALID (pPlr);
		
		if ((! pPlr->IsAI ()) && (! pPlr->IsLocal ()))
			{
			pPlayer = new CIPCPlayer( pPlr->GetName(), (WORD)pPlr->GetPlyrNum() );
			pPlayer->m_pPlyr = pPlr;

			AddTail( (CObject *)pPlayer );
			}
	}
}


CIPCPlayer *CIPCPlayerList::GetPlayer( const std::string& sName )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

			if( pPlayer->m_sName == sName.c_str() )
                return( pPlayer );
        }
    }
    return( NULL );
}

CIPCPlayer *CIPCPlayerList::GetPlayer( WORD wID )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

            if( pPlayer->m_wID == wID )
                return( pPlayer );
        }
    }
    return( NULL );
}


void CIPCPlayerList::RemovePlayer( const std::string& sName )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition();
        ( pos2 = pos1 ) != NULL; )
    {
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos1 );
        if( pPlayer == NULL )
            break;

        ASSERT_VALID( pPlayer );

		if( pPlayer->m_sName != sName.c_str() )
            continue;
            
        pPlayer = (CIPCPlayer *)GetAt( pos2 );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

        	RemoveAt( pos2 );
        	delete pPlayer;
        	break;
        }
    }
}

void CIPCPlayerList::RemovePlayer( WORD wID )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos1 );
        if( pPlayer == NULL )
            break;
            
        ASSERT_VALID( pPlayer );

        if( pPlayer->m_wID != wID )
            continue;
            
        pPlayer = (CIPCPlayer *)GetAt( pos2 );
        if( pPlayer != NULL )
        {
        	ASSERT_VALID( pPlayer );

        	RemoveAt( pos2 );
        	delete pPlayer;
        	break;
        }
    }
}

void CIPCPlayerList::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CIPCPlayer *pPlayer = (CIPCPlayer *)GetNext( pos );
            if( pPlayer != NULL )
            {
        		ASSERT_VALID( pPlayer );

                delete pPlayer;
            }
        }
    }
    RemoveAll();
}

CIPCPlayerList::~CIPCPlayerList()
{
	ASSERT_VALID( this );
    DeleteList();
}

