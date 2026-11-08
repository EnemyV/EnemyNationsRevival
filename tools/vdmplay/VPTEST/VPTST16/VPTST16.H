// vptst16.h : main header file for the VPTST16 application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CVptst16App:
// See vptst16.cpp for the implementation of this class
//

class CVptst16App : public CWinApp
{
public:
	CVptst16App();

// Overrides
	virtual BOOL InitInstance();

// Implementation

	//{{AFX_MSG(CVptst16App)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
