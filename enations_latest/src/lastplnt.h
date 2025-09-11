//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __LASTPLNT_H__
#define __LASTPLNT_H__

// lastplnt.h : main header file for The Last Planet application
//

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"
#include "version.h"

// #include "area.h"
#include "credits.h"
#include "cutscene.h"
#include "ipccomm.h"
#include "movie.h"
#include "new_game.h"
#include "toolbar.h"
#include "ui.h"
#include "unit_wnd.h"
#include "world.h"


class CConquerApp;
class CCreateBase;
class CDlgPlyrList;
class CDlgRelations;
class CDlgResearch;

#ifdef _CHEAT
extern BOOL _bShowRate;
extern BOOL _bClickAny;
extern BOOL _bMaxMaterials;
extern BOOL _bMaxRocket;
extern BOOL _bMaxPower;
extern int  _iFrameRate;
extern BOOL _bShowWorld;
extern BOOL _bShowStatus;
extern BOOL _bSeeAll;
extern BOOL _bShowPos;
extern BOOL _bShowAISelected;
extern int  _iScenarioOn;
#endif


/////////////////////////////////////////////////////////////////////////////
// This garbage is so we can get the exception address from the structured
// exception handler into our catch

const int NUM_EXCEP = 10;

class SE_Exception
{
  public:
    SE_Exception( ) {}
    ~SE_Exception( ) {}
    SE_Exception( unsigned uEc, void* pExCode )
    {
        m_uEc     = uEc;
        m_pExCode = pExCode;
    }

    unsigned int m_uEc;      // exception code
    void*        m_pExCode;  // the location of the exception code
    int          m_stack[NUM_EXCEP];
};

typedef void ( *TRANS_FUNC )( unsigned int u, EXCEPTION_POINTERS* pExp );

extern TRANS_FUNC prevFn;
void              CatchNum( int iNum );
void              CatchSE( SE_Exception e );
void              CatchOther( );


/////////////////////////////////////////////////////////////////////////////
// AI stuff for building a world
class AIinit
{
  public:
    AIinit( int iDiff, int iNumAI, int iNumHp, int iSize )
    {
        m_iDiff  = iDiff;
        m_iNumAI = iNumAI;
        m_iNumHp = iNumHp;
        m_iSize  = iSize;
    }

    int m_iDiff;
    int m_iNumAI;
    int m_iNumHp;
    int m_iSize;
};


/////////////////////////////////////////////////////////////////////////////
// CDlgMain dialog
// maybe the dialouge that comes up when the program starts?
// i think it's multiple, but not sure

class CDlgMain : public CDialog
{
    // Construction
  public:
    CDlgMain( CWnd* pParent = NULL );  // standard constructor
    ~CDlgMain( );

    enum
    {
        NUM_BTNS = 11
    };

    // Dialog Data
    //{{AFX_DATA(CDlgMain)
    enum
    {
        IDD = IDD_MAIN
    };
    // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // Implementation
  protected:
    void UpdateBlk( );

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDlgMain)
    virtual void PostNcDestroy( ) { delete this; }
    //}}AFX_VIRTUAL

    virtual void DoDataExchange( CDataExchange* pDX );  // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDlgMain)
    afx_msg int  OnCreate( LPCREATESTRUCT lpCreateStruct );
    virtual BOOL OnInitDialog( );
    virtual void OnOK( ) {}
    virtual void OnCancel( ) {}
    afx_msg void OnPaint( );
    afx_msg void OnMainScenario( );
    afx_msg void OnMainSingle( );
    afx_msg void OnMainCreate( );
    afx_msg void OnMainJoin( );
    afx_msg void OnMainLoad( );
    afx_msg void OnMainExit( );
    afx_msg void OnSize( UINT nType, int cx, int cy );
    afx_msg void OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct );
    afx_msg void OnMinimize( );
    afx_msg void OnMainLoadMulti( );
    afx_msg void OnMainCredits( );
    afx_msg void OnMainIntro( );
    afx_msg void OnMainOptions( );
    afx_msg void OnDestroy( );
    afx_msg void OnPaletteChanged( CWnd* pFocusWnd );
    afx_msg BOOL OnQueryNewPalette( );
    afx_msg BOOL OnEraseBkgnd( CDC* pDC );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP( )

    CDIB* m_pcdibWall;
    CDIB* m_pcdibBtns[NUM_BTNS];
    CDIB* m_pcdibTmp;
    int   m_iWid;
    int   m_iHt;
    BOOL  m_bTile;
};


