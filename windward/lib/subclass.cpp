// subclass.cpp : implementation file
// GG

#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "_res.h"
#include "subclass.h"
#include "dib.h"

BOOL	CGlobalSubClass::g_bSubclassing;
BOOL  bDoSubclass = TRUE;

//---------------------------------------------------------------------------
// CTextColors::CTextColors
//---------------------------------------------------------------------------
CTextColors::CTextColors(
	COLORREF colorrefHighlight,
	COLORREF colorrefText,
	COLORREF colorrefShadow )
	:
		m_colorrefHighlight( colorrefHighlight | 0x02000000 ),
		m_colorrefText     ( colorrefText      | 0x02000000 ),
		m_colorrefShadow   ( colorrefShadow    | 0x02000000 )
{
}

//---------------------------------------------------------------------------
// ScaleRect - Shrink/expand rect
//---------------------------------------------------------------------------
static void
ScaleRect(
	CRect * prect,
	int	  iScaleNumW,
	int	  iScaleDenW,
	int	  iScaleNumH,
	int	  iScaleDenH )
{
	if ( iScaleNumW != iScaleDenW )
	{
		ASSERT( iScaleDenW );

		int	iDelX = ( prect->Width() - MulDiv( prect->Width(), iScaleNumW, iScaleDenW )) >> 1;

		prect->left   += iDelX;
		prect->right  -= iDelX;
	}

	if ( iScaleNumH != iScaleDenH )
	{
		ASSERT( iScaleDenH );

		int	iDelY = ( prect->Height() - MulDiv( prect->Height(), iScaleNumH, iScaleDenH )) >> 1;

		prect->top    += iDelY;
		prect->bottom -= iDelY;
	}
}

//---------------------------------------------------------------------------
// FitFont - Change font size (if necessary) until 1-line string fits in rect
//---------------------------------------------------------------------------
static int						// 0 - failed, 1 - font fits, 2 - min size reached
FitFont(
	CFont	  		  * pfontSrc,
	CFont	  		  * pfontDst,
	HDC	  		    hdc,
	CString const & str,
	CRect	    		 rect,
	unsigned  		 uFormat,
	int				 iStartFontHeight,	// -1 to calculate height
	int				 iMinFontHeight )
{

	try
	{
	uFormat |= DT_CALCRECT;

	LOGFONT logfontSrc;
	LOGFONT logfont;
	CRect	  rectRequired( 0, 0, 0, 0 );

	if ( ! pfontSrc->GetLogFont( & logfontSrc ))
		return 0;

	int	  iFontHeight = iStartFontHeight;
	CDC	* pdc			  = CDC::FromHandle( hdc );

	for ( ; iFontHeight >= iMinFontHeight; iFontHeight-- )
	{
		if ( pfontDst->m_hObject != NULL )
			pfontDst->DeleteObject();

		memset( &logfont, 0, sizeof( logfont ));

		strcpy( logfont.lfFaceName, logfontSrc.lfFaceName );

		logfont.lfPitchAndFamily = logfontSrc.lfPitchAndFamily;
		logfont.lfHeight 			 = -iFontHeight;

		pfontDst->CreateFontIndirect( & logfont );

		// will this font fit?

		CFont * pfontOld = pdc->SelectObject( pfontDst );

		pdc->DrawText( str, &rectRequired, uFormat );

		pdc->SelectObject( pfontOld );

		if ( rectRequired.Width()  <= rect.Width() &&
			  rectRequired.Height() <= rect.Height() )
			return 1;
	}

	return 2;
	}

	catch (...)
		{
		pfontDst->m_hObject = NULL;
		return 0;
		}
}

//----------------------- C B u t t o n T r a c k e r -----------------------
//
// Keeps state of button being tracked

//---------------------------------------------------------------------------
// CButtonTracker::CButtonTracker
//---------------------------------------------------------------------------
CButtonTracker::CButtonTracker()
:
	m_nHitTest ( 0 )
{
	SetTracking( FALSE );
}

//---------------------------------------------------------------------------
// CButtonTracker::SetTracking
//---------------------------------------------------------------------------
void
CButtonTracker::SetTracking(
	BOOL bTracking )
{
	m_bTracking = m_bCursorIn = bTracking;
}

//------------------------ C F r a m e P a i n t e r ------------------------
//
// Extends CWnd for owner-draw frame painting.
// 
// To use:
//
// Call the static function SetDrawInfo() once to initialize.
// Call member funciton WindowProc() from your CWnd-derived object's WindowProc()

CDIB ** CFramePainter::s_ppdib = NULL;

//----------------------------------------------------------------------
// CFramePainter::SetDrawInfo
//----------------------------------------------------------------------
void
CFramePainter::SetDrawInfo(
	CDIB  * adib[ NUM_OD_BITMAPS ] )	// Array of dibs in order of OD_FRAME enum
{
	ASSERT_STRICT( AfxIsValidAddress( adib, NUM_OD_BITMAPS * sizeof( CDIB * )));

	#ifdef _DEBUG
	for ( int i = 0; i < NUM_OD_BITMAPS; ++i )
		ASSERT_STRICT_VALID( adib[ i ] );
	#endif

	s_ppdib = adib;
}

//----------------------------------------------------------------------
// CFramePainter::IsInitialized
//----------------------------------------------------------------------
BOOL
CFramePainter::IsInitialized()
{
	return NULL != s_ppdib;
}

//---------------------------------------------------------------------------
// CFramePainter::CFramePainter
//---------------------------------------------------------------------------
CFramePainter::CFramePainter()
	:
		m_bActive		 ( FALSE ),
		m_pbuttontracker( NULL  )
{
	// Initialize buttons to untracked

	m_abuttontracker[ SYSTEM_BTN   ].SetHitTest( HTSYSMENU );
	m_abuttontracker[ CLOSE_BTN    ].SetHitTest( HTCLOSE   );
	m_abuttontracker[ MINIMIZE_BTN ].SetHitTest( HTREDUCE  );
	m_abuttontracker[ MAXIMIZE_BTN ].SetHitTest( HTZOOM    );
}

