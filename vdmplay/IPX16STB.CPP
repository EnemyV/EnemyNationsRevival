#include "stdafx.h"

#define NWWIN
#define IPX16STUBS
#include <nwipxspx.h>
#include "ipx16stb.h"

static const char ipxdll[] = "NWIPXSPX.DLL";

static HINSTANCE hIpxDLL;

void vpUnloadIpxDll()
{
	VPENTER(vpUnloadIpxDll);
	if (hIpxDLL != NULL)
		FreeLibrary(hIpxDLL);
	VPEXIT();
}

BOOL vpLoadIpxDll()
{
	BOOL ret;
	VPENTER(vpLoadIpxDll);
	hIpxDLL = LoadLibrary(ipxdll);
	ret =(UINT) hIpxDLL > 32;
	VPEXITBOOL(ret);
	return ret;
}



#ifdef USE_IPX16STUBS

_VP_IPX16_STATE _vpIPX16state;


HINSTANCE VPLoadDll(HINSTANCE& hInst, LPCSTR lpszDLL, FARPROC* pProcPtrs, LPCSTR lpszProcName);

inline HINSTANCE VPLoadIpx16(FARPROC* proc, LPCSTR lpsz)
	{ return VPLoadDll(_vpIPX16state.m_hInstipx,  ipxdll , proc, lpsz); }
