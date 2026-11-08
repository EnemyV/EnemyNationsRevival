/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NONAMSPC_INC
#define NONAMSPC_INC

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

#ifndef SUCCESSFUL
#define SUCCESSFUL                0
#endif

#define MORE_NS_TO_READ           0
#define NO_EXTENDED_NS_INFO       9
#define NS_EOF                    0x8910

#define NW_NS_DOS     0
#define NW_NS_MAC     1
#define NW_NS_NFS     2
#define NW_NS_FTAM    3
#define NW_NS_OS2     4

#define NW_DS_DOS     0
#define NW_DS_MAC     1
#define NW_DS_FTAM    2

typedef struct
{
  BYTE  volNumber;
  BYTE  srcNameSpace;
  DWORD srcDirBase;
  BYTE  dstNameSpace;
  DWORD dstDirBase;
} NW_IDX;

typedef struct
{
  DWORD NSInfoBitMask;
  DWORD fixedBitMask;
  DWORD reservedBitMask;
  DWORD extendedBitMask;
  WORD  fixedBitsDefined;
  WORD  reservedBitsDefined;
  WORD  extendedBitsDefined;
  DWORD fieldsLenTable[32];
  BYTE  hugeStateInfo[16];
  DWORD hugeDataLength;
} NW_NS_INFO;

typedef struct
{
  DWORD spaceAlloc;
  DWORD attributes;
  WORD  flags;
  DWORD dataStreamSize;
  DWORD totalStreamSize;
  WORD  numberOfStreams;
  WORD  creationTime;
  WORD  creationDate;
  DWORD creatorID;
  WORD  modifyTime;
  WORD  modifyDate;
  DWORD modifierID;
  WORD  lastAccessDate;
  WORD  archiveTime;
  WORD  archiveDate;
  DWORD archiverID;
  WORD  inheritedRightsMask;
  DWORD dirEntNum;
  DWORD DosDirNum;
  DWORD volNumber;
  DWORD EADataSize;
  DWORD EAKeyCount;
  DWORD EAKeySize;
  DWORD NSCreator;
  BYTE  nameLength;
  BYTE  entryName[256];
} NW_ENTRY_INFO;

typedef struct _MODIFY_DOS_INFO
{
  DWORD attributes;
  WORD  createDate;
  WORD  createTime;
  DWORD  creatorID;
  WORD  modifyDate;
  WORD  modifyTime;
  DWORD  modifierID;
  WORD  archiveDate;
  WORD  archiveTime;
  DWORD  archiverID;
  WORD  lastAccessDate;
  WORD  inheritanceGrantMask;
  WORD  inheritanceRevokeMask;
  DWORD  maximumSpace;
} MODIFY_DOS_INFO;

typedef struct
{
  BYTE  volNumber;
  DWORD dirNumber;
  DWORD searchDirNumber;
} SEARCH_SEQUENCE;

typedef struct
{
  char NWPTR srcPath;
  char NWPTR dstPath;
  WORD dstPathSize;
} NW_NS_PATH;

typedef struct
{
  BYTE  openCreateMode;
  WORD  searchAttributes;
  DWORD reserved;
  DWORD createAttributes;
  WORD  accessRights;
  DWORD NetWareHandle;
  BYTE  openCreateAction;
} NW_NS_OPENCREATE, NW_NS_OPEN;


/* open/create modes */
#define OC_MODE_OPEN      0x01
#define OC_MODE_TRUNCATE  0x02
#define OC_MODE_REPLACE   0x02
#define OC_MODE_CREATE    0x08

/* open/create results */
#define OC_ACTION_NONE     0x00
#define OC_ACTION_OPEN     0x01
#define OC_ACTION_CREATE   0x02
#define OC_ACTION_TRUNCATE 0x04
#define OC_ACTION_REPLACE  0x04

/* return info mask */
#define IM_NAME               0x0001L
#define IM_ENTRY_NAME         0x0001L
#define IM_SPACE_ALLOCATED    0x0002L
#define IM_ATTRIBUTES         0x0004L
#define IM_SIZE               0x0008L
#define IM_TOTAL_SIZE         0x0010L
#define IM_EA                 0x0020L
#define IM_ARCHIVE            0x0040L
#define IM_MODIFY             0x0080L
#define IM_CREATION           0x0100L
#define IM_OWNING_NAMESPACE   0x0200L
#define IM_DIRECTORY          0x0400L
#define IM_RIGHTS             0x0800L
#define IM_ALMOST_ALL         0x0FEDL
#define IM_ALL                0x0FFFL
#define IM_REFERENCE_ID       0x1000L
#define IM_NS_ATTRIBUTES      0x2000L
#define IM_DATASTREAM_SIZES   0x4000L
#define IM_COMPRESSED_INFO    0x80000000L
#define IM_NS_SPECIFIC_INFO   0x80000000L

/* access rights attributes */
#ifndef AR_READ_ONLY
#define AR_READ               0x0001
#define AR_WRITE              0x0002
#define AR_READ_ONLY          0x0001
#define AR_WRITE_ONLY         0x0002
#define AR_DENY_READ          0x0004
#define AR_DENY_WRITE         0x0008
#define AR_COMPATIBILITY      0x0010
#define AR_WRITE_THROUGH      0x0040
#define AR_OPEN_COMPRESSED    0x0100
#define AR_DELETE_FILE_CLOSE  0x0400
#endif

/* Trustee Access Rights in a network directory */
/* NOTE: TA_OPEN is obsolete in 3.x */
#ifndef TA_NONE
#define TA_NONE       0x00
#define TA_READ       0x01
#define TA_WRITE      0x02
#define TA_OPEN       0x04
#define TA_CREATE     0x08
#define TA_DELETE     0x10
#define TA_OWNERSHIP  0x20
#define TA_SEARCH     0x40
#define TA_MODIFY     0x80
#define TA_ALL        0xFB
#endif