//---------------------------------------------------------------------------
// CFramePainter::WindowProc
//
//	Call from your CWnd-derived object's WindowProc to have owner-draw frame painted
//
// GGTODO: handle mini-captions?
//			  handle WM-SETTEXT, WM_GETTEXT
//---------------------------------------------------------------------------
BOOL							// TRUE if message handled
CFramePainter::WindowProc(
	HWND		 hWnd,
	UINT 		 Message,
	WPARAM 	 wParam,
	LPARAM	 lParam,
	LRESULT * presult )
{

	*presult = 0;

	if ( ! IsInitialized() )
		return FALSE;

	if ( !CGlobalSubClass::IsSubclassing() )
	{
		CString	* pstr  = ( CString * )::RemoveProp( hWnd, GetTextPropName() );
		HFONT		  hfont = ( HFONT     )::RemoveProp( hWnd, GetFontPropName() );

		delete pstr;

		return FALSE;
	}

	if ( ! bDoSubclass )
		return 0;
		
	switch ( Message )
	{
		case WM_QUERYNEWPALETTE:
		case WM_PALETTECHANGED :

			::PostMessage( hWnd, WM_NCPAINT, 0, 0 );

			return FALSE;

		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:	// Check and track caption-bar buttons

			if ( m_pbuttontracker )
			{
				if ( ::GetCapture() != hWnd )
				{
					m_pbuttontracker->SetTracking( FALSE );
					m_pbuttontracker = NULL;

					::ReleaseCapture();
					::SendMessage( hWnd, WM_NCPAINT, 0, 0 );
				}
				else
				{
					CPoint	point( lParam );

					::ClientToScreen( hWnd, &point );

					UINT	nHitTest = ::SendMessage( hWnd, WM_NCHITTEST, 0, MAKELPARAM( point.x, point.y ));

					if (( nHitTest == m_pbuttontracker->GetHitTest() ) != m_pbuttontracker->IsCursorIn() )
					{
						m_pbuttontracker->SetCursorIn( !m_pbuttontracker->IsCursorIn() );
						::SendMessage( hWnd, WM_NCPAINT, 0, 0 );
					}
				}

				return TRUE;
			}

			break;

		case WM_LBUTTONUP:

			if ( m_pbuttontracker )
			{
				CPoint	point( lParam );

				m_pbuttontracker->SetTracking( FALSE );
				m_pbuttontracker = NULL;

				::ReleaseCapture();
				::SendMessage( hWnd, WM_NCPAINT, 0, 0 );
				::ClientToScreen( hWnd, &point );

				WINDOWPLACEMENT	windowplacement;

				windowplacement.length = sizeof( windowplacement );

				::GetWindowPlacement( hWnd, &windowplacement );

				switch ( ::SendMessage( hWnd, WM_NCHITTEST, 0, MAKELPARAM( point.x, point.y )))
				{
					case HTCLOSE: 		::SendMessage( hWnd, WM_CLOSE, 0, 0 );
											break;

					case HTSYSMENU:	// no worky SendMessage( WM_SYSCOMMAND, SC_MOUSEMENU,  MAKELPARAM( point.x, point.y ));
											break;

					case HTREDUCE:		if ( SW_SHOWMINIMIZED == windowplacement.showCmd )
												::SendMessage( hWnd, WM_SYSCOMMAND, SC_RESTORE,  0 );
											else
												::SendMessage( hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0 );

											break;

					case HTZOOM:		if ( SW_SHOWMAXIMIZED == windowplacement.showCmd )
												::SendMessage( hWnd, WM_SYSCOMMAND, SC_RESTORE,    0 );
											else
												::SendMessage( hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0 );

											break;
				}

				return TRUE;
			}

			break;

		case WM_NCACTIVATE:
		{
			BOOL bActive = ( BOOL )wParam; 

			if ( m_bActive != bActive )
			{
				m_bActive = bActive;

				::SendMessage( hWnd, WM_NCPAINT, 0, 0 );
			}

			*presult = TRUE;

			return TRUE;
		}

		case WM_GETTEXTLENGTH:
		{
			CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

			if ( pstr )
				*presult = pstr->GetLength();
			else
				*presult = ::DefWindowProc( hWnd, Message, wParam, lParam );

			return TRUE;
		}

		case WM_GETTEXT:
		{
			CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

			if ( ! pstr )
			{
				LONG	lResult = ::DefWindowProc( hWnd, Message, wParam, lParam );

				pstr = new CString(( char const * )lParam );

				::SetProp( hWnd, GetTextPropName(), pstr );

				*presult = lResult;

				return TRUE;
			}

			int		nMaxChars = wParam;
			char *	psz       = ( char * )lParam;

			strncpy( psz, ( LPCSTR )*pstr, nMaxChars );

			*presult = Min( nMaxChars, pstr->GetLength() + 1 );

			return TRUE;
		}

		case WM_SETTEXT:
		{
			CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

			if ( !pstr )
			{
				pstr = new CString;

			 	::SetProp( hWnd, GetTextPropName(), pstr );
			}

			*pstr = ( char * )lParam;

			::SendMessage( hWnd, WM_NCPAINT, 0, 0 );

			*presult = TRUE;

			return TRUE;
		}

		case WM_SETFONT:
		{
			HFONT	hfont = ( HFONT )wParam;

			::SetProp( hWnd, GetFontPropName(), hfont );

			if ( lParam )
				::SendMessage( hWnd, WM_NCPAINT, 0, 0 );

			*presult = 0;

			return TRUE;
		}

		case WM_GETFONT:
		{
			HFONT	hfont = ( HFONT )::GetProp( hWnd, GetFontPropName() );

			*presult = ( LONG )hfont;

			return TRUE;
		}

		case WM_NCPAINT:
		{
			*presult = 0;

			HDC		hdc  		  = ::GetWindowDC( hWnd );
			CString	strCaption;
			
			CWnd::FromHandle( hWnd )->GetWindowText( strCaption );

			thePal.Paint (hdc);

			CRect	rectWnd;
			CRect	rectClient;

			::GetClientRect ( hWnd, ( RECT * )&rectClient );
			::ClientToScreen( hWnd, ( POINT * )&rectClient );
			::ClientToScreen( hWnd, ( POINT * )&rectClient + 1 );
			::GetWindowRect ( hWnd, ( RECT * )&rectWnd );

			CDIB	* pdib;
			CRect	  rectDst;

			int	iFrameW = rectClient.left - rectWnd.left;
			DWORD	dwStyle   = ::GetWindowLong( hWnd, GWL_STYLE );
			DWORD	dwExStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );

			//-------
			// Frame
			//-------

			if ( dwStyle 	& WS_THICKFRAME ||
				  dwExStyle & WS_EX_DLGMODALFRAME )
			{
				rectDst = CRect( 0, iFrameW, iFrameW, rectWnd.Height() - iFrameW );
				pdib    = s_ppdib[ FRAME_VERT ];
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				rectDst = CRect( rectWnd.Width() - iFrameW, iFrameW, rectWnd.Width(), rectWnd.Height() - iFrameW );
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				rectDst = CRect( iFrameW, 0, rectWnd.Width() - iFrameW, iFrameW );
				pdib    = s_ppdib[ FRAME_HORZ ];
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				rectDst = CRect( iFrameW, rectWnd.Height() - iFrameW, rectWnd.Width() - iFrameW, rectWnd.Height() );
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				//---------
				// Corners
				//---------

				rectDst = CRect( 0, 0, iFrameW, iFrameW );
				pdib    = s_ppdib[ FRAME_UL_CORNER ];
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				rectDst = CRect( rectWnd.Width() - iFrameW, 0, rectWnd.Width(), iFrameW );
				pdib    = s_ppdib[ FRAME_UR_CORNER ];
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				rectDst = CRect( 0, rectWnd.Height() - iFrameW, iFrameW, rectWnd.Height() );
				pdib    = s_ppdib[ FRAME_LL_CORNER ];
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );

				rectDst = CRect( rectWnd.Width() - iFrameW, rectWnd.Height() - iFrameW, rectWnd.Width(), rectWnd.Height() );
				pdib    = s_ppdib[ FRAME_LR_CORNER ];
				pdib->StretchBlt( hdc, rectDst, pdib->GetRect() );
			}

			//---------
			// Caption
			//---------

			// GGFIXIT: Handle all 3 caption types

			if ( WS_CAPTION == ( dwStyle & WS_CAPTION ))
			{
				int	iDib = m_bActive ? FRAME_TITLE_BAR_ACTIVE : FRAME_TITLE_BAR_INACTIVE;

				pdib = s_ppdib[ iDib ];

				CDIB	dib( pdib->GetColorFormat(),
							  pdib->GetType(),
							  pdib->GetDirection(),
							  rectWnd.Width() - 2 * iFrameW,
							  rectClient.top - rectWnd.top - iFrameW );

				CRect	rect = dib.GetRect();

				// if empty exit
				if ((rect.Height () <= 0) || (rect.Width () <= 0))
					{
					thePal.EndPaint (hdc);
					::ReleaseDC( hWnd, hdc );
					return TRUE;
					}

				//------------
				// Background 
				//------------

				pdib->StretchBlt( &dib, dib.GetRect(), pdib->GetRect() );

				int const	iCapH = Max( 0, rectClient.top - rectWnd.top - iFrameW );

				int const iBtnBorder = 0;
				int const iBtnSize   = iCapH - 2 * iBtnBorder;

				//---------
				// Buttons
				//---------

				pdib    = s_ppdib[ FRAME_BUTTONS ];
									 
				CRect		rectBtnSrc = pdib->GetRect();
				CSize		sizeBtnSrc = CSize( rectBtnSrc.Width() / 5, rectBtnSrc.Height() / 2 );
				CPoint	ptSrc;
				CRect		rectSrc;
				CRect		rectOffset = CRect( 0, 0, 0, 0 );
				int		iBtnIndex;

				//---------------
				// System button
				//---------------

				if ( WS_SYSMENU == ( WS_SYSMENU & dwStyle ))
				{
					UINT	nHitTest = ::SendMessage( hWnd, WM_NCHITTEST, 0, MAKELPARAM( rectWnd.left + iFrameW + 8,
																											 rectWnd.top  + iFrameW + 8 ));

					BOOL	bDrawSysButton = nHitTest == HTSYSMENU;

					if ( bDrawSysButton )
					{
						HICON	hicon = CWnd::FromHandle( hWnd )->GetIcon( FALSE );
		
						if ( hicon )
						{
							TRAP();

							::DrawIcon( hdc, iFrameW, iFrameW, hicon );

							rectOffset.left -= 16;	// GGFIXIT: magic #
						}
						else
						{
							iBtnIndex 	= 0;
							ptSrc   		= CPoint( iBtnIndex * sizeBtnSrc.cx, m_abuttontracker[ SYSTEM_BTN ].IsCursorIn() * sizeBtnSrc.cy );
							rectDst 		= CRect( 0, 0, iBtnSize, iBtnSize ) + CPoint( iBtnBorder, iBtnBorder );
							rectSrc 		= CRect( ptSrc, sizeBtnSrc );
		
							pdib->StretchTranBlt( &dib, rectDst, rectSrc );

							rectOffset.left -= iBtnSize;
						}
					}
		
					//--------------
					// Close button
					//--------------
		
					iBtnIndex 	= 4;
					ptSrc   		= CPoint( iBtnIndex * sizeBtnSrc.cx, m_abuttontracker[ CLOSE_BTN ].IsCursorIn() * sizeBtnSrc.cy );
					rectDst 		= CRect( 0, 0, iBtnSize, iBtnSize ) + CPoint( rect.Width() - iBtnBorder - iBtnSize, iBtnBorder );
					rectSrc 		= CRect( ptSrc, sizeBtnSrc );
		
					pdib->StretchTranBlt( &dib, rectDst, rectSrc );
				
					rectOffset.right -= iBtnSize;
				}
		
				//-------------------------
				// Maximize/Restore button
				//-------------------------
		
				WINDOWPLACEMENT	windowplacement;
				BOOL					bMinimizeBox = WS_MINIMIZEBOX == ( WS_MINIMIZEBOX & dwStyle );
				BOOL					bMaximizeBox = WS_MAXIMIZEBOX == ( WS_MAXIMIZEBOX & dwStyle );

				windowplacement.length = sizeof( windowplacement );
		
				::GetWindowPlacement( hWnd, &windowplacement );
		
				if ( bMaximizeBox )
				{
					bMinimizeBox = TRUE;
					iBtnIndex 	 = windowplacement.showCmd == SW_SHOWMAXIMIZED ? 3 : 2;
					ptSrc   		 = CPoint( iBtnIndex * sizeBtnSrc.cx, m_abuttontracker[ MAXIMIZE_BTN ].IsCursorIn() * sizeBtnSrc.cy );
					rectDst 		 = CRect( 0, 0, iBtnSize, iBtnSize ) + CPoint( rect.Width() - iBtnBorder - 2 * iBtnSize, iBtnBorder );
					rectSrc 		 = CRect( ptSrc, sizeBtnSrc );
		
					pdib->StretchTranBlt( &dib, rectDst, rectSrc );
		
					rectOffset.right -= iBtnSize;
				}
		
				//-------------------------
				// Minimize/Restore button
				//-------------------------
		
				if ( bMinimizeBox )
				{
					iBtnIndex = windowplacement.showCmd == SW_SHOWMINIMIZED ? 3 : 1;
		
					ptSrc   = CPoint( iBtnIndex * sizeBtnSrc.cx, m_abuttontracker[ MINIMIZE_BTN ].IsCursorIn() * sizeBtnSrc.cy );
					rectDst = CRect( 0, 0, iBtnSize, iBtnSize ) + CPoint( rect.Width() - iBtnBorder - ( 2 + bMaximizeBox ) * iBtnSize, iBtnBorder );
					rectSrc = CRect( ptSrc, sizeBtnSrc );
		
					pdib->StretchTranBlt( &dib, rectDst, rectSrc );
		
					rectOffset.right -= iBtnSize;
				}
		
				//------------------------------------
				// Blt caption background and buttons
				//------------------------------------
		
				CRect	rectCaption = CRect( iFrameW, iFrameW, rectWnd.Width() - iFrameW, iFrameW + iCapH );
		
				dib.BitBlt( hdc, rectCaption, CPoint( 0, 0 ));
		
				//--------------
				// Caption text
				//--------------
		
				CString 		strCaption;
				
				CWnd::FromHandle( hWnd )->GetWindowText( strCaption );

				TEXTMETRIC 	tm;
		
				CSize sizeText;
				
				if ( !::GetTextExtentPoint32( hdc, strCaption, strCaption.GetLength(), &sizeText ))
					{
					thePal.EndPaint (hdc);
					::ReleaseDC( hWnd, hdc );
					return TRUE;
					}
		
				VERIFY( ::GetTextMetrics( hdc, &tm ));
		
				int yHeight = tm.tmAscent + tm.tmDescent + tm.tmInternalLeading;
		
				rectCaption.InflateRect( 0, 1 );	// FIXIT???
		
				int yHeightDiff = ( rectCaption.Height() - yHeight + 1 ) / 2;
		
				CRect	rectText = rectCaption;
		
				rectText.InflateRect( &rectOffset );

				rectText.left += 8;
		
				::SetTextColor( hdc, ::GetSysColor( COLOR_CAPTIONTEXT ));
				::SetBkMode   ( hdc, TRANSPARENT );
				::ExtTextOut  ( hdc,
									 rectText.left,
									 rectText.top + yHeightDiff,
									 ETO_CLIPPED,
									 rectText,
									 strCaption,
									 strCaption.GetLength(),
									 NULL );
			}

			thePal.EndPaint (hdc);
			::ReleaseDC( hWnd, hdc );

			return TRUE;
		}

		case WM_NCLBUTTONDOWN:
		{
			UINT		nHitTest = wParam; 
			CPoint	point( lParam ); 

			switch ( nHitTest )
			{
				case HTZOOM:		
				case HTREDUCE:	
				case HTCLOSE:

					Track( nHitTest, hWnd );

					return TRUE;

				case HTSYSMENU:	return FALSE;	// Let Windows handle system menu
															// FIXIT: Need to handle ourselves eventually
															//			 to prevent Windows drawing
															//			 other caption buttons
			}

			return FALSE;
		}

		case WM_NCDESTROY:
		{
			CString	* pstr  = ( CString * )::RemoveProp( hWnd, GetTextPropName() );
			HFONT		  hfont = ( HFONT     )::RemoveProp( hWnd, GetFontPropName() );

			delete pstr;

			return FALSE;
		}
	}

	return FALSE;
}

