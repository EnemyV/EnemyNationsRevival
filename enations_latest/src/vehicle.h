//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __VEHICLE_H__
#define __VEHICLE_H__

// vehicle.h : header file for all vehicle stuff
//

#include "unit.h"
#include "unit_wnd.h"
#include "loadtruk.h"
#include "netcmd.h"


class CWndRoute;
class CTransportData;
class CVehicle;
class CBridgeUnit;


const int MAX_NUM_RETRIES = 25;
const int NUM_SUBS_OWNED = 4;


CHexCoord * GetVehiclePath (CTransportData const *pTd, CHexCoord src, CHexCoord dest, int & iNum);

#ifdef _DEBUG
	#define		ASSERT_VALID_LOC(p)		(p->AssertValidAndLoc ())
#else
	#define		ASSERT_VALID_LOC(p)	
#endif



/////////////////////////////////////////////////////////////////////////////
// CTransport - data on vehicles (again, should have been folded into CTransportData)

class CTransport : public CSpriteStore< CVehicleSprite >
{
#ifdef _DEBUG
friend class CTransportData;
friend class CBuilding;
friend class CVehicle;
#endif


public:
									CTransport ( char const * pszRifName );
									~CTransport ();

			void				InitData ();
			void				InitSprites ();
			void				InitLang ();

			CVehicleSprite * GetSprite( int iID ) const;

			void				Close ();
			CTransportData const * GetData (int iIndex) const;
			int		GetNumTransports () const;
			int					GetIndex (CTransportData const * GetData) const;

protected:
		CTransportData * _GetData (int iIndex) const;

		CTransportData *	m_pData;
		int				m_iNumTransports;

		static	int	g_aiDefaultID[];

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CTransportData - all transport specific fixed data

class CTransportData : public CUnitData
{
//#ifdef _DEBUG
friend class CVehicle;
//#endif
friend void CTransport::InitData ();
friend void CTransport::InitSprites ();
friend void CTransport::InitLang ();
public:

		int			GetClassType () { return CUnitData::transport; }

		enum TRANS_TYPE {  construction,		// this is returned by GetType ()
						med_truck,					// obsolete
						heavy_truck,
						light_scout,
						med_scout,
						heavy_scout,
						infantry_carrier,
						light_tank,
						med_tank,
						heavy_tank,
						light_art,
						med_art,
						heavy_art,
						light_cargo,
						gun_boat,
						destroyer,
						cruiser,
						landing_craft,
						infantry,
						rangers,
						marines, 						// obsolete
						num_types };

		enum TRANS_BASE_TYPE {	non_combat,
						artillery,
						troops,
						ship,
						combat };

#ifdef _DEBUG
			CTransportData () { m_iType = (TRANS_TYPE) -1; }
#else
			CTransportData () { }
#endif

			int			GetMaxMaterials () const;

			BOOL		CanTravelHex (CHex const * pHex) const;
			BOOL		CanEnterHex (CHexCoord const & hexSrc, CHexCoord const & hexDest, BOOL bOnWater, BOOL bStrict = TRUE) const;


			int			GetSetupFire () const;
			int			GetPeopleCarry () const;
			int			GetSpeed () const;
			int			GetWheelType () const;
			int			GetWaterDepth () const;

			TRANS_TYPE				GetType () const;
			TRANS_BASE_TYPE		GetBaseType () const;

			BOOL		IsCrane () const;
			BOOL		IsBoat () const;
			BOOL		IsCarrier () const;				// can carry units
			BOOL		IsTransport () const;			// can carry material
			BOOL		IsPeople () const;				// infantry
			BOOL		IsCarryable () const;			// can go in carrier
			BOOL		IsLcCarryable () const;			// can go in landing craft
			BOOL		IsRepairable () const;		// can be repaired

				enum TRANS_FLAGS { FLconstruction = 0x01, FLboat = 0x02, FLcarrier = 0x04, 
								FLtransport = 0x08, FLpeople = 0x10, FLcarryable = 0x20,
								FLrepairable = 0x40, FLlc_carryable = 0x80, 
								FLwheel_amb1 = 0x0100, FLwheel_amb2 = 0x0200,
								FL1hex = 0x0400, FLload_front = 0x0800, FLcivilian = 0x1000,
								FLshoot180 = 0x2000, FLstop_pause = 0x4000 };
			TRANS_FLAGS		GetVehFlags () const { return (m_transFlags); }

static int GetMaxDraft () { return (m_iMaxDraft); }

protected:
			TRANS_TYPE		m_iType;								// enum's above of vehicle types

