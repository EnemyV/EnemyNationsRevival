//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __UNIT_H__
#define __UNIT_H__

// unit.h : header file for all unit stuff (buildings & vehicles)
//

#include "base.h"
#include "sprite.h"
#include "icons.h"

class CStatInst;
class CWndUnitStat;


class CDlgCreateWorld;
class CHex;
class CHexCoord;
class CWndArea;
class CPlayer;
class CNetAnsTile;
class CMsgBldgMat;
class CMsgBlocked;
class CMsgVehLoc;
class CMsgVehGoto;
class CMsgUnitDamage;
class CMsgLoadCarrier;
class CMsgShoot;
class CMsgShootElem;
class CMsgUnloadCarrier;
class CProjMap;
class CUnit;


// this is so we don't have to keep allocating & sizing CDib on each call
class CUnitShowStat
{
public:
	CUnitShowStat () {}
	void Init ();
	void Close ();

	CStatInst		m_siText;
	CStatInst		m_si[3][3];
};

extern CUnitShowStat uShowStat;

//-----------------------------C F l a m e S p o t --------------------------

class CFlameSpot
{

public:

	enum
	{
		SMOKE_START_PERCENT = 20,
		FLAME_START_PERCENT = 50
	};

	void	SetAge( int iAgePercent );
	CRect	Draw	( CPoint ptHotSpotClient );
	BOOL	IsHit ( CPoint ptHotSpotClient, CPoint ptCursor ) const;

private:
	
	static Ptr< CEffectTile >	  m_fake;

	Ptr< CEffectTile >	m_ptrtileSmoke;
	Ptr< CEffectTile >	m_ptrtileFlame;
};

//-------------------------C D a m a g e D i s p l a y ----------------------
// 
//	Manage smoke/flame sprites

class CDamageDisplay
{

public:

	enum 
	{ 
		LAST_START_PERCENT = 70		// When the last flame hotspot starts
	};

	CDamageDisplay( CSprite const & );
  ~CDamageDisplay();

	void	SetDamage( int iPercent );
	CRect	Draw		( CSpriteView const &, CDrawParms const & );
	BOOL	IsHit 	( CSpriteView const &, CDrawParms const &, CPoint ptCursor ) const;
		 
private:

	int		 	  m_nSpots;
	CFlameSpot	* m_pflamespot;
};

/////////////////////////////////////////////////////////////////////////////
// CUnitData - data about a unit type
//
// Everything in the ~ first half of unit.h is data that describes a type of unit.
// So, for a light truck or coal mine, there is a single instance of the relevant
// below structures regardless of how many of the unit exist. (The structures in
// the second half of unit.h are instance per actual unit).
//
// Therefore, the data in the following structures is data that will NOT change
// between seperate instances of specific units. For example, damage is not tracked
// below because that is different for each instance of a unit. But MaxMaterials
// is tracked below because EVERY instance of a given vehicle type will have the same
// limit.
//
// CUnitData is all elements that are constant for both vehicles and structures.

