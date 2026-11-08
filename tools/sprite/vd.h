// vd.h : header file
//
#ifndef VD_H
#define VD_H

#include "sprd.h"

class CVD : public CSprite
{
protected:
	CVD();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CVD)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVD)
	public :
	virtual BOOL OnOpenDocument( LPCTSTR lpszPathName );
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVD();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CVD)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
