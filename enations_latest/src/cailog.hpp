////////////////////////////////////////////////////////////////////////////
//
//	CAILog.hpp : Declares the class interfaces for the CAILog Object
//
//	Last update:	02/28/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CAILOG_HPP__
#define __CAILOG_HPP__

#define LOG_MSG_LENGTH	120

class CAILog
{
	CStdioFile *m_pFile;
    CString m_sFile;
	CRITICAL_SECTION m_cs; // internal use only

public:
    
	CAILog();
    ~CAILog();
	BOOL Open( void );
    void Write( const char *pLine );
	void Close( void );
};

#endif // __CAILOG_HPP__
