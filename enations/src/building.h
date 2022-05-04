//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __BUILDING_H__
#define __BUILDING_H__

// building.h : header file for all building stuff
//

#include "unit.h"
#include "vehicle.h"


const int ROCKET_APT_CAP = 80;
const int ROCKET_OFC_CAP = 40;

const int BUILDING_BASE = 0;		// index of foundation (stored as a building)

// this is for damage drawing
const int DAMAGE_0 = 100;				// used for %
const int DAMAGE_1 = 90;				// <= then use bitmap damage == 1
const int DAMAGE_2 = 60;
const int DAMAGE_3 = 30;


class CStructureData;
class CBuildHousing;
class CBuildMaterials;
class CBuildVehicle;
class CBuildPower;
class CBuildResearch;
class CBuildWarehouse;
class CBuildMine;
class CBuildFarm;
class CBuildRepair;
class CBuildShipyard;
class CRepairBuilding;


/////////////////////////////////////////////////////////////////////////////
// CStructure - data on buildings
//
// This should have been in CStructureData (like CUnitStore above)

class CStructure : public CSpriteStore< CStructureSprite >
{
#ifdef _DEBUG
friend class CStructureData;
friend class CBuilding;
#endif
public:
									CStructure ( char const * pszRifName );
									~CStructure ();

			void				InitData ();
			void				InitSprites ();
			void				InitLang ();

			void				Close ();
			CStructureData const * GetData (int iIndex) const;
			int					GetNumBuildings () const;
			int					GetIndex (CStructureData const * GetData) const;
			int				GetTallestBuildingHeight( int iZoom );

protected:

      void MakeRotated( int	iIDSrc,
	                     int	iIndexSrc,
	                     int	iIDDst,
	                     int	iIndexDst,
	                     int	iRotRight );

		CStructureData * _GetData (int iIndex) const;

		CStructureData * *	m_ppData;
		int									m_iNumBuildings;
		int						m_iTallestBuildingHeight;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CStructureTypes - used by the build building dialog
//

#ifdef _DEBUG
class CStructureType : public CObject
#else
class CStructureType
#endif
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();
friend void CStructure::Close ();
public:
					CStructureType () {}
		char const *	GetDesc (int iInd) { ASSERT_STRICT ((0 <= iInd) && (iInd < 6)); return (m_sTitle[iInd]); }
		int						GetNum () { return (6); }

protected:
		CString		m_sTitle[6];

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CStructureData - data on buildings
//
// This is the base class for all structures. However, it is a virtual class
// in practice because for each building type, one of the CBuild**** classes
// below (all based on CStructureData) is what is actually created.

class CStructureData : public CUnitData
{
friend class CDlgBuildStructure;
friend class CBuildShipyard;
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();
#ifdef _DEBUG
friend class CVehicleBuilding;
friend class CMaterialBuilding;
friend class CPowerBuilding;
friend class CWarehouseBuilding;
friend class CMineBuilding;
friend class CFarmBuilding;
friend class CRepairBuilding;
friend class CShipyardBuilding;
#endif

public:
		int			GetClassType () { return CUnitData::structure; }

		enum BLDG_TYPE { city,								// this is returned by GetType ()
						rocket,
						apartment_1_1,			// #=frontier/estab/city, #=choice
						apartment_1_2,
						apartment_2_1,
						apartment_2_2,
						apartment_2_3,
						apartment_2_4,
						apartment_3_1,			// obsolete
						apartment_3_2,			// obsolete
						office_2_1,
						office_2_2,
						office_2_3,
						office_2_4,
						office_3_1,
						office_3_2,					// obsolete
						// above are only placed by computer
						barracks_2,
						barracks_3,					// obsolete
						command_center,
						embassy,
						farm,
						fort_1,
						fort_2,
						fort_3,
						heavy,
						light_1,
						light_2,
						lumber,
						oil_well,
						refinery,
						coal,
						iron,
						copper,
						power_1,
						power_2,
						power_3,
						research,
						repair,
						seaport,
						shipyard_1,
						shipyard_3,
						smelter,
						warehouse,
						light_0,
                  bridge_0,
                  bridge_1,
                  bridge_end_0,
                  bridge_end_1,
                  bridge_end_2,
                  bridge_end_3,
						num_types,
						
