/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWDEL_INC
#define NWDEL_INC

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
  DWORD  sequence;
  DWORD  parent;
  DWORD  attributes;
  BYTE   uniqueID;
  BYTE   flags;
  BYTE   nameSpace;
  BYTE   nameLength;
  BYTE   name [256];
  DWORD  creationDateAndTime;
  DWORD  ownerID;
  DWORD  lastArchiveDateAndTime;
  DWORD  lastArchiverID;
  DWORD  updateDateAndTime;
  DWORD  updatorID;
  DWORD  fileSize;
  BYTE   reserved[44];
  WORD   inheritedRightsMask;
  WORD   lastAccessDate;
  DWORD  deletedTime;
  DWORD  deletedDateAndTime;
  DWORD  deletorID;
  BYTE   reserved3 [16];
} NWDELETED_INFO;

NWCCODE NWAPI NWPurgeDeletedFile(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  DWORD sequence,
  DWORD volume,
  DWORD dirBase,
  BYTE NWPTR fileName);

NWCCODE NWAPI NWRecoverDeletedFile(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  DWORD sequence,
  DWORD volume,
  DWORD dirBase,
  char NWPTR deletedFileName,
  char NWPTR recoverFileName);

NWCCODE NWAPI NWScanForDeletedFiles(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  DWORD NWPTR sequence,
  DWORD NWPTR volume,
  DWORD NWPTR dirBase,
  NWDELETED_INFO NWPTR entryInfo);

NWCCODE NWAPI NWPurgeErasedFiles(NWCONN_HANDLE conn);

NWCCODE NWAPI NWRestoreErasedFile(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  char NWPTR oldName,
  char NWPTR newName);

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
$Header:   K:\archives\src\sdkinc\nwdel.h_v   1.1   09 Aug 1993 12:04:36   JWOODBUR  $
*/
