/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWMIGRAT_INC
#define NWMIGRAT_INC

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

#define MAX_NUM_OF_DATA_STREAMS       3
#define MAX_SIZE_OF_SM_STRING       128
#define MAX_SIZE_OF_SM_INFO         128
#define MAX_NUM_OF_SM                32

#define ERR_INVALID_SM_ID           240
#define ERR_SM_ALREADY_REGISTERED   241
#define ERR_SM_CREATE_FAILED        242
#define ERR_SM_CLOSE_FAILED         243
#define ERR_SM_WRITE_NO_SPACE       244
#define ERR_SM_WRITE_IO_ERROR       245
#define ERR_SM_READ_IO_ERROR        246
#define ERR_SM_OPEN_FAILED          247
#define ERR_SM_DELETE_FAILED        248

typedef struct
{
  DWORD IOStatus;
  DWORD InfoBlockSize;
  DWORD AvailSpace;
  DWORD UsedSpace;
  /* A length preceded string is followed by SMInfo data */
  BYTE SMInfo[MAX_SIZE_OF_SM_STRING + MAX_SIZE_OF_SM_INFO];
} SUPPORT_MODULE_INFO;

typedef struct
{
  DWORD numberOfSMs;
  DWORD SMIDs[MAX_NUM_OF_SM];
} SUPPORT_MODULE_IDS;

NWCCODE NWAPI NWMoveFileToDM(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE nameSpace,
  DWORD supportModuleID,
  DWORD saveKeyFlag);

NWCCODE NWAPI NWMoveFileFromDM(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE nameSpace);

NWCCODE NWAPI NWGetDMFileInfo(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  BYTE nameSpace,
  DWORD NWPTR supportModuleID,
  DWORD NWPTR restoreTime,
  DWORD NWPTR dataStreams);

NWCCODE NWAPI NWGetDMVolumeInfo(
  NWCONN_HANDLE conn,
  WORD volume,
  DWORD supportModuleID,
  DWORD NWPTR numberOfFilesMigrated,
  DWORD NWPTR totalMigratedSize,
  DWORD NWPTR spaceUsedOnDM,
  DWORD NWPTR limboSpaceUsedOnDM,
  DWORD NWPTR spaceMigrated,
  DWORD NWPTR filesInLimbo);

NWCCODE NWAPI NWGetSupportModuleInfo(
  NWCONN_HANDLE conn,
  DWORD infomationLevel,
  DWORD supportModuleID,
  BYTE NWPTR returnInfo,
  DWORD NWPTR returnInfoLen);

NWCCODE NWAPI NWGetDataMigratorInfo(
  NWCONN_HANDLE conn,
  DWORD NWPTR DMPresentFlag,
  DWORD NWPTR majorVersion,
  DWORD NWPTR minorVersion,
  DWORD NWPTR DMSMRegistered);

NWCCODE NWAPI NWGetDefaultSupportModule(
  NWCONN_HANDLE conn,
  DWORD NWPTR supportModuleID);

NWCCODE NWAPI NWSetDefaultSupportModule(
  NWCONN_HANDLE conn,
  DWORD NWPTR supportModuleID);

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
$Header:   K:\archives\src\sdkinc\nwmigrat.h_v   1.1   09 Aug 1993 12:04:40   JWOODBUR  $
*/
