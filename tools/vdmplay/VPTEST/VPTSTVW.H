// vptstvw.h : interface of the CVptstView class
//
/////////////////////////////////////////////////////////////////////////////

class CVptstView : public CView
{
protected: // create from serialization only
	CVptstView();
	DECLARE_DYNCREATE(CVptstView)

// Attributes
public:
	CVptstDoc* GetDocument();

// Operations
public:

// Implementation
public:
	virtual ~CVptstView();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CVptstView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vptstvw.cpp
inline CVptstDoc* CVptstView::GetDocument()
   { return (CVptstDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