//---------------------------------------------------------------------------
// CFramePainter::Track
//---------------------------------------------------------------------------
void 
CFramePainter::Track(
	UINT	nHitTest,
	HWND	hWnd )
{
	m_pbuttontracker = NULL;

	switch ( nHitTest )
	{
		case HTZOOM:		m_pbuttontracker = m_abuttontracker + MAXIMIZE_BTN;
								break;

		case HTREDUCE:		m_pbuttontracker = m_abuttontracker + MINIMIZE_BTN;
								break;

		case HTSYSMENU:	m_pbuttontracker = m_abuttontracker + SYSTEM_BTN;
								break;

		case HTCLOSE:		m_pbuttontracker = m_abuttontracker + CLOSE_BTN;
								break;
	}

	if ( m_pbuttontracker )
	{
		m_pbuttontracker->SetTracking( TRUE );

		::SetCapture( hWnd );
		::SendMessage( hWnd, WM_NCPAINT, 0, 0 );
	}
}

//------------------ C G l o b a l S u b C l a s s B a s e ------------------

class CGlobalSubClassBase
#ifdef _DEBUG
: public CObject
#endif
{

public:

				//-------------------------------------------------------------
				// Restore the default message handler, return the current one
				//-------------------------------------------------------------
	void		RestoreProc();

				//--------------------------------------------------------------
				// Default message handling - chain to the previous window proc
				//--------------------------------------------------------------
	LRESULT	Default( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );

protected:

	CGlobalSubClassBase( HWND, WNDPROC );
	~CGlobalSubClassBase ( ) { RestoreProc (); }

	#ifdef _DEBUG
	void AssertValid() const;
	#endif

private:

	HWND	 	m_hwnd;
	WNDPROC	m_wndprocPrev;	
};

//---------------------------------------------------------------------------
// CGlobalSubClassBase::CGlobalSubClassBase
//---------------------------------------------------------------------------
CGlobalSubClassBase::CGlobalSubClassBase(
	HWND		hwnd,
	WNDPROC	wndproc )
	:
		m_hwnd	    ( hwnd ),
		m_wndprocPrev( NULL )
{
	ASSERT( wndproc );

	if ( !wndproc )
		return;

	m_wndprocPrev = ( WNDPROC )::SetClassLong( m_hwnd, GCL_WNDPROC, ( DWORD )wndproc );
}

//---------------------------------------------------------------------------
// CGlobalSubClassBase::Default
//---------------------------------------------------------------------------
LRESULT
CGlobalSubClassBase::Default(
	HWND 		hwnd,
	UINT 		Message,
	WPARAM 	wParam,
	LPARAM 	lParam )
{
//BUGBUG - can't cause it will call us	ASSERT_VALID( this );

	if ( m_wndprocPrev )
		return ::CallWindowProc( m_wndprocPrev, hwnd, Message, wParam, lParam );
	else
		return ::DefWindowProc( hwnd, Message, wParam, lParam );
}

//---------------------------------------------------------------------------
// CGlobalSubClassBase::RestoreProc
//---------------------------------------------------------------------------
void
CGlobalSubClassBase::RestoreProc()
{
	ASSERT_VALID( this );

	if ( ::IsWindow( m_hwnd ) && m_wndprocPrev )
	{
		::SetClassLong( m_hwnd, GWL_WNDPROC, ( DWORD )m_wndprocPrev );

//		m_wndprocPrev = NULL;
	}
}

//---------------------------------------------------------------------------
// CGlobalSubClassBase::AssertValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void
CGlobalSubClassBase::AssertValid() const
{
	CObject::AssertValid();
}
#endif

//--------------------------------------------------------------------------
//
// Concrete subclasses

//--------------- C G l o b a l S u b C l a s s D i a l o g ----------------

class CGlobalSubClassDialog : public CGlobalSubClassBase
{

public:

	static LONG FAR PASCAL WindowProc( HWND   hWnd,
												  UINT   Message,
												  WPARAM wParam,
		      								  LPARAM lParam );

	CGlobalSubClassDialog( CDIB *, HWND );

	Ptr< CDIB >	MakeBackgroundDIB( HWND hwndCtl  ) const;
	Ptr< CDIB >	MakeBackgroundDIB( HWND hwndCtl, CRect rectCtl ) const;

	#ifdef _DEBUG
	void AssertValid() const;
	#endif

	CDIB	 * m_pdibBkgnd;
};

//--------------- C G l o b a l S u b c l a s s B u t t o n ----------------

class CGlobalSubClassButton : public CGlobalSubClassBase
{

public:

	static LONG FAR PASCAL WindowProc( HWND   hWnd,
												  UINT   Message,
												  WPARAM wParam,
		      								  LPARAM lParam );

	CGlobalSubClassButton( CDIB  				  * pdibPushButtonSmall,
								  CDIB  				  * pdibPushButtonLarge,
								  CDIB  				  * pdibRadioButton,
								  CDIB  				  * pdibCheckBox,
								  CFont 				  *,
								  CTextColors const &,
								  int 					 iSound,
								  HWND );

	static char const * GetTextPropName()  { return "WWSButtonText";  }
	static char const * GetFontPropName()  { return "WWSButtonFont";  }

	#ifdef _DEBUG
	void AssertValid() const;
	#endif

	CDIB			* m_pdibPushButtonSmall;
	CDIB			* m_pdibPushButtonLarge;
	CDIB			* m_pdibRadioButton;
	CDIB			* m_pdibCheckBox;
	CFont			* m_pfont;
	CTextColors	  m_textcolors;
	int		  	  m_iSound;
};

//--------------- C G l o b a l S u b c l a s s S t a t i c ----------------

class CGlobalSubClassStatic : public CGlobalSubClassBase
{

public:

	static LONG FAR PASCAL WindowProc( HWND   hWnd,
												  UINT   Message,
												  WPARAM wParam,
		      								  LPARAM lParam );

	CGlobalSubClassStatic( CTextColors const &, HWND );

	static char const * GetTextPropName() { return "WWSStaticText"; }
	static char const * GetFontPropName() { return "WWSStaticFont"; }

	#ifdef _DEBUG
	void AssertValid() const;
	#endif

	CTextColors m_textcolors;
};

//---------------------------------------------------------------------------
// CGlobalSubClassDialog::CGlobalSubClassDialog
//---------------------------------------------------------------------------
CGlobalSubClassDialog::CGlobalSubClassDialog(
	CDIB	* pdibBkgnd,
	HWND	  hWnd )
	:
		CGlobalSubClassBase( hWnd, WindowProc ),
		m_pdibBkgnd			 ( pdibBkgnd )
{
}

