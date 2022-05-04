//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __ICONS_H__
#define __ICONS_H__

#include <wndbase.h>


class CIcons;
class CWndBar;
class CWndStatBar;


/////////////////////////////////////////////////////////////////////////////
// CBitmapLib - storage for random UI bitmaps

class CBitmapLib
{
public:

	// BUGBUG - we should move these to the first N, have a LAST_BITMAP below and 
	// start with that in bitmaps.h. Or maybe even give windward.dll it's own RIF?
	enum BITMAP_ID
	{
		// Frame

		FRAME_LL_CORNER = 6,
		FRAME_LR_CORNER,
		FRAME_UL_CORNER,
		FRAME_UR_CORNER,
		FRAME_HORZ,
		FRAME_VERT,
		FRAME_BUTTONS,
		FRAME_TITLE_BAR_INACTIVE,
		FRAME_TITLE_BAR_ACTIVE,

		// Scroll bars

		SCROLLB_THUMB,
		SCROLLB_HORZ,
		SCROLLB_VERT,
		SCROLLB_DOWN,
		SCROLLB_LEFT,
		SCROLLB_RIGHT,
		SCROLLB_UP,

		// Dialog Box

		DLG_RADIO_BUTTONS,
		DLG_CHECK_BOXES,
		DLG_BKGND
	};

									CBitmapLib ();
									~CBitmapLib ();
		void					Init (CMmio * pMmio);
		void					Close ();

		CDIB *				GetByIndex (int iInd);

protected:

		friend class CConquerApp;

		int						m_iNumDibs;
		CDIB * *			m_ppDibs;
};


/////////////////////////////////////////////////////////////////////////////
// CStatData - all bitmaps for a status bar

class CStatData
{
friend CIcons;
public:
						CStatData ();
						~CStatData ();

		void		Init (CMmio * pMmio);

		int				m_ID;									// ID (how we store by in .MIF)

		int				m_leftOff;						// offset from left end before displaying icons/text
		int				m_rightOff;						// same for right end

																// icons are in the bitmap in one row
		int				m_cxIcon;							// width of each icon
		int				m_cyIcon;							// height of each icon
		int				m_nNeedIcon;					// number of "needed" icons (for animation)
						enum TYP_ICON { done, have_all, text, two_piece, bar, text_right };	// text: display text
		TYP_ICON	m_iTypIcon;						// done : fill first icon up to % done
																	// have_all : use 1/2 or 1/3(n) icons to show used, extra/need

																// bitmaps are in the bitmap in one row
		int				m_cxLeft;							// width of left end
		int				m_cxBack;							// width of middle/only piece
		int				m_cxRight;						// width of right end
		int				m_cyBack;							// height of all 3 pieces
						enum TYP_BACK { full_back, back_3, tile };	// tile : tile back bitmap to fill (no stretch)
		TYP_BACK	m_iTypBack;						// full_back : stretch back bitmap to fit window
																	// back_3 : place left, tile back, place right to fill

		CFont *						m_pFnt;					// font for text
		COLORREF					m_clr[6];				// 0,1: top/bottom status, 2,3: warn, 4,5: critical

		CDIB *	m_pcDib;							// the bitmap of all the icons
//	CCompressedDIB *	m_pcDib;				// the bitmap of all buttons
};


/////////////////////////////////////////////////////////////////////////////
// CIcons - store all stat bars here

class CIcons
{
public:
									CIcons ();
									~CIcons () { Close (); }
		void					Init (CMmio * pMmio);
		void					Close ();

		CStatData *		GetByIndex (int iInd);

protected:
		int						m_iNumBars;
		CStatData *		m_pStatData;
};


/////////////////////////////////////////////////////////////////////////////
// CStatInst - an instance of a CStatBar

class CStatInst
{
friend CWndStatBar;
// Construction
public:
					CStatInst ();
					~CStatInst ();

	void		Attach (CIcons * pIcons, int iIndex);

// Attributes
public:
	void		SetSize (CRect const & rect);
	void		SetBack (CDIB * pDib, CPoint const & pt) { m_pBackDib = pDib, m_ptBackOff = pt; }

	void		SetPer (int iPer);
	void		SetHaveNeed (int iHave, int iNeed);
				enum IMPORTANCE { status, warn, critical, inactive };
	void		SetText (char const * pText, IMPORTANCE iImp = status);

// Operations
public:
	void		SetPerBase (int iIcon);
	void		IncIcon ();			// inc need icon by 1
	void		DrawIcon (CDC * pDc) const;

// Implementation
public:

	void		DrawBackground () const;
	void		DrawForeground () const;

	void		DrawStatTwo () const;		
	void		DrawStatDone () const;		
	void		DrawStatHave () const;		
	void		DrawStatText () const;
	void		DrawStatBar () const;
	void		DrawSeries (int * piValues, int iNum) const;

	CStatData *				m_pStatData;			// the stat bar with the bitmaps
	CDIB *						m_pDib;					// scratch DIB for BLTing a window
	CRect							m_rDest;				// for the BLTs

protected:
	int								m_iPerDone;			// percentage done/have
	int								m_iNeed;				// percentage need
	int								m_iIconOn;			// for animating icons
	int								m_iBaseOn;			// for using materials for %

	CDIB *						m_pBackDib;			// bitmap that is the background (for transparent parts)
	CPoint						m_ptBackOff;		// where to start tiling from

	CString						m_sText;				// text for status text type windows
	IMPORTANCE				m_iImp;					// how important
};


/////////////////////////////////////////////////////////////////////////////
// CWndStatBar window

class CWndStatBar : public CWndBase
{
#ifdef _CHEAT
friend CWndBar;
#endif
// Construction
public:
					CWndStatBar();

	void		Create (CIcons * pIcons, int iIndex, CRect & rect, CWnd * pPar, CDIB * pBack = NULL);
	void		Attach (CIcons * pIcons, int iIndex) { m_statInst.Attach (pIcons, iIndex); }
	void		CheckSize ();

// Attributes
public:
	void		SetPer (int iPer);
	void		SetHaveNeed (int iHave, int iNeed);
	void		SetText (char const * pText, CStatInst::IMPORTANCE iImp = CStatInst::status);

// Operations
public:
	void		IncIcon () { m_statInst.IncIcon (); InvalidateRect (NULL); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndStatBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndStatBar();

	CStatInst				m_statInst;				// data to draw the icons

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndStatBar)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
