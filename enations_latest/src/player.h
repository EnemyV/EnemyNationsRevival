//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "base.h"
#include "netcmd.h"
#include "racedata.h"
#include "research.h"
#include "vpxfer.h"

class CNetCmd;
class CUnit;
class CDlgFile;
class CConquerApp;
class CHPRouter;
class CStructureData;


const int RELATIONS_ALLIANCE = 0;
const int RELATIONS_PEACE = 1;
const int RELATIONS_NEUTRAL = 2;
const int RELATIONS_WAR = 3;

const int MSG_POOL_SIZE = ( sizeof (CMsgBldgStat) + 3 ) & ~3;

typedef struct tagAI_INIT {
		DWORD			dwHdl;
		CHexCoord	hex;
		} AI_INIT;


#ifdef _DEBUG
extern BOOL TestEverything ();
#endif


/////////////////////////////////////////////////////////////////////////////
// CCitCon - for citizen construction

class CCitCon
{
public:
		CCitCon (int iBldg, CHexCoord const & hex, int iDir);

		int					m_iBldg;				// bldg to build
		CStructureData const *		m_pData;				// bldg to build
		CHexCoord		m_hex;					// hex on
		int					m_iDir;					// direction going
		int					m_iTryLeft;			// tries left
		int					m_iStraight;		// hexes we have gone straight
		int					m_iLeft;				// hexes since a left turn
		int					m_iRight;				// hexes since a right turn
		int					m_iNumRoads;		// how many roads made
		int					m_iBldgDir;			// direction to place bldg in
		BOOL				m_bTurn;				// if we have created a turn
};


/////////////////////////////////////////////////////////////////////////////
// CPlayer - a player in the game

class CPlayer	: public CObject
{
friend class CGame;
public:
			CPlayer () { _ctor (); }
			CPlayer (char const *pName, int iNetNum);
			~CPlayer ();

			void					Init (char const *pName, int iNet);
			void					Close ();

			BOOL					IsAI () const { ASSERT_STRICT_VALID (this); return ((BOOL) m_bAI); }
			BOOL					IsLocal () const { ASSERT_STRICT_VALID (this); return ((BOOL) m_bLocal); }
			BOOL					IsMe () const { ASSERT_STRICT_VALID (this); return ((BOOL) m_bMe); }
			void					SetAI (BYTE bAI);
			void					SetLocal (BYTE bLocal);

			DWORD					GetPalColor () const { return (m_clrPlyr); }
			COLORREF			GetRGBColor () const { return (m_rgbPlyr); }
			void					SetColor (COLORREF clr);

			BYTE					GetState () const { ASSERT_STRICT_VALID (this); return (m_bState); }
			void					SetState (BYTE bState) { ASSERT_STRICT_VALID (this); m_bState = bState; }
								enum { created, ready, wait, replace, load_file, load_pick, dead };

			void					SetAiHdl (DWORD dwHdl);
			DWORD					GetAiHdl () const { ASSERT_STRICT_VALID (this); return (m_dwAiHdl); }
			void					SetName (char const *pStr) { ASSERT_STRICT_VALID (this); m_sName = pStr; }
			char const *	GetName () const { ASSERT_STRICT_VALID (this); return (m_sName); }
			void					SetNetNum (VPPLAYERID iNum) { ASSERT_STRICT_VALID (this); m_iNetNum = iNum; }
			VPPLAYERID		GetNetNum () const { ASSERT_STRICT_VALID (this); return (m_iNetNum); }
			void					SetPlyrNum (int iNum);
			int						GetPlyrNum () const { ASSERT_STRICT_VALID (this); return (m_iPlyrNum); }

			void					StartGame ();
			void					StartLoop ();
			void					PeopleAndFood (int iNumSec);
			void					Research (int iNumSec);

			void					UpdateRacialAttributes ( int iRsrch );

			// used during initialization only
			CInitData			m_InitData;

			void					SetAttrib (int iInd, int iVal);
			int						GetAttrib (int iInd) const;

			void					UpdateRemote ( CNetSaveInfo * pMsg );

