////////////////////////////////////////////////////////////////////////////
//
//  CAIGMgr.hpp :  CAIGoalMgr object declaration
//                 the Last Planet AI
//               
//  Last update:    10/30/96
//
//  Copyright 1995, 1996 Glacier Edge Technology - All Rights Reserved
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIGoal.hpp"
#include "CAITask.hpp"
#include "CAIMap.hpp"
#include "CAIUnit.hpp"
#include "CAIOpFor.hpp"

#ifndef __CAIGMGR_HPP__
#define __CAIGMGR_HPP__

// the number of different types of units, that can be staged for an assault
// with a specific staging task
#define STAGING_UNITTYPES	4
#define STAGING_UPDATES		STAGING_UNITTYPES+1
#define	STAGING_TASKS		3

class CAIGoalMgr : public CObject
{
protected:
	int m_iPlayer;

	BOOL m_bRestart;

	BOOL m_bProductionChange;	// indicates an event occurred
								// that effected production
	BOOL m_bMapChanged;		// indicates an event occurred
							// that effected the map
	BOOL m_bGoalChange;		// indicates a goal was added or
							// removed from this goal list
	BOOL m_bGunsOrButter;	// TRUE = combat production gets priority
							// FALSE = non-combat vehicles need it
	BOOL m_bOceanWorld;		// TRUE - there is ocean on this world
							// FALSE - turn off all ocean required tasks
	BOOL m_bLakeWorld;		// TRUE - there is lake on this world
							// FALSE - turn off all lake required tasks
	BOOL m_bNeedTrucks;
	BOOL m_iNeedApt;
	BOOL m_iNeedOffice;

	int m_iLastFood;
	int m_iScenario; 	// 0=none, else the number of the current scenario



	POSITION m_pos;				// general pos used for inerating
	CAIGoalList *m_plGoalList;	// this player's goal list
	BOOL m_cPriority;

	int m_iNumRatios;
	int m_iNumUnits;		// used in m_pwaUnits && m_pwaVehGoals
	int m_iNumBldgs;		// used in m_pwaBldgs && m_pwaBldgGoals
	int m_iNumMats;			// used in m_pwaMatOnHand && m_pwaMatGoals
	int m_iNumAttribs;

	int m_iRDPath[CRsrchArray::num_types];	  // order in which to research topics	
	
	
public:	

	int m_iGasHave;			// updated regularly, no need to save
	int m_iGasNeed;
	int m_iPowerHave;		
	int m_iPowerNeed;
	int m_iPowerLvl;		// indicate type of power plant we can build

	DWORD m_dwRocket;		// id of the rocket building
	CAIMap *m_pMap;	// CAIMgr's CAIMap
	CAIUnitList *m_plUnits;	// list of CAIMgr's CAIUnits
	CAITaskList *m_plTasks;	// this player's list of tasks

	WORD *m_pwaRatios;	// building ratios adjusted by
						// effect of player attribs


							// actual counts of units
	WORD *m_pwaUnits;		// array of counts of CAI units
							// in vehicle type order by offset
	

	WORD *m_pwaBldgs;		// array of counts of CAI buildings
							// in building type order by offset
	

	WORD *m_pwaMatOnHand;	// array of materials on hand
	

	WORD *m_pwaAttribs;		// array of CPlayer::m_iAttrib[]
	

	WORD *m_pwaBldgGoals;	// array of CBuilding goals
	WORD *m_pwaVehGoals;	// array of CVehicle goals
	WORD *m_pwaMatGoals;	// array of materials needed for goals

	CAIOpForList *m_plOpFors;	// list of opposing players known
								// to this manager

	CAIGoalMgr( BOOL bRestart, int iPlayer,
		CAIMap *pMap, CAIUnitList *plUnits,
		CAIOpForList *plOpFors );
	~CAIGoalMgr();
	
	void SetProductionChange( void );
	void SetMapChange( void );
	BOOL GetGoalChange( void );
	void ResetGoalChange( void );

	void UpdateTaskPriorities( CAIMsg *pMsg );
	void UpdateSummarys( CAIMsg *pMsg );

	BOOL RealityCheck( DWORD dwOpforID );
	int GetCombat( void );
	CAIOpFor *GetOpFor( int iOpForID );
	void AddGoal( WORD wGoal );

	void Initialize( void );
	void InitializeTasks( void );
	void InitPlayer( void );
	void InitVehGoals( void );
	void InitVehSummary( void );
	void InitBldgGoals( void );
	void InitBldgSummary( void );
	void InitMatGoals( void );
	void InitMatOnHand( void );

	void Assess( CAIMsg *pMsg );

	void ConsiderProduction( CAIMsg *pMsg );
	void GetVehicleNeeds();
	void GetCommodityNeeds( void );
	void SetStartVehicles( CAIMsg *pMsg );
	void UpdateVehGoals( void );

	void ClearStagingVehicle( int iVeh, WORD *awTypes );
	void ProducingStagingVehicle( WORD *awTypes );
	void SetAssaultStagingVehicle( WORD *awTypes );
		
	void ProduceVehicles( int iVeh );
	void ProduceMaterials( int iMat );
	BOOL AddNewProducer( int iBldg, int iMat);
	BOOL AddNewFactory( int iBldg1, int iBldg2,
		WORD& wTask, WORD& wGoal );

