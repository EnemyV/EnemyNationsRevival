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
#include "netcmd.h"
#include "enprobes.h"


class CWndRoute;
class SDL2RouteWindow;
class SDL2BuildStructure;
class SDL2LoadTruckDialog;
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
			static std::string 	m_sAuto;
			static std::string 	m_sRoute;
			static std::string 	m_sIdle;
			static std::string 	m_sTravel;

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
// [SUBATTEMPT] selector record (015 T2 item 1/2 - INSTRUMENT ONLY, no behaviour
// change). CVehicle::FindSub is the only thing that picks m_ptNext, and a census
// snapshot cannot reconstruct which directions it walked or what turned each one
// down (WinAstra review 10: "T1 still infers history from a snapshot"). So the
// selector records its OWN latest attempt, in the loop, at the existing
// comparisons - it never re-runs FindSub/GetPath and never touches the random
// state. ONE bounded record per vehicle (latest attempt only, no stream), written
// on whatever thread ran FindSub; read back by TrafficCensus for [STUCK] context.
// Runtime-only, NOT serialized: zero-initialised by the in-class {} below so the
// existing ctor path in new_unit.cpp needs no change.
const int EN_SUB_MAXDIR = 8;   // widest FindSub sweep: FL1hex walks iDir -4..+3

// first rejecting gate per direction, in the order FindSub tests them
enum {
	EN_SUBGATE_NONE = 0,		// considered and not rejected (winner OR a valid loser)
	EN_SUBGATE_OLDNEXT,			// _next == the old m_ptNext
	EN_SUBGATE_NOTCLOSER,		// bCloser and not closer to m_hexNext
	EN_SUBGATE_OCCUPIED,		// a vehicle holds the sub (id/mode recorded, self flagged)
	EN_SUBGATE_BLDGENTRY,		// CanEnterBldg said no
	EN_SUBGATE_TERRDIR,			// CanEnterHex said no (directional terrain / bridge deck)
	EN_SUBGATE_SAMEBLDG,		// would turn back into the building we are standing on
	EN_SUBGATE_DIAGPAIR,		// diagonal step and ONE vehicle holds both corner subs
	EN_SUBGATE_ZEROCOST,		// GetTerrainCost == 0
	EN_SUBGATE_COSTTHRESH,		// first-winner test: iSpeed >= iBestSpeed * 8
	EN_SUBGATE_NUM };

// caller/rung tag - which of the seven real FindSub call sites ran this attempt
enum {
	EN_SUBTAG_OTHER = 0,		// a call site not on the list below
	EN_SUBTAG_TNS_FIRST,		// TryNewSub, first search
	EN_SUBTAG_TNS_REPATH,		// TryNewSub, second search after GetPath (TRUE)
	EN_SUBTAG_TNS_CLOSER,		// TryNewSub, closer-only anti-circle rung
	EN_SUBTAG_GNH_SPEED,		// GetNextHex, slow/impassable-terrain re-search
	EN_SUBTAG_GNH_CANTENTER,	// GetNextHex, CanEnter (m_ptNext) failed
	EN_SUBTAG_GNH_CLOSER,		// GetNextHex, closer-only anti-circle rung
	EN_SUBTAG_MSGNEXTHEX,		// MsgSetNextHex traffic shuffle (runs on the OTHER vehicle)
	EN_SUBTAG_NUM };

extern char const * EnSubTagName (int iTag);		// vehmove.cpp
extern char const * EnSubGateName (int iGate);		// vehmove.cpp