						apartment_base = apartment_1_1,
						num_apartments = 6,
						office_base = office_2_1,
						num_offices = 5,
						num_shareware_civ = 3,
						first_show = barracks_2,

		// these are to have a way to quickly test if a building is a specific type
		// GetBldgType will return the values above where there is only 1 of something
		// (like a coal mine).
						apartment = apartment_1_1,			// GetBldgType ()
						office = office_2_1,
						barracks = barracks_2,
						fort = fort_1,
						light = light_1,
						power = power_1,
						shipyard = shipyard_1 };

		enum BLDG_DLG_GRP {	dlg_no_show,		// for the build building dialog - building groups
						dlg_const_yards,
						dlg_natural_res,
						dlg_forts,
						dlg_manuf,
						dlg_admin,
						dlg_num_items };

		enum BLDG_UNION_TYPE { UTmaterials,		// builds materials - m_iUnionType values	GetUnionType ()
					UTvehicle,			// builds vehicles
					UThousing,			// place to live
					UTcommand,			// command center
					UTembassy,
					UTfort,
					UTpower,				// power plant
					UTresearch,
					UTrepair,
					UTwarehouse,		// market or warehouse
					UTmine,
					UTfarm,					// farm or lumber mill
					UTshipyard,			// builds & repairs
					num_union_types };

			CStructureData () { ctor (); }
			CStructureData (BLDG_UNION_TYPE iType) { ctor (); m_iUnionType = iType; }
			virtual ~CStructureData () {}

			BLDG_TYPE				GetType () const;					// CStructureData type (each instance) - 1 per sprite
			BLDG_TYPE					GetBldgType () const;			// CBuilding type - 1 apt, 1 office, etc.
			BLDG_UNION_TYPE		GetUnionType () const;		// CBuild*** type (each base type of building)

									enum BLDG_FLAGS { FlVehExit=0x01, FlShipExit=0x02, FlPartWater=0x04, FlBlackHole=0x08,
									FlOperAmb1 = 0x10, FlOperAmb2 = 0x20, FlConstAmb1 = 0x40, FlConstAmb2 = 0x80 };
			BLDG_FLAGS	GetBldgFlags () const { return m_bFlags; }

			int			GetPopHoused () const;	// uses MaxMaterials var

			BOOL			HasVehExit () const ;			// returns true if vehicles enter it
			BOOL			HasShipExit () const;			// returns true if ships enter it
			BOOL			IsPartWater () const;			// returns true if must be on both land and water

			int				GetCat () const;
			int				GetTimeBuild () const;
			int				GetMoraleEffect () const;
			int				GetBuild (int iInd) const;
			int				GetPower () const;
			float			GetNoPower () const;

			CHexCoord & GetExitHex () { return (m_hexExit); }
			CHexCoord const & GetExitHex () const { return (m_hexExit); }
			int				GetExitDir () const { return (m_iExitDir); }
			CHexCoord & GetShipHex () { return (m_hexShip); }
			CHexCoord const & GetShipHex () const { return (m_hexShip); }
			int				GetShipDir () const { return (m_iShipDir); }

			CBuildHousing *			GetBldHousing () const;
			CBuildMaterials *		GetBldMaterials () const;
			CBuildVehicle *			GetBldVehicle () const;
			CBuildPower *				GetBldPower () const;
			CBuildResearch *		GetBldResearch () const;
			CBuildMine *				GetBldMine () const;
			CBuildWarehouse *		GetBldWarehouse () const;
			CBuildFarm *				GetBldFarm () const;
			CBuildRepair *			GetBldRepair () const;
			CBuildShipyard *		GetBldShipyard () const;

static BOOL CanBuild (CHexCoord const & hex, int iBldgDir, int iBldgTyp, BOOL bShip, BOOL bRocket);

protected:
#ifdef _DEBUG
			void		ctor ();
#else
			void		ctor () {}
#endif

			BLDG_TYPE				m_iType;								// enum's above of building type
			BLDG_UNION_TYPE		m_iUnionType;						// CBuild*** class type (below)

