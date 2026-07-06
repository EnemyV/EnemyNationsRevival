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
					// Fuel Efficiency 1-10 (in-code, unlocked after gas_turbine): the first ten
					// levels of a 16-level line; cost doubles per level up to 32*B at level 6 then
					// goes flat +16*B per level (L7=48B..L16=192B, B=gas_turbine cost), while
					// cutting gas consumption on a diminishing curve to a 30% total saving at
					// level 10 (increments 5/4/4/3/3/3/2/2/2/2), then +1% per level to 36% at 16.
					// Levels 11-12 and 13-16 (+1% each) are appended at the END of the enum for
					// save parity. Named in research.cpp. Appended last so all the earlier
					// indices (incl. bridge_2 / RDPATH_SAVE_COUNT==53) stay put.
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
					// fracking is toggled ON (at +50% well energy +1 flat). Flat oil/min by tier
					// (5/7/9/11/13) via CPlayer::GetFrackOilPerMin. T1<-gas_turbine, T2-5 chain
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
					// Coal Liquefaction (in-code) -- 1 tier. A coal POWER PLANT, when its
					// alt-output toggle is ON, also converts 2 coal -> 1 oil (the AltOutput
					// system, eRatioConsume mode). Gated on this single tech; cost ~ a few x
					// the gas_turbine tech, chained off gas_turbine. Appended LAST so all
					// earlier indices (incl. bridge_2 / RDPATH_SAVE_COUNT==53) stay put.
					coal_liquefaction,
					// Charcoal (in-code) -- 4 tiers. A lumber MILL (UTfarm whose GetTypeFarm()
					// == lumber, i.e. the sawmill), once charcoal_1 is researched and its
					// alt-output toggle is ON, runs a kiln: harvested lumber is converted into
					// coal ("Charcoal" label only) at a fixed 2 lumber -> 1 coal (AltOutput
					// eRatioConsume), MODE-SWITCH (lumber output stops while the kiln runs). The
					// THROUGHPUT is tier-scaled: T1 enables it at a VERY LOW rate; T2-4 raise it
					// (CPlayer::GetCharcoalPct). T1 chained off gas_turbine; T2-4 chain the prior
					// tier. No energy cost. Appended LAST so all earlier indices (incl. bridge_2 /
					// RDPATH_SAVE_COUNT==53) stay put.
					charcoal_1,
					charcoal_2,
					charcoal_3,
					charcoal_4,
					// Fuel Efficiency 11-12 (in-code) — the two TOP levels of the fuel line
					// (levels 1-10 live above at fuel_efficiency_1..10). Split out here and
					// appended LAST so adding them does NOT shift any earlier enum index (old
					// saves store discovered-flags positionally; see player.cpp Serialize).
					// They continue the x2 cost chain (11<-fe_10, 12<-fe_11) and each add only
					// +1% gas saving (30% at level 10 -> 31% -> 32%), see CPlayer::GetFuelPct.
					fuel_efficiency_11,
					fuel_efficiency_12,
					// Vehicle Speed 11-12 (in-code) — two MORE speed levels beyond the base
					// vehicle_speed_1..10 line above. Each adds only +1% move speed (vs +2% for
					// levels 1-10; see CPlayer::GetSpeedPct) and chains off the previous speed
					// tier (11<-10, 12<-11). The WHOLE 12-tier speed line is a PREMIUM pure-
					// doubling cost curve (B * 2^(tier-1), NO flat cap; B = gas_turbine cost) —
					// unlike Fuel Efficiency, which caps at 32*B then goes flat. So speed keeps
					// escalating (tier 10=512*B, 11=1024*B, 12=2048*B), pricing the late tiers
					// for how deep they unlock. Appended LAST so no earlier enum index shifts —
					// old saves store the discovered-flags positionally.
					vehicle_speed_11,
					vehicle_speed_12,
					// Radar/Spotting tiers 6-7 (in-code) — two MORE diminishing-return sight
					// levels beyond spot_4/5 above; each 2x the previous tier's research cost and
					// chains off it (spot_6<-spot_5, spot_7<-spot_6). Sight bonus in
					// CUnit::AssignData (spot_6 ~+71.9%, spot_7 ~+73.4%); level lookup in
					// CPlayer::SetRsrch. Appended LAST so no earlier enum index shifts (old saves
					// store discovered-flags positionally; see player.cpp Serialize).
					spot_6,
					spot_7,
					// Fuel Efficiency 13-16 (in-code): 4 more +1% tiers appended for save parity.
					// Cost continues the flat +16*B ramp; counted in CPlayer::GetFuelPct.
					fuel_efficiency_13,
					fuel_efficiency_14,
					fuel_efficiency_15,
					fuel_efficiency_16,
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
