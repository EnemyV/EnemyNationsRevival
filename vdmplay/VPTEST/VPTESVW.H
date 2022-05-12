// vptesvw.h : interface of the CVptestView class
//
/////////////////////////////////////////////////////////////////////////////

class CVptestView : public CView
{
protected: // create from serialization only
	CVptestView();
	DECLARE_DYNCREATE(CVptestView)

// Attributes
public:
	CVptestDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVptestView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVptestView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CVptestView)
	afx_msg void OnVdmplayCleanup();
	afx_msg void OnVdmplayClosesession();
	afx_msg void OnVdmplayCreatesession();
	afx_msg void OnVdmplayEnumsessions();
	afx_msg void OnVdmplayJoinsession();
	afx_msg void OnVdmplaySendmessage();
	afx_msg void OnVdmplayStartup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vptesvw.cpp
inline CVptestDoc* CVptestView::GetDocument()
   { return (CVptestDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
