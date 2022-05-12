#ifndef __GAMENET_H__
#define __GAMENET_H__

#define GN_NAMESIZE 32
#define WM_GNNOTIFY (WM_USER+1000)
#define GN_PSEUDOSIZE 0

enum GNNOTIFICATION
{
	GN_SESSIONENUM,
	GN_PLAYERENUM,
	GN_JOIN,
	GN_LEAVE,
	GN_READDATA,
	GN_SENDDATA,
	GN_SESSIONCLOSE
};

enum GNERRORS
{
	GN_ERR_OK = 0,
	GN_ERR_NOMEM,
	GN_ERR_BAD_PROTOCOL,
	GN_ERR_NET_ERROR,
	GN_ERR_REMOTE_SESSION,
	GN_ERR_REMOTE_PLAYER,
	GN_ERR_SESSION_NOT_FOUND,
	GN_ERR_BAD_PLAYER_ID,
	GN_ERR_LOST_PLAYER
};
		
   
#define GNGETNOTIFICATION(wParam) LOBYTE(wParam)
#define GNGETERRORCODE(wParam) HIBYTE(wParam)

typedef void FAR* GNHANDLE;
typedef GNHANDLE GNSESSIONHANDLE;

typedef DWORD    GNPLAYERID;

// Predefined player Id's
const GNPLAYERID  GN_LOCALMACHINE = 0;  // Local GameNet Manager 
const GNPLAYERID  GN_SESSIONSERVER = 1; 
const GNPLAYERID  GN_ALLPLAYERS = ~0L,

typedef struct gnMsgHdr
{
	DWORD   msgFrom;
	DWORD   msgTo;
	DWORD   msgKind;
	DWORD   msgId;
} GNMSGHDR, FAR* LPGNMSGHDR, CONST FAR* LPCGNMSGHDR;


typedef struct gnNetAddress
{
	char   machineAddress[28];
	DWORD  entityId;
	
} GNNETADDRESS, FAR* LPGNNETADDRESS, CONST FAR* LPCGNETADDRESS;

typedef struct gnGUID
{
	char buf[32];
} GNGUID, FAR* LPGNGUID;

typedef GNNETADDRESS GNSESSIONID, FAR *LPGNSESSIONID;

typedef struct gnSessionInfo
{
	GNSESSIONID    sessionId;
	DWORD          playerCount;
	DWORD          sessionFlags;  // From enum GNSESSIONFLAGS
    DWORD          dataSize;
	char           sessionName[GN_PSEUDOSIZE];
	// char           sessionData[dataSize]; 
} GNSESSIONINFO, FAR* LPGNSESSIONINFO , CONST  FAR* LPCGNSESSIONINFO; 

typedef struct gnPlayerInfo
{
	GNNETADDRESS   playerAddress;
	GNPLAYERID     playerId;
	DWORD          playerFlags;  // From enum GNPLAYERFLAGS
	DWORD          dataSize;
	char           playerName[GN_PSEUDOSIZE];
	// char           playerData[dataSize]; 
} GNPLAYERINFO, FAR* LPGNPLAYERINFO, CONST FAR* LPCGNPLAYERINFO;

typedef struct gnMessage
{
	WPARAM     notificationCode;
	GNSESSIONHANDLE  session;
	LPVOID     userData;
	GNPLAYERID senderId;
	DWORD      dataLen;
	DWORD      itemCount;   // so we can pass multiple items of the same type
					        // in on message
	union
	{
		 LPCGNSESSIONINFO  sessionInfo; // GN_SESSIONENUM
		 LPCGNPLAYERINFO   playerInfo;  // GN_JOIN,GN_LEAVE,GN_PLAYERENUM
		 LPVOID            data;        // GN_READDATA
    };
} GNMESSGAGE, FAR* LPGNMESSAGE;



const DWORD GN_MAXPLAYERDATA = 256;
const DWORD GN_MAXSESSIONDATA = 256;
const DWORD GN_MAXSENDDATA  = 500;