			// productivity stuff
			int					GetPwrNeed () const { ASSERT_STRICT_VALID (this); return (m_iPwrNeed); }
			int					GetPwrHave () const { ASSERT_STRICT_VALID (this); return (m_iPwrHave); }

			// people are divided into those in vehicles (always fully staffed) and those
			// in buildings (can be partially staffed). Excess people go into the building
			// count and when a vehicle is built people are pulled from the building count
			// to staff the vehicle. We do insure that there is always at least 1 building
			// person to avoid divide overflows.
			int					GetPplNeedBldg () const { ASSERT_STRICT_VALID (this); return (m_iPplNeedBldg); }
			int					GetPplBldg () const { ASSERT_STRICT_VALID (this); return (m_iPplBldg); }
			int					GetPplVeh () const { ASSERT_STRICT_VALID (this); return (m_iPplVeh); }
			int					GetPplTotal () const { ASSERT_STRICT_VALID (this); return (m_iPplBldg + m_iPplVeh); }
			void				AddPplNeedBldg (int iAdd) { ASSERT_STRICT_VALID (this); m_iPplNeedBldg += iAdd; }
			void				AddPplBldg (int iAdd) { ASSERT_STRICT_VALID (this); m_iPplBldg += iAdd; }
			void				AddPplVeh (int iAdd) { ASSERT_STRICT_VALID (this); m_iPplVeh += iAdd; }
			void				PplBldgToVeh (int iAdd) { ASSERT_STRICT_VALID (this); m_iPplBldg -= iAdd; m_iPplBldg = __max (1, m_iPplBldg); m_iPplVeh += iAdd; }
			void				_SetPplBldg (int iNum) { ASSERT_STRICT_VALID (this); m_iPplBldg = iNum; }

			int					GetFood () const { ASSERT_STRICT_VALID (this); return (m_iFood); }
			void				AddFood (int iNum);
			void				AddFoodProd (int iNum) { ASSERT_STRICT_VALID (this); m_iFoodProd += iNum; }
			int					GetFoodProd () const { ASSERT_STRICT_VALID (this); return (m_iFoodProd); }
			int					GetFoodNeed () const { ASSERT_STRICT_VALID (this); return (m_iFoodNeed); }

			int					GetGasHave () const { ASSERT_STRICT_VALID (this); return (m_iGas); }
			int					GetGasNeed () const { ASSERT_STRICT_VALID (this); return (m_iGasNeed); }
			void				AddGas (int iNum);
			void				AddPwrNeed (int iAdd) { ASSERT_STRICT_VALID (this); m_iPwrNeed += iAdd; }
			void				AddPwrHave (int iAdd) { ASSERT_STRICT_VALID (this); m_iPwrHave += iAdd; }
			void				FuelVehicle () { ASSERT_STRICT_VALID (this); m_iGasUsed++; }
			BOOL				BuildRoad ();
			float				GetPplMult () const { ASSERT_STRICT_VALID (this); return (m_fPplMult); }
			float				GetPwrMult () const { ASSERT_STRICT_VALID (this); return (m_fPwrMult); }
			float				GetConstProd () const { ASSERT_STRICT_VALID (this); return (m_fConstProd); }
			float				GetMtrlsProd () const { ASSERT_STRICT_VALID (this); return (m_fMtrlsProd); }
			float				GetManfProd () const { ASSERT_STRICT_VALID (this); return (m_fManfProd); }
			float				GetMineProd () const { ASSERT_STRICT_VALID (this); return (m_fMineProd); }
			float				GetFarmProd () const { ASSERT_STRICT_VALID (this); return (m_fFarmProd); }
			float				GetPopGrowth () const { ASSERT_STRICT_VALID (this); return (m_fPopGrowth); }
			float				GetPopDeath () const { ASSERT_STRICT_VALID (this); return (m_fPopDeath); }
			float				GetEatingRate () const { ASSERT_STRICT_VALID (this); return (m_fEatingRate); }
			float				GetRsrchMult () const { ASSERT_STRICT_VALID (this); return (m_fRsrchProd); }
			float				GetAttackMult () const { ASSERT_STRICT_VALID (this); return (m_fAttack); }
			float				GetDefenseMult () const { ASSERT_STRICT_VALID (this); return (m_fDefense); }