#define IPX16LOAD(x) \
	VPLoadIpx16((FARPROC*)_vpIPX16.pfn##x, #x)
#define IPX16CALLR(x,a) IPX16LOAD(x); \
	return _vpIPX16.pfn##x[1] a

#define IPX16CALL(x,a) IPX16LOAD(x); \
	 _vpIPX16.pfn##x[1] a
	
	


extern "C"
{

int FAR PASCAL  VPThunkIPXCancelEvent(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock)
									  
{
	IPX16CALLR(IPXCancelEvent, (IPXTaskID, eventControlBlock));
}
	

void FAR PASCAL VPThunkIPXCloseSocket(
                                       DWORD IPXTaskID,
                                       WORD socket)
{
	IPX16CALL(IPXCloseSocket,(IPXTaskID, socket));
}
	

void FAR PASCAL VPThunkIPXDisconnectFromTarget(
                                       DWORD IPXTaskID, 
                                       BYTE FAR *internetAddress)
{
	IPX16LOAD(IPXDisconnectFromTarget);
		   
	_vpIPX16.pfnIPXDisconnectFromTarget[1](
                                        IPXTaskID, 
                                        internetAddress);
}
	

void FAR PASCAL VPThunkIPXGetInternetworkAddress(
                                       DWORD IPXTaskID, 
                                       BYTE FAR *internetAddress)
{
	IPX16LOAD(IPXGetInternetworkAddress);
		   
	_vpIPX16.pfnIPXGetInternetworkAddress[1](
                                       IPXTaskID, 
                                       internetAddress);
}
	

WORD FAR PASCAL VPThunkIPXGetIntervalMarker(
                                       DWORD IPXTaskID )
{
	IPX16LOAD(IPXGetIntervalMarker);
		   
	return _vpIPX16.pfnIPXGetIntervalMarker[1]( IPXTaskID );
}
	

int  FAR PASCAL VPThunkIPXGetLocalTarget(
                                       DWORD IPXTaskID, 
                                       BYTE FAR *destination, 
                                       BYTE FAR *immediateAddress, 
                                       int FAR *transportTime)
{
	IPX16LOAD(IPXGetLocalTarget);
		   
	return _vpIPX16.pfnIPXGetLocalTarget[1](
                                        IPXTaskID, 
                                        destination, 
                                        immediateAddress, 
                                        transportTime);
}
	

WORD FAR PASCAL VPThunkIPXGetLocalTargetAsync(
                                       AGLT FAR *listenAGLT,
                                       AGLT FAR *sendAGLT,
                                       BYTE FAR *destAddr)
{
	IPX16LOAD(IPXGetLocalTargetAsync);
		   
	return _vpIPX16.pfnIPXGetLocalTargetAsync[1](
                                       listenAGLT,
                                       sendAGLT,
                                       destAddr);
}
	

WORD FAR PASCAL VPThunkIPXGetMaxPacketSize(void)
{
	
	IPX16LOAD(IPXGetMaxPacketSize);
	
	return _vpIPX16.pfnIPXGetMaxPacketSize[1]();
}


WORD FAR PASCAL VPThunkIPXGetRetryCount(void)
{
	IPX16LOAD(IPXGetRetryCount);
	
	return _vpIPX16.pfnIPXGetRetryCount[1]();

}


int  FAR PASCAL VPThunkIPXInitialize(
                                       DWORD FAR *IPXTaskID, 
                                       WORD maxECBs, 
                                       WORD maxPacketSize)
{
	IPX16LOAD(IPXInitialize);
		   
	return _vpIPX16.pfnIPXInitialize[1](
                                       IPXTaskID, 
                                       maxECBs, 
                                       maxPacketSize);
}
	

int  FAR PASCAL VPThunkIPXSPXDeinit(DWORD IPXTaskID)
{
	IPX16LOAD(IPXSPXDeinit);
	int ret = _vpIPX16.pfnIPXSPXDeinit[1](IPXTaskID);
	
	_vpIPX16 = _vpIPX16Init;
	FreeLibrary(_vpIPX16state.m_hInstipx);
	_vpIPX16state.m_hInstipx = NULL; 
	return ret;
	
}
	

void FAR PASCAL VPThunkIPXListenForPacket(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(IPXListenForPacket);
		   
	_vpIPX16.pfnIPXListenForPacket[1](IPXTaskID, 
                                       eventControlBlock);
}
	

int  FAR PASCAL VPThunkIPXOpenSocket(
                                       DWORD IPXTaskID,
                                       WORD FAR *socket, 
                                       BYTE socketType)
{
	IPX16LOAD(IPXOpenSocket);
		   
	return _vpIPX16.pfnIPXOpenSocket[1](
                                       IPXTaskID,
                                       socket, 
                                       socketType);
}
	

void FAR PASCAL VPThunkIPXRelinquishControl(void)
{
	
	IPX16LOAD(IPXRelinquishControl);
	
	_vpIPX16.pfnIPXRelinquishControl[1]();
}

void FAR PASCAL VPThunkIPXScheduleIPXEvent(
                                       DWORD IPXTaskID, 
                                       WORD timeDelay, 
                                       ECB FAR *eventControlBlock)
{
   IPX16LOAD(IPXScheduleIPXEvent);
		   
   _vpIPX16.pfnIPXScheduleIPXEvent[1](
                                       IPXTaskID, 
                                       timeDelay, 
                                       eventControlBlock);
}
	

void FAR PASCAL VPThunkIPXSendPacket(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(IPXSendPacket);
		   
	_vpIPX16.pfnIPXSendPacket[1](
                                IPXTaskID, 
                                eventControlBlock);
}
	

void FAR PASCAL VPThunkIPXYield(void)
{
	IPX16LOAD(IPXYield);
		   
	_vpIPX16.pfnIPXYield[1]();
}


void FAR PASCAL VPThunkSPXAbortConnection(
                                       WORD SPXConnID)
{
   IPX16LOAD(SPXAbortConnection);
		   
   _vpIPX16.pfnSPXAbortConnection[1](SPXConnID);
}
	

FAR PASCAL VPThunkSPXEstablishConnection(
                                       DWORD IPXTaskID, 
                                       BYTE retryCount, 
                                       BYTE watchDog, 
                                       WORD FAR *SPXConnID, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(SPXEstablishConnection);
		   
	return _vpIPX16.pfnSPXEstablishConnection[1](
                                       IPXTaskID, 
                                       retryCount, 
                                       watchDog, 
                                       SPXConnID, 
                                       eventControlBlock);
}
	

int  FAR PASCAL VPThunkSPXGetConnectionStatus(
                                       DWORD IPXTaskID, 
                                       WORD SPXConnID, 
                                       CONNECTION_INFO FAR *connectionInfo)
{
	IPX16LOAD(SPXGetConnectionStatus);
		   
	return _vpIPX16.pfnSPXGetConnectionStatus[1](
                                       IPXTaskID, 
                                       SPXConnID, 
                                       connectionInfo);
}
	

int FAR PASCAL VPThunkSPXInitialize(  
                                       DWORD FAR *IPXTaskID, 
                                       WORD maxECBs, 
                                       WORD maxPacketSize, 
                                       BYTE FAR *majorRevisionNumber, 
                                       BYTE FAR *minorRevisionNumber, 
                                       WORD FAR *maxConnections, 
                                       WORD FAR *availableConnections)
{
	IPX16LOAD(SPXInitialize);
		   
	return _vpIPX16.pfnSPXInitialize[1](  
                                        IPXTaskID, 
                                        maxECBs, 
                                        maxPacketSize, 
                                        majorRevisionNumber, 
                                        minorRevisionNumber, 
                                        maxConnections, 
                                        availableConnections);
}
	

void FAR PASCAL VPThunkSPXListenForConnection(
                                       DWORD IPXTaskID, 
                                       BYTE retryCount, 
                                       BYTE watchDog, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(SPXListenForConnection);
		   
	_vpIPX16.pfnSPXListenForConnection[1](
                                        IPXTaskID, 
                                        retryCount, 
                                        watchDog, 
                                        eventControlBlock);
}
	

void FAR PASCAL VPThunkSPXListenForSequencedPacket(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(SPXListenForSequencedPacket);
		   
	_vpIPX16.pfnSPXListenForSequencedPacket[1](
                                       IPXTaskID, 
                                       eventControlBlock);
}
	

void FAR PASCAL VPThunkSPXSendSequencedPacket(
                                       DWORD IPXTaskID, 
                                       WORD SPXConnID, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(SPXSendSequencedPacket);
		   
	_vpIPX16.pfnSPXSendSequencedPacket[1](IPXTaskID, 
                                        SPXConnID, 
                                       eventControlBlock);
}
	

void FAR PASCAL VPThunkSPXTerminateConnection(
                                       DWORD IPXTaskID, 
                                       WORD SPXConnID, 
                                       ECB FAR *eventControlBlock)
{
	IPX16LOAD(SPXTerminateConnection);
 		   
	_vpIPX16.pfnSPXTerminateConnection[1](IPXTaskID, 
                                       SPXConnID, 
                                       eventControlBlock);
}
	



}

struct VP_IPX16_CALLS _vpIPX16 = 
{
 { VPThunkIPXCancelEvent, 0 },
 { VPThunkIPXCloseSocket, 0 },
 { VPThunkIPXDisconnectFromTarget, 0 },
 { VPThunkIPXGetInternetworkAddress, 0 },
 { VPThunkIPXGetIntervalMarker, 0 },
 { VPThunkIPXGetLocalTarget, 0 },
 { VPThunkIPXGetLocalTargetAsync, 0 },
 { VPThunkIPXGetMaxPacketSize, 0 },
 { VPThunkIPXGetRetryCount, 0 },
 { VPThunkIPXInitialize, 0 },
 { VPThunkIPXSPXDeinit, 0 },
 { VPThunkIPXListenForPacket, 0 },
 { VPThunkIPXOpenSocket, 0 },
 { VPThunkIPXRelinquishControl, 0 },
 { VPThunkIPXScheduleIPXEvent, 0 },
 { VPThunkIPXSendPacket, 0 },
 { VPThunkIPXYield, 0 },
 { VPThunkSPXAbortConnection, 0 },
 { VPThunkSPXEstablishConnection, 0 },
 { VPThunkSPXGetConnectionStatus, 0 },
 { VPThunkSPXInitialize, 0 },
 { VPThunkSPXListenForConnection, 0 },
 { VPThunkSPXListenForSequencedPacket, 0 },
 { VPThunkSPXSendSequencedPacket, 0 },
 { VPThunkSPXTerminateConnection, 0 }

};

VP_IPX16_CALLS _vpIPX16Init = 
{
 { VPThunkIPXCancelEvent, 0 },
 { VPThunkIPXCloseSocket, 0 },
 { VPThunkIPXDisconnectFromTarget, 0 },
 { VPThunkIPXGetInternetworkAddress, 0 },
 { VPThunkIPXGetIntervalMarker, 0 },
 { VPThunkIPXGetLocalTarget, 0 },
 { VPThunkIPXGetLocalTargetAsync, 0 },
 { VPThunkIPXGetMaxPacketSize, 0 },
 { VPThunkIPXGetRetryCount, 0 },
 { VPThunkIPXInitialize, 0 },
 { VPThunkIPXSPXDeinit, 0 },
 { VPThunkIPXListenForPacket, 0 },
 { VPThunkIPXOpenSocket, 0 },
 { VPThunkIPXRelinquishControl, 0 },
 { VPThunkIPXScheduleIPXEvent, 0 },
 { VPThunkIPXSendPacket, 0 },
 { VPThunkIPXYield, 0 },
 { VPThunkSPXAbortConnection, 0 },
 { VPThunkSPXEstablishConnection, 0 },
 { VPThunkSPXGetConnectionStatus, 0 },
 { VPThunkSPXInitialize, 0 },
 { VPThunkSPXListenForConnection, 0 },
 { VPThunkSPXListenForSequencedPacket, 0 },
 { VPThunkSPXSendSequencedPacket, 0 },
 { VPThunkSPXTerminateConnection, 0 }

};

_VP_IPX16_STATE::~_VP_IPX16_STATE()
{
	if (m_hInstipx != NULL)
		FreeLibrary(m_hInstipx);       // handle of WSOCK32.DLL
}



#endif 
