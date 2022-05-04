#ifndef __SUBCLASS_H__
#define __SUBCLASS_H__

// subclass.h : header file
//

//--------------------------- C T e x t C o l o r s -------------------------

class CTextColors
{

public:

	CTextColors( COLORREF colorrefHighlight,
					 COLORREF colorrefText,
					 COLORREF colorrefShadow );

	COLORREF	GetHighlightColor() const { return m_colorrefHighlight; }
	COLORREF	GetTextColor()      const { return m_colorrefText; 	  }
	COLORREF	GetShadowColor()    const { return m_colorrefShadow;    } 

private:

	COLORREF	m_colorrefHighlight;
	COLORREF	m_colorrefText;
	COLORREF	m_colorrefShadow;
};

//---------------------------- C O D B u t t o n ----------------------------
//	
//	Refuses reflection of WM_DRAWITEM so that owner-draw code in the 
// global subclass proc gets a crack at it.

template <class T>
class CWndOD : public T
{
protected:

	BOOL	OnChildNotify(	UINT,	WPARAM, LPARAM, LRESULT * );
};

//----------------------- C B u t t o n T r a c k e r -----------------------

class CButtonTracker
{

public:

	CButtonTracker();

	void	SetHitTest( int nHitTest )		{ m_nHitTest = nHitTest; }
	UINT	GetHitTest() const				{ return m_nHitTest; }

	void	SetTracking( BOOL bTracking );
	BOOL	IsTracking() const				{ return m_bTracking; }

	void	SetCursorIn( BOOL bCursorIn )	{ m_bCursorIn = bCursorIn; }
	BOOL	IsCursorIn() const				{ return m_bCursorIn; }

protected:

private:

	UINT	m_nHitTest;
	BOOL	m_bCursorIn;
	BOOL	m_bTracking;
};

//------------------------ C F r a m e P a i n t e r -----------------------

class CFramePainter
{

public:

	enum CAPTION_BTNS
	{
		SYSTEM_BTN,
		CLOSE_BTN,
		MINIMIZE_BTN,
		MAXIMIZE_BTN,
		NUM_CAPTION_BTNS
	};

	enum OD_FRAME	// Index into list of DIB pointers for owner-draw frame components
	{
		// Frame corners

		FRAME_LL_CORNER,
		FRAME_LR_CORNER,
		FRAME_UL_CORNER,
		FRAME_UR_CORNER,

		// Frame

		FRAME_HORZ,
		FRAME_VERT,

		// System buttons

		FRAME_BUTTONS,		// 5x2 matrix where cols are (system, minimize, maximize, restore, close)
								// rows are (button-up, button-down)

		// Title bar

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

		NUM_OD_BITMAPS
	};

	// Specify owner-draw bitmaps

	static void	SetDrawInfo( CDIB * adib[ NUM_OD_BITMAPS ] );
	static BOOL	IsInitialized();

	static char const * GetPropName() 	  { return "WWSFramePainter"; }
	static char const * GetTextPropName() { return "WWSText"; }
	static char const * GetFontPropName() { return "WWSFont"; }

	CFramePainter();

	BOOL	WindowProc(	HWND, UINT Message, WPARAM wParam, LPARAM	lParam, LRESULT *presult );

protected:

	void 		Track( UINT nHitTest, HWND );	// Track this caption button

private:

	static CDIB	** s_ppdib;	// Array of pointers to bitmaps (see OD_FRAME for order)

	CButtonTracker	  m_abuttontracker[ NUM_CAPTION_BTNS ];
	CButtonTracker * m_pbuttontracker;
	BOOL		  		  m_bActive;
};

//---------------------- C G l o b a l S u b C l a s s ---------------------

class CGlobalSubClass
{

public:

	static	void	Init();

	static	void	Subclass( CDIB  	* pdibBkgnd,
									 CDIB  	* pdibPushButtonSmall,
									 CDIB  	* pdibPushButtonLarge,
									 CDIB  	* pdibRadioButton,
									 CDIB  	* pdibCheckBox,
									 CFont 	* pfontButton,
									 int		  iSoundID,

									 CTextColors const & textcolorsButton,
									 CTextColors const & textcolorsStatic );

	static	void	UnSubClass();

	static	BOOL	DrawButton     ( DRAWITEMSTRUCT * );
	static	BOOL	DrawCheckBox   ( HWND );
	static	BOOL	DrawRadioButton( HWND );
	static	BOOL	DrawCheckButton( HWND, CDIB * );
	static	BOOL	DrawGroupBox   ( HWND );

	static	BOOL	IsSubclassing	() { return g_bSubclassing; }

	// 
	// These functions work whether or not subclassing is enabled
	//

	static	BOOL	DrawButton     ( DRAWITEMSTRUCT 		*,
											  CDIB			  		* pdibBtnSmall,
											  CDIB			  		* pdibBtnLarge,
											  CDIB			  		* pdibBackground,
											  CFont			  		* pfont,
											  CTextColors const 	& textcolors );

	static	Ptr< CDIB >	MakeBackgroundDIB   ( CDIB const * pdibBackground, HWND hwndCtl  );
	static	Ptr< CDIB >	MakeBackgroundDIB   ( CDIB const * pdibBackground, HWND hwndCtl, CRect rectCtl );
	static	Ptr< CDIB >	MakeBackgroundDIBWnd( CDIB const * pdibBackground, HWND hwnd,    CRect rectCtl );
	static	CRect			GetRect				  ( HWND	hwndCtl );
	static	CRect			GetBackgroundSrcRect( CRect const & rectWnd,
															 CRect const & rectClient,
															 CRect const & rectDIB );

protected:

	static	void	SubClassDialog  ( CDIB * pdibBkgnd );

	static	void	SubClassButton( CDIB    			 * pdibPushButtonSmall,
											 CDIB    			 * pdibPushButtonLarge, 	// Can be NULL
											 CDIB    			 * pdibRadioButton,
											 CDIB    			 * pdibCheckBox,
											 CFont 				 * pfontButton,
											 CTextColors const & textcolors,
											 int       				iSound ); 		// -1 for no sound

	static	void	SubClassStatic( CTextColors const & textcolors );

private:

	static BOOL g_bSubclassing;
};

#endif

