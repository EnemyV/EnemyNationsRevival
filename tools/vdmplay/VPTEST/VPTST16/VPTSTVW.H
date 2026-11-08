// vptstvw.h : interface of the CVptst16View class
//
/////////////////////////////////////////////////////////////////////////////

class CVptst16View : public CView
{
protected: // create from serialization only
	CVptst16View();
	DECLARE_DYNCREATE(CVptst16View)

// Attributes
public:
	CVptst16Doc* GetDocument();

// Operations
public:

// Implementation
public:
	virtual ~CVptst16View();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	
// Generated message map functions
protected:
	//{{AFX_MSG(CVptst16View)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vptstvw.cpp
inline CVptst16Doc* CVptst16View::GetDocument()
   { return (CVptst16Doc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
