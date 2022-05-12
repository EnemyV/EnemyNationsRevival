////////////////////////////////////////////////////////////////////////////
//
//  CAIGoalM.hpp :  CAIGoalMgr object declaration
//               	Divide and Conquer AI
//               
//  Last update:    09/27/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIGoal.hpp"
#include "CAIMap.hpp"
#include "CAIUnit.hpp"
#include "CAIOpFor.hpp"


#ifndef __CAIGOALMGR_HPP__
#define __CAIGOALMGR_HPP__

class CAIGoalMgr : public CObject
{
protected:
	int m_iPlayer;
	BOOL m_bRestart;
	BOOL m_bHaveCoastline;	// one or more CAIMaps has a coastline

	BOOL m_bProductionChange;	// indicates an event occurred
								// that effected production
	BOOL m_bMapChanged;		// indicates an event occurred
							// that effected the map
	BOOL m_bGoalChange;		// indicates a goal was added or
							// removed from this goal list

	POSITION m_pos;				// general pos used for inerating
	CAIGoalList *m_plGoalList;	// this player's goal list
	
public:	

	CAIMapList *m_plMaps;	// list of CAIMgr's CAIMaps
	CAIUnitList *m_plUnits;	// list of CAIMgr's CAIUnits

	CWordArray *m_pwaRatios;	// building ratios adjusted by
								// effect of player attribs

								// actual counts of units
	CWordArray *m_pwaUnits;		// array of counts of CAI units
								// in vehicle type order by offset
	CWordArray *m_pwaBldgs;		// array of counts of CAI buildings
								// in building type order by offset
	CWordArray *m_pwaMatOnHand; // array of materials on hand

	CWordArray *m_pwaAttribs;	// array of CPlayer::m_iAttrib[]

	CAIOpForList *m_plOpFors;	// list of opposing players known
								// to this manager

	CWordArray *m_pwaBldgGoals;	// array of CBuilding goals
	CWordArray *m_pwaVehGoals;	// array of CVehicle goals
	CWordArray *m_pwaMatGoals;	// array of materials needed for goals

	CAIGoalMgr( BOOL bRestart, int iPlayer,
		CAIMapList *plMaps, CAIUnitList *plUnits,
		CAIOpForList *plOpFors );
	~CAIGoalMgr();
	
	void SetProductionChange( void );
	void SetMapChange( void );
	BOOL GetGoalChange( void );
	void ResetGoalChange( void );

	void UpdateSummarys( CAIMsg *pMsg );

	void AddGoal( WORD wGoal );

	void Initialize( void );
	void InitPlayer( void );
	void InitVehGoals( void );
	void InitVehSummary( void );
	void InitBldgGoals( void );
	void InitBldgSummary( void );
	void InitMatGoals( void );
	void InitMatOnHand( void );

	BOOL Manage( CAIMsg *pMsg );
	void Assess( CAIMsg *pMsg );

	void ConsiderProduction( CAIMsg *pMsg );
	//void GetGoalEconomics( void );
	//void UpdateGoalCounts( void );
	void GetCommodityNeeds( void );
	void SetGoalEconomics( void );

	void ConstructBuilding( void );
	void ProduceVehicle( void );
	void ProduceMaterial( void );

	void ConsiderThreats( CAIMsg *pMsg );
	void ConsiderRelations( CAIMsg *pMsg );

	CAIGoal *FirstGoal( void );	// provides an external access
	CAIGoal *NextGoal( void );	// to this mgr's goal list
	CAIGoal *GetGoal( WORD wID );
	
	WORD GetVehGoals( WORD wOffset );
	WORD GetVehicle( WORD wOffset );
	void SetVehicle( WORD wOffset, WORD wCnt );
	WORD GetBuilding( WORD wOffset );
	void SetBuilding( WORD wOffset, WORD wCnt );
	
	// BUGBUG need to know what/how save/loads are to occur
	void Load( void );
	void Save( void );

	void DelBldgGoals( void );
	void DelVehGoals( void );
	void ClearMatOnHand( void );
	void ClearMatGoals( void );
	void DelMatGoals( void );
	void DelMatOnHand( void );
};

#endif // __CAIGOALMGR_HPP__
