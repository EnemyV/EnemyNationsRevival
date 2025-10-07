//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "CdLoc.h"
#include "DlgReg.h"
#include "ai.h"
#include "area.h"
#include "bitmaps.h"
#include "bmbutton.h"
#include "chat.h"
#include "creatmul.inl"
#include "creatsin.h"
//#include "dlgflic.h"
#include "error.h"
#include "join.h"
#include "license.h"
#include "options.h"
#include "racedata.h"
#include "scenario.h"
#include "sfx.h"
#include "sprite.h"
#include "stdafx.h"
#include "terrain.inl"
#include "tstsnds.h"

#include <processenv.h>

#include <ctype.h>
#include <locale.h>
#include <new.h>
#include <windward.h>

#ifndef __INCLUDE_THIS_LAST__
#include "lastplnt.h"
#endif


#ifdef _DEBUG

#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW




extern HANDLE hRenderEvent;
extern BOOL   bDoSubclass;


CString          GetDefaultApp( char const* pExt, char const* pDef, char const* pCmdLine );
LRESULT CALLBACK PerBarProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


static int aiBtns[] = { IDC_MAIN_CAMPAIGN, IDC_MAIN_SINGLE,   IDC_MAIN_CREATE, IDC_MAIN_JOIN,
                        IDC_MAIN_LOAD,     IDC_MAIN_LOAD_MUL, IDC_MAIN_INTRO,  IDC_MAIN_CREDITS,
                        IDC_MAIN_OPTIONS,  IDCANCEL,          IDC_MINIMIZE };
const int  NUM_BTNS = sizeof( aiBtns ) / sizeof( int );


#ifdef _CHEAT
BOOL _bShowRate       = TRUE;
BOOL _bClickAny       = FALSE;
BOOL _bMaxMaterials   = FALSE;
BOOL _bMaxRocket      = FALSE;
BOOL _bMaxPower       = FALSE;
BOOL _iFrameRate      = 1;
BOOL _bShowWorld      = TRUE;
BOOL _bShowStatus     = FALSE;
BOOL _bSeeAll         = FALSE;
BOOL _bShowPos        = FALSE;
BOOL _bShowAISelected = FALSE;
int  _iScenarioOn     = -1;
#endif

static char sCopyright[] = "Copyright (c) 1995 - 1997. Windward Studios, Inc. All Rights Reserved.";

extern COLORREF GetOurSysClr( int iInd );


/////////////////////////////////////////////////////////////////////////////
// This garbage is so we can get the exception address from the structured
// exception handler into our catch

TRANS_FUNC prevFn = NULL;

void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{

    SE_Exception exp( u, pExp->ExceptionRecord->ExceptionAddress );

    // get the call stack
    memset( exp.m_stack, 0, sizeof( exp.m_stack ) );

    // no clean way to do this so we walk the stack looking for pointers to code
    DWORD* pCall = (DWORD*)pExp->ContextRecord->Esp;
    for ( int iInd = 0; iInd < NUM_EXCEP; pCall++ )
    {
        // if the pointer is bad - we're done (we don't write but the stack is writeable)
        if ( IsBadWritePtr( pCall, 4 ) )
            break;

        // if it's code we save it (write because badcode only checks if can read)
        if ( ( IsBadWritePtr( (void*)*pCall, 4 ) ) && ( !IsBadCodePtr( (FARPROC)*pCall ) ) )
            if ( ( iInd < 3 ) || ( ( *pCall & 0x80000000 ) == 0 ) )
                exp.m_stack[iInd++] = *pCall;
    }

    throw exp;
}

void CatchNum( int iNum )
{

    bDoSubclass = FALSE;

    // turn the game off
    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);
    theGame.SetShouldProcessMessages(FALSE);
    theGame.EmptyQueue( );

    // no message if quitting
    if ( iNum == ERR_TLP_QUIT )
        return;

    CString sMsg;
    sMsg.LoadString( IDS_ERR_LOAD_1 );
    char sNum[20];
    if ( iNum >= ERR_BASE_USER_ERROR )
        iNum -= ERR_BASE_USER_ERROR;
    else
        iNum += 100;
    itoa( iNum, sNum, 10 );
    csPrintf( &sMsg, (char const*)sNum );
    AfxMessageBox( sMsg, MB_OK | MB_ICONSTOP );

    bDoSubclass = TRUE;
}

void CatchSE( SE_Exception e )
{

    bDoSubclass = FALSE;

    // turn the game off
    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);
    theGame.SetShouldProcessMessages(FALSE);
    theGame.EmptyQueue( );

    CDlgStackDump dlg;
    dlg.m_pe = &e;
    dlg.m_sText.LoadString( IDS_ERR_LOAD_3 );

    MEMORYSTATUS ms;
    ms.dwLength = sizeof( ms );
    GlobalMemoryStatus( &ms );
    const int ONE_MEG = 1024 * 1024;
    if ( ms.dwAvailPageFile / ONE_MEG < 8 )
    {
        CString sMsg;
        sMsg.LoadString( IDS_OUT_OF_MEMORY );
        dlg.m_sText = sMsg + "\r\n" + dlg.m_sText;
    }

    char sNum1[20], sNum2[80], sNumS[5][20];
    itoa( e.m_uEc, sNum1, 16 );
    switch ( (uint64_t)e.m_pExCode )
    {
    case STATUS_ACCESS_VIOLATION:
        strcpy( sNum2, "Access Violation" );
        break;
    case STATUS_IN_PAGE_ERROR:
        strcpy( sNum2, "Page Error" );
        break;
    case STATUS_FLOAT_INVALID_OPERATION:
        strcpy( sNum2, "FPU Invalid Op" );
        break;
    case STATUS_STACK_OVERFLOW:
        strcpy( sNum2, "Stack Overflow" );
        break;
    default:
        itoa( (int)e.m_pExCode, sNum2, 16 );
        break;
    }
    for ( int iOn = 0; iOn < 5; iOn++ ) itoa( e.m_stack[iOn], sNumS[iOn], 16 );
    csPrintf( &dlg.m_sText, (char const*)VER_STRING, (char const*)sNum1, (char const*)sNum2, (char const*)sNumS[0],
              (char const*)sNumS[1], (char const*)sNumS[2], (char const*)sNumS[3], (char const*)sNumS[4] );
    try
    {
        dlg.DoModal( );
    }
    catch ( ... )
    {
        AfxMessageBox( dlg.m_sText, MB_OK | MB_ICONSTOP );
    }

    bDoSubclass = TRUE;
}

void CatchOther( )
{

    bDoSubclass = FALSE;

    // turn the game off
    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);
    theGame.SetShouldProcessMessages(FALSE);
    theGame.EmptyQueue( );

    CString sMsg;
    sMsg.LoadString( IDS_ERR_LOAD_2 );
    AfxMessageBox( sMsg, MB_OK | MB_ICONSTOP );

    bDoSubclass = TRUE;
}


LRESULT CALLBACK RedTextProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

    switch ( uMsg )
    {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        ::BeginPaint( hWnd, &ps );
        RECT rect;
        char sText[20];
        ::GetClientRect( hWnd, &rect );
        ::GetWindowText( hWnd, sText, 19 );
        ::SetBkColor( ps.hdc, RGB( 192, 192, 192 ) );
        if ( sText[0] == '-' )
            ::SetTextColor( ps.hdc, RGB( 255, 0, 0 ) );
        else
            ::SetTextColor( ps.hdc, RGB( 0, 0, 0 ) );
        ::SetTextAlign( ps.hdc, TA_RIGHT );
        ::ExtTextOut( ps.hdc, rect.right, 0, ETO_CLIPPED | ETO_OPAQUE, &rect, sText, strlen( sText ), NULL );
        ::EndPaint( hWnd, &ps );
        return ( 0 );
    }

    case WM_SETTEXT: {
        LPCSTR pStr = (LPCSTR)lParam;
        if ( pStr != NULL )
        {
            HDC  hdc = ::GetDC( hWnd );
            RECT rect;
            ::GetClientRect( hWnd, &rect );
            ::SetBkColor( hdc, RGB( 192, 192, 192 ) );
            if ( *pStr == '-' )
                ::SetTextColor( hdc, RGB( 255, 0, 0 ) );
            else
                ::SetTextColor( hdc, RGB( 0, 0, 0 ) );
            ::SetTextAlign( hdc, TA_RIGHT );
            ::ExtTextOut( hdc, rect.right, 0, ETO_CLIPPED | ETO_OPAQUE, &rect, pStr, strlen( pStr ), NULL );
            ::ReleaseDC( hWnd, hdc );
        }

        return ( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
    }
    }

    return ( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
}

/////////////////////////////////////////////////////////////////////////////
// CConquerApp

BEGIN_MESSAGE_MAP( CConquerApp, CWinApp )
//{{AFX_MSG_MAP(CConquerApp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CConquerApp construction

CConquerApp::CConquerApp( ): m_MapClrFmt( CColorFormat::DEPTH_EIGHT ), 
                m_OtherClrFmt( CColorFormat::DEPTH_EIGHT )
{

    m_bInGame       = FALSE;
    m_pdlgRelations = NULL;
    m_pdlgFile      = NULL;
    m_pdlgRsrch     = NULL;
    m_hAccel        = NULL;
    m_hLibLang      = NULL;
    m_iLangCode     = 0;
    m_piLangAvail   = NULL;
    m_iNumLang      = 0;
    m_bSetSysColors = FALSE;

    m_bSubClass   = FALSE;
    m_iRequireCD  = FALSE;
    m_iMultVoices = TRUE;
    m_iHaveIntro  = TRUE;
    m_pdlgPause   = NULL;
    m_pdlgChat    = NULL;

    m_pLogFile = NULL;

    m_dwNextRender = m_dwMaxNextRender = 0;

    // 0 == NoEvent
    // 1 == Event
    // 2 == Just Say No
    m_bUseEvents = W32s != iWinType;

    m_iRestoreRes = FALSE;
    m_iOldWidth   = 640;
    m_iOldHeight  = 480;
    m_iOldDepth   = 8;
}

