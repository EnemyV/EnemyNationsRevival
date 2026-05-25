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
//  CChatBar — excluded from build (Phase 2d-cont, 2026-05-11). The MFC
//  declaration is kept for historical reference but not compiled. The
//  header is only transitively included via ipcchat.h, which is itself
//  only included by the excluded ipcchat.cpp.
//
////////////////////////////////////////////////////////////////////////////

class CChatBar;

#if 0  // MFC_LEGACY_CHATBAR

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

#endif // MFC_LEGACY_CHATBAR

#endif // __CCHATBAR_HPP__