			// first two my relations with them. Second two their relations with me
			int					GetRelations () const { return (m_iRelations); }
			void				SetRelations (int iVal);
			int					GetTheirRelations () const { return m_iTheirRelations; }
			void				SetTheirRelations (int iVal);

			int					GetBldgsBuilt () const { return m_iBldgsBuilt; }
			void				IncBldgsBuilt () { m_iBldgsBuilt++; }
			int					GetVehsBuilt () const { return m_iVehsBuilt; }
			void				IncVehsBuilt () { m_iVehsBuilt++; }
			void				_DecVehsBuilt () { m_iVehsBuilt--; }	// used in startup for vehicles on rocket
			int					GetMaterialMade (int iInd) const 
																	{ ASSERT ((0 <= iInd) && (iInd < CMaterialTypes::num_types)); 
																		return m_aiMade[iInd]; }
			void				IncMaterialMade (int iInd, int iAmt) 
																	{ ASSERT ((0 <= iInd) && (iInd < CMaterialTypes::num_types)); 
																		ASSERT (iAmt >= 0);
																		ASSERT (m_aiMade[iInd] < INT_MAX - iAmt - 16);
																		m_aiMade[iInd] += iAmt; }
			int					GetMaterialHave (int iInd) const 
																	{ ASSERT ((0 <= iInd) && (iInd < CMaterialTypes::num_types)); 
																		return m_aiHave[iInd]; }
			void				IncMaterialHave (int iInd, int iAmt) 
																	{ ASSERT ((0 <= iInd) && (iInd < CMaterialTypes::num_types)); 
																		ASSERT (((iAmt > 0) && (m_aiMade[iInd] < INT_MAX - iAmt - 16)) ||
																						((iAmt <= 0) && (m_aiHave[iInd]-iAmt >= 0)));
																		m_aiHave[iInd] += iAmt; }
			int					GetBldgsDest () const { return m_iBldgsDest; }
			void				IncBldgsDest () { m_iBldgsDest++; }
			int					GetVehsDest () const { return m_iVehsDest; }
			void				IncVehsDest () { m_iVehsDest++; }

			int					GetBldgsHave () const { return m_iBldgsHave; }
			void				AddBldgsHave (int iNum) { m_iBldgsHave += iNum; }
			int					GetVehsHave () const { return m_iVehsHave; }
			void				AddVehsHave (int iNum) { m_iVehsHave += iNum; }

			int					GetRsrchHave () const { return (m_iRsrchHave); }
			int					GetRsrchItem () const { return (m_iRsrchItem); }
			void				AddRsrch (int iNum) { m_iRsrchHave += iNum; }
			void				SetRsrchItem (int iItem) { m_iRsrchItem = iItem; }
			int					GetExists (int iIndex) const { return m_piBldgExists[iIndex]; }
			void				AddExists (int iIndex, int iNum) { m_piBldgExists [iIndex] += iNum; }
			CRsrchStatus & GetRsrch (int iInd) { return (m_aRsrch.ElementAt (iInd)); }
			BOOL				CanRsrch (int iIndex);

			BOOL				CanBridge () { return (GetRsrch (CRsrchArray::bridge).m_bDiscovered); }
			BOOL				CanMultiArea () { return (GetRsrch (CRsrchArray::radio).m_bDiscovered); }
			BOOL				CanDelayMail () { return (GetRsrch (CRsrchArray::mail).m_bDiscovered); }
			BOOL				CanEMail () { return (GetRsrch (CRsrchArray::email).m_bDiscovered); }
			BOOL				CanChat () { return (GetRsrch (CRsrchArray::telephone).m_bDiscovered); }
			BOOL				CanCopper () { return (GetRsrch (CRsrchArray::copper).m_bDiscovered); }

