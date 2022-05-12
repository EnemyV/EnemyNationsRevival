#ifndef SPTRCTRL_H
#define SPTRCTRL_H

typedef void (*TTreeActFunctionPtr)( TV_ITEM, void * );

class SpriteTreeCtrl : public CTreeCtrl
{
// Construction
public:
	SpriteTreeCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SpriteTreeCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~SpriteTreeCtrl();
	afx_msg void OnSPTreeProp();
	HTREEITEM AddNewItem( HTREEITEM hParent, const char *pszItemText, void *pUserData, HTREEITEM hInsertAfter );
	void DoForSubtree( HTREEITEM hItem, TTreeActFunctionPtr pAction, void *pUserData );

	// Generated message map functions
protected:
	//{{AFX_MSG(SpriteTreeCtrl)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