//---------------------------------------------------------------------------
// CGlobalSubClassDialog::MakeBackgroundDIB
//---------------------------------------------------------------------------
Ptr< CDIB >
CGlobalSubClassDialog::MakeBackgroundDIB(
	HWND	hwndCtl ) const
{
	return CGlobalSubClass::MakeBackgroundDIB( m_pdibBkgnd, hwndCtl );
}

//---------------------------------------------------------------------------
// CGlobalSubClassDialog::MakeBackgroundDIB
//---------------------------------------------------------------------------
Ptr< CDIB >
CGlobalSubClassDialog::MakeBackgroundDIB(
	HWND	hwndCtl,
	CRect	rectCtl ) const
{
	return CGlobalSubClass::MakeBackgroundDIB( m_pdibBkgnd, hwndCtl, rectCtl );
}

//---------------------------------------------------------------------------
// CGlobalSubClass::GetRect - Returns rect for child control in dialog box client area coords
//---------------------------------------------------------------------------
CRect
CGlobalSubClass::GetRect(
	HWND	hwndCtl )
{
	if ( !::IsWindow( hwndCtl ))
		return CRect( 0, 0, 0, 0 );

	HWND	 hwndDlg = ::GetParent( hwndCtl );

	if ( !::IsWindow( hwndDlg ))
		return NULL;

	// Get Ctl rect relative to diaog box

	CRect	rectCtl;
	CRect	rectDlg;

	::GetClientRect( hwndCtl, ( RECT * )&rectCtl );
	::GetClientRect( hwndDlg, ( RECT * )&rectDlg );

	CPoint	ptCtl = CPoint( 0, 0 );
	CPoint	ptDlg = CPoint( 0, 0 );

	::ClientToScreen( hwndCtl, ( POINT * )&ptCtl );
	::ClientToScreen( hwndDlg, ( POINT * )&ptDlg );

	ptCtl   -= ptDlg;
	rectCtl += ptCtl;

	return rectCtl;
}

//---------------------------------------------------------------------------
// CGlobalSubClass::GetBackgroundSrcRect 
//
// Given a window rect and a client rect, return the dib src rect to use
//---------------------------------------------------------------------------
CRect
CGlobalSubClass::GetBackgroundSrcRect(
	CRect const & rectWnd,
	CRect const & rectClient,
	CRect const & rectDIB )
{
	int	iW = Min( rectDIB.Width(),  rectWnd.Width()  );
	int	iH = Min( rectDIB.Height(), rectWnd.Height() );

	CRect	rect = rectClient;

	rect.left   = MulDiv( rect.left,   iW, rectWnd.Width()  );
	rect.right  = MulDiv( rect.right,  iW, rectWnd.Width()  );
	rect.top    = MulDiv( rect.top,    iH, rectWnd.Height() );
	rect.bottom = MulDiv( rect.bottom, iH, rectWnd.Height() );

	return rect;
}

//---------------------------------------------------------------------------
// CGlobalSubClass::MakeBackgroundDIB
//---------------------------------------------------------------------------
Ptr< CDIB >
CGlobalSubClass::MakeBackgroundDIB(
	CDIB const * pdibBackground,
	HWND	hwndCtl )
{
	CRect	rectCtl = GetRect( hwndCtl );

	if ( rectCtl.IsRectEmpty() )
		return NULL;

	return MakeBackgroundDIB( pdibBackground, hwndCtl, rectCtl );
}

//---------------------------------------------------------------------------
// CGlobalSubClass::MakeBackgroundDIBWnd
//---------------------------------------------------------------------------
Ptr< CDIB >
CGlobalSubClass::MakeBackgroundDIBWnd(
	CDIB const * pdibBackground,
	HWND			 hwnd,
	CRect			 rectCtl )
{
	if ( !::IsWindow( hwnd ))
		return NULL;

	CRect	rectWnd;

	::GetClientRect( hwnd, ( RECT * )&rectWnd );

	Ptr< CDIB >	ptrdib = new CDIB( pdibBackground->GetColorFormat(),
				  							 CBLTFormat::DIB_MEMORY,
				  							 pdibBackground->GetDirection(),
				  							 rectCtl.Width(),
				  							 rectCtl.Height() );

	// Transform Ctl rect to background dib coords

	rectCtl = GetBackgroundSrcRect( rectWnd, rectCtl, pdibBackground->GetRect() );

	(( CDIB * )pdibBackground )->StretchBlt( ptrdib.Value(), ptrdib->GetRect(), rectCtl );

	return ptrdib;
}

//---------------------------------------------------------------------------
// CGlobalSubClass::MakeBackgroundDIB
//---------------------------------------------------------------------------
Ptr< CDIB >
CGlobalSubClass::MakeBackgroundDIB(
	CDIB const * pdibBackground,
	HWND			 hwndCtl,
	CRect			 rectCtl )
{
	HWND	 hwndParent = ::GetParent( hwndCtl );

	return MakeBackgroundDIBWnd( pdibBackground, hwndParent, rectCtl );
}

//---------------------------------------------------------------------------
// CGlobalSubClassDialog::AssertValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void
CGlobalSubClassDialog::AssertValid() const
{
	CGlobalSubClassBase::AssertValid();

	ASSERT_VALID( m_pdibBkgnd );
}
#endif

//---------------------------------------------------------------------------
// CGlobalSubClassButton::CGlobalSubClassButton
//---------------------------------------------------------------------------
CGlobalSubClassButton::CGlobalSubClassButton(
	CDIB					* pdibPushButtonSmall,
	CDIB					* pdibPushButtonLarge,
	CDIB					* pdibRadioButton,
	CDIB					* pdibCheckBox,
	CFont					* pfont,
	CTextColors const & textcolors,
	int		  			  iSound,
	HWND	  	  			  hWnd )
	:
		CGlobalSubClassBase  ( hWnd, WindowProc ),
		m_pdibPushButtonSmall( pdibPushButtonSmall ),
		m_pdibPushButtonLarge( pdibPushButtonLarge ),
		m_pdibRadioButton    ( pdibRadioButton ),
		m_pdibCheckBox       ( pdibCheckBox ),
		m_pfont				   ( pfont ),
		m_iSound				   ( iSound ),
		m_textcolors			( textcolors )
{
}

//---------------------------------------------------------------------------
// CGlobalSubClassButton::AssertValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void
CGlobalSubClassButton::AssertValid() const
{
	CGlobalSubClassBase::AssertValid();

	ASSERT_VALID( m_pdibPushButtonSmall  );
	ASSERT_VALID_OR_NULL( m_pdibPushButtonLarge );
	ASSERT_VALID( m_pdibRadioButton );
	ASSERT_VALID( m_pdibCheckBox );
	ASSERT_VALID( m_pfont );
}
#endif

//---------------------------------------------------------------------------
// CGlobalSubClassStatic::CGlobalSubClassStatic
//---------------------------------------------------------------------------
CGlobalSubClassStatic::CGlobalSubClassStatic(
	CTextColors const & textcolors,
	HWND	  	  			  hWnd )
	:
		CGlobalSubClassBase( hWnd, WindowProc ),
		m_textcolors( textcolors )
{
}

//---------------------------------------------------------------------------
// CGlobalSubClassStatic::AssertValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void
CGlobalSubClassStatic::AssertValid() const
{
	CGlobalSubClassBase::AssertValid();
}
#endif

Ptr< CGlobalSubClassDialog >	g_ptrsubclassdialog;
Ptr< CGlobalSubClassButton >	g_ptrsubclassbutton;
Ptr< CGlobalSubClassStatic >	g_ptrsubclassstatic;

//---------------------- C G l o b a l S u b C l a s s X -------------------
//
// Owns the representative dialog box and controls.
// Owns the subclass objects.

class CGlobalSubClassX
{

public:

	CGlobalSubClassX();

	void	SubClassDialog( CDIB 				 * pdibBkgnd );
	void	SubClassButton( CDIB 				 * pdibPushButtonSmall,
								 CDIB 				 * pdibPushButtonLarge,
								 CDIB 				 * pdibRadioButton,
								 CDIB 				 * pdibCheckBox,
								 CFont 				 *,
								 CTextColors const &,
								 int 						iSound );

	void	SubClassStatic( CTextColors const & );

	CDialog	m_dlg;
};

static Ptr< CGlobalSubClassX >	g_ptrthesubclassx;

//---------------------------------------------------------------------------
// EraseBackground
//---------------------------------------------------------------------------
static void
EraseBackground(
	HWND	hWnd,
	HWND	hWndDlg,
	HDC	hDC )
{
	CRect	rectDlg;
	CRect	rectWnd;

	ASSERT( ::IsWindow( hWnd    ));
	ASSERT( ::IsWindow( hWndDlg ));

	CWnd	* pwndDlg = CWnd::FromHandle( hWndDlg );
	CWnd	* pwnd    = CWnd::FromHandle( hWnd    );

	pwndDlg->GetClientRect( &rectDlg );
	pwnd   ->GetClientRect( &rectWnd );

	pwndDlg->ClientToScreen( &rectDlg );
	pwnd   ->ClientToScreen( &rectWnd );

	rectWnd -= rectDlg.TopLeft();

	CGlobalSubClassDialog	*psubclassdialog = g_ptrsubclassdialog.Value();

	if ( !psubclassdialog )
		return;

	ASSERT_VALID( psubclassdialog );

	CDIB	*pdib    = psubclassdialog->m_pdibBkgnd;
	CRect	 rectDst;

	pwnd->GetClientRect( &rectDst );

	CRect	 rectSrc = CGlobalSubClass::GetBackgroundSrcRect( rectDlg, rectWnd, pdib->GetRect() );

	thePal.Paint( hDC );

	pdib->StretchBlt( hDC, rectDst, rectSrc );

	thePal.EndPaint( hDC );
}