			int					GetSpottingLevel () const { return (m_bSpotting); }
			int					GetRangeLevel () const { return (m_bRange); }
			int					GetAttackLevel () const { return (m_bAttack); }
			int					GetDefenseLevel () const { return (m_bDefense); }
			int					GetAccuracyLevel () const { return (m_bAccuracy); }

			void 				Serialize(CArchive& ar);

			AI_INIT 		ai;						// this is here cause we don't know when the AI
																// stops using it and for its size - big deal
			CHexCoord		m_hexMapStart;// used during initialization only

			int					m_iPerInit;		// percentage inited for startup

			int					GetHousingCap ( int iVal ) const { return ( m_bAI ? iVal * 10 : iVal ); }
			LONG				m_iAptCap;			// built apartment capacity
			LONG				m_iOfcCap;			// built office capacity
			LONG				m_iNumCranes;		// number of cranes we have
			LONG				m_iNumTrucks;		// number of trucks we have

			DWORD				m_dwIDRocket;		// our rocket

			int					m_iGameSpeed;		// speed this net player wants
			int					m_iNumDiscovered;

			int					m_iNumAiGpfs;		// how many times this AI player has GPFed

			CVPTransfer *	m_pXferToClient;	// from server to this player

			BOOL				m_bPauseMsgs;

			BOOL				m_bMsgDead;			// displayed message it is dead

			int					m_iBuiltBldgsHave;
			BOOL				m_bPlacedRocket;

protected:
			BOOL				BuildCcBldg (int iBldg);
			BOOL				InitialRoad ();
			BOOL				TryRoad (CCitCon * pCc);
			BOOL				BuildIt (CHexCoord const & hexTry, CCitCon * pCc);
			BOOL				CanBuild (CHexCoord const & hexTry, CCitCon * pCc);

			COLORREF		m_rgbPlyr;		// color to show this guy as
			DWORD				m_clrPlyr;		// bitmap value (BYTE - DWORD, uses low bits)

			// tracking power & people (this is global per player)
			LONG				m_iPwrNeed;		// power needed by all buildings
			LONG				m_iPwrHave;		// power presently generated
			LONG				m_iPplNeedBldg;		// people needed by all buildings
			LONG				m_iPplBldg;		// people presently have EXCEPT in vehicles
			LONG				m_iPplVeh;		// people in vehicles (Have+Veh == Total)
			LONG				m_iFood;			// food on hand
			LONG				m_iFoodProd;	// food produced in 1 minute
			LONG				m_iFoodNeed;	// food eaten in 1 minute
			LONG				m_iGas;				// gas on hand
			LONG				m_iGasUsed;		// gas being burnt by travelling vehicles
			LONG				m_iGasNeed;		// gas needed for the next minute
			LONG				m_iGasTurn;		// turns to use some gas
			float				m_fPplMult;		// __min (1.0, need/have)
			float				m_fPwrMult;

			LONG				m_iRsrchHave;	// people working on R&D (reduced by damage)
			LONG				m_iRsrchItem;	// item we are researching
			CArray <CRsrchStatus, CRsrchStatus *>	m_aRsrch;
			LONG *			m_piBldgExists;// bldg presently exists

			// attributes that I need as floats
			float				m_fConstProd;	// construction productivity
			float				m_fMtrlsProd;	// create materials productivity
			float				m_fManfProd;	// create vehicles productivity
			float				m_fMineProd;	// mining productivity
			float				m_fFarmProd;	// farming productivity
			float				m_fPopGrowth;	// population growth rate per second
			float				m_fPopDeath;	// population death rate from overpop & no food
			float				m_fEatingRate;// rate eats food at
			float				m_fPopMod;		// for handling remainder on pop growth
			float				m_fFoodMod;		// for handling remainder on food consumption
			float				m_fRsrchProd;	// research productivity
			float				m_fAttack;		// attack multiplier
			float				m_fDefense;		// defense multiplier

			// relations the human player on this machine has with this player
			LONG				m_iRelations;
			int					m_iTheirRelations;	// their relations with us

