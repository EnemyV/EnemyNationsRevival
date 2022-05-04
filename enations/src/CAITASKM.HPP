////////////////////////////////////////////////////////////////////////////
//
//  CAITaskM.hpp :  CAITaskMgr object declaration
//               	Divide and Conquer AI
//               
//  Last update:    09/15/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAITask.hpp"
#include "CAIGoalM.hpp"

#ifndef __CAITASKMGR_HPP__
#define __CAITASKMGR_HPP__

class CAITaskMgr : public CObject
{
protected:
	int m_iPlayer;
	BOOL m_bRestart;
	CAITaskList *m_plTasks;	// this player's list of tasks
	CAIGoalMgr *m_pGoalMgr;	// this player's goal manager
	
public:	
	CAITaskMgr( BOOL bRestart, int iPlayer,
		CAIGoalMgr *pGoalMgr );
	~CAITaskMgr();

	void Initialize( void );
	void UpdateGoalCounts( void );
	void UpdateTasks( CAIMsg *pMsg );
	void AddTask( CAITask *pTask );
	void RemoveTask( CAITask *pTask );
	void InitTasks( CAIGoal *pGoal );
	void Manage( CAIMsg *pMsg );
	void GenerateTaskOrder( CAIUnit *pUnit );
	void AssignTask( CAIUnit *pUnit );
	void AssignConstruction( CAIUnit *pUnit );
	void ConstructBuilding( CAIUnit *pUnit, CAITask *pTask );

	// BUGBUG need to know what/how save/loads are to occur
	void Load( void );
	void Save( void );
	void DeleteTasks( void );
};

#endif // __CAITASKMGR_HPP__