/* search attributes */
#ifndef SA_HIDDEN
#define SA_NORMAL         0x0000
#define SA_HIDDEN         0x0002
#define SA_SYSTEM         0x0004
#define SA_SUBDIR_ONLY    0x0010
#define SA_SUBDIR_FILES   0x8000
#define SA_ALL            0x8006
#endif

#define NW_TYPE_FILE      0x8000
#define NW_TYPE_SUBDIR    0x0010

#define NW_NAME_CONVERT     0x03
#define NW_NO_NAME_CONVERT  0x04

/* modify mask - use with MODIFY_DOS_INFO structure */
#define DM_ATTRIBUTES             0x0002L
#define DM_CREATE_DATE            0x0004L
#define DM_CREATE_TIME            0x0008L
#define DM_CREATOR_ID             0x0010L
#define DM_ARCHIVE_DATE           0x0020L
#define DM_ARCHIVE_TIME           0x0040L
#define DM_ARCHIVER_ID            0x0080L
#define DM_MODIFY_DATE            0x0100L
#define DM_MODIFY_TIME            0x0200L
#define DM_MODIFIER_ID            0x0400L
#define DM_LAST_ACCESS_DATE       0x0800L
#define DM_INHERITED_RIGHTS_MASK  0x1000L
#define DM_MAXIMUM_SPACE          0x2000L

NWCCODE NWAPI NWGetDirectoryBase(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE destNameSpace,
  NW_IDX NWPTR idxStruct);

NWCCODE NWAPI NWScanNSEntryInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE nameSpace,
  WORD attributes,
  SEARCH_SEQUENCE NWPTR sequence,
  char NWPTR searchPattern,
  DWORD returnInfoMask,
  NW_ENTRY_INFO NWPTR entryInfo);

NWCCODE NWAPI NWGetNSLoadedList(
  NWCONN_HANDLE conn,
  BYTE volNumber,
  BYTE listBufferSize,
  BYTE NWPTR NSLoadedList,
  BYTE NWPTR actualListSize);

NWCCODE NWAPI NWGetOwningNameSpace(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE NWPTR nameSpace);

NWCCODE NWAPI NWOpenCreateNSEntry(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE nameSpace,
  char NWPTR filePath,
  NW_NS_OPENCREATE NWPTR NSOpenCreate,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWOpenNSEntry(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE nameSpace,
  BYTE dataStream,
  char NWPTR filePath,
  NW_NS_OPEN NWPTR NSOpen,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWSetLongName(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE nameSpace,
  char NWPTR destPath,
  WORD destType,
  char NWPTR longName);

NWCCODE NWAPI NWWriteExtendedNSInfo(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_NS_INFO NWPTR NSInfo,
  BYTE NWPTR data);

NWCCODE NWAPI NWGetLongName(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE sourceNameSpace,
  BYTE destNameSpace,
  char NWPTR longName);

NWCCODE NWAPI NWWriteNSInfo(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_NS_INFO NWPTR NSInfo,
  BYTE  NWPTR data);

NWCCODE NWAPI NWGetNSInfo(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_NS_INFO NWPTR nsInfo);

NWCCODE NWAPI NWReadNSInfo(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_NS_INFO NWPTR NSInfo,
  BYTE NWPTR data);

NWCCODE NWAPI NWReadExtendedNSInfo(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_NS_INFO NWPTR NSInfo,
  BYTE NWPTR data);

NWCCODE NWAPI NWGetNSPath(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  WORD fileFlag,
  BYTE sourceNameSpace,
  BYTE destNameSpace,
  NW_NS_PATH NWPTR NSPath);

NWCCODE NWAPI NWAllocTempNSDirHandle(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  BYTE nameSpace,
  NWDIR_HANDLE NWPTR newDirHandle);

NWCCODE NWAPI NWAllocTempNSDirHandle2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE nameSpace,
  BYTE NWPTR newDirHandle,
  BYTE newNameSpace);

NWCCODE NWAPI NWGetNSEntryInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE sourceNameSpace,
  BYTE destNameSpace,
  WORD searchAttributes,
  DWORD returnInfoMask,
  NW_ENTRY_INFO NWPTR entryInfo);

NWCCODE NWAPI NWNSGetMiscInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE destNameSpace,
  NW_IDX NWPTR idxStruct);

NWCCODE NWAPI NWOpenDataStream(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR fileName,
  WORD dataStream,
  WORD attributes,
  WORD accessMode,
  DWORD NWPTR NetWareHandle,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWNSRename(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE nameSpace,
  char NWPTR oldName,
  WORD oldType,
  char NWPTR newName,
  BYTE renameFlag);

NWCCODE NWAPI NWSetNSEntryDOSInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE nameSpace,
  WORD searchAttributes,
  DWORD modifyDOSMask,
  MODIFY_DOS_INFO NWPTR dosInfo);

NWCCODE NWAPI NWGetFullPath(
   NWCONN_HANDLE  conn,
   BYTE           volNum,
   DWORD          dirBase,
   WORD           handleFlag,
   int            srcNamSpc,
   int            dstNamSpc,
   WORD           maxPathLen,
   char  NWFAR *  path,
   WORD  NWFAR *  pathType);

#ifdef NWDOS
#define __NWGetCurNS(a, b, c) NW_NS_DOS
#else
WORD NWAPI __NWGetCurNS(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   char NWPTR path);
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
$Header:   K:\archives\src\sdkinc\nwnamspc.h_v   1.2   09 Mar 1994 13:41:02   DOCOX  $
*/
