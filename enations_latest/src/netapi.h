//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __NETAPI_H__
#define __NETAPI_H__

#include "vdmplay.h"

const int iMaxNameLen = VP_MAXPLAYERDATA;
const VPGUID tlpGUID = { "F9FC4F00843C11cfB82000AA0047F4" };


class CCreateBase;
class CDlgCreate;
class CPlayer;
class CNetPublish;
class CNetJoin;
class CGame;


class CNetApi : public CObject
{
public:
		CNetApi ();
		~CNetApi( );
    

static		BOOL		SupportsProtocol (int iPrtcl) 
									{ return (vpSupportedTransports () & (1 << iPrtcl)); }
static		int			GetVersion () { return (vpGetVersion ()); }
static		LRESULT OnNetMsg (WPARAM wParam, LPARAM lParam);
static		void OnNetFlowOn ();
static		void OnNetFlowOff ();

		std::string	GetIServeAddress ();

		BOOL		OpenServer (int iPrtcl, HWND hWnd, char const *pName, void const * pData, void const * pPrtcl);
		BOOL		UpdateSessionData (void const * pData) { return (! vpUpdateSessionData (m_vpSession, pData)); }
		BOOL		OpenClient (int iPrtcl, HWND hWnd, void const * pPrtcl);
		std::string	GetAddress () const ;

		BOOL		StartEnum () { return (! vpEnumSessions (m_vpHdl, m_hWnd, TRUE, NULL)); }
		BOOL		StopEnum () { return (! vpStopEnumSessions (m_vpHdl)); }
		BOOL		SetSessionVisibility (BOOL bVis) { return (! vpSetSessionVisibility (m_vpSession, bVis)); }

		BOOL		Join (LPCVPSESSIONID id, CNetJoin const * pJn);
		VPPLAYERID	AddPlayer (CNetJoin const * pJn);

		void		DeletePlayer (VPPLAYERID idTo);
		void		SessionClose ();
		void		CloseSession ( BOOL bDelayClose );
		void		Close ( BOOL bDelayClose );

		BOOL		Send (VPPLAYERID idTo, LPCVPMSGHDR pData, int iLen);
		BOOL		Broadcast (LPCVPMSGHDR pData, int iLen, BOOL bLocal);

		int			GetMode () const { return (m_iMode); }
		int			GetType () const { return (m_iType); }

		// Re-assert client mode on the discovered-session Join path. OpenClient sets
		// m_iType=client, but the SDL2 Join dialog runs theGame.ctor()/Open() between the
		// browse-open and Join, which re-defaults theNet's m_iType to closed (the vp handle
		// survives — linux2 datapoint 2026-06-26). Call this right before Join() so the
		// :178 ASSERT(m_iType==client) holds without weakening it. (Win MP join via :160
		// is unaffected; calling this when already client is a harmless no-op.)
		void		SetClientType () { m_iType = client; }

		void		SetMode (int iMode) { ASSERT ((0 <= iMode) && (iMode <= num_modes)); m_iMode = iMode; }
						enum		{ closed,		// totally closed down
											opened,		// connected to net, no session
											joined,		// have joined a game
											starting,	// building a world
											playing,	// playing a game
											ending,		// ending a game
											num_modes };
											// for GetType ()
											enum { // closed,
														 server = 1,
														 client = 2 };

		VPSESSIONHANDLE		_GetSessionHandle () const { return m_vpSession; }

		// Host relay (docs/plans/host-relay-spec.md 8): is our link to that
		// player relayed through the host? Read-only. ProbePeerLink kicks the
		// one-shot direct dial that makes the answer meaningful in the lobby.
		// Both are engine-side no-ops while the relay gate is off.
		BOOL		PeerIsRelayed (VPPLAYERID id) const { return (vpPeerIsRelayed (m_vpSession, id)); }
		void		ProbePeerLink (VPPLAYERID id) { vpProbePeerLink (m_vpSession, id); }

private:
		VPHANDLE					m_vpHdl;
		VPSESSIONHANDLE		m_vpSession;
		HWND							m_hWnd;
		int								m_iMode;				// connection mode
		int								m_iType;				// client or server

		char							m_cFlags;
							enum { closeSession = 0x01, cleanup = 0x02 };
};

class CNetPublish {		// announce a game
public:
static	CNetPublish * Alloc (CCreateBase * pCm);
static	CNetPublish * Alloc (CGame * pG);

		int				m_iLen;								// how long this message is
		int				m_iNumOpponents;
		int				m_iAIlevel;
		int				m_iWorldSize;
		int				m_iPos;
		int				m_iNumPlayers;				// for loading/in-process games - how many total positions there are

		int				m_iGameID;						// 0x01 for tLP
		WORD			m_cVerMajor;					// same version, etc.
		WORD			m_cVerMinor;
		WORD			m_cVerRelease;
		char			m_cFlags;
							enum { fdebug = 0x01, fcheat = 0x02, fload = 0x04, finprogress = 0x08 };

		char			m_sPlyrName[1];	// server's player name
		//char		m_sPw[];				// game password
		//char		m_sGameName[];	// game name
		//char		m_sDesc[];			// game desc

		char const *	GetPlyrName () const { return (m_sPlyrName); }
		char const *	GetPw () const { return (m_sPlyrName + strlen (m_sPlyrName) + 1); }
		char const *	GetGameName () const { return (GetPw () + strlen (GetPw ()) + 1); }
		char const *	GetDesc () const { return (GetGameName () + strlen (GetGameName ()) + 1); }
};

class CNetJoin {		// join a session
public:
static	CNetJoin * Alloc (CPlayer const *pPlyr, BOOL bSrvr);

		int				m_iLen;								// how long this message is
		int				m_iPlyrNum;
		BOOL			m_bServer;
		char			m_sName[1];
		// note - we actually have the full name
};


extern CNetApi theNet;

#endif
