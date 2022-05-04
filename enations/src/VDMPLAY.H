#ifndef __VDMPLAY_H__
#define __VDMPLAY_H__

#ifndef CONST
#define CONST const
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif

#ifndef USHORT
#define USHORT WORD
#endif

#ifdef WIN32
#ifndef VPSYSTEM
#define VPAPI __declspec(dllimport)
#else
#define VPAPI __declspec(dllexport)
#endif
#else
#ifndef VPSYSTEM
#define VPAPI FAR PASCAL
#else
#define VPAPI APIENTRY
#endif

typedef const void FAR *LPCVOID;
#endif

// set this to one to use timestamp logging
#define VP_TIMESTAMP 0


#define WM_VPNOTIFY (WM_USER+1000)
#define WM_VPFLOWOFF (WM_USER+1001)
#define WM_VPFLOWON (WM_USER+1002)


#define VP_PSEUDOSIZE 2

#define VDMPLAY_INIFILE "VDMPLAY.INI"


#define VPAPIVERSION_MAJOR  0x00
#define VPAPIVERSION_MINOR  0x02

#define VPAPI_VERSION ((VPAPIVERSION_MAJOR << 8) | VPAPIVERSION_MINOR)

#pragma pack(8)

enum VPNOTIFICATION
{
	VP_SESSIONENUM,
	VP_PLAYERENUM,
	VP_JOIN,
	VP_LEAVE,
	VP_READDATA,
	VP_SENDDATA,
	VP_SESSIONCLOSE,
	VP_SERVERDOWN,
	VP_FTREQ,
	VP_NETDOWN
};

enum VPTRANSPORT
{
	VPT_TCP,
	VPT_IPX,
	VPT_NETBIOS,
	VPT_COMM,
	VPT_MODEM,
	VPT_TAPI,
	VPT_DP
};

enum VPERRORS
{
	VP_ERR_OK = 0,
	VP_ERR_NOMEM,
	VP_ERR_BAD_PROTOCOL,
	VP_ERR_NET_ERROR,
	VP_ERR_REMOTE_SESSION,
	VP_ERR_REMOTE_PLAYER,
	VP_ERR_LOCAL_PLAYER,
	VP_ERR_SESSION_NOT_FOUND,
	VP_ERR_BAD_PLAYER_ID,
	VP_ERR_LOST_PLAYER,
	VP_ERR_BUSY,
	VP_ERR_ADD_PLAYER_REFUSED,
	VP_ERR_BAD_CONFIG,
	VP_ERR_NET_DOWN
};
		
   
#define VPGETNOTIFICATION(wParam) LOBYTE(wParam)
#define VPGETERRORCODE(wParam) HIBYTE(wParam)

#ifndef MAKEWPARAM
#define MAKEWPARAM(h, l) (((h)<<8) | ((l)))
#endif

typedef void FAR* VPHANDLE;
typedef VPHANDLE VPSESSIONHANDLE;

typedef WORD    VPPLAYERID;
typedef VPPLAYERID FAR* LPVPPLAYERID;

// Predefined player Id's
const VPPLAYERID  VP_LOCALMACHINE = 0;  // Local VDMPLAY Manager 
const VPPLAYERID  VP_SESSIONSERVER = 1; 
const VPPLAYERID  VP_FIRSTPLAYER = 2;
const VPPLAYERID  VP_ALLPLAYERS = (VPPLAYERID) 0xFFFF;

typedef struct VPMsgHdr
{
	WORD         msgSize;
	BYTE		 msgKind;
	BYTE         msgFlags;
	VPPLAYERID   msgFrom;
	VPPLAYERID   msgTo;
	WORD         msgId;
#if VP_TIMESTAMP
	DWORD		 msgTime;
#endif
} VPMSGHDR,  FAR* LPVPMSGHDR;

typedef CONST VPMSGHDR FAR *  LPCVPMSGHDR;


typedef struct VPNetAddress
{
	char   machineAddress[28];
} VPNETADDRESS, FAR* LPVPNETADDRESS;

typedef CONST VPNETADDRESS FAR * LPCVPNETADDRESS;

typedef struct VPGUID
{
	char buf[32];
} VPGUID, FAR* LPVPGUID;

typedef CONST VPGUID  FAR * LPCVPGUID;

typedef VPNETADDRESS VPSESSIONID, FAR *LPVPSESSIONID;
typedef CONST VPSESSIONID FAR* LPCVPSESSIONID;

