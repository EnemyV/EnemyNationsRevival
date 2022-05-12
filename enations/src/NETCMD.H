//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __NETCMD_H__
#define __NETCMD_H__

#include "racedata.h"
#include "unit.h"

class CDlgCreate;
class CNetJoinGame;
class CNetJoinName;
class CNetPlayer;
class CNetPublish;
class CNetUnpublish;
class CVehicle;

class CMsgBldgNew;
class CMsgCompUnitDamageElem;
class CMsgVehNew;
class CMsgRoadNew;
class CMsgBridgeNew;
class CMsgVehCompLocElem;
class CPlayer;


#ifdef _DEBUG
#define		ASSERT_CMD(p)		(p->AssertMsgValid ())
#else
#define		ASSERT_CMD(p)		((void)0)
#endif

#pragma pack ( push, cnetcmd, 1 )

class CNetCmd : public VPMsgHdr
{
public:
enum {	cmd_ready,							// we're ready to start
				cmd_you_are,						// passes player number, get ready to start game
				cmd_player,							// each opponent
				cmd_start,							// start the game (build world data)
				cmd_plyr_status,				// tell other players this players status
				cmd_init_done,					// this player is ready to go
				cmd_chat,								// chat before the game starts
				cmd_play,								// all players read - start playing
				cmd_to_ai,							// human player becomes AI controlled
				cmd_to_hp,							// AI player becomes human controlled
				cmd_pause,							// pause the game
				cmd_resume,							// resume the game

				cmd_enum_plyrs,					// to server asking for players in game
				cmd_plyr_join,					// players avail to join (load game)
				cmd_select_plyr,				// select a player to play
				cmd_select_ok,					// selection was accepted
				cmd_select_not_ok,
				cmd_game_data,					// data for loading a game
				cmd_plyr_taken,					// loaded player taken

															// sent to server
				place_bldg,							// initial placement of a building
				place_veh,							// initial placement of a vehicle
				build_bldg,							// build a building
				build_veh,							// build a vehicle
				build_road,							// build a road on a hex
				build_bridge,						// build a bridge
				veh_goto,								// tell a vehicle to go somewhere
				trans_mat,							// transfer materials between units
				unit_control,						// stop/resume/destroy a unit
				unit_damage,						// notify of damage level
				unit_repair,						// repair unit
				destroy_unit,						// destroy a unit
				stop_destroy_unit,			// stop destroying a unit
				load_carrier,						// load units on a carrier
				unload_carrier,					// unload all units from a carrier

															// sent from server
				bldg_new,								// a new building is created
				bldg_stat,							// built/damage status of a bldg
				veh_new,								// a new vehicle is created
				veh_stat,								// damage status of a vehicle
				veh_loc,								// vehicle going to the next hex
				veh_dest,								// vehicle has arrived at its destination
				road_new,								// start a new road hex
				road_done,							// a new road hex is completed
				bridge_new,							// start a new bridge
				bridge_done,						// a new bridge is completed
				unit_set_damage,				// received damage level
				unit_set_repair,				// received damage level (repaired)
				unit_destroying,				// a unit is being destroyed
				stop_unit_destroying,		// a unit is stopped being destroyed
				delete_unit,						// a unit is gone (sent by owner)
				deploy_it,							// deploy vehicle
				plyr_dying,							// player is dying - kill all units
				scenario,								// scenario message to AI - scenario starting
				scenario_atk,						// scenario message to - atack specific HP units
				unit_loaded,						// unit loaded itself
				unit_repaired,					// unit repaired
				unit_attacked,					// unit was attacked
				build_civ,							// sent to AI to build a civ building

															// sent by players to themselves
				plyr_dead,							// player is dead - delete him

															// messages from AI to server
				veh_set_dest,						// set dest
				attack,									// attack a unit
				set_rsrch,							// set R&D to research
				see_unit,								// see a unit
				repair_veh,							// repair a vehicle
				repair_bldg,						// repair/help build a bldg

															// errors sent from server
				err_place_bldg,
				err_build_bldg,
				err_build_veh,
				err_build_road,
				err_build_bridge,
				err_veh_goto,
				err_veh_traffic,

				ipc_msg,								// generic chat message
				ai_msg,									// msg from non-server to AI player

