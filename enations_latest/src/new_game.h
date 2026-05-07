//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __NEW_GAME_H__
#define __NEW_GAME_H__

#include "resource.h"
#include "netapi.h"
#include "netcmd.h"
#include "racedata.h"
#include "chat.h"

class CCreateNewBase;
class CCreateLoadBase;
class CCreateBase;
class SDL2CreateStatus;


const int NUM_PROTOCOLS = 7;
const int aPr[NUM_PROTOCOLS] = {
						VPT_TCP,
						VPT_IPX,
						VPT_NETBIOS,
						VPT_DP,
						VPT_MODEM,
						VPT_TAPI,
						VPT_COMM };

// CDlgCreateStatus removed (replaced by SDL2CreateStatus)
// Forward decl so CCreateBase can hold SDL2CreateStatus*
class SDL2CreateStatus;


/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerList removed (Phase 2d) — was the multiplayer pre-game waiting
// room. The SDL2 multiplayer flow drives this state directly from
// SDL2_RunCreateNetworkFlow / SDL2_RunJoinNetworkFlow.


/////////////////////////////////////////////////////////////////////////////
// CCreateBase

class CCreateBase : public CObject
{
public:
		enum { scenario,
						single,
						create_net,
						join_net,
						load_single,
						load_multi,
						load_join,
						num_types };

		CCreateBase (int iTyp);
		virtual ~CCreateBase () { CloseAll (); }
		virtual void RemovePlayer (CPlayer *) {}
		virtual void UpdatePlyrStatus (CPlayer *, int) {}
		void	ToWorld ();
		virtual void AddPlayer (CPlayer *) {}
		virtual void	OnSessionEnum (LPCVPSESSIONINFO) {}
		virtual void	OnSessionClose (LPCVPSESSIONINFO) {}

		SDL2CreateStatus * GetDlgStatus () { ASSERT (m_psdlStatus != NULL); return (m_psdlStatus); }
		void		CreateDlgStatus ();
		void		ShowDlgStatus ();
		void		HideDlgStatus ();

		virtual void	Init () {}
		virtual void  ClosePick () {}
		virtual void  CloseAll ();
		virtual void  UpdateBtns () {}

		virtual CCreateNewBase *	GetNew () { ASSERT (FALSE); return (NULL); }
		virtual CCreateLoadBase *	GetLoad () { ASSERT (FALSE); return (NULL); }

		int						m_iTyp;
		int						m_iNet;				// net type (-1 -> no net)
		VPSESSIONID		m_ID;
		int						m_iAi;				// AI intelligence
		int						m_iNumAi;			// num AI players
		int						m_iSize;			// world size
		int						m_iPos;				// initial position
		int						m_iJoinUntil;	// can join until
		int						m_iNumPlayers;				// for loading/in-process games - how many total positions there are

		void *				m_pAdvNet;		// advanced net data

		std::string			m_sName;			// player name
		std::string			m_sRace;			// race name
		std::string			m_sPw;				// password
		std::string			m_sGameName;					// for create_net
		std::string			m_sGameDesc;

		// SDL2 progress dialog (renders on SDL window)
		SDL2CreateStatus * GetSDL2Status() { return m_psdlStatus; }

protected:
		SDL2CreateStatus *	m_psdlStatus = nullptr;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CMultiBase : public CCreateBase
{
public:
		CMultiBase (int iTyp) : CCreateBase (iTyp) {}

		virtual void  ClosePick () {}
		virtual void  CloseAll () { CCreateBase::CloseAll(); }

		void		CreateWndChat ();
		// CDlgPlayerList removed (Phase 2d) — these no-ops keep network handlers
		// (e.g. CmdPlyrJoin / load-game restore in player.cpp) compiling. The
		// SDL2 multiplayer flow renders the player list inside its own dialog.
		void		CreatePlyrList (CCreateBase *) {}
		struct PlyrListStub {
			void RemovePlayer(CPlayer*) {}
			void AddPlayer(CPlayer*) {}
			void UpdatePlyrStatus(CPlayer*, int) {}
			void UpdateBtns() {}
			void RemoveAll() {}
		} m_wndPlyrList;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

// CDlgPickRace + CDlgPickPlayer removed (Phase 2d) — SDL2PickRaceDialog and
// SDL2PickPlayerDialog now drive race / player selection from the SDL2 flows.

class CCreateNewBase
{
public:
		CCreateNewBase () {}

		virtual void  ClosePick ();
		virtual void  CloseAll () { ClosePick (); }

		CInitData					m_InitData;
};

class CCreateLoadBase
{
public:
		CCreateLoadBase () { m_bReplace = FALSE; }

		virtual void  ClosePick ();
		virtual void  CloseAll () { ClosePick (); }

		BOOL								m_bReplace;
};


#endif
