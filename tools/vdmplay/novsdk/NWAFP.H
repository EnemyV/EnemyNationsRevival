/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWAFP_INC
#define NWAFP_INC

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

/** This is the structure that the application expects to see. Note that the
    long name and short name will be null terminated, and one extra byte has
    been added to long name and short name to assure word alignment **/

typedef struct
{
  DWORD entryID;
  DWORD parentID;
  WORD attributes;
  DWORD dataForkLength;
  DWORD resourceForkLength;
  WORD numOffspring;
  WORD creationDate;
  WORD accessDate;
  WORD modifyDate;
  WORD modifyTime;
  WORD backupDate;
  WORD backupTime;
  BYTE finderInfo[32];
  char longName[34];
  DWORD ownerID;
  char shortName[14];
  WORD accessPrivileges;
  BYTE proDOSInfo[6];
} AFPFILEINFO, NW_AFP_FILE_INFO;

/** This is the structure that is actually returned from the NCP call **/

typedef struct
{
  DWORD entryID;
  DWORD parentID;
  WORD attributes;
  DWORD dataForkLength;
  DWORD resourceForkLength;
  WORD numOffspring;
  WORD creationDate;
  WORD accessDate;
  WORD modifyDate;
  WORD modifyTime;
  WORD backupDate;
  WORD backupTime;
  BYTE finderInfo[32];
  char longName[32];
  DWORD ownerID;
  char shortName[12];
  WORD accessPrivileges;
  BYTE proDOSInfo[6];
} RECPKT_AFPFILEINFO;

typedef struct
{
  WORD attributes;
  WORD creationDate;
  WORD accessDate;
  WORD modifyDate;
  WORD modifyTime;
  WORD backupDate;
  WORD backupTime;
  BYTE finderInfo[32];
  BYTE proDOSInfo[6];
} AFPSETINFO, NW_AFP_SET_INFO;


/* the following are the constants that can be used for requestMasks
   in NWAFPScanFileInformation and NWAFPGetFileInformation.
*/
#ifndef AFP_GET_ATTRIBUTES
#define AFP_GET_ATTRIBUTES       0x0001
#define AFP_GET_PARENT_ID        0x0002
#define AFP_GET_CREATE_DATE      0x0004
#define AFP_GET_ACCESS_DATE      0x0008
#define AFP_GET_MODIFY_DATETIME  0x0010
#define AFP_GET_BACKUP_DATETIME  0x0020
#define AFP_GET_FINDER_INFO      0x0040
#define AFP_GET_LONG_NAME        0x0080
#define AFP_GET_ENTRY_ID         0x0100
#define AFP_GET_DATA_LEN         0x0200
#define AFP_GET_RESOURCE_LEN     0x0400
#define AFP_GET_NUM_OFFSPRING    0x0800
#define AFP_GET_OWNER_ID         0x1000
#define AFP_GET_SHORT_NAME       0x2000
#define AFP_GET_ACCESS_RIGHTS    0x4000
#define AFP_GET_PRO_DOS_INFO     0x8000
#define AFP_GET_ALL              0xffff
#endif

/*
  The following constants are used for NWAFPSetFileInformation
*/
#ifndef AFP_SET_ATTRIBUTES
#define AFP_SET_ATTRIBUTES       0x0001
#define AFP_SET_CREATE_DATE      0x0004
#define AFP_SET_ACCESS_DATE      0x0008
#define AFP_SET_MODIFY_DATETIME  0x0010
#define AFP_SET_BACKUP_DATETIME  0x0020
#define AFP_SET_FINDER_INFO      0x0040
#define AFP_SET_PRO_DOS_INFO     0x8000
#endif

#ifndef AFP_SA_HIDDEN
#define AFP_SA_NORMAL        0x0000
#define AFP_SA_HIDDEN        0x0100
#define AFP_SA_SYSTEM        0x0200
#define AFP_SA_SUBDIR        0x0400
#define AFP_SA_FILES         0x0800
#define AFP_SA_ALL           0x0F00
#endif

NWCCODE NWAPI NWAFPAllocTemporaryDirHandle(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  char NWPTR AFPPathString,
  NWDIR_HANDLE NWPTR dirHandle,
  BYTE NWPTR accessRights);

NWCCODE NWAPI NWAFPCreateDirectory(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  BYTE NWPTR finderInfo,
  char NWPTR AFPPathString,
  DWORD NWPTR newAFPEntryID);

NWCCODE NWAPI NWAFPCreateFile(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  BYTE deleteExistingFile,
  BYTE NWPTR finderInfo,
  char NWPTR AFPPathString,
  DWORD NWPTR newAFPEntryID);

NWCCODE NWAPI NWAFPDelete(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  char NWPTR AFPPathString);

NWCCODE NWAPI NWAFPGetEntryIDFromName(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  char NWPTR AFPPathString,
  DWORD NWPTR newAFPEntryID);

NWCCODE NWAPI NWAFPGetEntryIDFromHandle(
  NWCONN_HANDLE conn,
  BYTE NWPTR NetWareHandle,
  WORD NWPTR volNumber,
  DWORD NWPTR AFPEntryID,
  BYTE NWPTR forkIndicator);

NWCCODE NWAPI NWAFPGetEntryIDFromPathName(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  DWORD NWPTR AFPEntryID);

NWCCODE NWAPI NWAFPGetFileInformation(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  WORD requestMask,
  char NWPTR AFPPathString,
  WORD structSize,
  NW_AFP_FILE_INFO NWPTR AFPFileInfo);

NWCCODE NWAPI NWAFPDirectoryEntry(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path);

NWCCODE NWAPI NWAFPOpenFileFork(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  BYTE forkIndicator,
  BYTE accessMode,
  char NWPTR AFPPathString,
  DWORD NWPTR fileID,
  DWORD NWPTR forkLength,
  BYTE NWPTR NetWareHandle,
  NWFILE_HANDLE NWPTR DOSFileHandle);

NWCCODE NWAPI NWAFPRename(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPSourceEntryID,
  DWORD AFPDestEntryID,
  char NWPTR AFPSourcePath,
  char NWPTR AFPDestPath);

NWCCODE NWAPI NWAFPScanFileInformation(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPEntryID,
  DWORD NWPTR AFPLastSeenID,
  WORD searchMask,
  WORD requestMask,
  char NWPTR AFPPathString,
  WORD structSize,
  NW_AFP_FILE_INFO NWPTR AFPFileInfo);

NWCCODE NWAPI NWAFPSetFileInformation(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD AFPBaseID,
  WORD requestMask,
  char NWPTR AFPPathString,
  WORD structSize,
  NW_AFP_SET_INFO NWPTR AFPSetInfo);

NWCCODE NWAPI NWAFPSupported(NWCONN_HANDLE conn, WORD volNumber);

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
$Header:   K:\archives\src\sdkinc\nwafp.h_v   1.1   09 Aug 1993 12:04:36   JWOODBUR  $
*/