#ifdef _DEBUG
class CUnitData : public CObject
#else
class CUnitData
#endif
{
#ifdef _DEBUG
friend class CUnit;
friend class CBuilding;
friend class CVehicle;
#endif

public:
		enum { soft, hard, naval, num_attacks };		// target type

							CUnitData ();
		virtual		~CUnitData () {}

						enum { structure, transport, unknown };
			virtual int			GetClassType () { return unknown; }

			int			GetPeople () const;
			int			GetDamagePoints () const;
			int			GetTargetType () const;
			int			GetExpRadius () const { return m_iExpRadius; }
			int			_GetFireRate () const;		// == 0 if can't shoot

						enum UNIT_DATA_FLAGS { FLfixed = 0x02, FLflash1 = 0x04, FLflash2 = 0x08, 
									FLhaveArt = 0x10 };
			UNIT_DATA_FLAGS	GetUnitFlags () const { return (m_udFlags); }
			int			GetSoundIdle () const { return (m_iSoundIdle); }
			int			GetSoundGo () const { return (m_iSoundGo); }
			int			GetSoundRun () const { return (m_iSoundRun); }
			int			GetSoundShoot () const { return (m_iSoundShoot); }
			int			GetProjectile () const { return (m_dwIDProj); }

			int			_GetSpottingRange () const;
			int			_GetRange () const;
			int			_GetAttack ( int iInd ) const { return m_iAttack [iInd]; }
			int			_GetDefense () const { return m_iDefense; }
			int			_GetAccuracy () const;

			CString const & GetDesc () const;
			CString const & GetText () const;
			int			GetCX () const;
			int			GetCY () const;

			void		ReadUnitData (CMmio & mmio);

			int			GetRsrchReq (int iInd) const;
			int			GetScenario () const { return m_iScenario; }

			// R&D can change these
			BOOL		IsDiscovered () const;
			BOOL		PlyrIsDiscovered (CPlayer * pPlyr) const;
			void		IncSpotting ();
			void		IncRange ();
			void		IncAttack ();
			void		IncDefense ();

protected:

			UNIT_DATA_FLAGS m_udFlags;								// see enums
			int			m_iSoundIdle;						// sound this unit makes idling
			int			m_iSoundGo;							// vehicles only - sound when starting
			int			m_iSoundRun;						// sound this unit makes when running
			int			m_iSoundShoot;					// sound when shooting
			int			m_dwIDProj;							// projectile

			// R&D dependency here
			int			m_iRsrchReq[4];					// can't build till this level of R&D reached
			int			m_iScenario;						// can't build till this scenario

			int			m_iPeople;							// people needed to operate it
			int			m_iMaxMaterials;				// how much it can hold
			int			m_iSpottingRange;				// how many hexes away it can see
			int			m_iDamagePoints;				// points of damage required to destroy

			int			m_iRange;								// maximum range it can shoot
			int			m_iAttack [3];					// attack - defense, not affected by defense, naval targets
			int			m_iExpRadius;						// how far away from impact it damages units
			int			m_iTargetType;					// soft, hard, naval
			int			m_iDefense;							// defense against 
			int			m_iFireRate;						// how often it fires
			int			m_iAccuracy;						// how accurate it is

			CString	m_sDesc;								// what its called
			CString	m_sText;								// explanation about it

			int			m_cx;										// size in hexes
			int			m_cy;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// This should have been in CUnitData - but I guessed wrong when first designing
// the reason I did was I was seperating out the sprites for the units from all
// the other data.

template <class T>
class CSpriteStore : public CSpriteCollection
{
public:

	CSpriteStore ( char const * pszRifName ) : CSpriteCollection( pszRifName ) {}

	T * GetSprite( int iID, int iIndex = 0, BOOL bStrict = FALSE ) const
	{ return ( T  * )CSpriteCollection::GetSprite( iID, iIndex, bStrict ); }

	T * GetSpriteByIndex( int iIndex ) const
	{ return ( T  * )CSpriteCollection::GetSpriteByIndex( iIndex ); }
};

//-------------------------------C T u r r e t s -----------------------------
//
// Collection of turrets

class CTurrets : public CSpriteStore< CVehicleSprite >
{

public:

	enum TURRET_TYPE
	{
		light_tank,
		boss8800,
		num_types
	};

	CTurrets( char const * pszRifName );

	void	InitData ();
	void	InitSprites ();

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

private:

};

//-------------------------------C F l a s h e s -----------------------------
//
// Collection of muzzle flashes

class CMuzzleFlashes : public CSpriteStore< CVehicleSprite >
{

public:

	enum MUZZLE_FLASH_TYPE
	{
		small_flash,
		large_flash,
		num_types
	};

	CMuzzleFlashes( char const * pszRifName );

	void	InitData ();
	void	InitSprites ();

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

private:

};

/////////////////////////////////////////////////////////////////////////////
// From here down we are now into the classes that represent a specific unit 
// on the board. So there is 1 instance of the appropiate class below for
// each unit. And in each case, that class points to the appropiate instance
// of the above fixed data class so you can easily access them.
//

//-------------------------------- C T r e e ----------------------------

class CTree : public CEffectTile
{
	public:

		CRect		Draw( const CHexCoord & );
		CPoint	GetDrawCorner( const CHexCoord & ) const;
};

//-------------------------------C E f f e c t -----------------------------
//
// Collection of single-view sprites

class CEffect : public CSpriteStore< CEffectSprite >
{

public:

	enum EFFECT_TYPE
	{
		tree,
		projectile,
		explosion,
		smoke,
		fire,
		flag,
		num_types
	};

	CEffect( char const * pszRifName );
  ~CEffect();

	void	InitData ();
	void	InitSprites ();
	void	Close();

	int		TreeCount() const				{ return m_nTrees; }
	CTree  * GetTree( int iIndex )		{
														ASSERT( 0 <= iIndex && iIndex < m_nTrees );
													  	ASSERT_VALID( m_ptrees + iIndex );
													  	return m_ptrees + iIndex;
													}

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

private:

	int		 m_nTrees;
	CTree 	*m_ptrees;
};


//-------------------------------- C F l a g --------------------------------

class CFlag : public CEffectTile
{

public:

	CFlag();
	CFlag( int iPlayer );

	CRect	Draw( CPoint const & ptHotSpot );

	void	Init( int iPlayer );

	virtual void	Serialize( CArchive & ar );

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

private:

	int	m_iPlayer;	// For serializing
};

//-------------------------- C M u z z l e F l a s h -------------------------

class CMuzzleFlash : public CUnitTile
{

public:

	CMuzzleFlash();

	CVehicleSprite * GetSprite() const { return ( CVehicleSprite * )CTile::GetSprite(); }

	virtual void	Serialize( CArchive & ar );
};

//------------------------------ C T u r r e t ------------------------------

class CTurret : public CUnitTile
{

public:

	CTurret();
	CTurret( int iTurretID );

	void	DoMuzzleFlash();

	CMuzzleFlash	* GetMuzzleFlash()  { return &m_muzzleflash;	}
	CVehicleSprite * GetSprite() const { return ( CVehicleSprite * )CTile::GetSprite(); }

	LONG	GetDir() const			{ return m_iDir; }
	void	SetDir( LONG iDir )	{ m_iDir = iDir; }

	virtual void	Serialize( CArchive & ar );

	LONG	m_iDir;	// angle turret is facing (0..63)

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	void	Init( int iTurretID );

private:

	CMuzzleFlash	m_muzzleflash;
};


/////////////////////////////////////////////////////////////////////////////
// CUnit - a building or vehicle. This is the data that every vehicle or
//         building needs to track on a per instance basis (like damage).

class CUnit : public CUnitTile
{
#ifdef _DEBUG
friend class CGameMap;		// for AssertValid w/o infinite recursion
#endif
friend class CWndOrdFrame;
friend class CWndOrders;

public:

	enum UNIT_TYPE {			// run time type identification - use GetUnitType ()
		building,
		vehicle };

									CUnit ();
		virtual				~CUnit ();

		virtual void	AddUnit ();
		virtual void	RemoveUnit ();

		void		AssignData (CUnitData const * pData);

		CDamageDisplay			* GetDamageDisplay()			{ return m_ptrdamagedisplay.Value(); }
		CDamageDisplay const * GetDamageDisplay() const { return m_ptrdamagedisplay.Value(); }

		int			GetSpottingRange () const;
		int			GetRange () const;
		int			GetAttack ( int iInd ) const { return m_iAttack [iInd]; }
		int			GetDefense () const { return m_iDefense; }
		int			GetAccuracy () const;
		int			GetFireRate () const;		// == 0 if can't shoot

		int			GetDamageInflicted (CUnit const * pTarget, BOOL bRand) const;
		void		AssessDamage ( CMapLoc const & ml, int iNumShots, CUnit * pTarget );

		void					Shoot (CUnit * pUnit, int iLOS);

		virtual BOOL	IsHit( CHexCoord, CPoint ) const PURE_FUNC
		virtual void	GetDesc (CString & sText) const PURE_FUNC
		virtual void	InvalidateStatus () const PURE_FUNC
		virtual void	MaterialChange ();

		virtual int		GetNumStatusBars () const { return 1; }
		virtual void	PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					PaintStatusMaterials (CStatInst * pSi, CDC * pDc) const;
		virtual void	ShowStatusText (CString & str);

								enum UNIT_FLAGS { dying = 0x01, selected = 0x02, stopped = 0x04, event = 0x08, 
												destroying = 0x10, scenario = 0x20, repair_stop = 0x40, abandoned = 0x80,
												dead = 0x100, unit_set_damage = 0x200, show_bldg = 0x0400 };
		void					SetFlag (UNIT_FLAGS fl) { m_unitFlags = (UNIT_FLAGS) ((int) m_unitFlags | (int) fl); }
		void					ClrFlag (UNIT_FLAGS fl) { m_unitFlags = (UNIT_FLAGS) ((int) m_unitFlags & ~ (int) fl); }
		BOOL					IsFlag (UNIT_FLAGS fl) { return ((BOOL) ((int) m_unitFlags & (int) fl)); }
		virtual void	CancelUnit () {}
		virtual void	StopUnit () { SetFlag (stopped); }
		virtual void	ResumeUnit () { ClrFlag (stopped); ClrFlag (event); ClrFlag (repair_stop); }

		UNIT_FLAGS		GetFlags () const { return (m_unitFlags); }
		void					EventOff () { ClrFlag (event); ClrFlag (repair_stop); }
		virtual void	SetDestroyUnit () { SetFlag (destroying); }
		virtual void	StopDestroyUnit () { ClrFlag (destroying); }

		virtual void	DestroyAllWindows () {}

		void					ShowWindow ();
		virtual void	UpdateChoices () {}

		void					MsgSetTarget (CHexCoord const & hex);
		void					MsgSetTarget (CSubHex const & sub);
		void					MsgSetTarget (CUnit * pUnit);
		void					_SetTarget (CUnit * pUnit);
		void					SetOppo (CUnit * pUnit);
		CUnit *				GetTarget () const { return m_pUnitTarget; }
		CUnit *				GetOppo () const { return m_pUnitOppo; }

		CUnitData const *	GetData () const;
		DWORD					GetID () const;
		CPlayer * 		GetOwner () const;
		void					_SetOwner ( CPlayer * pPlyr );
		int						GetStore (int iInd) const;
		int						GetTotalStore () const;
		UNIT_TYPE			GetUnitType () const;

		void					SetSelected ( BOOL bDoList );
		void					SetUnselected ( BOOL bDoList );
		BOOL					IsSelected () const { return (m_unitFlags & selected); }
		BOOL					IsPaused () const;
		BOOL					IsWaiting () const;

		void					StoreMsg (CMsgBldgMat * pMsg);
		void					SetStore (int iInd, int iNum);
		void					AddToStore (int iInd, int iNum);
		int						GetStoreDiff (int iInd, BOOL bReset = FALSE);
		void					ZeroStore ();

		CMapLoc const & GetWorldPixels () const { return (m_maploc); }

		int						GetDamagePoints () const;
		int						GetDamagePer () const;
		int						GetLastShowDamagePer () const;
		float					GetDamageMult () const;
		float					GetDamPerfMult () const;
		void					DecDamagePoints (int iDamage, DWORD dwKiller = 0);
		void					UpdateDamageLevel ();
		void					MsgSetFire (CMsgShootElem * pMsg);
		void					PrepareToDie (DWORD dwIDKiller);
		DWORD					GetKiller () const { return m_dwPlyrKiller; }

		// 1 damage message per frame
		//   to server
		int						GetDamageThisTurn () { int iRtn = m_iDamageThisTurn;
																					m_iDamageThisTurn = 0;
																					return ( iRtn ); }
		void					AddDamageThisTurn ( CUnit * pKiller, int iDam ) 
																				{ m_iDamageThisTurn += iDam;
																					if ( pKiller == NULL )
																						m_dwLastShooter = 0;
																					else
																						m_dwLastShooter = pKiller->GetID (); }
		DWORD		 			GetLastShooter ( ) const { return m_dwLastShooter; }
		//   from server
		void					SetUnitSetDamage ( CUnit * pKiller ) 
																				{	if ( pKiller == NULL )
																						m_dwLastShooter = 0;
																					else
																						m_dwLastShooter = pKiller->GetID ();
																					SetFlag ( unit_set_damage ); }
		void					ClrUnitSetDamage () { ClrFlag ( unit_set_damage ); }

		// visibility, spotting, come into view, etc.
		BOOL					IsVisible () const { return (m_iVisible); }
		void					IncVisible () { m_iVisible++; }
		void					IncVisible (int iNum) { m_iVisible += iNum; }
		int						GetVisible () const { return (m_iVisible); }
		void					DecVisible () { m_iVisible--; }
		void					DecSee (int iPlyrNum);
		void					IncSee (CUnit * pUnitSee);
		BOOL					GetSee (CUnit * pUnitSee);

		void					DetermineSpotting () { ASSERT (FALSE); }
		void					DecrementSpotting ();
		void					IncrementSpotting (CHexCoord const & hex);
		BOOL					SpottingOn () const { return m_bSpotted; }

		void					OppoAndOthers ();
		void					OtherOppo ();
		virtual void	DetermineOppo () {}
		void					DetermineAiSee ();

		CTurret *			GetTurret() { ASSERT_VALID_OR_NULL( m_ptrturret.Value() );
												  return m_ptrturret.Value();	}

		CTurret const *	GetTurret() const { ASSERT_VALID_OR_NULL( m_ptrturret.Value() );
												  		  return m_ptrturret.Value();	}

		CFlag *			GetFlag() 		 { return &m_flag; }
		CFlag const *	GetFlag() const { return &m_flag; }

		static COLORREF		GetOk () { return (m_clrOk); }
		static COLORREF		GetWarn () { return (m_clrWarn); }
		static COLORREF		GetBad () { return (m_clrBad); }
		static char const * GetsDamage () { return (m_sDamage); }

		static CString 		m_sDamage;
		static COLORREF		m_clrOk;
		static COLORREF		m_clrWarn;
		static COLORREF		m_clrBad;

		virtual void	Serialize (CArchive & ar);
		virtual void	FixUp ();
		virtual void	FixForPlayer () {}

		void					CheckOppo (CUnit * pTarget, int & iDamageOppo, CUnit * * ppOppo);

		int						m_iFrameHit;					// show it being hit in the world map

		DWORD					m_dwLastMatTime;			// last time it broadcast materials on hand
		char					m_iUpdateMat;					// 0 - no update
																				// 1 - update soon
																				// 2 - update now
		int						GetVoice ();

protected:

		void	InitSmokeAndFlame();

		void					CheckAiOppo (CUnit * pTarget, CUnit * * ppOppo, BOOL bIsSet);

		void					SetTurret( int iTurretID );

		// we dupe this here to handle units at different levels due to R&D
		LONG					m_iSpottingRange;				// how many hexes away it can see
		LONG					m_iRange;								// maximum range it can shoot
		LONG					m_iAttack [3];					// attack - defense, not affected by defense, naval targets
		LONG					m_iDefense;							// ground defense power
		LONG					m_iAccuracy;						// how accurate it is
		LONG					m_iFireRate;						// fire rate (affected by damage)

		int						m_iVoice;						// which voice we used

		CPlayer *			m_pOwner;						// pointer to our owner
		CUnitData const * m_pUnitData;		// building or vehicle type data
		DWORD					m_dwID;							// unique ID for each unit
		UNIT_TYPE			m_iUnitType;				// if building or vehicle

		// attacking a unit
		CUnit *				m_pUnitTarget;			// unit want to attack
		CSubHex				m_ptTarget;					// m_ptHead of target last time we checked LOS
		CSubHex				m_ptUs;							// m_ptHead of us last time we checked LOS
		LONG					m_iLOS;							// LOS to hexTarget
		CUnit *				m_pUnitOppo;				// unit attacking now
		CSubHex				m_ptOppo;						// m_ptHead of target last time we checked LOS
		CSubHex				m_ptUsOppo;					// m_ptHead of us last time we checked LOS
		LONG					m_iOppoLOS;					// LOS to hexTarget
		DWORD					m_dwReloadMod;			// time already spent reloading

		LONG					m_aiStore [CMaterialTypes::num_types];	// materials on hand
		LONG					m_aiBeforeUpdate[CMaterialTypes::num_types];	// change from for net message

		// who can see us, who is shooting at us - counted by number of units of that player
		DWORD *				m_pdwPlyrsSee;			// which players can see this unit
		// spotting bitmap
		DWORD					m_dwaSpot [SPOTTING_ARRAY_SIZE]; // a bitmask of hexes this unit can see in
		int						m_iVisible;							// number of HP units that can see me
		BOOL					m_bSpotted;					// TRUE when it has spotted (set visibility for the map)
		CHexCoord			m_hexSpotting;			// hex on when did spotting

		// damage level of the building
		LONG					m_iDamagePoints;		// points of damage remaining required to destroy (0 == destroy it)
		LONG					m_iDamagePer;				// 0 == destoryed, 100 == ok - this is set so we don't have to calc it all the time
		float					m_fDamageMult;			// 0.0 == destroyed, 1.0 == ok
		float					m_fDamPerfMult;			// 0.5 == destroyed, 1.0 == ok
		LONG					m_iLastShowDamagePer;

		int						m_iDamageThisTurn;
		DWORD					m_dwLastShooter;

		DWORD					m_dwPlyrKiller;			// the player that killed us

		UNIT_FLAGS		m_unitFlags;

		Ptr< CTurret >	 	m_ptrturret;
		CFlag					m_flag;

		Ptr< CDamageDisplay >	m_ptrdamagedisplay;

#ifdef _DEBUG
		BOOL					m_Initialized;			// TRUE when can be asserted

public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CExplGrp - track the groups of explosions, This is for both projectiles and
//            other explosions (are there any?)

class CExplData
{
public:
						CExplData ();
		void		InitData (CMmio * pMmio);
		void		PostSpriteInit ();

		CSprite *	m_pProjSprite;		// sprite for the projectile
		int			m_iProjDelay;			// frames to delay before starting

						// when it arrives
		CSprite *	m_pExpSprite;		// sprite for the explosion
		int			m_iExplSound;			// explosion sound

		int			m_iFlags;
					enum { has_dir = 0x01 };
};

class CExplGrp
{
enum { NUM_EXPL_DATA = 6 };
public:
						CExplGrp () { m_iNumExpl = 0; }
		void		InitData ();
		void		PostSpriteInit ();

		int									GetNum () const { return (m_iNumExpl); }
		CExplData const *		GetData (int iIndex) const
																		{ ASSERT ((0 < iIndex) && (iIndex < m_iNumExpl));
																			return (&m_Data[iIndex]); }
protected:
		int					m_iNumExpl;
		CExplData		m_Data [NUM_EXPL_DATA];
};


/////////////////////////////////////////////////////////////////////////////
// CProjBase - muzzle flash, projectile, & explosion

class CInitProjMem;

class CProjBase : public CEffectTile
{
friend CProjMap;
friend CInitProjMem;
public:
		enum TILE_TYPE { projectile, explosion };	// GG: Need it for drawing explosions in front of vehicles

		void *	operator new ( size_t iSiz );
		void *	operator new ( size_t iSiz, const char * pFile, int iLine );
		void operator delete ( void * pBuf );

		CProjBase (BYTE bTyp);

		virtual void					Operate () { TRAP (); }
		CRect	Draw( const CHexCoord & );

		Fixed	GetAlt() const	{ return m_fixAlt; }

		TILE_TYPE	GetType() const { return ( TILE_TYPE )m_bType; }

		DWORD			m_dwIDTarget;						// who we are going to hit (may be 0)
		DWORD			m_dwIDShooter;					// who did the shooting

protected:
		BYTE			m_bType;
// GG						enum { projectile, explosion };
		Fixed			m_fixAlt;

		static MEM_POOL		m_memPool;

		// for linked list in CProjMap
		CProjBase *		m_pNext;
};

class CInitProjMem
{
public:
			CInitProjMem ();
};

class CProjectile : public CProjBase
{
public:
		CProjectile (CUnit const * pUnit, CMapLoc const & end, DWORD dwIDTarget, int iNumShots);
		void					Operate ();
		CRect					Draw( const CHexCoord & );

		int								m_xAdd;									// added to m_maploc.x each step
		int								m_yAdd;									// added to .y
		Fixed							m_faAdd;								// added to m_fixAlt
		int								m_iSteps;								// steps remaining to operate
		int								m_iStepMod;							// mod of steps inc
		int								m_iProjDelay;						// frames to delay before starting
		CMapLoc						m_mlEnd;								// our destination
		int								m_iNumShots;						// num shots fired
		CExplData const *	m_pEd;									// the explosion data
};

class CExplosion : public CProjBase
{
public:
		CExplosion (CProjectile const * pProj, CUnit * pTarget);
		CExplosion ( CUnit const * pTarget, CMapLoc const & ml, DWORD dwIDShooter );
		void					Operate ();
		CRect					Draw( const CHexCoord & );

		int			m_iKillFrame;			// on or after this frame kill the building
};

const int PROJ_BASE_ALLOC_SIZE = __max ( sizeof (CProjectile), sizeof (CExplosion) );

// we map based on CHex
class CProjMap : public CMap <DWORD, DWORD, CProjBase *, CProjBase *>
{
public:
	CProjMap () { InitHashTable (HASH_INIT); }

	void					Add (CProjBase * pProj);
	void					Move (CHexCoord const & _hex, CProjBase * pProj);
	void					Remove (CProjBase * pProj);
	void					Remove (CHexCoord const & _hex, CProjBase * pProj);

	CProjBase *		GetFirst ( CHexCoord _hex ) const;
	static CProjBase *		GetNext ( CProjBase * pOn );

protected:
	DWORD					ToArg (CMapLoc const & ml) const;
	DWORD					ToArg (CHexCoord const & _hex) const;
};


CUnit * _GetUnit (DWORD dwID);
CUnit * GetUnit (DWORD dwID);
CUnit * GetUnit (CSubHex const & sub);	// bldg takes priority
CSubHex Rotate (int iDir, CSubHex const & ptHead, CSubHex const & ptTail);
void UnitShowStatus (void * pData, CDC * pDc, CRect const & rDraw, CDIB * pDib, CPoint const & ptOff);
void _UnitShowStatus (BOOL bText, void * pData, CDC * pDc, CRect const & rDraw, CDIB * pDibBack, CPoint const & ptOff);
void UnitStatusText (void * pData, CString & str);


extern CTurrets			theTurrets;
extern CMuzzleFlashes 	theFlashes;
extern CEffect				theEffects;
extern CExplGrp			theExplGrp;
extern CProjMap			theProjMap;


#endif
