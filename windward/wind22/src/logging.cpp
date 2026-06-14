
////////////////////////////////////////////////////////////////////////////
//
//  logging.cpp : Implements the class interfaces for the CLog Object
//
//  Last update:    03/14/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "_windwrd.h"
#include "w22_settings.h"


#ifdef _LOGOUT

#define new DEBUG_NEW

CLog theLog;


// create a log file
CLog::CLog() {

    memset( &m_cs, 0, sizeof( m_cs ) );
    InitializeCriticalSection( &m_cs );

    m_iSection = m_iLevel = 0;
    m_bLogToFile = m_bLogToDebug = m_bOpened = FALSE;
    m_pFile = NULL;
}

CLog::~CLog() {

    if ( ( m_bLogToFile ) || ( m_bLogToDebug ) )
        DeleteCriticalSection( &m_cs );

    if ( m_pFile != NULL )
        fclose( m_pFile );
}

BOOL CLog::OkLevel( int iLevel, int iSection ) const {

    if ( !m_bOpened )
        return ( TRUE );

    if ( ( iLevel > m_iLevel ) || ( !( m_iSection & iSection ) ) )
        return ( FALSE );
    return ( TRUE );
}

void CLog::Write( int iLevel, int iSection, char const* pBuf ) {


    if ( !m_bOpened ) {
        m_bOpened = TRUE;

        m_bLogToFile = w22::GetProfileInt( "Logging", "ToFile", 0 );

        if ( m_bLogToFile ) {
            const char* sName = w22::GetProfileString( "Logging", "FileName", "enations.log" );
            if ( fopen_s( &m_pFile, sName, "wb" ) != 0 || m_pFile == NULL )
                m_bLogToFile = FALSE;
        }

        m_bLogToDebug = w22::GetProfileInt( "Logging", "ToDebug", 0 );

        m_iSection = w22::GetProfileInt( "Logging", "Section", -1 );
        m_iLevel = w22::GetProfileInt( "Logging", "Level", LOG_PRI_USEFUL );
    }

    if ( ( iLevel > m_iLevel ) || ( !( m_iSection & iSection ) ) )
        return;

    EnterCriticalSection( &m_cs );

    // write to the file (we assume an exception is disk full)
    if ( m_bLogToFile && m_pFile != NULL ) {
        size_t len = strlen( pBuf );
        if ( fwrite( pBuf, 1, len, m_pFile ) < len ) {
            // disk full — truncate
            _chsize_s( _fileno( m_pFile ), 0 );
        }
        fflush( m_pFile );
    }

    if ( m_bLogToDebug )
#ifdef _DEBUG
        TRACE( pBuf );
#else
        OutputDebugString( pBuf );
#endif

    LeaveCriticalSection( &m_cs );
}

void logPrintf( int iLevel, int iSection, char const* pFrmt, ... ) {

    if ( !theLog.OkLevel( iLevel, iSection ) )
        return;

    // create the buffer
    char sBuf[2048];
    auto pInt = (int*)&pFrmt;
    _vsnprintf( sBuf, 2044, pFrmt, (char*)( pInt + 1 ) );
    strcat( sBuf, "\r\n" );

    theLog.Write( iLevel, iSection, sBuf );
}

#endif
