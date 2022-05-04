////////////////////////////////////////////////////////////////////////////
//
//  CAITask.cpp :  CAITask object
//                 Divide and Conquer AI
//               
//  Last update:    07/04/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"
#include "CAI.h"
#include "CAITask.hpp"
#include "CAIData.hpp"

#define new DEBUG_NEW

#if THREADS_ENABLED
extern CException *pException;		// standard exception for yielding
#endif

/////////////////////////////////////////////////////////////////////////////

CAITask::CAITask( WORD wID, BYTE cType, BYTE cPriority,
	WORD wOrderID, CWordArray *pwaParams )
{
	m_wID = wID;
	m_cType = cType;
	m_wGoalID = FALSE;
	m_wOrderID = wOrderID;
	m_cStatus = UNASSIGNED_TASK;
	m_cPriority = cPriority;
	m_pwaParams = NULL;
	
	if( pwaParams != NULL )
	{
		ASSERT_VALID( pwaParams );
		
		try
		{
			m_pwaParams = new WORD[MAX_TASKPARAMS];
			// MAX_TASKPARAMS is size
			for( int i=0; i<MAX_TASKPARAMS; ++i )
				m_pwaParams[i] = pwaParams->GetAt(i);
		}
		catch( CException e )
		{
			if( m_pwaParams != NULL )
			{
				delete [] m_pwaParams;
				m_pwaParams = NULL;
			}
			throw(ERR_CAI_BAD_NEW);
		}
	}
	
	ASSERT_VALID( this );
}

CAITask::~CAITask()
{
	ASSERT_VALID( this );
	
	if( m_pwaParams != NULL )
	{
		delete [] m_pwaParams;
		m_pwaParams = NULL;
	}
}

//
// return unique id of the task
//
WORD CAITask::GetID( void )
{
	ASSERT_VALID( this );
	return m_wID;
}

BYTE CAITask::GetType( void )
{
	ASSERT_VALID( this );
	return m_cType;
}

BYTE CAITask::GetStatus( void )
{
	ASSERT_VALID( this );
	return m_cStatus;
}

void CAITask::SetStatus( BYTE cStatus )
{
	ASSERT_VALID( this );
	m_cStatus = cStatus;
}

WORD CAITask::GetGoalID( void )
{
	ASSERT_VALID( this );
	return m_wGoalID;
}

void CAITask::SetGoalID( WORD wGoalID )
{
	ASSERT_VALID( this );
	m_wGoalID = wGoalID;
}

WORD CAITask::GetOrderID( void )
{
	ASSERT_VALID( this );
	return m_wOrderID;
}

BYTE CAITask::GetPriority( void )
{
	ASSERT_VALID( this );
	return m_cPriority;
}

void CAITask::SetPriority( BYTE cPriority )
{
	ASSERT_VALID( this );
	m_cPriority = cPriority;
}

void CAITask::SetTaskParam( WORD wOffset, WORD wValue )
{
	ASSERT_VALID( this );
	if( m_pwaParams != NULL )
	{
		ASSERT( (int)wOffset < MAX_TASKPARAMS );
		if( (int)wOffset < MAX_TASKPARAMS )
			m_pwaParams[wOffset] = wValue;
	}
}

WORD CAITask::GetTaskParam( WORD wOffset )
{
	ASSERT_VALID( this );
	if( m_pwaParams != NULL )
	{
		ASSERT( (int)wOffset < MAX_TASKPARAMS );
		if( (int)wOffset < MAX_TASKPARAMS )
			return( m_pwaParams[wOffset] );
	}
	return FALSE;
}

