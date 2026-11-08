/*      COPYRIGHT (c) 1989 by Novell, Inc.  All Rights Reserved.   */
#ifndef SAP_H
#define SAP_H

#ifndef _NXT_H
  #include <nxtw.h>
#endif /* _NXT_H */

/*____________________________________________________________________________


     Header file that includes the definitions and structures needed for

     the NETWARE API Service Advertising Protocals Logic

 ___________________________________________________________________________*/


   #ifndef TRUE
     #define TRUE                  1
     #define FALSE                 0
   #endif /* TRUE */

   #define SAP_SOCKET           0x0452
   #define SAP_PACKET_TYPE          2
   #define ONE_MINUTE           (60 * 18)
   #define IPX_EVENT_CANCELED    0xFC

   #ifndef SUCCESSFUL
   #define SUCCESSFUL            0x00
   #endif

   #define FAILURE                 (-1)
   #define MIN(a,b)          ((a) < (b) ? (a) : (b))
   #define NOT_SUPPORTED            1
   #define INVALID_QUERY_TYPE       2
   #define PERIODIC_ID_PACKET       2
   #define RESPONSE_ID_PACKET       4
   #define MAX_NUM_APPS            10


#define OPEN_SAP_SOCKET_ERROR          0x60
#define TOO_MANY_SERVERS_ON_NODE       0x61
#define SAP_MEMORY_ALLOC_ERROR         0x62
#define SERVER_ALREADY_ADVERTISING     0x63
#define MULTIPLE_INSTANCES_ON_SOCKET   0x64
#define SAP_MEMORY_FREE_ERROR          0x65
#define SAP_MEMORY_UNLOCK_ERROR        0x66
#define SAP_ECB_NOT_CANCELLED          0x67
#define SAP_SERVER_NOT_ADVERTISING     0x68

/*__________________________________________________________________________*/




   typedef struct SAPHeader
    {
       WORD          checksum;             /* high-low 1's complement */
       WORD          length;               /* high-low unsigned int */
       BYTE          transportControl;
       BYTE          packetType;
       IPXAddress    destination;
       IPXAddress    source;
       WORD          SAPPacketType;        /* 2 or 4 */
       WORD          serverType;           /* assigned by Novell */
       BYTE          serverName[48];       /* VAP name */
       IPXAddress    serverAddress;        /* server internetwork address */
       WORD          interveningNetworks;  /* # of networks packet must traverse */
    } SAPHeader;




/*__________________________________________________________________________*/





   typedef struct SAPQueryPacket
    {
       WORD          checksum;             /* high-low 1's complement */
       WORD          length;               /* high-low unsigned int */
       BYTE          transportControl;
       BYTE          packetType;
       IPXAddress    destination;
       IPXAddress    source;
       WORD          queryType;            /* high-low, 1 or 3 */
       WORD          serverType;           /* high-low, assigned by Novell */
    } SAPQueryPacket;



/*__________________________________________________________________________*/


   /* SAP packet */

   typedef struct
    {
       ECB         theECB;
       SAPHeader   packet;
    } SAP_BUFFER;



/*__________________________________________________________________________*/



   typedef struct
    {
       IPXHeader    Header;
       WORD         ResponseType;         /* HI - LO  */
       WORD         ServerType;           /* HI - LO  */
       BYTE         ServerName[48];
       BYTE         Network[4];           /* hi - lo  */
       BYTE         Node[6];              /* hi - lo  */
       BYTE         Socket[2];            /* hi - lo  */
       WORD         InterveningNetworks;  /* hi - lo  */
    } SAP;



/*__________________________________________________________________________*/


   typedef struct
    {
       ECB               theECB;
       SAPQueryPacket    SAPq;
    } SEND_PACKET;


/*__________________________________________________________________________*/



   typedef struct
    {
       ECB   theECB;
       SAP   SB;
    } RECEIVE_PACKET;

/*____________________________  PROTOTYPES  ________________________________*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

   extern void far pascal Advertiser(void);
   extern WORD far pascal AdvertiseService(
                                 WORD     sType,
                                 char far *sName,
                                 WORD far *sSocket );
   extern WORD FAR PASCAL QueryServices(
                                 WORD qType,
                                 WORD sType,
                                 WORD returnSize,
                                 SAP  FAR *serviceBuffer );
   extern void far pascal RespondToLocalQuery(void);
   extern WORD far pascal ShutdownSAP(
                                 char far *serverName);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif /* SAP_H */