//---------------------------------------------------------------------------
// CGlobalSubClassDialog::WindowProc
//---------------------------------------------------------------------------
LONG FAR PASCAL
CGlobalSubClassDialog::WindowProc(
	HWND   hWnd,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam )
{

	CGlobalSubClassDialog	*psubclassdialog = g_ptrsubclassdialog.Value();

	if ( !psubclassdialog )
		return 0;

	if ( !CGlobalSubClass::IsSubclassing() )
	{
		CFramePainter	*pframepainter = ( CFramePainter * )::RemoveProp( hWnd, CFramePainter::GetPropName() );

		delete pframepainter;

		return psubclassdialog->Default( hWnd, Message, wParam, lParam );
	}

	if ( ! bDoSubclass )
		return psubclassdialog->Default( hWnd, Message, wParam, lParam );
		
	// Non-client frame painting

	CFramePainter * pframepainter = ( CFramePainter * )::GetProp( hWnd, CFramePainter::GetPropName() );

	if ( pframepainter )
	{
		LRESULT	result;

		if ( pframepainter->WindowProc( hWnd, Message, wParam, lParam, &result ))
			return result;
	}

	switch ( Message )
	{
		case WM_CREATE:

			if ( CFramePainter::IsInitialized() )
				::SetProp( hWnd, CFramePainter::GetPropName(), new CFramePainter );

			break;

		case WM_ERASEBKGND:
		{
			EraseBackground( hWnd, hWnd, ( HDC )wParam );

			return TRUE;
		}

		case WM_QUERYNEWPALETTE:
		{
			if ( iWinType != W32s )
			{
				CClientDC dc( CWnd::FromHandle( hWnd ));

				thePal.PalMsg( dc.m_hDC, hWnd, WM_QUERYNEWPALETTE, 0, 0 );
			}

			break;
		}

		case WM_CTLCOLORBTN:
		{
			HDC	hdc = ( HDC )wParam;

			::SetBkMode( hdc, TRANSPARENT );

			return ( LONG )( HBRUSH )GetStockObject( HOLLOW_BRUSH );
		}

		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT	pdrawitem = ( LPDRAWITEMSTRUCT )lParam;

			switch ( pdrawitem->CtlType )
			{
				case ODT_BUTTON:
				
					return CGlobalSubClass::DrawButton( pdrawitem );
			}

			break;
		}

		case WM_NCDESTROY:
		{
			CFramePainter	*pframepainter = ( CFramePainter * )::RemoveProp( hWnd, CFramePainter::GetPropName() );

			delete pframepainter;

			if ( g_ptrthesubclassx.Value() && hWnd == g_ptrthesubclassx->m_dlg.m_hWnd )
				psubclassdialog->RestoreProc();

			break;
		}
	}

	return psubclassdialog->Default( hWnd, Message, wParam, lParam );
}

//---------------------------------------------------------------------------
// CGlobalSubClassButton::WindowProc
//---------------------------------------------------------------------------
LONG FAR PASCAL
CGlobalSubClassButton::WindowProc(
	HWND   hWnd,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam )
{

	CGlobalSubClassButton	*psubclassbutton = g_ptrsubclassbutton.Value();

	if ( !psubclassbutton )
		return 0;

	if ( !CGlobalSubClass::IsSubclassing() )
	{
		CString	* pstr   = ( CString * )::RemoveProp( hWnd, GetTextPropName()  );
		HFONT		  hfont  = ( HFONT     )::RemoveProp( hWnd, GetFontPropName()  );

		if ( pstr )
			delete pstr;

		return psubclassbutton->Default( hWnd, Message, wParam, lParam );
	}

	if ( ! bDoSubclass )
		return psubclassbutton->Default( hWnd, Message, wParam, lParam );
		
	switch ( Message )
	{
		case WM_CREATE:
		{
			CREATESTRUCT * pcreatestruct = ( CREATESTRUCT * )lParam;

			LONG	lStyle = pcreatestruct->style;

			CButton	*pbtn    = ( CButton * )CWnd::FromHandle( hWnd );
			DWORD		 dwStyle = lStyle & 0x00ff;

			switch ( dwStyle )
			{
				case BS_PUSHBUTTON:
				case BS_DEFPUSHBUTTON:

					pbtn->SetButtonStyle( BS_OWNERDRAW, FALSE );

					break;
			}
		}

		case WM_PAINT:
		{
			DWORD	dwStyle = ::GetWindowLong( hWnd, GWL_STYLE );

			switch ( dwStyle & 0x00ff )
			{
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:

					if ( CGlobalSubClass::DrawRadioButton( hWnd ))
						return 0;

					break;

				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:

					if ( CGlobalSubClass::DrawCheckBox( hWnd ))
						return 0;

					break;

				case BS_GROUPBOX:

					if ( CGlobalSubClass::DrawGroupBox( hWnd ))
						return 0;

					break;
			}

			break;
		}

		case BM_SETCHECK:
		{
			LRESULT	lResult = psubclassbutton->Default( hWnd, Message, wParam, lParam );

			::InvalidateRect( hWnd, NULL, FALSE );
			::UpdateWindow  ( hWnd );

			return lResult;
		}

		case WM_ERASEBKGND:

			return TRUE;

		case WM_NCDESTROY:
		{
			CString	* pstr   = ( CString * )::RemoveProp( hWnd, GetTextPropName()  );
			HFONT		  hfont  = ( HFONT     )::RemoveProp( hWnd, GetFontPropName()  );

			delete pstr;

			DWORD	dwStyle = ::GetWindowLong( hWnd, GWL_STYLE ) & 0x00ff;

			if ( g_ptrthesubclassx.Value() )
			{
				HWND	hwndBtn = ::GetDlgItem( g_ptrthesubclassx->m_dlg.m_hWnd, IDC_SUBCLASS_BUTTON );

				if ( hWnd == hwndBtn )
					psubclassbutton->RestoreProc();
			}

			break;
		}

		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			DWORD	dwStyle   = ::GetWindowLong( hWnd, GWL_STYLE ) & 0x00ff;

			switch ( dwStyle )
			{
				case BS_PUSHBUTTON:
				case BS_DEFPUSHBUTTON:

					if ( -1 != psubclassbutton->m_iSound )
						theMusicPlayer.PlayForegroundSound( psubclassbutton->m_iSound, FOREGROUND_PRI );

					break;

				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
				{
					LRESULT	lResult = psubclassbutton->Default( hWnd, Message, wParam, lParam );

					::InvalidateRect( hWnd, NULL, FALSE );
					::UpdateWindow  ( hWnd );

					return lResult;
				}
			}

			break;
		}
	}

	DWORD	dwStyle = ::GetWindowLong( hWnd, GWL_STYLE ) & 0x00ff;

	if ( BS_GROUPBOX == dwStyle )
		switch ( Message )
		{
			case WM_GETTEXTLENGTH:
			{
				CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

				if ( pstr )
					return pstr->GetLength();

				break;
			}

			case WM_GETTEXT:
			{
				CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

				if ( ! pstr )
				{
					LONG	lResult = psubclassbutton->Default( hWnd, Message, wParam, lParam );

					pstr = new CString(( char const * )lParam );

					::SetProp( hWnd, GetTextPropName(), pstr );

					return lResult;
				}

				int		nMaxChars = wParam;
				char *	psz       = ( char * )lParam;

				strncpy( psz, ( LPCSTR )*pstr, nMaxChars );

				return Min( nMaxChars, pstr->GetLength() + 1 );
			}

			case WM_SETTEXT:
			{
				CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

				if ( !pstr )
				{
					pstr = new CString;

				 	::SetProp( hWnd, GetTextPropName(), pstr );
				}

				*pstr = ( char * )lParam;

				::InvalidateRect( hWnd, NULL, TRUE );

				return TRUE;
			}

			case WM_SETFONT:
			{
				HFONT	hfont = ( HFONT )wParam;

				::SetProp( hWnd, GetFontPropName(), hfont );

				if ( lParam )
					::InvalidateRect( hWnd, NULL, TRUE );

				return 0;
			}

			case WM_GETFONT:
			{
				HFONT	hfont = ( HFONT )::GetProp( hWnd, GetFontPropName() );

				return ( LONG )hfont;
			}
		}

	return psubclassbutton->Default( hWnd, Message, wParam, lParam );
}

