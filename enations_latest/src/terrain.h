//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __TERRAIN_H__
#define __TERRAIN_H__

// terrain.h : header file for all hex/terrain stuff
//
#include "stdafx.h"

#include "base.h"
#include "sprite.h"
#include "unit.h"
#include "icons.h"

const int LOS_ALT = 4;		// alt difference needed to obscure

const int CITY_DESTROYED_OFF = 0;
const int CITY_DESTROYED_NUM = 8;
const int CITY_BUILD_OFF = 8;
const int CITY_BUILD_NUM = 7;


/****************************************************************************
The map is stored in row major order (0,0; 1,0; 2,0; 0,1; 1,1;...)
The m_iDir==0 slices it in the / direction and it rotates clockwise from
there (m_iDir == 1 bottom up \, m_iDir == 2 bottom up /, m_idir == 3 \
****************************************************************************/

class CDlgCreateWorld;
class CGameMap;
class CTerrain;
class CVehicle;

// this is so we don't have to keep allocating & sizing CDib on each call
class CTerrainShowStat
{
public:
	CTerrainShowStat () {}
	void Init ();
	void Close ();

	CStatInst		m_si[3];
};

/////////////////////////////////////////////////////////////////////////////
// CHex - a hex space

const int hHexWid [NUM_ZOOM_LEVELS] = {MAX_HEX_HT * 2, MAX_HEX_HT, MAX_HEX_HT / 2, MAX_HEX_HT / 4 };
const int hHexHt [NUM_ZOOM_LEVELS] = {MAX_HEX_HT, MAX_HEX_HT / 2, MAX_HEX_HT / 4, MAX_HEX_HT / 8 };

#pragma pack( push, chex, 1 )
class CHex : public CSimpleTile
{
#ifdef _DEBUG
friend class CBuilding;
#endif
friend class CGameMap;
friend class CUnit;

public:
	enum {			// terrain types
		city,			// CWndWorld m_clrTerrains is dependent on this
		desert,
		forest,
		lake,
		hill,
		mountain,
		ocean,
		plain,
		river,
		road,
		rough,
		swamp,
		coastline,
		fields,						// grown around farms
		resources,				// for resource view
		num_types };

	enum {			// altitudes
		map_step = 8,			// every 8 in altitude is a step up on the map screen
		unassigned = 0,
		ocean_floor = 1,
		sea_level = 16,		// == 16 -> ocean, == 17 -> land
		average = 44,
		mountain_top = 96,
		max = 104};

	enum {			// road facings
		r_horz,
		r_vert,
		r_t_up,		// edge top of T runs into
		r_t_rt,
		r_t_dn,
		r_t_lf,
		r_l_ur,		// corner vertex of L is in
		r_l_lr,
		r_l_ll,
		r_l_ul,
		r_x,
		r_path };	// show when have road cursor

	enum {			// coastlines
		land_ul,	// 1/4 land
		land_ur,
		land_lr,
		land_ll,
		land_up,	// 1/2 land
		land_rt,
		land_dn,
		land_lf,
		water_ul,	// 1/4 water
		water_ur,
		water_lr,
		water_ll,
		island };	// all 4 sides


	CHex ();
	~CHex () {}
		const CHex & operator= (CHex const & src);

	void		Init (int iAlt);
	void		InitType ();
	CRect		Draw( const CHexCoord & );
	CString	GetStatus ();

	CHexCoord	GetHex () const;

	int			GetAlt () const;
	int			GetSeaAlt () const;	// likje GetAlt but returns surface of ocean
	void		SetAlt (int iAlt);
	void		SetOceanAlt (int iAlt);
	int			GetAltDraw () const;
	void		AdjustY (int & y) const;
	int			GetAdjustY (int iZoom) const;
	int			GetAdjustStep () const;
	int			GetType () const;
	BOOL		IsWater () const;
	BOOL		CanRoad () const;

	int			GetUnitDir () const;
	void		SetUnitDir (int iDir);
	void		ClrUnitDir ();

