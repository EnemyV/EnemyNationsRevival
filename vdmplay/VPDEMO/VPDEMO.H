// vpdemo.h : main header file for the VPDEMO application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVpdemoApp:
// See vpdemo.cpp for the implementation of this class
//

union TransportData
{
	VPNETBIOSDATA	nb;
	VPTCPDATA		tcp;
	VPIPXDATA		ipx;
};

class CVpdemoApp : public CWinApp
{
public:
	CVpdemoApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVpdemoApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVpdemoApp)
	afx_msg void OnProtocolTcp();
	afx_msg void OnProtocolNetbios();
	afx_msg void OnProtocolIpx();
	afx_msg void OnProtocolTapi();
	afx_msg void OnProtocolModem();
	afx_msg void OnProtocolDp();
	afx_msg void OnProtocolComm();
	afx_msg void OnVpStartup();
	afx_msg void OnProtocolVpstartup();
	afx_msg void OnProtocolVpenumses();
	afx_msg void OnProtocolAdvancedCl();
	afx_msg void OnProtocolAdvancedSrv();
	afx_msg void OnCfgsim();
	afx_msg void OnGetserveraddr();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void SelectProtocol(UINT itemId, UINT transport);

	UINT	m_transport;
	TransportData* m_transportData;
};


/////////////////////////////////////////////////////////////////////////////