			BLDG_FLAGS					m_bFlags;

			// this is for building the building itself
			int			m_iBldgCat;							// for the build dialog
			int			m_iTimeBuild;						// time to build
			int			m_iMoraleEffect;				// effect on morale
			int			m_aiBuild [CMaterialTypes::num_build_types];	// materials needed to build

			// this is what it does
			int			m_iPower;								// power used while running
			float		m_fNoPower;							// multiplier of time required if low power

			// this is its exit in normal rotation
			CHexCoord			m_hexExit;					// for vehicles entering & exiting
			LONG					m_iExitDir;					// 0 == exit up, 1 == exit right ...
			CHexCoord			m_hexShip;					// for ships entering & exiting
			LONG					m_iShipDir;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildMaterials - a building that takes materials in one one side and produces
//                   other materials. Like a smelter.

class CBuildMaterials : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildMaterials () : CStructureData (CStructureData::UTmaterials) {}

			int			GetTime () const;
			int			GetInput (int iInd) const;
			int			GetOutput (int iInd) const;

protected:
			int			m_iTime;								// time to build
			int			m_aiInput [CMaterialTypes::num_types];	// materials needed for input
			int			m_aiOutput [CMaterialTypes::num_types];	// materials created from input

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CBuildVehicle - a building that produces vehicles. This is actually set up
//                 in 3 structures to break out the requriements for each
//                 vehicle a building can produce.

// what it takes to build one unit (in this factory)
class CBuildUnit : public CObject
{
friend class CDlgBuildStructure;
friend class CDlgBuildTransport;
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			int			GetTime () const;
			int			GetInput (int iInd) const;
			int			GetVehType () const;

protected:
			int			m_iVehType;							// vehicle built
			int			m_iTime;								// time to build
			int			m_aiInput [CMaterialTypes::num_build_types];	// materials needed to build

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


// this is the actual class created
//   its primarily an array of the unit's it can build
class CBuildVehicle : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();
public:
			CBuildVehicle ();
			~CBuildVehicle ();

			int						GetSize () const;
			CBuildUnit const *	GetUnit (int iInd) const;
			int						GetTotalInput (int iInd) const;

protected:
			CArray <CBuildUnit *, CBuildUnit *> m_aBldUnits;		// units (vehicles) this bldg can build
			int			m_aiTotalInput [CMaterialTypes::num_build_types];	// materials needed to build 1 of each

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildHousing - apartments

class CBuildHousing : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildHousing () : CStructureData (CStructureData::UThousing) {}

			int		GetCapacity () { ASSERT_STRICT_VALID (this); return (m_iCapacity); }

protected:
			int		m_iCapacity;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildCommand - a command post

class CBuildCommand : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildCommand () : CStructureData (CStructureData::UTcommand) {}

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildEmbassy

class CBuildEmbassy : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildEmbassy () : CStructureData (CStructureData::UTembassy) {}

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildFort

class CBuildFort : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildFort () : CStructureData (CStructureData::UTfort) {}

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildPower - a power plant

class CBuildPower : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildPower () : CStructureData (CStructureData::UTpower) {}

			int		GetPower () { ASSERT_STRICT_VALID (this); return (m_iPower); }
			int		GetInput () { ASSERT_STRICT_VALID (this); return (m_iInput); }
			int		GetRate () { ASSERT_STRICT_VALID (this); return (m_iRate); }

protected:
			int		m_iPower;				// power generated
			int		m_iInput;				// material type for power ( -1 -> none)
			int		m_iRate;				// number of frames 1 m_iInput lasts

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildResearch - R&D

class CBuildResearch : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildResearch () : CStructureData (CStructureData::UTresearch) {}

			int		GetRate () { ASSERT_STRICT_VALID (this); return (m_iRate); }

protected:
			int		m_iRate;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildRepair - repairs vehicles (a crane repairs buildings)

class CBuildRepair : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildRepair ();
			~CBuildRepair ();