/////////////////////////////////////////////////////////////////////////////
// CDlgVer dialog

class CDlgVer : public CDialog
{
    // Construction
  public:
    CDlgVer( CWnd* pParent = NULL );  // standard constructor

                                      // Dialog Data
    //{{AFX_DATA(CDlgVer)
    enum
    {
        IDD = IDD_VERSION
    };
    CString m_sVer;
    CString m_sOs;
    CString m_sNet;
    CString m_sThunk;
    CString m_sRif;
    CString m_sVideo;
    CString m_sSound;
    CString m_sSoundVer;
    CString m_sSpeed;
    CString m_sMemory;
    //}}AFX_DATA

    // Implementation
  protected:
    virtual void DoDataExchange( CDataExchange* pDX );  // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDlgVer)
    virtual BOOL OnInitDialog( );
    afx_msg void OnLicense( );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP( )
};

//---------------------------- C Z o o m D a t a ----------------------------
//
// Determine and store lowest zoom level & # of zoom levels in the sprites

class CZoomData
{

  public:
    CZoomData( int nDataZooms );

    int GetFirstZoom( ) const { return m_iFirstZoom; }     // Start zoom level (0 or 1)
    int GetNumDataZooms( ) const { return m_nDataZooms; }  // # zoom levels in enations.dat sprites

  private:
    enum
    {
        MIN_ZOOM_ZERO_MEGS = 24,
        AUTO_SELECT_ZOOM   = NUM_ZOOM_LEVELS
    };

    int m_iFirstZoom;  // Lowest supported zoom level
    int m_nDataZooms;  // # of zooms contained in the sprites
};

/////////////////////////////////////////////////////////////////////////////
// CConquerApp:
// See temp.cpp for the implementation of this class
//

typedef void( FNYIELDFUNC )( );

class CConquerApp : public CWinApp
{
  public:
    CConquerApp( );
    ~CConquerApp( );

    void CreateMain( );
    void DisableMain( );
    void DestroyMain( );
    void DestroyExceptMain( );
    void ReadyToJoin( );
    void ReadyToCreate( );
    void StartCreateWorld( );
    void CreateNewWorld( unsigned uRand, AIinit* pAiData, int iSide, int iSideSize );
    void StartAi( );
    void LetsGo( );
    void CloseWorld( );
    void DestroyWorld( );
    void ClearWorld( );    // clean out existing game to load new game
    void NewWorld( );      // read in sprites etc, before load file
    void RestartWorld( );  // get it going again

    void ShowPlayerList( );

    BOOL LoadData( );

#ifdef TESTINGGON
    void RunTests( );
#endif

#ifdef BUGBUG
    void LoadVer( );        // load data from the RIF files
    void LoadMain( );       // main screen
    void LoadSfx( );        // for game
    void LoadMidi( );       // for game
    void LoadWav( );        // for game
    void LoadSprites( );    // for game
    void LoadOther( );      // for game
    void LoadLang( );       // language specific part for game
    void UnloadMain( );     // main screen
    void UnloadSfx( );      // for game
    void UnloadMidi( );     // for game
    void UnloadWav( );      // for game
    void UnloadSprites( );  // for game
    void UnloadOther( );    // for game
    void UnloadLang( );     // language specific part for game
#endif

    

    CZoomData* GetZoomData( ) const
    {
        ASSERT( m_ptrzoomdata.Value( ) );
        return m_ptrzoomdata.Value( );
    }

    BOOL SaveGame( CWnd* pPar );  // saves game

    void ScenarioStart( );        // set up a scenario
    BOOL ScenarioEnd( );          // is the scenario over?

    void Minimize( );
    void CloseApp( );
    BOOL AmInGame( ) { return ( m_bInGame ); }

