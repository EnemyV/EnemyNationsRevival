// PalDoc.h : header file
//
#ifndef PALDOC_H
#define PALDOC_H

#include "quant.h"

class CPalDoc : public CDocument
{
protected:
	CPalDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CPalDoc)

	int CountFiles( const char *pCurrentPath );
	bool ProcessFiles( const char *pCurrentPath, ColorQuant *pQuant, bool bTerrain );

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPalDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPalDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CPalDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
