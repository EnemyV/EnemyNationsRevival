//--------------------------------------------------------------------------
//	flcctrl.cpp		CFlcCtrl class
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
// FIXIT: error-handling
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "_windwrd.h"
#include "flcctrl.h"

IMPLEMENT_DYNAMIC( CFlcCtrl, CStatic )

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW



BEGIN_MESSAGE_MAP(CFlcCtrl, CStatic)
	//{{AFX_MSG_MAP(CFlcCtrl)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//--------------------------------------------------------------
// CFlcInfo::CFlcInfo
//--------------------------------------------------------------
CFlcInfo::CFlcInfo()
	:
		m_nFadeInFrames ( 0 ),
		m_nFadeOutFrames( 0 )
{}

//--------------------------------------------------------------
// CFlcInfo::AddPause
//--------------------------------------------------------------
void
CFlcInfo::AddPause(
	CFlcPauseInfo const & pauseinfo )
{
	m_listpauseinfo.AddTail(( CFlcPauseInfo & )pauseinfo );
}

//--------------------------------------------------------------
// CFlcInfo::GetPause
//--------------------------------------------------------------
int
CFlcInfo::GetPause(
	int iFrame ) const
{
	for (	POSITION	position = m_listpauseinfo.GetHeadPosition(); NULL != position; )
	{
		CFlcPauseInfo	const & pauseinfo = m_listpauseinfo.GetNext( position );

		if ( iFrame == pauseinfo.GetStartFrame() )
			return pauseinfo.GetNumFrames();
	}

	return 0;
}

//--------------------------------------------------------------
// CFlcInfo::SetFadeIn
//--------------------------------------------------------------
void
CFlcInfo::SetFade(
	int iFramesFadeIn,
	int iFramesFadeOut )
{
	m_nFadeInFrames  = iFramesFadeIn;
	m_nFadeOutFrames = iFramesFadeOut;
}