struct EnSubAttempt
{
	DWORD			dwSeq;			// per-vehicle attempt counter; 0 = nothing recorded yet
	DWORD			dwTimeMs;		// raw timeGetTime () when the attempt ran
	DWORD			dwOkRun;		// consecutive successes immediately before this attempt
	BYTE			bTag;			// EN_SUBTAG_*
	BYTE			bCloser;		// the bCloser argument
	BYTE			bResult;		// 1 = FindSub returned TRUE
	BYTE			bNumDir;		// directions actually walked (<= EN_SUB_MAXDIR)
	int				iWheel;			// GetWheelType ()
	int				iVehType;		// CTransportData::GetType ()
	short			sHeadX, sHeadY;
	short			sTailX, sTailY;
	short			sOldNextX, sOldNextY;	// m_ptNext on entry (the excluded candidate)
	short			sHexNextX, sHexNextY;	// m_hexNext - what bCloser measures against
	short			sDestX, sDestY;			// m_hexDest - the FINAL destination
	short			sSelX, sSelY;			// m_ptNext on return (the selected next)
	signed char		acDir  [EN_SUB_MAXDIR];		// iDir, in loop order
	signed char		acGate [EN_SUB_MAXDIR];		// EN_SUBGATE_*
	signed char		acOccMode [EN_SUB_MAXDIR];	// occupant VEH_MODE, -1 = no occupant
	BYTE			abOccSelf [EN_SUB_MAXDIR];	// 1 = the occupant is this vehicle itself
	short			asCandX [EN_SUB_MAXDIR], asCandY [EN_SUB_MAXDIR];
	DWORD			adwOccId [EN_SUB_MAXDIR];	// occupant / diagonal blocker id, 0 = none
};

/////////////////////////////////////////////////////////////////////////////
// CVehicle - a Vehicle

class CMsgVehSetDest;

class CVehicle : public CUnit
{
friend const CMsgVehCompLocElem CMsgVehCompLocElem::operator = ( CVehicle const & src );
friend void SetVehDest (CMsgVehSetDest * pMsg);
friend void LoadCarrier (CMsgLoadCarrier * pMsg);
friend void UnloadCarrier (CMsgUnloadCarrier * pMsg);
friend void CTransport::InitData ();
friend void CTransport::InitSprites ();
friend void CTransport::InitLang ();
friend void CTransport::Close ();
friend class CWndOrders;
friend class CMsgVehGoto;
friend class CMsgVehLoc;
friend class CMsgBlocked;
friend class CDlgBuildStructure;
friend class CWndRoute;
#if EN_PATH_PROBES
friend class CPathMgr;	// mpath.reclamp probe reads/writes m_hexLastClamp
#endif

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
		void					ShowStatusText (std::string & str);

		void 					GetDesc (std::string & sText) const;
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

		// Effective cargo capacity: base (per-type data) scaled by the owner's
		// cargo_handling research (CPlayer::GetCargoPct). Use THIS, not
		// GetData()->GetMaxMaterials(), wherever capacity matters (load caps, UI,
		// the over-capacity TRAP) so the tech applies to every truck.
		int						GetMaxMaterials () const;

		void					DestroyRouteWindow ();
		void					DestroyBuildWindow ();
		void					DestroyAllWindows ();

		void					UpdateChoices ();
		CDlgBuildStructure * 	GetDlgBuild ();
		void					ShowLoadDialog ();

		static CVehicle * Create ( const CSubHex & ptHead, const CSubHex & ptTail, 
											int iVeh, int iOwner = 0, DWORD ID = 0, VEH_MODE iRouteMode = stop, 
											const CHexCoord & hexDest = CHexCoord (0,0), DWORD dwIDBldg = 0, int iDelay = 0);
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
		CBuilding *		GetConstBldg () const { return (m_pBldg); }	// build/repair work target
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
		// Looping vs one-shot route. TRUE (default) = the legacy behavior (cycle back to
		// the first stop at the end); FALSE = stop at the last stop. Runtime-only (not
		// serialized — defaults to looping on load to preserve save compatibility).
		BOOL				GetRouteLoop () const { return (m_bRouteLoop); }
		void				SetRouteLoop (BOOL b) { m_bRouteLoop = b; }
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
		void					RelocateTo (CSubHex const & ptHead, CHexCoord const & hexDest);  // re-place of an existing unit (AI 10-min teleport)

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

		CWndRoute *		m_pWndRoute;						// route window (MFC)
	SDL2RouteWindow* m_pSdlRoute = nullptr;				// route window (SDL2)

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
		int						GetEffPeopleCarry ();	// base unit hold + owner's Landing Craft tech bonus (landing craft only)
		int						GetCargoCount () const { return m_lstCargo.GetCount (); }
		POSITION			GetCargoHeadPosition () { return m_lstCargo.GetHeadPosition (); }
		CVehicle *		GetCargoNext (POSITION & pos) { return m_lstCargo.GetNext (pos); }

		void					DumpContents ();

