/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWDIRECT_INC
#define NWDIRECT_INC

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
   DWORD totalBlocks;
   DWORD availableBlocks;

   DWORD purgeableBlocks;       /* set to zero if a dirHandle is present */
   DWORD notYetPurgeableBlocks;/*....when the NWGetDIrSpaceInfo() is called */

   DWORD totalDirEntries;
   DWORD availableDirEntries;
   DWORD reserved;
   BYTE  sectorsPerBlock;
   BYTE  volLen;
   BYTE  volName[MAX_VOL_LEN];
} DIR_SPACE_INFO;

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

typedef struct
{
  DWORD objectID;
  WORD  objectRights;
} TRUSTEE_INFO;

NWCCODE NWAPI NWAddTrusteeToDirectory(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  DWORD trusteeID,
  BYTE rightsMask);

NWCCODE NWAPI NWDeleteTrusteeFromDirectory(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  DWORD objectID);

NWCCODE NWAPI NWGetEffectiveDirectoryRights(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  WORD NWPTR rightsMask);

NWCCODE NWAPI NWModifyMaximumRightsMask(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  BYTE revokeRightsMask,
  BYTE grantRightsMask);

NWCCODE NWAPI NWScanDirectoryForTrustees(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR searchDirPath,
  WORD NWPTR sequenceNumber,
  char NWPTR dirName,
  DWORD NWPTR dirDateTime,
  DWORD NWPTR ownerID,
  DWORD NWPTR trusteeIDs,
  BYTE NWPTR trusteeRights);

NWCCODE NWAPI NWScanDirectoryForTrustees2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR searchDirPath,
  DWORD NWPTR sequenceNumber,
  char NWPTR dirName,
  DWORD NWPTR dirDateTime,
  DWORD NWPTR ownerID,
  TRUSTEE_INFO NWPTR trusteeList);

#define NWScanDirectoryInformation(a, b, c, d, e, f, g, h) \
  NWIntScanDirectoryInformation(a, b, c, d, e, f, g, h, 0)

NWCCODE NWAPI NWIntScanDirectoryInformation(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR searchDirPath,
  WORD NWPTR sequenceNumber,
  char NWPTR dirName,
  DWORD NWPTR dirDateTime,
  DWORD NWPTR ownerID,
  BYTE NWPTR rightsMask,
  WORD augmentFlag);

#define NWScanDirectoryInformation2(a, b, c, d, e, f, g, h) \
        NWIntScanDirectoryInformation2(a, b, c, d, e, f, g, h, 0)

NWCCODE NWAPI NWIntScanDirectoryInformation2(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR searchDirPath,
  BYTE NWPTR sequenceNumber,
  char NWPTR dirName,
  DWORD NWPTR dirDateTime,
  DWORD NWPTR ownerID,
  BYTE NWPTR rightsMask,
  WORD augmentFlag);

NWCCODE NWAPI NWSetDirectoryInformation(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  DWORD dirDateTime,
  DWORD ownerID,
  BYTE rightsMask);

NWCCODE NWAPI NWSaveDirectoryHandle(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR saveBuffer);


NWCCODE NWAPI NWRestoreDirectoryHandle(
  NWCONN_HANDLE conn,
   char NWPTR saveBuffer,
   NWDIR_HANDLE NWPTR newDirHandle,
   BYTE NWPTR rightsMask);

NWCCODE NWAPI NWAllocPermanentDirectoryHandle(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   char NWPTR dirPath,
   NWDIR_HANDLE NWPTR newDirHandle,
   BYTE NWPTR effectiveRights);

NWCCODE NWAPI NWAllocTemporaryDirectoryHandle(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  NWDIR_HANDLE NWPTR newDirHandle,
  BYTE NWPTR rightsMask);

NWCCODE NWAPI NWDeallocateDirectoryHandle(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle);

NWCCODE NWAPI NWSetDirectoryHandlePath(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE sourceDirHandle,
  char NWPTR dirPath,
  NWDIR_HANDLE destDirHandle);

NWCCODE NWAPI NWGetDirectoryHandlePath(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath);

NWCCODE NWAPI NWCreateDirectory(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  BYTE accessMask);

NWCCODE NWAPI NWDeleteDirectory(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath);

NWCCODE NWAPI NWRenameDirectory(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR oldDirName,
  char NWPTR newDirName);

NWCCODE NWAPI NWSetDirSpaceLimit(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  DWORD spaceLimit);

NWCCODE NWAPI NWGetDirSpaceLimitList(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE NWPTR returnBuff);

NWCCODE NWAPI NWGetDirSpaceInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  WORD volNumber,
  DIR_SPACE_INFO NWPTR spaceInfo);

#ifndef TF_READ_ONLY
#define TF_NORMAL          0x0000
#define TF_READ_ONLY       0x0001L
#define TF_HIDDEN          0x0002L
#define TF_SYSTEM          0x0004L
#define TF_EXECUTE_ONLY    0x0008L

#define TF_DIRECTORY       0x0010L
#define TF_NEEDS_ARCHIVED  0x0020L
#define TF_EXECUTE_CONFIRM 0X0040L
#define TF_SHAREABLE       0x0080L

#define TF_LOW_SEARCH_BIT  0x0100L
#define TF_MID_SEARCH_BIT  0x0200L
#define TF_HI_SEARCH_BIT   0x0400L
#define TF_PRIVATE         0x0800L

#define TF_TRANSACTIONAL   0x1000L
#define TF_INDEXED         0x2000L
#define TF_READ_AUDIT      0x4000L
#define TF_WRITE_AUDIT     0x8000L

#define TF_PURGE           0x10000L
#define TF_RENAME_INHIBIT  0x20000L
#define TF_DELETE_INHIBIT  0x40000L
#define TF_COPY_INHIBIT    0x80000L
#define TF_AUDITING_BIT   0x00100000L
#endif

/* DIRECTORY ATTRIBUTES */

#define TD_HIDDEN          TF_HIDDEN
#define TD_SYSTEM          TF_SYSTEM
#define TD_PURGE           TF_PURGE
#define TD_PRIVATE         TF_PRIVATE
#define TD_VISIBLE         TF_PRIVATE
#define TD_RENAME_INHIBIT  TF_RENAME_INHIBIT
#define TD_DELETE_INHIBIT  TF_DELETE_INHIBIT

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
$Header:   K:\archives\src\sdkinc\nwdirect.h_v   1.3   18 Oct 1993 14:59:30   JWOODBUR  $
*/