    void PostIntro( );  // called when movie is over
    void InitCustomUI( );

    int    TextHt( ) const { return ( m_iCharHt ); }
    int    TextWid( ) const { return ( m_iCharWid ); }
    CFont& TextFont( ) { return ( m_Fnt ); }
    CFont& RDFont( ) { return ( m_FntRD ); }      // R&D desc
    CFont& DescFont( ) { return ( m_FntDesc ); }  // bldg/veh desc
    CFont& CostFont( ) { return ( m_FntCost ); }  // bldg/veh cost win

                                                  // BUGBUG	int		ListHt () const				{return (m_iUnitListHt);}

    int                      GetFirstZoom( ) const { return m_iFirstZoom; }     // Start zoom level (0 or 1)
    int                      GetNumDataZooms( ) const { return m_nDataZooms; }  // # zoom levels in enations.dat sprites
    BOOL                     UseZoom0( ) const { return m_bUseZoom0; }
    BOOL                     Have24Bit( ) const { return m_bHave24; }
    BOOL                     Use8Bit( ) const { return m_bUse8Bit; }
    BOOL                     IsShareware( ) const { return m_bShareware; }
    BOOL                     IsSecondDisk( ) const { return m_bSecondDisk; }
    BOOL                     HaveWAV( ) const { return m_bWAV; }
    int                      GetRifVer( ) const { return m_iRifVer; }
    BOOL                     RequireCD( ) const { return m_iRequireCD; }
    BOOL                     HaveMultVoices( ) const { return m_iMultVoices; }
    BOOL                     HaveIntro( ) const { return m_iHaveIntro; }
    CMusicPlayer::MUSIC_MODE GetMode( ) const { return m_mMode; }
    int                      GetCpuSpeed( ) const { return m_iCpuSpeed; }
    int                      GetCdSpeed( ) const { return m_iCdSpeed; }


    void Log( char const* pText );

    // Overrides
    BOOL InitInstance( );
    int  ExitInstance( );
    int  Run( );
    BOOL BaseYield( );
    BOOL FullYield( );

    void GraphicsEnginePump( );
    void ProcessAllMessages( );
    void RenderScreens( );
    void _RenderScreens( );

    BOOL YieldAndRenderEvent( );
    BOOL YieldAndRenderNoEvent( );
    BOOL CheckYield( );
    BOOL m_bUseEvents;

    HINSTANCE m_hLibLang;     // localized resources
    int       m_iLangCode;    // language
    int*      m_piLangAvail;  // languages available
    int       m_iNumLang;

    void SetLanguage( int iCode );
    int  GetLanguage( ) const;

    CDlgChatAll* GetDlgChat( );
    void         CloseDlgChat( );

    //{{AFX_MSG(CConquerApp)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP( )


  public:
    CString m_sOs;
    CString m_sNet;
    CString m_sRif;
    CString m_sVideo;
    CString m_sSound;
    CString m_sSoundVer;
    CString m_sSpeed;
    int     m_iRestoreRes;
    int     m_iOldWidth;
    int     m_iOldHeight;
    int     m_iOldDepth;

    CString m_sAppName;  // the name of the application
    CString m_sClsName;  // main window (for finding 1st instance)
    CString m_sWndCls;   // CWnd class for pop-up windows

    CString m_sResIni;   // the [1024x768] for the ini file
    int     m_iScrnX;    // screen width
    int     m_iScrnY;    // screen height
    int     m_iCol1;     // default positions for windows (left of area)
    int     m_iCol2;     // right of area
    int     m_iRow1;     // top of chat
    int     m_iRow2;     // top of advisor
    int     m_iRow3;     // top of button bar
    int     m_iRow4;     // top of drop-down (on right)

    BOOL    m_bPauseOnAct;
    BOOL    m_bSetSysColors;
    BOOL    m_bSubClass;
    CString m_sCdFile;             // file to check for to insure CD in

    char         m_szMapBPS[4];    // either "08" or "24"
    char         m_szOtherBPS[4];  // either "08" or "24"
    CColorFormat m_MapClrFmt;
    CColorFormat m_OtherClrFmt;

