/*      (c) COPYRIGHT 1988-1990 by Novell, Inc.  All Rights Reserved.   */
#ifndef _NXT_H
  #define _NXT_H
  /*__________________________________________________________________________
       Definitions and structures  for the Netware API Communications logic
    _________________________________________________________________________*/

     #include <nwcaldef.h>

  /* assigned socket types */

  #define SOC_DIAGNOSTICS
  #define SHORT_LIVED        0x00
  #define LONG_LIVED         0xFF
  #define ENABLE_WATCHDOG    0xFF
  #define DISABLE_WATCHDOG   0x00

  typedef struct IPXAddress
   {
      BYTE    network[4];                    /* high-low */
      BYTE    node[6];                       /* high-low */
      BYTE    socket[2];                     /* high-low */
   }IPXAddress;


  typedef struct IPXHeader
   {
      WORD        checkSum;                  /* high-low */
      WORD        length;                    /* high-low */
      BYTE        transportControl;
      BYTE        packetType;
      IPXAddress  destination;
      IPXAddress  source;
   }IPXHeader;


  typedef struct SPXHeader
   {
      WORD        checksum;               /* high-low 1's complement */
      WORD        length;                 /* high-low unsigned int */
      BYTE        transportControl;
      BYTE        packetType;
      IPXAddress  destination;
      IPXAddress  source;
      BYTE        connectionControl;      /* bit flags */
      BYTE        dataStreamType;
      WORD        sourceConnectionID;     /* high-low unsigned */
      WORD        destConnectionID;       /* high-low unsigned */
      WORD        sequenceNumber;         /* high-low unsigned */
      WORD        acknowledgeNumber;      /* high-low unsigned */
      WORD        allocationNumber;       /* high-low unsigned */
   }SPXHeader;

   typedef struct CONNECTION_INFO
   {
      BYTE    connectionState;
      BYTE    connectionFlags;
      WORD    sourceConnectionID;               /* hi-lo */
      WORD    destinationConnectionID;          /* hi-lo */
      WORD    sequenceNumber;                   /* hi-lo */
      WORD    acknowledgeNumber;                /* hi-lo */
      WORD    allocationNumber;                 /* hi-lo */
      WORD    remoteAcknowledgeNumber;          /* hi-lo */
      WORD    remoteAllocationNumber;           /* hi-lo */
      WORD    connectionSocket;                 /* hi-lo */
      BYTE    immediateAddress[6];
      IPXAddress   destination;
      WORD    retransmissionCount;              /* hi-lo */
      WORD    estimatedRoundTripDelay;          /* hi-lo */
      WORD    retransmittedPackets;             /* hi-lo */
      WORD    suppressedPackets;                /* hi-lo */
   } CONNECTION_INFO;


  #ifndef ECB_STRUCTURES_DEFINED
    #define ECB_STRUCTURES_DEFINED

    typedef struct ECBFragment
     {
        void far *address;
        WORD    size;                       /* low-high */
     }ECBFragment;

    typedef struct ECB
     {
        void far *linkAddress;
        void (far *ESRAddress)();
        BYTE        inUseFlag;
        BYTE        completionCode;
        WORD        socketNumber;                   /* high-low */
        BYTE        IPXWorkspace[4];                /* N/A */
        BYTE        driverWorkspace[12];            /* N/A */
        BYTE        immediateAddress[6];            /* high-low */
        WORD        fragmentCount;                  /* low-high */
        ECBFragment fragmentDescriptor[2];
     }ECB;
  #endif /* ECB_STRUCTURES_DEFINED */



  #define SPX_IS_INSTALLED            0xFF
  #define SPX_NOT_INSTALLED           0x00
  #define SPX_CONNECTION_OK           0x00
  #define SPX_CONNECTION_STARTED      0x00
  #define SPX_CONNECTION_ESTABLISHED  0x00
  #define SPX_PACKET_SUCCESSFUL       0x00
  #define SPX_SOCKET_NOT_OPENED       0xFF
  #define SPX_MALFORMED_PACKET        0xFD
  #define SPX_PACKET_OVERFLOW         0xFD
  #define SPX_LISTEN_CANCELED         0xFC
  #define SPX_CONNECTION_TABLE_FULL   0xEF
  #define SPX_INVALID_CONNECTION      0xEE
  #define SPX_NO_ANSWER_FROM_TARGET   0xED
  #define SPX_CONNECTION_FAILED       0xED
  #define SPX_CONNECTION_TERMINATED   0xED
  #define SPX_TERMINATED_POORLY       0xEC


#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

   extern int        IPXCancelEvent(ECB far *eventControlBlock);

   extern void cdecl IPXCloseSocket(WORD socketNumber);

   extern void       IPXDisconnectFromTarget(BYTE far *networkAddress);

   extern void       IPXGetInternetworkAddress(BYTE far *networkAddress);

   extern WORD cdecl IPXGetIntervalMarker(void);

   extern int  cdecl IPXGetLocalTarget(BYTE far *networkAddress,
                                       BYTE far *immediateAddress,
                                       int far *transportTime);

   extern WORD cdecl IPXGetMaxPacketSize(void);

   extern BYTE cdecl IPXInitialize(void);

   extern void       IPXListenForPacket(ECB far *eventControlBlock);

   extern int  cdecl IPXOpenSocket(BYTE far *socketNumber,
                                   BYTE socketType);

   /* internal procedure */
   extern int  cdecl _IPXPacket(ECB far *eventControlBlock,
                                WORD functionNumber);

   /* internal procedure */
   extern void cdecl _IPXrequest(WORD timeDelay,
                                 ECB far *eventControlBlock,
                                 WORD functionNumber);

   extern void cdecl IPXRelinquishControl(void);

   extern void       IPXScheduleIPXEvent(WORD timeUnits,
                                         ECB far *eventControlBlock);

   /* internal procedure */
   extern void       IPXScheduleSpecialEvent(WORD timeUnits,
                                             ECB far *eventControlBlock);

   extern void       IPXSendPacket(ECB far *eventControlBlock);

   extern void cdecl SPXAbortConnection(WORD connectionID);

   extern int  cdecl SPXEstablishConnection(BYTE retryCount,
                                            BYTE watchDog,
                                            WORD far * connectionID, 
                                            ECB far *eventControlBlock);

   extern int        SPXGetConnectionStatus(WORD connectionID,
                                            CONNECTION_INFO far *connectionInfo);

   extern int  cdecl SPXInitialize(BYTE far *majorRevisionNumber,
                                   BYTE far *minorRevisionNumber, 
                                   WORD far *maxConnections,
                                   WORD far *availableConnection);

   extern void cdecl SPXListenForConnection(BYTE retryCount,
                                            BYTE watchDog,
                                            ECB far *eventControlBlock);

   extern void cdecl SPXListenForSequencedPacket(ECB far *eventControlBlock);

   /* internal procedure */
   extern int  cdecl _SPXrequest(WORD connectionID,
                                 ECB far *eventControlBlock,
                                 WORD functionNumber);

   extern void       SPXSendSequencedPacket(WORD connectionID,
                                            ECB far *eventControlBlock);

   extern void       SPXTerminateConnection(WORD connectionID,
                                            ECB far *eventControlBlock);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif /* _NXT_H */