			// this tracks the players productivity for when the game ends
			LONG				m_iBldgsBuilt;
			LONG				m_iVehsBuilt;
			LONG				m_aiMade [CMaterialTypes::num_types];	// materials made
			LONG				m_aiHave [CMaterialTypes::num_types];	// materials on hand
			LONG				m_iBldgsDest;	// destroyed
			LONG				m_iVehsDest;

			// you're dead when you have no buildings (start at 1 & don't inc for rocket)
			LONG				m_iBldgsHave;
			LONG				m_iVehsHave;

			void				ctor ();			// reset (close)
			void				_ctor ();			// init everything

			CString			m_sName;			// name of player
			DWORD				m_dwAiHdl;		// AI internal handle
			VPPLAYERID	m_iNetNum;		// net connection to that player
			LONG				m_iPlyrNum;		// player number (0 == self, 1 == server)

			BYTE				m_bState;			// ready to play (used by create)

			BYTE				m_bAI;				// is an AI player
			BYTE				m_bLocal;			// is on this machine
			BYTE				m_bMe;				// is human on this machine

			BYTE				m_bSpotting;	// spotting ability
			BYTE				m_bRange;			// range ability
			BYTE				m_bAttack;		// attack ability
			BYTE				m_bDefense;		// defense ability
			BYTE				m_bAccuracy;	// accuracy ability

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CGame - a game

class CGame : public CObject
{
friend CDlgFile;
friend class CConquerApp;
#ifdef _DEBUG
friend class CPlayer;
friend class CMsgVehGoto;
#endif
public:
			CGame () { _ctor (); }
			~CGame () { Close (); }
			void				ctor ();

			void				Open (BOOL bLocal);
			void				Close ();
			CPlayer *		_GetPlayer (int iNetNum) const;
			CPlayer *		GetPlayer (int iNetNum) const;
			CPlayer *		_GetPlayerByPlyr (int iPlyrNum) const;
			CPlayer *		GetPlayerByPlyr (int iPlyrNum) const;
			BOOL				IsAllReady () const;

			void				AddPlayer (CPlayer *pPlr);
			void				AddAiPlayer (CPlayer *pPlr);
			void				AiTakeOverPlayer (CPlayer *pPlr, BOOL bStartThread, BOOL bShowDlg = TRUE);
			void				AiReleasePlayer (CPlayer *pPlr, int iNetNum, const char * pName, BOOL bLocal, BOOL bPlaying);
			void				DeletePlayer (CPlayer *pPlr);
			void				DeleteAll ();
			void				RemovePlayer (CPlayer * pPlr);
			void				AiPlayerIsDead (CPlayer *pPlr);

			// starting the game
			void				StartAllPlayers () const;
			void				StartNewWorld (unsigned uRand, int iSide, int iSideSize);
			void				LoadToPlyr ( CPlayer * pPlrLoad, CPlayer * pPlrAll );

			// running the game
			void				SetGameMul (int iSpeed);
			int					GetGameMul () const;
			void				SetSeed (unsigned uSeed) { m_uSeed = uSeed; }

			// sending messages
			VPPLAYERID	GetMyNetNum () const 
													{ if (m_pMe != NULL) return (m_pMe->GetNetNum ());
														if (m_bServer) return (VP_SESSIONSERVER);
														ASSERT (FALSE);
														return (VP_LOCALMACHINE); }
			VPPLAYERID	GetServerNetNum () const 
													{ if (m_pServer != NULL) return (m_pServer->GetNetNum ());
														return (VP_SESSIONSERVER); }
			void				AddToQueue (CNetCmd const * pCmd, int iLen);
			void				FreeQueueElem ( CNetCmd * pCmd );
			void				EmptyQueue ();
			void				ProcessMsg (CNetCmd * pCmd);
			void				SendToServer (CNetCmd const * pMsg, int iLen);
			void				PostToServer (CNetCmd const * pMsg, int iLen);
			void				PostToAll (CNetCmd const * pMsg, int iLen, BOOL bAI = TRUE);
			void				PostToAllClients (CNetCmd const * pMsg, int iLen, BOOL bAI = TRUE);
			void				PostToAllAi (CNetCmd const * pMsg, int iLen);
			void				PostToClient ( CPlayer * pPlr, CNetCmd const * pMsg, int iLen);
			void				PostToClient ( int iPlyr, CNetCmd const * pMsg, int iLen);
			void				PostToClientByNet ( int iPlyr, CNetCmd const * pMsg, int iLen);
			void				PostClientToClient ( CPlayer * pPlr, CNetCmd const * pMsg, int iLen);
			void				ProcessAllMessages ();