//---------------------------------------------------------------------------
// CGlobalSubClassStatic::WindowProc
//---------------------------------------------------------------------------
LONG FAR PASCAL
CGlobalSubClassStatic::WindowProc(
	HWND   hWnd,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam )
{

	CGlobalSubClassStatic	*psubclassstatic = g_ptrsubclassstatic.Value();

	if ( !psubclassstatic )
		return 0;

	if ( !CGlobalSubClass::IsSubclassing() )
	{
		CString	* pstr  = ( CString * )::RemoveProp( hWnd, GetTextPropName() );
		HFONT		  hfont = ( HFONT     )::RemoveProp( hWnd, GetFontPropName() );

		if ( pstr )
			delete pstr;

		return psubclassstatic->Default( hWnd, Message, wParam, lParam );
	}

	if ( ! bDoSubclass )
		return psubclassstatic->Default( hWnd, Message, wParam, lParam );
		
	if ( !g_ptrsubclassdialog.Value() )
		return 0;

	DWORD		dwStyle  		= ::GetWindowLong( hWnd, GWL_STYLE );
	DWORD		dwStaticStyle	= dwStyle & 0x0f;

	if ( dwStaticStyle != SS_SIMPLE &&
		  dwStaticStyle != SS_LEFT   &&
		  dwStaticStyle != SS_CENTER &&
		  dwStaticStyle != SS_RIGHT  &&
		  dwStaticStyle != SS_LEFTNOWORDWRAP )

		return psubclassstatic->Default( hWnd, Message, wParam, lParam );

	switch ( Message )
	{
		case WM_ERASEBKGND:

			return TRUE;

		case WM_PAINT:
		{
			unsigned	uFormat = DT_EXPANDTABS;

			switch ( dwStaticStyle )
			{
				case SS_SIMPLE:
					
					uFormat |= DT_LEFT | DT_SINGLELINE | DT_VCENTER;
					break;

				case SS_LEFT:
					
					uFormat |= DT_LEFT | DT_WORDBREAK;
					break;

				case SS_CENTER:
					
					uFormat |= DT_CENTER | DT_WORDBREAK;
					break;

				case SS_RIGHT:
					
					uFormat |= DT_RIGHT | DT_WORDBREAK;
					break;

				case SS_LEFTNOWORDWRAP:
					
					uFormat |= DT_LEFT | DT_SINGLELINE | DT_VCENTER;
					break;

				default:

					return psubclassstatic->Default( hWnd, Message, wParam, lParam );
			}

			CRect		rectDst;
    		CFont  * pfontPrev = NULL;

			::GetClientRect( hWnd, &rectDst );

			if ( rectDst.IsRectEmpty() )
				break;

			CRect			rectCtl = rectDst;
			Ptr< CDIB >	ptrdib  = g_ptrsubclassdialog->MakeBackgroundDIB( hWnd );

			if ( ! ptrdib.Value() )
				return 0;

			HDC	hdc = ptrdib->GetDC();

			if ( !hdc )
				break;

			CDC * pdc = CDC::FromHandle( hdc ); 
			thePal.Paint  ( pdc->m_hDC );

			// draw the text

			CString 	sText;

			CWnd::FromHandle( hWnd )->GetWindowText( sText );

			HFONT	hfont = ( HFONT )::SendMessage( hWnd, WM_GETFONT, 0, 0 );

			CFont	* pfont = NULL;

			if ( hfont )
			{
				pfont     = CFont::FromHandle( hfont );
				pfontPrev = pdc->SelectObject( pfont );
			}

		   int 	nBkModePrev     = pdc->SetBkMode( TRANSPARENT );
    		DWORD dwTextColorPrev = pdc->GetTextColor();

			rectDst.OffsetRect( 1, 1 );

			pdc->SetTextColor( psubclassstatic->m_textcolors.GetHighlightColor() );
			pdc->DrawText    ( sText, -1, &rectDst, uFormat );

			rectDst.OffsetRect( -2, -2 );

			pdc->SetTextColor( psubclassstatic->m_textcolors.GetShadowColor() );
			pdc->DrawText    ( sText, -1, &rectDst, uFormat );

			rectDst.OffsetRect( 1, 1 );

			pdc->SetTextColor( psubclassstatic->m_textcolors.GetTextColor() );
			pdc->DrawText    ( sText, -1, &rectDst, uFormat );

	    	pdc->SetTextColor( dwTextColorPrev );

    		if ( pfontPrev )
				pdc->SelectObject( pfontPrev );

		   pdc->SetBkMode( nBkModePrev );

			CPaintDC	dcWnd( CWnd::FromHandle( hWnd ));

			thePal.Paint  ( dcWnd.m_hDC );
			ptrdib->BitBlt( dcWnd.m_hDC, rectCtl, CPoint( 0, 0 ));

			thePal.EndPaint  ( pdc->m_hDC );
			thePal.EndPaint  ( dcWnd.m_hDC );
			ptrdib->ReleaseDC();
			return 0;
		}

		case WM_GETTEXTLENGTH:
		{
			CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

			if ( pstr )
				return pstr->GetLength();

			break;
		}

		case WM_GETTEXT:
		{
			CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

			if ( ! pstr )
			{
				LONG	lResult = psubclassstatic->Default( hWnd, Message, wParam, lParam );

				pstr = new CString(( char const * )lParam );

				::SetProp( hWnd, GetTextPropName(), pstr );

				return lResult;
			}

			int		nMaxChars = wParam;
			char *	psz       = ( char * )lParam;

			strncpy( psz, ( LPCSTR )*pstr, nMaxChars );

			return Min( nMaxChars, pstr->GetLength() + 1 );
		}

		case WM_SETTEXT:
		{
			CString	* pstr = ( CString * )::GetProp( hWnd, GetTextPropName() );

			if ( !pstr )
			{
				pstr = new CString;

			 	::SetProp( hWnd, GetTextPropName(), pstr );
			}

			*pstr = ( char * )lParam;

			::InvalidateRect( hWnd, NULL, TRUE );

			return TRUE;
		}

		case WM_SETFONT:
		{
			HFONT	hfont = ( HFONT )wParam;

			::SetProp( hWnd, GetFontPropName(), hfont );

			if ( lParam )
				::InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}

		case WM_GETFONT:
		{
			HFONT	hfont = ( HFONT )::GetProp( hWnd, GetFontPropName() );

			return ( LONG )hfont;
		}

		case WM_NCDESTROY:
		{
			CString	* pstr  = ( CString * )::RemoveProp( hWnd, GetTextPropName() );
			HFONT		  hfont = ( HFONT     )::RemoveProp( hWnd, GetFontPropName() );

			delete pstr;

			if ( g_ptrthesubclassx.Value() )
			{
				HWND	hwndStatic = ::GetDlgItem( g_ptrthesubclassx->m_dlg.m_hWnd, IDC_SUBCLASS_STATIC );

				if ( hWnd == hwndStatic )
					psubclassstatic->RestoreProc();
			}

			break;
		}
	}

	return psubclassstatic->Default( hWnd, Message, wParam, lParam );
}

//--------------------------------------------------------------------------
// CGlobalSubClassX::CGlobalSubclassX
//--------------------------------------------------------------------------
CGlobalSubClassX::CGlobalSubClassX()
{
	if ( !m_dlg.Create( IDD_SUBCLASS ))
		ThrowError( ERR_SUBCLASS_DLG_CREATE );
}

//--------------------------------------------------------------------------
// CGlobalSubClassX::SubClassDialog
//--------------------------------------------------------------------------
void
CGlobalSubClassX::SubClassDialog(
	CDIB * pdibBkgnd )
{
	ASSERT_STRICT( !g_ptrsubclassdialog.Value() );

	g_ptrsubclassdialog = new CGlobalSubClassDialog( pdibBkgnd, m_dlg.m_hWnd );
}

//--------------------------------------------------------------------------
// CGlobalSubClassX::SubClassButton
//--------------------------------------------------------------------------
void
CGlobalSubClassX::SubClassButton(
	CDIB 					* pdibPushButtonSmall,
	CDIB 					* pdibPushButtonLarge,
	CDIB 					* pdibRadioButton,
	CDIB 					* pdibCheckBox,
	CFont					* pfont,
	CTextColors const & textcolors,
	int		  			  iSound )
{
	ASSERT_STRICT(  g_ptrsubclassdialog.Value() );
	ASSERT_STRICT(  m_dlg.m_hWnd );
	ASSERT_STRICT( !g_ptrsubclassbutton.Value() );

	HWND	hwnd = ::GetDlgItem( m_dlg.m_hWnd, IDC_SUBCLASS_BUTTON );

	if ( hwnd )
		g_ptrsubclassbutton =
			new CGlobalSubClassButton( pdibPushButtonSmall,
				 								pdibPushButtonLarge,
				 								pdibRadioButton,
				 								pdibCheckBox,
												pfont,
												textcolors,
												iSound,
												hwnd );
}

//--------------------------------------------------------------------------
// CGlobalSubClassX::SubClassStatic
//--------------------------------------------------------------------------
void
CGlobalSubClassX::SubClassStatic(
	CTextColors const & textcolors )
{
	ASSERT(  g_ptrsubclassdialog.Value() );
	ASSERT(  m_dlg.m_hWnd );
	ASSERT( !g_ptrsubclassstatic.Value() );

	HWND	hwnd = ::GetDlgItem( m_dlg.m_hWnd, IDC_SUBCLASS_STATIC );

	if ( hwnd )
		g_ptrsubclassstatic = new CGlobalSubClassStatic( textcolors, hwnd );
}

//----------------------- C G l o b a l S u b C l a s s --------------------
//
// Public interface to global subclassing

//--------------------------------------------------------------------------
// CGlobalSubClass::Init
//--------------------------------------------------------------------------
void
CGlobalSubClass::Init()
{
	if ( g_ptrthesubclassx.Value() )
		return;

	g_ptrthesubclassx = new CGlobalSubClassX;
}

//--------------------------------------------------------------------------
// CGlobalSubClass::SubClassDialog
//--------------------------------------------------------------------------
void
CGlobalSubClass::SubClassDialog(
	CDIB * pdibBkgnd )
{
	ASSERT_STRICT( g_ptrthesubclassx.Value() );

	if ( g_ptrthesubclassx.Value() )
		g_ptrthesubclassx->SubClassDialog( pdibBkgnd );
}

//--------------------------------------------------------------------------
// CGlobalSubClass::SubClassButton
//--------------------------------------------------------------------------
void
CGlobalSubClass::SubClassButton(
	CDIB					* pdibPushButtonSmall,
	CDIB					* pdibPushButtonLarge,
	CDIB					* pdibRadioButton,
	CDIB					* pdibCheckBox,
	CFont					* pfont,
	CTextColors const & textcolors,
	int		  			  iSound )
{
	ASSERT_STRICT( g_ptrthesubclassx.Value() );

	if ( g_ptrthesubclassx.Value() )
		g_ptrthesubclassx->SubClassButton( pdibPushButtonSmall,
													  pdibPushButtonLarge,
													  pdibRadioButton,
													  pdibCheckBox,
													  pfont,
													  textcolors,
													  iSound );
}

