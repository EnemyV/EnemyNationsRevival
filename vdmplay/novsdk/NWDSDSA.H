/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef  _NWDSDSA_HEADER_
#define  _NWDSDSA_HEADER_

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif

#ifndef  _NWDSDC_HEADER_
#include <nwdsdc.h>
#endif

#ifndef NWCONNECT_INC
#include <nwconnec.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE NWAPI NWDSAddObject
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   int32 NWFAR       *iterationHandle,
   uint8             more,
   Buf_T NWFAR       *objectInfo
);

NWDSCCODE NWAPI NWDSBackupObject
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   int32 NWFAR       *iterationHandle,
   Buf_T NWFAR       *objectInfo
);

NWDSCCODE NWAPI NWDSCompare
(
   NWDSContextHandle context,
   char  NWFAR       *object,
   Buf_T NWFAR       *buf,
   uint8 NWFAR       *matched
);

/*-----------------------------------------------------------------------------
 *    In the case that objectName is a partition root, the partitionRootObject
 *    is the same as the objectName
 */

NWDSCCODE NWAPI NWDSGetPartitionRoot
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   char  NWFAR       *partitionRoot
);

NWDSCCODE NWAPI NWDSList
(
   NWDSContextHandle context,
   char  NWFAR       *object,
   int32 NWFAR       *iterationHandle,
   Buf_T NWFAR       *subordinates
);

NWDSCCODE NWAPI NWDSListContainers
(
   NWDSContextHandle context,
   char  NWFAR       *object,
   int32 NWFAR       *iterationHandle,
   Buf_T NWFAR       *subordinates
);

NWDSCCODE NWAPI NWDSListByClassAndName
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   char  NWFAR       *className,
   char  NWFAR       *subordinateName,
   int32 NWFAR       *iterationHandle,
   Buf_T NWFAR       *subordinates
);

NWDSCCODE NWAPI NWDSGetCountByClassAndName
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   char  NWFAR       *className,
   char  NWFAR       *subordinateName,
   int32 NWFAR       *count
);

NWDSCCODE NWAPI NWDSMapIDToName
(
   NWDSContextHandle    context,
   NWCONN_HANDLE     conn,
   uint32   objectID,
   char     NWFAR *object
);

NWDSCCODE NWAPI NWDSMapNameToID
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   char     NWFAR    *object,
   uint32   NWFAR    *objectID
);

NWDSCCODE NWAPI NWDSModifyObject
(
   NWDSContextHandle context,
   char     NWFAR    *objectName,
   int32    NWFAR    *iterationHandle,
   uint8             more,
   Buf_T    NWFAR    *changes
);

NWDSCCODE NWAPI NWDSModifyDN
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   char  NWFAR       *newDN,
   uint8             deleteOldRDN
);

NWDSCCODE NWAPI NWDSModifyRDN
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   char  NWFAR       *newDN,
   uint8             deleteOldRDN
);

NWDSCCODE NWAPI NWDSMoveObject
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   char  NWFAR       *destParentDN,
   char  NWFAR       *destRDN
);

NWDSCCODE NWAPI NWDSRead
(
   NWDSContextHandle context,
   char     NWFAR    *object,
   uint32            infoType,
   uint8             allAttrs,
   Buf_T    NWFAR    *attrNames,
   int32    NWFAR    *iterationHandle,
   Buf_T    NWFAR    *objectInfo
);

NWDSCCODE NWAPI NWDSReadObjectInfo
(
   NWDSContextHandle    context,
   char           NWFAR *object,
   char           NWFAR *distinguishedName,
   Object_Info_T  NWFAR *objectInfo
);

NWDSCCODE NWAPI NWDSRemoveObject
(
   NWDSContextHandle context,
   char  NWFAR       *object
);

NWDSCCODE NWAPI NWDSRestoreObject
(
   NWDSContextHandle context,
   char  NWFAR       *objectName,
   int32 NWFAR       *iterationHandle,
   uint8             more,
   uint32            size,
   uint8 NWFAR       *objectInfo
);

