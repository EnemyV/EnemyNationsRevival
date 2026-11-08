/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
/****************************************************************************
 лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
 BEGIN_MANUAL_ENTRY ( nwndscon.h() )

 SOURCE MODULE: nwndscon.h

 API NAME     :

 SYNTAX       :

 DESCRIPTION  :

 PARAMETERS   :   -> input          <-output

 INCLUDE      :

 RETURN       :

 MODIFICATIONS:

     January 20, 1992 - Art Nevarez, rolled-in from the OS/2 team.

 NCP CALLS
 ---------

 API CALLS
 ---------

 SEE ALSO:

 @Q1 HANDLES STRINGS? (Y\N):

 @Q2 HANDLES PATH? (Y\N):

 END_MANUAL_ENTRY
****************************************************************************/
#ifndef NWNDSCON_INC
#define NWNDSCON_INC

#define NWNDS_CONNECTION         0x0001
#define NWNDS_LICENSED           0x0002
#define NWNDS_AUTHENTICATED      0x0004
#define NWNDS_PACKET_BURST_AVAIL 0x0001
#define NWNDS_NEEDED_MAX_IO      0x0040
#define SYSTEM_LOCK              0x0
#define TASK_LOCK                0x4
#define SYSTEM_DISCONNECT        0x0
#define TASK_DISCONNECT          0x1

#define ALLREADY_ATTACHED        0x1
#define ATTACHED_NOT_AUTH        0X2
#define ATTACHED_AND_AUTH        0X4


#ifdef __cplusplus
	extern "C" {
#endif

NWCCODE NWAPI NWGetNearestDirectoryService
(
   NWCONN_HANDLE NWFAR *connID
);

NWCCODE NWAPI NWSetDefaultNameContext
(
   WORD contextLength,
   BYTE NWFAR *context
);

NWCCODE NWAPI NWGetDefaultNameContext
(
   WORD bufferSize,
   BYTE NWFAR *context
);

NWCCODE NWAPI NWGetConnectionIDFromAddress
(
   BYTE            transType,
   DWORD           transLen,
   BYTE            NWFAR *transBuf,
   NWCONN_HANDLE   NWFAR *connID
);

NWCCODE NWAPI NWDSGetConnectionInfo
(
   NWCONN_HANDLE  connID,
   BYTE NWFAR     *connStatus,
   BYTE NWFAR     *connType,
   BYTE NWFAR     *serverFlags,
   BYTE NWFAR     *serverName,
   BYTE NWFAR     *transType,
   DWORD NWFAR    *transLen,
   BYTE NWFAR     *transBuf,
   WORD NWFAR     *distance,
   WORD NWFAR     *maxPacketSize
);

NWCCODE NWAPI NWDSGetConnectionSlot
(
   BYTE         connType,
   BYTE         transType,
   DWORD        transLen,
   BYTE NWFAR   *transBuf,
   NWCONN_HANDLE NWFAR *connID
);

NWCCODE NWAPI NWGetPreferredDSServer
(
   NWCONN_HANDLE NWFAR *connID
);

NWCCODE NWAPI NWSetPreferredDSTree
(
   WORD length,
   BYTE NWFAR *treeName
);

NWCCODE NWAPI NWGetNumConnections
(
   WORD NWFAR *numConnections
);

NWCCODE NWAPI NWDSGetMonitoredConnection
(
   NWCONN_HANDLE NWFAR *connID
);

NWCCODE NWAPI NWDSSetMonitoredConnection
(
   NWCONN_HANDLE connID
);

NWCCODE NWAPI NWGetConnectionIDFromName
(
   DWORD          nameLen,
   BYTE           NWFAR *name,
   NWCONN_HANDLE  NWFAR *connID
);

NWCCODE NWAPI NWIsDSAuthenticated(void);

NWCCODE NWAPI NWDSLockConnection
(
   NWCONN_HANDLE connID
);

NWCCODE NWAPI NWDSUnlockConnection
(
   NWCONN_HANDLE connID
);

NWCCODE NWAPI NWGetPreferredConnName
(
   BYTE NWFAR *preferredName,
   BYTE NWFAR *preferredType
);

NWCCODE NWAPI NWFreeConnectionSlot
(
   NWCONN_HANDLE  connID,
   BYTE           disconnectType
);

NWCCODE NWAPI NWGetNextConnectionID
(
   NWCONN_HANDLE NWFAR *connID
);

#ifdef __cplusplus
	}
#endif

#endif
