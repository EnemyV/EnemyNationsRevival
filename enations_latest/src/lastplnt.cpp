//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "CdLoc.h"
#include "cpufeatures.h"   // EnCpu — runtime ISA detection/dispatch (GH #8)
#ifndef _WIN32
#include <fcntl.h>     // open  — POSIX single-instance flock guard (see FindWindow site)
#include <unistd.h>    // close
#include <sys/file.h>  // flock
#endif
#include "GameWindow.h"
#include "en_harness.h"   // in-process LLM-driving harness (all platforms; EN_HARNESS-gated)
#include "SDL2Compositor.h"
#include "SDL2Video.h"
#include "SDL2MainMenu.h"
#include "SDL2GameDialogs.h"
#include "SDL2Options.h"
#include "w22_settings.h"
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
#include "racedata.h"
#include "scenario.h"
#include "sfx.h"
#include "sprite.h"
#include "stdafx.h"
#include "terrain.inl"

#include <processenv.h>

#include <ctype.h>
#include <locale.h>
#include <new.h>
#include <windward.h>

#ifndef __INCLUDE_THIS_LAST__
#include "lastplnt.h"
#endif
#include "cpuspeed.hpp"


#ifdef _DEBUG

#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW




extern HANDLE hRenderEvent;
extern BOOL   bDoSubclass;


std::string      GetDefaultApp( char const* pExt, char const* pDef, char const* pCmdLine );
LRESULT CALLBACK PerBarProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


static int aiBtns[] = { IDC_MAIN_CAMPAIGN, IDC_MAIN_SINGLE,   IDC_MAIN_CREATE, IDC_MAIN_JOIN,
                        IDC_MAIN_LOAD,     IDC_MAIN_LOAD_MUL, IDC_MAIN_INTRO,  IDC_MAIN_CREDITS,
                        IDC_MAIN_OPTIONS,  IDCANCEL,          IDC_MINIMIZE };
const int  NUM_BTNS = sizeof( aiBtns ) / sizeof( int );


#ifdef _CHEAT
BOOL _bShowRate       = FALSE;
BOOL _bClickAny       = FALSE;
BOOL _bMaxMaterials   = FALSE;
BOOL _bMaxRocket      = FALSE;
BOOL _bMaxPower       = FALSE;
BOOL _iFrameRate      = 1;
BOOL _bShowWorld      = FALSE;
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
#ifdef _WIN64
    DWORD* pCall = (DWORD*)pExp->ContextRecord->Rsp;   // x64 stack pointer (Esp is 32-bit only)
#else
    DWORD* pCall = (DWORD*)pExp->ContextRecord->Esp;
#endif
    for ( int iInd = 0; iInd < NUM_EXCEP; pCall++ )
    {
        // if the pointer is bad - we're done (we don't write but the stack is writeable)
        if ( IsBadWritePtr( pCall, 4 ) )
            break;

        // if it's code we save it (write because badcode only checks if can read)
        if ( ( IsBadWritePtr( (void*)(uintptr_t)*pCall, 4 ) ) && ( !IsBadCodePtr( (FARPROC)(uintptr_t)*pCall ) ) )
            if ( ( iInd < 3 ) || ( ( *pCall & 0x80000000 ) == 0 ) )
                exp.m_stack[iInd++] = *pCall;
    }

    throw exp;
}

void CatchNum( int iNum )
{
    fprintf( stderr, "[CatchNum] game exception %d - shutting the game down\n", iNum );

    bDoSubclass = FALSE;

    // turn the game off
    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);
    theGame.SetShouldProcessMessages(FALSE);
    theGame.EmptyQueue( );

    // no message if quitting
    if ( iNum == ERR_TLP_QUIT )
        return;

    char sNum[20];
    if ( iNum >= ERR_BASE_USER_ERROR )
        iNum -= ERR_BASE_USER_ERROR;
    else
        iNum += 100;
    itoa( iNum, sNum, 10 );
    std::string sMsg = strPrintf( EnLoadStdString( IDS_ERR_LOAD_1 ).c_str(), sNum );
    EnMessageBox( sMsg.c_str(), MB_OK | MB_ICONSTOP );

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

    std::string sDumpText = EnLoadStdString( IDS_ERR_LOAD_3 );

    MEMORYSTATUS ms;
    ms.dwLength = sizeof( ms );
    GlobalMemoryStatus( &ms );
    const int ONE_MEG = 1024 * 1024;
    if ( ms.dwAvailPageFile / ONE_MEG < 8 )
    {
        sDumpText = EnLoadStdString( IDS_OUT_OF_MEMORY ) + "\r\n" + sDumpText;
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
        itoa( (int)(intptr_t)e.m_pExCode, sNum2, 16 );   // NT exception codes fit 32 bits (display only)
        break;
    }
    for ( int iOn = 0; iOn < 5; iOn++ ) itoa( e.m_stack[iOn], sNumS[iOn], 16 );
    sDumpText = strPrintf( sDumpText.c_str(), VER_STRING, sNum1, sNum2,
                           sNumS[0], sNumS[1], sNumS[2], sNumS[3], sNumS[4] );
    ::MessageBoxA( NULL, sDumpText.c_str(), "Enemy Nations - Exception", MB_OK | MB_ICONSTOP );

    bDoSubclass = TRUE;
}

#include <exception>
#include <typeinfo>

// Name the in-flight exception. Valid while a catch(...) handler is running, which
// is the only place CatchOther is called from. A bare "Unknown error" told a tester
// (and us) nothing; bad_alloc vs a game int-code vs something else are different bugs.
static std::string DescribeCurrentException( )
{
    std::exception_ptr p = std::current_exception( );
    if ( !p )
        return "no active exception";
    try
    {
        std::rethrow_exception( p );
    }
    catch ( const std::bad_alloc& e )   { return std::string( "std::bad_alloc: " ) + e.what( ); }
    catch ( const std::exception& e )   { return std::string( typeid( e ).name( ) ) + ": " + e.what( ); }
    catch ( int i )                     { return "int " + IntToStr( i ); }
    // Raw `throw( ERR_* )` sites (world.cpp CreateEx, area.cpp, the CAI code) throw the
    // ENUM type, which catch(int) does not catch — these were the anonymous "Unknown error".
    catch ( Error e )                   { const char* s = GetWind22ErrString( e );
                                          return ( s && s[0] ? std::string( s ) : "wind22 error" ) +
                                                 " (" + IntToStr( (int)e ) + ")"; }
    catch ( GameError e )               { return "game error " + IntToStr( (int)e - (int)ERR_BASE_USER_ERROR ); }
    catch ( ... )                       { return "non-standard exception type"; }
}

