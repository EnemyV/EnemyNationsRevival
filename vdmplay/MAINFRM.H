// mainfrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void SelectTransport(UINT id, int vpTransport);
	void EnableVpItems(BOOL flag);
	
// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg void OnTransportsIpx();
	afx_msg void OnTransportsNetbios();
	afx_msg void OnTransportsTcp();
	afx_msg void OnVdmplayCleanup();
	afx_msg void OnVdmplayCreatesession();
	afx_msg void OnVdmplayEnumsessions();
	afx_msg void OnVdmplayGetversion();
	afx_msg void OnVdmplayJoinsesionn();
	afx_msg void OnVdmplayStartup();
	afx_msg void OnVdmplayStopenumsessions();
	afx_msg void OnVdmplaySupportedprotocols();
	afx_msg LONG OnVpNotify(WPARAM w, LPARAM l);
	afx_msg void OnVdmplayClosesession();
	afx_msg void OnTransportsComm();
	afx_msg void OnTransportsModem();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