				// adding new ones down here so eric doesn't have to change constants
				out_of_LOS,							// your target can't be hit anymore
				set_relations,					// set relations
				shoot_gun,							// shooting at something
				game_speed,							// client set's speed
				bldg_materials,					// materials a building has
				cmd_get_file,						// get the game file (load net game)
				cmd_new_server,					// new system becoming server
				give_unit,							// give unit to new owner
				set_time,								// tell clients game time
				start_file,							// start sending file (load multi)
				start_loaded_game,			// start a loaded game
				cancel_load,						// join cancels a loaded game

				veh_comp_loc,						// group of vehicles going to the next hex
				comp_unit_damage,				// group of unit_damage messages
				comp_unit_set_damage,		// group of unit_set_damage messages
				pause_messages,					// stop sending status messages
				ai_gpf_takeover,				// restart the AI cause it GPF'ed

				need_save_info,					// tell all players to send save game info
				save_info,							// base save info
				research_disc,					// player has discovered research

				last_message						// used for ASSERT
				};

		CNetCmd () { m_bMsg = (BYTE) -1; }
		CNetCmd (int iCmd) { m_bMsg = (BYTE) iCmd; }

		int			GetType () const { return (m_bMsg); }

static		CNetUnpublish * AllocUnpublish (CString const & sName);
static		CNetJoinGame * 	AllocJoinGame (CString const & sName);
static		CNetJoinName * 	AllocJoinName (CString const & sName);
static		CNetPlayer * 		AllocPlayer (CPlayer *pPlr);

public:
		BYTE			m_bMsg;
		BYTE			m_bMemPool;				// 1 for fixed size pool

#ifdef _LOG_LAG
		DWORD			dwPostTime;
#endif

#ifdef _DEBUG
public:
	void AssertMsgValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// creating/joining/leaving game messages

class CNetReady : public CNetCmd {		// ready to start, passes init info
public:
		CNetReady (CInitData * pId);
		int				m_iPlyrNum;
		CInitData	m_InitData;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetPlyrJoin : public CNetCmd {
public:
		CNetPlyrJoin () : CNetCmd (cmd_plyr_join) {}

static CNetPlyrJoin * Alloc (const CNetPlyrJoin * pData);
static CNetPlyrJoin * Alloc (const CPlayer * pPlyr);

		int		m_iLen;			// len of alloc
		int		m_iPlyrNum;
		int		m_iPwrHave;
		int		m_iPwrNeed;
		int		m_iPplHave;
		int		m_iPplNeed;
		int		m_iMat [CMaterialTypes::num_types];
		int		m_iRsrchLevel;
		int		m_iNumBldgs;
		int		m_iNumVeh;
		BOOL	m_bAvail;
		char	m_sName[1];
		// note - we actually have the full name

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetEnumPlyrs : public CNetCmd {		// send player number, get ready to create
public:
		CNetEnumPlyrs (int iNetNum) : CNetCmd (cmd_enum_plyrs) { m_iNetNum = iNetNum; }
		int				m_iNetNum;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetSelectPlyr : public CNetCmd {		// send player number, get ready to create
public:
		CNetSelectPlyr (int iNetNum, int iPlyrNum, char const * pName ) : CNetCmd (cmd_select_plyr)
											{ m_iNetNum = iNetNum; m_iPlyrNum = iPlyrNum; strncpy (m_sName, pName, 80); m_sName[79] = 0; }
		int				m_iNetNum;
		int				m_iPlyrNum;
		char			m_sName [80];

		void	ToOk () { m_bMsg = cmd_select_ok; }
		void	ToNotOk () { m_bMsg = cmd_select_not_ok; }
		void	ToTaken () { m_bMsg = cmd_plyr_taken; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetYouAre : public CNetCmd {		// send player number, get ready to create
public:
		CNetYouAre (int iPlyr);
		int				m_iPlyrNum;
		int				m_iServerNum;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetPlayer : public CNetCmd {		// init info on an opponent
public:
		int				m_iLen;								// how long this message is
		int				m_iPlyrNum;
		int				m_iNetNum;
		CInitData	m_InitData;
		BYTE			m_bAI;
		BYTE			m_bLocal;
		BYTE			m_bServer;
		char			m_sName[1];
		// note - we actually have the full name

		int		GetLen () const { return (m_iLen); }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetStart : public CNetCmd {		// start a game
public:
		CNetStart (unsigned uRand, int iSide, int iSideSize, 
					int iAiDiff, int iNumAi, int iNumHp, int iStart);
		unsigned	m_uRand;
		int				m_iSide;
		int				m_iSideSize;
		// AI properties below
		int				m_iAi;
		int				m_iNumAi;
		int				m_iNumHp;
		int				m_iStart;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CNetPlyrStatus : public CNetCmd {
public:
		CNetPlyrStatus (int iNum, int iStatus) : CNetCmd (cmd_plyr_status) 
											{ m_iNetNum = iNum; m_iStatus = iStatus; }
	
		int			m_iNetNum;
		int			m_iStatus;
};

class CNetInitDone : public CNetCmd {
public:
		CNetInitDone (CPlayer const *pPlyr);
		int				m_iPlyrNum;
};

class CNetChat : public CNetCmd {
public:
		static CNetChat * Alloc (const CPlayer * pPlyr, const char * psMsg);

		int				m_iLen;		// length of message
		int				m_iPlyrNetNum;
		char			m_sMsg[1];
		// note - we actually have the full message
};

class CNetPlay : public CNetCmd {		// ready
public:
		CNetPlay (unsigned uRand) : CNetCmd (cmd_play) { m_uRand = uRand; }
		unsigned	m_uRand;
};

class CNetToAi : public CNetCmd {
public:
		CNetToAi (CPlayer * pPlyr);

		int		m_iPlyrNum;
};

class CNetToHp : public CNetCmd {
public:
		static	CNetToHp * Alloc (CPlayer * pPlyr);

		int		m_iPlyrNum;
		int		m_iNetNum;
		char	m_sName[1];		// actually all of it
};

class CNetGetFile : public CNetCmd {
public:
		CNetGetFile ( CPlayer * pPlyr, CPlayer * pPlyrServer, int iBufLen );

		int		m_iPlyrNum;
		int		m_iServerNum;
		int		m_iServerNetNum;
		int		m_iBufLen;
};


/////////////////////////////////////////////////////////////////////////////
// the following classes are base classes for common classes further down

class _CMsgVeh : public CNetCmd
{
public:
		_CMsgVeh () : CNetCmd (-1) { ctor (); }
		_CMsgVeh (int iMsg) : CNetCmd (iMsg) { ctor (); }

		DWORD				m_dwIDBldg;		// bldg that created it
		DWORD				m_dwID;				// if != 0 its the ID of the unit
		int					m_iPlyrNum;		// player requesting the placement
		int					m_iType;			// CStructureData []
		CSubHex			m_ptHead;			// head of initial loc
		CSubHex			m_ptTail;			// tail of initial loc
		CHexCoord		m_hexDest;		// where to send it to
		int					m_iRouteMode;	// what to set route mode to
		int					m_iDelay;			// delay in dropping it out

protected :
		void		ctor ();

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

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
		signed char	m_iDir;				// direction of building
		signed char	m_iType;			// building type to build
		signed char	m_iWhy;				// why a building can't be built there
		BYTE				m_bShow;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

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

class _CMsgVehGo : public CNetCmd
{
public:
		_CMsgVehGo () : CNetCmd (-1) {}
		_CMsgVehGo (int iMsg) : CNetCmd (iMsg) {}
		const _CMsgVehGo operator= (_CMsgVehGo const & src);

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the vehicle
		CSubHex			m_ptDest;			// where we are going
		CSubHex			m_ptNext;			// the next sub-hex the vehicle will go to
		CSubHex			m_ptHead;			// the sub-hex the vehicle's head is in
		CSubHex			m_ptTail;			// the sub-hex the vehicle's tail is in
		CHexCoord		m_hexNext;		// the next hex the vehicle is going to in its path - m_ptNext may NOT be in it
		CHexCoord		m_hexDest;		// the final destination of the vehicle
		int					m_iDir;				// the direction the vehicle is pointed (0..FULL_ROT)
		int					m_iTurretDir;	// the direction the turret is pointed (0..FULL_ROT)
		int					m_iXadd;			// how much to add to m_ptLoc.x each step
		int					m_iYadd;			// how much to add to m_ptLoc.y each step
		int					m_iDadd;			// how much to add to m_iDir each step
		int					m_iTadd;			// how much to add to m_iTurret each step
		int					m_iMode;			// m_cMode (RouteMode)
		int					m_iOwn;				// m_cOwn
		int					m_iStepsLeft;
		int					m_iSpeed;
		DWORD				m_dwBlocker;	// unit blocking veh
		BOOL				m_bOnWater;		// true if on water

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CMsgUnitDamage : public CNetCmd
{
public:
		CMsgUnitDamage () : CNetCmd (unit_damage) {}
		CMsgUnitDamage (CUnit const * pTarget, CUnit const * pShoot, CUnit const * pDamage, int iDamage);
		CMsgUnitDamage ( CMsgCompUnitDamageElem * pElem );

		DWORD				m_dwIDTarget;				// ID of unit being shot at
		int					m_iPlyrTarget;			// player being shot at
		DWORD				m_dwIDDamage;				// ID of unit being hit
		int					m_iPlyrDamage;			// player taking the damage
		DWORD				m_dwIDShoot;				// ID of unit shooting
		int					m_iPlyrShoot;				// player doing the shooting
		int					m_iDamageShot;			// amount of damage inflicted

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CMsgUnitSetDamage : public CNetCmd
{
public:
		CMsgUnitSetDamage () : CNetCmd (unit_set_damage) {}
		CMsgUnitSetDamage (DWORD dwDam, DWORD dwShoot, int iDam) : CNetCmd (unit_set_damage) 
														{ m_dwIDDamage = dwDam; 
															m_dwIDShoot = dwShoot;
															m_iDamageLevel = iDam; }
		CMsgUnitSetDamage ( CMsgCompUnitDamageElem * pElem );

		DWORD				m_dwIDDamage;				// ID of unit being hit
		DWORD				m_dwIDShoot;				// ID of unit shooting
		int					m_iDamageLevel;			// new damage level of unit

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// the following messages are sent to the server by a player

// place a building (start of game)
class CMsgPlaceBldg : public _CMsgBldg
{
public:
		CMsgPlaceBldg () : _CMsgBldg (place_bldg) {}
		CMsgPlaceBldg (CHexCoord const & hex, int iDir, int iBldg);

		CMsgBldgNew * ToNew () { m_bMsg = bldg_new; return ((CMsgBldgNew *) this); }
		void					ToErr () { m_bMsg = err_place_bldg; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// place a vehicle (start of game)
class CMsgPlaceVeh : public _CMsgVeh
{
public:
		CMsgPlaceVeh () : _CMsgVeh (place_veh) {}
//		CMsgPlaceVeh (CSubHex const & subHead, CSubHex const & subTail, CHexCoord const & hexDest, int iPlyr, int iType);
		CMsgPlaceVeh (CBuilding *pBldg, CHexCoord const & hexDest, int iPlyr, int iType);
		CMsgPlaceVeh (CHexCoord const & hexSrc, CHexCoord const & hexDest, int iPlyr, int iType);

		CMsgVehNew *	ToNew () { m_bMsg = veh_new; return ((CMsgVehNew *) this); }

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

		CMsgBldgNew * ToNew () { m_bMsg = bldg_new; return ((CMsgBldgNew *) this); }
		void					ToErr () { m_bMsg = err_build_bldg; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// tell a building to build a vehicle
class CMsgBuildVeh : public CNetCmd
{
public:
		CMsgBuildVeh () : CNetCmd (build_veh) {}
		CMsgBuildVeh (CBuilding const *pBldg, int iVehType, int iNum = 1);

		DWORD				m_dwID;				// ID of factory
		int					m_iVehType;		// type of vehicle to build
		int					m_iNum;				// how many

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// tell a vehicle to build a road - it must ALREADY be on it
class CMsgBuildRoad : public _CMsgRoad
{
public:
		CMsgBuildRoad () : _CMsgRoad (build_road) {}
		CMsgBuildRoad (CVehicle const *pVeh);

		void	ToErr () { m_bMsg = err_build_road; }
		CMsgRoadNew *	ToNew () { m_bMsg = road_new; return ((CMsgRoadNew *) this); }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// tell a vehicle to build a bridge - it must ALREADY be on it
class CMsgBuildBridge : public _CMsgBridge
{
public:
		CMsgBuildBridge () : _CMsgBridge (build_bridge) {}
		CMsgBuildBridge (CVehicle const *pVeh, CHexCoord const & hexEnd);

		void	ToErr () { m_bMsg = err_build_bridge; }
		CMsgBridgeNew *	ToNew () { m_bMsg = bridge_new; return ((CMsgBridgeNew *) this); }

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

		CMsgVehLoc *	ToLoc ( ) { m_bMsg = veh_loc; return ((CMsgVehLoc *) this); }
		void					ToErr (CUnit const * pBlk);
		void					ToTraffic () { m_bMsg = err_veh_traffic; }

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// tell 2 units to transfer materials
class CMsgTransMat : public CNetCmd
{
public:
		CMsgTransMat () : CNetCmd (trans_mat) {}
		CMsgTransMat (CUnit const *pSrc, CUnit const *pDest);

		DWORD				m_dwIDSrc;		// ID of unit giving up materials
		DWORD				m_dwIDDest;		// ID of unit receiving materials
		int					m_aiMat [CMaterialTypes::num_types];	// materials to transfer

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// start/stop/destroy unit
class CMsgUnitControl : public CNetCmd
{
public:
		CMsgUnitControl () : CNetCmd (unit_control) {}
		enum { cancel, stop, resume, num_cmds };
		CMsgUnitControl (CUnit const *pUnit, int iCmd);

		DWORD				m_dwID;				// ID of unit
		char				m_cCmd;				// command

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

class CMsgUnitRepair : public CNetCmd
{
public:
		CMsgUnitRepair () : CNetCmd (unit_repair) {}
		CMsgUnitRepair (CUnit const * pUnit, int iRepair);

		void		ToSetRepair () { m_bMsg = unit_set_repair; }

		DWORD				m_dwID;						// ID of unit being repaired
		int					m_iPlyr;					// player 
		int					m_iRepair;				// amount repaired
		int					m_iDamageLevel;		// NOT USED - new damage level of unit

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// tell server to start self-destruct
class CMsgDestroyUnit : public CNetCmd
{
public:
		CMsgDestroyUnit () : CNetCmd (destroy_unit) {}
		CMsgDestroyUnit (CUnit const *pUnit);

		DWORD				m_dwID;				// ID of unit

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// the following messages are sent to the players by the server

// build a building
class CMsgBldgNew : public _CMsgBldg
{
public:
		CMsgBldgNew () : _CMsgBldg (bldg_new) {}

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// the building status (% built, % damaged, paused, resumed, out of materials)
class CMsgBldgStat : public CNetCmd
{
public:
		CMsgBldgStat (CBuilding const *pBldg);
		enum { built = 0x01,
					 paused = 0x02,
					 resumed = 0x04,
					 out_mat = 0x08 };

		DWORD				m_dwID;				// ID of building
		int					m_iPlyrNum;		// player owning the building
		int					m_iBuilt;			// % built
		int					m_iConstDone;
		int					m_iFoundPer;
		int					m_iSkltnPer;
		int					m_iFinalPer;
		int					m_iType;
		int					m_iFlags;

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
		CMsgVehNew (CVehicle const * pVeh, DWORD dwIDBldg = 0);

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

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

// tell everyone where the vehicle is
class CMsgVehLoc : public _CMsgVehGo
{
public:
		CMsgVehLoc () : _CMsgVehGo (veh_loc) {}
		CMsgVehLoc (CVehicle const *pVeh);
		CMsgVehLoc (CMsgVehCompLocElem const * pElem);

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

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

// starting a road hex
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

// sent by AI to set a dest for a vehicle
class CMsgVehSetDest : public CNetCmd
{
public:
		CMsgVehSetDest (CVehicle const *pVeh);
		CMsgVehSetDest (DWORD dwID, CHexCoord const & hex, int iMode);
		CMsgVehSetDest (DWORD dwID, CSubHex const & hex, int iMode);

		DWORD				m_dwID;				// ID of vehicle
		CHexCoord		m_hex;
		CSubHex			m_sub;
		int					m_iSub;
		int					m_iMode;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// sent when a unit is going to be deleted
class CMsgUnitDying : public CNetCmd
{
public:
		CMsgUnitDying (CUnit const *pUnit, DWORD dwIDKiller);

		DWORD				m_dwID;				// ID of vehicle
		int					m_iPlyrNum;		// player owning the unit
		int					m_iPlyrKiller;// player that killed it
};

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

// sent to deploy a vehicle
class CMsgDeployIt : public CNetCmd
{
public:
		CMsgDeployIt (CVehicle const * pVeh);

		DWORD			m_dwID;

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// we're killing a player
class CMsgPlyrDying : public CNetCmd
{
public:
		CMsgPlyrDying () : CNetCmd (plyr_dying) {}
		CMsgPlyrDying (CPlayer const * pPlyr);

		int		m_iPlyrNum;
};

// we're killing a player
class CMsgPlyrDead : public CNetCmd
{
public:
		CMsgPlyrDead () : CNetCmd (plyr_dead) {}
		CMsgPlyrDead (CPlayer * pPlyr);

		CPlayer * m_pPlyr;
};

// AI sets new R&D
class CMsgRsrch : public CNetCmd
{
public:
		CMsgRsrch () : CNetCmd (set_rsrch) {}
		CMsgRsrch (int iPlyr, int iTopic) : CNetCmd (set_rsrch) { m_iPlyrNum = iPlyr; m_iTopic = iTopic; }

	int		m_iPlyrNum;
	int		m_iTopic;
};

// load a carrier
class CMsgLoadCarrier : public CNetCmd
{
public:
		CMsgLoadCarrier () : CNetCmd (load_carrier) {}
		CMsgLoadCarrier (CVehicle const * pCargo, CVehicle const * pCarrier);

	DWORD		m_dwIDCargo;			// unit to carry
	DWORD		m_dwIDCarrier;		// unit carrying it
};

// unload a carrier
class CMsgUnloadCarrier : public CNetCmd
{
public:
		CMsgUnloadCarrier () : CNetCmd (unload_carrier) {}
		CMsgUnloadCarrier (CVehicle const * pVeh);

	DWORD		m_dwID;		// unit carrying it
};

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

#ifdef _DEBUG
public:
	void AssertValid() const;
#endif
};

// Vehicle must be in repair building when this is sent
class CMsgRepairVeh : public CNetCmd
{
public:
		CMsgRepairVeh () : CNetCmd (repair_veh) {}
		CMsgRepairVeh (CVehicle const * pVeh);

	DWORD			m_dwIDVeh;
};

// Vehicle must be in repair building when this is sent
class CMsgRepairBldg : public CNetCmd
{
public:
		CMsgRepairBldg () : CNetCmd (repair_bldg) {}
		CMsgRepairBldg (CVehicle const * pVeh, CBuilding const * pBldg);

	DWORD			m_dwIDVeh;
	DWORD			m_dwIDBldg;
};

// telling the AI a scenario is starting
class CMsgScenario : public CNetCmd
{
public:
		CMsgScenario ();

	int			m_iScenario;			// scenario number
	int			m_iPlyrHp;				// human player
	int			m_iPlyrAi;				// AI player
};

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

// vehicle loaded on vehicle
class CMsgLoaded : public CNetCmd
{
public:
		CMsgLoaded () : CNetCmd (unit_loaded) {}
		CMsgLoaded (CVehicle const * pVehCarrier, CVehicle const * pCargo);

	DWORD			m_dwIDCarrier;
	DWORD			m_dwIDCargo;
};

// vehicle repaired
class CMsgRepaired : public CNetCmd
{
public:
		CMsgRepaired () : CNetCmd (unit_repaired) {}
		CMsgRepaired (CVehicle const * pVeh);

	DWORD			m_dwIDVeh;
	int				m_iPlyrNum;
};

// sending message to AI player over the wire
class CMsgAiMsg : public CNetCmd
{
public:
		CMsgAiMsg () : CNetCmd (ai_msg) {}

		static CMsgAiMsg * Alloc (CPlayer const * pPlyr, CNetCmd const * pMsg, int iLen);

	int			m_iAllocLen;
	int			m_iPlyrNum;
	int			m_iLen;
};

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

class CMsgMatChange : public CNetCmd
{
public:
			CMsgMatChange (CUnit const * pUnit);

	DWORD			m_dwID;
	int				m_iPlyrNum;
};

class CMsgSetRelations : public CNetCmd
{
public:
			CMsgSetRelations (CPlayer const * pPlyrSet, CPlayer const * pPlyrGet, int iLevel);

	int			m_iPlyrNumSet;
	int			m_iPlyrNumGet;
	int			m_iLevel;
};

// update materials a building has
class CMsgBldgMat : public CNetCmd
{
public:
		CMsgBldgMat () : CNetCmd (bldg_materials) { m_bTotal = FALSE; }
		CMsgBldgMat (CUnit * pUnit, BOOL bTotal = TRUE );

		DWORD				m_dwID;		// ID of unit receiving materials
		int					m_aiMat [CMaterialTypes::num_types];	// materials it now has
		BOOL				m_bTotal;
};

// client set game speed
class CMsgGameSpeed : public CNetCmd
{
public:
		CMsgGameSpeed () : CNetCmd (game_speed) {}
		CMsgGameSpeed ( int iSpeed );

		int			m_iPlyrNum;
		int			m_iSpeed;
};

class CMsgNewServer : public CNetCmd
{
public:
		CMsgNewServer () : CNetCmd (cmd_new_server) {}
		CMsgNewServer ( CPlayer const * pPlyr );

		int			m_iPlyrNum;
};

class CMsgGiveUnit : public CNetCmd
{
public:
		CMsgGiveUnit () : CNetCmd (give_unit) {}
		CMsgGiveUnit ( CUnit const * pUnit, CPlayer const * pPlyr );

		int			m_iPlyrNum;
		int			m_dwID;
		int			m_aiStore [CMaterialTypes::num_types];
};

class CMsgSetTime : public CNetCmd
{
public:
		CMsgSetTime () : CNetCmd (set_time) {}
		CMsgSetTime ( DWORD dwSec ) : CNetCmd (set_time) { m_dwTime = dwSec; }

		DWORD		m_dwTime;
};

class CMsgStartFile : public CNetCmd
{
public:
		CMsgStartFile () : CNetCmd (start_file) {}
		CMsgStartFile ( VPPLAYERID from, VPPLAYERID to ) : CNetCmd (start_file) 
											{ m_idFrom = from; m_idTo = to; }

		VPPLAYERID		m_idFrom;
		VPPLAYERID		m_idTo;
};

class CMsgStartLoadedGame : public CNetCmd
{
public:
		CMsgStartLoadedGame () : CNetCmd (start_loaded_game) {}
};

class CMsgCancelLoad : public CNetCmd
{
public:
		CMsgCancelLoad () : CNetCmd (cancel_load) {}
		CMsgCancelLoad ( CPlayer * pPlyr );

		int			m_iNetNum;
};

class CMsgPauseMsg : public CNetCmd
{
public:
		CMsgPauseMsg () : CNetCmd (pause_messages) {}
		CMsgPauseMsg ( BOOL bPause );

		int			m_iPlyrNum;
		BYTE		m_bPause;
};

class CMsgVehCompLocElem
{
public:
		CMsgVehCompLocElem () {}
		const CMsgVehCompLocElem operator= ( CVehicle const & src );

		DWORD				m_dwID;				// ID of vehicle
		WORD				m_wDestX;
		WORD				m_wDestY;
		WORD				m_wNextX;
		WORD				m_wNextY;
		WORD				m_wHeadX;
		WORD				m_wHeadY;
		WORD				m_wTailX;
		WORD				m_wTailY;
		WORD				m_wHexNextX;
		WORD				m_wHexNextY;
		WORD				m_wHexDestX;
		WORD				m_wHexDestY;
		BYTE				m_bDir;
		BYTE				m_bTurretDir;
		signed char	m_cXadd;
		signed char	m_cYadd;
		signed char	m_cDadd;
		signed char	m_cTadd;
		BYTE				m_bMode;
		BYTE				m_bStepsLeft;
		BYTE				m_bSpeed;
		BYTE				m_bFlags;
							enum { _own = 0x01, _on_water = 0x02 };
};

const int NUM_LOC_ELEM = ( VP_MAXSENDDATA - sizeof (CNetCmd) - sizeof (int) ) / sizeof (CMsgVehCompLocElem);
class CMsgVehCompLoc : public CNetCmd
{
public:
		CMsgVehCompLoc () : CNetCmd (veh_comp_loc) { m_iNumMsgs = 0; }

		int			AddVeh ( CVehicle * pVeh );
		void		Reset () { m_iNumMsgs = 0; }
		int			SendSize () const { return ( sizeof (CNetCmd) + sizeof (int) + m_iNumMsgs * sizeof (CMsgVehCompLocElem) ); }

		int			m_iNumMsgs;
		CMsgVehCompLocElem	m_aMVCLE [ NUM_LOC_ELEM ];
};

class CMsgCompUnitDamageElem
{
public:

		DWORD		m_dwID;
		DWORD		m_dwKiller;
		WORD		m_wDamage;
};

const int NUM_UNIT_DAMAGE_ELEM = ( VP_MAXSENDDATA - sizeof (CNetCmd) - sizeof (int) ) / sizeof (CMsgCompUnitDamageElem);
class CMsgCompUnitDamage : public CNetCmd
{
public:
		CMsgCompUnitDamage () : CNetCmd (comp_unit_damage) { m_iNumMsgs = 0; }

		int			AddUnit ( CUnit * pUnit, int iDam );
		void		Reset () { m_iNumMsgs = 0; }
		int			SendSize () const { return ( sizeof (CNetCmd) + sizeof (int) + m_iNumMsgs * sizeof (CMsgCompUnitDamageElem) ); }

		int			m_iNumMsgs;
		CMsgCompUnitDamageElem	m_aMCUDE [ NUM_UNIT_DAMAGE_ELEM ];
};

const int NUM_UNIT_SET_DAMAGE_ELEM = ( VP_MAXSENDDATA - sizeof (CNetCmd) - sizeof (int) ) / sizeof (CMsgCompUnitDamageElem);
class CMsgCompUnitSetDamage : public CNetCmd
{
public:
		CMsgCompUnitSetDamage () : CNetCmd (comp_unit_set_damage) { m_iNumMsgs = 0; }

		int			AddUnit ( CUnit * pUnit );
		void		Reset () { m_iNumMsgs = 0; }
		int			SendSize () const { return ( sizeof (CNetCmd) + sizeof (int) + m_iNumMsgs * sizeof (CMsgCompUnitDamageElem) ); }

		int			m_iNumMsgs;
		CMsgCompUnitDamageElem	m_aMCUDE [ NUM_UNIT_SET_DAMAGE_ELEM ];
};

class CMsgShootElem
{
public:

		DWORD				m_dwID;							// who is shooting
		DWORD				m_dwIDTarget;				// ID of unit being shot at
		WORD				m_wDestX;
		WORD				m_wDestY;
		WORD				m_wNumShots;				// number of shots
};

const int NUM_SHOOT_ELEM = ( VP_MAXSENDDATA - sizeof (CNetCmd) - sizeof (int) ) / sizeof (CMsgShootElem);
class CMsgShoot : public CNetCmd
{
public:
		CMsgShoot () : CNetCmd (shoot_gun) { m_iNumMsgs = 0; }

		int			AddShot ( CUnit const * pUnit, CUnit const * pTarget, CMapLoc const & mlDest, int iShots );
		void		Reset () { m_iNumMsgs = 0; }
		int			SendSize () const { return ( sizeof (CNetCmd) + sizeof (int) + m_iNumMsgs * sizeof (CMsgShootElem) ); }

		int			m_iNumMsgs;
		CMsgShootElem	m_aMSE [ NUM_SHOOT_ELEM ];
};

class CNetAiGpf : public CNetCmd {
public:
		CNetAiGpf (CPlayer const *pPlyr);
		int				m_iPlyrNum;
};

class CNetRsrchDisc : public CNetCmd {
public:
		CNetRsrchDisc (CPlayer const *pPlyr, int iRsrch);
		int				m_iPlyrNum;
		int				m_iRsrch;
};

class CNetNeedSaveInfo : public CNetCmd {
public:
		CNetNeedSaveInfo (CPlayer const *pPlyr);
		int				m_iPlyrNum;
};

class CNetSaveInfo : public CNetCmd {
public:
		CNetSaveInfo (CPlayer const *pPlyr);
		int				m_iPlyrNum;
		int				m_iPplBldg;
		int				m_iPplVeh;
		int				m_iFood;
		int				m_iGas;
		int				m_iRsrchItem;
		int				m_iPtsDiscovered;
};


#pragma pack ( pop, cnetcmd )

#endif
