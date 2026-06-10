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
