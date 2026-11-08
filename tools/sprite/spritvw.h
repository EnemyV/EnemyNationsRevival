// spritvw.h : interface of the CSpriteView class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef SPRITVW_H
#define SPRITVW_H

#include "sprd.h"

#include "drawdib.h"

class CSpriteView : public CScrollView
{
protected: // create from serialization only
	CSpriteView();
	DECLARE_DYNCREATE(CSpriteView)

// Attributes
private :
	DrawDIB		drawsurf;

public:
	CSprite* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpriteView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSpriteView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSpriteView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in spritvw.cpp
inline CSprite* CSpriteView::GetDocument()
   { return (CSprite*)m_pDocument; }
#endif

#endif
