/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWDENTRY_INC
#define NWDENTRY_INC

#ifndef NWCALDEF_INC
# include <nwcaldef.h>
#endif

#ifndef NWDIRECT_INC
# include <nwdirect.h>
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
  char entryName[16];
  DWORD creationDateAndTime;
  DWORD ownerID;
  DWORD sequenceNumber;
  TRUSTEE_INFO trusteeList[20];
} NWET_INFO;

typedef struct
{
  DWORD  updateDateAndTime;
  DWORD  updatorID;
  DWORD  fileSize;
  BYTE   reserved[44];
  WORD   inheritedRightsMask;
  WORD   lastAccessDate;
  BYTE   reserved2[28];
} NWFILE_INFO;
typedef struct
{
  DWORD  lastModifyDateAndTime;
  DWORD  nextTrusteeEntry;
  BYTE   reserved[48];
  DWORD  maximumSpace;
  WORD   inheritedRightsMask;
  BYTE   reserved2[14];
  DWORD  volObjectID;
  BYTE   reserved3[8];
} NWDIR_INFO;

typedef struct
{
  DWORD  sequence;
  DWORD  parent;
  DWORD  attributes;
  BYTE   uniqueID;
  BYTE   flags;
  BYTE   nameSpace;
  BYTE   nameLength;
  BYTE   name[12];
  DWORD  creationDateAndTime;
  DWORD  ownerID;
  DWORD  lastArchiveDateAndTime;
  DWORD  lastArchiverID;

  union
  {
    NWFILE_INFO file;
    NWDIR_INFO   dir;
  }info;

} NWENTRY_INFO;

typedef struct
{
  DWORD sequence;
  DWORD parent;
  DWORD attributes;
  BYTE  uniqueID;
  BYTE  flags;
  BYTE  nameSpace;
  BYTE  nameLength;
  BYTE  name [12];
  DWORD creationDateAndTime;
  DWORD ownerID;
  DWORD lastArchiveDateAndTime;
  DWORD lastArchiverID;
  DWORD updateDateAndTime;
  DWORD lastUpdatorID;
  DWORD dataForkSize;         /* file size */
  DWORD dataForkFirstFAT;
  DWORD nextTrusteeEntry;
  BYTE  reserved[36];
  WORD  inheritedRightsMask;
  WORD  lastAccessDate;
  DWORD deletedFileTime;
  DWORD deletedDateAndTime;
  DWORD deletorID;
  BYTE  reserved2 [16];
  DWORD otherForkSize[2];
} NW_EXT_FILE_INFO;

#define TR_NONE         0x0000
#define TR_READ         0x0001
#define TR_WRITE        0x0002
#define TR_OPEN         0x0004
#define TR_DIRECTORY    0x0004
#define TR_CREATE       0x0008
#define TR_DELETE       0x0010
#define TR_ERASE        0x0010
#define TR_OWNERSHIP    0x0020
#define TR_ACCESS_CTRL  0x0020
#define TR_FILE_SCAN    0x0040
#define TR_SEARCH       0x0040
#define TR_FILE_ACCESS  0x0040
#define TR_MODIFY       0x0080
#define TR_ALL          0x01FB
#define TR_SUPERVISOR   0x0100
#define TR_NORMAL       0x00FB

#ifndef MModifyNameBit
#define MModifyNameBit           0x0001L
#define MFileAttributesBit       0x0002L
#define MCreateDateBit           0x0004L
#define MCreateTimeBit           0x0008L
#define MOwnerIDBit              0x0010L
#define MLastArchivedDateBit     0x0020L
#define MLastArchivedTimeBit     0x0040L
#define MLastArchivedIDBit       0x0080L
#define MLastUpdatedDateBit      0x0100L
#define MLastUpdatedTimeBit      0x0200L
#define MLastUpdatedIDBit        0x0400L
#define MLastAccessedDateBit     0x0800L
#define MInheritedRightsMaskBit  0x1000L
#define MMaximumSpaceBit         0x2000L
#endif

NWCCODE NWAPI NWDeleteTrustee(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  DWORD objectID);

NWCCODE NWAPI NWAddTrustee(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  DWORD objectID,
  WORD rightsMask);

#define NWScanDirEntryInfo(a, b, c, d, e, f) \
        NWIntScanDirEntryInfo(a, b, c, d, e, f, 0)

NWCCODE NWAPI NWIntScanDirEntryInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  WORD attributes,
  DWORD NWPTR sequence,
  BYTE NWPTR searchPattern,
  NWENTRY_INFO NWPTR entryInfo,
  WORD augmentFlag);

#define NWScanForTrustees(a, b, c, d, e, f) \
        NWIntScanForTrustees(a, b, c, d, e, f, 0)

NWCCODE NWAPI NWIntScanForTrustees(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  DWORD NWPTR sequence,
  WORD NWPTR numberOfEntries,
  NWET_INFO NWPTR entryTrusteeInfo,
  WORD augmentFlag);

#define NWMoveDirEntry(a, b, c, d, e, f) \
        NWIntMoveDirEntry(a, b, c, d, e, f, 0)

NWCCODE NWAPI NWIntMoveDirEntry(
  NWCONN_HANDLE conn,
  BYTE searchAttribute,
  NWDIR_HANDLE sourceDirHandle,
  char NWPTR sourcePath,
  NWDIR_HANDLE destDirHandle,
  char NWPTR destPath,
  WORD augmentFlag);

NWCCODE NWAPI NWSetDirEntryInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE searchAttribute,
  DWORD sequence,
  DWORD changeBits,
  NWENTRY_INFO NWPTR newEntryInfo);

#define NWScanExtendedInfo(a, b, c, d, e, f) \
        NWIntScanExtendedInfo(a, b, c, d, e, f, 0)

NWCCODE NWAPI NWIntScanExtendedInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  BYTE attributes,
  DWORD NWPTR sequence,
  char NWPTR searchPattern,
  NW_EXT_FILE_INFO NWPTR entryInfo,
  WORD augmentFlag);

NWCCODE NWAPI NWGetEffectiveRights(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  WORD NWPTR effectiveRights);

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
$Header:   K:\archives\src\sdkinc\nwdentry.h_v   1.1   09 Aug 1993 12:04:38   JWOODBUR  $
*/
