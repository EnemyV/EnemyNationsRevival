/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWSYNC_INC
#define NWSYNC_INC

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

typedef struct
{
  NWCONN_NUM connNumber;
  WORD taskNumber;
  BYTE lockStatus;
} LOGICAL_LOCK;

typedef struct
{
  WORD useCount;
  WORD shareableLockCount;
  BYTE locked;
  WORD nextRequest;
  WORD numRecords;
  LOGICAL_LOCK logicalLock[128];
  WORD curRecord;
} LOGICAL_LOCKS;

typedef struct
{
  WORD taskNumber;
  BYTE lockStatus;
  char logicalName[128];
} CONN_LOGICAL_LOCK;

typedef struct
{
  WORD nextRequest;
  WORD numRecords;
  BYTE records[508];
  WORD curOffset;
  WORD curRecord;
} CONN_LOGICAL_LOCKS;

typedef struct
{
  WORD  loggedCount;
  WORD  shareableLockCount;
  DWORD recordStart;
  DWORD recordEnd;
  WORD  connNumber;
  WORD  taskNumber;
  BYTE  lockType;
} PHYSICAL_LOCK;

typedef struct
{
  WORD nextRequest;
  WORD numRecords;
  PHYSICAL_LOCK locks[32];
  WORD curRecord;
  BYTE reserved[8];
} PHYSICAL_LOCKS;

typedef struct
{
  WORD  taskNumber;
  BYTE  lockType;
  DWORD recordStart;
  DWORD recordEnd;
} CONN_PHYSICAL_LOCK;

typedef struct
{
  WORD nextRequest;
  WORD numRecords;
  CONN_PHYSICAL_LOCK locks[51];
  WORD curRecord;
  BYTE reserved[22];
} CONN_PHYSICAL_LOCKS;

typedef struct
{
  NWCONN_NUM connNumber;
  WORD taskNumber;
} SEMAPHORE;

typedef struct
{
  WORD nextRequest;
  WORD openCount;
  WORD semaphoreValue;
  WORD semaphoreCount;
  SEMAPHORE semaphores[170];
  WORD curRecord;
} SEMAPHORES;

typedef struct
{
  WORD openCount;
  WORD semaphoreValue;
  WORD taskNumber;
  char semaphoreName[128];
} CONN_SEMAPHORE;

typedef struct
{
  WORD nextRequest;
  WORD numRecords;
  BYTE records[508];
  WORD curOffset;
  WORD curRecord;
} CONN_SEMAPHORES;


NWCCODE NWAPI NWScanPhysicalLocksByFile(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePath,
  BYTE dataStream,
  NWINT16 NWPTR lastRecord,
  PHYSICAL_LOCK NWPTR lock,
  PHYSICAL_LOCKS NWPTR locks);

NWCCODE NWAPI NWScanLogicalLocksByConn(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNumber,
  NWINT16 NWPTR lastRecord,
  CONN_LOGICAL_LOCK NWPTR logicalLock,
  CONN_LOGICAL_LOCKS NWPTR logicalLocks);

NWCCODE NWAPI NWScanPhysicalLocksByConnFile(
  NWCONN_HANDLE  conn,
  NWCONN_NUM connNumber,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE dataStream,
  NWINT16 NWPTR lastRecord,
  CONN_PHYSICAL_LOCK NWPTR lock,
  CONN_PHYSICAL_LOCKS NWPTR locks);

NWCCODE NWAPI NWScanLogicalLocksByName(
  NWCONN_HANDLE conn,
   char NWPTR logicalName,
   NWINT16 NWPTR lastRecord,
   LOGICAL_LOCK NWPTR logicalLock,
   LOGICAL_LOCKS NWPTR logicalLocks);

NWCCODE NWAPI NWScanSemaphoresByConn(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNumber,
  NWINT16 NWPTR lastRecord,
  CONN_SEMAPHORE NWPTR semaphore,
  CONN_SEMAPHORES NWPTR semaphores);

NWCCODE NWAPI NWScanSemaphoresByName(
  NWCONN_HANDLE conn,
  char NWPTR semaphoreName,
  NWINT16 NWPTR lastRecord,
  SEMAPHORE NWPTR semaphore,
  SEMAPHORES NWPTR semaphores);


NWCCODE NWAPI NWSignalSemaphore(NWCONN_HANDLE conn, DWORD semaphoreHandle);
NWCCODE NWAPI NWCloseSemaphore(NWCONN_HANDLE conn, DWORD semaphoreHandle);
NWCCODE NWAPI NWOpenSemaphore(
  NWCONN_HANDLE conn,
  char NWPTR semaphoreName,
  NWINT16 initSemaphoreHandle,
  DWORD NWPTR semaphoreHandle,
  WORD NWPTR semaphoreOpenCount);

NWCCODE NWAPI NWExamineSemaphore(
  NWCONN_HANDLE conn,
  DWORD semaphoreHandle,
  NWINT16 NWPTR semaphoreValue,
  WORD NWPTR semaphoreOpenCount);

NWCCODE NWAPI NWWaitOnSemaphore(
  NWCONN_HANDLE conn,
  DWORD semaphoreHandle,
  WORD timeOutValue);

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
$Header:   K:\archives\src\sdkinc\nwsync.h_v   1.2   10 Aug 1993 12:50:52   JWOODBUR  $
*/
