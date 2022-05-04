////////////////////////////////////////////////////////////////////////////
//
//  CAIGoal.cpp :  CAIGoal object implementation
//                 Divide and Conquer AI
//               
//  Last update:    03/14/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"
#include "cai.h"
#include "CAIGoal.hpp"
#include "CAIData.hpp"

#define new DEBUG_NEW

extern CWordArray *pwaIG;		// initial goals array, all difficulties
extern CAIData *pGameData;	// pointer to game data interface

/////////////////////////////////////////////////////////////////////////////

CAIGoal::CAIGoal( WORD wID, BYTE cType, CWordArray *pwaTasks )
{
	m_wID = wID;
	m_pwaTasks = NULL;
	m_cType = cType;
	m_iNumTasks = 0;

	if( pwaTasks != NULL )
	{
		ASSERT_VALID( pwaTasks );
		
		m_iNumTasks = pwaTasks->GetSize();
		// m_iNumTask is not to be zero.
		if( !m_iNumTasks )
			m_iNumTasks = 0;

		try
		{
			m_pwaTasks = new WORD[m_iNumTasks];
			for( int i=0; i<m_iNumTasks; ++i )
				m_pwaTasks[i] = pwaTasks->GetAt(i);
		}
		catch( CException e )
		{
			if( m_pwaTasks != NULL )
			{
				delete [] m_pwaTasks;
				m_pwaTasks = NULL;
			}
			throw(ERR_CAI_BAD_NEW);
		}
	}
}
//
// return unique id of the goal
//
WORD CAIGoal::GetID( void )
{
	ASSERT_VALID( this );
	return m_wID;
}

BYTE CAIGoal::GetType( void )
{
	ASSERT_VALID( this );
	return m_cType;
}


WORD CAIGoal::GetTaskAt( int iAt )
{
	ASSERT_VALID( this );
	
	if( m_pwaTasks == NULL )
		return FALSE;

	if( iAt >= m_iNumTasks )
		return FALSE;

	return( m_pwaTasks[iAt] );
}

WORD CAIGoal::GetFirstTask( void )
{
	ASSERT_VALID( this );
	
	if( m_pwaTasks == NULL )
		return FALSE;

	return( m_pwaTasks[0] );
}

WORD CAIGoal::GetNextTask( WORD wPriorID )
{
	ASSERT_VALID( this );
	
	if( m_pwaTasks == NULL )
		return FALSE;

	WORD wLastID = m_pwaTasks[0];
	if( wLastID == wPriorID )
	{
		if( m_iNumTasks )
			return( m_pwaTasks[1] );
		else
			return FALSE;
	}

	for( int i=0; i<m_iNumTasks; ++i )
	{
		if( wLastID == wPriorID )
			return( m_pwaTasks[i] );
		else
			wLastID = m_pwaTasks[i];
	}
	return FALSE;
}

//
// this routine copies the goal to another instance of the goal
//
CAIGoal *CAIGoal::CopyGoal( void )
{
	ASSERT_VALID( this );
	
	CWordArray *pwaTasks = NULL;
	if( m_pwaTasks != NULL )
	{
		// create a task array just like the one for this goal
		try
		{
			pwaTasks = new CWordArray();

			pwaTasks->SetSize( m_iNumTasks );
			for( int i=0; i<pwaTasks->GetSize(); i++ )
				pwaTasks->SetAt( i, m_pwaTasks[i] );
		}
		catch( CException e )
		{
			// BUGBUG need to report this to the user?
			if( pwaTasks != NULL )
			{
				pwaTasks->RemoveAll();
				delete pwaTasks;
				//return NULL;
			}
			throw(ERR_CAI_BAD_NEW);
		}
	}
	
	// now try to create a copy of this goal
	CAIGoal *pGoal = NULL;
	try
	{
		pGoal = new CAIGoal( m_wID, m_cType, pwaTasks );
	}
	catch( CException e )
	{
		// BUGBUG need to report this to the user?
		if( pwaTasks != NULL )
		{
			pwaTasks->RemoveAll();
			delete pwaTasks;
			//return NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	if( pwaTasks != NULL )
	{
		pwaTasks->RemoveAll();
		delete pwaTasks;
	}
	
	return( pGoal );
}


CAIGoal::~CAIGoal()
{
	ASSERT_VALID( this );

	if( m_pwaTasks != NULL )
	{
		//ASSERT_VALID( m_pwaTasks );

		//m_pwaTasks->RemoveAll();
		//delete m_pwaTasks;
		delete [] m_pwaTasks;
		m_pwaTasks = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////

//
// this initializes a passed goal list, specific to a player and creates
// the basic start up goals for that player's list
//
BOOL CAIGoalList::Initialize( CAIGoalList *plGoals )
{
	ASSERT_VALID( this );
	if( plGoals == NULL )
		return( FALSE );
	ASSERT_VALID( plGoals );
		
	if( pwaIG == NULL )
		return( FALSE );
	ASSERT_VALID( pwaIG );
	
	
	CAIGoal *pGoal = NULL;
	CAIGoal *pNewGoal = NULL;

	// set starting offset based on difficulty level
	int iDiff = pGameData->m_iSmart * NUM_INITIAL_GOALS;

	WORD wID;
	for( int i=0; i<NUM_INITIAL_GOALS; ++i )
	{
		wID = pwaIG->GetAt( i + iDiff ); 
		if( !wID )
			break;
		pGoal = GetGoal( wID );
		if( pGoal != NULL )
		{
			try
			{
				pNewGoal = pGoal->CopyGoal();
				plGoals->AddTail( (CObject *)pNewGoal );
			}
			catch( CException e )
			{
				//return( FALSE );
				throw(ERR_CAI_BAD_NEW);
			}
		}
	}
	return( TRUE );
}

CAIGoalList::~CAIGoalList( void )
{
	ASSERT_VALID( this );
    DeleteList();
}

//
// return a pointer to the CAIGoal object matching the id passed
//
CAIGoal *CAIGoalList::GetGoal( WORD wId )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CAIGoal *pGoal = (CAIGoal *)GetNext( pos );
        if( pGoal != NULL )
        {
        	ASSERT_VALID( pGoal );

            if( pGoal->GetID() == wId )
                return( pGoal );
        }
    }
    return( NULL );
}

//
// delete CAIGoal objects in the list
//
void CAIGoalList::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CAIGoal *pGoal = (CAIGoal *)GetNext( pos );
            if( pGoal != NULL )
            {
            	ASSERT_VALID( pGoal );

                delete pGoal;
            }
        }
    }
    RemoveAll();
}

void CAIGoalList::RemoveGoal( WORD wAID )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CAIGoal *pGoal = (CAIGoal *)GetNext( pos1 );
        if( pGoal != NULL )
        {
            ASSERT_VALID( pGoal );

        	if( pGoal->GetID() != wAID )
            	continue;
        }

        pGoal = (CAIGoal *)GetAt( pos2 );
        if( pGoal != NULL )
        {
            ASSERT_VALID( pGoal );

        	RemoveAt( pos2 );
        	delete pGoal;
        	break;
        }
    }
}

// end of CAIGoal.cpp
