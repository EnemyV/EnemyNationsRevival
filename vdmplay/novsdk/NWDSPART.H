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

#ifndef  _NWDSPART_HEADER_
#define  _NWDSPART_HEADER_

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif

#ifndef  _NWDSDC_HEADER_
#include <nwdsdc.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE NWAPI NWDSAddPartition
(
   NWDSContextHandle context,
   char  NWFAR       *server,
   char  NWFAR       *partitionRoot,
   int32 NWFAR       *iterationHandle,
   uint8             more,
   Buf_T NWFAR       *objectInfo
);

NWDSCCODE NWAPI NWDSAddReplica
(
   NWDSContextHandle context,
   char  NWFAR       *server,
   char  NWFAR       *partitionRoot,
   uint32            replicaType
);

NWDSCCODE NWAPI NWDSChangeReplicaType
(
   NWDSContextHandle context,
   char  NWFAR       *replicaName,
   char  NWFAR       *server,
   uint32            newReplicaType
);

NWDSCCODE NWAPI NWDSJoinPartitions
(
   NWDSContextHandle context,
   char  NWFAR       *subordinatePartition,
   uint32            flags
);

NWDSCCODE NWAPI NWDSListPartitions
(
   NWDSContextHandle context,
   int32 NWFAR       *iterationHandle,
   char  NWFAR       *server,
   Buf_T NWFAR       *partitions
);

NWDSCCODE NWAPI NWDSRemovePartition
(
   NWDSContextHandle context,
   char  NWFAR       *partitionRoot
);

NWDSCCODE NWAPI NWDSRemoveReplica
(
   NWDSContextHandle context,
   char  NWFAR       *server,
   char  NWFAR       *partitionRoot
);

NWDSCCODE NWAPI NWDSSplitPartition
(
   NWDSContextHandle context,
   char  NWFAR       *subordinatePartition,
   uint32            flags
);

NWDSCCODE NWAPI NWDSPartitionReceiveAllUpdates
(
   NWDSContextHandle context,
   char  NWFAR       *partitionRoot,
   char  NWFAR       *serverName
);

NWDSCCODE NWAPI NWDSPartitionSendAllUpdates
(
   NWDSContextHandle context,
   char  NWFAR       *partitionRoot,
   char  NWFAR       *serverName
);

NWDSCCODE NWAPI NWDSSyncPartition
(
   NWDSContextHandle context,
   char NWFAR        *server,
   char NWFAR        *partition,
   uint32            seconds
);

NWDSCCODE NWAPI NWDSAbortPartitionOperation
(
   NWDSContextHandle context,
   char  NWFAR       *partitionRoot
);

#ifdef __cplusplus
}
#endif

#endif  /* _NWDSPART_HEADER_ */

/*
$Header:   K:\archives\src\sdkinc\nwdspart.h_v   1.2   06 Aug 1993 09:42:54   RCHILDER  $
*/