		// Traffic census, one line per LOCAL player (docs/plans/015-focus-investigation.md
		// 1.3, discussion repo): the trucks-moving metric plus the in-a-building-that-is-
		// not-my-destination count. Backs the harness `traffic` verb and the 30 s
		// [CENSUS] line in traffic.log. Read-only.
		static void			TrafficCensus (std::string & out);

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
		// iTag = EN_SUBTAG_* - which call site/rung is asking, for the [SUBATTEMPT]
		// record. Defaulted so nothing but the instrument has to care.
		BOOL					FindSub (BOOL bCloser = FALSE, int iTag = EN_SUBTAG_OTHER);
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
	SDL2BuildStructure*     m_pSdlBuild = nullptr;  // SDL2 non-modal build dialog
	SDL2LoadTruckDialog*    m_pSdlLoad  = nullptr;  // SDL2 non-modal load-cargo dialog

		// travelling in another vehicle, carrying another vehicle
		CVehicle *		m_pTransport;						// unit carrying us
		CList <CVehicle *, CVehicle *>	m_lstCargo;	// units we are carrying
		int						m_iCargoSize;						// how much we have
		CVehicle *		m_pVehLoadOn;						// vehicle to load on

		// where we are going
		CList <CRoute *, CRoute *> m_route;		// route its travelling
		BOOL				m_bRouteLoop;						// TRUE = loop the route (legacy); FALSE = stop at the end
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
		CHexCoord			m_hexStagnant;					// blocked-stagnation watch: last hex seen blocked at (transient, not saved)
		DWORD					m_dwStagnantSince;			// real ms when we first saw it blocked at that hex (0 = not watching)
		DWORD					m_dwEnteredWrongAt;			// traffic probe: game ms (theGame.GettimeGetTime) when [ENTERWRONG] fired for the
															// building we are in, 0 = none. Runtime-only, NOT serialized; cleared by ExitBuilding.
		BYTE					m_bPathFail;					// traffic probe: why the last GetPath() failed. 0 = none, 1 = vehicle-free search
															// failed, 2 = vehicle-aware search failed. Runtime-only, NOT serialized.
		// [PATHRES]/[REISSUE] join (015 R11 instrument, docs/plans/015-fix-plan.md 3.1 v2).
		// Runtime-only and NEVER serialized - the NSDMIs zero them without touching
		// CVehicle::ctor or the save format, exactly like m_subAttempt below. Written ONLY
		// under the EN_TRAFFIC_LOG gate, from whatever thread ran GetPath.
		DWORD					m_dwTrafPathSeq = 0;		// per-vehicle GetPath call counter; the `seq` field of [PATHRES]
		DWORD					m_dwTrafResSeq = 0;			// seq of the LAST EMITTED [PATHRES], 0 = none yet; printed by [GIVEUP]
		const char *			m_pszTrafResCls = NULL;		// class name of that record (a string literal), NULL = none; printed by [GIVEUP]
		BYTE					m_byTrafPrevFull = 1;		// was the PREVIOUS GetPath result the `full` class? Rate bound: a full
															// result is printed only when this is 0 (the recovery transition).
															// Starts at 1 so a vehicle that only ever succeeds stays silent.
		DWORD					m_dwTrafReissue = 0;		// seq of the last NON-FULL result, i.e. the failed search not yet
															// joined to an order; [REISSUE] prints it and clears it back to 0.
		BYTE					m_byTrafNoNotify = 0;		// [NONOTIFY_CHG] (015 R12): 1 = this vehicle hit
															// PostArrivedOrBlocked's blocked fall-through (human non-transport - nobody told).
															// Cleared by the next real mode change in _SetRouteMode, which logs it. Runtime-only,
															// NOT serialized, and NOTHING the game reads.
		// [PATHRES] emitter. A static MEMBER (like TrafficCensus) because it has to bump
		// the five fields above; iCls is the unit.cpp EN_PATHCLS_* value passed as an int
		// so that enum stays local to the probe block. Defined in unit.cpp, no-op when the
		// EN_TRAFFIC_LOG gate is off, and it writes NOTHING the game reads.
		static void				EnTrafPathRes (CVehicle * pVeh, int iCls, BOOL bNoOcc, const char * pszReason,
									   CHexCoord const & hexReq, CHexCoord const & hexFrom,
									   int iRetX, int iRetY, int iLen, int iRetries, long lBc);
		// [SUBATTEMPT] (015 T2): the LATEST FindSub attempt on this vehicle - one bounded
		// record, no stream. Written by FindSub on whatever thread ran it; read by
		// TrafficCensus for the [STUCK] "last attempt seq/age" context. Runtime-only and
		// NOT serialized - the = {} zero-inits it without touching CVehicle::ctor.
		EnSubAttempt			m_subAttempt = {};
#if EN_PATH_PROBES
		CHexCoord			m_hexLastClamp;					// mpath.reclamp probe: hex of last clamped path, (-1,-1) = none (transient, not saved)
#endif

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

// Traffic probe sink (vehicle.cpp). Switched at RUNTIME by EN_TRAFFIC_LOG (set, not "0")
// so one build serves every platform and the probes can stay in Release; writes
// traffic.log in the launch dir. EN_TRAFFIC_VEH=<id> follows one vehicle ([FOLLOW]).
extern bool		EnTrafficLogOn ();
extern void		EnTrafficLog (const char * pszFmt, ...);
extern DWORD	EnTrafficFollowId ();

// AI liveness counter (ai.cpp): bumped once per CAIMgr::Manage() pass, indexed by
// player number, so TrafficCensus can print it as `aiticks` - a counter that stops
// moving while that player still has vehicles is a dead/wedged AI thread, not a
// traffic jam. NOTE: the game has NO fixed player array to borrow a bound from -
// players live in CGame's CList and per-player buffers are sized at runtime from
// theGame.GetMaxPlyrNum() - so this probe carries its own generous fixed bound and
// EVERY access is range-checked. 64 matches the player-colour table's modulus
// (NUM_PLYR_COLORS, player.cpp), the widest player number the game ever renders.
const int EN_AI_TICK_PLYRS = 64;
extern volatile long g_alAiManageTicks [];

// AI seek-scan counters (defined in caitmgr.cpp), same bound and the same
// bounds-checked access rule as g_alAiManageTicks above - player numbers are
// runtime-assigned, so EVERY access is range-checked. They exist to answer one
// question: is CAIGoalMgr::GetOpForUnitScan what stops CAITaskMgr::AssignUnits
// from finishing a walk (20 of 26 AI workers sampled inside that scan on a
// contended-lock leaf, one scan ~100 s, so trucks never got dispatched).
//   ScansRun / ScanMsTotal / ScanMsMax - GetOpForUnitScan calls and their cost
//   ScansSkipBudget          - scans declined by the per-walk budget (part D)
//   WalksStarted / WalksDone - AssignUnits entries vs normal exits: started
//                              running away from done IS the stall
//   ScanCandTotal / Max      - per-candidate AssessThreat/AssessTarget cs takes
//                              per scan. Part C hoists the per-HEX takes only,
//                              on the assumption candidates are few; without
//                              this a flat lock-wait after part C cannot be
//                              told from "there were never many candidates"
//   ScanOpForTotal / Max     - CAIUnitList::GetOpForUnit calls per scan. That
//                              call takes cs on every index miss and allocates
//                              under it (caiunit.cpp:1523) and is deliberately
//                              NOT changed here, so "one acquisition per row"
//                              is only true while this number is small
//   SeekTargets              - seek units seen holding a target (DataDW != 0)
//                              so far in the current/last walk
//   SeekLoops                - [SEEKLOOP] wedges: SeekOpfor rejected the SAME
//                              target id on two consecutive turns of one call's
//                              `goto SeekNDestroy` loop. One bump per wedge, not
//                              per turn, so a non-zero value with frozen aiticks
//                              is the spin itself, not a rate
extern volatile long g_alAiScansRun [];
extern volatile long g_alAiScansSkipBudget [];
extern volatile long g_alAiScanMsTotal [];
extern volatile long g_alAiScanMsMax [];
extern volatile long g_alAiWalksStarted [];
extern volatile long g_alAiWalksDone [];
extern volatile long g_alAiScanCandTotal [];
extern volatile long g_alAiScanCandMax [];
extern volatile long g_alAiScanOpForTotal [];
extern volatile long g_alAiScanOpForMax [];
extern volatile long g_alAiSeekTargets [];
extern volatile long g_alAiSeekLoops [];

// R10 traffic DENOMINATORS (015 T2 item 4, defined in vehicle.cpp). Same bound and
// the same range-check-every-access rule as the AI counters above. These are EXACT
// event counts, deliberately kept apart from the sampled [SUBATTEMPT]/[STUCK] detail:
// a rate needs a denominator that is not itself throttled.
//   OrdersOk    - CAIUnit::SetDestination calls that passed the 30 s dedupe AND the
//                 same-location drop and actually reached theGame.PostToServer
//   OrdersWake  - the subset posted BECAUSE the target hex is where the unit already
//                 stands (the truck "wake" order): counted separately so an order rate
//                 is not inflated by wakes that ask for no movement at all
//   Steps       - PHYSICAL sub-step completions: bumped in CVehicle::ArrivedNextHex at
//                 the line where the head sub actually advances. Placement, carried
//                 (MoveCargo) and the stuck teleport never reach that line
//   Deliveries  - truck unloads completed at BOTH routers: the AI path
//                 (CAIMgr::DestinationResponse -> CAIRouter::UnloadMaterials) and,
//                 since 015 T2b, the human auto-router path
//                 (CHPRouter::DestinationResponse -> CHPRouter::UnloadMaterials,
//                 chproute.cpp - both its normal and its post-restore unload branch).
//                 A human row of 0 now means no deliveries, not an absent counter
//   SweepMsMax  - longest CAIMgr::HandleStuckVehicles call for that player, ms
extern volatile long g_alTrafOrdersOk [];
extern volatile long g_alTrafOrdersWake [];
extern volatile long g_alTrafSteps [];
extern volatile long g_alTrafDeliveries [];
extern volatile long g_alTrafSweepMsMax [];

// T2b GetPath return CLASSES (015 T2 item 1, defined in vehicle.cpp, stamped in
// unit.cpp at every exit of CVehicle::GetPath). Indexed [player][mode]. bNoOcc is
// handed STRAIGHT to CPathMgr::GetPath's `bVehBlock` parameter, documented at
// cpathmgr.h:134-135 as "default (FALSE) means that path goes thru vehicles, TRUE
// means vehicles will block" and used that way at cpathmgr.cpp:1072, so the original
// T2b legend had the two modes BACKWARDS. Corrected (015 R11 item 3):
//   mode 0 = bNoOcc FALSE = the path may run THROUGH vehicles = vehicle-FREE  - suffix `_free`
//   mode 1 = bNoOcc TRUE  = an occupied hex is no-entry       = vehicle-AWARE - suffix `_aware`
// EXACTLY ONE class is stamped per GetPath call, so for a given player the ten cells
// sum to the number of calls - that is what makes them usable as a denominator.
// Every classification is read off state GetPath has ALREADY computed; nothing here
// runs a second pathfind or re-queries the map. Same runtime bound and the same
// range-check-every-access rule as the counters above.
//   PathEmpty   - the "we're stuck" exit: no path at all (m_iPathLen <= 0), or a path
//                 whose first and last hex are the same. BOTH of its returns count
//                 here (the blocked one and the in-building cant_deploy guard)
//   PathRepPart - the "can't reach dest, same path as 2 ago" exit: the search came
//                 back SHORT of the requested destination and short at exactly the hex
//                 it stopped at last time, so the path is thrown away. Both of its
//                 exits count here (the cant_deploy return and the fall-through)
//   PathDegen   - ACCEPTED but degenerate: m_iPathLen == 1, i.e. the vehicle is handed
//                 a one-hex "path" (src == dest after the exit-hex adjustment)
//   PathAccPart - ACCEPTED but PARTIAL: the path ends somewhere other than the hex
//                 that was asked for, and NOT at the same short end as last time.
//                 This is GetPath's own truncation test (_newDest != _hexDest), not a
//                 shadow computation
//   PathFull    - ACCEPTED and complete: length > 1, ending on the requested hex
extern volatile long g_alTrafPathEmpty [][2];
extern volatile long g_alTrafPathRepPart [][2];
extern volatile long g_alTrafPathDegen [][2];
extern volatile long g_alTrafPathAccPart [][2];
extern volatile long g_alTrafPathFull [][2];


#endif