NWDSCCODE NWAPI NWDSSearch
(
   NWDSContextHandle context,
   char     NWFAR    *baseObjectName,
   int               scope,
   uint8             searchAliases,
   Buf_T    NWFAR    *filter,
   uint32            infoType,
   uint8             allAttrs,
   Buf_T    NWFAR    *attrNames,
   int32    NWFAR    *iterationHandle,
   int32             countObjectsToSearch,
   int32    NWFAR    *countObjectsSearched,
   Buf_T    NWFAR    *objectInfo
);

NWDSCCODE NWAPI NWDSOpenStream
(
   NWDSContextHandle context,
   char     NWFAR    *objectName,
   char     NWFAR    *attrName,
   uint32            flags,
   int      NWFAR    *fileHandle
);

NWDSCCODE NWAPI NWDSWhoAmI
(
   NWDSContextHandle context,
   char  NWFAR       *objectName
);

NWDSCCODE NWAPI NWDSGetServerDN
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   char NWFAR        *serverDN
);
                  
NWDSCCODE NWAPI NWDSGetServerAddresses
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   uint32 NWFAR      *countNetAddress, 
   Buf_T NWFAR       *netAddresses
);

NWDSCCODE NWAPI NWDSInspectEntry
(
   NWDSContextHandle context,
   char  NWFAR       *serverName,
   char  NWFAR       *objectName,
   Buf_T NWFAR       *errBuffer
);

NWDSCCODE NWAPI NWDSReadReferences
(
   NWDSContextHandle context,
   char     NWFAR    *serverName,
   char     NWFAR    *objectName,
   uint32            infoType,
   uint8             allAttrs,
   Buf_T    NWFAR    *attrNames,
   uint32            timeFilter,
   int32    NWFAR    *iterationHandle,
   Buf_T    NWFAR    *objectInfo
);


NWDSCCODE NWAPI NWDSExtSyncList
(
   NWDSContextHandle context,
   char        NWFAR *objectName,
   char        NWFAR *className,
   char        NWFAR *subordinateName,
   int32       NWFAR *iterationHandle,
   TimeStamp_T NWFAR *timeStamp,
   int               onlyContainers,
   Buf_T       NWFAR *subordinates
);

NWDSCCODE NWAPI NWDSExtSyncRead
(
   NWDSContextHandle context,
   char        NWFAR *objectName,
   uint32            infoType,
   uint8             allAttrs,
   Buf_T       NWFAR *attrNames,
   int32       NWFAR *iterationHandle,
   TimeStamp_T NWFAR *timeStamp,
   Buf_T       NWFAR *objectInfo
);

NWDSCCODE NWAPI NWDSExtSyncSearch
(
   NWDSContextHandle context,
   char        NWFAR *baseObjectName,
   int               scope,
   uint8             searchAliases,
   Buf_T       NWFAR *filter,
   TimeStamp_T NWFAR *timeStamp,
   uint32            infoType,
   uint8             allAttrs,
   Buf_T       NWFAR *attrNames,
   int32       NWFAR *iterationHandle,
   int32             countObjectsToSearch,
   int32       NWFAR *countObjectsSearched,
   Buf_T       NWFAR *objectInfo
);

NWDSCCODE NWAPI NWDSRemSecurityEquiv
(
   NWDSContextHandle context,
   char  NWFAR       *equalFrom,
   char  NWFAR       *equalTo
);

NWDSCCODE NWAPI NWDSAddSecurityEquiv
(
   NWDSContextHandle context,
   char  NWFAR       *equalFrom,
   char  NWFAR       *equalTo
);



#ifdef __cplusplus
}
#endif

#endif                           /* #ifndef _NWDSDSA_HEADER_ */

/*
$Header:   K:\archives\src\sdkinc\nwdsdsa.h_v   1.6   07 Jun 1994 09:52:42   RCHILDER  $
*/
