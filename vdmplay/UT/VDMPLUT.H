/*
 * $Header:$
 */
#ifndef __VDMPLUT_H__
#define __VDMPLUT_H__


#if defined(WIN32)
#define HWND32 HWND
#define BOOL32 BOOL
#else
#define HWND32 DWORD
#define BOOL32 DWORD
#endif

enum VPUT_CALLS 
{
	VPUT_GETVERSION,
	VPUT_SUPPORTEDTRANSPORTS,
	VPUT_STARTUP,
	VPUT_CLEANUP,
	VPUT_ENUMSESSIONS,
 	VPUT_STOPENUMSESSIONS,
	VPUT_ENUMPLAYERS,
	VPUT_CREATESESSION,
	VPUT_GETSESSIONINFO,
    VPUT_GETSESSIONINFOBYID,
	VPUT_UPDATESESSIONDATA,
	VPUT_JOINSESSION,
	VPUT_ADDPLAYER,
	VPUT_SENDDATA,
	VPUT_CLOSESESSION,
	VPUT_GETADDRESSSTRING,
	VPUT_GETADDRESS,
	VPUT_ACKNOWLEDGE,
	VPUT_INVITEPLAYER,
	VPUT_REJECTPLAYER,
	VPUT_KILLPLAYER,
	VPUT_SETSESSIONVISIBILITY,
	VPUT_ADVDIALOG,
	VPUT_STARTFT,
	VPUT_ACCEPTFT,
	VPUT_SENDBLOCK,
	VPUT_GETBLOCK,
	VPUT_STOPFT,
	VPUT_ABORTWAIT,
	VPUT_GETSERVERADDRESS
};


struct vpStartup_Args
{
	IN DWORD version; 
    IN LPCVPGUID guid; 
	IN DWORD sessionDataSize;
	IN DWORD playerDataSize;
	IN DWORD protocol; 
	IN LPCVOID protocolData;
};


struct vpEnumSes_Args
{
	IN VPHANDLE pHdl;
	IN HWND32 hWnd;
	IN BOOL32 dontAutoStop; // if FALSE will AutoStop enum
	IN LPCVOID userData;
};

struct vpEnumPl_Args
{
	IN VPHANDLE pHdl;
	IN HWND32 hWnd;
	IN LPCVPSESSIONID sessionId;
    IN LPCVOID userData;
};

struct vpCreateSes_Args
{
	IN VPHANDLE pHdl;
	IN HWND32 hWnd;
	IN LPCSTR sessionName;
	IN DWORD sessionFlags;
	IN LPCVOID userData;
};

struct vpGetSesInfo_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	OUT LPVPSESSIONINFO sInfo;	
};
struct vpGetSesInfo2_Args
{
	IN VPHANDLE pHdl;
	IN HWND32 hWnd;
	IN LPCVPSESSIONID sessionId;
	LPCVOID userData;
};
struct vpUpdSesData_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	IN LPCVOID sessionData;
};


struct vpJoin_Args
{
	IN VPHANDLE pHdl;
	IN HWND32		hWnd;
	IN LPCVPSESSIONID sessionId;
	IN LPCSTR playerName;
	IN DWORD  playerFlags;
	IN LPCVOID userData;
};

struct vpAddPlayer_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	IN LPCSTR playerName;
	IN DWORD  playerFlags;
	IN LPCVOID userData;
	OUT LPVPPLAYERID playerId;
};

struct vpSendData_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	IN VPPLAYERID toPlayerId;
	IN VPPLAYERID fromPlayerId;
	IN LPCVOID data;
    IN DWORD dataLen;
	IN DWORD sendFlags;   // these are from enum VPSENDFLAGS
	IN LPCVOID userData;
};



struct vpCloseSes_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	IN LPCVOID userData;	
};


struct vpGetAddressStr_Args
{
	IN VPHANDLE pHdl;
	IN LPCVPNETADDRESS addr;
	LPSTR buf;
	DWORD bufSize;	
};

struct vpGetAddress_Args
{
	IN VPHANDLE pHdl;
	OUT LPVPNETADDRESS addr;	
};


struct vpAck_Args
{
	IN VPHANDLE pHdl;
	LPCVPMESSAGE pMsg;	
};

struct vpKillPlayer_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	IN VPPLAYERID playrId;
};

struct vpSetSesVisi_Args
{
	IN VPSESSIONHANDLE pSesHdl;
	IN BOOL32 visibility;
};

struct vpAdvDialog_Args
{
	IN  HWND32 hWnd;
	IN  DWORD  protocol;
	IN  BOOL32 server;
};

struct vpAbortWait_Args
{
	IN VPHANDLE pHdl;
};

#endif
/*
 * $Log:$
 */