			int			m_iSetupFire;						// time to set up (artillery)
			int			m_iPeopleCarry;					// people it can carry
			int			m_iSpeed;								// vehicle speed on a road
			int			m_cWheelType;						// type of wheels
			int			m_cWaterDepth;					// land - how deep water it can travel
																						// sea - how much depth they need
			TRANS_FLAGS			m_transFlags;								// see enums

public:
			static CString 		m_sAuto;
			static CString 		m_sRoute;
			static CString 		m_sIdle;
			static CString 		m_sTravel;

static int m_iMaxDraft;								// depth of water needed for biggest boat

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// From here down we are now into the classes that represent a specific unit 
// on the board. So there is 1 instance of the appropiate class below for
// each unit. And in each case, that class points to the appropiate instance
// of the above fixed data class so you can easily access them.
//


/////////////////////////////////////////////////////////////////////////////
// CRoute - used by CVehicle to store the route of a vehicle.

class CRoute : public CObject
{
public:
		enum { waypoint, unload, load };

		CRoute () {}
		CRoute (CHexCoord & hex, int iType) { ASSERT ((0 <= iType) && (iType <= 2));
											m_hex = hex; m_iType = (BYTE) iType; }
		~CRoute () {}

		BOOL operator== (CRoute & src ) const
										{ return ((m_hex.X() == src.m_hex.X()) && (m_hex.Y() == src.m_hex.Y()) && (m_iType == src.m_iType)); }
		BOOL operator!= (CRoute & src ) const
										{ return ((m_hex.X() != src.m_hex.X()) || (m_hex.Y() != src.m_hex.Y()) || (m_iType != src.m_iType)); }

		CHexCoord const & GetCoord () const { return (m_hex); }
		int				GetRouteType () const { return (m_iType); }

		void 					Serialize (CArchive & ar);

protected:
		CHexCoord		m_hex;			// where to go
		BYTE				m_iType;		// enum values above of what to do at this location
};


/////////////////////////////////////////////////////////////////////////////
// CVehicle - a Vehicle

class CMsgVehSetDest;

class CVehicle : public CUnit
{
friend const CMsgVehCompLocElem CMsgVehCompLocElem::operator = ( CVehicle const & src );
friend static void SetVehDest (CMsgVehSetDest * pMsg);
friend static void LoadCarrier (CMsgLoadCarrier * pMsg);
friend static void UnloadCarrier (CMsgUnloadCarrier * pMsg);
friend void CTransport::InitData ();
friend void CTransport::InitSprites ();
friend void CTransport::InitLang ();
friend void CTransport::Close ();
friend CWndOrders;
friend CMsgVehGoto;
friend CMsgVehLoc;
friend CMsgBlocked;
friend CDlgBuildStructure;
friend CWndRoute;

public:
		enum VEH_MODE { 	stop,				// m_iMode - vehicle is stopped
						moving,			// is travelling to a dest or route
						run,				// is building at location
						contention,	// wants hex another vehicle also took
						blocked,		// waiting cause blocked
						cant_deploy,// can't drop into building that created it
						deploy_it,	// server found space - GO
						traffic,		// waiting for the slot ahead to clear
						num_mode };	// for ASSERT

		enum VEH_EVENT { none, route, build, build_road, attack, load, repair_self, repair_bldg, num_event };		// m_iEvent

									CVehicle () { ctor (); }
									~CVehicle ();
									// FIXIT: Once all calls are located, default iTurretID to -1
									CVehicle (int iVeh, int iOwner = 0, DWORD ID = 0);

		virtual void	AddUnit ();
		virtual void	RemoveUnit ();

		virtual int		GetNumStatusBars () const;
		virtual void	PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					PaintStatusCarrier (CStatInst * pSi, CDC * pDc) const;
		void					ShowStatusText (CString & str);

		void 					GetDesc (CString & sText) const;
		CRect					Draw( const CHexCoord & );
		BOOL					IsHit( CHexCoord, CPoint ) const;