//--------------------------------------------------------------------------
// CGlobalSubClass::SubClassStatic
//--------------------------------------------------------------------------
void
CGlobalSubClass::SubClassStatic(
	CTextColors const & textcolors )
{
	ASSERT( g_ptrthesubclassx.Value() );

	if ( g_ptrthesubclassx.Value() )
		g_ptrthesubclassx->SubClassStatic( textcolors );
}

//--------------------------------------------------------------------------
// CGlobalSubClass::DrawButton
//--------------------------------------------------------------------------
BOOL
CGlobalSubClass::DrawButton(
	DRAWITEMSTRUCT * pdrawitem )
{
	ASSERT_STRICT( ODT_BUTTON == pdrawitem->CtlType );

	CGlobalSubClassButton	*psubclassbutton = g_ptrsubclassbutton.Value();

	if ( !psubclassbutton )
		return FALSE;

	if ( ! g_ptrsubclassdialog.Value() )
		return FALSE;

	return DrawButton( pdrawitem,
							 psubclassbutton->m_pdibPushButtonSmall,
							 psubclassbutton->m_pdibPushButtonLarge,
							 g_ptrsubclassdialog->m_pdibBkgnd,
							 psubclassbutton->m_pfont,
							 psubclassbutton->m_textcolors );
}

//--------------------------------------------------------------------------
// CGlobalSubClass::DrawButton
//--------------------------------------------------------------------------
BOOL
CGlobalSubClass::DrawButton(
	DRAWITEMSTRUCT 	* pdrawitem,
	CDIB					* pdibBtnSmall,
	CDIB					* pdibBtnLarge,
	CDIB					* pdibBackground,
	CFont					* pfont,
	CTextColors const & textcolors )
{
	ASSERT_STRICT( ODT_BUTTON == pdrawitem->CtlType );

	HWND	 hwndBtn = pdrawitem->hwndItem;
	HDC	 hdcCtl  = pdrawitem->hDC;
	int	 iIndex  = 0;
	int	 iResult;

	if ( !hdcCtl )
		return FALSE;

	if ( ODS_DISABLED & pdrawitem->itemState )
		iIndex = 2;
	else if ( ODS_SELECTED & pdrawitem->itemState )
		iIndex = 1;
	else if ( ODS_FOCUS & pdrawitem->itemState )
		iIndex = 3;

	CRect	rectBtn;

	::GetClientRect( hwndBtn, ( RECT * )&rectBtn );

	CDIB * pdibBtn;

	if ( !pdibBtnLarge )
		pdibBtn = pdibBtnSmall;
	else
	{
		if ( rectBtn.Height() > ( pdibBtnLarge->GetHeight() + pdibBtnSmall->GetHeight() ) >> 1 ||
			  rectBtn.Width () > ( pdibBtnLarge->GetWidth () + pdibBtnSmall->GetWidth () ) >> 1 )
			pdibBtn = pdibBtnLarge;
		else
			pdibBtn = pdibBtnSmall;
	}

	// Draw rect from the background dib to the target dib

	Ptr< CDIB >	ptrdib = MakeBackgroundDIB( pdibBackground, hwndBtn );

	if ( !ptrdib.Value() )
		return FALSE;

	CRect	rectSrc;
	CRect	rectDst;

	// Blt the button over the background, with transparency

	rectDst = ptrdib->GetRect();
	rectSrc = CRect( iIndex * pdibBtn->GetWidth() / 4, 
						  0,
						  ( iIndex + 1 ) * pdibBtn->GetWidth() / 4,
						  pdibBtn->GetHeight() );

	pdibBtn->StretchTranBlt( ptrdib.Value(), rectDst, rectSrc );

	// draw the text

	CString 	sText;

	CWnd::FromHandle( hwndBtn )->GetWindowText( sText );

	#define BUTTON_BORDER_WIDTH	8
	#define BUTTON_BORDER_HEIGHT	5

	rectDst.DeflateRect( BUTTON_BORDER_WIDTH, BUTTON_BORDER_HEIGHT );

	HDC	hdcDIB = ptrdib->GetDC();

	if ( !hdcDIB )
		return FALSE;
	thePal.Paint( hdcDIB );

	CFont		font;
	unsigned	uFormat = DT_VCENTER | DT_SINGLELINE;
	int		iFontHeight = ::MulDiv( 8, ::GetDeviceCaps( hdcDIB, LOGPIXELSY ), 72 );

	if ( ! FitFont( pfont, &font, hdcDIB, sText, rectDst, uFormat, iFontHeight, 6 ))
		{
		thePal.EndPaint( hdcDIB );
		ptrdib->ReleaseDC();
		return FALSE;
		}

	uFormat |= DT_CENTER;

	int 	nBkModePrev 	 = ::SetBkMode( hdcDIB, TRANSPARENT );
	HFONT	hfontPrev;
	if ( font.m_hObject != NULL)
		hfontPrev		 = ( HFONT )::SelectObject( hdcDIB, font.m_hObject );
	else
		hfontPrev		 = NULL;
	DWORD dwTextColorPrev = ::SetTextColor( hdcDIB, textcolors.GetHighlightColor() );

	if ( 1 == iIndex )
		rectDst.OffsetRect( 1, 2 );

	rectDst.OffsetRect( 1, 1 );

	::DrawText( hdcDIB, sText, -1, &rectDst, uFormat );

	rectDst.OffsetRect( -2, -2 );

	::SetTextColor( hdcDIB, textcolors.GetShadowColor() );

	iResult = ::DrawText( hdcDIB, sText, -1, &rectDst, uFormat );

	rectDst.OffsetRect( 1, 1 );

	if ( 0 == ( pdrawitem->itemState & ( ODS_GRAYED | ODS_DISABLED )))
	{
		::SetTextColor( hdcDIB, textcolors.GetTextColor() );

		iResult = ::DrawText( hdcDIB, sText, -1, &rectDst, uFormat );
	}

	if ( hfontPrev )
		::SelectObject( hdcDIB, hfontPrev );

  	::SetTextColor( hdcDIB, dwTextColorPrev );
   ::SetBkMode   ( hdcDIB, nBkModePrev );

	// Draw the button and background to the screen

	thePal.Paint( hdcCtl );

	rectDst = pdrawitem->rcItem;

	iResult = ptrdib->BitBlt( hdcCtl, rectDst, CPoint( 0, 0 ));

	thePal.EndPaint( hdcCtl );
	thePal.EndPaint( hdcDIB );
	ptrdib->ReleaseDC();

	return TRUE;
}

//--------------------------------------------------------------------------
// CGlobalSubClass::DrawCheckBox
//--------------------------------------------------------------------------
BOOL
CGlobalSubClass::DrawCheckBox(
	HWND hWnd )
{
	CGlobalSubClassButton	* psubclassbutton = g_ptrsubclassbutton.Value();

	return DrawCheckButton( hWnd, psubclassbutton->m_pdibCheckBox );
}

//--------------------------------------------------------------------------
// CGlobalSubClass::DrawRadioButton
//--------------------------------------------------------------------------
BOOL
CGlobalSubClass::DrawRadioButton(
	HWND	hWnd )
{
	CGlobalSubClassButton	* psubclassbutton = g_ptrsubclassbutton.Value();

	return DrawCheckButton( hWnd, psubclassbutton->m_pdibRadioButton );
}

//--------------------------------------------------------------------------
// CGlobalSubClass::DrawCheckButton
//--------------------------------------------------------------------------
BOOL
CGlobalSubClass::DrawCheckButton(
	HWND	 hWnd,
	CDIB * pdibCheckButton )
{
	CGlobalSubClassStatic	* psubclassstatic = g_ptrsubclassstatic.Value();
	CGlobalSubClassButton	* psubclassbutton = g_ptrsubclassbutton.Value();

	if ( !psubclassstatic )
		return FALSE;

	if ( !psubclassbutton )
		return FALSE;

	if ( !g_ptrsubclassdialog.Value() )
		return FALSE;

	CRect		rectDst;
	CFont  * pfontPrev = NULL;

	::GetClientRect( hWnd, &rectDst );

	if ( rectDst.IsRectEmpty() )
		return TRUE;

	CRect			rectCtl = rectDst;
	Ptr< CDIB >	ptrdib  = g_ptrsubclassdialog->MakeBackgroundDIB( hWnd );

	if ( ! ptrdib.Value() )
		return FALSE;

	// draw the button

	int	iIndex  = 0;
	DWORD	dwStyle = ::GetWindowLong( hWnd, GWL_STYLE );

	if ( 1 != SendMessage( hWnd, BM_GETCHECK, 0, 0 ))
		iIndex++;

	BOOL	bDisabled = 0 != ( WS_DISABLED & dwStyle );

	if ( bDisabled )
		iIndex += 2;

	CRect		rectDstCheckButton( 0, 0, pdibCheckButton->GetWidth() / 4, pdibCheckButton->GetHeight() );
	CPoint	ptSrcCheckButton  ( iIndex * pdibCheckButton->GetWidth() / 4, 0 );

	int	iShiftY = ( rectDst.Height() - rectDstCheckButton.Height() ) / 2;

	rectDstCheckButton.top    += iShiftY;
	rectDstCheckButton.bottom += iShiftY;

	pdibCheckButton->TranBlt( ptrdib.Value(), rectDstCheckButton, ptSrcCheckButton );

	// draw the text

	HDC	hdc = ptrdib->GetDC();

	if ( !hdc )
		return FALSE;

	CDC * pdc = CDC::FromHandle( hdc ); 
	thePal.Paint ( pdc->m_hDC );

	CString 	 sText;

	rectDst.left += 3 * pdibCheckButton->GetWidth() / 8;

	CWnd::FromHandle( hWnd )->GetWindowText( sText );

	HFONT	hfont = ( HFONT )::SendMessage( hWnd, WM_GETFONT, 0, 0 );

	CFont	* pfont = NULL;

	if ( hfont )
	{
		pfont     = CFont::FromHandle( hfont );
		pfontPrev = pdc->SelectObject( pfont );
	}

   int 	nBkModePrev     = pdc->SetBkMode( TRANSPARENT );
	DWORD dwTextColorPrev = pdc->GetTextColor();

	unsigned	uFormat = DT_EXPANDTABS | DT_LEFT | DT_SINGLELINE | DT_VCENTER;

	rectDst.OffsetRect( 1, 1 );

	pdc->SetTextColor( psubclassstatic->m_textcolors.GetHighlightColor() );
	pdc->DrawText    ( sText, -1, &rectDst, uFormat );

	rectDst.OffsetRect( -2, -2 );

	pdc->SetTextColor( psubclassstatic->m_textcolors.GetShadowColor() );
	pdc->DrawText    ( sText, -1, &rectDst, uFormat );

	rectDst.OffsetRect( 1, 1 );

	if ( ! bDisabled )
	{
		pdc->SetTextColor( psubclassstatic->m_textcolors.GetTextColor() );
		pdc->DrawText    ( sText, -1, &rectDst, uFormat );
	}

	if ( dwTextColorPrev )
	  	pdc->SetTextColor( dwTextColorPrev );

	if ( pfontPrev )
		pdc->SelectObject( pfontPrev );

   pdc->SetBkMode( nBkModePrev );

  	CPaintDC	dcWnd( CWnd::FromHandle( hWnd ));

	thePal.Paint  ( dcWnd.m_hDC );
	ptrdib->BitBlt( dcWnd.m_hDC, rectCtl, CPoint( 0, 0 ));

	thePal.EndPaint  ( dcWnd.m_hDC );
	thePal.EndPaint ( pdc->m_hDC );
	ptrdib->ReleaseDC();

	return TRUE;
}

