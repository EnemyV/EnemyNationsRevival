//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "_windwrd.h"
#include "_res.h"
#include "w22_settings.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "init.h"

int iWinType = WNT; // the default
CWinApp* ptheApp = NULL;

// for asserts
int __iAssertPriority = ASSERT_PRI_CRITICAL;
int __iAssertSection = -1;

void PureFunc() {

    ::MessageBoxA( NULL, "Pure Virtual Function - report the addresses in the next MessageBox",
                   "Enemy Nations", MB_OK | MB_ICONSTOP | MB_TASKMODAL );
    // force a GPF
    char* pBuf = 0;
    char ch = *pBuf;
}

BOOL GetDllVersion( char const* pFile, DWORD& dwMS, DWORD& dwLS ) {

    dwMS = dwLS = 0;

    char sDir[140];
    GetSystemDirectory( sDir, 128 );
    strcat( sDir, "\\" );
    strcat( sDir, pFile );
    DWORD dwHdl = NULL;
    int iSize = (int)GetFileVersionInfoSize( sDir, &dwHdl );
    if ( iSize == 0 )
        return ( FALSE );

    void* pBuf = malloc( iSize );
    GetFileVersionInfo( sDir, dwHdl, iSize, pBuf );
    void FAR* pData;
    UINT uiSize = sizeof( VS_FIXEDFILEINFO );
    VerQueryValue( pBuf, "\\", &pData, &uiSize );

    VS_FIXEDFILEINFO FAR* pVffi = ( VS_FIXEDFILEINFO FAR* ) pData;
    dwMS = pVffi->dwFileVersionMS;
    dwLS = pVffi->dwFileVersionLS;

    free( pBuf );
    return ( TRUE );
}

// this sets up an app for us
// returns TRUE if can run
void InitWindwardLib1( CWinApp const* pWa ) {

    ptheApp = (CWinApp*)pWa;
}

BOOL InitWindwardLib2() {

    // OS version checks removed — only running on modern Windows (Win7+)
    iWinType = WNT;

    // check DirectX version (effectively dead on modern Windows — DDraw/DSound are 10.x)
    DWORD dwMS, dwLS;
    if ( GetDllVersion( "ddraw.dll", dwMS, dwLS ) )
        if ( ( dwMS < 0x40000 ) || ( ( dwMS == 0x40000 ) && ( dwLS < 0x55E0001 ) ) ) {
            char msg[512];
            _snprintf_s( msg, sizeof(msg), _TRUNCATE,
                "DirectDraw version %u.%u.%u.%u is too old.",
                HIWORD( dwMS ), LOWORD( dwMS ), HIWORD( dwLS ), LOWORD( dwLS ) );
            CDlgMsg dlg;
            if ( dlg.MsgBox( msg, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "DirectDraw" ) != IDYES )
                return ( FALSE );
        }
    if ( GetDllVersion( "dsound.dll", dwMS, dwLS ) )
        if ( ( dwMS < 0x40000 ) || ( ( dwMS == 0x40000 ) && ( dwLS < 0x55B0001 ) ) ) {
            char msg[512];
            _snprintf_s( msg, sizeof(msg), _TRUNCATE,
                "DirectSound version %u.%u.%u.%u is too old.",
                HIWORD( dwMS ), LOWORD( dwMS ), HIWORD( dwLS ), LOWORD( dwLS ) );
            CDlgMsg dlg;
            if ( dlg.MsgBox( msg, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "DirectSound" ) != IDYES )
                return ( FALSE );
        }

#ifdef _DEBUG
    // get assert flags here
    __iAssertPriority = w22::GetProfileInt( "Logging", "Priority", ASSERT_PRI_CRITICAL );
    __iAssertSection = w22::GetProfileInt( "Logging", "Section", -1 );
#endif

    return ( TRUE );
}