		void					Operate ();
		void					Move ();
		BOOL					MoveInHex ();
		void					ArrivedDest ();
		BOOL					FindNextHex ();
		void					ArrivedNextHex ();
		BOOL					GetNextHex (BOOL bNew);
		BOOL					TryNextHex ();
		void					CheckNextHex ();
		void					SetHexDest ();
		void					SetMoveParams (BOOL bFixTurret);
		void					ZeroMoveParams ();
		void					MoveCargo ( BOOL bCarried );

		BOOL					CanShootAt (CUnit * pUnit);

		void					ChangeTile (int iType, int iIndex);
		void					InvalidateStatus () const;
		void					StartTravel (BOOL bGetPath);
		void					MsgSetNextHex (CMsgVehLoc * pMsg);
		void					SetFromMsg (CMsgVehLoc * pMsg, BOOL bWorld);
		void					StopUnit ();
		void					ResumeUnit ();

		void					EnterBuilding ();
		void					ExitBuilding ();
		void					CheckExit ();

		CSubHex const & 		GetPtDest () const { return (m_ptDest); }
		CHexCoord 				 	GetHexDest () const { return (m_hexDest); }
		CSubHex const & 		GetPtNext () const { return (m_ptNext); }
		CSubHex const & 		GetPtHead () const { return (m_ptHead); }
		CSubHex const & 		GetPtTail () const { return (m_ptTail); }
		CHexCoord 					GetHexHead () const { CHexCoord _hex ( m_ptHead );
																							_hex.Wrap ();
																							return _hex; }
		int						GetMainDir () const { return ((((m_iDir + FULL_ROT / 16) * 8) / FULL_ROT) & 0x07); }
		int						GetDir () const { return m_iDir; }
		int						CalcDir () const;
		int						CalcNextDir () const;
		int						CalcBaseDir () const;
		int						CalcNextBaseDir () const;

		CTransportData const *	GetData () const;

		void					DestroyRouteWindow ();
		void					DestroyBuildWindow ();
		void					DestroyLoadWindow ();
		void					DestroyAllWindows ();

		void					UpdateChoices ();
		CDlgBuildStructure * 	GetDlgBuild ();
		CDlgLoadTruck * 	GetDlgLoad ();
		void					NullLoadWindow () { m_pDlgLoad = NULL; }

		static CVehicle * Create ( const CSubHex & ptHead, const CSubHex & ptTail, 
											int iVeh, int iOwner = 0, DWORD ID = 0, VEH_MODE iRouteMode = stop, 
											CHexCoord & hexDest = CHexCoord (0,0), DWORD dwIDBldg = 0, int iDelay = 0);
		static void				StopConstruction (CBuilding * pBldg);
		static void				GetExitLoc (CBuilding const * pBldg, int iType, CSubHex & subNext, CSubHex & subHead, CSubHex & subTail);

		void					HandleCombat ();
		void					StopShooting (CUnit * pNewTarget);

		int						GetProd (float fProd);

		void					DetermineSpotting ();
		void					DetermineOppo ();

		void					ConstructBuilding ();
		void					ConstructRoad ();
		void					StartConst (CBuilding * pBldg);
		CBuilding *		GetConst () const { ASSERT_STRICT_VALID_OR_NULL (m_pBldg); return (m_pBldg); }
		VEH_EVENT			GetEvent () const { return (m_iEvent); }
		int						GetBldgType () const { return (m_iBldgType); }
		void					SetBldgType (int iType) { m_iBldgType = iType; }
		CHexCoord const & GetHexBldg () const { return (m_hexBldg); }
		void					SetBuilding (CHexCoord const & hex, int iBldgType, int iBldgDir);
		void					SetEvent (VEH_EVENT iEvent);

		int						GetRoadPer () const { return m_iLastPer; }

		void					BuildAns (CNetAnsTile * pCmd);
		void					BuildBldg ();
		void					BuildRoad ();
		void					SetRoad ( const CHexCoord & hexSrc, const CHexCoord & hexDest);
		void					SetRoadHex (CHexCoord const & hex) { m_hexStart = m_hexEnd = hex; }
		void					SetRoadHex (CHexCoord const & hexStart, CHexCoord const & hexEnd) { m_hexStart = hexStart; m_hexEnd = hexEnd; }
		void					SetBridgeHex (CHexCoord const & hexStart, CHexCoord const & hexEnd, DWORD dwID, int iAlt);