//--------------------------------------------------------------------------
// CGlobalSubClass::DrawGroupBox
//--------------------------------------------------------------------------
BOOL
CGlobalSubClass::DrawGroupBox(
	HWND	hWnd )
{
	CGlobalSubClassStatic	*psubclassstatic = g_ptrsubclassstatic.Value();

	if ( !psubclassstatic )
		return FALSE;

	if ( !g_ptrsubclassdialog.Value() )
		return FALSE;

	CRect	rectDst;

	::GetClientRect( hWnd, &rectDst );

	if ( rectDst.IsRectEmpty() )
		return TRUE;

	// draw the lines

  	CPaintDC	dcCtl( CWnd::FromHandle( hWnd ));

	thePal.Paint( dcCtl.m_hDC );

	HFONT	hfont     = ( HFONT )::SendMessage( hWnd, WM_GETFONT, 0, 0 );
	HFONT	hfontPrev = NULL;
	
	if ( hfont )
		hfontPrev = ( HFONT )::SelectObject( dcCtl.m_hDC, hfont );

	rectDst.left += 2;
	rectDst.top  += 2;

	CPen	 	* ppenOld   = NULL;
	CBrush 	* pbrush    = CBrush::FromHandle(( HBRUSH )GetStockObject( HOLLOW_BRUSH ));
	CBrush 	* pbrushOld = dcCtl.SelectObject( pbrush );
	CSize		  sizeA     = dcCtl.GetTextExtent( "A", 1 );
	CPoint	  ptCorner  = CPoint( sizeA.cy, sizeA.cy );

	CPen		  penHighlight( PS_SOLID, 1, psubclassstatic->m_textcolors.GetHighlightColor() );
	CPen		  penText     ( PS_SOLID, 1, psubclassstatic->m_textcolors.GetTextColor()      );
	CPen		  penShadow   ( PS_SOLID, 1, psubclassstatic->m_textcolors.GetShadowColor()    );

	rectDst.top += sizeA.cy >> 1;

	ppenOld = dcCtl.SelectObject( &penHighlight );

	dcCtl.RoundRect   ( rectDst, ptCorner );

	rectDst.OffsetRect  ( -1, -1 );
	dcCtl  .SelectObject( & penText );
	dcCtl  .RoundRect   ( rectDst, ptCorner );

	rectDst.OffsetRect  ( -1, -1 );
	dcCtl  .SelectObject( & penShadow );
	dcCtl  .RoundRect   ( rectDst, ptCorner );

	if ( ppenOld )
		dcCtl.SelectObject( ppenOld );

	if ( pbrushOld )
		dcCtl.SelectObject( pbrushOld );

	// Draw caption

	CString 	strText;

	CWnd::FromHandle( hWnd )->GetWindowText( strText );

	CSize	sizeText	= dcCtl.GetTextExtent( strText );
	CRect	rectCtl	= CGlobalSubClass::GetRect( hWnd );

	if ( hfontPrev )
		::SelectObject( dcCtl.m_hDC, hfontPrev );

	sizeText.cx += sizeA.cx;
	sizeText.cy += 1;

	CRect rectText = CRect( CPoint( sizeA.cx, 0 ), sizeText );

	rectDst   = rectText;
	rectText += rectCtl.TopLeft();

	Ptr< CDIB >	ptrdib  = g_ptrsubclassdialog->MakeBackgroundDIB( hWnd, rectText );

	if ( ! ptrdib.Value() )
		{
		thePal.EndPaint( dcCtl.m_hDC );
		return FALSE;
		}

	HDC	hdcdib  = ptrdib->GetDC();

	if ( !hdcdib )
		{
		thePal.EndPaint( dcCtl.m_hDC );
		return FALSE;
		}

	CDC * pdcDIB = CDC::FromHandle( hdcdib );
	thePal.Paint( pdcDIB->m_hDC );

	CFont	* pfont		= NULL;
	CFont * pfontPrev = NULL;

	if ( hfont )
	{
		pfont     = CFont::FromHandle( hfont );
		pfontPrev = pdcDIB->SelectObject( pfont );
	}

   int 	nBkModePrev     = pdcDIB->SetBkMode( TRANSPARENT );
	DWORD dwTextColorPrev = pdcDIB->GetTextColor();

	CRect	rectDIB = ptrdib->GetRect();

	rectDIB.OffsetRect( 1, 1 );

	unsigned	uFormat = DT_EXPANDTABS | DT_CENTER | DT_SINGLELINE | DT_VCENTER;

	pdcDIB->SetTextColor( psubclassstatic->m_textcolors.GetHighlightColor() );
	pdcDIB->DrawText    ( strText, -1, &rectDIB, uFormat );

	rectDIB.OffsetRect( -2, -2 );

	pdcDIB->SetTextColor( psubclassstatic->m_textcolors.GetShadowColor() );
	pdcDIB->DrawText    ( strText, -1, &rectDIB, uFormat );

	rectDIB.OffsetRect( 1, 1 );

	pdcDIB->SetTextColor( psubclassstatic->m_textcolors.GetTextColor() );
	pdcDIB->DrawText    ( strText, -1, &rectDIB, uFormat );

  	pdcDIB->SetTextColor( dwTextColorPrev );

	if ( pfontPrev )
		pdcDIB->SelectObject( pfontPrev );

   pdcDIB->SetBkMode( nBkModePrev );

	thePal.Paint  ( dcCtl.m_hDC );
	ptrdib->BitBlt( dcCtl.m_hDC, rectDst, CPoint( 0, 0 ));

	thePal.EndPaint( dcCtl.m_hDC );
	thePal.EndPaint( pdcDIB->m_hDC );

	ptrdib->ReleaseDC();

	return TRUE;
}

//--------------------------------------------------------------------------
// CGlobalSubClass::Subclass
//--------------------------------------------------------------------------
void
CGlobalSubClass::Subclass(
	CDIB  				* pdibBkgnd,
	CDIB  				* pdibPushButtonSmall,
	CDIB  				* pdibPushButtonLarge,
	CDIB  				* pdibRadioButton,
	CDIB  				* pdibCheckBox,
	CFont 				* pfontButton,
	int					  iSoundID,
	CTextColors const & textcolorsButton,
	CTextColors const & textcolorsStatic )
{
	Init();

	SubClassDialog( pdibBkgnd );

	SubClassButton( pdibPushButtonSmall,
						 pdibPushButtonLarge,
						 pdibRadioButton,
						 pdibCheckBox,
						 pfontButton,
						 textcolorsButton,
						 iSoundID );

	SubClassStatic( textcolorsStatic );

	g_bSubclassing = TRUE;
}

//--------------------------------------------------------------------------
// CGlobalSubClass::UnSubclass
//--------------------------------------------------------------------------
void
CGlobalSubClass::UnSubClass()
{
	if ( g_ptrsubclassdialog.Value() )
		g_ptrsubclassdialog->RestoreProc();

	if ( g_ptrsubclassbutton.Value() )
		g_ptrsubclassbutton->RestoreProc();

	if ( g_ptrsubclassstatic.Value() )
		g_ptrsubclassstatic->RestoreProc();

	g_bSubclassing = FALSE;
}

//---------------------------- C W n d O D < T > ----------------------------

//--------------------------------------------------------------------------
// CWndOD<T>::OnChildNotify - MFC reflects WM_DRAWITEM to the control,
//								 		but we want the parent to handle the message
//--------------------------------------------------------------------------
template <class T>
BOOL
CWndOD<T>::OnChildNotify(
	UINT		 message,
	WPARAM	 wParam,
	LPARAM	 lParam,
	LRESULT * presult )
{
	if ( message == WM_DRAWITEM )
		return FALSE;

	return T::OnChildNotify( message, wParam, lParam, presult );
}

static CWndOD< CButton >	g_btnFake;	// Get the template code instantiated
