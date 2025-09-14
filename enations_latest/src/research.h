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


class CDlgDiscover;


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

	CString		m_sName;								// name of item
	CString		m_sDesc;								// description in choose dialog
	CString		m_sResult;							// description in discovered dialog

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
					num_types	};

	CRsrchArray () {}

	void			Open ();
	void			Close ();
};

class CResearchListBox : public CListBox
{
  public:
    virtual int CompareItem( LPCOMPAREITEMSTRUCT lpCompareItemStruct );
};

/////////////////////////////////////////////////////////////////////////////
// CDlgResearch dialog

class CDlgResearch : public CDialog
{
// Construction
public:
	CDlgResearch(CWnd* pParent = NULL);   // standard constructor

	void		UpdateChoices (BOOL bCheck);
	void		ItemDiscovered (int iItem);
	void		UpdateProgress ();

	// Dialog Data
	//{{AFX_DATA(CDlgResearch)
	enum { IDD = IDD_RESEARCH };
	CWndOD< CButton >	m_btnDiscovery;
	CListBox	m_lstRsrch;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgResearch)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:

	CDlgDiscover *	m_pDlgDiscvr;
	int							m_iRsrchNum;
	int							m_iPerDone;
	CString					m_strDesc;
	CDIB *					m_pDib;
	CDIB *					m_pDibBtn;
	CStatInst				m_statInst;				// data to draw the icons
	CFont						m_fntList;
	BOOL						m_bShareLimit;

	// Generated message map functions
	//{{AFX_MSG(CDlgResearch)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkRsrchList();
	afx_msg void OnSelchangeRsrchList();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnRsrchFound();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern void ConstructElements (CRsrchStatus * pNewElem, int iCount);
extern void DestructElements (CRsrchStatus * pNewElem, int iCount);
extern void ConstructElements (CRsrchItem * pNewElem, int iCount);
extern void DestructElements (CRsrchItem * pNewElem, int iCount);


extern CRsrchArray theRsrch;


/////////////////////////////////////////////////////////////////////////////
// CDlgDiscover dialog

class CDlgDiscover : public CDialog
{
// Construction
public:
	CDlgDiscover(CWnd* pParent = NULL);   // standard constructor

	void		ItemDiscovered (int iItem);

// Dialog Data
	//{{AFX_DATA(CDlgDiscover)
	enum { IDD = IDD_RSRCH_FOUND };
	CString	m_strText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDiscover)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy() { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:
	void		NewItem ();

	int			m_iRsrchNum;

	// Generated message map functions
	//{{AFX_MSG(CDlgDiscover)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