//
// produce of copy of this task and return it
//
CAITask *CAITask::CopyTask( void )
{
	ASSERT_VALID( this );
	
	CAITask *pTask = NULL;
	CWordArray *pwaParams = NULL;

	// create an array of parameters, if necessary
	if( m_pwaParams != NULL )
	{
		try
		{
			pwaParams = new CWordArray();
			pwaParams->SetSize( MAX_TASKPARAMS );
			for( int i=0; i<pwaParams->GetSize(); ++i )
				pwaParams->SetAt( i, m_pwaParams[i] );
		}
		catch( CException e )
		{
			if( pwaParams != NULL )
			{
				pwaParams->RemoveAll();
				delete pwaParams;
				pwaParams = NULL;
			}
			throw(ERR_CAI_BAD_NEW);
		}
	}
	
	// now create a copy of this task
	try
	{
		pTask = new CAITask( m_wID, m_cType, m_cPriority, 
			m_wOrderID, pwaParams );
	}
	catch( CException e )
	{
		if( pwaParams != NULL )
		{
			pwaParams->RemoveAll();
			delete pwaParams;
			pwaParams = NULL;
		}
		throw(ERR_CAI_BAD_NEW);
	}

	// clean up
	if( pwaParams != NULL )
	{
		pwaParams->RemoveAll();
		delete pwaParams;
		pwaParams = NULL;
	}

	return( pTask );
}

/////////////////////////////////////////////////////////////////////////////

CAITaskList::~CAITaskList( void )
{
	ASSERT_VALID( this );
    DeleteList();
}
//
// return a pointer to the CAITask object matching the id passed
//
CAITask *CAITaskList::GetTask( WORD wId, WORD wGoal )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

            if( pTask->GetID() == wId &&
				pTask->GetGoalID() == wGoal )
                return( pTask );
        }
    }
    return( NULL );
}
//
// return a pointer to the CAITask object matching the id passed
//
CAITask *CAITaskList::FindTask( WORD wId )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

            if( pTask->GetID() == wId )
                return( pTask );
        }
    }
    return( NULL );
}

//
// BUGBUG no yield in this function
//
// delete CAITask objects in the list
//
void CAITaskList::DeleteList( void )
{
	ASSERT_VALID( this );

    if( GetCount() )
    {
        POSITION pos = GetHeadPosition();
        while( pos != NULL )
        {   
            CAITask *pTask = (CAITask *)GetNext( pos );
            if( pTask != NULL )
            {
        		ASSERT_VALID( pTask );

                delete pTask;
            }
        }
    }
    RemoveAll();
}

// never yield in this function
//
void CAITaskList::RemoveTask( WORD wAID, WORD wGoal )
{
	ASSERT_VALID( this );

    POSITION pos1, pos2;
    for( pos1 = GetHeadPosition(); 
        ( pos2 = pos1 ) != NULL; )
    {
        CAITask *pTask = (CAITask *)GetNext( pos1 );
        if( pTask == NULL )
            break;
            
        ASSERT_VALID( pTask );

        if( pTask->GetID() != wAID &&
			pTask->GetGoalID() != wGoal )
            continue;
            
        pTask = (CAITask *)GetAt( pos2 );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

        	RemoveAt( pos2 );
        	delete pTask;
        	break;
        }
    }
}

//
// get first instance of this task type that is
// not currently assigned, regardless of priority
// 
CAITask *CAITaskList::GetUnassignedTask( int iTask )
{
	ASSERT_VALID( this );

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );
			if( pTask->GetID() == (WORD)iTask &&
				pTask->GetStatus() == UNASSIGNED_TASK )
				return( pTask );
		}
	}
	return( NULL );
}

CAITask *CAITaskList::GetPatrolTask( int iType )
{
	ASSERT_VALID( this );

	CAITask *pPickedTask = NULL;
	int iPriority = 0;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );
			if( pTask->GetID() == IDT_PATROL )
			{
				// navy patrols are different than land patrols
				if( iType == CUnitData::naval )
				{
					if( pTask->GetGoalID() != IDG_SEAWAR &&
						pTask->GetGoalID() != IDG_SHORES )
						continue;
				}

				if( (int)pTask->GetPriority() > iPriority )
				{
					pPickedTask = pTask;
					iPriority = (int)pTask->GetPriority();
				}
				break;
			}
		}
	}
	return( pPickedTask );
}


