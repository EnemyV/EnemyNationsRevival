#ifndef __IPX16STB_H__
#define __IPX16STB_H__

#define USE_IPX16STUBS
#ifdef USE_IPX16STUBS


struct VP_IPX16_CALLS
{


    int (FAR PASCAL *  pfnIPXCancelEvent[2])(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock);

    void (FAR PASCAL * pfnIPXCloseSocket[2])(
                                       DWORD IPXTaskID,
                                       WORD socket);

    void (FAR PASCAL * pfnIPXDisconnectFromTarget[2])(
                                       DWORD IPXTaskID, 
                                       BYTE FAR *internetAddress);

    void (FAR PASCAL * pfnIPXGetInternetworkAddress[2])(
                                       DWORD IPXTaskID, 
                                       BYTE FAR *internetAddress);

    WORD (FAR PASCAL * pfnIPXGetIntervalMarker[2])(
                                       DWORD IPXTaskID );

    int ( FAR PASCAL * pfnIPXGetLocalTarget[2])(
                                       DWORD IPXTaskID, 
                                       BYTE FAR *destination, 
                                       BYTE FAR *immediateAddress, 
                                       int FAR *transportTime);

    WORD (FAR PASCAL * pfnIPXGetLocalTargetAsync[2])(
                                       AGLT FAR *listenAGLT,
                                       AGLT FAR *sendAGLT,
                                       BYTE FAR *destAddr);

    WORD (FAR PASCAL * pfnIPXGetMaxPacketSize[2])(void);

    WORD (FAR PASCAL * pfnIPXGetRetryCount[2])(void);

    int ( FAR PASCAL * pfnIPXInitialize[2])(
                                       DWORD FAR *IPXTaskID, 
                                       WORD maxECBs, 
                                       WORD maxPacketSize);

    int ( FAR PASCAL * pfnIPXSPXDeinit[2])(DWORD IPXTaskID);

    void (FAR PASCAL * pfnIPXListenForPacket[2])(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock);

    int ( FAR PASCAL * pfnIPXOpenSocket[2])(
                                       DWORD IPXTaskID,
                                       WORD FAR *socket, 
                                       BYTE socketType);

    void (FAR PASCAL * pfnIPXRelinquishControl[2])(void);

    void (FAR PASCAL * pfnIPXScheduleIPXEvent[2])(
                                       DWORD IPXTaskID, 
                                       WORD timeDelay, 
                                       ECB FAR *eventControlBlock);

    void (FAR PASCAL * pfnIPXSendPacket[2])(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock);

    void (FAR PASCAL * pfnIPXYield[2])(void);

  
    void (FAR PASCAL * pfnSPXAbortConnection[2])(
                                       WORD SPXConnID);

    int ( FAR PASCAL * pfnSPXEstablishConnection[2])(
                                       DWORD IPXTaskID, 
                                       BYTE retryCount, 
                                       BYTE watchDog, 
                                       WORD FAR *SPXConnID, 
                                       ECB FAR *eventControlBlock);

    int ( FAR PASCAL * pfnSPXGetConnectionStatus[2])(
                                       DWORD IPXTaskID, 
                                       WORD SPXConnID, 
                                       CONNECTION_INFO FAR *connectionInfo);

    int ( FAR PASCAL * pfnSPXInitialize[2])(  
                                       DWORD FAR *IPXTaskID, 
                                       WORD maxECBs, 
                                       WORD maxPacketSize, 
                                       BYTE FAR *majorRevisionNumber, 
                                       BYTE FAR *minorRevisionNumber, 
                                       WORD FAR *maxConnections, 
                                       WORD FAR *availableConnections);

    void (FAR PASCAL * pfnSPXListenForConnection[2])(
                                       DWORD IPXTaskID, 
                                       BYTE retryCount, 
                                       BYTE watchDog, 
                                       ECB FAR *eventControlBlock);

    void (FAR PASCAL * pfnSPXListenForSequencedPacket[2])(
                                       DWORD IPXTaskID, 
                                       ECB FAR *eventControlBlock);

    void (FAR PASCAL * pfnSPXSendSequencedPacket[2])(
                                       DWORD IPXTaskID, 
                                       WORD SPXConnID, 
                                       ECB FAR *eventControlBlock);

    void (FAR PASCAL * pfnSPXTerminateConnection[2])(
                                       DWORD IPXTaskID, 
                                       WORD SPXConnID, 
                                       ECB FAR *eventControlBlock);


};


#ifndef IPX16STUBS
#define IPXCancelEvent _vpIPX16.pfnIPXCancelEvent[0]
#define IPXCloseSocket _vpIPX16.pfnIPXCloseSocket[0]
#define IPXDisconnectFromTarget _vpIPX16.pfnIPXDisconnectFromTarget[0]
#define IPXGetInternetworkAddress _vpIPX16.pfnIPXGetInternetworkAddress[0]
#define IPXGetIntervalMarker _vpIPX16.pfnIPXGetIntervalMarker[0]
#define IPXGetLocalTarget _vpIPX16.pfnIPXGetLocalTarget[0]
#define IPXGetLocalTargetAsync _vpIPX16.pfnIPXGetLocalTargetAsync[0]
#define IPXGetMaxPacketSize _vpIPX16.pfnIPXGetMaxPacketSize[0]
#define IPXGetRetryCount _vpIPX16.pfnIPXGetRetryCount[0]
#define IPXInitialize _vpIPX16.pfnIPXInitialize[0]
#define IPXSPXDeinit _vpIPX16.pfnIPXSPXDeinit[0]
#define IPXListenForPacket _vpIPX16.pfnIPXListenForPacket[0]
#define IPXOpenSocket _vpIPX16.pfnIPXOpenSocket[0]
#define IPXRelinquishControl _vpIPX16.pfnIPXRelinquishControl[0]
#define IPXScheduleIPXEvent _vpIPX16.pfnIPXScheduleIPXEvent[0]
#define IPXSendPacket _vpIPX16.pfnIPXSendPacket[0]
#define IPXYield _vpIPX16.pfnIPXYield[0]
#define SPXAbortConnection _vpIPX16.pfnSPXAbortConnection[0]
#define SPXEstablishConnection _vpIPX16.pfnSPXEstablishConnection[0]
#define SPXGetConnectionStatus _vpIPX16.pfnSPXGetConnectionStatus[0]
#define SPXInitialize _vpIPX16.pfnSPXInitialize[0]
#define SPXListenForConnection _vpIPX16.pfnSPXListenForConnection[0]
#define SPXListenForSequencedPacket _vpIPX16.pfnSPXListenForSequencedPacket[0]
#define SPXSendSequencedPacket _vpIPX16.pfnSPXSendSequencedPacket[0]
#define SPXTerminateConnection _vpIPX16.pfnSPXTerminateConnection[0]
#endif

extern VP_IPX16_CALLS _vpIPX16;
extern VP_IPX16_CALLS _vpIPX16Init;

class _VP_IPX16_STATE
{
public:
	HINSTANCE m_hInstipx;      // handle of nwipxspx.dll
	_VP_IPX16_STATE() : m_hInstipx(NULL) {  }
	virtual ~_VP_IPX16_STATE();
};


#endif


#endif 