			BOOL					CanRepair (int iInd) const;
			CBuildUnit const *	GetRepair (int iInd) const;
			int						GetSize () const { return (m_aRprUnits.GetSize ()); }
			int						GetTotalRepair (int iInd) const;

protected:
			CArray <CBuildUnit *, CBuildUnit *> m_aRprUnits;		// units (vehicles) this bldg can repair
			int			m_aiTotalRepair [CMaterialTypes::num_build_types];	// materials needed to fully repair 1 of each

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildShipyard - inherits from CBuildVehicle & CBuildRepair

class CBuildShipyard : public CBuildVehicle
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildShipyard ();
			~CBuildShipyard ();

			BOOL					CanRepair (int iInd) const;

protected:

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildWarehouse (includes the rocket)

class CBuildWarehouse : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildWarehouse () : CStructureData (CStructureData::UTwarehouse) {}

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildMine - mines minerals out of the ground. Incluses an oil well

class CBuildMine : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildMine () : CStructureData (CStructureData::UTmine) {}

			int				GetTimeToMine () const { return (m_iTime); }
			int				GetAmount () const { return (m_iAmount); }
			int				GetTypeMines () const { return (m_iOutput); }

protected:
			int			m_iTime;								// time to mine
			int			m_iAmount;							// amount produced in MAX_DENSITY ground
			int			m_iOutput;							// CMaterialTypes:: it mines

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuildFarm - gets materials from adjoining hexes. Includes a lumber mill.

class CBuildFarm : public CStructureData
{
friend void CStructure::InitData ();
friend void CStructure::InitSprites ();
friend void CStructure::InitLang ();

public:
			CBuildFarm () : CStructureData (CStructureData::UTfarm) {}

			void			BuildFarm ();

			int				GetTimeToFarm () const { return (m_iTime); }
			int				GetTypeFarm () const { return (m_iOutput); }
			int				GetQuantity () const { return (m_iQuantity); }

static int			GetCowDistance () { return (3); }

protected:
			int			m_iTime;								// time to mine
			int			m_iOutput;							// CMaterialTypes:: it mines
			int			m_iQuantity;						// how much you get each time

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBuilding - a building. As with CStructureData, a CBuilding derived class
//             (see below) is what is actually created in MOST cases.

class CBuilding : public CUnit
{
friend class CGame;
friend CWndOrders;
friend void CUnit::DecDamagePoints (int iDamage, DWORD dwKiller);

public:
									CBuilding () { ctor (); }
									CBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0);
									~CBuilding ();

		virtual void	AddUnit ();
		virtual void	RemoveUnit ();

		virtual void GetInputs (int * pVals) const;
		virtual void GetAccepts (int * pVals) const;
		virtual void ConstComplete () { }

		void					AnimateOperating (BOOL bOper);
		void					StopUnit () { CUnit::StopUnit (); AnimateOperating (FALSE); }
		void					ResumeUnit () { CUnit::ResumeUnit (); 
																	if ( ! IsConstructing () )
																		AnimateOperating (TRUE); }
		void					EventOff () { CUnit::EventOff (); 
																if ( IsOperating () && (! (m_unitFlags & stopped)) && (! IsConstructing ()) )
																	AnimateOperating (TRUE); }
		virtual BOOL	IsOperating () const
															{ return (! (m_unitFlags & stopped)) && (! IsConstructing ()); }
		void					SetDestroyUnit () { m_fOperMod = 0.0f; SetFlag (destroying); }

		void					HandleCombat ();

		void					DetermineSpotting ();
		void					DetermineOppo ();
		void					MakeBldgVisible ();

		BOOL					IsLive () const;	// TRUE if we can see it now
		void					UpdateStore (BOOL bForce);

		void					DrawStatusPer (CDC *pDc, CRect *pRect) const;
		virtual int		GetNumStatusBars () const;
		virtual void	PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		virtual void	ShowStatusText (CString & str);

		void 					GetDesc (CString & sText) const;
		CRect					Draw( const CHexCoord & );
		void					DrawFoundation( const CHexCoord & );
		CRect					DrawTurret( CPoint );
		BOOL					IsHit( CHexCoord, CPoint ) const;
		void					Operate ();
		void					InvalidateStatus () const;
		BOOL					IsConstructing () const;
		void					MaterialChange ();