void CatchOther( char const* pContext )
{
    std::string sWhat = DescribeCurrentException( );

    fprintf( stderr, "[CatchOther] unknown game exception (%s) - shutting the game down\n", sWhat.c_str( ) );

    bDoSubclass = FALSE;

    // turn the game off
    theGame.SetShouldAnimate(FALSE);
    theGame.SetShouldOperate(FALSE);
    theGame.SetShouldProcessMessages(FALSE);
    theGame.EmptyQueue( );

    // Headline stays the shipped string; the detail lines are what a bug report needs.
    std::string sMsg = EnLoadStdString( IDS_ERR_LOAD_2 );
    sMsg += "\r\n\r\nException: " + sWhat;
    if ( pContext && *pContext )
        sMsg += "\r\n" + std::string( pContext );
    sMsg += "\r\nVersion: " + std::string( VER_STRING );

    OutputDebugStringA( ( "[CatchOther] " + sMsg + "\n" ).c_str( ) );
    EnMessageBox( sMsg.c_str( ), MB_OK | MB_ICONSTOP );

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
        ::GetWindowTextA( hWnd, sText, 19 );
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
    m_bWorldTearingDown = FALSE;
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

    // Phase 4c prep: HFONT cleanup (was automatic via CFont dtor)
    if ( m_Fnt )     { ::DeleteObject( m_Fnt );     m_Fnt = NULL; }
    if ( m_FntRD )   { ::DeleteObject( m_FntRD );   m_FntRD = NULL; }
    if ( m_FntDesc ) { ::DeleteObject( m_FntDesc ); m_FntDesc = NULL; }
    if ( m_FntCost ) { ::DeleteObject( m_FntCost ); m_FntCost = NULL; }
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CConquerApp object

CConquerApp theApp;


int _excep_new_handler( size_t size )
{
    // Log the failed allocation. The size is the SINGLE request that exceeded
    // available address space — for a 32-bit process this is almost always a
    // corrupted count read from a save / archive (e.g. 0xFFFFFFFF interpreted
    // as a length). Capturing the size and a stack snapshot here makes it
    // possible to pinpoint the bad reader instead of guessing.
    char head[256];
    _snprintf_s( head, sizeof( head ), _TRUNCATE,
                 "[bad_alloc] requested %zu bytes (%.2f MB)",
                 size, (double)size / (1024.0 * 1024.0) );
    ::OutputDebugStringA( head );
    ::OutputDebugStringA( "\n" );
    theApp.Log( head );

#ifdef _WIN32
    // Brief stack snapshot — symbols only resolve if dbghelp + PDB present,
    // but raw module+offset is still searchable in the map file.
    void*  frames[ 24 ];
    USHORT count = ::CaptureStackBackTrace( 1, 24, frames, nullptr );
    for ( USHORT i = 0; i < count; ++i )
    {
        HMODULE hMod = NULL;
        char line[ 256 ];
        if ( ::GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCSTR)frames[ i ], &hMod ) && hMod )
        {
            char modPath[ MAX_PATH ] = { 0 };
            ::GetModuleFileNameA( hMod, modPath, sizeof( modPath ) );
            const char* base = strrchr( modPath, '\\' );
            base = base ? base + 1 : modPath;
            uintptr_t off = (uintptr_t)frames[ i ] - (uintptr_t)hMod;
            _snprintf_s( line, sizeof( line ), _TRUNCATE,
                         "  [%02u] %p  %s+0x%zX", i, frames[ i ], base, (size_t)off );
        }
        else
        {
            _snprintf_s( line, sizeof( line ), _TRUNCATE,
                         "  [%02u] %p", i, frames[ i ] );
        }
        ::OutputDebugStringA( line );
        ::OutputDebugStringA( "\n" );
        theApp.Log( line );
    }
#endif

    // Show the size in the message box so users can flag huge requests
    // (e.g. 4 GB) immediately instead of just seeing a generic OOM.
    char msg[ 512 ];
    std::string stdMsg = EnLoadStdString( IDS_NO_MEMORY );
    _snprintf_s( msg, sizeof( msg ), _TRUNCATE,
                 "%s\n\n(Requested %zu bytes / %.2f MB. See log for stack.)",
                 stdMsg.c_str(), size, (double)size / (1024.0 * 1024.0) );
    EnMessageBox( msg, MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP );
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

    fwrite( pText, 1, iLen, m_pLogFile );
    fwrite( "\r\n", 1, 2, m_pLogFile );

    fflush( m_pLogFile );
}

