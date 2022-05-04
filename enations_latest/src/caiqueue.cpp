////////////////////////////////////////////////////////////////////////////
//
//  CAIQueue.cpp : CAIQueue object declaration
//                 Divide and Conquer AI
//               
//  Last update:    09/07/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAIQueue.hpp"
#include "CAIMsg.hpp"

IMPLEMENT_SERIAL( CAIQueue, CObList, 0 );

#define new DEBUG_NEW

CAIQueue::CAIQueue( int iPlayer )
{
	m_iPlayer = iPlayer;
	m_Pos = NULL;
}

CAIQueue::~CAIQueue()
{
	DeleteList();
}

//
// delete CNetCmd objects in the list
//
void CAIQueue::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CAIMsg *pMsg = (CAIMsg *)GetNext( pos );
            if( pMsg != NULL )
            {
            	ASSERT_VALID( pMsg );

                delete pMsg;
            }
        }
    }
    RemoveAll();
}


void CAIQueue::Serialize( CArchive& archive )
{
    ASSERT_VALID( this );

    CObList::Serialize( archive );

    if( archive.IsStoring() )
    {
    	archive << (WORD)m_iPlayer;
		//archive << m_Pos;
	}
    else
    {
		// BUGBUG will the pos be correct from a serialization?
		WORD wJunk;
    	archive >> wJunk;
    	m_iPlayer = (int)wJunk;
		//archive >> m_Pos;
	}
}

// end of CAIQueue.cpp