		virtual void	VehicleLeaving ( CVehicle * ) { }

		void					MaterialMessage ();
		virtual void	FixUp ();

		int						GetProd (float fProd);
		int						GetProdNoDamage (float fProd);
		int						GetProdNoPeople (float fProd);
		float					GetFrameProd (float fProd);	// prod for 1 frame
		float					GetFrameProdNoPwr (float fProd);	// prod for 1 frame - no power multiplier
		float					GetFrameProdNoPeople (float fProd);	// prod for 1 frame - no people multiplier

		int						GetBuildPer () const { return m_iLastPer; }

		void					AssignToHex (CHexCoord hex, int iAlt);
		CHexCoord &		GetHex ();
		CHexCoord const &		GetHex () const;
		int						GetDir () const { return (m_iBldgDir); }
		int						GetDrawDir () const { return ((GetDir() + xiDir) & 0x03); }
		CPoint				GetDrawCorner( const CHexCoord & ) const;

		int						GetPerHave () const;
		int						GetBuiltPer () const;

		void					SetConstPer ();
		void					UpdateConst ( CMsgBldgStat * pMsg );

		void					Construct ();
		void					BuildMaterials ();
		int						NeedToBuild (int iInd, int iNum = 100) const;
		int						NeedToRepair (int iInd, int iPts) const;
		int						PointsRepaired (int iInd, int iMat) const;
		int						GetBldgMatReq (int iInd, BOOL bSubStore) const;
		int						GetBldgMatRepair (int iInd) const;
		virtual int		GetBldgResReq (int iInd, BOOL) const { return GetBldgMatReq (iInd, TRUE); }
		virtual int		GetNextMinuteMat (int iInd) const { return GetBldgResReq (iInd, FALSE); }

		void					AddConstDone (int iDone);
		CStructureSprite *GetSprite() const
								{
									#ifdef _DEBUG
									(( CStructureSprite * )m_psprite )->CheckValid();
									#endif
									return ( CStructureSprite * )m_psprite;
								}

		static CBuilding * Create (CHexCoord const & hex, int iBldg, int iBldgDir, CVehicle *pVeh, int iOwner, DWORD ID, BOOL bShow);
		static CBuilding * Alloc (int iBldg, int iBldgDir, int iOwner, DWORD ID);
		static void	DetermineExit (int iBldgDir, int iBldgTyp, CHexCoord & hexExit, int & iExitDir, BOOL bShip);

		CStructureData const *	GetData () const;

		void 					Serialize (CArchive & ar);
		void					FixForPlayer ();

		CHexCoord			GetExit (int iWheelTyp);
		CHexCoord			GetExitDest (CTransportData const * pTd, BOOL bFar);

		CHexCoord & 	GetExitHex () { return (m_hexExit); }
		CHexCoord const & GetExitHex () const { return (m_hexExit); }
		int						GetExitDir () const { return (m_iExitDir); }
		CHexCoord			GetShipHex () { return (m_hexShip); }
		CHexCoord const & GetShipHex () const { return (m_hexShip); }
		int						GetShipDir () const { return (m_iShipDir); }

		BOOL			IsFoundationComplete() 			const { return -1 == m_iFoundPer; }
		BOOL			IsSkltnComplete() 			const { return -1 == m_iSkltnPer; }
		int				GetConstDone () const { return m_iConstDone; }
		int				GetFoundPer () const { return m_iFoundPer; }
		int				GetSkltnPer () const { return m_iSkltnPer; }
		int				GetFinalPer () const { return m_iFinalPer; }

		BOOL			IsTopHex( const CHexCoord & ) const;
		BOOL			IsTwoPiece() const;

		CHexCoord 	GetLeftHex( int iDir ) const;
		CHexCoord 	GetTopHex ( int iDir ) const;

		int			GetCX () const { return (m_cx); }
		int			GetCY () const { return (m_cy); }

