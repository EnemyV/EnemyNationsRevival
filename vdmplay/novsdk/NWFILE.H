/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWFILE_INC
#define NWFILE_INC

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

#ifndef FILE_LOCKS_ONLY      /* Skip most of file stuff if lock only */

typedef struct NW_FILE_INFO
{
  char  fileName[14];
  BYTE  fileAttributes;
  BYTE  extendedFileAttributes;
  DWORD fileSize;
  WORD  creationDate;
  WORD  lastAccessDate;
  DWORD lastUpdateDateAndTime;
  DWORD fileOwnerID;
  DWORD lastArchiveDateAndTime;
} NW_FILE_INFO;

typedef struct NW_FILE_INFO2
{
  BYTE  fileAttributes;
  BYTE  extendedFileAttributes;
  DWORD fileSize;
  WORD  creationDate;
  WORD  lastAccessDate;
  DWORD lastUpdateDateAndTime;
  DWORD fileOwnerID;
  DWORD lastArchiveDateAndTime;
  char  fileName[260];
} NW_FILE_INFO2;

typedef struct SEARCH_FILE_INFO
{
  WORD  sequenceNumber;
  WORD  reserved;
  char  fileName[15];
  BYTE  fileAttributes;
  BYTE  fileMode;
  DWORD fileLength;
  WORD  createDate;
  WORD  accessDate;
  WORD  updateDate;
  WORD  updateTime;
} SEARCH_FILE_INFO;

typedef struct SEARCH_DIR_INFO
{
  WORD  sequenceNumber;
  WORD  reserved1;
  char  directoryName[15];
  BYTE  directoryAttributes;
  BYTE  directoryAccessRights;
  WORD  createDate;
  WORD  createTime;
  DWORD owningObjectID;
  WORD  reserved2;
  WORD  directoryStamp;
} SEARCH_DIR_INFO;

typedef struct
{
  BYTE taskNumber;
  BYTE lockType;
  BYTE accessControl;
  BYTE lockFlag;
  BYTE volNumber;
  WORD dirEntry;
  char fileName[14];
} CONN_OPEN_FILE;

typedef struct
{
  WORD nextRequest;
  BYTE connCount;
  CONN_OPEN_FILE connInfo[22];
} CONN_OPEN_FILES;


typedef struct
{
  WORD  taskNumber;
  BYTE  lockType;
  BYTE  accessControl;
  BYTE  lockFlag;
  BYTE  volNumber;
  DWORD parent;
  DWORD dirEntry;
  BYTE  forkCount;
  BYTE  nameSpace;
  BYTE  nameLen;
  char  fileName[255];
} OPEN_FILE_CONN;

typedef struct
{
  WORD nextRequest;
  WORD openCount;
  BYTE buffer[512];
  WORD curRecord;
} OPEN_FILE_CONN_CTRL;

typedef struct
{
  NWCONN_NUM connNumber;
  WORD taskNumber;
  BYTE lockType;
  BYTE accessControl;
  BYTE lockFlag;
} CONN_USING_FILE;

typedef struct
{
  WORD nextRequest;
  WORD useCount;
  WORD openCount;
  WORD openForReadCount;
  WORD openForWriteCount;
  WORD denyReadCount;
  WORD denyWriteCount;
  BYTE locked;
  BYTE forkCount;
  WORD connCount;
  CONN_USING_FILE connInfo[70];
} CONNS_USING_FILE;

NWCCODE NWAPI NWSetCompressedFileSize
(
	NWCONN_HANDLE conn,
	DWORD fileHandle,
	DWORD reqFileSize,
	DWORD NWFAR *resFileSize
);

NWCCODE NWAPI NWFileServerFileCopy(
  NWFILE_HANDLE srcFileHandle,
  NWFILE_HANDLE dstFileHandle,
  DWORD srcOffset,
  DWORD dstOffset,
  DWORD bytesToCopy,
  DWORD NWPTR bytesCopied);

NWCCODE NWAPI NWGetFileConnectionID(
  NWFILE_HANDLE fileHandle,
  NWCONN_HANDLE NWPTR conn);

#define NWFileSearchInitialize(a, b, c, d, e, f, g) \
        NWIntFileSearchInitialize(a, b, c, d, e, f, g, 0)

NWCCODE NWAPI NWIntFileSearchInitialize(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE NWPTR path,
  BYTE NWPTR volNumber,
  WORD NWPTR dirID,
  WORD NWPTR sequenceNumber,
  BYTE NWPTR accessRights,
  WORD augmentFlag);

#define NWFileSearchContinue(a, b, c, d, e, f, g) \
        NWIntFileSearchContinue(a, b, c, d, e, f, g, 0)

NWCCODE NWAPI NWIntFileSearchContinue(
  NWCONN_HANDLE conn,
  BYTE volNum,
  WORD dirID,
  WORD searchContext,
  BYTE searchAttributes,
  char NWPTR searchPath,
  char NWPTR returnBuffer,
  WORD augmentFlag);

#define NWScanFileInformation(a, b, c, d, e, f) \
        NWIntScanFileInformation(a, b, c, d, e, f, 0)