BOOL CConquerApp::InitInstance( )
{
#ifdef LOGGINGON
    OutputDebugStringA( "InitInstance\n" );
#endif

    // Wire the "locate the data file" picker. wind22 throws ERR_DATAFILE_OPEN when
    // it cannot open the .dat and retries with whatever this returns. Registered
    // here because wind22 sits below the game UI and must not include it.
    // NOTE: this runs at theDataFile.Init (~line 922) but m_gameWindow is not
    // created until ~1495, so an SDL2 dialog is not available yet. Use the OS
    // picker, which needs no game state.
    SetLocateDataFileHandler( []( const char* pszWanted, char* pszOut, int cbOut ) -> bool
    {
#ifdef _WIN32
        // Ask first: an unattended/harness run must not block on a modal dialog.
        CString sMsg;
        sMsg.Format( "ENations.dat could not be found.\n\nLooked for:\n%s\n\n"
                     "Locate it now?", pszWanted ? pszWanted : "" );
        if ( ::MessageBoxA( NULL, sMsg, "Enemy Nations", MB_YESNO | MB_ICONWARNING ) != IDYES )
            return false;

        char szFile[ 1024 ] = { 0 };
        strcpy( szFile, "ENations.dat" );
        OPENFILENAMEA ofn = { 0 };
        ofn.lStructSize = sizeof( ofn );
        ofn.hwndOwner   = NULL;
        ofn.lpstrFilter = "Enemy Nations data (ENations.dat)\0ENations.dat\0"
                          "Data files (*.dat)\0*.dat\0All files (*.*)\0*.*\0";
        ofn.lpstrFile   = szFile;
        ofn.nMaxFile    = sizeof( szFile );
        ofn.lpstrTitle  = "Locate ENations.dat";
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if ( !::GetOpenFileNameA( &ofn ) )
            return false;
        if ( !szFile[ 0 ] || (int)strlen( szFile ) >= cbOut )
            return false;
        strcpy( pszOut, szFile );
        return true;
#else
        // POSIX has no OS picker we can call this early (SDL2 has no file dialog
        // and the game window does not exist yet). Returning false leaves the
        // clean "could not open" failure + log line. Needs zenity/kdialog on
        // Linux and NSOpenPanel on mac.
        (void)pszWanted; (void)pszOut; (void)cbOut;
        return false;
#endif
    } );

    // FIRST thing, before anything can fail: record what this CPU supports.
    // GH #8 was a silent illegal-instruction death on pre-AVX2 hardware with
    // nothing in any log to say so — the user saw the splash and then nothing.
    // Logging the ISA level here means the next "it just closes" report arrives
    // with the one fact needed to classify it. Also primes the EnCpu cache
    // before any dispatch can consult it.
    EnCpu::LogFeatures( );

    // Set application name first (used by SetRegistryKey internally)
    m_pszAppName = _tcsdup( _T("Second Chance") );
    // this redirects profile strings to HKEY_CURRENT_USER
    SetRegistryKey( _T("Second Chance") );

    InitWindwardLib1( this );

    


    EnWriteProfileString( "ADPCM", "Error", "OK" );

    if ( EnGetProfileInt( "Advanced", "Log", 0 ) )
    {
        EnMessageBox( IDS_EN_LOGGING, MB_OK | MB_ICONINFORMATION );
        std::string sName = EnGetProfileStdString( "Advanced", "LogName", GameLogFile );
        m_pLogFile = fopen( sName.c_str(), "wb" );  // Phase 4c prep — replaces CFile
        if ( m_pLogFile != NULL )
        {
            time_t t;
            time( &t );
            struct tm* _now = localtime( &t );
            Log( asctime( _now ) );
            Log( VER_STRING );
        }
    }
    else if ( ::GetPrivateProfileInt( "vdmplay", "UseLogFile", 0, "vdmplay.ini" ) )
        EnMessageBox( IDS_VP_LOGGING, MB_OK | MB_ICONINFORMATION );

    EnWriteProfileString( "Advanced", "Version", VER_STRING );

    // over-ride default event method
    m_bUseEvents  = EnGetProfileInt( "Advanced", "Events", m_bUseEvents );
    m_bPauseOnAct = EnGetProfileInt( "Advanced", "Pause", TRUE );

    // load the correct language
    m_iLangCode  = EnGetProfileInt( "Advanced", "Language", PRIMARYLANGID( LANGIDFROMLCID( ::GetUserDefaultLCID( ) ) ) );
    std::string sLib = "ENLang" + IntToStr( m_iLangCode ) + ".DLL";
    if ( ( m_hLibLang = LoadLibrary( sLib.c_str() ) ) != NULL )
        AfxSetResourceHandle( m_hLibLang );

    // init critical section (before maybe exiting below)
    memset( &cs, 0, sizeof( cs ) );
    InitializeCriticalSection( &cs );
    hRenderEvent = CreateEvent( NULL, TRUE, FALSE, "RenderEvent" );

    m_sAppName = EnLoadStdString( IDS_MAIN_TITLE );

    // Get CPU Speed
    CPUInfo cpu;
    double  mhz = cpu.get_cpu_mhz( );

    // get the CPU speed (needed before screen res)
    m_iCpuSpeed = mhz;

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
            int iRes     = EnGetProfileInt( "Advanced", "ScreenResolution", 0 );
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
                std::string sRes = IntToStr( m_iOldWidth ) + "x" + IntToStr( m_iOldHeight ) + "x" +
                                   IntToStr( m_iOldDepth );
                std::string sMsg = strPrintf( EnLoadStdString( IDS_KILLER_RES ).c_str(),
                                              sRes.c_str(), pRes );
                if ( EnMessageBoxOnce( sMsg.c_str(), MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "ScreenRes", IDNO ) == IDYES )
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
    // Route the main window class through CWndStub::StaticWndProc so
    // CWndMain::OnCreate / OnPaint / OnEraseBkgnd / etc. virtuals actually fire.
    wc.lpfnWndProc   = &CWndStub::StaticWndProc;
    wc.hInstance     = ::GetModuleHandle( NULL );  // Phase 4c prep: was AfxGetInstanceHandle
    wc.hIcon         = LoadIcon( IDI_MAIN );
    wc.hCursor       = LoadStandardCursor( IDC_ARROW );
    wc.lpszClassName = m_sClsName.c_str();
    if ( !::RegisterClass( &wc ) ) {  // Phase 4c prep: was AfxRegisterClass
        return FALSE;
    }

    HWND hPrevWnd = ::FindWindow( m_sClsName.c_str(), m_sAppName.c_str() );
#ifndef _WIN32
    // POSIX single-instance guard. FindWindow is a NULL stub on POSIX (win32_compat.h),
    // so the Win check never fires and repeated launches stack live clients — which
    // corrupts MP sessions (ghost players, contradictory inputs, append-mode log
    // contamination) and invalidates every smoke a POSIX node joins. This is a release
    // gate (LinuxOpus root-cause f70a02b). Hold an advisory flock for our whole lifetime:
    // the FIRST instance acquires it; a second launch cannot and is treated exactly like
    // hPrevWnd != NULL, so the existing IDS_MULT_INST prompt fires. The lock releases
    // automatically on exit OR crash (kernel drops the fd) — no stale-PID file to clean.
    // EN_ALLOW_MULTI=1 bypasses it for dev/harness scenarios that intentionally run two.
    {
        const char* allowMulti = getenv( "EN_ALLOW_MULTI" );
        if ( !( allowMulti && allowMulti[0] == '1' ) )
        {
            static int s_singletonFd = -1;   // held for the whole process lifetime
            s_singletonFd = ::open( "/tmp/.enations.singleton.lock", O_CREAT | O_RDWR, 0644 );
            if ( s_singletonFd >= 0 && ::flock( s_singletonFd, LOCK_EX | LOCK_NB ) != 0 )
            {
                ::close( s_singletonFd );
                s_singletonFd = -1;
                fprintf( stderr, "[singleton] another Second Chance instance already holds "
                                 "the lock — this launch is a second copy\n" );
                hPrevWnd = (HWND)1;   // non-NULL sentinel -> IDS_MULT_INST prompt below
            }
        }
    }
#endif
    if ( hPrevWnd != NULL )
        if ( EnMessageBox( IDS_MULT_INST, MB_YESNO | MB_ICONQUESTION ) == IDYES )
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

    // Tell wind22 about our main window so it doesn't need ptheApp->m_pMainWnd
    w22::SetMainHWND( m_wndMain.m_hWnd );

    Log( "Initialize windward.lib" );

    // set it up
    if ( !InitWindwardLib2( ) )
        return ( FALSE );

    if ( CNetApi::GetVersion( ) < 0x01000021 )
    {
        EnMessageBox( IDS_VDMPLAY_VER, MB_OK | MB_ICONSTOP );
        return ( FALSE );
    }
    // SDL_mixer version (MSS version check removed)

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
    char msg[256];
    snprintf( msg, sizeof(msg), "OS Detected: %s\n", m_sOs.c_str() );
    OutputDebugStringA( msg );
#endif

    OSVERSIONINFO ovi;
    memset( &ovi, 0, sizeof( ovi ) );
    ovi.dwOSVersionInfoSize = sizeof( ovi );
    GetVersionEx( &ovi );
    m_sOs += " " + LongToStr( ovi.dwMajorVersion ) + "." + LongToStr( ovi.dwMinorVersion ) + " (";
    if ( ( ovi.dwBuildNumber & 0xFFFF0000 ) == 0 )
        m_sOs += LongToStr( ovi.dwBuildNumber ) + ")";
    else
        m_sOs += LongToStr( ovi.dwBuildNumber >> 16 ) + "," + LongToStr( ovi.dwBuildNumber & 0xFFFF ) + ")";
    if ( iWinType == W32s )
    {
        WORD wVer = LOWORD( GetVersion( ) );
        m_sOs += " [Windows " + IntToStr( LOBYTE( wVer ) ) + "." + IntToStr( HIBYTE( wVer ) ) + "]";
    }
    Log( m_sOs.c_str( ) );

    long lVer = CNetApi::GetVersion( );
    m_sNet    = "VDMPlay API " + IntToStr( HIBYTE( HIWORD( lVer ) ) ) + "." +
             IntToStr( LOBYTE( HIWORD( lVer ) ) ) + "." + IntToStr( LOWORD( lVer ) );
    Log( m_sNet.c_str( ) );

    MEMORYSTATUS ms;
    ms.dwLength = sizeof( ms );
    GlobalMemoryStatus( &ms );
    const int ONE_MEG   = 1024 * 1024;
    std::string sMemory = "Memory (avail/total) Physical: " + IntToStr( ms.dwAvailPhys / ONE_MEG ) + "M/" +
                          IntToStr( ms.dwTotalPhys / ONE_MEG ) +
                          "M Virtual: " + IntToStr( ms.dwAvailPageFile / ONE_MEG ) + "M/" +
                          IntToStr( ms.dwTotalPageFile / ONE_MEG ) + "M";
    Log( sMemory.c_str() );

    // enough memory?
    // need 8M system
    if ( ms.dwTotalPhys < 1024 * 1024 * 7 )
    {
        if ( EnMessageBoxOnce( IDS_ERROR_LOW_PHYS_MEM, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThan8Meg" ) !=
             IDYES )
            return ( 0 );
        Log( "Error: Not enough physical memory to run" );
        m_wndMain.UpdateWindow( );
    }
    if ( ms.dwTotalPageFile < 1024 * 1024 * MEM_NEEDED_BASE )
    {
        std::string sNum = IntToStr( MEM_NEEDED_BASE );
        std::string sText = strPrintf( EnLoadStdString( IDS_ERROR_LOW_VIRT_MEM ).c_str(), sNum.c_str() );
        if ( EnMessageBoxOnce( sText.c_str(), MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThan8Meg" ) != IDYES )
            return ( 0 );
        Log( "Error: Not enough virtual memory to run" );
        m_wndMain.UpdateWindow( );
    }
    if ( ( ms.dwTotalPhys < 1024 * 1024 * 7 ) || ( ms.dwAvailPageFile < 1024 * 1024 * ( MEM_NEEDED_BASE - 10 ) ) )
    {
        if ( EnMessageBoxOnce( IDS_ERROR_LOW_AVAIL_MEM, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
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

    switch ( EnGetProfileInt( "Advanced", "ColorDepth", 0 ) )
    {
    case 2:
        m_bUse8Bit = FALSE;
        if ( iBytesPerPixel == 1 )
            break;
        if ( bForce8 )
        {
            if ( EnMessageBoxOnce( IDS_ERROR_LOW_COLOR_DEPTH, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
                             "LessThanColorDepth" ) == IDYES )
                m_bUse8Bit = TRUE;
            m_wndMain.UpdateWindow( );
        }
        else if ( m_iCpuSpeed <= 200 )
        {
            if ( EnMessageBoxOnce( IDS_ERROR_LOW_COLOR_DEPTH2, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
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

    // Zoom Levels (SDL2 Advanced Options dialog): 0 = "All 4 levels" (enable the closest zoom,
    // m_iFirstZoom=0), 1 = "3 levels", 2 = "2 levels" (both start at zoom 1). The legacy MFC dialog
    // used DIFFERENT values (1 = "force zoom 0"); that dialog is gone, so honor the SDL2 dialog's
    // semantics directly — this is why picking a zoom level "wasn't respected". "All 4" still respects
    // the per-bit-depth memory budget (bUse0) so a genuinely low-memory box gracefully drops to 3. The
    // obsolete P/200 CPU gate is dropped (no modern machine trips it; Linux CPU-speed detect is flaky).
    switch ( EnGetProfileInt( "Advanced", "Zoom", 0 ) )
    {
    case 0:   // All 4 levels
        m_bUseZoom0 = bUse0;
        if ( !bUse0 )
        {
            if ( EnMessageBoxOnce( IDS_ERROR_LOW_ZOOM, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "LessThanZoom" ) ==
                 IDYES )
                m_bUseZoom0 = FALSE;
            m_wndMain.UpdateWindow( );
        }
        break;

    default:  // "3 levels" / "2 levels" -> start at zoom 1
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
        std::string sName;
        pMmio->AscendChunk( );
        pMmio->DescendChunk( 'N', 'A', 'M', 'E' );
        pMmio->ReadString( sName );

        delete pMmio;

        bErr = FALSE;
        if ( ( m_iRifVer != VER_RIFF ) || ( sName != GameDataName ) )
        {
            TRAP( );
            std::string sNum1 = IntToStr( m_iRifVer );
            std::string sNum2 = IntToStr( VER_RIFF );
            // IDS_WRONG_DATA_FILE has 4 positional placeholders (%1=actualName,
            // %2=actualVer, %3=expectedName, %4=expectedVer). The legacy code
            // passed only 3 args, which made strPrintf walk past the va_list
            // tail and dereference garbage when this branch fired.
            std::string sMsg = strPrintf( EnLoadStdString( IDS_WRONG_DATA_FILE ).c_str(),
                                          sName.c_str(), sNum1.c_str(),
                                          GameDataName, sNum2.c_str() );
            if ( EnMessageBox( sMsg.c_str(), MB_YESNO | MB_ICONSTOP ) != IDYES )
                return ( 0 );
            bErr = TRUE;
        }
    } while ( bErr );

    // if .dat < 400M then it's shareware (anti-pirate) — Phase 5c: Win32 file-size check
    {
        WIN32_FILE_ATTRIBUTE_DATA wfad;
        if ( ::GetFileAttributesExA( theDataFile.GetName(), GetFileExInfoStandard, &wfad ) )
        {
            ULARGE_INTEGER size;
            size.HighPart = wfad.nFileSizeHigh;
            size.LowPart  = wfad.nFileSizeLow;
            if ( size.QuadPart < 400000000ULL )
                m_bShareware = TRUE;
        }
    }

    // warn on 16-bit
    if ( !m_bUse8Bit )
    {
        HDC hdc    = GetDC( NULL );
        int iDepth = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
        ReleaseDC( NULL, hdc );
        if ( iDepth == 16 )
        {
            EnMessageBoxOnce( IDS_ERROR_16_BIT_WARNING, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "16bitWarning" );
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
                EnMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            // fix for bug in old setup
            RegSetValueEx( key, NULL, NULL, REG_SZ, (unsigned char*)"4", 2 );

            unsigned long iLen = 256;
            DWORD         dwTyp, dwLen = sizeof( DWORD );
            // The registry value is a REG_DWORD (4 bytes). It MUST be read into a
            // 32-bit variable: on LP64 (macOS/Linux) `time_t` is 8 bytes, so
            // reading 4 bytes into a time_t leaves the high half uninitialized and
            // `dwTime == 41` / the date math below operate on garbage. (On the
            // original 32-bit Windows build time_t happened to be 4 bytes.)
            DWORD         dwTime = 0;
            if ( RegQueryValueEx( key, "CD-ROM", NULL, &dwTyp, (unsigned char*)&dwTime, &dwLen ) != ERROR_SUCCESS )
            {
                TRAP( );
                Log( "Expired 2" );
                EnMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            if ( dwTyp != REG_DWORD )
            {
                TRAP( );
                Log( "Expired 3" );
                EnMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
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
                time_t tInstalled = (time_t)dwTime;
                char   sBuf[64];
                strftime( sBuf, sizeof(sBuf), "Installed: %x", localtime( &tInstalled ) );
                Log( sBuf );
                EnMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
                return ( 0 );
            }

            RegCloseKey( key );
        }

        else
        {
            time_t iTime = ::GetProfileIntA( "DOS Emulation", "_COMM", -1 );
            if ( iTime == -1 )
            {
                TRAP( );
                char sBuf[20];
                itoa( iToday + i1Month, sBuf, 10 );
                ::WriteProfileStringA( "DOS Emulation", "_COMM", sBuf );
            }
            else
                // is it earlier (ie did they advance the date before installing)?
                if ( iToday < (int)iTime )
            {
                iToday -= i1Month / 2;
                char sBuf[20];
                itoa( iToday + i1Month, sBuf, 10 );
                ::WriteProfileStringA( "DOS Emulation", "_COMM", sBuf );
            }
            else if ( iToday > (int)iTime + i1Month )
            {
                TRAP( );
                char sBuf[64];
                strftime( sBuf, sizeof(sBuf), "Installed: %x", localtime( &iTime ) );
                Log( sBuf );
                EnMessageBox( IDS_DEMO_OVER, MB_OK | MB_ICONSTOP );
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
        int iTry = EnGetProfileInt( "Game", "NumDemo", 1 );
        EnWriteProfileInt( "Game", "NumDemo", iTry + 1 );
        if ( ( iTry % 25 ) == 0 )
        {
            std::string sNum = IntToStr( iTry );
            std::string sMsg = strPrintf( EnLoadStdString( IDS_DEMO_25 ).c_str(), sNum.c_str() );
            if ( EnMessageBox( sMsg.c_str(), MB_YESNO | MB_ICONSTOP ) != IDYES )
            {
                EnWriteProfileInt( "Game", "NumDemo", iTry );
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
#ifdef _WIN32
            ThrowError( ERR_RES_NO_ACCEL );
#else
            // Linux has no embedded accelerator resource; hotkeys come via SDL
            // key events, so a null accel table is non-fatal here.
            Log( "No accelerator table (Linux: no embedded resources) — continuing" );
#endif
        Log( "Accelerators loaded" );

        // screen size, create button brushes
        m_iScrnX = GetSystemMetrics( SM_CXSCREEN );
        m_iScrnY = GetSystemMetrics( SM_CYSCREEN );

#ifdef BUGBUG
        // use Ctl3D
        SetDialogBkColor( ::GetOurSysClr( COLOR_BTNFACE ), ::GetOurSysClr( COLOR_WINDOWTEXT ) );
        Enable3dControls( );
#endif
        m_bSetSysColors = EnGetProfileInt( "Advanced", "SetSysColors", 0 );


        // RedText class for -#s in dialogs
//        if ( m_hPrevInstance == NULL )  // If statement removed because hPrevInstance is always null in modern windows
//        {
        WNDCLASS wc;
        memset( &wc, 0, sizeof( wc ) );
        wc.lpfnWndProc   = RedTextProc;
        wc.hInstance     = ::GetModuleHandle( NULL );  // Phase 4c prep: was AfxGetInstanceHandle
        wc.lpszClassName = "RedText";
        if ( !RegisterClass( &wc ) )
            return ( FALSE );
        memset( &wc, 0, sizeof( wc ) );
        wc.lpfnWndProc   = PerBarProc;
        wc.cbWndExtra    = 2;
        wc.hInstance     = ::GetModuleHandle( NULL );  // Phase 4c prep: was AfxGetInstanceHandle
        wc.lpszClassName = "dcPerBar";
        if ( !RegisterClass( &wc ) )
            return ( FALSE );
        Log( "Window classes registered" );
//        }

#ifdef _CHEAT
        _bShowRate       = EnGetProfileInt( "Debug", "ShowRate", 0 );
        _bClickAny       = EnGetProfileInt( "Cheat", "ClickAny", 0 );
        _bMaxMaterials   = EnGetProfileInt( "Cheat", "MaxMaterials", 0 );
        _bMaxRocket      = EnGetProfileInt( "Cheat", "MaxRocket", 0 );
        _bMaxPower       = EnGetProfileInt( "Cheat", "MaxPower", 0 );
        _iFrameRate      = EnGetProfileInt( "Cheat", "FrameRate", 1 );
        _iFrameRate      = __minmax( 1, 48, _iFrameRate );
        _bSeeAll         = EnGetProfileInt( "Cheat", "SeeAll", 0 );
        _bShowWorld      = EnGetProfileInt( "Cheat", "SeeWorld", 0 );
        _bShowStatus     = EnGetProfileInt( "Cheat", "ShowStatus", 0 );
        _bShowPos        = EnGetProfileInt( "Cheat", "ShowPos", 0 );
        _bShowAISelected = EnGetProfileInt( "Cheat", "ShowAISelected", 0 );
        _iScenarioOn     = EnGetProfileInt( "Cheat", "Scenario", -1 );
#endif

        // get screen resolution, default positions for windows
        m_sResIni = IntToStr( m_iScrnX ) + "x" + IntToStr( m_iScrnY );
        m_iCol1   = m_iScrnX / 5;
        m_iCol2   = __min( ( m_iScrnX * 4 ) / 5, m_iScrnX - 256 );
        m_iRow1   = m_iScrnY / 4;
        m_iRow2   = ( m_iScrnY * 9 ) / 16;
        m_iRow4   = ( m_iScrnY * 5 ) / 32;

        // get the font and sizes for the button bars
        Log( "Creating fonts" );
        CWindowDC dc( (CWnd*)NULL );

        // get the main font - we try Newtown, then Arial, then Arial condensed till we fit
        LOGFONT lf;
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight   = EnGetProfileInt( "StatusBar", "CharHeight", 16 );
        std::string sFont = EnGetProfileStdString( "StatusBar", "Font", "Newtown Italic" );
        strncpy( lf.lfFaceName, sFont.c_str(), LF_FACESIZE - 1 );
        m_Fnt = ::CreateFontIndirect( &lf );

        TEXTMETRIC tm;
        dc.GetTextMetrics( &tm );
        m_iCharHt   = tm.tmHeight;
        m_iCharWid  = tm.tmAveCharWidth;
        m_iBtnBevel = GetSystemMetrics( SM_CXFRAME ) / 2;
        if ( m_iBtnBevel < 2 )
            m_iBtnBevel = 2;

        // dialog fonts
        int iHt = EnGetProfileInt( "StatusBar", "RDHeight", 14 );
        sFont   = EnGetProfileStdString( "StatusBar", "RDFont", "Lucida Console" );
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight = iHt;
        strncpy( lf.lfFaceName, sFont.c_str(), LF_FACESIZE - 1 );
        m_FntRD = ::CreateFontIndirect( &lf );

        iHt   = EnGetProfileInt( "StatusBar", "DescHeight", 18 );
        sFont = EnGetProfileStdString( "StatusBar", "DescFont", "Newtown Italic" );
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight = iHt;
        strncpy( lf.lfFaceName, sFont.c_str(), LF_FACESIZE - 1 );
        m_FntDesc = ::CreateFontIndirect( &lf );

        iHt   = EnGetProfileInt( "StatusBar", "CostHeight", 11 );
        sFont = EnGetProfileStdString( "StatusBar", "CostFont", "Lucida Console" );
        memset( &lf, 0, sizeof( lf ) );
        lf.lfHeight = iHt;
        strncpy( lf.lfFaceName, sFont.c_str(), LF_FACESIZE - 1 );
        m_FntCost = ::CreateFontIndirect( &lf );
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
            // m_wndMain is CWndStub-derived (not CWnd), so we can't do
            // `m_pMainWnd = &m_wndMain` directly. CWnd::FromHandle returns
            // a *temporary* CWnd that MFC garbage-collects in OnIdle, which
            // makes storing it in m_pMainWnd dangerous (crashes inside MFC).
            // Instead: use a permanent proxy CWnd and Attach our HWND to it —
            // this puts the HWND in MFC's permanent handle map. The proxy
            // doesn't get message dispatch (that goes through CWndStub via
            // the registered wndproc), but it lets CWinApp::Run see a valid
            // m_pMainWnd->m_hWnd.
            static CWnd s_mfcMainWndProxy;
            if ( s_mfcMainWndProxy.m_hWnd == NULL )
                s_mfcMainWndProxy.Attach( m_wndMain.m_hWnd );
            m_pMainWnd = &s_mfcMainWndProxy;

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

            // (Compositor wallpaper load moved below, after GameWindow::Create —
            // m_gameWindow doesn't exist yet here, so a load at this point always
            // silently skipped and the load-game flow flashed the dark-gold
            // null-wallpaper fallback.)

// time the CD // we dont have a cd anymore
            m_iCdSpeed = 100; // assume fast CD drive
// Was `#ifndef _GG && 0` — the author's intent was to disable the CD-speed timing
// below (hence the hardcode above), but the preprocessor IGNORES tokens after the
// identifier (the C4067 warning), and _GG is defined nowhere — so the "dead" block
// still RAN: it overwrote m_iCdSpeed from the profile / a disk-read timing loop,
// which on a slow first read could drop below 4 and silently force midi_only music
// (see the m_iCdSpeed < 4 test below). #if 0 = what was meant. (2026-08-07)
#if 0
            if ( ( m_iCdSpeed = EnGetProfileInt( "Advanced", "CDspeed", 0 ) ) <= 0 )
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
                switch ( EnGetProfileInt( "Advanced", "Music", -1 ) )
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
                        if ( EnMessageBoxOnce( IDS_ERROR_LOW_MUSIC, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings",
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
        m_sWndCls = EnRegisterWndClass( "EnPopupWnd",
                                        CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                                        LoadStandardCursor( IDC_ARROW ) );

#ifdef _DEBUG
        theDataFile.DisableNegativeSeekChecking( );
#endif

        // demo license agreement
        if ( IsShareware( ) )
            if ( EnGetProfileInt( "Game", "NoIntro", 0 ) == 0 )
            {
                PostIntro( );
                goto MovieDone;
            }

#ifdef BUGBUG
        // remind them to register
        if ( W32s != iWinType )
            if ( ( !IsShareware( ) ) && ( EnGetProfileInt( "Warnings", "Register", 0 ) == 0 ) )
            {
                CDlgReg dlg( &m_wndMain );
                dlg.DoModal( );
                m_wndMain.UpdateWindow( );
            }
#endif

        // Initialize SDL2 rendering window before PostIntro/CreateMain
        // so the SDL window is available for the main menu
        try {
            m_gameWindow = GameWindow::Create("Enemy Nations - Game View", m_iScrnX, m_iScrnY);
        } catch (...) {
            // Non-fatal: fall back to MFC rendering
        }

        // Start the in-process LLM-driving harness (screenshot/click/keys/state
        // queries) once the SDL window exists. All platforms. No-op unless
        // EN_HARNESS is set in the env — no socket, no thread, no cost.
        if ( m_gameWindow )
            EnHarness_Start( m_gameWindow->GetWindow(), m_gameWindow->GetRenderer() );

        // Load the compositor's WL tile wallpaper NOW that the window exists. The
        // earlier attempt (right after theBitmaps.Init above) is guarded by
        // m_gameWindow — which is only created HERE, so it always silently skipped
        // and the compositor's wallpaper stayed null. RenderWallpaper's null
        // fallback is a solid dark-gold fill, which is what flashed during the
        // load-game flow (status dialog repaints the background every frame) until
        // DestroyMain transferred the menu's tile mid-load.
        if ( m_gameWindow && m_gameWindow->GetCompositor() )
        {
            if ( m_gameWindow->GetCompositor()->LoadWallpaper() )
                Log( "SDL2 compositor: wallpaper loaded (post window create)" );
            else
                Log( "SDL2 compositor: wallpaper load failed (post window create)" );
        }

        // Phase 6 hotfix: make the legacy Win32 main window (EnemyNations-
        // MainWindow stub) transparent + click-through once the SDL window
        // is up. Otherwise both windows compete for Z-order during video
        // playback — clicking to skip a video can land on the Win32 stub
        // instead, bringing it forward and pushing the SDL render target
        // behind it.
        //
        // Important: do NOT SW_HIDE the window. The MFC vehicle/building
        // list-box child windows (m_wndVehicles.m_ListBox, etc.) are
        // created later in newworld.cpp via CreateWindow with this stub
        // as the parent; if the parent is hidden, MFC's CListBox lazy-
        // create leaves m_hWnd NULL and AddToList silently bails — which
        // makes CVehicle::InvalidateStatus FindItem return -1 and trip the
        // ASSERT at unit.cpp:2914. WS_EX_LAYERED + alpha=0 +
        // WS_EX_TRANSPARENT keeps the window in the tree but invisible
        // and click-through. (This mirrors what newworld.cpp:568-570 was
        // already doing later in the lifecycle for the post-game-create
        // window; we just need it earlier so videos benefit.)
        if ( m_gameWindow && m_wndMain.m_hWnd )
        {
            LONG ex = ::GetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE );
            ::SetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE,
                             ex | WS_EX_LAYERED | WS_EX_TRANSPARENT );
            ::SetLayeredWindowAttributes( m_wndMain.m_hWnd, 0, 0, LWA_ALPHA );
        }

        // Play the startup movie via SDL2
        if ( ( HaveIntro( ) ) && ( EnGetProfileInt( "Game", "NoIntro", 0 ) == 0 ) )
        {
            // Temporarily open SDL_mixer so video audio works via Mix_HookMusic.
            // PostIntro() will call theMusicPlayer.Open() later for the full init.
            // allowed_changes = 0: the video hook feeds raw 22050/S16/stereo (its 44100→
            // 22050 downsample ratio is compile-time) — Mix_OpenAudio's default flags let
            // WASAPI open at the endpoint rate (48kHz) with NO conversion → sped-up intro
            // audio (same root as the in-game chipmunk fix in CMusicPlayer::OpenDigital).
            bool tempAudio = false;
            if ( Mix_OpenAudioDevice( 22050, AUDIO_S16SYS, 2, 2048, NULL, 0 ) == 0 )
                tempAudio = true;

            if ( m_gameWindow )
            {
                SDL2VideoPlayer::PlayVideo( m_gameWindow.get(), "assets/videos/logo.mpg" );
                SDL2VideoPlayer::PlayVideo( m_gameWindow.get(), "assets/videos/intro.mpg" );
            }

            if ( tempAudio )
                Mix_CloseAudio();

            PostIntro( );
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
        EnMessageBox( IDS_ERR_LOAD_2, MB_OK | MB_ICONSTOP );
        return ( 0 );
    }

    // list out version
    m_sRif = "Data Ver: " + IntToStr( theApp.GetRifVer( ) ) + "." + IntToStr( VER_RIFF );
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
    Log( m_sRif.c_str( ) );

    // video info
    m_sVideo = "Video: ";
    switch ( ptrthebltformat->GetType( ) )
    {
    case CBLTFormat::DIB_WING:
        m_sVideo += "WinG";
        break;
    case CBLTFormat::DIB_DIBSECTION:
        m_sVideo += "CreateDIBSection";
        break;
    case CBLTFormat::DIB_MEMORY:
        m_sVideo += "StretchDIBits";
        break;
    case CBLTFormat::DIB_SDL_SURFACE:
        m_sVideo += "SDL_Surface";
        break;
    default:
        m_sVideo += "?";
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
    m_sVideo += "), " + IntToStr( ptrthebltformat->GetBitsPerPixel( ) ) + "-bit, (" +
                IntToStr( GetSystemMetrics( SM_CXSCREEN ) ) + "x" +
                IntToStr( GetSystemMetrics( SM_CYSCREEN ) ) + "x";
    hdc           = GetDC( NULL );
    int iBitDepth = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
    ReleaseDC( NULL, hdc );
    m_sVideo += IntToStr( iBitDepth ) + ")";
    Log( m_sVideo.c_str( ) );

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
    if ( !theMusicPlayer.WavOk( ) )
        m_sSound += " {WAV driver failed}";
    else if ( !theMusicPlayer.IsRunning( ) )
        m_sSound += " {turned off}";
    Log( m_sSound.c_str( ) );

    {
        int iRate, iChannels;
        std::string sDriverName;
        theMusicPlayer.GetDigitalConfig( &iRate, &iChannels, sDriverName );
        if ( iRate > 0 )
            m_sSoundVer = std::string( "Audio: " ) + theMusicPlayer.GetVersion( ) + " " +
                          IntToStr( iRate ) + "Hz/" + IntToStr( iChannels ) + "ch, " + sDriverName;
        else
            m_sSoundVer = std::string( "Audio: " ) + theMusicPlayer.GetVersion( ) + " {off}";
    }
    Log( m_sSoundVer.c_str( ) );

    m_sSpeed = "CPU Speed: ~" + IntToStr( theApp.GetCpuSpeed( ) ) + "  CD-ROM Speed: ~" +
               IntToStr( theApp.GetCdSpeed( ) ) + "X";
    Log( m_sSpeed.c_str( ) );

    if ( iWinType == W32s )
    {
        Log( "iWinType w32s" );
        WORD        wVer   = myGetThrdUtlsVersion( );
        std::string sThunk = "Threads DLL " + IntToStr( HIBYTE( wVer ) ) + "." + IntToStr( LOBYTE( wVer ) );
        Log( sThunk.c_str( ) );
    }

    Log( "Initialization complete" );

    return TRUE;
}

CDlgPause* CConquerApp::GetDlgPause( )
{

    if ( m_pdlgPause == NULL )
        m_pdlgPause = new CDlgPause( CWnd::FromHandle( m_wndMain.m_hWnd ) );
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
        theMusicPlayer.Open( EnGetProfileInt( "Game", "Music", 50 ), EnGetProfileInt( "Game", "Sound", 50 ), m_mMode,
                             SFXGROUP::global );

        if ( EnGetProfileInt( "Game", "CustomUI", W32s != iWinType ) ) {
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

    // The SDL-driven main loop (CConquerApp::Run -> GameWindow::PollEvents) never
    // sees the WM_QUIT above — SDL's message pump discards the NULL-hwnd thread
    // message. Push an SDL_QUIT so PollEvents() returns true and the loop exits.
    if ( m_gameWindow )
        m_gameWindow->RequestQuit( );
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
    // The real, visible window is the SDL m_gameWindow on BOTH platforms. Once the
    // SDL main menu / game is up, the MFC m_pMainWnd (m_wndMain) is SW_HIDE-den, so
    // m_pMainWnd->ShowWindow(SW_MINIMIZE) minimized a hidden window and the Minimize
    // button looked dead — on mac (m_pMainWnd is a no-op stub) AND on Windows (real
    // HWND, but hidden). Minimize the whole SDL window group instead; MinimizeAll()
    // also hides the detached ALWAYS_ON_TOP panels that otherwise stay up in-game.
    if ( m_gameWindow )
        m_gameWindow->MinimizeAll( );
#ifdef _WIN32
    else if ( m_pMainWnd )
        m_pMainWnd->ShowWindow( SW_MINIMIZE );   // fallback: no SDL window yet
#endif
}

void CConquerApp::CreateMain( )
{

    // close chat (cancel on multi create)
    CloseDlgChat( );

    // CWndMovie excluded from build (Phase 2d) — SDL2VideoPlayer is synchronous,
    // nothing to destroy at this point.

    // if coming from a game setup - kill it
    theGame.SetShouldProcessMessages(FALSE);
    DestroyExceptMain( );

    m_wndMain.SetProgPos( CWndMain::playing );

    bDoSubclass = TRUE;

    // SDL2 main menu is the only path. CDlgMain MFC fallback excluded from
    // build (Phase 2d).
    if ( !m_sdlMainMenu )
    {
        m_sdlMainMenu = std::make_unique<SDL2MainMenu>();
        if ( !m_sdlMainMenu->Initialize( m_gameWindow.get() ) )
            m_sdlMainMenu.reset();
    }
    if ( m_sdlMainMenu && m_sdlMainMenu->IsInitialized() )
    {
        m_wndMain.ShowWindow( SW_HIDE );
        m_gameWindow->Show();
        m_gameWindow->SetMainMenu( m_sdlMainMenu.get() );
        m_gameWindow->Raise();
        // Restore a visible arrow cursor. Cancelling a game mid-creation returns
        // here while the area map (rocket placement) had hidden the OS cursor via
        // the app-global SDL_ShowCursor(DISABLE); without this the menu has no
        // cursor (no window-enter event fires when the pointer is already over us).
        m_gameWindow->SetArrowCursor();
        Log( "Using SDL2 main menu" );
    }

    theGame.SetState( CGame::main );

    CheckForCD( );

    theMusicPlayer.PlayExclusiveMusic( MUSIC::GetID( MUSIC::main_screen ) );
}

void CConquerApp::DisableMain( )
{

    if ( m_sdlMainMenu )
    {
        // Stop rendering the SDL menu but keep SDL window visible
        // so the create-status dialog can render on it during game creation
        if ( m_gameWindow )
        {
            m_gameWindow->SetMainMenu( nullptr );
            // Don't hide — the SDL window is needed for the status dialog
        }
        return;
    }

    // CDlgMain excluded from build (Phase 2d).
}

void CConquerApp::DestroyMain( )
{

    if ( m_sdlMainMenu )
    {
        if ( m_gameWindow )
        {
            m_gameWindow->SetMainMenu( nullptr );

            // Transfer the WL24 tile wallpaper to the compositor before
            // Shutdown() frees it, so it's available during world building.
            SDL2Compositor* comp = m_gameWindow->GetCompositor();
            SDL_Surface* tile = m_sdlMainMenu->GetTileWallpaper();
            if ( comp && tile )
            {
                // Duplicate the surface — Shutdown() will free the original
                SDL_Surface* copy = SDL_ConvertSurface( tile, tile->format, 0 );
                if ( copy )
                    comp->SetWallpaperSurface( copy );
            }
        }
        m_sdlMainMenu->Shutdown();
        m_sdlMainMenu.reset();
    }

    // Make MFC main window fully transparent during gameplay.
    // It stays valid and in-place (GetDC/RectVisible work) but invisible.
    ::SetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE,
                     ::GetWindowLong( m_wndMain.m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED );
    ::SetLayeredWindowAttributes( m_wndMain.m_hWnd, 0, 0, LWA_ALPHA );
    if ( m_gameWindow )
    {
        m_gameWindow->Show();
        m_gameWindow->Raise();
    }

    // CDlgMain excluded from build (Phase 2d).
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
    // CDlgChatAll excluded from build (Phase 2d). SDL2ChatWindow is the
    // intended replacement; until its network routing is wired, no chat UI.
    return nullptr;
}

void CConquerApp::CloseDlgChat( )
{
    // CDlgChatAll excluded from build (Phase 2d).
}

int CConquerApp::ExitInstance( )
{
    // Idempotent: ExitInstance is reached from TWO paths on quit — the message
    // loop calls it on WM_QUIT (mainloop.cpp `return ExitInstance()`), then WinMain
    // calls it again unconditionally after Run() returns. Running the full teardown
    // twice re-entered myThreadClose() with the global thread-lock `cs` already
    // torn down by the first pass -> EnterCriticalSection on a zeroed CS -> exit-time
    // 0xC0000005 on every quit. Tear down exactly once; later calls are no-ops.
    static bool s_bExited = false;
    if ( s_bExited )
        return 0;
    s_bExited = true;

    CGlobalSubClass::UnSubClass( );

    // Stop and join the AI worker threads FIRST — before ANY game data is
    // torn down. They scan the live world (GetCHexData/AiFillHexLiveNoLock,
    // pathing) right up until they exit; closing sprites/terrain/world under
    // them was the exit-time 0xC0000005 on every quit (2026-06-11). With the
    // AiThread bEndThreads check this join completes in ~100ms per worker.
    myThreadClose( (THREADEXITFUNC)AiExit );

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

    delete m_pdlgPause;
    m_pdlgPause = NULL;

    DestroyExceptMain( );  // if in create
    if ( m_wndBar.IsCreated() )
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
        std::string sLoad = EnLoadStdString( IDS_LEAVING );
        dc.TextOut( 0, 0, sLoad.c_str(), (int)sLoad.size() );
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
        fclose( m_pLogFile );
        m_pLogFile = NULL;
    }

    // show the order form
    if ( IsShareware( ) )
    {
        std::string sCmd = GetDefaultApp( ".doc", "write", "order.doc" );

        STARTUPINFO si;
        memset( &si, 0, sizeof( si ) );
        si.cb          = sizeof( si );
        si.wShowWindow = SW_SHOWMAXIMIZED;
        si.dwFlags     = STARTF_USESHOWWINDOW;
        PROCESS_INFORMATION pi;

        CreateProcess( NULL, &sCmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    return ( CConquerAppSuper::ExitInstance( ) );
}

// Show a Yes/No or Yes/No/Cancel message box via SDL2 if available, else MFC.
static int SDL2_MessageBox( int idsString, bool yesNoCancel = false )
{
    // Load the string from the resource table
    std::string msg = EnLoadStdString( idsString );

    // Replace \n with space for single-line display in our dialog
    for ( auto& c : msg )
        if ( c == '\n' ) c = ' ';

    if ( theApp.m_gameWindow )
    {
        SDL2MessageBox::Style style = yesNoCancel
            ? SDL2MessageBox::YesNoCancel
            : SDL2MessageBox::YesNo;
        SDL2MessageBox dlg( theApp.m_gameWindow.get(), msg, style );
        return dlg.DoModal();
    }

    // MFC fallback
    UINT flags = yesNoCancel
        ? ( MB_YESNOCANCEL | MB_ICONQUESTION )
        : ( MB_YESNO | MB_ICONSTOP | MB_TASKMODAL );
    return EnMessageBox( idsString, flags );
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
            if ( SDL2_MessageBox( IDS_SERVER_QUIT ) == IDNO )
                return ( FALSE );
        }
        else if ( SDL2_MessageBox( IDS_SINGLE_QUIT ) == IDNO )
            return ( FALSE );
    }
    else if ( SDL2_MessageBox( IDS_CLIENT_QUIT ) == IDNO )
        return ( FALSE );

    CWndArea* pWnd = theAreaList.GetTop( );
    if ( pWnd != NULL )
        if ( ( pWnd->GetMode( ) != CWndArea::rocket_ready ) && ( pWnd->GetMode( ) != CWndArea::rocket_pos ) &&
             ( pWnd->GetMode( ) != CWndArea::rocket_wait ) )
        {
            iRtn = SDL2_MessageBox( IDS_SAVE_OLD, true );
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
            for ( CWndAnim* pWnd : theAnimList )
            {
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

    return ( CConquerAppSuper::PreTranslateMessage( pMsg ) );
}

#ifdef _DEBUG
void CConquerApp::AssertValid( ) const
{

    CConquerAppSuper::AssertValid( );

    ASSERT_VALID( &m_wndWorld );
    // ASSERT_VALID( &m_wndChat );  // ChatStub is not CObject-derived (Phase 2d-cont)
    ASSERT_VALID( &m_wndBar );
    ASSERT_VALID( &m_wndBldgs );
    ASSERT_VALID( &m_wndVehicles );

    ASSERT_VALID( &m_wndMain );
    // CDlgMain excluded from build (Phase 2d).
    ASSERT_VALID_OR_NULL( m_pCreateGame );
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgMain dialog — excluded from build (Phase 2d). SDL2MainMenu is the
// live main-menu path. Body kept in source for reference but not compiled.
#if 0  // MFC_LEGACY_MAIN_MENU

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
    std::string sTitle = EnLoadStdString( IDS_MAIN_TITLE );
    SetWindowText( sTitle.c_str() );

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
    std::string sTitle = EnLoadStdString( IDS_MAIN_TITLE );
    LOGFONT lf;
    memset( &lf, 0, sizeof( lf ) );
    lf.lfWidth  = ( 3 * ( iWid / (int)sTitle.size( ) ) ) / 4;
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
        dc.DrawText( sTitle.c_str(), -1, &rect, dtFmt );
        rect.top--;
        rect.left -= iJmp;
    }

    dc.SetTextColor( PALETTERGB( 90, 74, 57 ) );
    dc.DrawText( sTitle.c_str(), -1, &rect, dtFmt );

    rect.top = 0;
    dc.SelectObject( pOld );
    fnt.DeleteObject( );
    dc.SetTextColor( PALETTERGB( 255, 255, 255 ) );

#ifdef _CHEAT
    std::string sVer = "Version: " VER_STRING;
#ifdef _DEBUG
    sVer += " (debug, cheat)";
#else
    sVer += " (cheat)";
#endif
    TEXTMETRIC tm;
    dc.GetTextMetrics( &tm );
    dc.TextOut( tm.tmAveCharWidth, theApp.m_iScrnY - tm.tmHeight, sVer.c_str(), (int)sVer.size() );
#endif

    // put up copyright
    std::string sCopy = EnLoadStdString( IDS_COPYRIGHT );
    GetClientRect( &rect );
    dc.DrawText( sCopy.c_str(), -1, &rect, DT_CALCRECT | DT_CENTER | DT_SINGLELINE | DT_TOP );
    int iHt     = rect.Height( );
    rect.top    = theApp.m_iScrnY - iHt - iHt / 2;
    rect.bottom = theApp.m_iScrnY;
    rect.left   = theApp.m_iScrnX - rect.Width( ) - iHt / 2;
    rect.right  = theApp.m_iScrnX;
    dc.DrawText( sCopy.c_str(), -1, &rect, DT_CENTER | DT_SINGLELINE | DT_TOP );

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

    CRect rect( lpDIS->rcItem );
    char  sText[256];
    pWnd->GetWindowText( sText, (int)sizeof( sText ) );
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
        EnMessageBoxOnce( IDS_ERROR_NT_NET_BUG, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NTnetBug" );
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
        EnMessageBoxOnce( IDS_ERROR_NT_NET_BUG, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NTnetBug" );
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
        EnMessageBoxOnce( IDS_ERROR_NT_NET_BUG, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NTnetBug" );
        UpdateWindow( );
    }
    iNumTimesNet++;

    ASSERT( theApp.m_pCreateGame == NULL );
    theApp.m_pCreateGame = new CCreateLoadMulti( );
    theApp.m_pCreateGame->Init( );
}

void CDlgMain::OnMainCredits( )
{

    // Removed CHEAT-only CDlgTestSounds branch (TestAudio reg key) along
    // with CDlgTestSounds itself.

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
    SDL2OptionsDialog dlg( theApp.m_gameWindow.get() );
    dlg.DoModal();
}

#endif // MFC_LEGACY_MAIN_MENU

/////////////////////////////////////////////////////////////////////////////
// CDlgVer dialog removed; SDL2VersionDialog (SDL2Dialogs.cpp) replaces it.

// CDlgStackDump removed: declared but never instantiated outside this file
// (the exception-stack copy-to-clipboard dialog had no live UI).
