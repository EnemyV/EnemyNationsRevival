#ifndef __LOGGING_H__
#define __LOGGING_H__

////////////////////////////////////////////////////////////////////////////
//
//	logging.h - generic logging system
//
//	Last update:	02/28/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////


const int LOG_PRI_ALWAYS = 0;
const int LOG_PRI_CRITICAL = 1;
const int LOG_PRI_USEFUL = 2;
const int LOG_PRI_VERBOSE = 3;

// windward.lib sections
const int LOG_SEC_DIB =					0x00000001;
const int LOG_SEC_MISC =				0x00000002;
const int LOG_SEC_RESERVED =		0x00000004;


#ifndef _LOGOUT

//BUGBUG __inline void logPrintf (int , int , char const * , ...) {}

#else


class CLog
{
public:
	CLog();
  ~CLog();

	void	Write (int iPriority, int iSection, char const * pBuf);

	BOOL	OkLevel (int iPriority, int iSection) const;

protected:

	CFile m_File;
	CRITICAL_SECTION m_cs;

	int			m_iSection;
	int			m_iLevel;
	BOOL		m_bLogToFile;
	BOOL		m_bLogToDebug;

	BOOL		m_bOpened;
};


void logPrintf (int iPriority, int iSection, char const * pFrmt, ...);

extern CLog theLog;

#endif 


#endif 
