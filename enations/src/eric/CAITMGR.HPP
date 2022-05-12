////////////////////////////////////////////////////////////////////////////
//
//  CAITMgr.hpp :  CAITaskMgr object declaration
//                 the Last Planet - AI
//               
//  Last update:    10/25/96
//
//  Copyright 1995,1996 Glacier Edge Technology - All Rights Reserved
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAIGMgr.hpp"

#ifndef __CAITMGR_HPP__
#define __CAITMGR_HPP__

class CAITaskMgr : public CObject
{
protected:
	int m_iPlayer;
	BOOL m_bRestart;
	
	CAIGoalMgr *m_pGoalMgr;	// this player's goal manager
	
public:	
	CAITaskMgr( BOOL bRestart, int iPlayer,
		CAIGoalMgr *pGoalMgr );
	~CAITaskMgr();

	void UpdateTasks( CAIMsg *pMsg );
	void Manage( CAIMsg *pMsg );

	void ConstructionError( CAIMsg *pMsg );
	BOOL AssignProductionTask( CAIUnit *pUnit );

	void AssignUnits( void );
	void BalancePatrols( void );
	BOOL CanPatrol( CAIUnit *pPatrolUnit, CAIUnit *pPatrolBldg );
	BOOL IsPatrolled( CAIUnit *pBldg );
	void FindAvailablePatrol( CAIUnit *pBldg );
	void BalancePatrols( CAIMsg *pMsg );

	void CheckFactorys( void );
	void UnitControl( CAIMsg *pMsg );
	void GenerateTaskOrder( CAIUnit *pUnit );
	void UnAssignTask( WORD wTask, WORD wGoal );

	void AssignInitialAssault(void);
	void AssignTask( CAIUnit *pUnit );

	void AssignNavy( CAIUnit *pUnit );
	void AssignCombat( CAIUnit *pUnit );
	void AssignTransport( CAIUnit *pUnit );
	void AssignPatrol( CAIUnit *pUnit );
	void AssignScout( CAIUnit *pUnit );
	void AssignConstruction( CAIUnit *pUnit );
	BOOL AssignRepair(CAIUnit *pCrane);

	void RepairingUnit( CAIUnit *paiUnit );
	void ReconOpForCity( CAIUnit *pCbtVeh, CAITask *pTask );
	void AttackUnit( CAIUnit *pUnit, CAITask *pTask );
	void StageUnit( CAIUnit *pCbtVeh, CAITask *pTask );
	void SeekOpfor( CAIUnit *pUnit, CAITask *pTask );
	void ProduceVehicle( CAIUnit *pUnit, CAITask *pTask );

	void PatrolSea( CAIUnit *pUnit, CAITask *pTask, 
		CTransportData const *ptdShip );
	void PatrolArea( CAIUnit *pUnit, CAITask *pTask );
	void ExploreMap( CAIUnit *pUnit, CAITask *pTask );

	BOOL IsStagingCompete( CAITask *pTask, int iType =0 );
	BOOL ContinueStaging( CAIUnit *pStagingVeh, CAITask *pTask );

	void BuildRoad( CAIUnit *pUnit, CAITask *pTask );
	void ConstructBuilding( CAIUnit *pUnit, CAITask *pTask );
	BOOL RepairConstruction( CAIUnit *pUnit );

	void ConsiderPatrols( CAITask *pTask );
	BOOL InRange( CHexCoord& hehVeh, int iSpotting );
	BOOL InRange( CAIUnit *pUnit,  CHexCoord& hex );
	void LoadCargo( CAIUnit *pUnit, CAITask *pTask );
	void LoadIFV( CAIUnit *pUnit, CAITask *pTask );
	void LoadLandingCraft( CAIUnit *pUnit, CAITask *pTask );
	void LoadTroops( CAIUnit *pUnit, CAITask *pTask );
	CAIUnit *FindTroopToLoad( CHexCoord& hexCarrier, 
		CAITask *pTask, int& iDist, int iMarines );

	void UnloadCargo( CAIUnit *pUnit );
	void MoveToRange(CAIUnit *pUnit,  CHexCoord hex);

	void SetTaskForceDestination( int iX, int iY, CAITask *pTask );
	void ClearTaskUnit( CAIUnit *pUnit );

	void AttackAlert( CAIMsg *pMsg );
	void SpottingAlert( CAIMsg *pMsg );
	void RunAway( CAIUnit *pTargeted, CAIUnit *pAttacker, CAIMsg *pMsg );
	void SyncStageCargo( CAIUnit *pCarrier, CAITask *pTask );
	void LoadPatrolCargo( CAIUnit *pUnit );
	BOOL FindPatrolCargo( CAIUnit *pCarrier, CAITask *pTask );
	BOOL FindPatrolCarrier( CAIUnit *pCargo, CAITask *pTask );
	void RepairUnit( CAIMsg *pMsg, int iDmgPer );
	BOOL TargetVehicles( CAIUnit *pAIUnit );
	void ClearTarget( CAIMsg *pMsg );
};

#endif // __CAITMGR_HPP__