    CFile* m_pLogFile;  // for tracking loading

    int FlatDimen( ) const { return ( m_iBtnBevel ); }
    int BevelDimen( ) const { return ( m_iBtnBevel ); }

    // windows used while playing
    CWndWorld         m_wndWorld;     // the world map window
    CWndComm          m_wndChat;      // the chat & e-mail window
    CWndBar           m_wndBar;       // the button bar window
    CWndListBuildings m_wndBldgs;     // the buildings listbox
    CWndListVehicles  m_wndVehicles;  // the vehicles listbox
    CDlgRelations*    m_pdlgRelations;
    CDlgFile*         m_pdlgFile;
    CDlgResearch*     m_pdlgRsrch;

    CWndMain      m_wndMain;       // the main window - first created, last destroyed
    CDlgMain*     m_pdlgMain;      // the main menu
    CCreateBase*  m_pCreateGame;   // holder for creating game data & dialogs
    CWndMovie     m_wndMovie;
    CWndCredits   m_wndCredits;    // playing credits
    CDlgPlyrList* m_pdlgPlyrList;  // for server to kill players
    CWndCutScene  m_wndCutScene;
    CDlgChatAll*  m_pdlgChat;

    CDlgPause* GetDlgPause( );

    HACCEL GetAccel( ) const { return m_hAccel; }

  protected:
    CDlgPause* m_pdlgPause;  // when game paused

    int   m_iCharHt;         // standard font we use
    int   m_iCharWid;
    int   m_iBtnBevel;
    CFont m_Fnt;                         // font used for everything else
    CFont m_FntRD;                       // cost font (descriptions in dialogs)
    CFont m_FntDesc;                     // cost font (descriptions in dialogs)
    CFont m_FntCost;                     // cost font (descriptions in dialogs)

    BOOL                     m_bInGame;  // we're in a game
    BOOL                     m_bTimeLimit;
    CMusicPlayer::MUSIC_MODE m_mMode;    // music mode

    HACCEL m_hAccel;                     // global accelerators

    DWORD m_dwNextRender;                // time to next 24th of a second
    DWORD m_dwMaxNextRender;             // time to next 24th + 1/12 (max we wait)

    Ptr<CZoomData> m_ptrzoomdata;

    int  m_iFirstZoom;   // Lowest supported zoom level
    int  m_nDataZooms;   // # of zooms contained in the sprites
    int  m_bHave24;      // TRUE if have 24-bit art in RIF
    int  m_bShareware;   // TRUE if shareware version
    int  m_bSecondDisk;  // TRUE if second (friend) disk
    int  m_bWAV;         // TRUE if have digital audio (always have MIDI)
    int  m_iRifVer;      // RIF version number
    int  m_iRequireCD;   // must have CD
    int  m_iMultVoices;  // has multiple voices
    int  m_iHaveIntro;   // has intro
    BOOL m_bUse8Bit;     // must use 8-bit art
    BOOL m_bUseZoom0;    // load zoom 0
    int  m_iCpuSpeed;    // a P/133 should return 133
    int  m_iCdSpeed;     // a 4X CD should return 4
    MSG  m_msgCur;       // current window message?

    BOOL PreTranslateMessage( MSG* pMsg );


#ifdef _DEBUG
  public:
    virtual void AssertValid( ) const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CDlgStackDump dialog

class CDlgStackDump : public CDialog
{
    // Construction
  public:
    CDlgStackDump( CWnd* pParent = NULL );  // standard constructor

                                            // Dialog Data
    SE_Exception* m_pe;

    //{{AFX_DATA(CDlgStackDump)
    enum
    {
        IDD = IDD_GPF_DATA
    };
    CString m_sText;
    //}}AFX_DATA


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDlgStackDump)
  protected:
    virtual void DoDataExchange( CDataExchange* pDX );  // DDX/DDV support
                                                        //}}AFX_VIRTUAL

                                                        // Implementation
  protected:
    // Generated message map functions
    //{{AFX_MSG(CDlgStackDump)
    afx_msg void OnCopyStack( );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP( )
};

extern CConquerApp theApp;

#endif
