// td.h : header file
//
#ifndef TD_H
#define TD_H

#include "sprd.h"

/////////////////////////////////////////////////////////////////////////////
// CTD document

class CTD : public CSprite
{
protected:
	CTD();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTD)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTD)
	public :
	virtual BOOL OnOpenDocument( LPCTSTR lpszPathName );
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTD();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTD)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