//
// find the highest priority, unassigned scouting task:
// IDT_SCOUT, IDT_SCOUT0, IDT_SCOUT1, IDT_SCOUT2, IDT_SCOUT3
// and return a pointer to it
//
CAITask *CAITaskList::GetScoutTask( void )
{
	ASSERT_VALID( this );

	CAITask *pPickedTask = NULL;
	int iPriority = 0;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			if( pTask->GetStatus() != UNASSIGNED_TASK )
				continue;

			switch( pTask->GetID() )
			{
				case IDT_FINDOPFORS:
				case IDT_FINDOPFORCITY:
				case IDT_SCOUT:
				case IDT_SCOUT0:
				case IDT_SCOUT1:
				case IDT_SCOUT2:
				case IDT_SCOUT3:

				// BUGBUG for testing, force IDT_FINDOPFORCITY
				//	if( pTask->GetID() == IDT_FINDOPFORCITY )
				//		return( pTask );
					
					if( (int)pTask->GetPriority() > iPriority )
					{
						pPickedTask = pTask;
						iPriority = (int)pTask->GetPriority();
					}
					break;
				default:
					break;
			}
		}
	}
	return( pPickedTask );
}

//
// a type of prioritization is needed in goal mgr to be used
// during selecting a construction task, which means that
// the task list must be made aware of the priorities
// set by the goal manager
//
CAITask *CAITaskList::GetConstructionTask( void )
{
	ASSERT_VALID( this );

	CAITask *pPickedTask = NULL;
	int iPriority = 0;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );
			if( pTask->GetTaskParam(ORDER_TYPE) == CONSTRUCTION_ORDER
				&& pTask->GetStatus() == UNASSIGNED_TASK )
			{
				// only can task switch to these tasks
				if( pTask->GetID() == IDT_REPAIRING ||
					pTask->GetID() == IDT_REPAIR )
					continue;

				// tasks with 0 priority may not be researched
				if( (int)pTask->GetPriority() > iPriority )
				{
					pPickedTask = pTask;
					iPriority = (int)pTask->GetPriority();

				}				
			}
        }
    }

	if( pPickedTask != NULL )
		return( pPickedTask );

	// if no unassigned construction then consider finding the
	// road construction task in process that could use some help
	pPickedTask = FindTask( IDT_CONSTRUCT );
    return( pPickedTask );
}

//
// find the highest priority production task that can be
// handled by the CStructureData type of production facility
// passed and return a pointer to the task
//
CAITask *CAITaskList::GetProductionTask( int iFactoryType )
{
	ASSERT_VALID( this );

	CAITask *pPickedTask = NULL;
	int iPriority = 0;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

#if 0
			if( pTask->GetID() == IDT_MAKELSCOUT ||
				pTask->GetID() == IDT_MAKEMSCOUT ||
				pTask->GetID() == IDT_MAKEHSCOUT )
				iPriority = 0;
#endif
			// tasks with 0 priority may not be researched
			if( pTask->GetTaskParam(ORDER_TYPE) == PRODUCTION_ORDER &&
				(pTask->GetTaskParam(PRODUCTION_TYPE) == CStructureData::UTvehicle ||
				pTask->GetTaskParam(PRODUCTION_TYPE) == CStructureData::UTshipyard) )
			{
				// can this vehicle, from the task, be produced at this factory
				if( pTask->GetTaskParam(PRODUCTION_ID1) != iFactoryType &&
					pTask->GetTaskParam(PRODUCTION_ID2) != iFactoryType )
					continue;

				// BUGBUG this is for testing only
				//if( pTask->GetID() == IDT_MAKELSCOUT )
				//	return( pTask );

				// get the last task in the list, with the highest priority
				// but it has to have some priority to be picked
				if( (int)pTask->GetPriority() &&
					(int)pTask->GetPriority() >= iPriority )
				{
					pPickedTask = pTask;
					iPriority = (int)pTask->GetPriority();
				}
			}
        }
    }

	return( pPickedTask );
}
//
// consider the combat tasks and select the one
// with the highest priority
//
CAITask *CAITaskList::GetCombatTask( int iUnitType, CAITask *pSkipTask /*= NULL*/ )
{
	ASSERT_VALID( this );

	// get pointer to vehicle type data
	CTransportData const *pVehData = theTransports.GetData(iUnitType);
	if( pVehData == NULL )
		return( NULL );

	if( pVehData->GetTargetType() == CUnitData::naval )
		return( NULL );

	CAITask *pPickedTask = NULL;
	int iPriority = 0;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			if( pTask->GetStatus() != UNASSIGNED_TASK )
				continue;
			if( pTask->GetType() != COMBAT_TASK )
				continue;

			// NOTE hard coded this test so we don't have
			// to do a lookup in the standard goal list 
			WORD wGoal = pTask->GetGoalID();
			if( wGoal != IDG_LANDWAR &&
				wGoal != IDG_BASICDEFENSE &&
				wGoal != IDG_ADVDEFENSE )
				continue;

			// BUGBUG for testing, force this task
			//if( pTask->GetID() == IDT_PREPAREWAR )
			//	return( pTask );
			// BUGBUG testing
			//if( pTask->GetID() == IDT_PATROL )
			//	continue;
			
			// BUGBUG skip for now, no code handles this yet
			if( pTask->GetID() == IDT_ESCORT )
				continue;

			// skip the passed task
			if( pSkipTask != NULL &&
				pTask == pSkipTask )
				continue;

			if( (int)pTask->GetPriority() > iPriority )
			{
				pPickedTask = pTask;
				iPriority = (int)pTask->GetPriority();
			}
		}
	}

	return( pPickedTask );
}