	int			GetCursorMode () const;
				enum { no_cur, ok_cur, warn_cur, bad_cur, lousy_cur, land_exit_cur, sea_exit_cur };
	void		SetCursor ();
	void		ClrCursor ();
																					//         but let me do it - it affects other places
	void		PaintStatusBars (CStatInst * pSi, CDC * pDc) const;

	void		SetTree (int iTree);
	int			GetTree () const;

	void		ChangeToRoad (CHexCoord & _hex, BOOL bCallNext = TRUE, BOOL bForce = FALSE);
	void		SetType (int iType);

				enum { ul=0x01, ur=0x02, ll=0x04, lr=0x08, bldg=0x10, 
								minerals = 0x20, bridge = 0x40, proj = 0x80 };
				enum { veh = (ul | ur | ll | lr), unit = (bldg | veh) };
	static BYTE		GetVehUnits ( CSubHex const & _subSrc )
													{ BYTE bVeh = 1 << (_subSrc.x & 1);
														if ( _subSrc.y & 1 )
															bVeh <<= 2;
														return bVeh; }
	BYTE		GetUnits () const { return (m_bUnit); }
	void		OrUnits (int iVal) { m_bUnit |= (BYTE) iVal; }
	void		NandUnits (int iVal) { m_bUnit &= (BYTE) ~ iVal; }

	void		IncVisible ();
	void		IncVisible (int iNum);
	void		DecVisible ();
	int			GetVisible () const;
	BOOL		GetVisibility () const;

	void		SetBridge();
	void		ClrBridge();
	BOOL		IsBridge() const;

	BYTE		GetVisibleType ();
	void		SetVisibleType ( int iType );

	CBuilding *GetVisibleBuilding( CHexCoord hexcoord ) const;

	void 		Serialize (CArchive & ar);

	CTerrainSprite *	GetSprite() { return ( CTerrainSprite * )m_psprite; }

protected:

