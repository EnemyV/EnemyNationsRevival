//--------------------------------------------------------------------------
//	flcctrl.h		CFlcCtrl class
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef __CFLCCTRL_H__
#define __CFLCCTRL_H__

#include "thielen.h"
#include "flcanim.h"
#include "dibwnd.h"

#define	FLC_PLAY  1
#define  FLC_STOP  2
#define  FLC_KEY   3

//------------------------- C F l c P a u s e I n f o -----------------------

class CFlcPauseInfo
{

public:

	CFlcPauseInfo()
		:
			m_iStartFrame( 0 ),
			m_nFrames	 ( 0 )
	{}

	CFlcPauseInfo( int iStartFrame, int nFrames )
		:
			m_iStartFrame( iStartFrame ),
			m_nFrames	 ( nFrames )
	{}

	int	GetStartFrame() const { return m_iStartFrame; }
	int	GetNumFrames()  const { return m_nFrames; }

	private:

	int	m_iStartFrame;
	int	m_nFrames;
};

//------------------------------ C F l c I n f o ---------------------------

class CFlcInfo
{

public:

	CFlcInfo();

	void	AddPause	 ( CFlcPauseInfo const & );
	void	SetFade   (	int iFadeInFrames, int iFadeOutFrames );

	int	GetPause	 ( int iFrame ) 	const;
	int	GetFadeIn () 					const { return m_nFadeInFrames; }
	int	GetFadeOut() 					const { return m_nFadeOutFrames; }

private:

	int	m_nFadeInFrames;
	int	m_nFadeOutFrames;

	CList< CFlcPauseInfo, CFlcPauseInfo & >	m_listpauseinfo;
};

//------------------------------ C F l c C t r l ---------------------------

class CFlcCtrl : public CStatic
{

public:

	DECLARE_DYNAMIC( CFlcCtrl )

	CFlcCtrl();
  ~CFlcCtrl();

	BOOL	Create( DWORD dwStyle, const RECT & rect, CWnd * pParentWnd, UINT nID );

	void	Play	( Ptr< CFile > const &, Ptr< CFlcInfo > const & );
	void	Stop	();

	BOOL	IsPlaying() 		const { return m_bPlaying;      }

	int	GetFramesPlayed()	const { return m_nFramesPlayed;   }
	int	GetFrameNumber()	const { return GetFramesPlayed(); }

	#ifdef _DEBUG
	void AssertValid() const;
	#endif

protected:

	enum FRAME_ACTION { STOP_FLIC, PAUSE_FRAME, RENDER_FRAME, FADEIN_FRAME, FADEOUT_FRAME };

	void	Load  ( Ptr< CFile > const & );
	void	Unload();
	void	FadeIn();
	void	FadeOut();
	void	Render();
	void	DoPostRender();
	void	UpdatePalette();

	BOOL	IsNextFrameTime() 				  const;
	void	NotifyParent	( int iMessage ) const;
	
	FRAME_ACTION	GetFrameAction();

	//{{AFX_MSG(CFlcCtrl)
	afx_msg	int	OnCreate ( LPCREATESTRUCT lpCreateStruct );
	afx_msg	void	OnPaint();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg 	void 	OnSize(UINT nType, int cx, int cy);
	afx_msg 	void 	OnDestroy();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

	BOOL					m_bPlaying;
	Ptr< CFlcAnim >	m_ptrflcanim;
	Ptr< CFlcInfo >	m_ptrflcinfo;
	CDIBWnd				m_dibwnd;
	DWORD					m_dwFrameTime;
	int					m_nFramesPlayed;
	int					m_nPauseFrames;
	int					m_nFadeInFrames;
	int					m_nFadeOutFrames;
	RGBQUAD			 *	m_prgbquadCur;
	RGBQUAD			 *	m_prgbquadFlic;
};

#endif

