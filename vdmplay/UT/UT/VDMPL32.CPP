/*
 * $Header: /vdmplay/ut/vdmpl32.cpp 6     6/01/96 20:34 Vadim $
 */
#define  VPSYSTEM
#include "stdafx.h"

extern "C"
{
#define W32SUT_32
#include "w32sut.h"
}

#include "vdmplut.h"





UT32PROC  pUt=NULL;
int       cProcessesAttached = 0;
HMODULE hInst = NULL;		



extern "C"
{

BOOL LoadUT()
{
    BOOL ret = UTRegister( hInst,        // DB32.DLL module handle
                       "VDMPL16.DLL",   // name of 16bit thunk dll
                       NULL,         // no init routine
                       "UTPROC",     // name of 16bit dispatch routine
                                     // exported from DB16.DLL
                       &pUt,   // Global variable to receive thunk address
                       NULL,         // no call back function
                       NULL          // no shared memroy
                     );


	if (!ret)
	{
		DWORD err = GetLastError();
		char buf[128];

		wsprintf(buf, "Error %08lx calling UTRegister", err);

		MessageBox(NULL, buf, "VDMPLAY", MB_OK);
	}

	return ret;
	 	


}



DWORD VPAPI vpGetVersion()
{
	return pUt(NULL, VPUT_GETVERSION, NULL);
}



DWORD VPAPI vpSupportedTransports()
{
	if (!pUt)
		LoadUT();

	return pUt(NULL, VPUT_SUPPORTEDTRANSPORTS, NULL);
 }

	 



VPHANDLE VPAPI vpStartup(IN DWORD version, 
                   IN LPCVPGUID guid, 
				   IN DWORD sessionDataSize,
				   IN DWORD playerDataSize,
				   IN UINT protocol, IN LPCVOID protocolData)

{
	vpStartup_Args a;
	LPVOID trans[3];
	
	a.version = version; 
    a.guid = guid;
	a.sessionDataSize = sessionDataSize;
	a.playerDataSize = playerDataSize;
	a.protocol = protocol; 
	a.protocolData = protocolData;

	trans[0] = &a.guid;
	trans[1] = &a.protocolData;
	trans[2] = NULL;
	
	return (VPHANDLE) pUt(&a, VPUT_STARTUP, trans);
}


BOOL VPAPI vpCleanup(IN VPHANDLE pHdl)
{

	return (BOOL) pUt(&pHdl, VPUT_CLEANUP, NULL);
}


	
BOOL VPAPI vpEnumSessions(IN VPHANDLE pHdl, 
                    IN HWND hWnd,
					IN BOOL dontAutoStop, // if FALSE will AutoStop enum
					IN LPCVOID userData)
{
	vpEnumSes_Args a;
	
	a.pHdl = pHdl;
	a.hWnd = hWnd;
	a.dontAutoStop = dontAutoStop; // if FALSE will AutoStop enum
	a.userData = userData;	
	
	
	return (BOOL) pUt(&a, VPUT_ENUMSESSIONS, NULL);
	

}


				
// Must be called if dontAutoStop was specified in the eEnumSessions call
BOOL VPAPI vpStopEnumSessions(IN VPHANDLE pHdl)
{
	return (BOOL) pUt(&pHdl, VPUT_STOPENUMSESSIONS, NULL);
}


BOOL VPAPI vpEnumPlayers(IN VPHANDLE pHdl,
				   IN HWND hWnd, 
	               IN LPCVPSESSIONID sessionId,
				   IN LPCVOID userData)
{
	vpEnumPl_Args a;
	LPVOID trans[2];
	
	a.pHdl = pHdl;
	a.hWnd = hWnd;
	a.sessionId = sessionId;
    a.userData = userData;
	
	trans[0] = &a.sessionId;
	trans[1] = NULL;

	return (BOOL) pUt(&a, VPUT_ENUMPLAYERS, trans); 
}


VPSESSIONHANDLE VPAPI vpCreateSession(IN VPHANDLE pHdl, 
								IN HWND hWnd, 
                                IN LPCSTR sessionName,
								IN DWORD sessionFlags,
							    IN LPCVOID userData)
{
	vpCreateSes_Args a;
	LPVOID trans[2];

	a.pHdl = pHdl;
	a.hWnd = hWnd;
	a.sessionName = sessionName;
	a.sessionFlags = sessionFlags;
	a.userData = userData;
	
	trans[0] = &a.sessionName;
	trans[1] = NULL;
	
	return (VPSESSIONHANDLE) pUt(&a, VPUT_CREATESESSION, trans);

}

							
// Use it to get info on the session created/joined	 by you					
BOOL VPAPI vpGetSessionInfo(IN VPSESSIONHANDLE pSesHdl, OUT LPVPSESSIONINFO sInfo)
{
	vpGetSesInfo_Args a;
	LPVOID trans[2];
	
	
	a.pSesHdl = pSesHdl;
	a.sInfo = sInfo;	
	
	trans[0] = &a.sInfo;
	trans[1] = NULL;
	
	return (BOOL) pUt(pSesHdl, VPUT_GETSESSIONINFO, trans); 

}


// Use it to get an info about a session that is not joined/created by you
BOOL VPAPI vpGetSessionInfoById(IN VPHANDLE pHdl, 
	                      IN HWND hWnd, 
                          IN LPCVPSESSIONID sessionId,
						  LPCVOID userData)
{
	vpGetSesInfo2_Args  a;
	LPVOID  trans[2];

	a.pHdl = pHdl;
	a.hWnd = hWnd;
	a.sessionId = sessionId;
	a.userData = userData;
	
	trans[0] = &a.sessionId;
	trans[1] = NULL;
	
	return (BOOL) pUt(&a, VPUT_GETSESSIONINFOBYID, trans);
	
}


// This =  will work only for the session created by the caller
BOOL VPAPI vpUpdateSessionData(IN VPSESSIONHANDLE pSesHdl, IN LPCVOID sessionData)
{
	vpUpdSesData_Args a;
	LPVOID trans[2];
	
	a.pSesHdl = pSesHdl;
	a.sessionData = sessionData;

	trans[0] = &a.sessionData;
	trans[1] = NULL;
	
	
	return (BOOL) pUt(&a, VPUT_UPDATESESSIONDATA, trans);

}


					  
VPSESSIONHANDLE VPAPI vpJoinSession(IN VPHANDLE pHdl,
							  IN HWND hWnd,
						      IN LPCVPSESSIONID sessionId, 
				              IN LPCSTR playerName,
							  IN DWORD  playerFlags,
							  IN LPCVOID userData)
{
	vpJoin_Args a;
	LPVOID trans[3];

	a.pHdl = pHdl;
	a.hWnd = 	hWnd;
	a.sessionId = sessionId;
	a.playerName = playerName;
	a.playerFlags =  playerFlags;
	a.userData = userData;
	
	trans[0] = &a.sessionId;
	trans[1] = &a.playerName;
	trans[2] = NULL;

	return (VPSESSIONHANDLE) (*pUt)(&a, VPUT_JOINSESSION, trans);
	
}




// Add a player to the session, this will work only for 
// the session created on this machine
BOOL VPAPI vpAddPlayer(IN VPSESSIONHANDLE pSesHdl,
				              IN LPCSTR playerName,
							  IN DWORD  playerFlags,
							  IN LPCVOID userData,
							  OUT LPVPPLAYERID playerId)
{
	vpAddPlayer_Args a;
	LPVOID trans[3];
	
	a.pSesHdl = pSesHdl;
	a.playerName = playerName;
	a.playerFlags =  playerFlags;
	a.userData = userData;
	a.playerId = playerId;

	trans[0] = &a.playerName;
	trans[1] = &a.playerId;
	trans[2] = NULL;
	
	return (BOOL) pUt(&a, VPUT_ADDPLAYER, trans);
	
	
}


						  
						  
BOOL VPAPI vpSendData(IN VPSESSIONHANDLE pSesHdl, 
                IN VPPLAYERID toPlayerId, 
				IN VPPLAYERID fromPlayerId,
                IN LPCVOID data, IN DWORD dataLen, 
				IN DWORD sendFlags,   // these are from enum VPSENDFLAGS
                LPCVOID userData)
{
	vpSendData_Args a;
	LPVOID trans[2];

	a.pSesHdl = pSesHdl;
	a.toPlayerId = toPlayerId;
	a.fromPlayerId = fromPlayerId;
	a.data = data;
    a.dataLen = dataLen;
	a.sendFlags = sendFlags;   // these are from enum VPSENDFLAGS
	a.userData = userData;

	trans[0] = &a.data;
	trans[1] = NULL;
	
	return (BOOL) pUt(&a, VPUT_SENDDATA, trans);
	
}


			
BOOL VPAPI vpCloseSession(IN VPSESSIONHANDLE pSesHdl, IN LPCVOID userData)
{
	vpCloseSes_Args a;
	
	
	a.pSesHdl = pSesHdl;
	a.userData = userData;	


	
	return (BOOL) pUt(&a, VPUT_CLOSESESSION, NULL);
}



DWORD VPAPI vpGetAddressString(IN VPHANDLE pHdl, IN LPCVPNETADDRESS addr,  LPSTR buf, DWORD bufSize)
{

	vpGetAddressStr_Args a;
 	LPVOID trans[3];

	a.pHdl = pHdl;
	a.addr = addr;
	a.buf = buf;
	a.bufSize = bufSize;	

	trans[0] = &a.addr;
	trans[1] = &a.buf;
	trans[2] = NULL;
	
	return (DWORD) pUt(&a, VPUT_GETADDRESSSTRING, trans);
	

}


BOOL VPAPI vpGetAddress(IN VPHANDLE pHdl, OUT LPVPNETADDRESS addr)
{
	vpGetAddress_Args a;
	LPVOID trans[2];

	a.pHdl = pHdl;
	a.addr = addr;	

	trans[0] = &a.addr;
	trans[1] = NULL;
	
	return (BOOL) pUt(&a, VPUT_GETADDRESS, trans);
	
	
	
}

// this should be called after processing the WM_VPNOTIFY message 
BOOL VPAPI vpAcknowledge(IN VPHANDLE pHdl, LPCVPMESSAGE pMsg)
{
	vpAck_Args a;
 	LPVOID trans[2];

	a.pHdl = pHdl;
	a.pMsg = pMsg;
	
	trans[0] = &a.pMsg;
	trans[1] = NULL;
	
	return (BOOL) pUt(&a, VPUT_ACKNOWLEDGE, trans);
	
}	




	

// One of these MUST be called in responce to VP_JOIN 
// for NOAUTOJOIN sessions
BOOL VPAPI vpInvitePlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId)
{
	vpKillPlayer_Args a;

	a.pSesHdl = pSesHdl;
	a.playrId = playrId;

	
	return (BOOL) pUt(&a, VPUT_INVITEPLAYER, NULL);

}

