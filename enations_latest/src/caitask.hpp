////////////////////////////////////////////////////////////////////////////
//
//  CAITask.hpp :  CAITask object
//                 Divide and Conquer AI
//               
//  Last update:    07/04/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include <afxwin.h>

#ifndef __CAITASK_HPP__
#define __CAITASK_HPP__

class CAITask : public CObject
{
protected:

	WORD m_wID;					// unique id of the Task

	BYTE m_cType;				// ORDER, INFO, MESSAGE, COMBAT
	BYTE m_cStatus;				// UNASSIGNED, INPROCESS, COMPLETED
	WORD m_wGoalID;				// indicates the goal of the task
	WORD m_wOrderID;			// the CNetCmd order for this task

	WORD *m_pwaParams;	// task parameters

	BYTE m_cPriority;			// priority to complete: 0=low 100=high

public:

	CAITask() {};
	~CAITask();
	CAITask( WORD wID, BYTE cType, BYTE cPriority, 
		WORD wOrderID, CWordArray *pwaParams );

	WORD GetID( void );

	BYTE GetType( void );
	BYTE GetStatus( void );
	void SetStatus( BYTE cStatus );

	WORD GetGoalID( void );
	void SetGoalID( WORD );
	WORD GetOrderID( void );

	BYTE GetPriority( void );
	void SetPriority( BYTE );
	WORD GetTaskParam( WORD wOffset );
	void SetTaskParam( WORD wOffset, WORD wValue );
	
	CAITask *CopyTask( void );
};

class CAITaskList : public CObList
{
public:

	CAITaskList() {};
	~CAITaskList();
	
	CAITask *FindTask( WORD wId );
	CAITask *GetTask( WORD wID, WORD wGoal );
	void RemoveTask( WORD wID, WORD wGoal );
	void DeleteList( void );

	CAITask *GetUnassignedTask( int iTask );
	CAITask *GetPatrolTask( int iType );
	CAITask *GetScoutTask( void );
	CAITask *GetConstructionTask( void );
	CAITask *GetProductionTask( int iFactoryType );
	CAITask *GetCombatTask( int iUnitType, CAITask *pSkipTask = NULL );
	CAITask *GetNavyTask( int iUnitType );
};

#endif // __CAITASK_HPP__