			// events
			void				Event (int ID, int iTyp, CUnit *pUnit);
			void				Event (int ID, int iTyp, CPlayer * pPlayer);
			void				Event (int ID, int iTyp, int iVal);
			void				Event (int ID, int iTyp);
			void				_Event (int ID, int iTyp, char const * pText, int iVoice);
			void				MulEvent (int ID, CUnit * pUnit);

			int					GetSideSize () const { return (m_iSideSize); }
			void				SetSideSize (int iSideSize) { m_iSideSize = iSideSize; }
			int					GetScenario () const { return (m_iScenarioNum); }
			void				IncScenario () { m_iScenarioNum++; }
			void				SetScenario (int iNum) { m_iScenarioNum = iNum; }
			int					GetMaxPlyrNum () const { return (m_iNextPlyrNum); }
			void				_SetMaxPlyrNum ( int iNum ) { m_iNextPlyrNum = iNum; }

			BOOL				IsNetGame () const { return m_bIsNetGame; }
			BOOL				AmServer () const { ASSERT_STRICT_VALID (this); return (m_bServer); }
			void				SetServer (BOOL bSrvr) { ASSERT_STRICT_VALID (this); m_bServer = bSrvr; }
			void				_SetIsNetGame ( BOOL bIsNet ) { m_bIsNetGame = bIsNet; }
			BOOL				HaveHP () const { ASSERT_STRICT_VALID (this); return (m_bHP); }
			void				SetHP (BOOL bSrvr) { ASSERT_STRICT_VALID (this); m_bHP = bSrvr; }
			BOOL				HaveAI () const { ASSERT_STRICT_VALID (this); return (m_bAI); }
			void				SetAI (BOOL bSrvr) { ASSERT_STRICT_VALID (this); m_bAI = bSrvr; }

			BOOL				AreMsgsPaused () const { return m_bPauseMsgs; }
			BOOL				CheckAreMsgsPaused () { if ( ! m_bPauseMsgs ) return FALSE;
																					if ( ! m_bUnPauseMe) return TRUE;
																					m_bPauseMsgs = FALSE;
																					return FALSE; }
			void				SetMsgsPaused ( BOOL bPause );
			void				PauseTimerFired () { m_uTimer = 0; }

			BOOL				IsToldPause () const { return m_bToldPause; }
			void				SetToldPause () { m_bToldPause = TRUE; }
			void				ClrToldPause () { m_bToldPause = FALSE; }
			BOOL				IsNetPause () const { return m_bNetPause; }
			void				SetNetPause () { m_bNetPause = TRUE; }
			void				ClrNetPause () { m_bNetPause = FALSE; }

			BOOL				DoMsgs () const { ASSERT_STRICT_VALID (this); return (m_bMessages); }
			void				SetMsgs (BOOL bMsgs) { ASSERT_STRICT_VALID (this); m_bMessages = bMsgs; }

			BOOL				DoAnim () const { ASSERT_STRICT_VALID (this); return (m_bAnimate); }
			BOOL				DoOper () const { ASSERT_STRICT_VALID (this); return (m_bOperate); }
			void				SetAnim (BOOL bAnimate) 
																{ ASSERT_STRICT_VALID (this); 
																	if ( ! m_bHP )
																		return;
																	if ((bAnimate) && (! m_bAnimate))
																		m_dwFrameTimeLast = timeGetTime ();
																	m_bAnimate = bAnimate; }
			void				SetOper (BOOL bOperate)
																{ ASSERT_STRICT_VALID (this); 
																	if ((bOperate) && (! m_bOperate))
																		m_dwOperTimeLast = timeGetTime ();
																	m_bOperate = bOperate; }

