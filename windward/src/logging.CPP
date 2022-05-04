
////////////////////////////////////////////////////////////////////////////
//
//  logging.cpp : Implements the class interfaces for the CLog Object
//
//  Last update:    03/14/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"
#include "_windwrd.h"


#ifdef _LOGOUT

#define new DEBUG_NEW

CLog theLog;


// create a log file
CLog::CLog()
{

	memset (&m_cs, 0, sizeof (m_cs));
	InitializeCriticalSection(&m_cs);

	m_iSection = m_iLevel = 0;
	m_bLogToFile = m_bLogToDebug = m_bOpened = FALSE;
	m_File.m_hFile = (unsigned int) -1;
}

CLog::~CLog()
{

	if ((m_bLogToFile) || (m_bLogToDebug))
		DeleteCriticalSection(&m_cs);

	if (m_File.m_hFile != -1)
		m_File.Close ();
}

BOOL CLog::OkLevel (int iLevel, int iSection) const
{

	if (! m_bOpened)
		return (TRUE);
		
	if ((iLevel > m_iLevel) || (! (m_iSection & iSection)))
		return (FALSE);
	return (TRUE);
}

void CLog::Write (int iLevel, int iSection, char const * pBuf)
{

	if (! m_bOpened)
		{
		m_bOpened = TRUE;

		m_bLogToFile = ptheApp->GetProfileInt ("Logging", "ToFile", 0);

		if (m_bLogToFile)
			{
			CString sName = ptheApp->GetProfileString ("Logging", "FileName", CString (AfxGetAppName () + CString (".log")));
			if (m_File.Open (sName, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary) == 0)
				m_bLogToFile = FALSE;
			}

		m_bLogToDebug = ptheApp->GetProfileInt ("Logging", "ToDebug", 0);

		m_iSection = ptheApp->GetProfileInt ("Logging", "Section", -1);
		m_iLevel = ptheApp->GetProfileInt ("Logging", "Level", LOG_PRI_USEFUL);
		}

	if ((iLevel > m_iLevel) || (! (m_iSection & iSection)))
		return;

	EnterCriticalSection (&m_cs);

	// write to the file (we assume an exception is disk full)
	if (m_bLogToFile)
		{
		try
			{
			m_File.Write (pBuf, strlen (pBuf));
			}
		catch (...)
			{
			m_File.SetLength (0);
			}
		}

	if (m_bLogToDebug)
#ifdef _DEBUG
		TRACE (pBuf);
#else
		OutputDebugString (pBuf);
#endif

	LeaveCriticalSection (&m_cs);
}

void logPrintf (int iLevel, int iSection, char const * pFrmt, ...)
{

	if (! theLog.OkLevel (iLevel, iSection))
		return;
		
	// create the buffer
	char sBuf[2048];
	int * pInt = (int *) &pFrmt;
	_vsnprintf (sBuf, 2044, pFrmt, (char *) (pInt+1));
	strcat (sBuf, "\r\n");

	theLog.Write (iLevel, iSection, sBuf);
}

#endif
