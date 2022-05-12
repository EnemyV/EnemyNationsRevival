////////////////////////////////////////////////////////////////////////////
//
//  CAIMgr.hpp :  CAIMgr object declaration
//                Divide and Conquer AI
//               
//  Last update:  10/17/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "CAITMgr.hpp"
#include "CAIRoute.hpp"

#ifndef __CAIMGR_HPP__
#define __CAIMGR_HPP__

// number of functions that will use the idle time (no messages)
// to do stuff
#define MAX_IDLE_FUNCTIONS	7

//
// this class provides the highest level of management for the AI
//
class CAIMgr : public CObject
{
	// STATE flags, all finite and message related
	//
	BOOL m_bMsgArrived;	// flag indicating that a message has arrived
						// and has not yet been handled
	BOOL m_bTaskChange; // flag indicating a task was changed by
						// a message
	BOOL m_bGoalChange; // flag indicating a goal was affected by
						// a message

	BOOL m_bGoalAssess; // flag indicating goals need to be reassessed
	BOOL m_bRelationChange;
	BOOL m_bProductionChange;
	BOOL m_bMapChanged;
	BOOL m_bAttackOccurred;
	BOOL m_bVehicleArrived;
	BOOL m_bVehicleError;
	BOOL m_bUnitDestroyed;
	BOOL m_bLocChanged;
	BOOL m_bCheckResearch;
	BOOL m_bPlaceRocket;
	BOOL m_bPlayerDying;
	BOOL m_bScenario;
	BOOL m_bAttackUnit;
	BOOL m_bAttackPlayer;
	BOOL m_bPlaceBldg;
	BOOL m_bOutLOS;
	BOOL m_bUnitLoaded;
	BOOL m_bGiveUnit;

	// BUGBUG temporary for testing
	int m_iAIShooter; 
	int m_iHPShooter;

	int m_iScenario;

	CRITICAL_SECTION m_cs; // internal use only, for message input/update

	int m_iPlayer;	// this player's id to the game
	BOOL m_bIsAI;	// TRUE indicates the player is AI
					// FALSE indicates the player is HP
public:
	BOOL m_bIsDead; // TRUE indicates that AiKillPlayer() was called
private:

	int m_iIdle;	// counter for cycles without messages (idle time)
	BOOL m_bIdleFunction[MAX_IDLE_FUNCTIONS];

	int m_iBlockX;	// original location provided by the game, it is usually
	int m_iBlockY;	// the center of the block to which this player was placed

	CAIGoalMgr *m_pGoalMgr;	// this manager's goals
	CAITaskMgr *m_pTaskMgr; // this manager's tasks
	CAIRouter *m_pRouter;	// this manager's router

	CAIOpForList *m_plOpFors;	// list of opposing players known
								// to this manager
	CAIUnitList *m_plUnits;	// list of all CAUnits for this manager
	
	CAIMap *m_pMap;	// view of the world for this player
	CObList *m_plMsgQueue;	// a queue of messages for this manager
	CObList *m_plTmpQueue;	// messages as arrived

	// NOTE may want to switch to this type of container
	//CList<CAIMsg *,CAIMsg *> *m_plMsgQueue;
	//CList<CAIMsg *,CAIMsg *> *m_plTmpQueue;
	
public:

	CAIMgr( int iPlayer );
	~CAIMgr();

	int GetPlayer( void );
	BOOL IsAI( void );
	void SetAI( BOOL );
	void SetDead( void );
    void Manage( void );

	void UpdateLocation( CAIMsg *pMsg );
	void UpdatePlayer( CAIMsg *pMsg );
	void UpdateUnits( CAIMsg *pMsg );

	//void ResearchCompleted( int iTopic );
	int NextTopic( CPlayer *pPlayer );

	void ReplaceRocket( CAIMsg *pMsg );
	void SetInitialPos( void );
	void SaveGame( CFile *pFile );
	void Save( CFile *pFile );
	void LoadGame( CFile *pFile );
	void Load( CFile *pFile );

	void AssumeControl( int iBlockX, int iBlockY );
	void CreateData( int iBlockX, int iBlockY );
	void CreateOpFors( void );
	void ConvertHPUnits( void );
    void CreateCAUnits( void );
    void CreateManagers( void );
    void CreateMap( void );
    
	void FakeOutMatMsg( CAIUnit *paiBldg );
    void MessageArrived( CNetCmd const *pMsg );
	void PrioritizeMessage( void ); //CAIMsg *pNewMsg );
	void ProcessMessage( CAIMsg *pMsg );

	void HandleStuckVehicles( void );
	void ResignGame( void );
	BOOL IsEmbraced( CAIMsg *pMsg );
	void VehicleErrorResponse( CAIMsg *pMsg );
	void DestinationResponse( CAIMsg *pMsg );
	void AttackResponse( CAIMsg *pMsg );
	void AttackUnit( CAIMsg *pMsg );
	void LoadedResponse( CAIMsg *pMsg );
	void PlaceBldg( CAIMsg *pMsg );
	void OutLOSResponse( CAIMsg *pMsg );
    
	BOOL AutoFire( CUnit *pFiring, CUnit const *pTarget );
    void AdjustAttribs( CPlayer *pPlayer );
    void AddInitPosUnit( CAIMsg *pMsg );

#ifdef _LOGOUT
	void ReportPlayerStats( void );
	void SumUpMaterialOnHand( void );
#endif
};

class CAIMgrList : public CObList
{
public:

	CAIMgrList();
	//CAIMgrList() {};
	~CAIMgrList();
    
    void LoadStandardData( void );
    void DeleteManager( int iPlayer );
    void RemoveManager( int iPlayer );
	CAIMgr *GetManager( int iPlayer );
	void DeleteList( void );
};

#endif // __CAIMGR_HPP__
