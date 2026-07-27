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
#include "winappstub.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "init.h"

int iWinType = WNT; // the default
CWinAppStub* ptheApp = NULL;

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
void InitWindwardLib1( CWinAppStub const* pWa ) {
    ptheApp = (CWinAppStub*)pWa;
}

BOOL InitWindwardLib2() {

    // OS version checks removed — only running on modern Windows (Win7+)
    iWinType = WNT;

    // Phase 6 Stage 4: DirectDraw removed entirely. The DirectSound version
    // gate that used to live here is gone too — audio runs through SDL_mixer,
    // not DirectSound, so the dsound.dll version no longer matters (and the
    // check blocks a non-Windows build where dsound.dll doesn't exist).

#ifdef _DEBUG
    // get assert flags here
    __iAssertPriority = w22::GetProfileInt( "Logging", "Priority", ASSERT_PRI_CRITICAL );
    __iAssertSection = w22::GetProfileInt( "Logging", "Section", -1 );
#endif

    return ( TRUE );
}