// sprd.h : header file
//
#ifndef SPRD_H
#define SPRD_H

#include "sp.h"

/////////////////////////////////////////////////////////////////////////////
// CSprite document

class CSprite : public CDocument
{
protected:
	CSprite();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSprite)

// Attributes
public:
	//  Pointers to the sprites for this document - each document
	//  has four sprites, one for each supported bit depth.
	Sprite *	pSprite8;
#ifdef CREATE15
	Sprite *	pSprite15;
#endif
#ifdef CREATE16
	Sprite *	pSprite16;
#endif
	Sprite *	pSprite24;
#ifdef CREATE32
	Sprite *	pSprite32;
#endif
	Sprite		*pCurrentSprite;
	char		aDirectory[ MAX_PATH ];

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprite)
	public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL
 	void		OnAnimateSprite( void );

// Implementation
public:
	virtual ~CSprite();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CSprite)
	afx_msg void OnFileSavesprite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
