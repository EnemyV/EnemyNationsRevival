/*
 * $Header: /vdmplay/ut/vput16.cpp 5     6/01/96 20:34 Vadim $
 */

#include "../stdafx.h"
#define W32SUT_16
#include "w32sut.h"
#include "vdmplut.h"

/*
 * 16bit dispatcher function.
 * exported by DB16.DLL
 */

extern "C" VPHANDLE vpUtStartup(IN DWORD version, 
                   IN LPCVPGUID guid, 
				   IN DWORD sessionDataSize,
				   IN DWORD playerDataSize,
				   IN UINT protocol, IN LPCVOID protocolData);

 
extern "C" 
DWORD FAR PASCAL
__export UTProc(LPVOID lpBuf, DWORD dwFunc)
{

   union
   {   
   	   LPVOID           buf;
	   VPSESSIONHANDLE* pSesHdl;
	   VPHANDLE*        pHdl;
	   vpStartup_Args* vpStartup;
	   vpEnumSes_Args* vpEnumSes;
	   vpEnumPl_Args* vpEnumPl;
	   vpCreateSes_Args* vpCreateSes;
	   vpGetSesInfo_Args* vpGetSesInfo;
	   vpGetSesInfo2_Args* vpGetSesInfo2;
	   vpUpdSesData_Args* vpUpdSesData;
	   vpJoin_Args* vpJoin;
	   vpAddPlayer_Args* vpAddPlayer;
	   vpSendData_Args* vpSendData;
	   vpCloseSes_Args* vpCloseSes;
	   vpGetAddressStr_Args* vpGetAddressStr;
	   vpGetAddress_Args* vpGetAddress;
	   vpAck_Args* vpAck;
	   vpKillPlayer_Args* vpKillPlayer;
	   vpSetSesVisi_Args* vpSetSesVisi;
	   
   } a;
	

   a.buf = lpBuf;
   
   switch(dwFunc)
   {
	   case VPUT_GETVERSION:
	   {
		   return vpGetVersion();
	   }
	   case VPUT_SUPPORTEDTRANSPORTS:
	   {
		   return vpSupportedTransports();
	   }
	   case VPUT_STARTUP:
	   {
		   return (DWORD) vpUtStartup(a.vpStartup->version,
			   a.vpStartup->guid,
			   a.vpStartup->sessionDataSize,
			   a.vpStartup->playerDataSize,
			   a.vpStartup->protocol,
			   a.vpStartup->protocolData);
	   }
	   case VPUT_CLEANUP:
	   {
		   return vpCleanup(*a.pHdl);
	   }
	   case VPUT_ENUMSESSIONS:
	   {
		   return vpEnumSessions(a.vpEnumSes->pHdl,
			   (HWND) a.vpEnumSes->hWnd,
			  (BOOL) a.vpEnumSes->dontAutoStop,
			   a.vpEnumSes->userData);
	   }
	   case VPUT_STOPENUMSESSIONS:
	   {
		   return vpStopEnumSessions(*a.pHdl);
	   }
	   case VPUT_ENUMPLAYERS:
	   {
		   return vpEnumPlayers(a.vpEnumPl->pHdl, 
			  (HWND) a.vpEnumPl->hWnd,
			   a.vpEnumPl->sessionId,
			   a.vpEnumPl->userData);
	   }
	   case VPUT_CREATESESSION:
	   {
		   return (DWORD) vpCreateSession(
			   a.vpCreateSes->pHdl,
			  (HWND) a.vpCreateSes->hWnd,
			   a.vpCreateSes->sessionName,
			   a.vpCreateSes->sessionFlags,
			   a.vpCreateSes->userData);
	   }
	   case VPUT_GETSESSIONINFO:
	   {
		   return vpGetSessionInfo(a.vpGetSesInfo->pSesHdl, 
			   a.vpGetSesInfo->sInfo);
	   }
	   case VPUT_GETSESSIONINFOBYID:
	   {
		   return vpGetSessionInfoById(a.vpGetSesInfo2->pHdl,
			  (HWND) a.vpGetSesInfo2->hWnd,
			   a.vpGetSesInfo2->sessionId,
			   a.vpGetSesInfo2->userData);
	   }
	   case VPUT_UPDATESESSIONDATA:
	   {
		   return vpUpdateSessionData(a.vpUpdSesData->pSesHdl,
			  a.vpUpdSesData->sessionData);
	   }
	   case VPUT_JOINSESSION:
	   {
		   return (DWORD) vpJoinSession(a.vpJoin->pHdl,
			   (HWND) a.vpJoin->hWnd,
			   a.vpJoin->sessionId,
			   a.vpJoin->playerName,
			   a.vpJoin->playerFlags,
			   a.vpJoin->userData);
	   }
	   case VPUT_ADDPLAYER:
	   {
		   return vpAddPlayer(a.vpAddPlayer->pSesHdl, 
			   a.vpAddPlayer->playerName,
			   a.vpAddPlayer->playerFlags,
			   a.vpAddPlayer->userData,
			   a.vpAddPlayer->playerId);
	   }
	   case VPUT_SENDDATA:
	   {
		   return vpSendData(a.vpSendData->pSesHdl,
			   a.vpSendData->toPlayerId,
			   a.vpSendData->fromPlayerId,
			   a.vpSendData->data,
			   a.vpSendData->dataLen,
			   a.vpSendData->sendFlags,
			   a.vpSendData->userData);
	   }
	   case VPUT_CLOSESESSION:
	   {
		   return vpCloseSession(a.vpCloseSes->pSesHdl,
			   a.vpCloseSes->userData);
	   }
	   case VPUT_GETADDRESSSTRING:
	   {
		   return vpGetAddressString(a.vpGetAddressStr->pHdl,
			  a.vpGetAddressStr->addr,
		      a.vpGetAddressStr->buf,
		      a.vpGetAddressStr->bufSize);
				  
				  
	   }
	   case VPUT_GETADDRESS:
	   {
		   return vpGetAddress(a.vpGetAddress->pHdl, 
			   a.vpGetAddress->addr);
	   }
	   case VPUT_ACKNOWLEDGE:
	   {
		   return vpAcknowledge(a.vpAck->pHdl, a.vpAck->pMsg);
	   }
	   case VPUT_INVITEPLAYER:
	   {
		   return vpInvitePlayer(a.vpKillPlayer->pSesHdl,
			   a.vpKillPlayer->playrId);
	   }
	   case VPUT_REJECTPLAYER:
	   {
		   return vpRejectPlayer(a.vpKillPlayer->pSesHdl,
			   a.vpKillPlayer->playrId);

	   }
	   case VPUT_KILLPLAYER:
	   {
		   return vpKillPlayer(a.vpKillPlayer->pSesHdl,
			   a.vpKillPlayer->playrId);

	   } 
	 }
	   
   /*
     * call 16bit DB services based on the function Id.
     */

 
    return( 0 );
}

// We need this routine to make Tools.h++ happy
extern "C" void exit(int x)
{
	abort();
}

/*
 * $Log: /vdmplay/ut/vput16.cpp $
 * 
 * 5     6/01/96 20:34 Vadim
 * Shows signs of life
 */