		void					GetPath (BOOL bNoOcc);
		BOOL					HavePath () const;
		BOOL					HavePathOrNext () const;
		void					PathNextHex ();
		void					DeletePath ();
		void					SetLocation (CHexCoord & hex, POSITION pos, int iType);
		CList <CRoute *, CRoute *> &	GetRouteList () { ASSERT_STRICT_VALID (this); return (m_route); }
		POSITION			GetRoutePos () const { ASSERT_STRICT_VALID (this); return (m_pos); }
		VEH_MODE			GetRouteMode () const;
		void					SetRoutePos (POSITION pos);
		void					SetRouteMode (VEH_MODE iMode);
		void					SetModeGo ();
		void					PostArrivedOrBlocked ();
		void					CheckAroundBuilding ();

		void					TakeOwnership ();
		int						GetHexOwnership () const { return (m_cOwn); }
		void					ReleaseOwnership ();
		void					CantInBldg (CBuilding const * pBldg);
		void					MakeBlocked ();
		void					ForceAtDest ();

		BOOL					CanEnterBldg ( CBuilding * pBldg ) const;

		void					AtNewLoc ();

						enum VEH_POS { any, sub, full, center };
		void					SetDest (CHexCoord const & hex)
													{ SetDestAndMode (hex, sub); }
		void					SetDest (CSubHex const & hex)
													{ SetDestAndMode (hex, sub); }
		void					SetDestMode (VEH_POS iMode) { m_iDestMode = iMode; }
		VEH_POS				GetDestMode () const { return (m_iDestMode); }
		void					SetDestAndMode (CHexCoord const & hex, VEH_POS iMode)
													{ CSubHex _sub (hex.X () * 2, hex.Y () * 2);
														SetDestAndMode (_sub, iMode); }
		void					SetDestAndMode (CSubHex sub, VEH_POS iMode);
		void					KickStart ();

		void					SetEventAndRoute (VEH_EVENT iEvent, VEH_MODE iMode) { SetEvent (iEvent); SetRouteMode (iMode); }
		void					_SetEventAndRoute (VEH_EVENT iEvent, VEH_MODE iMode) { SetEvent (iEvent); _SetRouteMode (iMode); }
		void					_SetRouteMode (VEH_MODE iMode);

		void					Wheels (BOOL bEnable, BOOL bSfx);
		void					HandleDest ();
		void					Load ();
		void					Unload ();
		void					UnloadCarrier ();
		BOOL					IsLoadOk (int iVehTyp) const;

		void 					Serialize (CArchive & ar);
		void					FixUp ();
		void					FixForPlayer ();

		CWndRoute *		m_pWndRoute;						// route window

		static int **	m_apiWid;								// used for spotting range
		static int		m_iMaxRange;

		CVehicleSprite * 	GetSprite() const
								{
									#ifdef _DEBUG
									(( CVehicleSprite * )m_psprite )->CheckValid();
									#endif
									return ( CVehicleSprite * )m_psprite;
								}

		CVehicle *		GetTransport () const { return m_pTransport; }
		CVehicle *		GetLoadOn () const { return m_pVehLoadOn; }
		void					SetTransport ( CVehicle * pCarrier );
		void					SetLoadOn ( CVehicle * pCarrier );
		int						GetCargoSize () const { return m_iCargoSize; }
		int						GetCargoCount () const { return m_lstCargo.GetCount (); }
		POSITION			GetCargoHeadPosition () { return m_lstCargo.GetHeadPosition (); }
		CVehicle *		GetCargoNext (POSITION & pos) { return m_lstCargo.GetNext (pos); }

		void					DumpContents ();

		void					TempTargetOff () { m_bFlags &= ~ temp_target; }

		void					HpControlOn () { m_bFlags |= hp_controls; }
		void					HpControlOff () { m_bFlags &= ~ hp_controls; }
		BOOL					IsHpControl () const { return (m_bFlags & hp_controls); }

