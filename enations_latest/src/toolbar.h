//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__


#include "bmbutton.h"
#include "icons.h"
#include "resource.h"
#include "ui.h"


const int BAR_BTN_X_SKIP  = 4;   // x skip between buttons
const int BAR_BTN_Y_START = 4;   // where buttons start on the background
const int BAR_BTN_HT      = 38;  // height of button backdrop
const int BAR_TEXT_HT     = 28;  // height of text backdrop
const int TOOLBAR_HT      = BAR_BTN_HT + BAR_TEXT_HT;


typedef void ( *FNSTATUSLINE )( void* pData, CDC* pDc, CRect const& rDraw, CDIB* pDib, CPoint const& ptOff );


/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat window

class CWndStatLine : public CWndStatBar
{
    // Construction
  public:
    CWndStatLine( );
    void  SetStatusFunc( void ( *fnStat )( void* pData, CDC* pDc, CRect const& rDraw, CDIB* pDib, CPoint const& ptOff ),
                         void* pData = NULL );
    void  SetText( char const* pText, CStatInst::IMPORTANCE iImp = CStatInst::status );
    void* GetStatusData( ) const { return m_pFnData; }

    // Generated message map functions
  protected:
    //{{AFX_MSG(CWndUnitStat)
    afx_msg void OnPaint( );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP( )

    FNSTATUSLINE m_fnStatus;
    void*        m_pFnData;
};


/////////////////////////////////////////////////////////////////////////////
// CWndBar window

const int NUM_BAR_BTNS = 8;
class CWndBar : public CWndAnim
{
    // Construction
  public:
    CWndBar( );

    void Create( );
    void CheckButtons( );
    void _GotoScience( );

    void InvalidateStatus( void* pData );
    void EnableButton( int ID, BOOL bEnable );

    void SetStatusFunc( int iLine,
                        void ( *fn )( void* pData, CDC* pDc, CRect const& rDraw, CDIB* pDib, CPoint const& ptOff ),
                        void* pData = NULL );
    void SetStatusText( int iLine, const char* psText = NULL, CStatInst::IMPORTANCE iImp = CStatInst::status );
#ifdef _CHEAT
    void SetDebugText( int iLine, const char* psText );
#endif

    void UpdateHelp( CWnd* pWnd );
#ifdef ENATIONS_USE_STUB_WND
    void UpdateHelp( CWndStub* pWnd ) { UpdateHelp( CWnd::FromHandle( pWnd ? pWnd->m_hWnd : NULL ) ); }
#endif
    void UpdateGas( );
    void UpdatePower( );
    void UpdatePeople( );
    void UpdateFood( );
    void UpdateTime( );

    void ReRender() {}  // GDI-based; no pre-render needed
    void Draw();        // Capture GDI content to SDL panel

    // True once Create() has run and the SDL panel exists. Replaces the
    // legacy `m_wndBar.m_hWnd != NULL` "is initialized?" check at ~18 sites.
    bool IsCreated() const { return m_sdlPanel != nullptr; }

    // Wrappers that hide the legacy CWndStatBar / CWndStatLine children from
    // callers, so we can drop those children when the MFC HWND goes away.
    void FlashLowIcon( int idx );          // mainloop's low-resource flasher
    void* GetStatusLineData( int line );    // status-callback owner (for "is this me?")
    void ClearStatusFunc( int line );       // detach status-callback
    void InvalidateStatusLine( int line );  // force redraw of a text line

    // Shadow the CWndStub geometry/visibility methods so callers that target
    // the bar (m_wndBar.SetWindowPos / .ShowWindow / .GetWindowRect) route
    // through the SDL panel once the MFC HWND is gone. With the HWND still
    // present these just delegate to the base implementation.
    BOOL SetWindowPos( HWND hwndAfter, int x, int y, int cx, int cy, UINT flags );
    BOOL ShowWindow( int nCmdShow );
    BOOL GetWindowRect( RECT* pRect ) const;

    // Shadow DestroyWindow so the SDL panel + SDL2Toolbar get torn down even
    // when there's no MFC HWND. Without this, CWndStub::DestroyWindow sees
    // m_hWnd == NULL, returns FALSE immediately, and OnDestroy never fires —
    // leaving the toolbar panel in the compositor where it intercepts input
    // after the world closes (hangs the main menu on quit-to-menu).
    BOOL DestroyWindow();

    class SDL2Panel* m_sdlPanel = nullptr;

  public:
    // Generated message map functions
    //{{AFX_MSG(CWndBar)
    afx_msg void    GotoArea( );
    afx_msg void    GotoWorld( );
    afx_msg void    GotoChat( );
    afx_msg void    GotoVehicles( );
    afx_msg void    GotoBuildings( );
    afx_msg void    GotoRelations( );
    afx_msg void    GotoScience( );
    afx_msg void    GotoFile( );
    afx_msg int     OnCreate( LPCREATESTRUCT lpCreateStruct );
    afx_msg void    OnTimer( UINT nIDEvent );
    afx_msg void    OnPaint( );
    afx_msg void    OnSize( UINT nType, int cx, int cy );
    afx_msg void    OnClose( );
    afx_msg LRESULT OnButtonMouseMove( WPARAM wParam, LPARAM lParam );
    afx_msg LRESULT OnStatusMouseMove( WPARAM wParam, LPARAM lParam );
    afx_msg void    OnDestroy( );
    afx_msg void    OnMouseMove( UINT nFlags, CPoint point );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP( )

  public:
    CBmButton m_BmBtns[NUM_BAR_BTNS];  // buttons

    CWndStatBar m_wndStat[4];  // 4 status bars
    enum
    {
        gas,
        power,
        people,
        food
    };
    CWndStatBar  m_wndTime;     // time clock
    CWndStatLine m_wndText[2];  // 2 text lines

    static const int aID[NUM_BAR_BTNS];
    static const int aBtn[NUM_BAR_BTNS];
    static const int aHelp[NUM_BAR_BTNS];
    static std::string m_sChat1;
    static std::string m_sChat2;
    static std::string m_sScience;
    static std::string m_sRelations;
};


#endif
