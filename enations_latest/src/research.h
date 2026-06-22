//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __RESEARCH_H__
#define __RESEARCH_H__

// research.h : header file
//

#include "resource.h"
#include "icons.h"


// CDlgDiscover removed (replaced by SDL2DiscoverDialog)


void ResearchDiscovered (int iRsrch);


/////////////////////////////////////////////////////////////////////////////
// CRsrchStatus - status of our research

class CRsrchStatus : public CObject
{
public:
		CRsrchStatus ();

		virtual void	Serialize (CArchive & ar);

	BYTE			m_bDiscovered;					// TRUE if has been discovered
	LONG			m_iPtsDiscovered;				// points researched so far

};


/////////////////////////////////////////////////////////////////////////////
// CRsrchItem - data about each R&D item

class CRsrchItem : public CObject
{
public:
		CRsrchItem ();
		virtual ~CRsrchItem ();

	int				m_iPtsRequired;					// points required to discover
	int *			m_piRsrchRequired;			// other items that must be researched first
	int				m_iNumRsrchRequired;
	int *			m_piBldgsRequired;			// buildings that must be built first
	int				m_iNumBldgsRequired;
	int				m_iScenarioReq;					// cannot be discovered till this scenario

	std::string	m_sName;							// name of item
	std::string	m_sDesc;							// description in choose dialog
	std::string	m_sResult;						// description in discovered dialog

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CRsrchArray : public CArray <CRsrchItem, CRsrchItem *>
{
public:
	enum {	nothing,
					balloons,
					gliders,
					prop_planes,
					jet_planes,
					rockets,
					sailboats,
					motorboats,
					cargo_handling,
					fire_control,
					landing_craft,
					heavy_naval,
					medium_vehicle,
					heavy_vehicle,
					armored_vehicle,
					artillery,
					tanks,
					medium_facilities,
					large_facilities,
					advanced_facilities,
					fortification,
					radio,
					mail,
					email,
					telephone,
					gas_turbine,
					nuclear,
					bridge,
					const_1,
					const_2,
					const_3,
					manf_1,
					manf_2,
					manf_3,
					mine_1,
					mine_2,
					farm_1,
					spot_1,
					spot_2,
					spot_3,
					range_1,
					range_2,
					range_3,
					atk_1,
					atk_2,
					atk_3,
					def_1,
					def_2,
					def_3,
					copper,
					acc_1,
					acc_2,
					acc_3,
					// In-code topics (not in the DAT file) — appended by CRsrchArray::Open
					// after the RSRH load. Each bridge tier doubles the points cost of the
					// previous and extends the max bridge span by +25% of the base span
					// (see CPlayer::GetMaxSpan). MUST stay last: CAIGoalMgr serializes its
					// RDPath blob at the legacy pre-tier count (see RDPATH_SAVE_COUNT).
					bridge_2,
					bridge_3,
					bridge_4,
					bridge_5,
					// Cargo Handling line 2-4 (in-code, mirroring the bridge tiers): each
					// doubles the previous tier's cost and adds +10% truck cargo capacity over
					// the base cargo_handling research (see CPlayer::GetCargoPct). Named
					// Servo-Loaders / Modular Cargo Pods / Grav-Assisted Hauling in research.cpp.
					// Appended AFTER the bridge tiers so bridge_2 keeps its index
					// (RDPATH_SAVE_COUNT==53).
					cargo_handling_2,
					cargo_handling_3,
					cargo_handling_4,
					// Fuel Efficiency 1-10 (in-code, unlocked after gas_turbine): a 10-level
					// repeatable line, each level costing double the previous in points and
					// cutting gas consumption by 5% of what remains (diminishing; see
					// CPlayer::GetFuelPct). Named in research.cpp. Appended last so all the
					// earlier indices (incl. bridge_2 / RDPATH_SAVE_COUNT==53) stay put.
					fuel_efficiency_1,
					fuel_efficiency_2,
					fuel_efficiency_3,
					fuel_efficiency_4,
					fuel_efficiency_5,
					fuel_efficiency_6,
					fuel_efficiency_7,
					fuel_efficiency_8,
					fuel_efficiency_9,
					fuel_efficiency_10,
					// Pontoon Bridges (in-code): an EARLY, cheap bridge tech that unlocks
					// bridge building at HALF the span of the full Bridges tech. Kept
					// INDEPENDENT of (not a prereq of) the base bridge tech, so the AI's
					// frozen research path can still reach full Bridges. See
					// CPlayer::CanBridge / GetMaxSpan. Appended last to keep earlier
					// indices (incl. bridge_2 / RDPATH_SAVE_COUNT==53) put.
					bridge_short,
					// Vehicle Speed 1-10 (in-code): a 10-level line that boosts vehicle
					// movement +2% per level (see CPlayer::GetSpeedPct). Gated off the Fuel
					// Efficiency line: level 1 needs the first two fuel-efficiency techs, and
					// each later level needs the previous speed level plus the next fuel
					// level. Appended last to keep earlier indices put.
					vehicle_speed_1,
					vehicle_speed_2,
					vehicle_speed_3,
					vehicle_speed_4,
					vehicle_speed_5,
					vehicle_speed_6,
					vehicle_speed_7,
					vehicle_speed_8,
					vehicle_speed_9,
					vehicle_speed_10,
					// Radar/Spotting tiers 4-5 (in-code) — extend the DAT spot_1..3 line with two
					// diminishing-return levels; each 2x the previous tier's research cost. Appended
					// LAST so all earlier indices (incl. bridge_2 / RDPATH_SAVE_COUNT==53) stay put.
					// Per-level sight bonus is applied in CUnit::AssignData (spot_4 ~+62.5%, spot_5 ~+68.75%).
					spot_4,
					spot_5,
					// Landing Craft capacity tiers 2-3 (in-code) — each adds +1 to the landing
					// craft's unit hold over the base 2 (so 2 -> 3 -> 4). Fairly expensive;
					// chain off the base landing_craft tech (lc_2<-landing_craft, lc_3<-lc_2).
					// Bonus applied in CVehicle::GetEffPeopleCarry via CPlayer::GetLandingCraftBonus.
					// Appended LAST so all earlier indices (incl. bridge_2 / RDPATH_SAVE_COUNT==53) stay put.
					landing_craft_2,
					landing_craft_3,
					// Fracking (#23, in-code) — 5 tiers. EXHAUSTED oil wells trickle oil when
					// fracking is toggled ON (at +50% well energy). Flat oil/min by tier
					// (10/15/20/25/30) via CPlayer::GetFrackOilPerMin. T1<-gas_turbine, T2-5 chain
					// the prev tier + a Fuel-Efficiency level. Appended LAST so earlier indices
					// (incl. bridge_2 / RDPATH_SAVE_COUNT==53) stay put; old saves load via the
					// count-prefixed research-status read (player.cpp ~910) + auto-resize.
					fracking_1,
					fracking_2,
					fracking_3,
					fracking_4,
					fracking_5,
					// BioFuel (#33, in-code) — 6 tiers. Farms also produce oil (the existing
					// `oil` resource; "Bio Oil" label in farm UI only) when toggled ON, as a % of
					// food output (10/12/14/16/18/20%) via CPlayer::GetBioOilPct. T1 gated on
					// farming + gas_turbine + some Fuel-Efficiency + Vehicle-Speed; T2-6 chain the
					// prev tier. Appended LAST (same save-parity reason as above).
					biofuel_1,
					biofuel_2,
					biofuel_3,
					biofuel_4,
					biofuel_5,
					biofuel_6,
					num_types	};

	CRsrchArray () {}

	void			Open ();
	void			Close ();
};

// CDlgResearch + CResearchListBox removed (Phase 2d) — replaced by
// SDL2ResearchDialog. The CRsrchArray / CRsrchItem / CRsrchStatus state classes
// below remain (used by AI research progression and serialization).

extern void ConstructElements (CRsrchStatus * pNewElem, int iCount);
extern void DestructElements (CRsrchStatus * pNewElem, int iCount);
extern void SerializeElements( CArchive& ar, CRsrchStatus* pNewElem, int iCount );
extern void ConstructElements (CRsrchItem * pNewElem, int iCount);
extern void DestructElements (CRsrchItem * pNewElem, int iCount);


extern CRsrchArray theRsrch;


// CDlgDiscover removed (replaced by SDL2DiscoverDialog)

#endif