			DWORD				GetFramesElapsed () const { return (m_dwFramesElapsed); }
			DWORD				GetOpersElapsed () const { return (m_dwOpersElapsed); }
			DWORD				GetOperSecElapsed () const { return (m_dwOperSecElapsed); }
			DWORD				GettimeGetTime () const { return m_dwtimeGetTime; }
			void				_SettimeGetTime () { m_dwtimeGetTime = timeGetTime (); }

			DWORD				GetElapsedSeconds () const { return ( ( m_dwElapsedTime >> 4 ) / 24 ); }
			void				SetElapsedSeconds ( int iSec ) { m_dwElapsedTime = ( iSec << 4 ) * 24; }
			DWORD				GetFrame () const { return (m_dwFrame); }

			void				CheckAlliances ();
			BOOL				AnyAlliances () const { return m_bHaveAlliances; }

			int					GetState () const { return (m_iState); }
			void				SetState (int iState) { m_iState = iState; }
							enum { main, open, wait_start, init, wait_AI, init_AI, AI_done, play, close, save, error, other };
			void				SetNetJoin (int iVal) { m_iNetJoin = iVal; }
			int					GetNetJoin () const { return (m_iNetJoin); }
							enum { create = 0, approve = 1, any = 2 };
			void				IncTry () { m_iTryCount++; }
			void				DecTry () { m_iTryCount--; }
			int					GetTry () const { return (m_iTryCount); }

			DWORD				GetID () { return (m_dwNextUnitID++); }

			// players
			CPlayer *		GetMe () { ASSERT_STRICT_VALID (this); ASSERT (m_pMe != NULL); return (m_pMe); }
			CPlayer *		_GetMe () { ASSERT_STRICT_VALID (this); return (m_pMe); }
			CPlayer *		GetServer () { ASSERT (! m_bServer); ASSERT_STRICT_VALID (this); ASSERT (m_pServer != NULL); return (m_pServer); }
			void				_SetMe (CPlayer * pPlyr) { m_pMe = pPlyr; }
			void				_SetServer (CPlayer * pPlyr) { m_pServer = pPlyr; }

			CList <CPlayer *, CPlayer *> & GetAi () { ASSERT_STRICT_VALID (this); return (m_lstAi); }
			CList <CPlayer *, CPlayer *> & GetAll () { ASSERT_STRICT_VALID (this); return (m_lstAll); }

			CList <CPlayer *, CPlayer *> m_lstDead;		// dead players

			CList <CPlayer *, CPlayer *> m_lstLoad;		// loading a multi-player game
			int					m_iSavedPlyrNum;	// player we saved as (for spotting)

#ifdef _DEBUG
			DWORD				GetNextID () { return (m_dwNextUnitID); }
#endif

			void 				Serialize(CArchive& ar);
			int					LoadGame (CWnd * pPar, BOOL bReplace);
			int					StartGame (BOOL bReplace);
			int					SaveGame (CWnd * pPar);


			CPtrList		m_lstMsgs;			// posted messages
			MEM_POOL		m_memPoolLarge;
			MEM_POOL		m_memPoolSmall;

			CString			m_sFileName;		// file name of the game (if loaded or saved)
			DWORD				m_dwFinalRand;	// final seed at end of init

			// what the game was started at (for loading descriptions)
			LONG					m_iAi;				// AI intelligence
			LONG					m_iSize;			// world size
			LONG					m_iPos;				// initial position
			CString				m_sGameName;					// for create_net
			CString				m_sGameDesc;

			CHPRouter *		m_pHpRtr;

			LONG					m_iScenarioVar;				// used for scenarios
			DWORD					m_adwScenarioUnits[5];