		BOOL IsInvalidated() const { return GetHex().IsInvalidated(); }
		void SetInvalidated();

protected:
		void					ctor ();
		void					DetermineConstPer ();
		void					DropUnits (int iVehTyp, int iWheelTyp, int iNum, int iNumSkip, int *piTime, int iBlkSiz, CHexCoord const & hex, CHexCoord const & hexUL);
		BOOL					InitRocket (CHexCoord const & hex, BOOL bMe);

		CHexCoord			m_hex;							// CHex we are assigned to
		LONG					m_iBldgDir;

		CHexCoord			m_hexExit;					// for vehicles entering & exiting
		int						m_iExitDir;					// 0 == exit up, 1 == exit right ...
		CHexCoord			m_hexShip;					// for ships entering & exiting
		int						m_iShipDir;
																			//   for iDir == 0
		LONG					m_iLastPer;
		LONG					m_iBuildDone;						// -1 == done, time so far in process

		// remainder from last operate loop
		float					m_fOperMod;					// apply to next operate

		// construction vars
		LONG					m_iConstDone;						// -1 == done, else time building so far
		LONG					m_iFoundTime;						// extra time for foundation on non-plain terrain
		LONG					m_iFoundPer;						// -1 == done, % of foundation done
		LONG					m_iSkltnPer;						// -1 == done, % of bldg skeleton
		LONG					m_iFinalPer;						// -1 == done, % of final bldg 
		LONG					m_iLastMaterialTake;		// eighth of last material taken
		LONG					m_aiRepair [CMaterialTypes::num_build_types];	// points toward repairing
		LONG					m_iRepairWork;					// work done this frame
		LONG					m_iRepairMod;						// left over work

		// these are what the HP on this system sees
		LONG					m_iVisConstDone;
		LONG					m_iVisFoundPer;
		LONG					m_iVisSkltnPer;
		LONG					m_iVisFinalPer;

		// bldg & veh const
		LONG					m_iMatTo;								// % have enough mat for (100 == all)

		LONG					m_cx;										// size in hexes (in case building is turned)
		LONG					m_cy;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CMaterialBuilding - creates materials from materials

class CMaterialBuilding : public CBuilding
{
public:

									CMaterialBuilding () { }
									CMaterialBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { }

		virtual BOOL	IsOperating () const;
		virtual int		GetNextMinuteMat (int iInd) const;
		virtual void	ShowStatusText (CString & str);

		virtual void GetInputs (int * pVals) const;
		virtual void GetAccepts (int * pVals) const { GetInputs (pVals); }

protected:

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CVehicleBuilding - a vehicle factory

class CVehicleBuilding : public CBuilding
{
friend CDlgBuildTransport;
public:

									CVehicleBuilding () { ctor (); }
									CVehicleBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { ctor (); }
									~CVehicleBuilding ();

		virtual void GetInputs (int * pVals) const;
		virtual void GetAccepts (int * pVals) const { GetInputs (pVals); }

		void					DestroyAllWindows ();

		virtual BOOL	IsOperating () const;
		void					UpdateChoices ();
		CDlgBuildTransport *	GetDlgBuild ();
		CDlgBuildTransport *	QueryDlgBuild () { return m_pDlgTransport; }

		void					StartVehicle (int iIndex, int iNum);	// for vehicle factories
		int						GetNum () const { return (m_iNum); }
		void					CancelUnit ();
		void					BuildVehicle ();
		CBuildUnit const *			GetBldUnt () const;
		void					MaterialChange ();
		int						GetNumStatusBars () const { return 3; }
		void					PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					ShowStatusText (CString & str);

									// iInd material needed to finish vehicle building
		int						NeedToFinish (int iInd) const;
		int						GetBldgResReq (int iInd, BOOL bAll) const;
		virtual int		GetNextMinuteMat (int iInd) const;

		void					AssignBldUnit (int iIndex);

		void 					Serialize (CArchive & ar);

protected:
		void					ctor ();

		CDlgBuildTransport *	m_pDlgTransport;

		CBuildUnit const *	m_pBldUnt;				// vehicle building
		LONG								m_aiUsed [CMaterialTypes::num_build_types];	// materials used for this vehicle
		int									m_iNum;						// number to build

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CRepairBuilding - repairs vehicles

class CRepairBuilding : public CBuilding
{
public:

