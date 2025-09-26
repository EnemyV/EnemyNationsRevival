//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __UNIT_WND_H__
#define __UNIT_WND_H__

#include <wndbase.h>

#include "resource.h"
#include "bmbutton.h"
#include "unit.h"


class CBuildUnit;
class CVehicleBuilding;
class CStructureData;


/////////////////////////////////////////////////////////////////////////////
// CWndRoute

class CWndRoute : public CWndBase
{
// Construction
public:
	enum { NUM_ROUTE_BTNS = 7 };

	CWndRoute (CVehicle * pVeh);
	virtual ~CWndRoute () {}

	void		Create (CWndArea * pPar);


// Attributes
public:
	void	NewRoute (CVehicle *pVeh);
	void	CallGotoOn (int iMode);
	void	BtnPressed ();
	void	Invalidate ();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndRoute)
	protected:
	virtual void			PostNcDestroy () { delete this; }
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
protected:
	CVehicle *	m_pVeh;
	int					m_iXmin;
	int					m_iYmin;

	CListBox		m_listbox;
	CTextButton	m_Btns[NUM_ROUTE_BTNS];

	void		VehDesc (CVehicle *pVeh, POSITION & pos, CString & sDesc);

	//{{AFX_MSG(CWndRoute)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void Waypoint ();
	afx_msg void Unload ();
	afx_msg void Load ();
	afx_msg void Goto ();
	afx_msg void Delete ();
	afx_msg void Start ();
	afx_msg void Auto ();
	afx_msg	void OnLbnClk ();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CWndListUnits window

class CWndListUnits : public CWndPrimary
{
// Construction
public:
	CWndListUnits();

// Attributes
public:

// Operations
public:
	void		AddToList (CUnit *pUnit);
	void		RemoveFromList (CUnit *pUnit);
	void		RemoveAll () { m_ListBox.ResetContent (); }

	int			FindItem (CUnit const * pUnit);

// Implementation
public:
	virtual ~CWndListUnits() {}

public:
	CListBox		m_ListBox;
	CDIB *			m_pDib;
	CDIB *			m_pDibBack;			// background bitmaps
	CDIB *			m_pDibUnit;			// building or vehicle bitmap
	int					m_iStatHt;

protected:
	// Generated message map functions
	//{{AFX_MSG(CWndListUnits)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg	void OnLbnClk ();
	afx_msg	void OnLbnDblClk ();
	afx_msg int OnCompareItem(int nIDCtl, LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CWndListBuildings window

class CWndListBuildings : public CWndListUnits
{
// Construction
public:
					CWndListBuildings() {}
	void		Create ();

// Attributes
public:

// Operations
public:

// Implementation
public:
	virtual ~CWndListBuildings() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CWndListBuildings)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CWndListVehicles window

class CWndListVehicles : public CWndListUnits
{
// Construction
public:
					CWndListVehicles() {}
	void		Create ();

// Attributes
public:

// Operations
public:

// Implementation
public:
	virtual ~CWndListVehicles() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CWndListVehicles)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CUnitButton window

class CUnitButton : public CButton
{
// Construction
public:
	CUnitButton ();
	virtual ~CUnitButton () {}

	BOOL		Create ( char const * psText, CRect & rect, CWnd *pPar, int ID, int IDGroup, CDIB * pBack, CDIB * pBtn, CDIB * pOvrly );

	void		SetNum ( int iNum );

// Attributes
public:
	void *	m_pData;		// data we can assign to each

// Operations
public:
	void		SetToggleState (BOOL bDown);
	int			GetToggleState () const { return (m_cState & 0x01); }

// Implementation
public:
	void 	DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	// Generated message map functions
	//{{AFX_MSG(CMyButton)
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CDIB *				m_pDib;					// scratch bitmap to build up image
	CDIB *				m_pBackDib;			// bitmap that is the background (for transparent parts)
	CDIB *				m_pBtnDib;			// bitmap of buttons
	CDIB *				m_pOvrlyDib;		// art to put on button
	int						m_iOvrlyNum;		// overlay number in bitmap
	int						m_IDGroup;			// for notification messages

	BYTE					m_cState;				// if button is down or up & if it's toggleable
							enum { down = 0x01 };
};


/////////////////////////////////////////////////////////////////////////////
// CDlgBuildStructure dialog

class CDlgBuildStructure : public CDialog
{
// Construction
public:
	CDlgBuildStructure (CWnd *, CVehicle *);	// standard constructor
	void		Create (CWnd *);
	void		UpdateChoices ();

// Dialog Data
	//{{AFX_DATA(CDlgBuildStructure)
	enum { IDD = IDD_BUILD_STRUCTURE };
	CWndOD< CButton >	m_btnBuild;
	//}}AFX_DATA

// Implementation
protected:
	BOOL			CanBuild (int iIndex, CStructureData const * pSd);

	CStructureData const * m_pSd;
	CString		m_strDesc;

	CString		m_sCost;
	CString		m_sHave;
	CString		m_sNeed;
	CString		m_sTime;
	CString		m_sOper;
	CString		m_sPeople;
	CString		m_sPower;

	CVehicle *	m_pVehPar;

	int				m_iCatOn;
	int				m_iBldgOn;
	CUnitButton		m_btnCat [6];
	CUnitButton		m_btnBldg [6];

	// for drawing the listboxes
	CDIB *		m_pDibBkgnd;		// build up background (back & text)
	CDIB *		m_pDibBtn;

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgBuildStructure)
	protected:
	virtual void			PostNcDestroy () { delete this; }
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CDlgBuildStructure)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg LRESULT OnSelChange (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDblClk (WPARAM wParam, LPARAM lParam);
	afx_msg void OnSelchangeBuildListCat();
	afx_msg void OnSelchangeBuildListBldgs();
	afx_msg void OnDblclkBuildListBldgs();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CDlgBuildVehicle dialog

class CDlgBuildTransport : public CDialog
{
// Construction
public:
	CDlgBuildTransport (CWnd *, CVehicleBuilding *);	// standard constructor
	void		Create (CWnd *);
	void		UpdateChoices ();
	void		UpdateStatus ( int iPer );

// Dialog Data
	//{{AFX_DATA(CDlgBuildTransport)
	enum { IDD = IDD_BUILD_TRANSPORT };
	CEdit	m_edtNum;
	CSpinButtonCtrl	m_spnNum;
	CWndOD< CButton >	m_btnBuild;
	CString			m_sNum;
	//}}AFX_DATA

// Implementation
protected:
	CBuildUnit const * m_pBu;
	CString		m_strDesc;

	int				m_iNum;
	CString		m_sCost;
	CString		m_sHave;
	CString		m_sNeed;
	CString		m_sTime;
	CString		m_sPeople;

	CVehicleBuilding *	m_pBldgPar;
	CStatInst				m_statInst;				// data to draw the icons

	int				m_iVehOn;
	CUnitButton		m_btnVeh [6];

	CDIB * 		m_pDibBkgnd;		// for building up background
	CDIB *		m_pDibBtn;			// for building ok/cancel buttons

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgBuildTransport)
	protected:
	virtual void			PostNcDestroy () { delete this; }
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CDlgBuildTransport)
	afx_msg void OnChangeNum();
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnSelChange (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDblClk (WPARAM wParam, LPARAM lParam);
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnSelchangeBuildList();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  protected:
    bool m_bInitComplete = false;
};


#endif
