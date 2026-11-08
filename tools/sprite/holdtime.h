#ifndef HOLDTIME_H
#define HOLDTIME_H

#include "resource.h"

class CHoldTime : public CDialog
{
// Construction
public:
	CHoldTime(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHoldTime)
	enum { IDD = IDD_HOLDTIME };
	int		m_holdTime;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHoldTime)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHoldTime)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif //  HOLDTIME_H
