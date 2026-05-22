////////////////////////////////////////////////////////////////////////////
//
//  CAIGoal.hpp :  CAIGoal object
//                 Divide and Conquer AI
//               
//  Last update:    12/14/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////


// Phase 1g step 2: CObject from mfc_compat.h under gate.
#if defined(ENATIONS_USE_STUB_WND) && defined(ENATIONS_USE_STUB_APP)
#include <windows.h>
#else
#include <afxwin.h>
#endif

#ifndef __CAIGOAL_HPP__
#define __CAIGOAL_HPP__

//
// declares primary/secondary goal objects which are used to contain
// specific data for the goals to be achieved by the AI
//
class CAIGoal : public CObject
{
protected:

	WORD m_wID;			// unique id of the goal

	WORD *m_pwaTasks;	// array of tasks of the goal
	int m_iNumTasks;

	BYTE m_cType;		// TRADE, INDUSTRY, WARFARE
	
public:

	CAIGoal() {};
	~CAIGoal();
	CAIGoal( WORD wID, BYTE cType, CWordArray *pwaTasks );
		
	WORD GetID( void );
	BYTE GetType( void );
	WORD GetTaskAt( int iAt );
	WORD GetFirstTask( void );
	WORD GetNextTask( WORD wPriorID );

	CAIGoal *CopyGoal( void );
};

class CAIGoalList : public CObList
{
public:

	CAIGoalList() {};
	~CAIGoalList();

	BOOL Initialize( CAIGoalList *plGoals );
	
	CAIGoal *GetGoal( WORD wID );
	void RemoveGoal( WORD wID );
	void DeleteList( void );
};

#endif // __CAIGOAL_HPP__

