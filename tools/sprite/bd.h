// sprd.h : header file
//
#ifndef BD_H
#define BD_H

#include "sprd.h"

/////////////////////////////////////////////////////////////////////////////
// CSprite document

class CBD : public CSprite
{
protected:
	CBD();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBD)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBD)
	public :
	virtual BOOL OnOpenDocument( LPCTSTR lpszPathName );
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBD();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CBD)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
