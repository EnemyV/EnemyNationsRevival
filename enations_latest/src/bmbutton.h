//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __BMBUTTON_H__
#define __BMBUTTON_H__

#include "msgs.h"


void FitDrawText ( CDC * pDc, CFont & fnt, CString & sText, CRect & rect );
void PaintBorder ( CDIB * pDib, CRect const & rect, BOOL bBack );
void PaintBevel (CDC & dc, CRect & rEdge, unsigned uWid, CBrush & brTop, CBrush & brBottom);

								 
/////////////////////////////////////////////////////////////////////////////
// CBmBtnData

class CBmBtnData
{
public:
	CBmBtnData ();
	~CBmBtnData ();
	void				Init (CMmio * pMmio);

	CDIB *			GetByIndex (int iInd);
	int					Width () const { return m_rDest.right; }
	int					Height () const { return m_rDest.bottom; }

	void				Tile (CDIB * pDibDest, CRect const & rectDest) const;

	CDIB *	m_pcDib;				// the bitmap of all buttons
//	CCompressedDIB *	m_pcDib;				// the bitmap of all buttons
	CDIB *	m_pDib;					// scratch bitmap for building button on
	CRect							m_rDest;				// for the BLTs
};

class CTextBtnData
{
public:
	CTextBtnData ();
	~CTextBtnData ();
	void				Init (CMmio * pMmio);

	CRect				m_rDest;				// for the BLTs
	CDIB *	m_pcDib;				// the bitmap of all buttons
//	CCompressedDIB *	m_pcDib;				// the bitmap of the three button states
	CFont							m_fntText;			// the font for the text
	COLORREF					m_clr [2];			// fore/back colors of font
};


/////////////////////////////////////////////////////////////////////////////
// CMyButton window - base for button classes below

class CMyButton : public CButton
{
// Construction
public:
	CMyButton ();
	virtual ~CMyButton () {}

	BOOL		Create (char const * psText, int idHelp, CRect & rect, CDIB * pBackDib, CWnd *pPar, int ID);

// Attributes
public:

// Operations
public:
	char const * GetHelp () const { return (m_sHelp); }
	void		SetToggleMode (BOOL bOn) { if (bOn) m_cState |= 0x80; else m_cState &= ~ 0x80; }
	void		SetToggleState (BOOL bDown);
	int			GetToggleState () const { return (m_cState & 0x01); }

// Implementation
public:

protected:
	// Generated message map functions
	//{{AFX_MSG(CMyButton)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CDIB *				m_pBackDib;			// bitmap that is the background (for transparent parts)

	CString				m_sHelp;				// help text for button
	BYTE					m_cState;				// if button is down or up & if it's toggleable
							enum { down = 0x01, toggle = 0x80 };
};


/////////////////////////////////////////////////////////////////////////////
// CBmButton window

class CBmButton : public CMyButton
{
// Construction
public:
	CBmButton() {}

// Attributes
public:

// Operations
public:
	BOOL 		Create (int iBtnNum, int idHelp, CBmBtnData * pBbd, CRect & rect, CDIB * pBackDib, CWnd *pPar, int ID);

// Implementation
public:
	void 	DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	// Generated message map functions
	//{{AFX_MSG(CBmButton)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int						m_iBtnNum;			// button index in CBmBtnData
	int						m_iBmOn;				// for animating buttons
	CBmBtnData *	m_pBmBtnData;		// bitmap of buttons
};


/////////////////////////////////////////////////////////////////////////////
// COrdButton window

class CTextButton : public CMyButton
{
// Construction
public:
	CTextButton ();
	~CTextButton ();

// Attributes
public:

// Operations
public:
	BOOL 		Create (int idText, int idHelp, CTextBtnData * pTbd, CRect & rect, CDIB * pBackDib, CWnd *pPar, int ID);

// Implementation
public:
	void 	DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	// Generated message map functions
	//{{AFX_MSG(CTextButton)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CTextBtnData *	m_pTextBtnData;	// bitmap of button states
	CDIB *	m_pDib;					// scratch bitmap for building button on
};


#endif