CConquerApp::~CConquerApp( )
{

    CRaceDef::Close( ptheRaces );
    ptheRaces = NULL;

    CGlobalSubClass::UnSubClass( );
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CConquerApp object

CConquerApp theApp;


int _excep_new_handler( size_t )
{

    AfxMessageBox( IDS_NO_MEMORY, MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP );
    ::PostQuitMessage( 0 );
    throw;
    return ( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CConquerApp initialization

const int MEM_NEEDED_BASE          = 60;
const int MEM_NEEDED_8_BIT_ZOOM_0  = 150;
const int MEM_NEEDED_MUSIC_MIXED   = 70;
const int MEM_NEEDED_MUSIC_DIGITAL = 80;
const int MEM_PHYS_NEEDED_16_BIT   = 16;
const int MEM_NEEDED_16_BIT        = 130;
const int MEM_NEEDED_16_BIT_ZOOM_0 = 300;
const int MEM_PHYS_NEEDED_24_BIT   = 24;
const int MEM_NEEDED_24_BIT        = 180;
const int MEM_NEEDED_24_BIT_ZOOM_0 = 450;
const int MEM_PHYS_NEEDED_32_BIT   = 32;
const int MEM_NEEDED_32_BIT        = 240;
const int MEM_NEEDED_32_BIT_ZOOM_0 = 600;


void CConquerApp::Log( char const* pText )
{
#ifdef LOGGINGON
    OutputDebugStringA( pText);
    OutputDebugStringA( "\n" );
#endif
    if ( m_pLogFile == NULL )
        return;

    // elim any existing \n
    int         iLen = strlen( pText );
    char const* pEnd = pText + iLen - 1;
    while ( ( iLen >= 1 ) && ( ( *pEnd == '\n' ) || ( *pEnd == '\r' ) ) )
    {
        pEnd--;
        iLen--;
    }

    m_pLogFile->Write( pText, iLen );
    m_pLogFile->Write( "\r\n", 2 );

    m_pLogFile->Flush( );
}

BOOL CConquerApp::InitInstance( )
{
#ifdef LOGGINGON
    OutputDebugStringA( "InitInstance\n" );
#endif

    InitWindwardLib1( this );

    WriteProfileString( "ADPCM", "Error", "OK" );

    if ( GetProfileInt( "Advanced", "Log", 0 ) )
    {
        AfxMessageBox( IDS_EN_LOGGING, MB_OK | MB_ICONINFORMATION );
        m_pLogFile    = new CFile( );
        CString sName = GetProfileString( "Advanced", "LogName", GameLogFile );
        if ( m_pLogFile->Open( sName, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite ) == 0 )
        {
            m_pLogFile->Close( );
            delete m_pLogFile;
            m_pLogFile = NULL;
        }
        else
        {
            time_t t;
            time( &t );
            struct tm* _now = localtime( &t );
            Log( asctime( _now ) );
            Log( VER_STRING );
        }
    }
    else if ( ::GetPrivateProfileInt( "vdmplay", "UseLogFile", 0, "vdmplay.ini" ) )
        AfxMessageBox( IDS_VP_LOGGING, MB_OK | MB_ICONINFORMATION );

    WriteProfileString( "Advanced", "Version", VER_STRING );

    // over-ride default event method
    m_bUseEvents  = GetProfileInt( "Advanced", "Events", m_bUseEvents );
    m_bPauseOnAct = GetProfileInt( "Advanced", "Pause", TRUE );

    // load the correct language
    m_iLangCode  = GetProfileInt( "Advanced", "Language", PRIMARYLANGID( LANGIDFROMLCID( ::GetUserDefaultLCID( ) ) ) );
    CString sLib = "ENLang" + IntToCString( m_iLangCode ) + ".DLL";
    if ( ( m_hLibLang = LoadLibrary( sLib ) ) != NULL )
        AfxSetResourceHandle( m_hLibLang );

    // init critical section (before maybe exiting below)
    memset( &cs, 0, sizeof( cs ) );
    InitializeCriticalSection( &cs );
    hRenderEvent = CreateEvent( NULL, TRUE, FALSE, "RenderEvent" );

    m_sAppName.LoadString( IDS_MAIN_TITLE );

    // get the CPU speed (needed before screen res)
    if ( ( m_iCpuSpeed = GetProfileInt( "Advanced", "CPUspeed", 0 ) ) < 60 )
    {
        BOOL        bGotSpeed = FALSE;
        SYSTEM_INFO si;
        ::GetSystemInfo( &si );

        if ( ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) && ( si.wProcessorLevel >= 5 ) )
        {
            // in case processor can't handle rdtsc
            try
            {
                static int dwStart, dwHigh, dwEnd;
                m_iCpuSpeed = INT_MAX;
                for ( int iTry = 0; iTry < 4; iTry++ )
                {
                    ::Sleep( 10 );

                    // get the clock count
                    __asm
                        {
                        _emit 0fh
                        _emit 31h  // rdtsc
                        mov [dwStart], eax
                        mov [dwHigh], edx
                        }

                    // wait 250 ms
                    DWORD dwTime = timeGetTime( ) + 250;
                    while ( timeGetTime( ) < dwTime )
                        ;

                    __asm
                    {
                        _emit 0fh
                        _emit 31h  // rdtsc
                        mov [dwEnd], eax
                        sub [dwHigh], edx
                    }

                    if ( dwHigh == 0 )
                    {
                        int iTime   = dwEnd - dwStart;
                        m_iCpuSpeed = __min( iTime, m_iCpuSpeed );
                    }
                }
                m_iCpuSpeed = ( ( m_iCpuSpeed >> 12 ) * 133 + ( 0x1FAD / 2 ) ) / 0x1FAD;
                bGotSpeed   = TRUE;
            }

            catch ( ... )
            {
            }
        }

        if ( !bGotSpeed )
        {
            m_iCpuSpeed = INT_MAX;
            int iPri    = GetThreadPriority( );
            ::Sleep( 100 );

            for ( int iTry = 0; iTry < 16; iTry++ )
            {
                SetThreadPriority( THREAD_PRIORITY_HIGHEST );
                ::Sleep( 10 );
                DWORD dwStart = timeGetTime( );
                _asm {
                    push	ebx
                    mov	ecx, 100000h
                    jmp	_flush2
_flush2:
                    mov eax, 01234h
                    mov edx, 10h
                    mov	ebx, 100
                    div ebx
                    div	ebx
                    loop	_flush2
                    pop	ebx
                }
                DWORD dwTime = timeGetTime( ) - dwStart;
                SetThreadPriority( iPri );
                ::Sleep( 10 );
                m_iCpuSpeed = __min( m_iCpuSpeed, (int)dwTime );
            }

            m_iCpuSpeed = __max( 1, m_iCpuSpeed );
            m_iCpuSpeed = ( 133 * 703 + 351 ) / m_iCpuSpeed;
        }
    }
    m_iCpuSpeed = __max( 60, m_iCpuSpeed );

    // if we can't switch to 640x480 then we punt on all of this
    if ( iWinType != W32s )
    {
#ifdef LOGGINGON
        OutputDebugStringA( "iWinType != W32s" );
#endif
        DEVMODE dev;
        memset( &dev, 0, sizeof( dev ) );
        dev.dmPelsWidth  = 640;
        dev.dmPelsHeight = 480;
        dev.dmSize       = sizeof( dev );
        dev.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        dev.dmBitsPerPel = 8;

        LONG lRtn = ChangeDisplaySettings( &dev, CDS_TEST );
        char sBuf[80];
        sprintf( sBuf, "ChangeDisplaySettings (test) = %d", lRtn );
        Log( sBuf );
        if ( lRtn == DISP_CHANGE_SUCCESSFUL )
        {
            // do we set the screen resolution?
            int iRes     = GetProfileInt( "Advanced", "ScreenResolution", 0 );
            m_iOldWidth  = GetSystemMetrics( SM_CXSCREEN );
            m_iOldHeight = GetSystemMetrics( SM_CYSCREEN );
            HDC hdc      = GetDC( NULL );
            m_iOldDepth  = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
            ReleaseDC( NULL, hdc );

            // get best res
            dev.dmPelsWidth  = 640;
            dev.dmPelsHeight = 480;
            int   iBest      = 2;
            char* pRes       = "640x480x8";
            if ( m_iCpuSpeed > 100 )  // 800x600
            {
                iBest            = 3;
                pRes             = "800x600x8";
                dev.dmPelsWidth  = 800;
                dev.dmPelsHeight = 600;
                if ( m_iCpuSpeed > 130 )  // 1024x768
                {
                    iBest            = 4;
                    pRes             = "1024x768x8";
                    dev.dmPelsWidth  = 1024;
                    dev.dmPelsHeight = 768;
                    if ( m_iCpuSpeed > 160 )  // 1280x1024
                        iBest = 5;
                }
            }

            sprintf( sBuf, "ChangeDisplaySettings ScreenResolution=%d, Best=%d", iRes, iBest );
            Log( sBuf );

            // if we are not on use native OR native is ok - no message
            BOOL bNativeOk =
                ( m_iOldWidth * m_iOldHeight * ( ( m_iOldDepth + 7 ) / 8 ) ) / m_iCpuSpeed <= ( 640 * 480 ) / 60;
            if ( ( iBest < 5 ) && ( iRes == 0 ) && !bNativeOk )
            {
                CDlgMsg dlg;
                CString sMsg;
                sMsg.LoadString( IDS_KILLER_RES );
                CString sRes = IntToCString( m_iOldWidth ) + "x" + IntToCString( m_iOldHeight ) + "x" +
                               IntToCString( m_iOldDepth );
                csPrintf( &sMsg, (char const*)sRes, pRes );
                if ( dlg.MsgBox( sMsg, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "ScreenRes", IDNO ) == IDYES )
                    iRes = iBest;
            }
            else if ( iRes == 1 )
                iRes = iBest;

            // ok - we change if iRes is 2 - 4
            if ( ( 2 <= iRes ) && ( iRes <= 4 ) )
            {
                switch ( iRes )
                {
                case 3:
                    dev.dmPelsWidth  = 800;
                    dev.dmPelsHeight = 600;
                    break;
                case 4:
                    dev.dmPelsWidth  = 1024;
                    dev.dmPelsHeight = 768;
                    break;
                default:
                    dev.dmPelsWidth  = 640;
                    dev.dmPelsHeight = 480;
                    break;
                }

                m_iRestoreRes    = TRUE;
                dev.dmSize       = sizeof( dev );
                dev.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
                dev.dmBitsPerPel = 8;

                LONG lRtn = ChangeDisplaySettings( &dev, 0 );
                sprintf( sBuf, "ChangeDisplaySettings (%dx%dx%d) = %d", dev.dmPelsWidth, dev.dmPelsHeight,
                         dev.dmBitsPerPel, lRtn );
                Log( sBuf );
            }
        }
    }

    m_sClsName = "EnemyNationsMainWindow";
    WNDCLASS wc;
    memset( &wc, 0, sizeof( wc ) );
    wc.lpfnWndProc   = ::DefWindowProc;
    wc.hInstance     = AfxGetInstanceHandle( );
    wc.hIcon         = LoadIcon( IDI_MAIN );
    wc.hCursor       = LoadStandardCursor( IDC_ARROW );
    wc.lpszClassName = m_sClsName;
    if ( !AfxRegisterClass( &wc ) ) {
        return FALSE;
    }

    HWND hPrevWnd = ::FindWindow( m_sClsName, m_sAppName );
    if ( hPrevWnd != NULL )
        if ( AfxMessageBox( IDS_MULT_INST, MB_YESNO | MB_ICONQUESTION ) == IDYES )
        {
            ::SetForegroundWindow( hPrevWnd );
            return ( FALSE );
        }

    // set up exception handling
    ::_set_new_handler( _excep_new_handler );
    prevFn = _set_se_translator( trans_func );

    // needed for autoplay, etc.
    Log( "Create main window" );
    m_wndMain.Create( );
    m_wndMain.ShowWindow( SW_SHOW );
    m_wndMain.InvalidateRect( NULL );
    m_wndMain.UpdateWindow( );

    Log( "Initialize windward.lib" );

    // set it up
    if ( !InitWindwardLib2( ) )
        return ( FALSE );

    if ( CNetApi::GetVersion( ) < 0x01000021 )
    {
        AfxMessageBox( IDS_VDMPLAY_VER, MB_OK | MB_ICONSTOP );
        return ( FALSE );
    }
    char sTmp[12];
    strncpy( sTmp, theMusicPlayer.GetVersion( ), 10 );
    sTmp[10]      = 0;
    int   iMajVer = atoi( sTmp );
    char* pBuf    = sTmp;
    while ( isdigit( *pBuf ) ) pBuf++;
    while ( !isdigit( *pBuf ) ) pBuf++;
    int iMinVer = atoi( pBuf );
    if ( ( iMajVer < 3 ) || ( ( iMajVer == 3 ) && ( iMinVer < 6 ) ) )
    {
        AfxMessageBox( IDS_MSS_VER, MB_OK | MB_ICONSTOP );
        return ( FALSE );
    }

    // list out version
    switch ( iWinType )
    {
    case W32s:
        m_sOs = "Win32s";
        break;
    case W95:
        m_sOs = "Windows95";
        break;
    case WNT:
        m_sOs = "Win/NT";
        break;
    default:
        m_sOs = "Unknown";
        break;
    }

#ifdef LOGGINGON
    CString msg;
    msg.Format( "OS Detected: %s\n", m_sOs );
    OutputDebugStringA( msg );
#endif

    OSVERSIONINFO ovi;
    memset( &ovi, 0, sizeof( ovi ) );
    ovi.dwOSVersionInfoSize = sizeof( ovi );
    GetVersionEx( &ovi );
    m_sOs += " " + LongToCString( ovi.dwMajorVersion ) + "." + LongToCString( ovi.dwMinorVersion ) + " (";
    if ( ( ovi.dwBuildNumber & 0xFFFF0000 ) == 0 )
        m_sOs += LongToCString( ovi.dwBuildNumber ) + ")";
    else
        m_sOs += LongToCString( ovi.dwBuildNumber >> 16 ) + "," + LongToCString( ovi.dwBuildNumber & 0xFFFF ) + ")";
    if ( iWinType == W32s )
    {
        WORD wVer = LOWORD( GetVersion( ) );
        m_sOs += " [Windows " + IntToCString( LOBYTE( wVer ) ) + "." + IntToCString( HIBYTE( wVer ) ) + "]";
    }
    Log( m_sOs );

    long lVer = CNetApi::GetVersion( );
    m_sNet    = "VDMPlay API " + IntToCString( HIBYTE( HIWORD( lVer ) ) ) + "." +
             IntToCString( LOBYTE( HIWORD( lVer ) ) ) + "." + IntToCString( LOWORD( lVer ) );
    Log( m_sNet );

    MEMORYSTATUS ms;
    ms.dwLength = sizeof( ms );
    GlobalMemoryStatus( &ms );
    const int ONE_MEG   = 1024 * 1024;
    CString sMemory = "Memory (avail/total) Physical: " + IntToCString( ms.dwAvailPhys / ONE_MEG ) + "M/" +
                      IntToCString( ms.dwTotalPhys / ONE_MEG ) +
                      "M Virtual: " + IntToCString( ms.dwAvailPageFile / ONE_MEG ) + "M/" +
                      IntToCString( ms.dwTotalPageFile / ONE_MEG ) + "M";
    Log( sMemory );

    // enough memory?
    // need 8M system
    if ( ms.dwTotalPhys < 1024 * 1024 * 7 )
    {
        CDlgMsg dlg;
        if ( dlg.MsgBox( IDS_ERROR_LOW_PHYS_MEM, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThan8Meg" ) !=
             IDYES )
            return ( 0 );
        Log( "Error: Not enough physical memory to run" );
        m_wndMain.UpdateWindow( );
    }
    if ( ms.dwTotalPageFile < 1024 * 1024 * MEM_NEEDED_BASE )
    {
        CString sText;
        sText.LoadString( IDS_ERROR_LOW_VIRT_MEM );
        CString sNum;
        sNum = IntToCString( MEM_NEEDED_BASE );
        csPrintf( &sText, (char const*)sNum );
        CDlgMsg dlg;
        if ( dlg.MsgBox( sText, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThan8Meg" ) != IDYES )
            return ( 0 );
        Log( "Error: Not enough virtual memory to run" );
        m_wndMain.UpdateWindow( );
    }
    if ( ( ms.dwTotalPhys < 1024 * 1024 * 7 ) || ( ms.dwAvailPageFile < 1024 * 1024 * ( MEM_NEEDED_BASE - 10 ) ) )
    {
        CDlgMsg dlg;
        if ( dlg.MsgBox( IDS_ERROR_LOW_AVAIL_MEM, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                         "NotEnoughFreeMem" ) != IDYES )
            return ( 0 );
        m_wndMain.UpdateWindow( );
    }

    // figure out what we run at
    // first - what should we be at
    HDC hdc            = GetDC( NULL );
    int iBytesPerPixel = ( GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES ) + 7 ) / 8;
    ReleaseDC( NULL, hdc );
    BOOL bForce8 = FALSE;
    switch ( iBytesPerPixel )
    {
    case 1:
        // don't care
        break;
    case 2:
        if ( ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_16_BIT ) ||
             ( ms.dwTotalPhys < 1024 * 1024 * MEM_PHYS_NEEDED_16_BIT ) )
            bForce8 = TRUE;
        break;
    case 3:
        if ( ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_24_BIT ) ||
             ( ms.dwTotalPhys < 1024 * 1024 * MEM_PHYS_NEEDED_24_BIT ) )
            bForce8 = TRUE;
        break;
    case 4:
        if ( ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_32_BIT ) ||
             ( ms.dwTotalPhys < 1024 * 1024 * MEM_PHYS_NEEDED_32_BIT ) )
            bForce8 = TRUE;
        break;
    }

    switch ( GetProfileInt( "Advanced", "ColorDepth", 1 ) )
    {
    case 2:
        m_bUse8Bit = FALSE;
        if ( iBytesPerPixel == 1 )
            break;
        if ( bForce8 )
        {
            CDlgMsg dlg;
            if ( dlg.MsgBox( IDS_ERROR_LOW_COLOR_DEPTH, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                             "LessThanColorDepth" ) == IDYES )
                m_bUse8Bit = TRUE;
            m_wndMain.UpdateWindow( );
        }
        else if ( m_iCpuSpeed <= 200 )
        {
            CDlgMsg dlg;
            if ( dlg.MsgBox( IDS_ERROR_LOW_COLOR_DEPTH2, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                             "LessThanColorDepth" ) == IDYES )
                m_bUse8Bit = TRUE;
            m_wndMain.UpdateWindow( );
        }
        break;

    case 0:
        // if a P/200 or less stay at 8-bit
        if ( m_iCpuSpeed <= 200 )
            m_bUse8Bit = TRUE;
        else
            m_bUse8Bit = bForce8;
        break;

    default:
        m_bUse8Bit = TRUE;
        break;
    }

    // now figure the zoom level
    if ( m_bUse8Bit )
        iBytesPerPixel = 1;
    BOOL bUse0 = TRUE;
    switch ( iBytesPerPixel )
    {
    case 1:
        if ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_8_BIT_ZOOM_0 )
            bUse0 = FALSE;
        break;
    case 2:
        if ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_16_BIT_ZOOM_0 )
            bUse0 = FALSE;
        break;
    case 3:
        if ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_24_BIT_ZOOM_0 )
            bUse0 = FALSE;
        break;
    case 4:
        if ( ms.dwAvailPageFile < 1024 * 1024 * MEM_NEEDED_32_BIT_ZOOM_0 )
            bUse0 = FALSE;
        break;
    }

    switch ( theApp.GetProfileInt( "Advanced", "Zoom", 2 ) )
    {
    case 1:
        m_bUseZoom0 = TRUE;
        if ( !bUse0 )
        {
            CDlgMsg dlg;
            if ( dlg.MsgBox( IDS_ERROR_LOW_ZOOM, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThanZoom" ) ==
                 IDYES )
                m_bUseZoom0 = FALSE;
            m_wndMain.UpdateWindow( );
        }
        else if ( m_iCpuSpeed <= 200 )
        {
            CDlgMsg dlg;
            if ( dlg.MsgBox( IDS_ERROR_LOW_ZOOM2, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThanZoom" ) ==
                 IDYES )
                m_bUseZoom0 = FALSE;
            m_wndMain.UpdateWindow( );
        }
        break;

    case 0:
        // if a P/200 or less stay at no zoom 0
        if ( m_iCpuSpeed <= 200 )
            m_bUseZoom0 = FALSE;
        else
            m_bUseZoom0 = bUse0;
        break;

    default:
        m_bUseZoom0 = FALSE;
        break;
    }

    // init the random number generator
    MySrand( timeGetTime( ) );

    Log( "Load .dat file" );

    // determine the data file
    BOOL bErr = FALSE;
    do
    {
        if ( !theDataFile.Init( GameDataFile, VER_RIFF, bErr ) )
            return ( 0 );
        m_wndMain.UpdateWindow( );
        theDataFile.SetCountryCode( m_iLangCode );

#ifdef _DEBUG
        theDataFile.EnableNegativeSeekChecking( );
#endif

        // get the RIF version, etc.
        CMmio* pMmio = theDataFile.OpenAsMMIO( "version", "VERN" );
        pMmio->DescendRiff( 'V', 'E', 'R', 'N' );
        pMmio->DescendList( 'V', 'E', 'R', 'N' );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
        m_iRifVer     = pMmio->ReadShort( );
        m_iFirstZoom  = pMmio->ReadShort( );
        m_nDataZooms  = pMmio->ReadShort( );
        m_bHave24     = pMmio->ReadShort( );
        m_bShareware  = pMmio->ReadShort( );
        m_bSecondDisk = pMmio->ReadShort( );
        m_bWAV        = pMmio->ReadShort( );
        m_iRequireCD  = FALSE; // pMmio->ReadShort( ); // no longer needs CD!
        m_iMultVoices = pMmio->ReadShort( );
        m_iHaveIntro  = pMmio->ReadShort( );

        // international versions
        m_iNumLang    = pMmio->ReadShort( );
        m_piLangAvail = new int[m_iNumLang];
        for ( int iOn = 0; iOn < m_iNumLang; iOn++ ) m_piLangAvail[iOn] = pMmio->ReadShort( );

        // are we forced?
        if ( !m_bHave24 )
            m_bUse8Bit = TRUE;
        if ( m_iFirstZoom != 0 )
            m_bUseZoom0 = FALSE;

#ifdef _GG
        m_bShareware = FALSE;
        if ( m_bHave24 )
            m_bUse8Bit = FALSE;  // GGTESTING
#endif

        // check the version
        CString sName;
        pMmio->AscendChunk( );
        pMmio->DescendChunk( 'N', 'A', 'M', 'E' );
        pMmio->ReadString( sName );

        delete pMmio;

        bErr = FALSE;
        if ( ( m_iRifVer != VER_RIFF ) || ( sName != GameDataName ) )
        {
            TRAP( );
            CString sMsg, sNum1, sNum2;
            sMsg.LoadString( IDS_WRONG_DATA_FILE );
            sNum1 = IntToCString( m_iRifVer );
            sNum2 = IntToCString( VER_RIFF );
            csPrintf( &sMsg, (char const*)sName, (char const*)sNum1, (char const*)sNum2 );
            if ( AfxMessageBox( sMsg, MB_YESNO | MB_ICONSTOP ) != IDYES )
                return ( 0 );
            bErr = TRUE;
        }
    } while ( bErr );

    // if .dat < 400M then it's shareware (anti-pirate)
    CFileStatus fs;
    CFile::GetStatus( theDataFile.GetName( ), fs );
    if ( fs.m_size < 400000000 )
        m_bShareware = TRUE;

    // warn on 16-bit
    if ( !m_bUse8Bit )
    {
        HDC hdc    = GetDC( NULL );
        int iDepth = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
        ReleaseDC( NULL, hdc );
        if ( iDepth == 16 )
        {
            CDlgMsg dlg;
            dlg.MsgBox( IDS_ERROR_16_BIT_WARNING, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "16bitWarning" );
        }
    }

    // only play demo version for a month
    if ( IsShareware( ) )
    {
        const int i1Month = 60 * 60 * 24 * 30;
        int       iToday  = (int)time( NULL );

        if ( iWinType != W32s )
        {
            HKEY key;
            if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\DOS Emulation\\xCompatibility", NULL,
                               KEY_ALL_ACCESS, &key ) != ERROR_SUCCESS )
            {
                Log( "Expired 1" );
                AfxMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            // fix for bug in old setup
            RegSetValueEx( key, NULL, NULL, REG_SZ, (unsigned char*)"4", 2 );

            unsigned long iLen = 256;
            DWORD         dwTyp, dwLen = sizeof( DWORD );
            time_t        dwTime;
            if ( RegQueryValueEx( key, "CD-ROM", NULL, &dwTyp, (unsigned char*)&dwTime, &dwLen ) != ERROR_SUCCESS )
            {
                TRAP( );
                Log( "Expired 2" );
                AfxMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            if ( dwTyp != REG_DWORD )
            {
                TRAP( );
                Log( "Expired 3" );
                AfxMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            // if it's 41 then we need to set it
            if ( dwTime == 41 )
                RegSetValueEx( key, "CD-ROM", NULL, REG_DWORD, (unsigned char*)&iToday, sizeof( iToday ) );

            // have they had a month?
            else
                // is it earlier (ie did they advance the date before installing)?
                if ( iToday < (int)dwTime )
            {
                iToday -= i1Month / 2;
                RegSetValueEx( key, "CD-ROM", NULL, REG_DWORD, (unsigned char*)&iToday, sizeof( iToday ) );
            }
            else if ( iToday > (int)dwTime + i1Month )
            {
                CTime   _time( dwTime );
                CString sBuf = _time.Format( "Installed: %x" );
                Log( sBuf );
                AfxMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            RegCloseKey( key );
        }

        else
        {
            time_t iTime = ::GetProfileInt( "DOS Emulation", "_COMM", -1 );
            if ( iTime == -1 )
            {
                TRAP( );
                char sBuf[20];
                itoa( iToday + i1Month, sBuf, 10 );
                ::WriteProfileString( "DOS Emulation", "_COMM", sBuf );
            }
            else
                // is it earlier (ie did they advance the date before installing)?
                if ( iToday < (int)iTime )
            {
                iToday -= i1Month / 2;
                char sBuf[20];
                itoa( iToday + i1Month, sBuf, 10 );
                ::WriteProfileString( "DOS Emulation", "_COMM", sBuf );
            }
            else if ( iToday > (int)iTime + i1Month )
            {
                TRAP( );
                CTime   _time( iTime );
                CString sBuf = _time.Format( "Installed: %x" );
                Log( sBuf );
                AfxMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }
        }
    }

    Log( "Check for CD" );

    // do we have a CD?
    if ( !CheckForCD( ) )
        return ( 0 );

    // shareware notice
    if ( IsShareware( ) )
    {
        int iTry = GetProfileInt( "Game", "NumDemo", 1 );
        WriteProfileInt( "Game", "NumDemo", iTry + 1 );
        if ( ( iTry % 25 ) == 0 )
        {
            CString sMsg;
            sMsg.LoadString( IDS_DEMO_25 );
            CString sNum = IntToCString( iTry );
            csPrintf( &sMsg, (char const*)sNum );
            if ( AfxMessageBox( sMsg, MB_YESNO | MB_ICONSTOP ) != IDYES )
            {
                WriteProfileInt( "Game", "NumDemo", iTry );
                return ( 0 );
            }
        }

        CDlgLicense dlgLic( 4 );
        if ( dlgLic.DoModal( ) != IDOK )
            return ( 0 );
        m_wndMain.UpdateWindow( );
    }
    else if ( theApp.IsSecondDisk( ) )
    {
        CDlgLicense dlgLic( 5 );
        if ( dlgLic.DoModal( ) != IDOK )
            return ( 0 );
        m_wndMain.UpdateWindow( );
    }

    // set up the game
    Log( "Setup app" );
    try
    {
        m_hAccel = ::LoadAccelerators( m_hInstance, MAKEINTRESOURCE( IDR_ACCEL ) );
        if ( m_hAccel == NULL )
            ThrowError( ERR_RES_NO_ACCEL );
        Log( "Accelerators loaded" );

        // screen size, create button brushes
        m_iScrnX = GetSystemMetrics( SM_CXSCREEN );
        m_iScrnY = GetSystemMetrics( SM_CYSCREEN );

#ifdef BUGBUG
        // use Ctl3D
        SetDialogBkColor( ::GetOurSysClr( COLOR_BTNFACE ), ::GetOurSysClr( COLOR_WINDOWTEXT ) );
        Enable3dControls( );
#endif
        m_bSetSysColors = GetProfileInt( "Advanced", "SetSysColors", 0 );


        // RedText class for -#s in dialogs
//        if ( m_hPrevInstance == NULL )  // If statement removed because hPrevInstance is always null in modern windows
//        {
        WNDCLASS wc;
        memset( &wc, 0, sizeof( wc ) );
        wc.lpfnWndProc   = RedTextProc;
        wc.hInstance     = AfxGetInstanceHandle( );
        wc.lpszClassName = "RedText";
        if ( !RegisterClass( &wc ) )
            return ( FALSE );
        memset( &wc, 0, sizeof( wc ) );
        wc.lpfnWndProc   = PerBarProc;
        wc.cbWndExtra    = 2;
        wc.hInstance     = AfxGetInstanceHandle( );
        wc.lpszClassName = "dcPerBar";
        if ( !RegisterClass( &wc ) )
            return ( FALSE );
        Log( "Window classes registered" );
//        }

#ifdef _CHEAT
        _bShowRate       = GetProfileInt( "Debug", "ShowRate", 0 );
        _bClickAny       = GetProfileInt( "Cheat", "ClickAny", 0 );
        _bMaxMaterials   = GetProfileInt( "Cheat", "MaxMaterials", 0 );
        _bMaxRocket      = GetProfileInt( "Cheat", "MaxRocket", 0 );
        _bMaxPower       = GetProfileInt( "Cheat", "MaxPower", 0 );
        _iFrameRate      = GetProfileInt( "Cheat", "FrameRate", 1 );
        _iFrameRate      = __minmax( 1, 48, _iFrameRate );
        _bSeeAll         = GetProfileInt( "Cheat", "SeeAll", 0 );
        _bShowWorld      = GetProfileInt( "Cheat", "SeeWorld", 0 );
        _bShowStatus     = GetProfileInt( "Cheat", "ShowStatus", 0 );
        _bShowPos        = GetProfileInt( "Cheat", "ShowPos", 0 );
        _bShowAISelected = theApp.GetProfileInt( "Cheat", "ShowAISelected", 0 );
        _iScenarioOn     = theApp.GetProfileInt( "Cheat", "Scenario", -1 );
#endif

        // get screen resolution, default positions for windows
        m_sResIni = IntToCString( m_iScrnX ) + "x" + IntToCString( m_iScrnY );
        m_iCol1   = m_iScrnX / 5;
        m_iCol2   = __min( ( m_iScrnX * 4 ) / 5, m_iScrnX - 256 );
        m_iRow1   = m_iScrnY / 4;
        m_iRow2   = ( m_iScrnY * 9 ) / 16;
        m_iRow4   = ( m_iScrnY * 5 ) / 32;

        // get the font and sizes for the button bars
        Log( "Creating fonts" );
        CWindowDC dc( NULL );

        // get the main font - we try Newtown, then Arial, then Arial condensed till we fit
        LOGFONT lf;
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight   = GetProfileInt( "StatusBar", "CharHeight", 16 );
        CString sFont = GetProfileString( "StatusBar", "Font", "Newtown Italic" );
        strncpy( lf.lfFaceName, sFont, LF_FACESIZE - 1 );
        m_Fnt.CreateFontIndirect( &lf );

        TEXTMETRIC tm;
        dc.GetTextMetrics( &tm );
        m_iCharHt   = tm.tmHeight;
        m_iCharWid  = tm.tmAveCharWidth;
        m_iBtnBevel = GetSystemMetrics( SM_CXFRAME ) / 2;
        if ( m_iBtnBevel < 2 )
            m_iBtnBevel = 2;

        // dialog fonts
        int iHt = GetProfileInt( "StatusBar", "RDHeight", 14 );
        sFont   = GetProfileString( "StatusBar", "RDFont", "Lucida Console" );
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight = iHt;
        strncpy( lf.lfFaceName, sFont, LF_FACESIZE - 1 );
        m_FntRD.CreateFontIndirect( &lf );

        iHt   = GetProfileInt( "StatusBar", "DescHeight", 18 );
        sFont = GetProfileString( "StatusBar", "DescFont", "Newtown Italic" );
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight = iHt;
        strncpy( lf.lfFaceName, sFont, LF_FACESIZE - 1 );
        m_FntDesc.CreateFontIndirect( &lf );

        iHt   = GetProfileInt( "StatusBar", "CostHeight", 11 );
        sFont = GetProfileString( "StatusBar", "CostFont", "Lucida Console" );
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight = iHt;
        strncpy( lf.lfFaceName, sFont, LF_FACESIZE - 1 );
        m_FntCost.CreateFontIndirect( &lf );
        Log( "fonts Created" );

        m_iRow3 = m_iScrnY - TOOLBAR_HT;

        // Determine how many zoom levels to use
        m_ptrzoomdata = new CZoomData( m_nDataZooms );

        // read the basics from the RIF file
        {
            Log( "Reading palette" );
            CMmio* pMmio = theDataFile.OpenAsMMIO( "misc", "MISC" );
            Log( "palette read" );
            pMmio->DescendRiff( 'M', 'I', 'S', 'C' );

            pMmio->DescendList( 'P', 'A', 'L', 'T' );
            pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
            int   iLen = (int)pMmio->ReadLong( );
            char* pBmp = new char[iLen];
            pMmio->Read( pBmp, iLen );
            pMmio->AscendChunk( );
            pMmio->AscendList( );
            Log( "Initialize palette" );
            thePal.Init( );
            Log( "palette initialized" );
            thePal.SetColors( (RGBQUAD*)( pBmp + sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) ), 0, 256 );
            Log( "palette colors set" );
            delete[] pBmp;

            // this is our main window - first created and last destroyed
            // already created by here but now we can load it's data (palette above)
            m_wndMain.LoadData( );
            m_pMainWnd = &m_wndMain;

            // set up the thread code if we're Win32s (after window created)
            Log( "Initialize AI multi-threading" );
            myThreadInit( (AITHREAD)AiThread );
            Log( "AI multi-threading initialized" );

            // set up async disk reads
            // BUGBUG		myStartThread (CDiskCache::ThreadFunc, &theDiskCache, pri_high);
            // BUGBUG		theDiskCache.Open ( m_wndMain.m_hWnd );

            // set up color depths
            if ( m_bUse8Bit )
                ptrthebltformat = new CBLTFormat( CColorFormat( CColorFormat::DEPTH_EIGHT ) );
            else
                ptrthebltformat = new CBLTFormat;

            if ( 8 == ptrthebltformat->GetBitsPerPixel( ) )
                strcpy( m_szMapBPS, "08" );
            else
                strcpy( m_szMapBPS, "24" );
            strcpy( m_szOtherBPS, m_szMapBPS );
#ifdef BUGBUG
            TRAP( );
            if ( m_bUse8Bit )
                m_MapClrFmt.SetBitsPerPixel( CColorFormat::DEPTH_EIGHT );
            else
                m_MapClrFmt.CalcScreenFormat( );
            if ( 8 == m_MapClrFmt.GetBitsPerPixel( ) )
                strcpy( m_szMapBPS, "08" );
            else
                strcpy( m_szMapBPS, "24" );

            if ( !m_bHave24 )
            {
                TRAP( );
                m_OtherClrFmt.SetBitsPerPixel( CColorFormat::DEPTH_EIGHT );
                strcpy( m_szOtherBPS, "08" );
            }
            else
            {
                TRAP( );
                m_OtherClrFmt.CalcScreenFormat( );
                if ( 8 == m_OtherClrFmt.GetBitsPerPixel( ) )
                    strcpy( m_szOtherBPS, "08" );
                else
                    strcpy( m_szOtherBPS, "24" );
            }
#endif

            // load the buttons
            pMmio->DescendList( 'B', 'T', m_szOtherBPS[0], m_szOtherBPS[1] );
            theBmBtnData.Init( pMmio );
            pMmio->AscendList( );

            pMmio->DescendList( 'T', 'X', m_szOtherBPS[0], m_szOtherBPS[1] );
            theTextBtnData.Init( pMmio );
            theLargeTextBtnData.Init( pMmio );
            theCutTextBtnData.Init( pMmio );
            pMmio->AscendList( );

            pMmio->DescendList( 'I', 'C', m_szOtherBPS[0], m_szOtherBPS[1] );
            theIcons.Init( pMmio );
            pMmio->AscendList( );

            // bitmaps
            pMmio->DescendList( 'B', 'M', m_szOtherBPS[0], m_szOtherBPS[1] );
            theBitmaps.Init( pMmio );

            pMmio->AscendList( );

            delete pMmio;

// time the CD
#ifndef _GG
            if ( ( m_iCdSpeed = GetProfileInt( "Advanced", "CDspeed", 0 ) ) <= 0 )
            {
                CFile* pFile = theDataFile.OpenAsFile( "music" );
                void*  pBuf  = malloc( 0x10000 );
                int    iPri  = GetThreadPriority( );
                ::Sleep( 0 );
                SetThreadPriority( THREAD_PRIORITY_HIGHEST );
                pFile->Seek( 0, CFile::begin );
                pFile->Read( pBuf, 0x1000 );

                m_iCdSpeed = timeGetTime( );
                pFile->Seek( 1234, CFile::begin );
                pFile->Read( pBuf, 0x10000 );
                pFile->Seek( 1234 + 0x80000, CFile::begin );
                pFile->Read( pBuf, 0x10000 );
                pFile->Seek( 1234 + 0x20000, CFile::begin );
                pFile->Read( pBuf, 0x10000 );
                m_iCdSpeed = timeGetTime( ) - m_iCdSpeed;

                SetThreadPriority( iPri );
                m_iCdSpeed = __max( 1, m_iCdSpeed );
                m_iCdSpeed = 2137 / m_iCdSpeed;
                m_iCdSpeed = __max( 1, m_iCdSpeed );
                free( pBuf );
                delete pFile;
            }

#endif
            // read in the music & sound
            // grab the audio
            if ( !m_bWAV )
                m_mMode = CMusicPlayer::MUSIC_MODE::midi_only;
            else
                switch ( GetProfileInt( "Advanced", "Music", -1 ) )
                {
                case 2:
                    if ( ( ms.dwAvailPageFile < 1000 * 1000 * MEM_NEEDED_MUSIC_MIXED ) || ( m_iCdSpeed < 4 ) )
                        m_mMode = CMusicPlayer::MUSIC_MODE::midi_only;
                    else
                        m_mMode = CMusicPlayer::MUSIC_MODE::mixed;
                    break;

                case 1:
                    m_mMode = CMusicPlayer::MUSIC_MODE::wav_only;
                    if ( ( ms.dwAvailPageFile < 1000 * 1000 * MEM_NEEDED_MUSIC_DIGITAL ) || ( m_iCdSpeed < 6 ) ||
                         ( m_iCpuSpeed < 120 ) )
                    {
                        CDlgMsg dlg;
                        if ( dlg.MsgBox( IDS_ERROR_LOW_MUSIC, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                                         "LessThanMusic" ) == IDYES )
                        {
                            if ( ( ms.dwAvailPageFile < 1000 * 1000 * MEM_NEEDED_MUSIC_MIXED ) || ( m_iCdSpeed < 4 ) )
                                m_mMode = CMusicPlayer::MUSIC_MODE::midi_only;
                            else
                                m_mMode = CMusicPlayer::MUSIC_MODE::mixed;
                        }
                        m_wndMain.UpdateWindow( );
                    }
                    break;

                default:
                    if ( ( ms.dwAvailPageFile < 1000 * 1000 * MEM_NEEDED_MUSIC_MIXED ) || ( m_iCdSpeed < 4 ) )
                        m_mMode = CMusicPlayer::MUSIC_MODE::midi_only;
                    else if ( ( ms.dwAvailPageFile < 1000 * 1000 * MEM_NEEDED_MUSIC_DIGITAL ) || ( m_iCdSpeed < 6 ) ||
                              ( m_iCpuSpeed < 120 ) )
                        m_mMode = CMusicPlayer::MUSIC_MODE::mixed;
                    else
                        m_mMode = CMusicPlayer::MUSIC_MODE::wav_only;
                    break;
                }
            Log( "Initialize music & sfx" );
            theMusicPlayer.InitData( m_mMode, SFXGROUP::global );
            m_mMode = theMusicPlayer.GetMode( );
            Log( "Music & sfx initialized" );
        }

        thePal.UpdateDeviceColors( 0, 256 );

        // this is the window class for all our popup windows
        m_sWndCls = AfxRegisterWndClass( CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                                         LoadStandardCursor( IDC_ARROW ), 0, 0 );

#ifdef _DEBUG
        theDataFile.DisableNegativeSeekChecking( );
#endif

        // demo license agreement
        if ( IsShareware( ) )
            if ( GetProfileInt( "Game", "NoIntro", 0 ) == 0 )
            {
                PostIntro( );
                goto MovieDone;
            }

#ifdef BUGBUG
        // remind them to register
        if ( W32s != iWinType )
            if ( ( !IsShareware( ) ) && ( GetProfileInt( "Warnings", "Register", 0 ) == 0 ) )
            {
                CDlgReg dlg( &m_wndMain );
                dlg.DoModal( );
                m_wndMain.UpdateWindow( );
            }
#endif

        // Play the startup movie
        if ( ( HaveIntro( ) ) && ( GetProfileInt( "Game", "NoIntro", 0 ) == 0 ) )
        {
            try
            {
                m_wndMovie.AddMovie( "logo.avi" );
//                m_wndMovie.AddMovie( "headgame.avi" );  // This file doesnt exist
                m_wndMovie.AddMovie( "intro.avi" );
                m_wndMovie.Create( FALSE );
            }
            catch ( ... )
            {
                PostIntro( );
            }
        }
        else {
            PostIntro();
        }

    MovieDone:;
    }

    catch ( int iErr )
    {
        CatchNum( iErr );
        return ( 0 );
    }
    catch ( SE_Exception& e )
    {
        CatchSE( e );
        return ( 0 );
    }
    catch ( ... )
    {
        AfxMessageBox( IDS_ERR_LOAD_2, MB_OK | MB_ICONSTOP );
        return ( 0 );
    }

    // list out version
    m_sRif = "Data Ver: " + IntToCString( theApp.GetRifVer( ) ) + "." + IntToCString( VER_RIFF );
    if ( theApp.IsShareware( ) )
        m_sRif += " {Shareware}";
    if ( theApp.HaveWAV( ) )
        m_sRif += ", WAV";
    else
        m_sRif += ", MIDI";
    if ( theApp.Have24Bit( ) )
        m_sRif += ", 24-bit";
    else
        m_sRif += ", 8-bit";
    if ( theApp.GetFirstZoom( ) )
        m_sRif += ", Zoom1";
    else
        m_sRif += ", Zoom0";
    Log( m_sRif );

    // video info
    m_sVideo = "Video: ";
    switch ( ptrthebltformat->GetType( ) )
    {
    case CBLTFormat::DIB_DIRECTDRAW:
        m_sVideo += "DirectDraw";
        break;
    case CBLTFormat::DIB_WING:
        m_sVideo += "WinG";
        break;
    case CBLTFormat::DIB_DIBSECTION:
        m_sVideo += "CreateDIBSection";
        break;
    case CBLTFormat::DIB_MEMORY:
        m_sVideo += "StretchDIBits";
        break;
    }
    m_sVideo += " (";

    switch ( ptrthebltformat->GetDirection( ) )
    {
    case CBLTFormat::DIR_TOPDOWN:
        m_sVideo += "top-down";
        break;
    case CBLTFormat::DIR_BOTTOMUP:
        m_sVideo += "bottom-up";
        break;
    }
    m_sVideo += "), " + IntToCString( ptrthebltformat->GetBitsPerPixel( ) ) + "-bit, (" +
                IntToCString( GetSystemMetrics( SM_CXSCREEN ) ) + "x" +
                IntToCString( GetSystemMetrics( SM_CYSCREEN ) ) + "x";
    hdc           = GetDC( NULL );
    int iBitDepth = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
    ReleaseDC( NULL, hdc );
    m_sVideo += IntToCString( iBitDepth ) + ")";
    Log( m_sVideo );

    // sound info
    m_sSound = "Sound: ";
    switch ( theMusicPlayer.GetMode( ) )
    {
    case CMusicPlayer::MUSIC_MODE::midi_only:
        m_sSound += "MIDI Music";
        break;
    case CMusicPlayer::MUSIC_MODE::mixed:
        m_sSound += "MIDI && Digital Music";
        break;
    case CMusicPlayer::MUSIC_MODE::wav_only:
        m_sSound += "Digital Music";
        break;
    }
    if ( theMusicPlayer.UseDirectSound( ) )
        m_sSound += " (DirectSound)";
    else
        m_sSound += " (MME)";

    if ( ( !theMusicPlayer.MidiOk( ) ) && ( !theMusicPlayer.WavOk( ) ) )
        m_sSound += " {MIDI and WAV drivers failed}";
    else if ( !theMusicPlayer.MidiOk( ) )
        m_sSound += " {MIDI driver failed}";
    else if ( !theMusicPlayer.WavOk( ) )
        m_sSound += " {WAV driver failed}";
    else if ( !theMusicPlayer.IsRunning( ) )
        m_sSound += " {turned off}";
    Log( m_sSound );

    if ( theMusicPlayer._GetHDig( ) == 0 )
        m_sSoundVer = "MSS: " + CString( theMusicPlayer.GetVersion( ) ) + " {off}";
    else
    {
        char sBuf[130], *psFmt;
        long iRate, iFmt;
        sBuf[0] = 0;
        AIL_digital_configuration( theMusicPlayer._GetHDig( ), &iRate, &iFmt, sBuf );
        switch ( iFmt )
        {
        case DIG_F_MONO_8:
            psFmt = "K/8-bit/Mono, ";
            break;
        case DIG_F_MONO_16:
            psFmt = "K/16-bit/Mono, ";
            break;
        case DIG_F_STEREO_8:
            psFmt = "K/8-bit/Stereo, ";
            break;
        case DIG_F_STEREO_16:
            psFmt = "K/16-bit/Stereo, ";
            break;
        }
        m_sSoundVer =
            "MSS: " + CString( theMusicPlayer.GetVersion( ) ) + " " + IntToCString( iRate ) + CString( psFmt ) + sBuf;
    }
    Log( m_sSoundVer );

    m_sSpeed = "CPU Speed: ~" + IntToCString( theApp.GetCpuSpeed( ) ) + "  CD-ROM Speed: ~" +
               IntToCString( theApp.GetCdSpeed( ) ) + "X";
    Log( m_sSpeed );

    if ( iWinType == W32s )
    {
        Log( "iWinType w32s" );
        WORD    wVer   = myGetThrdUtlsVersion( );
        CString sThunk = "Threads DLL " + IntToCString( HIBYTE( wVer ) ) + "." + IntToCString( LOBYTE( wVer ) );
        Log( sThunk );
    }

    Log( "Initialization complete" );

    return TRUE;
}

CDlgPause* CConquerApp::GetDlgPause( )
{

    if ( m_pdlgPause == NULL )
        m_pdlgPause = new CDlgPause( &m_wndMain );
    return ( m_pdlgPause );
}

void CConquerApp::PostIntro( )
{
    static BOOL bDidIt = FALSE;  // could be called twice on exception above

    m_wndMain.SetProgPos( CWndMain::playing );
    ShowCursor( TRUE );

    GetDlgPause( );

    if ( !bDidIt )
    {
        bDidIt = TRUE;

        // start the audio
        theMusicPlayer.Open( GetProfileInt( "Game", "Music", 100 ), GetProfileInt( "Game", "Sound", 100 ), m_mMode,
                             SFXGROUP::global );

        if ( GetProfileInt( "Game", "CustomUI", W32s != iWinType ) ) {
            InitCustomUI();
        }
        theMusicPlayer.YieldPlayer( );
    }

    CreateMain( );
}

void CConquerApp::CloseApp( )
{
    static BOOL bCalled = FALSE;

    if ( bCalled )
        return;
    bCalled = TRUE;

    DestroyWorld( );

    m_wndMain.SetProgPos( CWndMain::exiting );

    CGlobalSubClass::UnSubClass( );

    ::PostQuitMessage( 0 );
}

//---------------------------------------------------------------------------
// CConquerApp::InitCustomUI	Enable global subclassing for NC and controls
//---------------------------------------------------------------------------
void CConquerApp::InitCustomUI( )
{
    CTextColors textcolorsButton( RGB( 132, 154, 255 ), RGB( 230, 180, 115 ), RGB( 40, 50, 100 ) );

    CTextColors textcolorsStatic( RGB( 230, 190, 120 ), RGB( 84, 96, 216 ), RGB( 100, 80, 55 ) );

    auto bkgnd = theBitmaps.GetByIndex(CBitmapLib::DLG_BKGND);
    auto rdioBtn = theBitmaps.GetByIndex(CBitmapLib::DLG_RADIO_BUTTONS);
    auto chkBox = theBitmaps.GetByIndex(CBitmapLib::DLG_CHECK_BOXES);
    auto soundid = SOUNDS::GetID(SOUNDS::button);

    CGlobalSubClass::Subclass( bkgnd, theTextBtnData.m_pcDib, theLargeTextBtnData.m_pcDib,
                               rdioBtn, chkBox, &theLargeTextBtnData.m_fntText, soundid,
                               textcolorsButton, textcolorsStatic );

    CFramePainter::SetDrawInfo( theBitmaps.m_ppDibs + CBitmapLib::FRAME_LL_CORNER );

    m_bSubClass = TRUE;
}

void CConquerApp::Minimize( )
{

    m_pMainWnd->ShowWindow( SW_MINIMIZE );
}

void CConquerApp::CreateMain( )
{

    // close chat (cancel on multi create)
    CloseDlgChat( );

    // kill movie if running
    if ( ( m_wndMovie.m_hWnd != NULL ) && ( !m_wndMovie.m_bInDestroy ) )
        m_wndMovie.DestroyWindow( );

    // if coming from a game setup - kill it
    theGame.SetShouldProcessMessages(FALSE);
    DestroyExceptMain( );

    m_wndMain.SetProgPos( CWndMain::playing );

    bDoSubclass = TRUE;

    if ( m_pdlgMain == NULL )
    {
        m_pdlgMain = new CDlgMain( m_pMainWnd );
        m_pdlgMain->Create( IDD_MAIN, m_pMainWnd );
    }

    m_pdlgMain->EnableWindow( TRUE );
    m_pdlgMain->ShowWindow( SW_SHOW );
    theGame.SetState( CGame::main );

    CheckForCD( );

    theMusicPlayer.PlayExclusiveMusic( MUSIC::GetID( MUSIC::main_screen ) );
}

void CConquerApp::DisableMain( )
{

    if ( m_pdlgMain == NULL )
        return;

    m_pdlgMain->EnableWindow( FALSE );
    m_pdlgMain->ShowWindow( SW_HIDE );
}

void CConquerApp::DestroyMain( )
{

    if ( m_pdlgMain == NULL )
        return;

    CDlgMain* pDm = m_pdlgMain;
    m_pdlgMain    = NULL;
    pDm->DestroyWindow( );
}

void CConquerApp::DestroyExceptMain( )
{

    if ( m_pCreateGame != NULL )
    {
        m_pCreateGame->CloseAll( );
        delete m_pCreateGame;
        m_pCreateGame = NULL;
    }

    // kill pause window
    if ( ( m_pdlgPause != NULL ) && ( m_pdlgPause->m_hWnd != NULL ) )
    {
        m_pdlgPause->DestroyWindow( );
        m_pdlgPause = NULL;
    }

    // kill cut scene
    if ( m_wndCutScene.m_hWnd != NULL )
        m_wndCutScene.DestroyWindow( );
}

CDlgChatAll* CConquerApp::GetDlgChat( )
{

    if ( m_pdlgChat == NULL )
        m_pdlgChat = new CDlgChatAll( &m_wndMain );
    if ( m_pdlgChat->m_hWnd == NULL )
        m_pdlgChat->Create( IDD_CHAT_INIT, &m_wndMain );

    return ( m_pdlgChat );
}

void CConquerApp::CloseDlgChat( )
{

    if ( ( m_pdlgChat != NULL ) && ( m_pdlgChat->m_hWnd != NULL ) )
        m_pdlgChat->DestroyWindow( );
    m_pdlgChat = NULL;
}

int CConquerApp::ExitInstance( )
{

    CGlobalSubClass::UnSubClass( );

    // close out sprites
    theTransports.Close( );
    theTurrets.Close( );
    theFlashes.Close( );
    theStructures.Close( );
    theEffects.Close( );
    theTerrain.Close( );

    _set_se_translator( prevFn );

    // BUGBUG	theDiskCache.Close ();

    theIcons.Close( );

    theMusicPlayer.Close( );

    myThreadClose( (THREADEXITFUNC)AiExit );

    delete m_pdlgPause;
    m_pdlgPause = NULL;

    DestroyExceptMain( );  // if in create
    if ( m_wndBar.m_hWnd != NULL )
        DestroyWorld( );  // game
    DestroyMain( );       // main window (dialog)

    // draw black so no palette uglyness
    if ( m_pMainWnd != NULL )
    {
        CWindowDC dc( &m_wndMain );
        CBrush    brBlack;
        brBlack.CreateSolidBrush( RGB( 0, 0, 0 ) );
        CRect rect( 0, 0, GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) );
        dc.FillRect( &rect, &brBlack );
        dc.SetBkMode( TRANSPARENT );
        dc.SetTextColor( RGB( 0, 0, 0 ) );
        CString sLoad;
        sLoad.LoadString( IDS_LEAVING );
        dc.TextOut( 0, 0, sLoad );
    }

    m_wndMain.DestroyWindow( );  // background window

    DeleteCriticalSection( &cs );
    CloseHandle( hRenderEvent );

#ifdef USE_SMARTHEAP
 //   MemUnregisterTask( );
#endif

    if ( m_hLibLang != NULL )
        FreeLibrary( m_hLibLang );
    delete[] m_piLangAvail;

    if ( m_iRestoreRes )
    {
        DEVMODE dev;
        memset( &dev, 0, sizeof( dev ) );
        dev.dmSize       = sizeof( dev );
        dev.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        dev.dmPelsWidth  = m_iOldWidth;
        dev.dmPelsHeight = m_iOldHeight;
        dev.dmBitsPerPel = m_iOldDepth;

        LONG lRtn = ChangeDisplaySettings( &dev, 0 );
        char sBuf[80];
        sprintf( sBuf, "restore ChangeDisplaySettings (%dx%dx%d) = %d", dev.dmPelsWidth, dev.dmPelsHeight,
                 dev.dmBitsPerPel, lRtn );
        Log( sBuf );
    }

    if ( m_pLogFile != NULL )
    {
        m_pLogFile->Close( );
        delete m_pLogFile;
        m_pLogFile = NULL;
    }

    // show the order form
    if ( IsShareware( ) )
    {
        CString sCmd = GetDefaultApp( ".doc", "write", "order.doc" );

        STARTUPINFO si;
        memset( &si, 0, sizeof( si ) );
        si.cb          = sizeof( si );
        si.wShowWindow = SW_SHOWMAXIMIZED;
        si.dwFlags     = STARTF_USESHOWWINDOW;
        PROCESS_INFORMATION pi;

        CreateProcess( NULL, (char*)(char const*)sCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    return ( CWinApp::ExitInstance( ) );
}

// true - continue
BOOL CConquerApp::SaveGame( CWnd* pPar )
{

    // if we aren't playing - exit
    if ( theGame.GetState( ) != CGame::play )
        return ( TRUE );

    ASSERT( TestEverything( ) );

    int iRtn;
    if ( theGame.AmServer( ) )
    {
        if ( theGame.IsNetGame( ) )
        {
            if ( AfxMessageBox( IDS_SERVER_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL ) == IDNO )
                return ( FALSE );
        }
        else if ( AfxMessageBox( IDS_SINGLE_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL ) == IDNO )
            return ( FALSE );
    }
    else if ( AfxMessageBox( IDS_CLIENT_QUIT, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL ) == IDNO )
        return ( FALSE );

    CWndArea* pWnd = theAreaList.GetTop( );
    if ( pWnd != NULL )
        if ( ( pWnd->GetMode( ) != CWndArea::rocket_ready ) && ( pWnd->GetMode( ) != CWndArea::rocket_pos ) &&
             ( pWnd->GetMode( ) != CWndArea::rocket_wait ) )
        {
            iRtn = AfxMessageBox( IDS_SAVE_OLD, MB_YESNOCANCEL | MB_ICONQUESTION );
            if ( iRtn == IDCANCEL )
                return ( FALSE );
            if ( iRtn == IDYES )
                if ( theGame.SaveGame( pPar ) != IDOK )
                    return ( FALSE );
        }

    return ( TRUE );
}

BOOL CConquerApp::PreTranslateMessage( MSG* pMsg )
{

    if ( pMsg->message == WM_KEYDOWN )
        switch ( pMsg->wParam )
        {
        case VK_F1:
            theApp.WinHelp( 0, HELP_CONTENTS );
            return ( TRUE );

#ifdef _CHEAT
        // repaint everything
        case VK_F11:
            CWndAnim::InvalidateAllWindows( );
            break;

        // erase WinG screen area
        case VK_F12: {
            POSITION pos = theAnimList.GetHeadPosition( );
            while ( pos != NULL )
            {
                CWndAnim* pWnd = theAnimList.GetNext( pos );
                CClientDC dc( pWnd );
                CBrush    br;
                br.CreateSolidBrush( RGB( 0, 0, 0 ) );
                CRect rect;
                pWnd->GetClientRect( &rect );
                dc.FillRect( &rect, &br );
            }
            break;
        }
#endif
        }

    return ( CWinApp::PreTranslateMessage( pMsg ) );
}

#ifdef _DEBUG
void CConquerApp::AssertValid( ) const
{

    CWinApp::AssertValid( );

    ASSERT_VALID( &m_wndWorld );
    ASSERT_VALID( &m_wndChat );
    ASSERT_VALID( &m_wndBar );
    ASSERT_VALID( &m_wndBldgs );
    ASSERT_VALID( &m_wndVehicles );

    ASSERT_VALID( &m_wndMain );
    ASSERT_VALID_OR_NULL( m_pdlgMain );
    ASSERT_VALID_OR_NULL( m_pCreateGame );
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgMain dialog

CDlgMain::CDlgMain( CWnd* pParent /*=NULL*/ ): CDialog( CDlgMain::IDD, pParent )
{
    //{{AFX_DATA_INIT(CDlgMain)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_bTile = FALSE;

    m_pcdibTmp  = NULL;
    m_pcdibWall = NULL;
    for ( int iInd = 0; iInd < NUM_BTNS; iInd++ ) m_pcdibBtns[iInd] = NULL;
}

CDlgMain::~CDlgMain( )
{

    delete m_pcdibWall;
    for ( int iInd = 0; iInd < NUM_BTNS; iInd++ ) delete m_pcdibBtns[iInd];
    delete m_pcdibTmp;
}

void CDlgMain::DoDataExchange( CDataExchange* pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CDlgMain)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP( CDlgMain, CDialog )
//{{AFX_MSG_MAP(CDlgMain)
ON_WM_CREATE( )
ON_WM_PAINT( )
ON_BN_CLICKED( IDC_MAIN_CAMPAIGN, OnMainScenario )
ON_BN_CLICKED( IDC_MAIN_SINGLE, OnMainSingle )
ON_BN_CLICKED( IDC_MAIN_CREATE, OnMainCreate )
ON_BN_CLICKED( IDC_MAIN_JOIN, OnMainJoin )
ON_BN_CLICKED( IDC_MAIN_LOAD, OnMainLoad )
ON_BN_CLICKED( IDCANCEL, OnMainExit )
ON_WM_SIZE( )
ON_WM_DRAWITEM( )
ON_BN_CLICKED( IDC_MINIMIZE, OnMinimize )
ON_BN_CLICKED( IDC_MAIN_LOAD_MUL, OnMainLoadMulti )
ON_BN_CLICKED( IDC_MAIN_CREDITS, OnMainCredits )
ON_BN_CLICKED( IDC_MAIN_INTRO, OnMainIntro )
ON_BN_CLICKED( IDC_MAIN_OPTIONS, OnMainOptions )
ON_WM_SYSCOMMAND( )
ON_WM_DESTROY( )
ON_WM_ERASEBKGND( )
ON_WM_PALETTECHANGED( )
ON_WM_QUERYNEWPALETTE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


// data for each button
class _BTN_DATA
{
  public:
    _BTN_DATA( UINT _ID, int _x, int _y, UINT _fmt, CRect _rText, CPoint _ptDnOff )
    {
        ID      = _ID;
        x       = _x;
        y       = _y;
        fmt     = _fmt;
        rText   = _rText;
        ptDnOff = _ptDnOff;
    }

    UINT   ID;
    int    x;
    int    y;
    UINT   fmt;
    CRect  rText;
    CPoint ptDnOff;
};

static _BTN_DATA _btnData[NUM_BTNS] = {
    _BTN_DATA( IDC_MAIN_LOAD, 784, 25, DT_CENTER | DT_VCENTER | DT_SINGLELINE, CRect( 18, 10, 213, 84 ),
               CPoint( 244, 12 ) ),
    _BTN_DATA( IDC_MAIN_OPTIONS, 1060, 28, DT_CENTER | DT_VCENTER | DT_SINGLELINE, CRect( 34, 9, 214, 86 ),
               CPoint( 243, 12 ) ),
    _BTN_DATA( IDC_MAIN_LOAD_MUL, 776, 135, DT_CENTER | DT_VCENTER | DT_SINGLELINE, CRect( 26, 17, 214, 87 ),
               CPoint( 257, 17 ) ),
    _BTN_DATA( IDC_MAIN_CREDITS, 1052, 138, DT_CENTER | DT_VCENTER | DT_SINGLELINE, CRect( 30, 16, 211, 82 ),
               CPoint( 249, 15 ) ),
    _BTN_DATA( IDC_MAIN_CAMPAIGN, 100, 489, DT_LEFT | DT_WORDBREAK, CRect( 57, 12, 181, 110 ), CPoint( 287, 29 ) ),
    _BTN_DATA( IDC_MAIN_SINGLE, 293, 490, DT_LEFT | DT_WORDBREAK, CRect( 37, 12, 170, 112 ), CPoint( 260, 28 ) ),
    _BTN_DATA( IDC_MAIN_CREATE, 345, 632, DT_LEFT | DT_WORDBREAK, CRect( 25, 11, 140, 65 ), CPoint( 203, 19 ) ),
    _BTN_DATA( IDC_MAIN_JOIN, 326, 744, DT_LEFT | DT_WORDBREAK, CRect( 26, 14, 149, 78 ), CPoint( 213, 25 ) ),
    _BTN_DATA( IDC_MAIN_INTRO, 868, 407, DT_CENTER | DT_WORDBREAK, CRect( 38, 38, 177, 93 ), CPoint( 249, 47 ) ),
    _BTN_DATA( IDCANCEL, 1021, 604, DT_CENTER | DT_VCENTER | DT_SINGLELINE, CRect( 41, 40, 186, 73 ),
               CPoint( 284, 51 ) ),
    _BTN_DATA( IDC_MINIMIZE, 1035, 703, DT_CENTER | DT_VCENTER | DT_SINGLELINE, CRect( 59, 11, 201, 46 ),
               CPoint( 296, 21 ) )
};

/////////////////////////////////////////////////////////////////////////////
// CDlgMain message handlers

// main GAME window? like not main menu, its created when we start a new game, and is
// sized the entire window
int CDlgMain::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
#ifdef LOGGINGON
    OutputDebugStringA( "CDlgMain::OnCreate\n" );
#endif

    if ( CDialog::OnCreate( lpCreateStruct ) == -1 )
        return -1;

    // get the art
    CMmio* pMmio = theDataFile.OpenAsMMIO( "misc", "MISC" );

    pMmio->DescendRiff( 'M', 'I', 'S', 'C' );
    try
    {
        m_bTile = FALSE;
        pMmio->DescendList( 'M', 'N', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1] );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
    }
    catch ( ... )
    {
        m_bTile = TRUE;
        delete pMmio;
        CMmio* pMmio = theDataFile.OpenAsMMIO( "misc", "MISC" );
        pMmio->DescendRiff( 'M', 'I', 'S', 'C' );
        pMmio->DescendList( 'W', 'L', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1] );
        pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
    }

    delete m_pcdibWall;
    m_pcdibWall =
        new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY, ptrthebltformat->GetMemDirection( ) );
    m_pcdibWall->Load( *pMmio );

    // load buttons if we have the main screen
    if ( !m_bTile )
    {
        pMmio->AscendChunk( );
        for ( int iInd = 0; iInd < NUM_BTNS; iInd++ )
        {
            pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
            m_pcdibBtns[iInd] =
                new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY, CBLTFormat::DIR_TOPDOWN );
            m_pcdibBtns[iInd]->Load( *pMmio );
            pMmio->AscendChunk( );
        }

        CRect rect;
        GetClientRect( &rect );
        m_pcdibTmp = new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY,
                               ptrthebltformat->GetMemDirection( ), rect.Width( ), rect.Height( ) );
    }

    delete pMmio;

    return 0;
}

