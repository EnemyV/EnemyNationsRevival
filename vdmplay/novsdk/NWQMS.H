/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWQMS_INC
#define NWQMS_INC

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

#define QF_AUTO_START          0x08
#define QF_ENTRY_RESTART       0x10
#define QF_ENTRY_OPEN          0x20
#define QF_USER_HOLD           0x40
#define QF_OPERATOR_HOLD       0x80

#define QS_CANT_ADD_JOBS       0x01
#define QS_SERVERS_CANT_ATTACH 0x02
#define QS_CANT_SERVICE_JOBS   0x04

typedef struct
{
  BYTE clientStation;
  BYTE clientTask;
  DWORD clientID;
  DWORD targetServerID;
  BYTE targetExecutionTime[6];
  BYTE jobEntryTime[6];
  WORD jobNumber;
  WORD jobType;
  BYTE jobPosition;
  BYTE jobControlFlags;
  BYTE jobFileName[14];
  BYTE jobFileHandle[6];
  BYTE servicingServerStation;
  BYTE servicingServerTask;
  DWORD servicingServerID;
  BYTE jobDescription[50];
  BYTE clientRecordArea[152];
} QueueJobStruct;

typedef struct
{
  BYTE clientStation;
  BYTE clientTask;
  DWORD clientID;
  DWORD targetServerID;
  BYTE targetExecutionTime[6];
  BYTE jobEntryTime[6];
  WORD jobNumber;
  WORD jobType;
  BYTE jobPosition;
  BYTE jobControlFlags;
  BYTE jobFileName[14];
  BYTE jobFileHandle[6];
  BYTE servicingServerStation;
  BYTE servicingServerTask;
  DWORD servicingServerID;
} ReplyJobStruct;

typedef struct
{
  DWORD clientStation;
  DWORD clientTask;
  DWORD clientID;
  DWORD targetServerID;
  BYTE  targetExecutionTime[6];
  BYTE  jobEntryTime[6];
  DWORD jobNumber;
  WORD  jobType;
  WORD  jobPosition;
  WORD  jobControlFlags;
  BYTE  jobFileName[14];
  DWORD jobFileHandle;
  DWORD servicingServerStation;
  DWORD servicingServerTask;
  DWORD servicingServerID;
  BYTE  jobDescription[50];
  BYTE  clientRecordArea[152];
} NWQueueJobStruct;

typedef struct
{
  DWORD clientStation;
  DWORD clientTask;
  DWORD clientID;
  DWORD targetServerID;
  BYTE  targetExecutionTime[6];
  BYTE  jobEntryTime[6];
  DWORD jobNumber;
  WORD  jobType;
  WORD  jobPosition;
  WORD  jobControlFlags;
  BYTE  jobFileName[14];
  DWORD jobFileHandle;
  DWORD servicingServerStation;
  DWORD servicingServerTask;
  DWORD servicingServerID;
} NWReplyJobStruct;

typedef struct
{
  DWORD totalQueueJobs;
  DWORD replyQueueJobNumbers;
  DWORD jobNumberList[250];   /* 250 to hold job #'s for old NCP*/
} QueueJobListReply;

NWCCODE NWAPI NWCreateQueueFile(
  NWCONN_HANDLE conn,
  DWORD queueID,
  QueueJobStruct NWPTR job,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWCreateQueueFile2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  NWQueueJobStruct NWPTR job,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWCloseFileAndStartQueueJob(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWCloseFileAndStartQueueJob2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWCloseFileAndAbortQueueJob(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWCloseFileAndAbortQueueJob2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWRemoveJobFromQueue(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber);

NWCCODE NWAPI NWRemoveJobFromQueue2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber);

NWCCODE NWAPI NWGetQueueJobList(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD NWPTR jobCount,
  WORD NWPTR jobList);

NWCCODE NWAPI NWGetQueueJobList2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD queueStartPosition,
  QueueJobListReply NWPTR job);

NWCCODE NWAPI NWReadQueueJobEntry(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  QueueJobStruct NWPTR job);

NWCCODE NWAPI NWReadQueueJobEntry2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  NWQueueJobStruct NWPTR job);

