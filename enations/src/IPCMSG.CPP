//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//
//  IPCMsg.cpp :  CEMsg object implementation
//               Divide and Conquer
//               
//  Last update:    09/02/95
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"
#include "resource.h"
#include "IPCMsg.hpp"
#include "player.h"

IMPLEMENT_SERIAL( CEMsg, CObject, 0 );
IMPLEMENT_SERIAL( CEMsgList, CObList, 0 );

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


///////////////////////////////////////////////////////////////////////
// CMsgIPC is used to send chat and email across the network

// these guys convert a CMsgIpc to/from a single buffer that can be sent over the net
char * CMsgIPC::ToBuf (int * piLen)
{

	*piLen = sizeof (CMsgIPC) + m_sMessage.GetLength () + m_sSubject.GetLength () + 4;
	char * pRtn;

	try
		{
		pRtn = new char [*piLen];
		}
	catch (...)
		{
		return (NULL);
		}

	memcpy (pRtn, this, sizeof (CMsgIPC));
	char *pTmp = pRtn + sizeof (CMsgIPC);

	strcpy (pTmp, m_sMessage);
	pTmp += m_sMessage.GetLength () + 1;
	strcpy (pTmp, m_sSubject);

	return (pRtn);
}

void CMsgIPC::PostToClient ()
{

	int iLen;
	char * pBuf = ToBuf (&iLen);

	if ( iLen > VP_MAXSENDDATA )
		{
		TRAP ();
		AfxMessageBox ( IDS_MAX_MAIL, MB_OK );
		delete [] pBuf;
		return;
		}

	if (pBuf != NULL)
		{
		theGame.PostClientToClient ( theGame.GetPlayerByPlyr (m_iTo), (CMsgIPC *) pBuf, iLen);
		delete [] pBuf;
		}
}

// BUGBUG for testing, this is CObject derived but
// in reality it must be CNetCmd derived
#if USE_FAKE_NET
CMsgIPC::CMsgIPC( int iType )
#else
CMsgIPC::CMsgIPC( int iType ) : CNetCmd (ipc_msg)
#endif
{
    m_iTo = 0;			// id of player to receive message
    m_iFrom = 0;		// id of player sending message
    m_iType = iType;	// enum value == type of message
    m_iLen = 0;			// length in bytes of message
	m_iCC = 0;
}

CMsgIPC::~CMsgIPC()
{
	m_sMessage.Empty();
	m_sSubject.Empty();
}

///////////////////////////////////////////////////////////////////////

void CEMsg::Serialize( CArchive& archive )
{
    ASSERT_VALID( this );

    CObject::Serialize( archive );

    if( archive.IsStoring() )
    {
		archive << (WORD)m_iID;
		archive << (WORD)m_iFrom;
		archive << (WORD)m_iTo;
		archive << (WORD)m_iCC;
		archive << m_wStatus;
		archive << m_sSubject;
		archive << m_sMessage;
	}
	else
	{
		WORD wJunk;
		archive >> wJunk; 
		m_iID = (int)wJunk;
		archive >> wJunk; 
		m_iFrom = (int)wJunk;
		archive >> wJunk; 
		m_iTo = (int)wJunk;
		archive >> wJunk; 
		m_iCC = (int)wJunk;
		
		archive >> m_wStatus; 
		archive >> m_sSubject;
		archive >> m_sMessage;

		m_pOpenWnd = NULL;
	}
}

CEMsg::~CEMsg()
{
	ASSERT_VALID( this );
	m_sSubject.Empty();
	m_sMessage.Empty();
}

CEMsg::CEMsg( int iID, int iFrom, int iTo, 
	const char *pzSubject, const char *pzMessage )
{
	m_iID = iID;
	m_iFrom = iFrom;
	m_iTo = iTo;
	m_wStatus = 0;
	m_iCC = 0;

	m_sSubject = pzSubject;
	m_sMessage = pzMessage;

	m_pOpenWnd = NULL;

	ASSERT_VALID( this );
}

///////////////////////////////////////////////////////////////////////////

CEMsgList::~CEMsgList( void )
{
	ASSERT_VALID( this );
    DeleteList();
}

// determine the next or prev message, from the message
// of the id passed, based on the command passed
//
// IDS_MAIL_PREV
// IDS_MAIL_NEXT
//
CEMsg *CEMsgList::GetNextPrevMsg( int iID, int iCmd )
{
	ASSERT_VALID( this );

	if( iCmd == IDS_MAIL_PREV )
	{
		int iLastID = m_iLastID;

    	POSITION pos = GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CEMsg *pMsg = (CEMsg *)GetNext( pos );
        	if( pMsg != NULL )
        	{
        		ASSERT_VALID( pMsg );

            	if( pMsg->m_iID == iID )
				{
					return( GetMessage(iLastID) );
                }
				iLastID = pMsg->m_iID;
        	}
    	}

	}
	else if( iCmd == IDS_MAIL_NEXT )
	{
		BOOL bHit = FALSE;
    	POSITION pos = GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CEMsg *pMsg = (CEMsg *)GetNext( pos );
        	if( pMsg != NULL )
        	{
        		ASSERT_VALID( pMsg );

				if( bHit )
					return( pMsg );

            	if( pMsg->m_iID == iID )
					bHit = TRUE;
			}
		}
	}
	return( NULL );
}

CEMsg *CEMsgList::GetMessage( int iID )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CEMsg *pMsg = (CEMsg *)GetNext( pos );
        if( pMsg != NULL )
        {
        	ASSERT_VALID( pMsg );

            if( pMsg->m_iID == iID )
                return( pMsg );
        }
    }
    return( NULL );
}

void CEMsgList::RemoveMessage( int iID )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CEMsg *pMsg = (CEMsg *)GetNext( pos1 );
        if( pMsg == NULL )
            break;
            
        ASSERT_VALID( pMsg );

        if( pMsg->m_iID != iID )
            continue;
            
        pMsg = (CEMsg *)GetAt( pos2 );
        if( pMsg != NULL )
        {
        	ASSERT_VALID( pMsg );

        	RemoveAt( pos2 );
        	delete pMsg;
        	break;
        }
    }
}

void CEMsgList::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CEMsg *pMsg = (CEMsg *)GetNext( pos );
            if( pMsg != NULL )
            {
        		ASSERT_VALID( pMsg );

                delete pMsg;
            }
        }
    }
    RemoveAll();
}

// end of IPCMsg.cpp