NWCCODE NWAPI NWIntScanFileInformation(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePattern,
  BYTE searchAttributes,
  NWINT16 NWPTR sequenceNumber,
  NW_FILE_INFO NWPTR info,
  WORD augmentFlag);

NWCCODE NWAPI NWSetFileInformation(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePattern,
  BYTE searchAttributes,
  NW_FILE_INFO NWPTR info);

NWCCODE NWAPI NWSetFileInformation2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePattern,
  BYTE searchAttributes,
  NW_FILE_INFO2 NWPTR info);

#define NWScanFileInformation2(a, b, c, d, e, f) \
        NWIntScanFileInformation2(a, b, c, d, e, f, 0)

NWCCODE NWAPI NWIntScanFileInformation2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePattern,
  BYTE searchAttributes,
  BYTE NWPTR sequenceNumber,
  NW_FILE_INFO2 NWPTR info,
  WORD augmentFlag);

NWCCODE NWAPI NWSetFileAttributes(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR fileName,
  BYTE searchAttributes,
  BYTE newAttributes);

NWCCODE NWAPI NWGetExtendedFileAttributes2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE NWPTR extendedAttributes);

NWCCODE NWAPI NWScanConnectionsUsingFile(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePath,
  NWINT16 NWPTR lastRecord,
  CONN_USING_FILE NWPTR fileUse,
  CONNS_USING_FILE NWPTR fileUsed);

NWCCODE NWAPI NWScanOpenFilesByConn2(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNum,
  NWINT16 NWPTR lastRecord,
  OPEN_FILE_CONN_CTRL NWPTR openCtrl,
  OPEN_FILE_CONN NWPTR openFile);

NWCCODE NWAPI NWScanOpenFilesByConn(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNumber,
  NWINT16 NWPTR lastRecord,
  CONN_OPEN_FILE NWPTR openFile,
  CONN_OPEN_FILES NWPTR openFiles);

NWCCODE NWAPI NWSetExtendedFileAttributes2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE extendedAttributes);

NWCCODE NWAPI NWRenameFile(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE oldDirHandle,
  char NWPTR oldFileName,
  BYTE searchAttributes,
  NWDIR_HANDLE newDirHandle,
  char NWPTR newFileName);

#define NWEraseFiles(a, b, c, d) \
        NWIntEraseFiles(a, b, c, d, 0)

NWCCODE NWAPI NWIntEraseFiles(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR filePath,
  BYTE searchAttributes,
  WORD augmentFlag);

NWCCODE NWAPI NWGetSparseFileBitMap(
  NWCONN_HANDLE conn,
  DWORD fileHandle,
  NWINT16 flag,
  DWORD offset,
  DWORD NWPTR blockSize,
  BYTE NWPTR bitMap);

#endif

#define NWLOCKS_INCLUDED

NWCCODE NWAPI NWLogPhysicalRecord(
  NWFILE_HANDLE fileHandle,
  DWORD recordStartOffset,
  DWORD recordLength,
  BYTE lockFlags,
  WORD timeOut);

NWCCODE NWAPI NWLockPhysicalRecordSet(
  BYTE lockFlags,
  WORD timeOut);

NWCCODE NWAPI NWReleasePhysicalRecordSet(void);

NWCCODE NWAPI NWClearPhysicalRecordSet(void);

NWCCODE NWAPI NWReleasePhysicalRecord(
  NWFILE_HANDLE fileHandle,
  DWORD recordStartOffset,
  DWORD recordSize);

NWCCODE NWAPI NWClearPhysicalRecord(
  NWFILE_HANDLE fileHandle,
  DWORD recordStartOffset,
  DWORD recordSize);

NWCCODE NWAPI NWLockFileLockSet(WORD timeOut);
NWCCODE NWAPI NWReleaseFileLockSet(void);
NWCCODE NWAPI NWClearFileLockSet(void);

NWCCODE NWAPI NWClearFileLock2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path);

NWCCODE NWAPI NWReleaseFileLock2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path);

NWCCODE NWAPI NWLogFileLock2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE lockFlags,
  WORD timeOut);

NWCCODE NWAPI NWLogLogicalRecord(
  NWCONN_HANDLE conn,
  char NWPTR logicalRecordName,
  BYTE lockFlags,
  WORD timeOut);

NWCCODE NWAPI NWLockLogicalRecordSet(
  BYTE lockFlags,
  WORD timeOut);

NWCCODE NWAPI NWReleaseLogicalRecordSet(void);
NWCCODE NWAPI NWClearLogicalRecordSet(void);

NWCCODE NWAPI NWReleaseLogicalRecord(
  NWCONN_HANDLE conn,
  char NWPTR logicalRecordName);

NWCCODE NWAPI NWClearLogicalRecord(
  NWCONN_HANDLE conn,
  char NWPTR logicalRecordName);

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
$Header:   K:\archives\src\sdkinc\nwfile.h_v   1.3   18 Oct 1993 14:58:12   JWOODBUR  $
*/