			// windpow positions of the human player on this system
			CHexCoord					m_hexAreaCenter;
			WINDOWPLACEMENT		m_wpArea;
			WINDOWPLACEMENT		m_wpWorld;
			WINDOWPLACEMENT		m_wpChat;
			WINDOWPLACEMENT		m_wpBldgs;
			WINDOWPLACEMENT		m_wpVehicles;
			WINDOWPLACEMENT		m_wpRelations;
			WINDOWPLACEMENT		m_wpFile;
			WINDOWPLACEMENT		m_wpRsrch;

			#ifdef _DEBUG
			BOOL	m_bUpdateFlag;
			#endif

			DWORD 				m_dwMaj;
			DWORD 				m_dwMin;
			DWORD 				m_dwVer;
			WORD 					m_wDbg;
			WORD 					m_wCht;

			// for joining a game (load/in progress)
			CVPTransfer *	m_pXferFromServer;	// from server to me
			void				* m_pGameFile;				// game file we are sending/receiving
			int						m_iGameBufLen;			// how big it is
			int						m_iNumSends;				// how many we are sending to

			CString				m_sPwJoin;					// password for joining a game

			CMsgShoot			m_msgShoot;					// batch up shoot commands

protected:
			void				_ctor ();
			void				CloseAll ();
			void				ReadWP ( CArchive & ar, WINDOWPLACEMENT & wp, int iMode );

			CPlayer *		m_pMe;
			CPlayer *		m_pServer;
			CList <CPlayer *, CPlayer *> m_lstAi;				// the AI players (only on server)
			CList <CPlayer *, CPlayer *> m_lstAll;			// includes me too

			LONG				m_iNextPlyrNum;	// PlyrNum for next player
			LONG				m_iNextAINum;
			DWORD				m_dwNextUnitID;	// unique ID for each unit

			BOOL				m_bIsNetGame;		// TRUE if a net game
			BOOL				m_bServer;			// TRUE if I'm the server
			BOOL				m_bHP;					// TRUE if have a Human Player (on server only)
			BOOL				m_bAI;					// TRUE if have AI players (on server only)

			BOOL				m_bPauseMsgs;		// don't post high volume messages
			BOOL				m_bToldPause;		// told others to not post high-volume to me
			BOOL				m_bNetPause;		// net told me to stop posting
			BOOL				m_bUnPauseMe;		// unpause me next oppo
			UINT				m_uTimer;

			BOOL				m_bMessages;		// process messages
			BOOL				m_bAnimate;			// animate the screen
			DWORD				m_dwFrameTimeLast;		// timeGetTime of last render loop
			DWORD				m_dwFramesElapsed;// frames (1/24) elapsed since last loop

			BOOL				m_bOperate;			// operate the economy/games
			DWORD				m_dwOperTimeLast;		// timeGetTime of last oper loop
			DWORD				m_dwOpersElapsed;// operations elapsed since last loop
			DWORD				m_dwOperSecElapsed;	// seconds elapsed since last loop
			DWORD				m_dwOperSecFrames;	// frames towards next 1 sec loop

			DWORD				m_dwElapsedTime;	// time playing in 24ths of a sec (stops when paused)
			DWORD				m_dwFrame;			// GG: frames since game start,
														//     for calculating ambient frames, animating building cursor, etc.
			DWORD				m_dwtimeGetTime;	// call timeGetTime once a frame

			int					m_iState;				// game state
			int					m_iNetJoin;			// when can join game
			int					m_iTryCount;		// how many try's deep

			BOOL				m_bHaveAlliances;		// TRUE if have alliances with anyone

			LONG				m_iSideSize;		// number of hexes along 1 side of a map block
			LONG				m_iScenarioNum;	// -1 if not a scenario
			LONG				m_iSpeedMul;

			DWORD				m_uSeed;				// rand seed

			CMapLoc			m_maploc;				// area window for save/restore
			int					m_iDir;
			int					m_iZoom;

			LONG				m_xScreen;			// screen resolution saved at
			LONG				m_yScreen;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

extern void SerializeElements (CArchive & ar, CPlayer * * pPlyr, int nCount);

extern CGame theGame;


#endif
