#ifndef __LOADTRUK_H__
#define __LOADTRUK_H__

// loadtruk.h : header file
//

class CVehicle;


/////////////////////////////////////////////////////////////////////////////
// CDlgLoadTruck dialog

class CDlgLoadTruck : public CDialog
{
// Construction
public:
	CDlgLoadTruck(CWnd* pParent = NULL, CVehicle * pVehPar = NULL);   // standard constructor

	void		UpdateLimits ();
	void		Transfer (int iType, int iAmount);

// Dialog Data
	//{{AFX_DATA(CDlgLoadTruck)
	enum { IDD = IDD_TRUCK };
	CWndOD< CButton >	m_btnAuto;
	CWndOD< CButton >	m_btnOk;
	CSpinButtonCtrl	m_spnCopper;
	CSpinButtonCtrl	m_spnSteel;
	CSpinButtonCtrl	m_spnOil;
	CSpinButtonCtrl	m_spnLumber;
	CSpinButtonCtrl	m_spnIron;
	CSpinButtonCtrl	m_spnCoal;
	CString		m_sCoal;
	CString		m_sIron;
	CString		m_sLumber;
	CString		m_sOil;
	CString		m_sSteel;
	CString		m_sCopper;
	CString		m_sMax;
	int		m_iMaxCoal;
	int		m_iMaxIron;
	int		m_iMaxLumber;
	int		m_iMaxOil;
	int		m_iMaxSteel;
	int		m_iMaxCopper;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgLoadTruck)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void			PostNcDestroy () { delete this; }
	//}}AFX_VIRTUAL

// Implementation
protected:
		void		InitData (CSpinButtonCtrl * pSpn, CString & sCtrl, int ID, int iOn, CBuilding * pBldg, int & iMax);
		void		UpdateItemLimit (CSpinButtonCtrl * pSpn, int iOn, CBuilding * pBldg, int & iMax);

	CVehicle *	m_pVehPar;
	CBuilding *	m_pBldgPar;

	// Generated message map functions
	//{{AFX_MSG(CDlgLoadTruck)
	virtual BOOL OnInitDialog();
	afx_msg void OnTruckLoadBldg();
	afx_msg void OnTruckLoadVeh();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnTruckAuto();
	afx_msg void OnTruckUnload();
	afx_msg void OnChangeItem();
	afx_msg void OnDestroy();
	afx_msg void OnTruckLoad();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
