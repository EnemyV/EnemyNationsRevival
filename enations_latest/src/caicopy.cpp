////////////////////////////////////////////////////////////////////////////
//
//  CAICopy.cpp :  CAICopy object declaration
//                 Divide and Conquer AI
//               
//  Last update:    07/08/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAICopy.hpp"

#define new DEBUG_NEW

CAICopy::CAICopy( int iType )
{
	m_iType = iType;

	for( int i=0; i<CAI_DATA_SLOTS; ++i )
	{
		m_aiDataIn[i] = 0;
		m_aiDataOut[i] = 0;
	}
	ASSERT_VALID( this );
}

/////////////////////////////////////////////////////////////////////////////

CAICopyList::~CAICopyList( void )
{
	ASSERT_VALID( this );
    DeleteList();
}

CAICopy *CAICopyList::GetCopy( int iType )
{
	ASSERT_VALID( this );

	// BUGBUG - got a NULL here once
	if ( this == NULL )
		return NULL;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CAICopy *pCopy = (CAICopy *)GetNext( pos );
        if( pCopy != NULL )
        {
        	ASSERT_VALID( pCopy );

            if( pCopy->m_iType == iType )
                return( pCopy );
        }
    }
    return( NULL );
}
//
// delete CAICopy objects in the list
//
void CAICopyList::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CAICopy *pCopy = (CAICopy *)GetNext( pos );
            if( pCopy != NULL )
            {
            	ASSERT_VALID( pCopy );

                delete pCopy;
            }
        }
    }
    RemoveAll();
}

void CAICopyList::RemoveCopy( int iType )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CAICopy *pCopy = (CAICopy *)GetNext( pos1 );
        if( pCopy != NULL )
        {
            ASSERT_VALID( pCopy );

        	if( pCopy->m_iType != iType )
            	continue;
        }

        pCopy = (CAICopy *)GetAt( pos2 );
        if( pCopy != NULL )
        {
            ASSERT_VALID( pCopy );

        	RemoveAt( pos2 );
        	delete pCopy;
        	break;
        }
    }
}

// end of CAICopy.cpp
