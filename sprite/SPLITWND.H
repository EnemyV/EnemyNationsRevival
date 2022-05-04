// splitwnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSplitWnd frame with splitter

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CSplitWnd : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CSplitWnd)
protected:
	CSplitWnd();           // protected constructor used by dynamic creation

// Attributes
protected:
	CSplitterWnd    m_wndSplitter;
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitWnd)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSplitWnd();

	// Generated message map functions
	//{{AFX_MSG(CSplitWnd)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