BOOL CDlgMain::OnEraseBkgnd( CDC* )
{
    return TRUE;
}

BOOL CDlgMain::OnInitDialog( )
{
#ifdef LOGGINGON
    OutputDebugStringA( "CDlgMain::OnInitDialog\n" );
#endif

#ifdef _DEBUG
    //dbgMemSetDefaultErrorOutput( DBGMEM_OUTPUT_FILE, "malloc.log" );
    //dbgMemReportLeakage( NULL, 1, 1 );
#endif

    CDialog::OnInitDialog( );

    // get the parent window and take up the same space.
    CRect rect;
    theApp.m_pMainWnd->GetWindowRect( &rect );
    SetWindowPos( NULL, rect.left, rect.top, rect.Width( ), rect.Height( ), SWP_NOZORDER );

    // kill any dialogs that might be up
    theApp.DestroyExceptMain( );

    SendMessage( WM_SETICON, (WPARAM)TRUE, (LPARAM)theApp.LoadIcon( MAKEINTRESOURCE( IDI_MAIN ) ) );
    CString sTitle;
    sTitle.LoadString( IDS_MAIN_TITLE );
    SetWindowText( sTitle );

    // if shareware no loading
    if ( ( theApp.IsShareware( ) ) || ( theApp.IsSecondDisk( ) ) )
    {
        GetDlgItem( IDC_MAIN_LOAD )->EnableWindow( FALSE );
        GetDlgItem( IDC_MAIN_LOAD_MUL )->EnableWindow( FALSE );
    }
    // second CD can JOIN game, not create it
    if ( theApp.IsSecondDisk( ) )
        GetDlgItem( IDC_MAIN_CREATE )->EnableWindow( FALSE );

    // if no movie - disable the button
    if ( !theApp.HaveIntro( ) )
        GetDlgItem( IDC_MAIN_INTRO )->EnableWindow( FALSE );

#ifdef HACK_TEST_AI
    GetDlgItem( IDC_MAIN_CREATE )->EnableWindow( FALSE );
    GetDlgItem( IDC_MAIN_JOIN )->EnableWindow( FALSE );
    GetDlgItem( IDC_MAIN_LOAD )->EnableWindow( FALSE );
    GetDlgItem( IDC_MAIN_LOAD_MUL )->EnableWindow( FALSE );
#endif

    // resize for screen
    if ( m_bTile )
        for ( int iOn = 0; iOn < NUM_BTNS; iOn++ )
        {
            CWnd* pBtn = GetDlgItem( aiBtns[iOn] );
            CRect rect;
            pBtn->GetClientRect( &rect );
            pBtn->SetWindowPos( NULL, 0, 0, rect.Width( ) / 2 + ( theApp.m_iScrnX * rect.Width( ) ) / 2560,
                                rect.Height( ) / 2 + ( theApp.m_iScrnY * rect.Height( ) ) / 2048,
                                SWP_NOMOVE | SWP_NOZORDER );
        }

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDlgMain::OnSize( UINT nType, int cx, int cy )
{
#ifdef LOGGINGON
    // Print a message with the cx and cy values
    char buf[128];
    sprintf_s( buf, sizeof( buf ), "OnSize called: cx=%d, cy=%d\n", cx, cy );
    OutputDebugStringA( buf );
#endif

    CDialog::OnSize( nType, cx, cy );

    // not init'ed yet
    CWnd* pMain = GetDlgItem( IDC_MAIN_CAMPAIGN );
    if ( pMain == NULL )
    {
        // probably not an issue!
#ifdef LOGGINGON
     //   OutputDebugStringA( "not init'ed yet\n" );
#endif
        return;
    }
    else 
    {
#ifdef LOGGINGON
     //   OutputDebugStringA( "OnSize a go!\n" );
#endif
    }

    // arrange buttons - if using art
    if ( !m_bTile )
    {
        CRect rect;
        GetClientRect( &rect );

        for ( int iInd = 0; iInd < NUM_BTNS; iInd++ )
        {
            CWnd* pWnd = GetDlgItem( _btnData[iInd].ID );
            pWnd->SetWindowPos( NULL, ( _btnData[iInd].x * rect.Width( ) ) / m_pcdibWall->GetWidth( ),
                                ( _btnData[iInd].y * rect.Height( ) ) / m_pcdibWall->GetHeight( ),
                                ( m_pcdibBtns[iInd]->GetWidth( ) * rect.Width( ) ) / ( m_pcdibWall->GetWidth( ) * 3 ),
                                ( m_pcdibBtns[iInd]->GetHeight( ) * rect.Height( ) ) / m_pcdibWall->GetHeight( ),
                                SWP_NOZORDER );
        }
        m_pcdibTmp->Resize( rect.Width( ), rect.Height( ) );
        return;
    }

    // no art - regular dialog

    // arrange the button positions
    CRect rect;
    GetClientRect( &rect );
    CRect rectBtn;
    pMain->GetClientRect( &rectBtn );

    // figure the Y button position
    int iYadd = rectBtn.Height( ) + rectBtn.Height( ) / 2;
    int iHt   = rectBtn.Height( ) + iYadd * ( ( NUM_BTNS - 1 ) / 2 - 1 );
    int iY    = ( rect.Height( ) - iHt ) / 2;
    iY        = __max( iY, 0 );
    if ( iHt > rect.Height( ) )
        iYadd = rect.Height( ) / ( ( NUM_BTNS - 1 ) / 2 );

    // figure the X button position
    int iXadd = rectBtn.Width( ) + rectBtn.Width( ) / 4;
    int iX    = ( rect.Width( ) - iXadd - rectBtn.Width( ) ) / 2;
    iX        = __max( 0, iX );
    if ( iXadd + rectBtn.Width( ) > rect.Width( ) )
        iXadd = rect.Width( ) / 2;

    // lets position them
    int x = iX, y = iY;
    for ( int iOn = 0; iOn < NUM_BTNS; iOn++ )
    {
        GetDlgItem( aiBtns[iOn] )->SetWindowPos( NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
        if ( iOn == NUM_BTNS / 2 - 1 )
        {
            x += iXadd;
            y = iY;
        }
        else if ( iOn < NUM_BTNS - 2 )
            y += iYadd;
        else
        {
            CRect rReg, rMe;
            GetDlgItem( aiBtns[0] )->GetClientRect( &rReg );
            GetDlgItem( aiBtns[iOn] )->GetClientRect( &rMe );
            x += rReg.Width( ) - rMe.Width( );
        }
    }
}

void CDlgMain::UpdateBlk( )
{

    // background
    CRect rWall;
    GetClientRect( &rWall );
    m_pcdibWall->StretchBlt( m_pcdibTmp, rWall, m_pcdibWall->GetRect( ) );

    // walk the buttons
    for ( int iInd = 0; iInd < NUM_BTNS; iInd++ )
    {
        CButton* pWnd = (CButton*)GetDlgItem( _btnData[iInd].ID );
        if ( !pWnd )
            continue;

        // up/down/disabled
        CRect rectSrc( m_pcdibBtns[iInd]->GetRect( ) );
        int   iWid    = rectSrc.Width( ) / 3;
        rectSrc.right = rectSrc.left + iWid;
        if ( !pWnd->IsWindowEnabled( ) )
            rectSrc.OffsetRect( rectSrc.Width( ) * 2, 0 );
        else if ( pWnd->GetState( ) & 0x04 )
            rectSrc.OffsetRect( rectSrc.Width( ), 0 );

        // get where to put it
        CRect rectDest;
        pWnd->GetClientRect( &rectDest );
        pWnd->MapWindowPoints( this, &rectDest );
        m_pcdibBtns[iInd]->StretchTranBlt( m_pcdibTmp, rectDest, rectSrc );
    }
}

void CDlgMain::OnPaint( )
{

    CPaintDC dc( this );  // device context for painting

    thePal.Paint( dc.m_hDC );

    dc.SetBkMode( TRANSPARENT );

    CRect rect;
    GetClientRect( &rect );

    if ( !m_bTile )
    {
        UpdateBlk( );
        m_pcdibTmp->StretchBlt( dc, rect, m_pcdibTmp->GetRect( ) );
    }
    else
        m_pcdibWall->Tile( dc, rect );

    UINT dtFmt;
    int  iWid = rect.Width( );
    int  iJmp;
    if ( !m_bTile )
    {
        rect.right = ( 5 * ( rect.left + rect.Width( ) ) ) / 8;
        rect.left += rect.Width( ) / 8;
        dtFmt = DT_LEFT | DT_TOP | DT_WORDBREAK;
        iJmp  = 2;
    }
    else
    {
        dtFmt = DT_CENTER | DT_SINGLELINE | DT_TOP;
        iJmp  = 4;
    }

    // put up the title
    CString sTitle;
    sTitle.LoadString( IDS_MAIN_TITLE );
    LOGFONT lf;
    memset( &lf, 0, sizeof( lf ) );
    lf.lfWidth  = ( 3 * ( iWid / sTitle.GetLength( ) ) ) / 4;
    lf.lfHeight = lf.lfWidth * 2;
    lf.lfWeight = 800;
    strcpy( lf.lfFaceName, "Book Antiqua" );
    CFont fnt;
    fnt.CreateFontIndirect( &lf );
    CFont* pOld = dc.SelectObject( &fnt );

    int iShift = lf.lfWidth / 30;
    rect.top   = lf.lfHeight / 2 + iShift;
    rect.left += iShift * iJmp;
    dc.SetTextColor( PALETTERGB( 144, 127, 116 ) );
    while ( iShift-- )
    {
        dc.DrawText( sTitle, -1, &rect, dtFmt );
        rect.top--;
        rect.left -= iJmp;
    }

    dc.SetTextColor( PALETTERGB( 90, 74, 57 ) );
    dc.DrawText( sTitle, -1, &rect, dtFmt );

    rect.top = 0;
    dc.SelectObject( pOld );
    fnt.DeleteObject( );
    dc.SetTextColor( PALETTERGB( 255, 255, 255 ) );

#ifdef _CHEAT
    CString sVer( "Version: " VER_STRING );
#ifdef _DEBUG
    sVer += " (debug, cheat)";
#else
    sVer += " (cheat)";
#endif
    TEXTMETRIC tm;
    dc.GetTextMetrics( &tm );
    dc.TextOut( tm.tmAveCharWidth, theApp.m_iScrnY - tm.tmHeight, sVer );
#endif

    // put up copyright
    CString sCopy;
    sCopy.LoadString( IDS_COPYRIGHT );
    GetClientRect( &rect );
    dc.DrawText( sCopy, -1, &rect, DT_CALCRECT | DT_CENTER | DT_SINGLELINE | DT_TOP );
    int iHt     = rect.Height( );
    rect.top    = theApp.m_iScrnY - iHt - iHt / 2;
    rect.bottom = theApp.m_iScrnY;
    rect.left   = theApp.m_iScrnX - rect.Width( ) - iHt / 2;
    rect.right  = theApp.m_iScrnX;
    dc.DrawText( sCopy, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_TOP );

    thePal.EndPaint( dc.m_hDC );
    // Do not call CDialog::OnPaint() for painting messages
}

void CDlgMain::OnDrawItem( int, LPDRAWITEMSTRUCT lpDIS )
{

    ASSERT_VALID( this );

    CDC* pDc = CDC::FromHandle( lpDIS->hDC );
    if ( pDc == NULL )
        return;
    CWnd* pWnd = CWnd::FromHandle( lpDIS->hwndItem );
    if ( pWnd == NULL )
        return;

    // set the palette
    thePal.Paint( pDc->m_hDC );

    CRect   rect( lpDIS->rcItem );
    CString sText;
    pWnd->GetWindowText( sText );
    int iInd = 0;  // in case m_bTile

    CRect rPos;
    pWnd->GetClientRect( &rPos );
    pWnd->MapWindowPoints( this, &rPos );

    if ( !m_bTile )
    {
        UpdateBlk( );

        // find it
        for ( iInd = 0; iInd < NUM_BTNS; iInd++ )
            if ( _btnData[iInd].ID == lpDIS->CtlID )
                break;
        if ( _btnData[iInd].ID != lpDIS->CtlID )
        {
            thePal.EndPaint( pDc->m_hDC );
            return;
        }

        rect = _btnData[iInd].rText;

        CRect rWall;
        GetClientRect( &rWall );
        int iWid = m_pcdibBtns[iInd]->GetRect( ).Width( ) / 3;

        // if down we need to shift
        if ( lpDIS->itemState & ODS_SELECTED )
            rect.OffsetRect( ( rPos.Width( ) * ( ( _btnData[iInd].ptDnOff.x - iWid ) - _btnData[iInd].rText.left ) ) /
                                 iWid,
                             ( rPos.Height( ) * ( _btnData[iInd].ptDnOff.y - _btnData[iInd].rText.top ) ) /
                                 m_pcdibBtns[iInd]->GetRect( ).Height( ) );

        // adjust to this resolution
        rect.left   = ( rect.left * rWall.Width( ) ) / m_pcdibWall->GetWidth( );
        rect.top    = ( rect.top * rWall.Height( ) ) / m_pcdibWall->GetHeight( );
        rect.right  = ( rect.right * rWall.Width( ) ) / m_pcdibWall->GetWidth( );
        rect.bottom = ( rect.bottom * rWall.Height( ) ) / m_pcdibWall->GetHeight( );

        // switch to our DC
        thePal.EndPaint( pDc->m_hDC );
        pDc = CDC::FromHandle( m_pcdibTmp->GetDC( ) );
        if ( pDc == NULL )
            return;
        thePal.Paint( pDc->m_hDC );
    }

    // draw buttons
    else
    {
        CBrush brFace, brTop, brBottom;
        // grey if disabled
        brBottom.CreateSolidBrush( PALETTERGB( 38, 46, 49 ) );
        brFace.CreateSolidBrush( PALETTERGB( 70, 86, 82 ) );
        brTop.CreateSolidBrush( PALETTERGB( 103, 127, 121 ) );

        if ( lpDIS->itemState & ODS_SELECTED )
            PaintBevel( *pDc, rect, 6, brBottom, brTop );
        else
            PaintBevel( *pDc, rect, 6, brTop, brBottom );
        rect.InflateRect( -6, -6 );
        pDc->FillRect( &rect, &brFace );
        rect.InflateRect( -6, -4 );
    }

    // text
    pDc->SetBkMode( TRANSPARENT );

    // get font
    LOGFONT lf;
    memset( &lf, 0, sizeof( lf ) );
    lf.lfHeight = ( 5 * rect.Height( ) ) / 4;
    lf.lfWeight = 400;
    strcpy( lf.lfFaceName, "Book Antiqua" );
    CFont fnt;
    fnt.CreateFontIndirect( &lf );
    CFont* pOld = pDc->SelectObject( &fnt );

    // does it fit?
    CRect rFit( rect );
    pDc->DrawText( sText, -1, &rFit, DT_CALCRECT | _btnData[iInd].fmt );
    if ( ( rFit.right > rect.right ) || ( rFit.bottom > rect.bottom ) )
    {
        // make it smaller
        int iHt = lf.lfHeight;
        while ( iHt-- > 10 )
        {
            memset( &lf, 0, sizeof( lf ) );
            lf.lfWeight = 400;
            strcpy( lf.lfFaceName, "Book Antiqua" );
            lf.lfHeight = iHt;
            pDc->SelectObject( pOld );
            fnt.DeleteObject( );
            fnt.CreateFontIndirect( &lf );

            // see if this works
            pDc->SelectObject( &fnt );
            rFit = rect;
            pDc->DrawText( sText, -1, &rFit, DT_CALCRECT | _btnData[iInd].fmt );
            if ( ( rFit.right <= rect.right ) && ( rFit.bottom <= rect.bottom ) )
                break;
        }
    }

    rect.OffsetRect( 1, ( rect.Height( ) - rFit.Height( ) ) / 2 + 1 );
    rect.bottom = rect.top + rFit.Height( ) + 1;
    if ( !m_bTile )
        rect.OffsetRect( rPos.left, rPos.top );

    rect.OffsetRect( 0, 1 );
    pDc->SetTextColor( PALETTERGB( 222, 202, 202 ) );
    for ( int x = 0; x < 2; x++ )
    {
        for ( int y = 0; y < 2; y++ )
        {
            pDc->DrawText( sText, -1, &rect, _btnData[iInd].fmt );
            rect.OffsetRect( 1, 0 );
        }
        rect.OffsetRect( -2, 1 );
    }

    rect.OffsetRect( -2, -4 );
    pDc->SetTextColor( PALETTERGB( 44, 53, 46 ) );
    for ( int x = 0; x < 3; x++ )
    {
        for ( int y = 0; y < 3; y++ )
        {
            pDc->DrawText( sText, -1, &rect, _btnData[iInd].fmt );
            rect.OffsetRect( 1, 0 );
        }
        rect.OffsetRect( -3, 1 );
    }

    if ( !( lpDIS->itemState & ( ODS_GRAYED | ODS_DISABLED ) ) )
    {
        rect.OffsetRect( 2, -1 );
        if ( lpDIS->itemState & ODS_FOCUS )
            pDc->SetTextColor( PALETTERGB( 239, 201, 201 ) );
        else
            pDc->SetTextColor( PALETTERGB( 173, 156, 140 ) );
        pDc->DrawText( sText, -1, &rect, _btnData[iInd].fmt );
    }

    pDc->SelectObject( pOld );

    // BLT to the screen
    if ( !m_bTile )
    {
        CPoint pt( rPos.left, rPos.top );
        if ( m_pcdibTmp->GetDirection( ) == CBLTFormat::DIR_BOTTOMUP )
            pt.y = m_pcdibTmp->GetHeight( ) - rPos.top - ( lpDIS->rcItem.bottom - lpDIS->rcItem.top );
        m_pcdibTmp->BitBlt( lpDIS->hDC, &( lpDIS->rcItem ), pt );
        if ( m_pcdibTmp->IsBitmapSelected( ) )
            m_pcdibTmp->ReleaseDC( );
    }

    thePal.EndPaint( pDc->m_hDC );
}

void CDlgMain::OnPaletteChanged( CWnd* pFocusWnd )
{
    static BOOL bInFunc = FALSE;

    CDialog::OnPaletteChanged( pFocusWnd );

    // Win32s locks up if we do the below code
    if ( iWinType == W32s )
        return;

    // stop infinite recursion
    if ( bInFunc )
        return;
    bInFunc = TRUE;

    CClientDC dc( this );
    int       iRtn = thePal.PalMsg( dc.m_hDC, m_hWnd, WM_PALETTECHANGED, (WPARAM)pFocusWnd->m_hWnd, 0 );

    // invalidate the window
    if ( iRtn )
        InvalidateRect( NULL );

    SendMessage( WM_NCPAINT, 0, 0 );

    bInFunc = FALSE;
}

BOOL CDlgMain::OnQueryNewPalette( )
{

    if ( iWinType == W32s )
        return CDialog::OnQueryNewPalette( );

    CClientDC dc( this );
    thePal.PalMsg( dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0 );

    SendMessage( WM_NCPAINT, 0, 0 );
    return CDialog::OnQueryNewPalette( );
}

void CDlgMain::OnDestroy( )
{

    theApp.m_pdlgMain = NULL;

    delete m_pcdibWall;
    m_pcdibWall = NULL;

    for ( int iInd = 0; iInd < NUM_BTNS; iInd++ )
    {
        delete m_pcdibBtns[iInd];
        m_pcdibBtns[iInd] = NULL;
    }

    delete m_pcdibTmp;
    m_pcdibTmp = NULL;

    CDialog::OnDestroy( );
}

void CDlgMain::OnMainExit( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.DestroyMain( );
    theApp.CloseApp( );
}

void CDlgMain::OnMainScenario( )
{

    ASSERT( theApp.m_pCreateGame == NULL );
    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.m_pCreateGame = new CCreateScenario( );
    theApp.m_pCreateGame->Init( );
}

void CDlgMain::OnMainSingle( )
{

    ASSERT( theApp.m_pCreateGame == NULL );
    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.m_pCreateGame = new CCreateSingle( );
    theApp.m_pCreateGame->Init( );
}

static int iNumTimesNet = 0;
void       CDlgMain::OnMainCreate( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    if ( theApp.IsSecondDisk( ) )
        return;

    if ( ( iWinType == WNT ) && ( iNumTimesNet > 0 ) )
    {
        CDlgMsg dlg;
        dlg.MsgBox( IDS_ERROR_NT_NET_BUG, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NTnetBug" );
        UpdateWindow( );
    }
    iNumTimesNet++;

    ASSERT( theApp.m_pCreateGame == NULL );
    theApp.m_pCreateGame = new CCreateMulti( );
    theApp.m_pCreateGame->Init( );
}

void CDlgMain::OnMainJoin( )
{

    if ( ( iWinType == WNT ) && ( iNumTimesNet > 0 ) )
    {
        CDlgMsg dlg;
        dlg.MsgBox( IDS_ERROR_NT_NET_BUG, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NTnetBug" );
        UpdateWindow( );
    }
    iNumTimesNet++;

    ASSERT( theApp.m_pCreateGame == NULL );
    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.m_pCreateGame = new CJoinMulti( );
    theApp.m_pCreateGame->Init( );
}

void CDlgMain::OnMainLoad( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    if ( ( theApp.IsShareware( ) ) || ( theApp.IsSecondDisk( ) ) )
        return;

    ASSERT( theApp.m_pCreateGame == NULL );
    theApp.m_pCreateGame = new CCreateLoadSingle( );
    theApp.m_pCreateGame->Init( );
}

void CDlgMain::OnMinimize( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.Minimize( );
}

void CDlgMain::OnMainLoadMulti( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    if ( ( theApp.IsShareware( ) ) || ( theApp.IsSecondDisk( ) ) )
        return;

    if ( ( iWinType == WNT ) && ( iNumTimesNet > 0 ) )
    {
        CDlgMsg dlg;
        dlg.MsgBox( IDS_ERROR_NT_NET_BUG, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NTnetBug" );
        UpdateWindow( );
    }
    iNumTimesNet++;

    ASSERT( theApp.m_pCreateGame == NULL );
    theApp.m_pCreateGame = new CCreateLoadMulti( );
    theApp.m_pCreateGame->Init( );
}

void CDlgMain::OnMainCredits( )
{

#ifdef _CHEAT
    if ( theApp.GetProfileInt( "Cheat", "TestAudio", 0 ) == 1 )
    {
        CDlgTestSounds dlg;
        dlg.DoModal( );
        return;
    }
#endif

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.m_wndCredits.Create( );
}

void CDlgMain::OnMainIntro( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    theApp.DestroyMain( );
    theApp.m_wndMovie.AddMovie( "intro.avi" );
    theApp.m_wndMovie.Create( TRUE );
}

void CDlgMain::OnMainOptions( )
{

    theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::button ), SFXPRIORITY::selected_pri );
    CDlgOptions dlg( this );
    dlg.DoModal( );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgVer dialog

CDlgVer::CDlgVer( CWnd* pParent /*=NULL*/ ): CDialog( CDlgVer::IDD, pParent )
{
    //{{AFX_DATA_INIT(CDlgVer)
    m_sVer      = "Version: " VER_STRING;
    m_sOs       = _T("");
    m_sNet      = _T("");
    m_sThunk    = _T("");
    m_sRif      = _T("");
    m_sVideo    = _T("");
    m_sSound    = _T("");
    m_sSoundVer = _T("");
    m_sSpeed    = _T("");
    //}}AFX_DATA_INIT

#ifdef _DEBUG
    m_sVer += " (debug, cheat)";
#else
#ifdef _CHEAT
    m_sVer += " (cheat)";
#endif
#endif
    m_sVer += " - " __DATE__ "  " __TIME__;

    switch ( iWinType )
    {
    case W32s:
        m_sOs = "Win32s";
        break;
    case W95:
        m_sOs = "Windows95";
        break;
    case WNT:
        m_sOs = "Win/NT";
        break;
    default:
        m_sOs = "Unknown";
        break;
    }

    OSVERSIONINFO ovi;
    memset( &ovi, 0, sizeof( ovi ) );
    ovi.dwOSVersionInfoSize = sizeof( ovi );
    if ( GetVersionEx( &ovi ) )
        m_sOs += " " + LongToCString( ovi.dwMajorVersion ) + "." + LongToCString( ovi.dwMinorVersion ) + " (" +
                 LongToCString( ovi.dwBuildNumber ) + ")";
    if ( iWinType == W32s )
    {
        WORD wVer = LOWORD( GetVersion( ) );
        m_sOs += " [Windows " + IntToCString( LOBYTE( wVer ) ) + "." + IntToCString( HIBYTE( wVer ) ) + "]";
    }

    long lVer = CNetApi::GetVersion( );
    m_sNet    = "VDMPlay API " + IntToCString( HIBYTE( HIWORD( lVer ) ) ) + "." +
             IntToCString( LOBYTE( HIWORD( lVer ) ) ) + "." + IntToCString( LOWORD( lVer ) );

    m_sRif = "Data Ver: " + IntToCString( theApp.GetRifVer( ) ) + "." + IntToCString( VER_RIFF );
    if ( theApp.IsShareware( ) )
        m_sRif += " {Shareware}";
    if ( theApp.HaveWAV( ) )
        m_sRif += ", WAV";
    else
        m_sRif += ", MIDI";
    if ( theApp.Have24Bit( ) )
        m_sRif += ", 24-bit";
    else
        m_sRif += ", 8-bit";
    if ( theApp.GetFirstZoom( ) )
        m_sRif += ", Zoom1";
    else
        m_sRif += ", Zoom0";

    // video info
    m_sVideo = "Video: ";
    switch ( ptrthebltformat->GetType( ) )
    {
    case CBLTFormat::DIB_DIRECTDRAW:
        m_sVideo += "DirectDraw";
        break;
    case CBLTFormat::DIB_WING:
        m_sVideo += "WinG";
        break;
    case CBLTFormat::DIB_DIBSECTION:
        m_sVideo += "CreateDIBSection";
        break;
    case CBLTFormat::DIB_MEMORY:
        m_sVideo += "StretchDIBits";
        break;
    }
    m_sVideo += " (";

    switch ( ptrthebltformat->GetDirection( ) )
    {
    case CBLTFormat::DIR_TOPDOWN:
        m_sVideo += "top-down";
        break;
    case CBLTFormat::DIR_BOTTOMUP:
        m_sVideo += "bottom-up";
        break;
    }
    m_sVideo += "), " + IntToCString( ptrthebltformat->GetBitsPerPixel( ) ) + "-bit (" +
                IntToCString( GetSystemMetrics( SM_CXSCREEN ) ) + "x" +
                IntToCString( GetSystemMetrics( SM_CYSCREEN ) ) + "x";
    HDC hdc       = ::GetDC( NULL );
    int iBitDepth = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
    ::ReleaseDC( NULL, hdc );
    m_sVideo += IntToCString( iBitDepth ) + ")";

    // sound info
    m_sSound = "Sound: ";
    switch ( theMusicPlayer.GetMode( ) )
    {
    case CMusicPlayer::MUSIC_MODE::midi_only:
        m_sSound += "MIDI Music";
        break;
    case CMusicPlayer::MUSIC_MODE::mixed:
        m_sSound += "MIDI && Digital Music";
        break;
    case CMusicPlayer::MUSIC_MODE::wav_only:
        m_sSound += "Digital Music";
        break;
    }
    if ( theMusicPlayer.UseDirectSound( ) )
        m_sSound += " (DirectSound)";
    else
        m_sSound += " (MME)";

    if ( ( !theMusicPlayer.MidiOk( ) ) && ( !theMusicPlayer.WavOk( ) ) )
        m_sSound += " {MIDI and WAV drivers failed}";
    else if ( !theMusicPlayer.MidiOk( ) )
        m_sSound += " {MIDI driver failed}";
    else if ( !theMusicPlayer.WavOk( ) )
        m_sSound += " {WAV driver failed}";
    else if ( !theMusicPlayer.IsRunning( ) )
        m_sSound += " {turned off}";

    if ( theMusicPlayer._GetHDig( ) == 0 )
        m_sSoundVer = "MSS: " + CString( theMusicPlayer.GetVersion( ) ) + " {off}";
    else
    {
        char sBuf[130], *psFmt;
        long iRate, iFmt;
        sBuf[0] = 0;
        AIL_digital_configuration( theMusicPlayer._GetHDig( ), &iRate, &iFmt, sBuf );
        switch ( iFmt )
        {
        case DIG_F_MONO_8:
            psFmt = "K/8-bit/Mono, ";
            break;
        case DIG_F_MONO_16:
            psFmt = "K/16-bit/Mono, ";
            break;
        case DIG_F_STEREO_8:
            psFmt = "K/8-bit/Stereo, ";
            break;
        case DIG_F_STEREO_16:
            psFmt = "K/16-bit/Stereo, ";
            break;
        }
        m_sSoundVer =
            "MSS: " + CString( theMusicPlayer.GetVersion( ) ) + " " + IntToCString( iRate ) + CString( psFmt ) + sBuf;
    }

    m_sSpeed = "CPU Speed: ~" + IntToCString( theApp.GetCpuSpeed( ) ) + "  CD-ROM Speed: ~" +
               IntToCString( theApp.GetCdSpeed( ) ) + "X";

    MEMORYSTATUS ms;
    ms.dwLength = sizeof( ms );
    GlobalMemoryStatus( &ms );
    const int ONE_MEG = 1024 * 1024;
    m_sMemory         = "Memory (avail/total) Physical: " + IntToCString( ms.dwAvailPhys / ONE_MEG ) + "M/" +
                IntToCString( ms.dwTotalPhys / ONE_MEG ) +
                "M Virtual: " + IntToCString( ms.dwAvailPageFile / ONE_MEG ) + "M/" +
                IntToCString( ms.dwTotalPageFile / ONE_MEG ) + "M";

    if ( iWinType == W32s )
    {
        WORD wVer = myGetThrdUtlsVersion( );
        m_sThunk  = "Threads DLL " + IntToCString( HIBYTE( wVer ) ) + "." + IntToCString( LOBYTE( wVer ) );
    }
}

void CDlgVer::DoDataExchange( CDataExchange* pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CDlgVer)
    DDX_Text( pDX, IDC_VER_TEXT, m_sVer );
    DDX_Text( pDX, IDC_VER_TEXT_OS, m_sOs );
    DDX_Text( pDX, IDC_VER_NET, m_sNet );
    DDX_Text( pDX, IDC_VER_THUNK, m_sThunk );
    DDX_Text( pDX, IDC_VER_RIF, m_sRif );
    DDX_Text( pDX, IDC_VER_VIDEO, m_sVideo );
    DDX_Text( pDX, IDC_VER_SOUND, m_sSound );
    DDX_Text( pDX, IDC_VER_SOUND2, m_sSoundVer );
    DDX_Text( pDX, IDC_VER_SPEED, m_sSpeed );
    DDX_Text( pDX, IDC_VER_MEMORY, m_sMemory );
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP( CDlgVer, CDialog )
//{{AFX_MSG_MAP(CDlgVer)
ON_BN_CLICKED( IDC_LICENSE, OnLicense )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CDlgVer message handlers

BOOL CDlgVer::OnInitDialog( )
{
    CDialog::OnInitDialog( );

    CenterWindow( theApp.m_pMainWnd );

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDlgVer::OnLicense( )
{
    // TODO: Add your control notification handler code here
    CDlgLicense dlgLic( theApp.IsShareware( ) ? 2 : 3, TRUE );
    dlgLic.DoModal( );
}
/////////////////////////////////////////////////////////////////////////////
// CDlgStackDump dialog


CDlgStackDump::CDlgStackDump( CWnd* pParent /*=NULL*/ ): CDialog( CDlgStackDump::IDD, pParent )
{
    //{{AFX_DATA_INIT(CDlgStackDump)
    m_sText = _T("");
    //}}AFX_DATA_INIT
}


void CDlgStackDump::DoDataExchange( CDataExchange* pDX )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CDlgStackDump)
    DDX_Text( pDX, IDC_STACK_DUMP, m_sText );
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP( CDlgStackDump, CDialog )
//{{AFX_MSG_MAP(CDlgStackDump)
ON_BN_CLICKED( IDC_COPY_STACK, OnCopyStack )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CDlgStackDump message handlers

void CDlgStackDump::OnCopyStack( )
{

    // open the clipboard
    if ( !OpenClipboard( ) )
    {
        MessageBeep( MB_ICONEXCLAMATION );
        return;
    }
    if ( !EmptyClipboard( ) )
    {
        CloseClipboard( );
        MessageBeep( MB_ICONEXCLAMATION );
        return;
    }

    MEMORYSTATUS ms;
    ms.dwLength = sizeof( ms );
    GlobalMemoryStatus( &ms );
    const int ONE_MEG = 1024 * 1024;

    char sNum1[20], sNum2[20];
    itoa( m_pe->m_uEc, sNum1, 16 );
    itoa( (int)m_pe->m_pExCode, sNum2, 16 );
    CString sMsg;
    if ( ms.dwAvailPageFile / ONE_MEG < 8 )
    {
        sMsg.LoadString( IDS_OUT_OF_MEMORY );
        sMsg += "\r\n";
    }
    sMsg += "Sorry, an unknown error occured.\r\nVersion: " + CString( VER_STRING ) + "\r\n" +
            "Type: " + CString( sNum1 ) + ", Address: 0x" + CString( sNum2 ) + "\r\n";
    for ( int iOn = 0; iOn < NUM_EXCEP; iOn++ )
    {
        itoa( m_pe->m_stack[iOn], sNum1, 16 );
        sMsg += "     0x" + CString( sNum1 ) + "\r\n";
    }

    time_t t;
    time( &t );
    struct tm* _now = localtime( &t );
    sMsg += CString( asctime( _now ) );
    sMsg += theApp.m_sOs + "\r\n";
    sMsg += theApp.m_sNet + "\r\n";

    sMsg += "Memory (avail/total) Physical: " + IntToCString( ms.dwAvailPhys / ONE_MEG ) + "M/" +
            IntToCString( ms.dwTotalPhys / ONE_MEG ) + "M Virtual: " + IntToCString( ms.dwAvailPageFile / ONE_MEG ) +
            "M/" + IntToCString( ms.dwTotalPageFile / ONE_MEG ) + "M\r\n";
    sMsg += theApp.m_sRif + "\r\n";
    sMsg += theApp.m_sVideo + "\r\n";
    sMsg += theApp.m_sSound + "\r\n";
    sMsg += theApp.m_sSoundVer + "\r\n";
    sMsg += theApp.m_sSpeed + "\r\n";

    // set the data
    int     iLen = sMsg.GetLength( ) + 1;
    HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, iLen + 1 );
    if ( hMem != NULL )
    {
        void* pData = GlobalLock( hMem );
        if ( pData != NULL )
        {
            memcpy( pData, (char const*)sMsg, iLen );
            GlobalUnlock( hMem );
            SetClipboardData( CF_TEXT, hMem );
        }
    }

    CloseClipboard( );
}
