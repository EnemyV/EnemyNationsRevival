//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __CCHATBAR_HPP__
#define __CCHATBAR_HPP__

////////////////////////////////////////////////////////////////////////////
//
//  chatbar.hpp : generic toolbar for use
//                Divide and Conquer
//               
//  Last update:    08/22/95
//
////////////////////////////////////////////////////////////////////////////


#include <afxext.h>

/////////////////////////////////////////////////////////////////////////////
// CChatBar dialog

class CChatBar : public CDialogBar
{
	CWnd *m_pParent;	// parent window

// Construction
public:
	CChatBar(CWnd* pParent = NULL);   // standard constructor

	void InitBar();

// Implementation
protected:
	afx_msg void OnSize( UINT nType, int cx, int cy );
	virtual void PostNcDestroy();

	DECLARE_MESSAGE_MAP()
};

#endif // __CCHATBAR_HPP__
