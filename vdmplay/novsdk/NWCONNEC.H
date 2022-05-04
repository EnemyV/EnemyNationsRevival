/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWCONNECT_INC
#define NWCONNECT_INC

#ifndef NWCALDEF_INC
# include <nwcaldef.h>
#endif

#ifdef __N_USE_PRAGMA_PACK__
#pragma pack(1)
#elif defined(__BORLANDC__)
#pragma option -a-
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define C_SNAMESIZE 48
typedef struct
{
  NWCONN_HANDLE connID;
  WORD  connectFlags;
  WORD  sessionID;
  NWCONN_NUM  connNumber;
  BYTE  serverAddr[12];
  WORD  serverType;
  char  serverName[C_SNAMESIZE];
  WORD  clientType;
  char  clientName[C_SNAMESIZE];
} CONNECT_INFO;

typedef struct
{
   DWORD systemElapsedTime;
   BYTE bytesRead[6];
   BYTE bytesWritten[6];
   DWORD totalRequestPackets;
} CONN_USE;

typedef struct tNWINET_ADDR
{
  BYTE networkAddr[4];
  BYTE netNodeAddr[6];
  WORD socket;
  WORD connType;  /* 3.11 and above only: 0=not in use, 2=NCP, 3=AFP */
} NWINET_ADDR;

#define CONNECTION_AVAILABLE            0x0001
#define CONNECTION_PRIVATE              0x0002  /* obsolete */
#define CONNECTION_LOGGED_IN            0x0004
#define CONNECTION_LICENSED             0x0004
#define CONNECTION_BROADCAST_AVAILABLE  0x0008
#define CONNECTION_ABORTED              0x0010
#define CONNECTION_REFUSE_GEN_BROADCAST 0x0020
#define CONNECTION_BROADCASTS_DISABLED  0x0040
#define CONNECTION_PRIMARY              0x0080
#define CONNECTION_NDS                  0x0100
#define CONNECTION_PNW                  0x4000
#define CONNECTION_AUTHENTICATED        0x8000  /* obsolete */

/* the following are for NWGetConnInfo */
/* ALL is VLM, OS2 and NT - NOT NETX */
#define NW_CONN_TYPE           1   /* returns WORD (VLM) */
#define   NW_CONN_BIND    0x0031
#define   NW_CONN_NDS     0x0032
#define   NW_CONN_PNW     0x0033
#define NW_AUTHENTICATED       3  /* returns WORD = 1 if authenticated (ALL)*/
#define NW_PBURST              4  /* returns WORD = 1 if pburst (VLM) */
#define NW_VERSION             8  /* returns WORD (VLM)  */
#define NW_HARD_COUNT          9  /* returns WORD (VLM)  */
#define NW_CONN_NUM           13  /* returns WORD (ALL)  */
#define NW_TRAN_TYPE          15  /* returns WORD (VLM)  */
#define   NW_TRAN_IPX     0x0021
#define   NW_TRAN_TCP     0x0022
#define NW_SESSION_ID     0x8000  /* returns WORD) (VLM) */
#define NW_SERVER_ADDRESS 0x8001  /* returns 12 byte address (ALL) */
#define NW_SERVER_NAME    0x8002  /* returns 48 byte string  (ALL) */

NWCCODE NWAPI NWGetConnInfo(NWCONN_HANDLE conn, WORD type, void NWPTR data);

NWCCODE NWAPI NWLockConnection(NWCONN_HANDLE conn);

NWCCODE NWAPI NWGetConnectionUsageStats(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNumber,
  CONN_USE NWPTR statusBuffer);

NWCCODE NWAPI NWGetConnectionInformation(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNumber,
  char  NWPTR objectName,
  WORD  NWPTR objectType,
  DWORD NWPTR objectID,
  BYTE  NWPTR loginTime);

NWCCODE NWAPI NWGetInternetAddress(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNumber,
  BYTE NWPTR internetAddress);

NWCCODE NWAPI NWGetInetAddr(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNum,
  NWINET_ADDR NWPTR inetAddr);

void NWAPI NWGetMaximumConnections(WORD NWPTR maxConnections);

NWCCODE NWAPI NWGetConnectionList(
  WORD mode,
  NWCONN_HANDLE NWPTR connListBuffer,
  WORD connListSize,
  WORD NWPTR numConnections);

NWCCODE NWAPI NWGetConnectionStatus(
  NWCONN_HANDLE conn,
  CONNECT_INFO NWPTR connInfo,
  WORD connInfoSize);

NWCCODE NWAPI NWGetConnectionNumber(
  NWCONN_HANDLE conn,
  NWCONN_NUM NWPTR connNumber);

NWCCODE NWAPI NWClearConnectionNumber(
   NWCONN_HANDLE conn,
   NWCONN_NUM connNumber);

NWCCODE NWAPI NWGetDefaultConnectionID(NWCONN_HANDLE NWPTR conn);

#define NWGetConnectionID(a, b, c, d) NWGetConnectionHandle(a, b, c, d)

NWCCODE NWAPI NWGetConnectionHandle(
  BYTE NWPTR serverName,
  WORD mode,
  NWCONN_HANDLE NWPTR conn,
  WORD NWPTR connScope);

NWCCODE NWAPI NWSetPrimaryConnectionID(NWCONN_HANDLE conn);

NWCCODE NWAPI NWGetPrimaryConnectionID(NWCONN_HANDLE NWPTR conn);

NWCCODE NWAPI NWGetObjectConnectionNumbers(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  WORD NWPTR numConnections,
  NWCONN_NUM NWPTR connList,
  WORD maxConnections);

NWCCODE NWAPI NWGetConnListFromObject(
   NWCONN_HANDLE conn,
   DWORD objectID,
   DWORD searchConnNum,
   WORD NWPTR connListLen,
   DWORD NWPTR connList
);

#ifndef NWOS2
NWCCODE NWAPI NWIsIDInUse(NWCONN_HANDLE conn);
NWCCODE NWAPI NWGetPreferredServer(NWCONN_HANDLE NWPTR conn);
NWCCODE NWAPI NWSetPreferredServer(NWCONN_HANDLE conn);
#else
NWCCODE NWAPI NWResetConnectionConfig(DWORD flags);
#endif

#ifdef __cplusplus
}
#endif

#ifdef __N_USE_PRAGMA_PACK__
#pragma pack()
#elif defined(__BORLANDC__)
#pragma option -a.
#endif

#endif

/*
$Header:   K:\archives\src\sdkinc\nwconnec.h_v   1.3   08 Oct 1993 18:10:40   JWOODBUR  $
*/