	BOOL AutoFire( int iPlayer );
	void ConsiderThreats( CAIMsg *pMsg );
	void AdjustThreats( void );
	void AttackPlayer( int iOpforID );
	void Scenario( int iScenario );
	void ScenarioPriority( CAITask *pTask );

	void ConsiderRoads( void );
	BOOL IsGasAvailable( void );
	BOOL NeedGas( void );
	void ConsiderTrucks( CAIMsg *pMsg );
	BOOL NeedCranes( void );
	void NeedTrucks( void );
	void IdleCrane( void );
	void CheckPlayer( void );
	int GetNumVehToBuild( int iVehType );
	int GetPowerToBe( void );
	void AddPowerTask( void );
	void ResearchCompleted( int iTopic );
	int NextResearchTopic( CPlayer *pPlayer );
	void CheckResearch( void );
	BOOL CheckAbandonedBuildings( void );
	BOOL IsAbandonedBuilding( int iBldg );

	CAIGoal *FirstGoal( void );	// provides an external access
	CAIGoal *NextGoal( void );	// to this mgr's goal list
	CAIGoal *GetGoal( WORD wID );

	void AddTask( CAITask *pTask );
	void RemoveTask( CAITask *pTask );
	void InitTasks( CAIGoal *pGoal );
	void DeleteTasks( void );
	
	void InitTaskPriorities( void );
	void UpdateInfoTasks( CAIMsg *pMsg );
	void UpdateConstructionTasks( CAIMsg *pMsg );
	void UpdateCombatTasks( CAIMsg *pMsg );
	void UpdateProductionTasks( CAIMsg *pMsg );
	void UpdateStagingTasks( void );
	void ConsiderReassignment( CAITask *pToTask, 
		CAITask *pCntTask, CAITask *pFromTask, WORD wUnitCat );


	WORD GetVehGoals( WORD wOffset );
	WORD GetVehicle( WORD wOffset );
	void SetVehicle( WORD wOffset, WORD wCnt );
	WORD GetBuilding( WORD wOffset );
	void SetBuilding( WORD wOffset, WORD wCnt );
	WORD GetBuildingCnt( void );

	BOOL GetPathRating( CHexCoord& hexVeh, CHexCoord& hexAttacked, int iVehType );

	// unit type specific threat assessment routines <g>
	int AssessThreat(CBuilding *pData, int iKindOf);
	int AssessThreat(CVehicle *pData, int iKindOf);

	CAITask *GetProductionTask(CAIUnit *pUnit);
	void LaunchAssault( CAITask *pTask );
	void ReconInForce( CAITask *pTask, CHexCoord hcTo );
	void LaunchAssault( CAITask *pTask, CHexCoord hcTo );
	void CancelAssault( CAITask *pTask );
	void UpdateTaskForce( CAITask *pTask, 
		CHexCoord& hcStart, CHexCoord& hcEnd, WORD wNewTask );

	BOOL IsTargetReachable( CHexCoord& hexTarget, CAITask *pTask );
	void FindAssaultTarget( CHexCoord& hexTarget, 
		CAITask *pTask, CAIOpFor *pOpFor );

	//void GetSupport( CAIUnit *pTarget, 
	//	CAIUnit *pAttacker, CAIMsg *pMsg );
	void KillOpfor( int );

	DWORD GetOpforTarget( int iOpForID, int iVehType );
	DWORD GetOpForUnit( int iHow, int iKindOf, CAIUnit *pUnit );
	void SetMapOpFor( void );
	void SetKnownOpFor( int iOpForID );
	int GetOpForId( int iType, CHexCoord hex, BOOL bKnown );

	CAITask *GetPatrolTask( int iUnitType );
	void FindInfStagingArea( CAITask *pTask );
	BOOL CanBuildVehType( int iVehType );
	void GetNewStagingArea( CAITask *pTask );
	void GetStagingArea( CAITask *pTask );
	void GetScoutBaseHex( CAITask *pTask );
	void ConvertPatrolPoints( CAIHex *pHexes, 
		CHexCoord& hcStart, CHexCoord& hcEnd );
	void GetSeaPatrol( CAIUnit *pUnit, CAITask *pTask, 
		CTransportData const *ptdShip );
	DWORD GetPatrolBuilding( CAITask *pTask, BOOL bSeaBuilding=FALSE );
	void GetPatrolHex( CAIUnit *pUnit, CHexCoord& hex );

	//void GetPatrolPoints( CAITask *pTask );
	
	// BUGBUG need to know what/how save/loads are to occur
	void Load( CFile *pFile, CAIMap *pMap, 
		CAIUnitList *plUnits, CAIOpForList *plOpFors );
	void Save( CFile *pFile );

	void ClearVehGoals( void );
	void ClearVehSummary( void );

	void DelBldgGoals( void );
	void DelVehGoals( void );
	void ClearMatOnHand( void );
	void ClearMatGoals( void );
	void DelMatGoals( void );
	void DelMatOnHand( void );

#ifdef _LOGOUT
	void ReportSummaries( void );
#endif
};

#endif // __CAIGMGR_HPP__