//--------------------------------------------------------------
// CFlcCtrl::CFlcCtrl
//--------------------------------------------------------------
CFlcCtrl::CFlcCtrl()
	:
		m_bPlaying		 ( FALSE ),
		m_prgbquadCur	 ( NULL  ),
		m_prgbquadFlic	 ( NULL  )
{
	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// CFlcCtrl::~CFlcCtrl
//--------------------------------------------------------------
CFlcCtrl::~CFlcCtrl()
{
	delete [] m_prgbquadCur;
	delete [] m_prgbquadFlic;
}

//--------------------------------------------------------------------------
// CFlcCtrl::Create
//--------------------------------------------------------------------------
BOOL
CFlcCtrl::Create(
	DWORD 		dwStyle,
	const RECT& rect,
	CWnd* 		pParentWnd,
	UINT 			nID )
{
	ASSERT_VALID( this );

	BOOL	bRet = CStatic::Create( NULL, dwStyle, rect, pParentWnd, nID );

	ASSERT_VALID( this );

	return bRet;
}

//--------------------------------------------------------------------------
// CFlcCtrl::OnCreate
//--------------------------------------------------------------------------
int
CFlcCtrl::OnCreate(
	LPCREATESTRUCT lpCreateStruct )
{
	CStatic::OnCreate( lpCreateStruct );

	ASSERT( ptrthebltformat.Value() );

	CColorFormat					colorformat( CColorFormat::DEPTH_EIGHT );
	CBLTFormat::DIB_TYPE			edibtype = ptrthebltformat->GetType();
	CBLTFormat::DIB_DIRECTION	edibdir  = ptrthebltformat->GetDirection();

	if ( CBLTFormat::DIB_DIRECTDRAW == edibtype )
		edibtype = CBLTFormat::DIB_DIBSECTION;

	m_dibwnd.Init( m_hWnd, new CDIB( colorformat, edibtype, edibdir ));

	return 0;
}

//--------------------------------------------------------------
// CFlcCtrl::OnSize
//--------------------------------------------------------------
void
CFlcCtrl::OnSize(
	UINT 	nType,
	int 	cx,
	int 	cy ) 
{
	CStatic::OnSize( nType, cx, cy );
	
	CRect rect;

	GetClientRect( &rect );

	m_dibwnd.Size( rect.Width(), rect.Height() );
}

//--------------------------------------------------------------
// CFlcCtrl::OnDestroy
//--------------------------------------------------------------
void
CFlcCtrl::OnDestroy()
{
	ASSERT_VALID( this );

	Stop();

	CStatic::OnDestroy();

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// CFlcCtrl::OnPaint
//--------------------------------------------------------------
void
CFlcCtrl::OnPaint()
{
	CPaintDC dc( this ); // device context for painting

	m_dibwnd.Paint( dc.m_ps.rcPaint );
}

void CFlcCtrl::OnPaletteChanged(CWnd* pFocusWnd)
{
static BOOL bInFunc = FALSE;

	CStatic::OnPaletteChanged(pFocusWnd);

	// Win32s locks up if we do the below code
	if (iWinType == W32s)
		return;
		
	// stop infinite recursion
	if (bInFunc)
		return;
	bInFunc = TRUE;

	if (thePal.PalMsg (m_dibwnd.GetHDC (), m_dibwnd.GetHWND (), WM_PALETTECHANGED, (WPARAM) pFocusWnd->m_hWnd, 0))
		::InvalidateRect (m_dibwnd.GetHWND (), NULL, TRUE);

	bInFunc = FALSE;
}

BOOL CFlcCtrl::OnQueryNewPalette()
{

	// Win32s locks up if we do the below code
	if (iWinType == W32s)
		return CStatic::OnQueryNewPalette();

	thePal.PalMsg (m_dibwnd.GetHDC (), m_hWnd, WM_QUERYNEWPALETTE, 0, 0);

	return CStatic::OnQueryNewPalette();
}

//--------------------------------------------------------------------------
// CFlcCtrl::Play
//--------------------------------------------------------------------------
void
CFlcCtrl::Play(
	Ptr< CFile > 	 const & ptrFile,
	Ptr< CFlcInfo > const & ptrflcinfo )
{
	ASSERT_VALID( this );

	if ( IsPlaying() )
		return;

	m_ptrflcinfo = ptrflcinfo;

	Load( ptrFile );

	NotifyParent( FLC_PLAY );

	m_dwFrameTime 	= timeGetTime();
	m_bPlaying  	= TRUE;

	MSG	msg;

	for ( ;; )
	{ 
		while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE )) 
		{ 
			if ( !ptheApp->PumpMessage() ) 
			{ 
				m_bPlaying = FALSE;

            ::PostQuitMessage( 0 ); 

            break; 
			}

			if ( !IsPlaying() )
				return;
		} 

		if ( IsNextFrameTime() )
		
			switch ( GetFrameAction() )
			{
				case STOP_FLIC:		Stop();		break;
				case FADEIN_FRAME:	FadeIn();	break;
				case FADEOUT_FRAME:	FadeOut();	break;
				case RENDER_FRAME:	Render();	break;
				case PAUSE_FRAME:					 	break;

				default:	ASSERT( 0 );
			}
	}

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// CFlcCtrl::Stop
//--------------------------------------------------------------
void
CFlcCtrl::Stop()
{
	ASSERT_VALID( this );

	if ( !IsPlaying() )
	{
		ASSERT( !m_ptrflcanim.Value() );
		return;
	}

	m_dibwnd.GetDIB()->Clear();
	m_dibwnd.Invalidate();
	m_dibwnd.Update();

	m_bPlaying = FALSE;

	Unload();
	NotifyParent( FLC_STOP );

	ASSERT_VALID( this );
}

//--------------------------------------------------------------------------
// CFlcCtrl::Load
//--------------------------------------------------------------------------
void
CFlcCtrl::Load(
	Ptr< CFile > const & ptrfile )
{
	ASSERT_VALID( this );

	Unload();

	if ( m_ptrflcinfo.Value() )
		if ( m_ptrflcinfo->GetFadeIn() ||
			  m_ptrflcinfo->GetFadeOut() )
		{
			m_prgbquadCur  = new RGBQUAD [ 256 ];
			m_prgbquadFlic = new RGBQUAD [ 256 ];
		}

	m_ptrflcanim = new CFlcAnim( ptrfile );

	m_ptrflcanim->Init( &m_dibwnd );

	CRect	rect( 0, 0, m_ptrflcanim->GetWidth(),
							m_ptrflcanim->GetHeight() );

	CalcWindowRect( &rect );

	CRect	rectWnd;

	GetWindowRect( &rectWnd );

	int	iCenterX = rectWnd.left + rectWnd.Width()  / 2;
	int	iCenterY = rectWnd.top  + rectWnd.Height() / 2;
	int	iOffsetX = iCenterX 		- rect.Width()  	 / 2;
	int	iOffsetY = iCenterY 		- rect.Height() 	 / 2;

	CPoint	ptOffset = CPoint( iOffsetX, iOffsetY );

	rect += ptOffset;

	MoveWindow( &rect, FALSE );

	if ( m_ptrflcinfo->GetFadeIn() )
	{
		m_ptrflcanim->SetIgnorePalette( TRUE );

		memset( m_prgbquadCur, 0, 256 * sizeof( RGBQUAD ));

		UpdatePalette();
	}

	ASSERT_VALID( this );
}

//--------------------------------------------------------------------------
// CFlcCtrl::Unload
//--------------------------------------------------------------------------
void
CFlcCtrl::Unload()
{
	ASSERT_VALID( this );

	m_nFramesPlayed  = 0;
	m_nPauseFrames   = 0;
	m_nFadeInFrames  = 0;
	m_nFadeOutFrames = 0;
	m_ptrflcanim     = NULL;

	delete [] m_prgbquadCur;
	delete [] m_prgbquadFlic;

	m_prgbquadCur  = NULL;
	m_prgbquadFlic = NULL;

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// CFlcCtrl::IsNextFrameTime
//--------------------------------------------------------------
BOOL
CFlcCtrl::IsNextFrameTime() const
{
	ASSERT_VALID( this );

	ASSERT( IsPlaying() );

	DWORD	dwCurTime = timeGetTime();

	if ( dwCurTime >= m_dwFrameTime )
	{
		(( CFlcCtrl * )this )->m_dwFrameTime = dwCurTime + m_ptrflcanim->GetSpeed();

		return TRUE;
	}

	ASSERT_VALID( this );

	return FALSE;
}

//--------------------------------------------------------------
// CFlcCtrl::NotifyParent
//--------------------------------------------------------------
void
CFlcCtrl::NotifyParent(
	int iMessage ) const
{
	ASSERT_VALID( this );

	ASSERT( GetParent() );
	ASSERT( ::IsWindow( GetParent()->m_hWnd ));

	GetParent()->PostMessage( WM_COMMAND,
						  			  MAKEWPARAM( GetWindowLong( m_hWnd, GWL_ID ), iMessage ),
									  LPARAM( m_hWnd ));
	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// CFlcCtrl::GetFrameAction
//--------------------------------------------------------------
CFlcCtrl::FRAME_ACTION
CFlcCtrl::GetFrameAction()
{
	if ( m_nFadeInFrames )
	{
		m_nFadeInFrames--;

		return FADEIN_FRAME;
	}

	if ( m_nPauseFrames )
	{
		m_nPauseFrames--;

		return PAUSE_FRAME;
	}

	if ( m_nFadeOutFrames )
	{
		m_nFadeOutFrames--;

		return m_nFadeOutFrames ? FADEOUT_FRAME : STOP_FLIC;
	}
	return RENDER_FRAME;
}

//--------------------------------------------------------------
// CFlcCtrl::FadeIn
//--------------------------------------------------------------
void
CFlcCtrl::FadeIn()
{
	ASSERT( m_ptrflcinfo.Value() );

	int	n = m_ptrflcinfo->GetFadeIn();
	int	k = n - m_nFadeInFrames;

	ASSERT( n );

	for ( int i = 0; i < 256; ++i )
	{
		m_prgbquadCur[ i ].rgbRed   = k * m_prgbquadFlic[ i ].rgbRed   / n;
		m_prgbquadCur[ i ].rgbGreen = k * m_prgbquadFlic[ i ].rgbGreen / n;
		m_prgbquadCur[ i ].rgbBlue  = k * m_prgbquadFlic[ i ].rgbBlue  / n;
	}

	UpdatePalette();
}

//--------------------------------------------------------------
// CFlcCtrl::FadeOut
//--------------------------------------------------------------
void
CFlcCtrl::FadeOut()
{
	ASSERT( m_ptrflcinfo.Value() );

	int	n = m_ptrflcinfo->GetFadeOut();
	int	k = m_nFadeOutFrames - 1;

	ASSERT( 0 <= k );
	ASSERT( n );

	for ( int i = 0; i < 256; ++i )
	{
		m_prgbquadCur[ i ].rgbRed   = k * m_prgbquadFlic[ i ].rgbRed   / n;
		m_prgbquadCur[ i ].rgbGreen = k * m_prgbquadFlic[ i ].rgbGreen / n;
		m_prgbquadCur[ i ].rgbBlue  = k * m_prgbquadFlic[ i ].rgbBlue  / n;
	}

	UpdatePalette();
}

//--------------------------------------------------------------
// CFlcCtrl::UpdatePalette
//--------------------------------------------------------------
void
CFlcCtrl::UpdatePalette()
{
	thePal.SetColors( m_prgbquadCur, 0, 256 );

	// FIXIT: move this into CDibWnd
	m_dibwnd.GetDIB()->SyncPalette();
	m_dibwnd.Invalidate();	// FIXIT: AnimatePalette() doesn't seem to do anything
}

//--------------------------------------------------------------
// CFlcCtrl::RenderFrame
//--------------------------------------------------------------
void
CFlcCtrl::Render()
{
	if ( m_ptrflcanim->DisplayNextFrame() )
		DoPostRender();
	else if ( m_ptrflcinfo.Value() && m_ptrflcinfo->GetFadeOut() )
	{
		m_nFadeOutFrames = m_ptrflcinfo->GetFadeOut();

		ASSERT( m_ptrflcanim->GetPalette() );

		memcpy( m_prgbquadFlic, m_ptrflcanim->GetPalette(), 256 * sizeof( RGBQUAD ));
	}
	else
		Stop();
}

//--------------------------------------------------------------
// CFlcCtrl::DoPostRender - Call after a real frame is rendered
//--------------------------------------------------------------
void
CFlcCtrl::DoPostRender()
{
	m_nFramesPlayed++;

	if ( m_ptrflcinfo.Value() )
	{
		m_nPauseFrames = m_ptrflcinfo->GetPause( GetFrameNumber() );

		if ( 1 == m_nFramesPlayed )
		{
			m_nFadeInFrames = m_ptrflcinfo->GetFadeIn();

			if ( m_nFadeInFrames )
			{
				ASSERT( m_ptrflcanim->GetPalette() );

				memcpy( m_prgbquadFlic, m_ptrflcanim->GetPalette(), 256 * sizeof( RGBQUAD ));

				m_ptrflcanim->SetIgnorePalette( FALSE );
			}
		}
	}
}

//--------------------------------------------------------------
// CFlcCtrl::AssertValid
//--------------------------------------------------------------
#ifdef _DEBUG
void
CFlcCtrl::AssertValid() const
{
	CStatic::AssertValid();

	if ( m_bPlaying )
		ASSERT_VALID( m_ptrflcanim.Value() );

	ASSERT_VALID( &m_dibwnd );
}
#endif

//--------------------------------------------------------------
// CFlcCtrl::OnKeyDown
//--------------------------------------------------------------
void CFlcCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CStatic::OnKeyDown( nChar, nRepCnt, nFlags );

	NotifyParent( FLC_KEY );
}


