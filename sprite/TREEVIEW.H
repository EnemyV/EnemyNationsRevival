// treeview.h : header file
//
#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "sptrctrl.h"

class CTView : public CScrollView
{
protected:
	CTView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTView)

// Attributes
public:
	SpriteTreeCtrl	m_treeCtl;

// Operations
public:
//	HTREEITEM AddOneItem( HTREEITEM hParent, char *szItemText, DWORD userData, HTREEITEM hInsertAfter );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg void OnSelChanged( NMHDR *pNotifyStruct, LRESULT *pResult );
  	DECLARE_MESSAGE_MAP()
};

#endif
