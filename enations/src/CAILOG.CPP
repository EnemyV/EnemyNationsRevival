////////////////////////////////////////////////////////////////////////////
//
//  CAILog.cpp : Implements the class interfaces for the CAILog Object
//
//  Last update:    03/14/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"

#include "CAILog.hpp"
#include "error.h"

#define new DEBUG_NEW

#if _LOGGING

CAILog::CAILog()
{
    m_sFile = "tlpai.log";
    m_pFile = NULL;

	InitializeCriticalSection(&m_cs);
}

CAILog::~CAILog()
{
	DeleteCriticalSection(&m_cs);

    if( m_pFile != NULL )
    {
        m_pFile->Close();
        delete m_pFile;
        m_pFile = NULL;
    }
}

BOOL CAILog::Open( void )
{
    // open the file of data
    CStdioFile *pFile = new CStdioFile;

    CFileStatus status;
    UINT nAccess = CFile::modeWrite;

    // GetStatus will return TRUE if file exists,
    // or FALSE if it doesn't exist
    if( !CFile::GetStatus( m_sFile, status ) )
        nAccess |= CFile::modeCreate;
    else
        CFile::Remove( m_sFile );
    
    if( !pFile->Open( m_sFile, 
        CFile::modeCreate | CFile::modeWrite | CFile::typeText ) )
    {
        delete pFile;
        return FALSE;
    }
    m_pFile = pFile;
	return TRUE;
}

void CAILog::Write( const char *pLine )
{
	EnterCriticalSection(&m_cs);

	try
    {
        m_pFile->WriteString( pLine );
    }
    catch( CFileException theException )
    {
      	m_pFile->Close();
       	delete m_pFile;
       	m_pFile = NULL;

		LeaveCriticalSection(&m_cs);

		throw(ERR_CAI_BAD_FILE);
    }
	LeaveCriticalSection(&m_cs);
}

void CAILog::Close( void )
{
    if( m_pFile != NULL )
    {
        m_pFile->Close();
        delete m_pFile;
        m_pFile = NULL;
    }
}

#endif // end of CLog.cpp