		void					ToldAiStopOn () { m_bFlags |= told_ai_stop; }
		void					ToldAiStopOff () { m_bFlags &= ~ told_ai_stop; }
		BOOL					IsToldAiStop () const { return (m_bFlags & told_ai_stop); }

		void					DoSpottingOn () { m_bFlags |= do_spotting; }
		void					DoSpottingOff () { m_bFlags &= ~ do_spotting; }
		BOOL					DoSpotting () const { return (m_bFlags & do_spotting); }

		void					NewLocOn () { m_bFlags |= new_loc; }
		void					NewLocOff () { m_bFlags &= ~ new_loc; }
		BOOL					IsNewLoc () const { return (m_bFlags & new_loc); }

		void					OnWaterOn () { m_bFlags |= on_water; }
		void					OnWaterOff () { m_bFlags &= ~ on_water; }
		BOOL					IsOnWater () const { return (m_bFlags & on_water); }
		void					SetOnWater (BOOL bOnWater) { if (bOnWater) OnWaterOn (); else OnWaterOff (); }

		CBridgeUnit 	 * GetBridgeOn() const;	// Pointer to bridge unit under maploc center, or NULL if none

		void					AddSubOwned ( int x, int y );
		void					RemoveSubOwned ( int x, int y );

		BOOL					IsInBuilding() const;	// GG

protected:
		BOOL					NextVisible (int * * ppiOn, int * piRange, int & iXmax, int iYmin, int iYmax, CHexCoord const & hexOrig, CHexCoord & hexDest, int & iMode, int iDif);
		void					DetermineSpeed (BOOL bEvent);
		CHexCoord			_NextRoadHex (CHexCoord const & _hexOn);
		BOOL					NextRoadHex ();
		BOOL 					GetDrawParms( CQuadDrawParms &drawparms, CBridgeUnit const * ) const;

		void					ctor ();
		void					SetLoc (BOOL bNew);
		void					AssignNextHex ();
		BOOL					TestStuck ();
		void					HandleBlocked ();
		BOOL					TryNewSub (BOOL bNoNewPath);
		BOOL					FindSub (BOOL bCloser = FALSE);
		CSubHex				Rotate (int iDir);
		void					Turn180 ();
		BOOL					IsPassable (CSubHex const & _sub, BOOL bStrict = TRUE);
		BOOL					CanEnter (CSubHex const & _sub, BOOL bStrict = TRUE);
		int		GetTiltIndex( BOOL bOnBridge ) 		const;

		BOOL	GetHotSpotClient(	CSpriteView		const	&,
										CQuadDrawParms	const	&,
										CHotSpotKey::HOTSPOT_TYPE,
										int,
										CPoint * ) const;

		CDlgBuildStructure * 	m_pDlgStructure;
		CDlgLoadTruck *				m_pDlgLoad;

		// travelling in another vehicle, carrying another vehicle
		CVehicle *		m_pTransport;						// unit carrying us
		CList <CVehicle *, CVehicle *>	m_lstCargo;	// units we are carrying
		int						m_iCargoSize;						// how much we have
		CVehicle *		m_pVehLoadOn;						// vehicle to load on

		// where we are going
		CList <CRoute *, CRoute *> m_route;		// route its travelling
		POSITION			m_pos;									// element we are travelling to
		CSubHex				m_ptDest;								// final sub-hex we are going to
		CHexCoord			m_hexDest;							// final hex we are going to
		CHexCoord			m_hexLastDest;					// to stop back and forth
		VEH_POS				m_iDestMode;						// dest mode (sub, full, center)

		CHexCoord			m_hexVis;								// hex visibility determined on

		// to stop circling
		CSubHex				m_subOn;								// sub testing for circling
		CSubHex				m_subBlocked;						// sub last blocked on
		int						m_iTimesOn;							// how many times we have been on this
		int						m_iNumClosest;					// number of times we have tested to see if we are as close as possible
		int						m_iNumRetries;					// number of times we have tried for a new dest
		int						m_iClosest;							// closest we got to the dest

		DWORD					m_dwTimeBlocked;				// we only wait 1.2 * hex transit time when blocked
		LONG					m_iBlockCount;					// number of consecutive times blocked

		DWORD					m_dwTimeJump;						// for AI trucks & cranes we transport if can't get there by this time

