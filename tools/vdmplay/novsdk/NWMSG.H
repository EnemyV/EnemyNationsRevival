/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWMSG_INC
#define NWMSG_INC

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

NWCCODE NWAPI NWDisableBroadcasts(NWCONN_HANDLE conn);
NWCCODE NWAPI NWEnableBroadcasts(NWCONN_HANDLE conn);

NWCCODE NWAPI NWSendBroadcastMessage(
  NWCONN_HANDLE conn,
  char NWPTR message,
  WORD connCount,
  WORD NWPTR connList,
  BYTE NWPTR resultList);

NWCCODE NWAPI NWGetBroadcastMessage(
  NWCONN_HANDLE conn,
  char NWPTR message);

NWCCODE NWAPI NWGetBroadcastMode(NWCONN_HANDLE conn, WORD NWPTR mode);
NWCCODE NWAPI NWSetBroadcastMode(NWCONN_HANDLE conn, WORD mode);
NWCCODE NWAPI NWBroadcastToConsole(NWCONN_HANDLE conn, char NWPTR message);

NWCCODE NWAPI NWSendConsoleBroadcast(
  NWCONN_HANDLE conn,
  char NWPTR message,
  WORD connCount,
  WORD NWPTR connList);

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
$Header:   K:\archives\src\sdkinc\nwmsg.h_v   1.1   09 Aug 1993 12:04:44   JWOODBUR  $
*/