typedef struct VPSessionInfo
{
	VPGUID         gameId;          
	VPSESSIONID    sessionId;
	DWORD          version;
	DWORD          playerCount;
	DWORD          sessionFlags;  // From enum VPSESSIONFLAGS
	DWORD          playerDataSize; 
    DWORD          dataSize;
	char           sessionName[VP_PSEUDOSIZE];
	// char           sessionData[dataSize]; 
} VPSESSIONINFO, FAR* LPVPSESSIONINFO;

typedef  CONST VPSESSIONINFO FAR* LPCVPSESSIONINFO; 

typedef struct VPPlayerInfo
{
	VPNETADDRESS   playerAddress;
	VPPLAYERID     playerId;
	DWORD          playerFlags;  // From enum VPPLAYERFLAGS
	DWORD          dataSize;
	char           playerName[VP_PSEUDOSIZE];
	// char           playerData[dataSize]; 
} VPPLAYERINFO, FAR* LPVPPLAYERINFO;

typedef CONST VPPLAYERINFO FAR* LPCVPPLAYERINFO;


// Player enumertaion Info
typedef struct VPPlayerEnumInfo
{
	WORD        	playerIndex;    // the message with Index == Count means last message
	WORD        	playerCount;
	VPPLAYERINFO 	playerInfo;

} VPPENUMINFO, FAR* LPVPENUMINFO;


typedef CONST VPPENUMINFO FAR* LPCVPPENUMINFO;

typedef struct VPFTInfo
{
	enum { FTREQSENT=1, FTREQRECVD, FTACKSEND, FTACKRECVD, FTSEND, FTRECEIVE, FTDONE };
	
	DWORD	ftTotalSize;  // Total transfer size
	DWORD   ftXferCount;  // How much already transferred
	DWORD	ftLastCount;  // size of last transfered block
	VPPLAYERID ftFromId;  // sender player
	VPPLAYERID ftToId;	  // receiver player
	DWORD	   ftState;	  // transfer state
	DWORD	   ftErr;	  // error code
	DWORD	   ftTpData;  // transport specific data
	LPVOID	   ftContext; // internal use
	DWORD	   ftStatus2;	  // internal use

} VPFTINFO, FAR *LPVPFTINFO;

typedef CONST VPFTINFO FAR* LPCVPFTINFO;
 


typedef struct VPMessage
{
	DWORD       notificationCode;
	VPSESSIONHANDLE  session;
	LPVOID     userData;
	VPPLAYERID senderId;
	VPPLAYERID toId;
	DWORD      dataLen;
	DWORD      itemCount;   // so we can pass multiple items of the same type
					        // in on message
	union
	{
		 LPCVPSESSIONINFO  sessionInfo; // VP_SESSIONENUM
		 LPCVPPLAYERINFO   playerInfo;  // VP_JOIN,VP_LEAVE
		 LPCVPPENUMINFO    playerEnumInfo; // VP_PLAYERENUM
		 LPCVPFTINFO	   ftInfo;		   // VP_FTREQUEST
		 LPVOID            data;        // VP_READDATA
    } u;

	DWORD	creationTime;	// When this notification was created
	DWORD	recTime;		// When this notification was received
	DWORD	postTime;		// When this notification was posted
	DWORD   createMsTime;	// When this notification was created in 
							// timeGetTime units

} VPMESSAGE, FAR* LPVPMESSAGE;

typedef CONST VPMESSAGE FAR* LPCVPMESSAGE;

typedef struct VPTcpData
{
	DWORD   serverAddress;
	USHORT  wellKnownPort;
} VPTCPDATA, FAR* LPVPTCPDATA;

typedef CONST VPTCPDATA FAR* LPCVPTCPDATA;

typedef struct VPIpxData
{
	char    netNum[4];
	char    nodeNum[6];
	USHORT  wellKnownPort;
} VPIPXDATA, FAR* LPVPIPXDATA;

typedef CONST VPIPXDATA FAR* LPCVPIPXDATA;


typedef struct VPNetbiosData
{
	WORD		lana;
	char	 	stationName[16+1];
} VPNETBIOSDATA, FAR* LPVPNETBIOSDATA;

typedef CONST VPNETBIOSDATA FAR* LPCVPNETBIOSDATA;

typedef struct VPCommData
{
	char deviceName[128];
	char listenInit[128];
	char callInit[128];
	char callNumber[64];
	char dialPrefix[32];
	char dialSuffix[32];
} VPCOMMDATA, FAR* LPVPCOMMDATA;