									CRepairBuilding () { ctor (); }
									CRepairBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { ctor (); }
									~CRepairBuilding ();

		CBuildRepair const *	GetData () const;

		virtual BOOL	IsOperating () const;
		virtual void GetInputs (int * pVals) const;
		virtual void GetAccepts (int * pVals) const { GetInputs (pVals); }

		void					BuildRepair ();
		void					RepairVehicle (CVehicle * pVeh);
		void					CancelUnit ();
		void					MaterialChange ();
		int						GetNumStatusBars () const { return 3; }
		void					PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					ShowStatusText (CString & str);

		CVehicle *		GetVehRepairing () { return (m_pVehRepairing); }

		void 					Serialize (CArchive & ar);
		virtual void	FixUp ();

		void					AssignBldUnit (int iIndex);
		void					VehicleLeaving ( CVehicle * pVeh );
		int						GetBldgResReq (int iInd, BOOL bAll) const;
		virtual int		GetNextMinuteMat (int iInd) const { return GetBldgResReq (iInd, FALSE); }

protected:
		void					ctor ();

		CVehicle *		m_pVehRepairing;								// vehicle presently repairing
		CBuildUnit const *	m_pBldUnt;								// data on vehicle we are repairing
		CList <CVehicle *, CVehicle *>	m_lstNext;		// next vehicle to repair

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CShipyardBuilding - builds & repairs ships

class CShipyardBuilding : public CVehicleBuilding
{
public:

									CShipyardBuilding () : CVehicleBuilding () { ctor (); }
									CShipyardBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) :
																	CVehicleBuilding (iBldg, iBldgDir, iOwner, ID) { ctor (); }
									~CShipyardBuilding ();

		CBuildShipyard const *	GetData () const;

		virtual BOOL	IsOperating () const;
		virtual void GetInputs (int * pVals) const;
		virtual void GetAccepts (int * pVals) const { GetInputs (pVals); }

		void					BuildShipyard ();
		void					StartVehicle (int iIndex, int iNum);
		void					RepairVehicle (CVehicle * pVeh);
		void					CancelUnit ();

		void					MaterialChange ();
		int						GetNumStatusBars () const { return 3; }
		void					PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					ShowStatusText (CString & str);

									// iInd material needed to finish vehicle building
		int						NeedToFinish (int iInd) const;
		CVehicle *		GetVehRepairing () { return (m_pVehRepairing); }
		int						GetBldgResReq (int iInd, BOOL bAll) const;
		virtual int		GetNextMinuteMat (int iInd) const { return GetBldgResReq (iInd, FALSE); }

		void 					Serialize (CArchive & ar);
		virtual void	FixUp ();
		void					AssignBldUnit (int iIndex);
		void					VehicleLeaving ( CVehicle * pVeh );

protected:
		void					ctor ();

		CVehicle *		m_pVehRepairing;								// vehicle presently repairing
		CList <CVehicle *, CVehicle *>	m_lstNext;		// next vehicle to repair

		int						m_iMode;
									enum { nothing, build, repair };

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CPowerBuilding - a power plant

class CPowerBuilding : public CBuilding
{
public:

									CPowerBuilding ();
									CPowerBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0);

		virtual BOOL	IsOperating () const;
		void					BuildPower ();
		void					ShowStatusText (CString & str);
		virtual int		GetNextMinuteMat (int iInd) const;

		virtual void GetInputs (int * pVals) const;
		virtual void GetAccepts (int * pVals) const { GetInputs (pVals); }

protected:

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CWarehouseBuilding - stores anything

class CWarehouseBuilding : public CBuilding
{
public:

									CWarehouseBuilding () { }
									CWarehouseBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { }

		virtual void GetAccepts (int * pVals) const;

protected:

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CHousingBuilding - apts & offices

class CHousingBuilding : public CBuilding
{
public:

									CHousingBuilding () { }
									CHousingBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { }

		int						GetNumStatusBars () const { if (m_iConstDone != -1) return (3); return 2; }
		void					PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					ShowStatusText (CString & str);

protected:
};

/////////////////////////////////////////////////////////////////////////////
// CMineBuilding - a mine

class CMineBuilding : public CBuilding
{
public:

