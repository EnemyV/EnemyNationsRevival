////////////////////////////////////////////////////////////////////////////
//
//  IPCMsg.hpp :  CEMsg object
//                Divide and Conquer
//               
//  Last update:    10/03/96       - 09/02/95
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "netcmd.h"

#ifndef __IPCMSG_HPP__
#define __IPCMSG_HPP__

#define USE_FAKE_NET 0	// BUGBUG turn on to use fake network
						// or turn off to use game's network
#define MAX_CC 	20		// BUGBUG hardcoded limit on copy to's

#define IPC_MSG_BM_HEIGHT	16

#define IPC_MESSAGE_ARRIVED WM_USER+2000
#define IPC_MESSAGE_UPDATE 	WM_USER+2001
#define IPC_DESTROY_READWND	WM_USER+2002
#define IPC_EMAIL_REPLY		WM_USER+2003
#define IPC_EMAIL_FORWARD	WM_USER+2004
#define IPC_EMAIL_DELETE	WM_USER+2005
#define IPC_EMAIL_SCROLL	WM_USER+2006
#define IPC_CHAT_IN			WM_USER+2010
#define IPC_CHAT_OUT		WM_USER+2011
#define IPC_CREATE_CHAT		WM_USER+2012
#define IPC_SENDWND_SIZE	WM_USER+2013
#define IPC_UNHIDE_WINDOW	WM_USER+2014


#define MSG_HAS_BEEN_READ	0x0001	// bitmap flags for m_wStatus
#define MSG_IS_CHAT			0x1000
#define MSG_IS_EMAIL		0x2000
#define MSG_IS_VOICE		0x4000

//
// this will be the message sent over the network
//
#if USE_FAKE_NET
class CMsgIPC : public CObject // BUGBUG needs to be CNetCmd
#else
class CMsgIPC : public CNetCmd
#endif
{
public:
    	
	CMsgIPC( int iType );
	~CMsgIPC();

	void		PostToClient ();
	char *	ToBuf (int * piLen);

    enum { email,
    	   chat,
    	   voice };
    
    int m_iTo;		// id of player to receive message
    int m_iFrom;	// id of player sending message
	int m_iCC;		// chat code
    int m_iType;	// enum value == type of message
    int m_iLen;		// length in bytes of message

	CString m_sMessage;	// actual variable length message 
    CString m_sSubject; // used for email only
};


class CEMsg : public CObject
{
	DECLARE_SERIAL( CEMsg );

public:
	int m_iID;				// unique id for this message
	int m_iFrom;			// id of CPlayer initiating email
	int m_iTo;				// id of CPlayer to receive email
	int m_iCC;				// chat code
	WORD m_wStatus;			// status bitmap
	CString m_sSubject;		// subject of email
	CString m_sMessage;		// body of message
	CWnd *m_pOpenWnd;		// pointer to open window for this type

	CEMsg() {};
	~CEMsg();

	CEMsg( int iID, int iFrom, int iTo, 
		const char *pzSubject, const char *pzMessage );

	virtual void Serialize( CArchive& archive );
};

class CEMsgList : public CObList
{
	DECLARE_SERIAL( CEMsgList );

public:
	int m_iLastID;	// last id used

	CEMsgList() {};
	~CEMsgList();

	CEMsg *GetNextPrevMsg( int iID, int iCmd );
	CEMsg *GetMessage( int iID );
	void RemoveMessage( int iID );
	void DeleteList( void );
};

#endif // __IPCMSG_HPP__
