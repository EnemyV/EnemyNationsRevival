////////////////////////////////////////////////////////////////////////////
//
//  CAIMsg.cpp :  CAIMsg object implementation
//                Divide and Conquer AI
//               
//  Last update:    09/17/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAIMsg.hpp"
#include "CAIData.hpp"

IMPLEMENT_SERIAL( CAIMsg, CObject, 0 );

#define new DEBUG_NEW

CAIMsg::CAIMsg( CAIMsg *SrcMsg )
{
	m_iMsg = SrcMsg->m_iMsg;
	m_uFlags = SrcMsg->m_uFlags;
	m_iPriority = SrcMsg->m_iPriority;
	m_dwID = SrcMsg->m_dwID;
	m_iX = SrcMsg->m_iX;
	m_iY = SrcMsg->m_iY;
	m_ieX = SrcMsg->m_ieX;
	m_ieY = SrcMsg->m_ieY;
	m_idata1 = SrcMsg->m_idata1;
	m_idata2 = SrcMsg->m_idata2;
	m_idata3 = SrcMsg->m_idata3;
	m_dwID2 = SrcMsg->m_dwID2;
}

CAIMsg::CAIMsg( CNetCmd const *pMsg )
{
	CMsgRoadNew *pRoadNewMsg = NULL;
	CMsgBuildBldg *pBldBldg = NULL;
	CMsgVehDest *pDestMsg = NULL;
	CMsgVehLoc *pLocMsg = NULL;
	CMsgVehStat *pVStatMsg = NULL;
	CMsgVehNew *pVNewMsg = NULL;
	CMsgUnitSetDamage *pSetDmgMsg = NULL;
	CMsgUnitDamage *pDmgMsg = NULL;
	CMsgRoadDone *pRoadMsg = NULL;
	CMsgBldgStat *pBStatMsg = NULL;
	CMsgBldgNew *pBNewMsg = NULL;
	CMsgVehGoto *pVehGoto = NULL;
	CMsgPlaceBldg *pPlaceBldg = NULL;
	CMsgDeleteUnit *pDelUnit = NULL;
	CMsgSeeUnit *pSeeUnit = NULL;
	CMsgUnitAttacked *pAttackedMsg = NULL;
	CMsgPlyrDying *pDyingMsg = NULL;
	CMsgRepaired *pRepairedMsg = NULL;
	CMsgScenario *pScenMsg = NULL;
	CMsgScenarioAtk *pScenAttkMsg = NULL;
	CMsgBuildCiv *pBldCivMsg = NULL;
	CMsgOutOfLos *pOutLOSMsg = NULL;
	CMsgBridgeNew *pBrdgNewMsg = NULL;
	CMsgBridgeDone *pBrdgDoneMsg = NULL;
	CMsgLoaded *pUnitLoadedMsg = NULL;
	CMsgUnitRepair *pUnitRepairMsg = NULL;
	CMsgAttack *pAttackMsg = NULL;
	CMsgGiveUnit *pGiveUnitMsg = NULL;


	m_iMsg = pMsg->GetType();
	switch( m_iMsg )
	{
		case CNetCmd::cmd_play:
			m_uFlags = 0;
			m_iPriority = 100;
			m_dwID = 0;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0; 
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = 0;
			m_dwID2 = (DWORD)0;
			break;

/*

class CMsgGiveUnit : public CNetCmd
{
public:
		CMsgGiveUnit () : CNetCmd (give_unit) {}
		CMsgGiveUnit ( CUnit const * pUnit, CPlayer const * pPlyr );

		int			m_iPlyrNum;
		int			m_dwID;
};
*/


		case CNetCmd::give_unit:
			pGiveUnitMsg = (CMsgGiveUnit *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pGiveUnitMsg->m_dwID;
			m_iX = 0; 
			m_iY = 0; 
			m_ieX = 0; 
			m_ieY = 0; 
			m_idata1 = pGiveUnitMsg->m_iPlyrNum;
			m_idata2 = 0;
			m_idata3 = 0;
			m_dwID2 = 0;
			break;

/*

// vehicle loaded on vehicle
class CMsgLoaded : public CNetCmd
{
public:
		CMsgLoaded () : CNetCmd (unit_loaded) {}
		CMsgLoaded (CVehicle const * pVehCarrier, CVehicle const * pCargo);

	DWORD			m_dwIDCarrier;
	DWORD			m_dwIDCargo;
};

*/

		case CNetCmd::unit_loaded:
			pUnitLoadedMsg = (CMsgLoaded *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pUnitLoadedMsg->m_dwIDCarrier;
			m_iX = 0; 
			m_iY = 0; 
			m_ieX = 0; 
			m_ieY = 0; 
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = 0;
			m_dwID2 = pUnitLoadedMsg->m_dwIDCargo;
			break;

/*
// tell a vehicle to build a bridge - it must ALREADY be on it
// the ends must be on land
class _CMsgBridge : public CNetCmd
{
public:
		_CMsgBridge () : CNetCmd (-1) {}
		_CMsgBridge (int iMsg) : CNetCmd (iMsg) {}

		DWORD				m_dwIDBrdg;		// ID of bridge
		int					m_iPlyrNum;		// player requesting the placement
		DWORD				m_dwIDVeh;		// ID of vehicle
		CHexCoord		m_hexStart;		// location to build at
		CHexCoord		m_hexEnd;
		int					m_iAlt;				// bridge altitude
		int					m_iMode;
						enum { one_hex, street };

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
// starting a bridge hex
class CMsgBridgeNew : public _CMsgBridge
{
public:
		CMsgBridgeNew () : _CMsgBridge (bridge_new) {}

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// bridge hex finished
class CMsgBridgeDone : public _CMsgBridge
{
public:
		CMsgBridgeDone () : _CMsgBridge (bridge_done) {}
		CMsgBridgeDone (CVehicle const *pVeh, CHexCoord const & hexStart, CHexCoord const & hexEnd);

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/


		case CNetCmd::bridge_new:
			pBrdgNewMsg = (CMsgBridgeNew *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pBrdgNewMsg->m_dwIDBrdg;
			m_iX = pBrdgNewMsg->m_hexStart.X();
			m_iY = pBrdgNewMsg->m_hexStart.Y();
			m_ieX = pBrdgNewMsg->m_hexEnd.X();
			m_ieY = pBrdgNewMsg->m_hexEnd.Y(); 
			m_idata1 = pBrdgNewMsg->m_iMode;
			m_idata2 = 0;
			m_idata3 = pBrdgNewMsg->m_iPlyrNum;
			m_dwID2 = pBrdgNewMsg->m_dwIDVeh;
			break;


		case CNetCmd::bridge_done:
			pBrdgDoneMsg = (CMsgBridgeDone *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pBrdgDoneMsg->m_dwIDBrdg;
			m_iX = pBrdgDoneMsg->m_hexStart.X();
			m_iY = pBrdgDoneMsg->m_hexStart.Y();
			m_ieX = pBrdgDoneMsg->m_hexEnd.X();
			m_ieY = pBrdgDoneMsg->m_hexEnd.Y(); 
			m_idata1 = pBrdgDoneMsg->m_iMode;
			m_idata2 = 0;
			m_idata3 = pBrdgDoneMsg->m_iPlyrNum;
			m_dwID2 = pBrdgDoneMsg->m_dwIDVeh;
			break;

/*
// build an apt/office
class CMsgBuildCiv : public CNetCmd
{
public:
		CMsgBuildCiv () : CNetCmd (build_civ) {}
		CMsgBuildCiv (CPlayer const * pPlyr, int iBldgNum);

	int				m_iPlyrNum;
	int				m_iBldgNum;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/

		case CNetCmd::build_civ:
			pBldCivMsg = (CMsgBuildCiv *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = 0;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0; 
			m_idata1 = pBldCivMsg->m_iBldgNum;
			m_idata2 = 0;
			m_idata3 = pBldCivMsg->m_iPlyrNum;
			m_dwID2 = (DWORD)0;
			break;

/*

// telling the AI a scenario is starting
class CMsgScenario : public CNetCmd
{
public:
		CMsgScenario ();

	int			m_iScenario;			// scenario number
	int			m_iPlyrHp;				// human player
	int			m_iPlyrAi;				// AI player
};

*/

		case CNetCmd::scenario:
			pScenMsg = (CMsgScenario *)pMsg;
			m_uFlags = 0;
			m_iPriority = 100;
			m_dwID = 0;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0; 
			m_idata1 = pScenMsg->m_iScenario;
			m_idata2 = pScenMsg->m_iPlyrHp;
			m_idata3 = pScenMsg->m_iPlyrAi;
			m_dwID2 = (DWORD)0;
			break;

/*
// telling the AI to attack a unit
class CMsgScenarioAtk : public CNetCmd
{
public:
		CMsgScenarioAtk (CUnit const *pTarget, int iPlyrAtk, DWORD dwAtk, int iWght);
		CMsgScenarioAtk (CUnit const *pTarget, CPlayer const *pAtk, int iWght);

	int			m_iPlyrHp;				// human player
	int			m_iPlyrAi;				// AI player to attack
	DWORD		m_dwIDTarget;			// target to attack
	DWORD		m_dwIDAtk;				// AI unit to attack (may be 0)
	int			m_iWeight;				// how important you do it
				enum { suggestion, important, must };
};
*/

		case CNetCmd::scenario_atk:
			pScenAttkMsg = (CMsgScenarioAtk *)pMsg;
			m_uFlags = pScenAttkMsg->m_iWeight;
			m_iPriority = 72;
			m_dwID = pScenAttkMsg->m_dwIDTarget;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0; 
			m_idata1 = pScenAttkMsg->m_iPlyrHp;
			m_idata2 = 0;
			m_idata3 = pScenAttkMsg->m_iPlyrAi;
			m_dwID2 = pScenAttkMsg->m_dwIDAtk;
			break;


/*
// sent when a vehicle reaches its dest hex
class CMsgVehDest : public CNetCmd
{
public:
		CMsgVehDest (CVehicle const *pVeh);

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the vehicle
		CHexCoord		m_hexDest;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::veh_dest:		// vehicle arrived at destination
			pDestMsg = (CMsgVehDest *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pDestMsg->m_dwID;
			m_iX = pDestMsg->m_hexDest.X();
			m_iY = pDestMsg->m_hexDest.Y();
			m_ieX = 0;
			m_ieY = 0; 
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = pDestMsg->m_iPlyrNum;
			m_dwID2 = (DWORD)0;
			break;

/*
// tell everyone where the vehicle is
class CMsgVehGoto : public _CMsgVehGo
{
public:
		CMsgVehGoto () : _CMsgVehGo (veh_goto) {}
		CMsgVehGoto (CVehicle const *pVeh);

		CMsgVehLoc *	ToLoc ( ) { m_iMsg = veh_loc; return ((CMsgVehLoc *) this); }
		void					ToErr () { m_iMsg = err_veh_goto; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class _CMsgVehGo : public CNetCmd
{
public:
		_CMsgVehGo () : CNetCmd (-1) {}
		_CMsgVehGo (int iMsg) : CNetCmd (iMsg) {}

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the vehicle
		CSubHex			m_ptDest;			// where we are going
		CSubHex			m_ptNext;			// the next sub-hex the vehicle will go to
		CSubHex			m_ptHead;			// the sub-hex the vehicle's head is in
		CSubHex			m_ptTail;			// the sub-hex the vehicle's tail is in
		CHexCoord		m_hexNext;		// the next hex the vehicle is going to in its path - m_ptNext may NOT be in it
		CHexCoord		m_hexDest;		// the final destination of the vehicle
		int					m_iDir;				// the direction the vehicle is pointed (0..FULL_ROT)
		int					m_iXadd;			// how much to add to m_ptLoc.x each step
		int					m_iYadd;			// how much to add to m_ptLoc.y each step
		int					m_iDadd;			// how much to add to m_iDir each step
		int					m_iMode;			// m_cMode (RouteMode)
		int					m_iOwn;				// m_cOwn
		int					m_iStepsLeft;
		int					m_iSpeed;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::veh_loc:		// vehicle location change
			pLocMsg = (CMsgVehLoc *)pMsg;
			m_uFlags = pLocMsg->m_iMode;
			m_iPriority = 70;
			m_dwID = pLocMsg->m_dwID;
			m_iX = (pLocMsg->m_ptHead.x / 2);
			m_iY = (pLocMsg->m_ptHead.y / 2);
			m_ieX = pLocMsg->m_hexNext.X();
			m_ieY = pLocMsg->m_hexNext.X();
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = pLocMsg->m_iPlyrNum;
			m_dwID2 = (DWORD)0;
			break;
/*
// the vehicle status (% damaged, out of materials)
class CMsgVehStat : public CNetCmd
{
public:
		CMsgVehStat (CVehicle const *pVeh);
		enum { damaged = 0x01,
					 out_mat = 0x02 };

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the vehicle
		int					m_iVehType;		// CDataTransport
		int					m_iFlags;
		int					m_iDamage;		// % damaged

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::veh_stat:		// vehicle status change
			pVStatMsg = (CMsgVehStat *)pMsg;
			m_uFlags = pVStatMsg->m_iFlags;
			m_iPriority = 70;
			m_dwID = pVStatMsg->m_dwID;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0; 
			m_idata1 = pVStatMsg->m_iVehType;
			m_idata2 = pVStatMsg->m_iDamage;
			m_idata3 = pVStatMsg->m_iPlyrNum;
			m_dwID2 = (DWORD)0;
			break;

/*
class _CMsgVeh : public CNetCmd
{
public:
		_CMsgVeh () : CNetCmd (-1) {}
		_CMsgVeh (int iMsg) : CNetCmd (iMsg) {}

		DWORD				m_dwIDBldg;		// bldg that created it
		DWORD				m_dwID;				// if != 0 its the ID of the unit
		int					m_iPlyrNum;		// player requesting the placement
		int					m_iType;			// CStructureData []
		CSubHex			m_ptHead;			// head of initial loc
		CSubHex			m_ptTail;			// tail of initial loc
		CHexCoord		m_hexDest;		// where to send it to
		int					m_iRouteMode;	// what to set route mode to

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// a vehicle is created (100% built)
class CMsgVehNew : public _CMsgVeh
{
public:
		CMsgVehNew () : _CMsgVeh (veh_new) {}
		CMsgVehNew (CBuilding * pBldg, CHexCoord const & hexDest, int iPlyrNum, int iType);

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::veh_new:		// create a vehicle
			pVNewMsg = (CMsgVehNew *)pMsg;
			m_uFlags = 0;
			m_iPriority = 74;
			m_dwID = pVNewMsg->m_dwID;
			m_iX = pVNewMsg->m_ptHead.x / 2;
			m_iY = pVNewMsg->m_ptHead.y / 2;
			m_ieX = pVNewMsg->m_hexDest.X();
			m_ieY = pVNewMsg->m_hexDest.Y();
			m_idata1 = pVNewMsg->m_iType;
			m_idata2 = pVNewMsg->m_iRouteMode;
			m_idata3 = pVNewMsg->m_iPlyrNum;
			m_dwID2 = pVNewMsg->m_dwIDBldg;
			break;

/*
// vehicle repaired
class CMsgRepaired : public CNetCmd
{
public:
		CMsgRepaired () : CNetCmd (unit_repaired) {}
		CMsgRepaired (CVehicle const * pVeh);

	DWORD			m_dwIDVeh;
};

*/

		case CNetCmd::unit_repaired:		// vehicle repaired
			pRepairedMsg = (CMsgRepaired *)pMsg;
			m_uFlags = 0;
			m_iPriority = 50;
			m_dwID = pRepairedMsg->m_dwIDVeh;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = 0;
			m_dwID2 = 0;
			break;

/*
class CMsgUnitRepair : public CNetCmd
{
public:
		CMsgUnitRepair () : CNetCmd (unit_repair) {}
		CMsgUnitRepair (CBuilding const * pBldg, int iRepair);

		void		ToSetRepair () { m_iMsg = unit_set_repair; }

		DWORD				m_dwID;						// ID of unit being repaired
		int					m_iPlyr;					// player 
		int					m_iRepair;				// amount repaired
		int					m_iDamageLevel;		// NOT USED - new damage level of unit

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/

		case CNetCmd::unit_repair:		// vehicle repaired
		case CNetCmd::unit_set_repair:
			pUnitRepairMsg = (CMsgUnitRepair *)pMsg;
			m_uFlags = 0;
			m_iPriority = 50;
			m_dwID = pUnitRepairMsg->m_dwID;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pUnitRepairMsg->m_iRepair;
			m_idata2 = 0;
			m_idata3 = pUnitRepairMsg->m_iPlyr;
			m_dwID2 = 0;
			break;


/*
class _CMsgDamage : public CNetCmd
{
public:
		_CMsgDamage () : CNetCmd (-1) {}
		_CMsgDamage (int iMsg) : CNetCmd (iMsg) {}

		DWORD				m_dwIDTarget;				// ID of unit being shot at
		int					m_iPlyrTarget;			// player being shot at
		DWORD				m_dwIDDamage;				// ID of unit being hit
		int					m_iPlyrDamage;			// player taking the damage
		DWORD				m_dwIDShoot;				// ID of unit shooting
		int					m_iPlyrShoot;				// player doing the shooting
		int					m_iDamageShot;			// amount of damage inflicted
		int					m_iDamageLevel;			// new damage level

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CMsgUnitDamage : public _CMsgDamage
{
public:
		CMsgUnitDamage () : _CMsgDamage (unit_damage) {}
		CMsgUnitDamage (CUnit const * pTarget, CUnit const * pShoot, CUnit const * pDamage, int iDamage);

		void		ToSetDamage () { m_iMsg = unit_set_damage; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CMsgUnitSetDamage : public _CMsgDamage
{
public:
		CMsgUnitSetDamage () : _CMsgDamage (unit_set_damage) {}

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

*/
		case CNetCmd::unit_damage:
			pDmgMsg = (CMsgUnitDamage *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pDmgMsg->m_dwIDTarget;			// ID of unit being shot at
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pDmgMsg->m_iDamageShot;
			m_idata2 = pDmgMsg->m_iPlyrShoot;
			m_idata3 = pDmgMsg->m_iPlyrTarget;
			m_dwID2 = pDmgMsg->m_dwIDShoot;
			break;

		case CNetCmd::unit_set_damage:
			TRAP ();
#ifdef BUGBUG
			pSetDmgMsg = (CMsgUnitSetDamage *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pSetDmgMsg->m_dwIDDamage;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0; // pSetDmgMsg->m_iDamageLevel;
			m_idata2 = pSetDmgMsg->m_iPlyrShoot;
			m_idata3 = pSetDmgMsg->m_iPlyrTarget;
			m_dwID2 = pSetDmgMsg->m_dwIDShoot;
#endif
			break;

/*
// sent when can't shoot at unit anymore
class CMsgOutOfLos : public CNetCmd
{
public:
		CMsgOutOfLos (CUnit const * pAttacker, CUnit const * pTarget);

		DWORD			m_dwIDAttacker;
		DWORD			m_iPlyrAttacker;
		DWORD			m_dwIDTarget;
		DWORD			m_iPlyrTarget;
};
*/

		case CNetCmd::out_of_LOS:
			pOutLOSMsg = (CMsgOutOfLos *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pOutLOSMsg->m_dwIDAttacker;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0;
			m_idata2 = pOutLOSMsg->m_iPlyrTarget;
			m_idata3 = pOutLOSMsg->m_iPlyrAttacker;
			m_dwID2 = pOutLOSMsg->m_dwIDTarget;
			break;

/*
// sent by AI to attack a vehicle
class CMsgAttack : public CNetCmd
{
public:
		CMsgAttack (DWORD dwShooter, DWORD dwTarget);

		DWORD				m_dwShooter;		// unit shooting
		DWORD				m_dwTarget;			// unit being shot at

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::attack:
			pAttackMsg = (CMsgAttack *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pAttackMsg->m_dwShooter;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = 0;
			m_dwID2 = pAttackMsg->m_dwTarget;
			break;


/*
// attack a unit
class CMsgUnitAttacked : public CNetCmd
{
public:
		CMsgUnitAttacked () : CNetCmd (unit_attacked) {}
		CMsgUnitAttacked (CUnit const * pMe, CUnit const * pTarget);

	DWORD			m_dwIDme;				// unit that saw it
	int				m_iPlyrNumMe;
	CHexCoord	m_hexMe;
	DWORD			m_dwIDtarget;		// unit attacked
	int				m_iPlyrNumTarget;
	CHexCoord	m_hexTarget;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

*/

		case CNetCmd::unit_attacked:
			pAttackedMsg = (CMsgUnitAttacked *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pAttackedMsg->m_dwIDtarget;	// receiving player's unit
			m_iX = pAttackedMsg->m_hexTarget.X();
			m_iY = pAttackedMsg->m_hexTarget.Y();
			m_ieX = pAttackedMsg->m_hexMe.X();
			m_ieY = pAttackedMsg->m_hexMe.Y();
			m_idata1 = 0;
			m_idata2 = pAttackedMsg->m_iPlyrNumMe;
			m_idata3 = pAttackedMsg->m_iPlyrNumTarget; // receiving player id
			m_dwID2 = pAttackedMsg->m_dwIDme;
			break;


/*
class CMsgBldgStat : public CNetCmd
{
public:
		CMsgBldgStat (CBuilding const *pBldg);
		enum { built = 0x01,
					 damaged = 0x02,
					 paused = 0x04,
					 resumed = 0x08,
					 out_mat = 0x10 };

		DWORD				m_dwID;				// ID of building
		int					m_iPlyrNum;		// player owning the building
		int					m_iBuilt;			// % built
		int					m_iType;	// CDataStructure
		int					m_iFlags;
		int					m_iDamage;		// % damaged

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::bldg_stat:		// building status change
			pBStatMsg = (CMsgBldgStat *)pMsg;
			m_uFlags = pBStatMsg->m_iFlags;
			m_iPriority = 70;
			m_dwID = pBStatMsg->m_dwID;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pBStatMsg->m_iType;
			m_idata2 = pBStatMsg->m_iBuilt;
			m_idata3 = pBStatMsg->m_iPlyrNum;
			m_dwID2 = (DWORD)0;
			break;
/*
class _CMsgBldg : public CNetCmd
{
public:
		_CMsgBldg () : CNetCmd (-1) {}
		_CMsgBldg (int iMsg) : CNetCmd (iMsg) {}

		int					m_iPlyrNum;		// player requesting the placement
		DWORD				m_dwIDBldg;		// ID of building
		DWORD				m_dwIDVeh;		// ID of vehicle building (NULL if start place)
		CHexCoord		m_hexBldg;		// location to build at
		int					m_iDir;				// direction of building
		int					m_iType;			// building type to build

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// place a building (start of game)
class CMsgPlaceBldg : public _CMsgBldg
{
public:
		CMsgPlaceBldg () : _CMsgBldg (place_bldg) {}
		CMsgPlaceBldg (CHexCoord const & hex, int iDir, int iBldg);

		CMsgBldgNew * ToNew () { m_iMsg = bldg_new; return ((CMsgBldgNew *) this); }
		void					ToErr () { m_iMsg = err_place_bldg; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::err_place_bldg:	// could not place building
		case CNetCmd::place_bldg:		// place a building
			pPlaceBldg = (CMsgPlaceBldg *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pPlaceBldg->m_dwIDBldg;
			m_iX = pPlaceBldg->m_hexBldg.X();
			m_iY = pPlaceBldg->m_hexBldg.Y();
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pPlaceBldg->m_iType;
			m_idata2 = pPlaceBldg->m_iDir;
			m_idata3 = pPlaceBldg->m_iPlyrNum;
			m_dwID2 = pPlaceBldg->m_dwIDVeh;
			break;


/*
class _CMsgBldg : public CNetCmd
{
public:
		_CMsgBldg () : CNetCmd (-1) {}
		_CMsgBldg (int iMsg) : CNetCmd (iMsg) {}

		int					m_iPlyrNum;		// player requesting the placement
		DWORD				m_dwIDBldg;		// ID of building
		DWORD				m_dwIDVeh;		// ID of vehicle building (NULL if start place)
		CHexCoord		m_hexBldg;		// location to build at
		int					m_iDir;				// direction of building
		int					m_iType;			// building type to build

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// a building is created (0% built)
class CMsgBldgNew : public _CMsgBldg
{
public:
		CMsgBldgNew () : _CMsgBldg (bldg_new) {}

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::bldg_new:		// create a building
			pBNewMsg = (CMsgBldgNew *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pBNewMsg->m_dwIDBldg;
			m_iX = pBNewMsg->m_hexBldg.X();
			m_iY = pBNewMsg->m_hexBldg.Y();
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pBNewMsg->m_iType;
			m_idata2 = pBNewMsg->m_iDir;
			m_idata3 = pBNewMsg->m_iPlyrNum;
			m_dwID2 = pBNewMsg->m_dwIDVeh;
			break;

/*
// tell a vehicle to build a road - it must ALREADY be on it
class _CMsgRoad : public CNetCmd
{
public:
		_CMsgRoad () : CNetCmd (-1) {}
		_CMsgRoad (int iMsg) : CNetCmd (iMsg) {}

		int					m_iPlyrNum;		// player requesting the placement
		DWORD				m_dwID;				// ID of vehicle
		CHexCoord		m_hexBuild;		// location to build at
		int					m_iMode;
						enum { one_hex, street };

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// a road in a single hex is completed (100% built)
// road hex finished
class CMsgRoadDone : public _CMsgRoad
{
public:
		CMsgRoadDone () : _CMsgRoad (road_done) {}
		CMsgRoadDone (CVehicle const *pVeh);

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CMsgRoadNew : public _CMsgRoad
{
public:
		CMsgRoadNew () : _CMsgRoad (road_new) {}
		CMsgRoadNew (CVehicle const *pVeh);

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::road_done:		// road completed
			pRoadMsg = (CMsgRoadDone *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pRoadMsg->m_dwID;
			m_iX = pRoadMsg->m_hexBuild.X();
			m_iY = pRoadMsg->m_hexBuild.Y();
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0;
			m_idata2 = pRoadMsg->m_iMode;
			m_idata3 = pRoadMsg->m_iPlyrNum;
			m_dwID2 = 0;
			break;

/*			
// tell a vehicle to build a road - it must ALREADY be on it
class CMsgBuildRoad : public _CMsgRoad
{
public:
		CMsgBuildRoad () : _CMsgRoad (build_road) {}
		CMsgBuildRoad (CVehicle const *pVeh);

		void	ToErr () { m_iMsg = err_build_road; }
		CMsgRoadNew *	ToNew () { m_iMsg = road_new; return ((CMsgRoadNew *) this); }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::err_build_road: // could not build here
		case CNetCmd::road_new:		// road completed
			pRoadNewMsg = (CMsgRoadNew *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pRoadNewMsg->m_dwID;
			m_iX = pRoadNewMsg->m_hexBuild.X();
			m_iY = pRoadNewMsg->m_hexBuild.Y();
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0;
			m_idata2 = pRoadNewMsg->m_iMode;
			m_idata3 = pRoadNewMsg->m_iPlyrNum;
			m_dwID2 = 0;
			break;


/*
class CMsgPlyrDying : public CNetCmd
{
public:
		CMsgPlyrDying () : CNetCmd (plyr_dying) {}
		CMsgPlyrDying (CPlayer const * pPlyr);

		int		m_iPlyrNum;
};
*/
		case CNetCmd::plyr_dying:		// we're killing a player
			pDyingMsg = (CMsgPlyrDying *)pMsg;
			m_uFlags = 0;
			m_iPriority = 100;
			m_dwID = 0;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = 0;
			m_idata2 = 0;
			m_idata3 = pDyingMsg->m_iPlyrNum;
			m_dwID2 = 0;
			break;

/*
// sent when a unit needs to be deleted
class CMsgDeleteUnit : public CNetCmd
{
public:
		CMsgDeleteUnit (CUnit const *pUnit, DWORD dwIDKiller);

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the unit
		int					m_iPlyrKiller;// player that killed it

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

*/
		case CNetCmd::delete_unit:		// sent when a unit needs to be deleted
			pDelUnit = (CMsgDeleteUnit *)pMsg;
			m_uFlags = 0;
			m_iPriority = 90;
			m_dwID = pDelUnit->m_dwID;
			m_iX = 0;
			m_iY = 0;
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pDelUnit->m_iPlyrKiller;
			m_idata2 = 0;
			m_idata3 = pDelUnit->m_iPlyrNum;
			m_dwID2 = 0;
			break;

/*
// see a unit that previously was unseen
class CMsgSeeUnit : public CNetCmd
{
public:
		CMsgSeeUnit () : CNetCmd (see_unit) {}
		CMsgSeeUnit (CUnit const * pMe, CUnit const * pSpot);

	DWORD			m_dwIDme;				// unit that saw it
	int				m_iPlyrNumMe;
	CHexCoord	m_hexMe;
	DWORD			m_dwIDspot;			// unit seen
	int				m_iPlyrNumSpot;
	CHexCoord	m_hexSpot;
};
*/

		case CNetCmd::see_unit:		// see a unit that previously was unseen
			pSeeUnit = (CMsgSeeUnit *)pMsg;
			m_uFlags = 0;
			m_iPriority = 80;
			m_dwID = pSeeUnit->m_dwIDme;
			m_iX = pSeeUnit->m_hexMe.X();
			m_iY = pSeeUnit->m_hexMe.Y();
			m_ieX = pSeeUnit->m_hexSpot.X();
			m_ieY = pSeeUnit->m_hexSpot.Y();
			m_idata1 = 0;
			m_idata2 = pSeeUnit->m_iPlyrNumSpot;
			m_idata3 = pSeeUnit->m_iPlyrNumMe;
			m_dwID2 = pSeeUnit->m_dwIDspot;
			break;


/*
class _CMsgVehGo : public CNetCmd
{
public:
		_CMsgVehGo () : CNetCmd (-1) {}
		_CMsgVehGo (int iMsg) : CNetCmd (iMsg) {}

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the vehicle
		CSubHex			m_ptDest;			// where we are going
		CSubHex			m_ptNext;			// the next sub-hex the vehicle will go to
		CSubHex			m_ptHead;			// the sub-hex the vehicle's head is in
		CSubHex			m_ptTail;			// the sub-hex the vehicle's tail is in
		CHexCoord		m_hexNext;		// the next hex the vehicle is going to in its path - m_ptNext may NOT be in it
		CHexCoord		m_hexDest;		// the final destination of the vehicle
		int					m_iDir;				// the direction the vehicle is pointed (0..FULL_ROT)
		int					m_iXadd;			// how much to add to m_ptLoc.x each step
		int					m_iYadd;			// how much to add to m_ptLoc.y each step
		int					m_iDadd;			// how much to add to m_iDir each step
		int					m_iMode;			// m_cMode (RouteMode)
		int					m_iOwn;				// m_cOwn
		int					m_iStepsLeft;
		int					m_iSpeed;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// tell a vehicle to go to a destination
class CMsgVehGoto : public _CMsgVehGo
{
public:
		CMsgVehGoto () : _CMsgVehGo (veh_goto) {}
		CMsgVehGoto (CVehicle const *pVeh);

		CMsgVehLoc *	ToLoc ( ) { m_iMsg = veh_loc; return ((CMsgVehLoc *) this); }
		void					ToErr () { m_iMsg = err_veh_goto; }
		void					ToTraffic () { m_iMsg = err_veh_traffic; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/

		case CNetCmd::err_veh_traffic:		// a movement error occurred
		case CNetCmd::err_veh_goto:		// a movement error occurred
			pVehGoto = (CMsgVehGoto *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;					// highest priority
			m_dwID = pVehGoto->m_dwID;			// who it was
			m_iX = pVehGoto->m_ptHead.x / 2;	// current location
			m_iY = pVehGoto->m_ptHead.y / 2;
			m_ieX = pVehGoto->m_hexDest.X();	// destination
			m_ieY = pVehGoto->m_hexDest.Y();
			m_idata1 = pVehGoto->m_hexNext.X();	// next hex to enter
			m_idata2 = pVehGoto->m_hexNext.Y();
			m_idata3 = pVehGoto->m_iPlyrNum;	// player that owns it
			m_dwID2 = 0;	// pVehGoto->m_dwIDBlocker; // unit in m_hexNext
			break;
/*
// tell a vehicle to build a building - it must ALREADY be adjacent
class _CMsgBldg : public CNetCmd
{
public:
		_CMsgBldg () : CNetCmd (-1) { ctor (); }
		_CMsgBldg (int iMsg) : CNetCmd (iMsg) { ctor (); }
		void ctor ();

		int					m_iPlyrNum;		// player requesting the placement
		DWORD				m_dwIDBldg;		// ID of building
		DWORD				m_dwIDVeh;		// ID of vehicle building (NULL if start place)
		CHexCoord		m_hexBldg;		// location to build at
		int					m_iDir;				// direction of building
		int					m_iType;			// building type to build
		int					m_iWhy;				// why a building can't be built there

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
// tell a vehicle to build a building - it must ALREADY be adjacent
class CMsgBuildBldg : public _CMsgBldg
{
public:
		CMsgBuildBldg () : _CMsgBldg (build_bldg) {}
		CMsgBuildBldg (CVehicle const *pVeh, CHexCoord const & hex, int iDir, int iBldg);

		CMsgBldgNew * ToNew () { m_iMsg = bldg_new; return ((CMsgBldgNew *) this); }
		void					ToErr () { m_iMsg = err_build_bldg; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};
*/
		case CNetCmd::err_build_bldg:
			pBldBldg = (CMsgBuildBldg *)pMsg;
			m_uFlags = 0;
			m_iPriority = 70;
			m_dwID = pBldBldg->m_dwIDBldg;
			m_iX = pBldBldg->m_hexBldg.X();
			m_iY = pBldBldg->m_hexBldg.Y();
			m_ieX = 0;
			m_ieY = 0;
			m_idata1 = pBldBldg->m_iType;
			m_idata2 = pBldBldg->m_iWhy;
			m_idata3 = pBldBldg->m_iPlyrNum;
			m_dwID2 = pBldBldg->m_dwIDVeh;
			break;
		default:
			break;
	}
}

void CAIMsg::Serialize( CArchive& archive )
{
    ASSERT_VALID( this );

    CObject::Serialize( archive );

    if( archive.IsStoring() )
    {
    	archive << m_dwID;
    }
    else
    {
		archive >> m_dwID;
	}
}

CAIMsg::~CAIMsg()
{
}

// end of CAIMsg.cpp

