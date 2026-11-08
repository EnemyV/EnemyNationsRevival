/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWDPATH_INC
#define NWDPATH_INC

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

/* status values for NWGetDriveStatus */
#define NW_UNMAPPED_DRIVE     0x0000
#define NW_FREE_DRIVE         0x0000
#define NW_CDROM_DRIVE        0x0400
#define NW_LOCAL_FREE_DRIVE   0x0800
#define NW_LOCAL_DRIVE        0x1000
#define NW_NETWORK_DRIVE      0x2000
#define NW_LITE_DRIVE         0x4000
#define NW_PNW_DRIVE          0x4000
#define NW_NETWARE_DRIVE      0x8000

/* return error for NWGetDriveStatus */
#define NW_INVALID_DRIVE       15

/* defined for pathFormat parameter in NWGetDriveStatus */
#define NW_FORMAT_NETWARE       0
#define NW_FORMAT_SERVER_VOLUME 1
#define NW_FORMAT_DRIVE         2
#define NW_FORMAT_UNC           3

NWCCODE NWAPI NWSetDriveBase(
  WORD driveNumber,
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR dirPath,
  WORD driveScope);

int NWAPI NWGetInitDrive(
   void);

NWCCODE NWAPI NWSetInitDrive(
   NWCONN_HANDLE conn);

NWCCODE NWAPI NWSetInitDrive2(
   NWCONN_HANDLE conn,
   int drive);

NWCCODE NWAPI NWSetSearchDriveVector(
   char NWPTR vectorBuffer );

NWCCODE NWAPI NWGetSearchDriveVector(
   char NWPTR vectorBuffer );

NWCCODE NWAPI NWDeleteDriveBase(
  WORD driveNumber,
  WORD driveScope);

NWCCODE NWAPI NWGetPathFromDirectoryEntry(
  NWCONN_HANDLE conn,
  BYTE volume,
  WORD dirEntry,
  BYTE NWPTR length,
  BYTE NWPTR pathName);

NWCCODE NWAPI NWGetDrivePath(
  WORD driveNumber,
  WORD mode,
  NWCONN_HANDLE NWPTR conn,
  char NWPTR basePath,
  WORD NWPTR driveScope);

NWCCODE NWAPI NWGetDriveInformation(
  WORD driveNumber,
  WORD mode,
  NWCONN_HANDLE NWPTR conn,
  NWDIR_HANDLE NWPTR dirHandle,
  WORD NWPTR driveScope,
  char NWPTR dirPath);

NWCCODE NWAPI NWGetDriveStatus(
  WORD driveNumber,
  WORD pathFormat,
  WORD NWPTR status,
  NWCONN_HANDLE NWPTR conn,
  char NWPTR rootPath,
  char NWPTR relativePath,
  char NWPTR fullPath);

NWCCODE NWAPI NWGetFirstDrive(
  WORD NWPTR firstDrive);

NWCCODE NWAPI NWParseNetWarePath(
  char NWPTR path,
  NWCONN_HANDLE NWPTR conn,
  NWDIR_HANDLE NWPTR dirHandle,
  char NWPTR newPath);

NWCCODE NWAPI NWParsePath(
  char NWPTR path,
  char NWPTR serverName,
  NWCONN_HANDLE NWPTR conn,
  char NWPTR volName,
  char NWPTR dirPath);

char NWPTR  NWAPI NWStripServerOffPath(
  char NWPTR path,
  char NWPTR server);

NWCCODE NWAPI NWCreateUNCPath(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWFAR *path,
  char NWFAR *UNCPath);

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
$Header:   K:\archives\src\sdkinc\nwdpath.h_v   1.5   21 Oct 1993 12:56:28   JWOODBUR  $
*/