									CMineBuilding () { ctor (); }
									CMineBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { ctor (); }
									~CMineBuilding () { UpdateGround (); }

		virtual BOOL	IsOperating () const
												{ if ( ! CBuilding::IsOperating () ) return FALSE;
													return m_iMinerals > 0; }
		static int		TotalQuantity (CHexCoord const & hex, int iTyp, int iDir);
		static int		TotalDensity (CHexCoord const & hex, int iTyp, int iDir);
		void					BuildMine ();
		void					UpdateMine ();		// copy ground data to mine
		void					UpdateGround ();	// copy mine data to ground

		// show what we have mined
		int						GetNumStatusBars () const { if (m_iConstDone != -1) return (3); return 2; }
		void					ShowStatusText (CString & str);

		void 					Serialize (CArchive & ar);

protected:
		void					ctor ();

		LONG					m_iDensity;					// for mines - density of ground underneath
		LONG					m_iMinerals;				//             minerals left
		float					m_fAmountMod;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CFarmBuilding - a farm or lumber mill

// this is how we track land use - 1 per hex
class CLandUse
{
public:
		CLandUse ();
		
		CHexCoord		m_hex;						// hex we are on
		LONG				m_iTimeToNext;		// when we next change
		BYTE				m_iLevelOn;				// tiles go from level 0 to N (harvest)
};

class CFarmBuilding : public CBuilding
{
public:

									CFarmBuilding () { ctor (); }
									CFarmBuilding (int iBldg, int iBldgDir, int iOwner=0, DWORD ID = 0) : 
																	CBuilding (iBldg, iBldgDir, iOwner, ID) { ctor (); }

		static int		LandMult (CHexCoord _hex, int iTyp, int iDir);
		void					BuildFarm ();
		void					UpdateFarm ();

		// lumber for mill, production rate for farm
		int						GetNumStatusBars () const;
		void					PaintStatusBars (CStatInst * pSi, int iNum, CDC * pDc) const;
		void					ShowStatusText (CString & str);

		void 					Serialize (CArchive & ar);

protected:
		void					ctor ();

		LONG					m_iTerMult;											// multiplier for terrain

		CList <CLandUse *, CLandUse *>	m_landUse;		// surrounding tiles we are using
		LONG					m_iTimeToNext;									// time to next change

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// unit maps - we store a map of all instances so we can walk all members and
// find one quickly by dwID

class CBuildingMap : public CMap <DWORD, DWORD, CBuilding *, CBuilding *>
{
public:
	CBuildingMap () { InitHashTable (HASH_INIT); }

	void					Add (CBuilding * pBldg);
	void					Remove (CBuilding * pBldg);

	CBuilding *		GetBldg (DWORD dwID) const;
	CBuilding *		_GetBldg (DWORD dwID) const;
};

// tracks hexes units are on
class CBuildingHex : public CMap <DWORD, DWORD, CBuilding *, CBuilding *>
{
public:
	CBuildingHex () { InitHashTable (HASH_INIT); }

	void					GrabHex (CBuilding const * pBldg);
	void					_GrabHex (int x, int y, CBuilding const * pBldg);
	void					ReleaseHex (CBuilding const * pBldg);
	void					_ReleaseHex (int x, int y);

	CBuilding *		GetBuilding (CHexCoord const & hex) const;
	CBuilding *		GetBuilding (CSubHex const & sub) const;
	CBuilding *		GetBuilding (int x, int y) const;

	CBuilding *		_GetBuilding (CHexCoord const & hex) const;
	CBuilding *		_GetBuilding (CSubHex const & sub) const;
	CBuilding *		_GetBuilding (int x, int y) const;

protected:
	DWORD					ToArg (int x, int y) const;
	DWORD					_ToArg (int x, int y) const;
};


extern CStructure theStructures;
extern CBuildingMap theBuildingMap;
extern CBuildingHex theBuildingHex;
extern CStructureType theStructureType;


extern void SerializeElements (CArchive & ar, CBuilding * * ppBldg, int nCount);


#endif