		BYTE					m_cOwn;									// TRUE if own's hexes
		VEH_MODE			m_cMode;								// mode we are in (goto, route, operate)

		int						m_iDelay;								// delay on deploying

		// construction vehicle
		CBuilding *		m_pBldg;								// building we are building
		VEH_EVENT			m_iEvent;								// what to do when arrive
		CHexCoord			m_hexBldg;							// build this building
		LONG					m_iBldgType;						// at this location
		LONG					m_iBuildDir;						// direction to drop it at

		CHexCoord			m_hexStart;							// building a road
		CHexCoord			m_hexEnd;
		LONG					m_iBuildDone;						// -1 == done, time so far in process
		LONG					m_iLastPer;

		// these handle dwNow on the last Operate, and remainders from that move
		LONG					m_lOperMod;							// apply to next operate
		int						m_FireSetupMod;					// for artillery setup

		// new how we are getting there				// each hex is divided into 4 sub-hexes
		CSubHex				m_ptNext;								// sub-hex want car to go to
		CSubHex				m_ptHead;								// sub-hex head of car is in
		CSubHex				m_ptTail;								// sub-hex tail of car is in
		CHexCoord			m_hexNext;							// next hex in path
		LONG					m_iStepsLeft;						// steps left moving to this hex
		LONG					m_iDir;									// angle car is facing (0..63)
		float					m_fVehMove;

		CSubHex				SubsOwned [NUM_SUBS_OWNED];					// track what we own for theVehicleHex

		LONG					m_iSpeed;								// speed on this hex
		LONG					m_iXadd;								// -1,0,1 for moving
		LONG					m_iYadd;
		LONG					m_iDadd;
		LONG					m_iTadd;

		enum { do_spotting = 0x01, new_loc = 0x02, hp_controls = 0x04, told_ai_stop = 0x08,
						on_water = 0x10, at_end_of_path = 0x20, dump_contents = 0x40, temp_target = 0x80 };
		WORD					m_bFlags;

		// getting the path
		CHexCoord *		m_phexPath;							// array returned from getpath
		LONG					m_iPathOff;							// which element we need to go to next
		int						m_iPathLen;							// how many elements there are

		CMsgVehLoc		m_mvlTraffic;						// last reported position

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
	void 	AssertValidAndLoc () const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// unit maps - we store a map of all instances so we can walk all members and
// find one quickly by dwID

class CVehicleMap : public CMap <DWORD, DWORD, CVehicle *, CVehicle *>
{
public:
	CVehicleMap () { InitHashTable (HASH_INIT); }

	void					Add (CVehicle * pVeh);
	void					Remove (CVehicle * pVeh);

	CVehicle *		GetVehicle (DWORD dwID) const;
	CVehicle *		_GetVehicle (DWORD dwID) const;
};

// tracks hexes units are on
class CVehicleHex : public CMap <DWORD, DWORD, CVehicle *, CVehicle *>
{
public:
	CVehicleHex () { InitHashTable (HASH_INIT); }

	void					GrabHex (CSubHex const & pt, CVehicle * pUnit)
																			{ GrabHex (pt.x, pt.y, pUnit); }
	void					GrabHex (int x, int y, CVehicle * pUnit);
	void					ReleaseHex (CSubHex const & pt, CVehicle * pVeh)
																			{ ReleaseHex (pt.x, pt.y, pVeh); }
	void					ReleaseHex (int x, int y, CVehicle * pVeh);

	CVehicle *		GetVehicle (CSubHex const & pt) const;
	CVehicle *		GetVehicle (int x, int y) const;

	CVehicle *		_GetVehicle (CSubHex const & pt) const;
	CVehicle *		_GetVehicle (int x, int y) const;

	void					CheckHex (CSubHex const & _sub);

protected:
	DWORD					ToArg (int x, int y) const;
	DWORD					_ToArg (int x, int y) const;
};


int _CalcBaseDir (CPoint const & ptHead, CPoint const & ptTail);

extern CTransport theTransports;
extern CVehicleMap theVehicleMap;
extern CVehicleHex theVehicleHex;

extern void SerializeElements (CArchive & ar, CVehicle * * ppVeh, int nCount);
extern void SerializeElements (CArchive & ar, CRoute * * ppRt, int nCount);


#endif