BOOL VPAPI vpRejectPlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId)
{
	vpKillPlayer_Args a;

	a.pSesHdl = pSesHdl;
	a.playrId = playrId;

	
	return (BOOL) pUt(&a, VPUT_REJECTPLAYER, NULL);
}

BOOL VPAPI vpKillPlayer(IN VPSESSIONHANDLE pSesHdl, IN VPPLAYERID playerId)
{
	vpKillPlayer_Args a;

	a.pSesHdl = pSesHdl;
	a.playrId = playerId;

	
	return (BOOL) pUt(&a, VPUT_KILLPLAYER, NULL);

}


	
BOOL VPAPI vpSetSessionVisibility(
	IN VPSESSIONHANDLE pSesHdl,  // Handle returned by vpCreateSession
	IN BOOL visibility			 // If set to FALSE the server stops responding to to server enumeration requests
		)
{
	vpSetSesVisi_Args a;

	a.pSesHdl = pSesHdl;  
	a.visibility = visibility;


	return (BOOL) pUt(&a, VPUT_SETSESSIONVISIBILITY, NULL);

}
	
	






BOOL __stdcall
DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
  hInst = hInstance;

  if ( fdwReason == DLL_PROCESS_ATTACH ) 
  {

    /*
     * Registration of UT need to be done only once for first
     * attaching process.
     */

    if ( cProcessesAttached++ ) 
	{
        return(TRUE);
    }

    return LoadUT();

  } 
  else if ( fdwReason == DLL_PROCESS_DETACH ) 
  {

      if ( --cProcessesAttached == 0 ) 
	  {
          UTUnRegister( hInst );
      }
  }

  return TRUE;


}

}
/*
 * $Log: /vdmplay/ut/vdmpl32.cpp $
 * 
 * 6     6/01/96 20:34 Vadim
 * Shows signs of life
 * 
 * 5     6/01/96 15:01 Vadim
 * More fixes to header
 */