typedef CONST  VPCOMMDATA FAR* LPCVPCOMMDATA;

const DWORD VP_MAXPLAYERDATA = 256;
const DWORD VP_MAXSESSIONDATA = 256;
const DWORD VP_MAXSENDDATA  = 500;

enum VP_SESSIONFLAGS 
{ 
	VP_NOAUTOJOIN=1    // Require the APP to confirm player join requests
}; 

enum VPPLAYERFLAGS { VP_AIPLAYER=1, VP_IGNOREBROADCAST=2 }; 

enum VPSENDFLAGS { VP_BROADCAST=1, VP_MUSTDELIVER=2 };

#pragma pack()

extern "C"
{

DWORD    VPAPI vpGetVersion();     // Get the DLL version (VER_MAJOR) << 8 | VER_MINOR)

// Get the transports supported as a bit mask.
// Usage:  if (vpSupportedTransports() && (1 << VPT_TCP)) .... 
DWORD    VPAPI vpSupportedTransports(); 

VPHANDLE VPAPI vpStartup(
		IN DWORD version, // Game Version
		IN LPCVPGUID guid,      // Game Identifications
		IN DWORD sessionDataSize, // size of the session specific data
		IN DWORD playerDataSize,  // size of the player specific data
		IN UINT protocol,          // protocol to use
		IN LPCVOID protocolData   //
		);
			   
BOOL VPAPI vpCleanup(
		IN VPHANDLE pHdl  // Handle returned by the vpStartup
		);


BOOL VPAPI vpEnumSessions(
		IN VPHANDLE pHdl,    // Handle returned by the vpStartup
        IN HWND hWnd,        // Window to receive the notifcation messages
		IN BOOL dontAutoStop, // if FALSE will AutoStop enum
		IN LPCVOID userData    // user data associated with the notifcations
		);
				
// Must be called if dontAutoStop was specified in the eEnumSessions call
BOOL VPAPI vpStopEnumSessions(
		IN VPHANDLE pHdl  // Handle returned by the vpStartup
	);


BOOL VPAPI vpStartRegistrationServer(IN VPHANDLE pHdl, 
                    IN HWND hWnd,
					IN LPCVOID userData);

				

BOOL VPAPI vpStopRegistrationServer(IN VPHANDLE pHdl);



BOOL VPAPI vpEnumPlayers(
		   IN VPHANDLE pHdl, // Handle returned by the vpStartup
		   IN HWND hWnd,     // Window to receive the notifcation messages
           IN LPCVPSESSIONID sessionId,  // Session id to enumerate players
		   IN LPCVOID userData  // user data associated with the notifcations
		);
			   
VPSESSIONHANDLE VPAPI vpCreateSession(
			IN VPHANDLE pHdl, // Handle returned by the vpStartup
			IN HWND hWnd, // Window to receive the notifcation messages
            IN LPCSTR sessionData,  // session private data
			IN DWORD sessionFlags,
			IN LPCVOID userData  // user data associated with the notifcations
		);


BOOL VPAPI vpSetSessionVisibility(
		IN VPSESSIONHANDLE pSesHdl,  // Handle returned by vpCreateSession
		IN BOOL visibility   //  If set to FALSE the server stops responding to to server enumeration requests
		);
		 

							
// Use it to get info on the session created/joined	 by you					
BOOL VPAPI vpGetSessionInfo(
		IN VPSESSIONHANDLE pSesHdl,   // Handle returned by vpCreate or vpJoin
		OUT LPVPSESSIONINFO pInfo     // Where to store returned data
	);

// Use it to get an info about a session that is not joined/created by you
BOOL VPAPI vpGetSessionInfoById(
		IN VPHANDLE pHdl, // Handle returned by the vpStartup
	    IN HWND hWnd,     // Window to receive the notifcation messages
        IN LPCVPSESSIONID sessionId,  // Session ID to query about
	    LPCVOID userData   // user data to associated with the notification
		);


// This will work only for the session created by the caller
BOOL VPAPI vpUpdateSessionData(
			IN VPSESSIONHANDLE pSesHdl,   // handle returned by vpCreateSession
			IN LPCVOID sessionData        // private session data
		);
					  
VPSESSIONHANDLE VPAPI vpJoinSession(
		IN VPHANDLE pHdl, // Handle returned by the vpStartup
		IN HWND hWnd,     // Window to receive the notifcation messages
		IN LPCVPSESSIONID sessionId, // Session ID to join
		IN LPCSTR playerData,   // Player private data (name + ...) 
		IN DWORD  playerFlags,
		IN LPCVOID userData     // user data to associate with notification
		);



// Add a player to the session, this will work only for 
// the session created on this machine
BOOL VPAPI vpAddPlayer(
		IN VPSESSIONHANDLE pHdl,  // session handle returned by vpCreateSession
		IN LPCSTR playerData,     // private player data
		IN DWORD  playerFlags,    
		IN LPCVOID userData,       // userdata associated with the notifications
		OUT LPVPPLAYERID playerId // returned player id
		);
						  
// Send inter-player data						  
BOOL VPAPI vpSendData(
		IN VPSESSIONHANDLE pSesHdl,   // session handle
        IN VPPLAYERID toPlayerId,     // destination player
		IN VPPLAYERID fromPlayerId,   // source player
        IN LPCVOID data,               // data (must acoomodate VPMSGHDR)
		IN DWORD dataLen,			  // size of data
		IN DWORD sendFlags,			  // these are from enum VPSENDFLAGS
        LPCVOID userData				  // user data associated with notification
		);

// Close the joined or created session
BOOL VPAPI vpCloseSession(
		IN VPSESSIONHANDLE pSesHdl,   // session handle to close
		IN LPCVOID userData			  // user data associated with notification
		);

// Return a printable form of a given network address. Return the actual
// length of the string. 
DWORD VPAPI vpGetAddressString(
		IN VPHANDLE pHdl,		   // Handle returned by vpStartup
		IN LPCVPNETADDRESS addr,   // address in question
		LPSTR buf,				   // where to store the printable form
		DWORD bufSize			   // size of the above buffer
	);

// Return the address of the network interface
BOOL VPAPI vpGetAddress(
		IN VPHANDLE pHdl,				// Handle returned by vpStartup
		OUT LPVPNETADDRESS addr		    // Where to store the addreess
	);

// Start File Transfer
BOOL VPAPI vpStartFT(IN VPSESSIONHANDLE pSesHdl, // session handle 
					  INOUT LPVPFTINFO ftInfo     // file transfer info
					  );

// Accept incoming file transfer request
BOOL vpAcceptFT(IN VPSESSIONHANDLE pSesHdl,	// session handle
				  INOUT LPVPFTINFO ftInfo		// file transfer info
				  );    
	 

// Send a file  data block
BOOL VPAPI vpSendBlock(IN VPSESSIONHANDLE pSesHdl, // session handle 
					   IN LPVPFTINFO ftInfo,  // file transfer info
					   IN LPCVOID buf, 
					   IN DWORD bufSize
					   );

// Receive	a file data block
BOOL VPAPI vpGetBlock(IN VPSESSIONHANDLE pSesHdl,  // session handle 
					   INOUT LPVPFTINFO ftInfo, 	  // file transfer info
					   OUT LPVOID buf, 
					   IN  DWORD bufSize
					   );

// Terminate a File Transfer
BOOL VPAPI vpStopFT(IN VPSESSIONHANDLE pSesHdl, // session handle 
						 INOUT LPVPFTINFO ftInfo	// file transfer info
						 );

// this should be called after processing the WM_VPNOTIFY message 
BOOL VPAPI vpAcknowledge(IN VPHANDLE pHdl, LPCVPMESSAGE pMsg);

// One of these MUST be called in responce to VP_JOIN 
// for NOAUTOJOIN sessions
BOOL VPAPI vpInvitePlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId);
BOOL VPAPI vpRejectPlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId);
BOOL VPAPI vpKillPlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId);

void VPAPI vpAdvDialog (HWND hWnd, int iProtocol, BOOL bServer);

void VPAPI vpAbortWait(VPHANDLE pHdl);

BOOL VPAPI vpGetServerAddress(
		IN VPHANDLE pHdl,				// Handle returned by vpStartup
		OUT LPVPNETADDRESS addr		    // Where to store the addreess
	);


#if  !defined(__VDMPLUI_H__) && !defined(VPSYSTEM)

BOOL VPAPI vpAdvancedSetup(int protocol);

#endif

}

inline size_t vpPlayerInfoSize(LPCVPPLAYERINFO pInfo) 
{
	return sizeof(*pInfo) + (size_t) pInfo->dataSize - sizeof(pInfo->playerName);
}

inline size_t vpSessionInfoSize(LPCVPSESSIONINFO sInfo) 
{						      
	return sizeof(*sInfo) + (size_t) sInfo->dataSize - sizeof(sInfo->sessionName);
}



#endif 