CAITask *CAITaskList::GetNavyTask( int iUnitType )
{
	ASSERT_VALID( this );

	// get pointer to vehicle type data
	CTransportData const *pVehData = theTransports.GetData(iUnitType);
	if( pVehData == NULL )
		return( NULL );

	//if( pVehData->GetTargetType() != CUnitData::naval )
	//	return( NULL );

	// some vehicles come into this, which are not naval vehicles
	// and still need a navy task cause its an amphibious assault
	BOOL bAmphib = FALSE;
	switch( iUnitType )
	{
		case CTransportData::rangers:
		case CTransportData::light_tank:
		case CTransportData::med_tank:
		case CTransportData::light_art:
			bAmphib = TRUE;
		default:
			break;
	}


	CAITask *pPickedTask = NULL;
	int iPriority = 0;

    POSITION pos = GetHeadPosition();
    while( pos != NULL )
    {   
#if 0 //THREADS_ENABLED
		// this function is not yielding
		myYieldThread();
		//if( myYieldThread() == TM_QUIT )
		//	throw(ERR_CAI_TM_QUIT); // THROW( pException );
#endif
        CAITask *pTask = (CAITask *)GetNext( pos );
        if( pTask != NULL )
        {
        	ASSERT_VALID( pTask );

			if( pTask->GetStatus() != UNASSIGNED_TASK )
				continue;
			if( pTask->GetType() != COMBAT_TASK )
				continue;
			
			// NOTE hard coded this test so we don't have
			// to do a lookup in the standard goal list 
			WORD wGoal = pTask->GetGoalID();
			if( wGoal != IDG_PIRATE &&
				wGoal != IDG_SHORES &&
				wGoal != IDG_REPELL &&
				wGoal != IDG_SEAWAR &&
				wGoal != IDG_SEAINVADE )
				continue;

			if( bAmphib )
			{
				if( wGoal != IDG_SEAINVADE ||
					pTask->GetID() != IDT_PREPAREWAR )
					continue;
			}

			//if( bAmphib && 
			//	(wGoal == IDG_PIRATE ||
			//	wGoal == IDG_SHORES ||
			//	wGoal == IDG_REPELL) )
			//	continue;

			if( (int)pTask->GetPriority() > iPriority )
			{
				pPickedTask = pTask;
				iPriority = (int)pTask->GetPriority();
			}
		}
	}

	return( pPickedTask );
}


// end of CAITask.cpp