NWCCODE NWAPI NWGetQueueJobFileSize(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  DWORD NWPTR fileSize);

NWCCODE NWAPI NWGetQueueJobFileSize2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  DWORD NWPTR fileSize);

NWCCODE NWAPI NWChangeQueueJobEntry(
  NWCONN_HANDLE conn,
  DWORD queueID,
  QueueJobStruct NWPTR job);

NWCCODE NWAPI NWChangeQueueJobEntry2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  NWQueueJobStruct NWPTR job);

NWCCODE NWAPI NWChangeQueueJobPosition(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  BYTE newJobPosition);

NWCCODE NWAPI NWChangeQueueJobPosition2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  DWORD newJobPosition);

NWCCODE NWAPI NWServiceQueueJob(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD targetJobType,
  QueueJobStruct NWPTR job,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWServiceQueueJob2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD targetJobType,
  NWQueueJobStruct NWPTR job,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWAbortServicingQueueJob(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWAbortServicingQueueJob2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWChangeToClientRights(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber);

NWCCODE NWAPI NWChangeToClientRights2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber);

NWCCODE NWAPI NWFinishServicingQueueJob(
  NWCONN_HANDLE conn,
  DWORD queueID,
  WORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWFinishServicingQueueJob2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD jobNumber,
  NWFILE_HANDLE fileHandle);

NWCCODE NWAPI NWGetPrinterQueueID(
  NWCONN_HANDLE conn,
  WORD printerNumber,
  DWORD NWPTR queueID);

NWCCODE NWAPI NWCreateQueue(
  NWCONN_HANDLE conn,
  char NWPTR queueName,
  WORD  queueType,
  BYTE  dirPath,
  char  NWPTR pathName,
  DWORD NWPTR queueID);

NWCCODE NWAPI NWDestroyQueue(NWCONN_HANDLE conn, DWORD queueID);

NWCCODE NWAPI NWReadQueueCurrentStatus(
  NWCONN_HANDLE conn,
  DWORD queueID,
  BYTE  NWPTR queueStatus,
  WORD  NWPTR numberOfJobs,
  WORD  NWPTR numberOfServers,
  DWORD NWPTR serverIDlist,
  WORD  NWPTR serverConnList);

NWCCODE NWAPI NWReadQueueCurrentStatus2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD NWPTR queueStatus,
  DWORD NWPTR numberOfJobs,
  DWORD NWPTR numberOfServers,
  DWORD NWPTR serverIDlist,
  DWORD NWPTR serverConnList);

NWCCODE NWAPI NWSetQueueCurrentStatus(
  NWCONN_HANDLE conn,
  DWORD queueID,
  BYTE queueStatus);

NWCCODE NWAPI NWSetQueueCurrentStatus2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD queueStatus);

NWCCODE NWAPI NWReadQueueServerCurrentStatus(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD serverID,
  WORD serverConnection,
  void NWPTR statusRecord);

NWCCODE NWAPI NWReadQueueServerCurrentStatus2(
  NWCONN_HANDLE conn,
  DWORD queueID,
  DWORD serverID,
  DWORD serverConnection,
  void NWPTR statusRecord);

NWCCODE NWAPI NWAttachQueueServerToQueue(NWCONN_HANDLE conn, DWORD queueID);
NWCCODE NWAPI NWDetachQueueServerFromQueue(NWCONN_HANDLE conn, DWORD queueID);
NWCCODE NWAPI NWRestoreQueueServerRights(NWCONN_HANDLE conn);

NWCCODE NWAPI NWSetQueueServerCurrentStatus(
  NWCONN_HANDLE conn,
  DWORD queueID,
  void NWPTR statusRecord);

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
$Header:   K:\archives\src\sdkinc\nwqms.h_v   1.1   09 Aug 1993 12:04:28   JWOODBUR  $
*/