enum GN_SESSIONFLAGS 
{ 
	GN_NOAUTOJOIN=1    // Require the APP to confirm player join requests
}; 

enum GNPLAYERFLAGS { GN_AIPLAYER=1, GN_IGNOREBROADCAST=2 }; 

enum GNSENDFLAGS { GN_BROADCAST=1, GN_MUSTDELIVER=2 };

GNHANDLE gnStartup(IN DWORD version, 
                   IN LPCGNGUID guid, 
				   IN DWORD sessionDataSize,
				   IN DWORD userDataSize,
				   IN UINT protocol);
			   
BOOL gnCleanup(IN GNHANDLE pHdl);
BOOL gnEnumSessions(IN GNHANDE pHdl, 
                    IN HWND hWnd,
					IN BOOL dontAutoStop, // if FALSE will AutoStop enum
					IN LPVOID userData);
				
// Must be called if dontAutoStop was specified in the eEnumSessions call
BOOL gnStopEnumSessions(in GNHANDLE pHdl);

BOOL gnEnumPlayers(IN GNHANDLE pHdl,
				   IN HWND hWnd, 
	               IN LPCGNSESSIONID sessionId,
				   IN LPVOID userData);
			   
GNSESSIONHANDLE gnCreateSession(IN GNHANDLE pHdl, 
								IN HWND hWnd, 
                                IN LPCSTR sessionName,
								IN DWORD sessionFlags,
								IN LPVOID sessionData,
							    IN LPVOID userData);
							
// Use it to get info on the session created/joined	 by you					
BOOL gnGetSessionInfo(IN GNSESSIONHANDLE pSesHdl, OUT LPGSESSIONINFO);

// Use it to get an info about a session that is not joined/created by you
BOOL gnGetSessionInfoById(IN GHANDLE pHdl, 
	                      IN HWND hWnd, 
                          IN LPCSESSIONID sessionId,
						  LPVOID userData);


// This will work only for the session created by the caller
BOOL gnUpdateSessionData(IN GNSESSIONHANDLE, IN LPVOID sessionData);
					  
GNSESSIONHANDLE gnJoinSession(IN GNHANDLE pHdl,
							  IN HWND hWnd,
						      IN LPCGNSESSIONID sessionId, 
				              IN LPCSTR playerName,
							  IN DWORD  playerFlags,
							  IN LPVOID userData,
							  OUT LPGNPLAYERID playerId);


// Add a player to the session, this will work only for 
// the session created on this machine
BOOL gnAddPlayer(IN GNSESSIONHANDLE pHdl,
				              IN LPCSTR playerName,
							  IN DWORD  playerFlags,
							  IN LPVOID userData,
							  OUT LPGNPLAYERID playerId);
						  
						  
BOOL gnSendData(IN GNSESSIONHANDLE pHdl, 
                IN GNPLAYERID toPlayerId, 
				IN GNPLAYERID fromPlayerId,
                IN LPVOID data, IN DWORD dataLen, 
				IN DWORD sendFlags,   // these are from enum GNSENDFLAGS
                LPVOID userData);
			
BOOL gnCloseSesion(IN GNSESSIONHANDLE pSesHdl, IN LPVOID userData);


// this should be called after processing the WM_GNNOTIFY message 
BOOL gnAcknowledge(IN GHANDLE pHdl, LPCGNMESSAGE pMsg);

// One of these MUST be called in responce to GN_JOIN 
// for NOAUTOJOIN sessions
BOOL gnInvitePlayer(IN GNSESSIONHANDLE pSesHdl, GNPLAYERID playrId);
BOOL gnRejectPlayer(IN GNSESSIONHANDLE pSesHdl, GNPLAYERID playrId);


inline DWORD gnPlayerInfoSize(LPCGNPLAYERINFO pInfo) 
{
	return sizeof(*pInfo) + pInfo->dataSize - sizeof(pInfo->playerName);
}

inline DWORD gnSessionInfoSize(LPCGNSESSIONINFO sInfo) 
{
	return sizeof(*sInfo) + sInfo->dataSize - sizeof(sInfo->playerName);
}

#endif 