		BYTE			m_bType;			// 0 - 3 : terrain type
														// 4 - 6 : tree type
														// 4 - 7 : also bldg dir
		BYTE			m_bAlt;				// 0 - 6 : altitude at Lower Left corner
														// 7 : is cursor
		BYTE			m_bUnit;			// flags if buildings and/or vehicles here
		BYTE			m_bVisible;		// count of number of vehicles that can see it
														// note - if we get 256 it wraps but it will undo ok & all we get
														//        is a hex invisible when it should be visible
#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};
#pragma pack( pop, chex )

/////////////////////////////////////////////////////////////////////////////
// CTerrainData - data on terrain hexes

#ifdef _DEBUG
class CTerrainData : public CObject
#else
class CTerrainData
#endif
{
#ifdef _DEBUG
friend class CVehicle;
#endif
friend CTerrain;
public:
							CTerrainData () {}

			int				GetBuildMult () const { ASSERT_STRICT_VALID (this); return (m_iBuildMult); }
			int				GetFarmMult () const { ASSERT_STRICT_VALID (this); return (m_iFarmMult); }
			int				GetWheelMult (int iInd) const { ASSERT_STRICT_VALID (this); 
														ASSERT_STRICT ((0 <= iInd) && (iInd < NUM_WHEEL_TYPES));
														return (m_iWheelMult [iInd]); }
			int				GetDefenseMult (int iInd) const { ASSERT_STRICT_VALID (this);
														ASSERT_STRICT ((0 <= iInd) && (iInd < NUM_WHEEL_TYPES));
														return (m_iDefenseMult [iInd]); }
			CString const &	GetDesc () const { ASSERT_STRICT_VALID (this); return (m_sDesc); }

			static int GetBuildRoadTime () { return (8); }

protected:
			int			m_iBuildMult;						// extra time to put in foundation
			int			m_iFarmMult;						// reduction in farm output
			int			m_iWheelMult [NUM_WHEEL_TYPES]; // multiplier for movement
			int			m_iDefenseMult [NUM_WHEEL_TYPES]; // multiplier for defense at that location
			CString	m_sDesc;								// what its called

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CTerrain : public CSpriteStore< CTerrainSprite >
{
#ifdef _DEBUG
friend class CTerrainData;
#endif
public:
			CTerrain( char const * pbyRifName ) : CSpriteStore< CTerrainSprite >( pbyRifName ) {}
									~CTerrain ();

			void				InitData ();
			void				InitSprites ();
			void				InitLang ();

			void	MakeRotated( int iIDSrc, int iIndexSrc, int iIDDst, int iIndexDst, int iRotRight );

			CTerrainData const & GetData (int iType) const;

protected:
		CTerrainData 		m_Data[CHex::num_types];

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CGameMap - the map

// return TRUE if done
typedef int (FNENUMHEX) (CHex * pHex, CHexCoord hex, void  *pData);

class CGameMap : public CObject
{
friend class CHexCoord;
friend class CSubHex;
friend class CMapLoc;
friend class CHex;
#ifdef _CHEAT
friend class CUnit;
#endif

public:
	CGameMap ();
	~CGameMap () {Close ();}

	void		GetWorldSize (int iSize, int & iSide, int & iSideSize);
	void		Init (int iSide, int iSideSize, int iScenario);
	void		InitSquare (int x1, int y1, int x2, int y2, int iTyp1, int iTyp2, int iTyp3, int iTyp4);
	void		InitSquarePass2 (int x1, int y1, int x2, int y2, int iTyp1, int iTyp2, int iTyp3, int iTyp4);
	int			DepositMinerals (int x, int y, int iTyp, int iNum);
	int			MakeMineral (int x, int y, int iTyp, int iSideSize);
	void		CheckAlt ();
	void		AddCoastlines ();
	void		EliminateSingles ();
	void		MakeLakes ();

	void		Close ();
	void		Update (CAnimAtr & aa);
	void		UpdateRect (CAnimAtr & aa, CRect, CDrawParms::UPDATE_MODE );
	CSize		GetSize () const {ASSERT_STRICT_VALID (this); return (CSize (m_eX, m_eY));}
	int			Get_eX () const { return (m_eX); }	// do NOT put assert's in this
	int			Get_eY () const { return (m_eY); }
	int			GetSideSize () const { return (m_iSideSize); }
	int			GetSideShift () const { return (m_iSideShift); }

	CHex *	GetHex (CHexCoord const & hex);
	CHex *	GetHex (CSubHex const & sub);
	CHex *	GetHex (int x, int y);
	CHex *	_GetHex (int x, int y);
	CHex *	_GetHex (CHexCoord const & hex);
	CHex *	_GetHex (CSubHex const & sub);

	CHex const *	GetHex (CHexCoord const & hex) const;
	CHex const *	GetHex (CSubHex const & sub) const;
	CHex const *	GetHex (int x, int y) const;
	CHex const *	_GetHex (int x, int y) const;
	CHex const *	_GetHex (CHexCoord const & hex) const;
	CHex const *	_GetHex (CSubHex const & sub) const;

	// these set up the building curosr
	void		SetBldgCur (CHexCoord const & hex, int iBldg, int iBldgDir, int iCur);
	void		ClrBldgCur ();
	int			IsBldgCurOk () const { return (m_iBldgCur); }
	BOOL		HaveBldgCur () const { return (m_cxBldgCur > 0); }
	CHex *		m_pLandExit;		// the hex with the land exit
	int				m_iLandDir;			// the direction of the land exit
	CHex *		m_pShipExit;		// the hex with the ship exit
	int				m_iShipDir;			// the direction of the ship exit

	CHex *	_Xinc (CHex * pHex);
	CHex *	_Xinc (CHex * pHex, int iNum);
	CHex *	_Yinc (CHex * pHex);
	CHex *	_Yinc (CHex * pHex, int iNum);
	int			WrapX (int x) const;
	int			WrapY (int y) const;
	void		SpiralHexes (CHexCoord const & hex, int iRadius, int (fnEnum) (CHex * pHex, CHexCoord hex, void  *pData), void  *pData = NULL);
	void		EnumHexes (CHexCoord const & hex, int ex, int ey, int (fnEnum) (CHex * pHex, CHexCoord hex, void  *pData), void  *pData = NULL);
	void		_EnumHexes( CHexCoord const & hex, int ex, int ey, int (fnEnum) (CHex * pHex, CHexCoord hex, void  *pData), void  *pData = NULL );
	void		DirMult (int iDir, int *pInt) const;

	// building stuff
	int			FoundationCost (CHexCoord const & hex, int iType, int iDir, CVehicle const * pVeh, int * piAlt = NULL, int * piWhy = NULL) const;
	int			BridgeCost (CHexCoord const & hexBgn, CHexCoord const & hexEnd, CVehicle const * pVeh, int * piAlt = NULL, int * piWhy = NULL) const;
					enum { ok, bldg_next, water_next, bldg_or_river, veh_in_way, water, no_water, no_land_exit, no_water_exit, steep };

	// battle stuff
	int			GetRangeDistance (CUnit const * pSrc, CUnit const * pDest) const;
	int			GetRangeDistance (CHexCoord & hex1, CHexCoord & hex2);
	int			GetRangeDistance (CSubHex & sub1, CSubHex & sub2);
	int			LineOfSight (CUnit const * pSrc, CUnit const * pDest) const;

	// path & spotting stuff
	int			GetRangeBase () { return (1); }
	int			GetTravelTime (CHexCoord const & hexSrc, CHexCoord const & hexDest, int iVehType);
	int			GetTerrainCost (CHexCoord const & hex, int iDir, int iWheel);
	int			GetTerrainCost (CHexCoord const & hex, CHexCoord const & hexNext, int iDir, int iWheel);
	int			GetTerrainCost (CHex const * pHex, CHex const * pHexDest, int iDir, int iWheel);
	int			_GetTerrainCost (CHex const * pHex, CHex const * pHexDest, int iDir, int iWheel);
	int			GetTerrainBase () { return (2); }

	// these list the sizes of the hex depending on the level of zoom
	static int		HexWid (int iZoom) {ASSERT_STRICT ((iZoom >= 0) && (iZoom < NUM_ZOOM_LEVELS)); return (hHexWid[iZoom]);}
	static int		HexHt (int iZoom) {ASSERT_STRICT ((iZoom >= 0) && (iZoom < NUM_ZOOM_LEVELS)); return (hHexHt[iZoom]);}

	void 		Serialize(CArchive& ar);


	CHexValidMatrix 		 *	GetHexValidMatrix() 		  { ASSERT( m_ptrhexvalidmatrix.Value() );
																		 return m_ptrhexvalidmatrix.Value(); }

	CHexValidMatrix const *	GetHexValidMatrix() const { ASSERT( m_ptrhexvalidmatrix.Value() );
																		 return m_ptrhexvalidmatrix.Value(); }

protected:
	int			CheckIt (CSpriteView const *pSv, int x, int y) const;
	BOOL		MakePeak (int xOk, int yOk, int xTest, int yTest, int iSidesize, BOOL bEasy = FALSE);
	void		MakeRiver (int x, int y, BOOL & bFound);
	void		MakeTerrain (int x, int y, int iTyp, int iSideSize);

	void		CheckOcean();

	LONG		m_eX;							// size of world
	LONG		m_eY;
	LONG		m_iSideSize;			// size of a player blk on map
	LONG		m_iSideShift;			// how much to shift for a mul/div
	CHex *	m_pHex;						// hexes in row major order

	LONG		m_iHexMask;				// used to wrap CHexCoord's
	LONG		m_iSubMask;				// used to wrap CSubHex's
	LONG		m_iWidthHalf;			// half the width (used in CHexCoord::Diff)
	LONG		m_iLocMask;
	LONG		m_iLocHalf;

	int				m_iBldgCur;			// CHex:: value of cursor
	CHexCoord	m_hexBldgCur;		// location of bldg cur
	int				m_cxBldgCur;		// == 0 for none
	int				m_cyBldgCur;

	Ptr< CHexValidMatrix	>	m_ptrhexvalidmatrix;

#ifdef _CHEAT
	long		GetHexOff (CHex const *pHex) const { return (pHex - m_pHex); }
#endif
#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


void TerrainShowStatus (void * pData, CDC * pDc, CRect const & rDraw, CDIB * pDib, CPoint const & ptOff);


extern CGameMap theMap;
extern CTerrain theTerrain;
extern CTerrainShowStat tShowStat;

#endif
