/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWBINDRY_INC
#define NWBINDRY_INC

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

/* Bindery object types (in HIGH-LOW order) */
#define OT_WILD                 0xFFFF
#define OT_UNKNOWN              0x0000
#define OT_USER                 0x0100
#define OT_USER_GROUP           0x0200
#define OT_PRINT_QUEUE          0x0300
#define OT_FILE_SERVER          0x0400
#define OT_JOB_SERVER           0x0500
#define OT_GATEWAY              0x0600
#define OT_PRINT_SERVER         0x0700
#define OT_ARCHIVE_QUEUE        0x0800
#define OT_ARCHIVE_SERVER       0x0900
#define OT_JOB_QUEUE            0x0A00
#define OT_ADMINISTRATION       0x0B00
#define OT_NAS_SNA_GATEWAY      0x2100
#define OT_REMOTE_BRIDGE_SERVER 0x2600
#define OT_TCPIP_GATEWAY        0x2700

/* Extended bindery object types */
#define OT_TIME_SYNCHRONIZATION_SERVER 0x2D00
#define OT_ARCHIVE_SERVER_DYNAMIC_SAP  0x2E00
#define OT_ADVERTISING_PRINT_SERVER    0x4700
#define OT_BTRIEVE_VAP                 0x5000
#define OT_PRINT_QUEUE_USER            0x5300


/* Bindery object and property flags */
#define BF_STATIC   0x00
#define BF_DYNAMIC  0x01
#define BF_ITEM     0x00
#define BF_SET      0x02

/*********  Bindery object and property security access levels  **********/
#define BS_ANY_READ      0x00   /* Readable by anyone                */
#define BS_LOGGED_READ   0x01   /* Must be logged in to read         */
#define BS_OBJECT_READ   0x02   /* Readable by same object or super  */
#define BS_SUPER_READ    0x03   /* Readable by supervisor only       */
#define BS_BINDERY_READ  0x04   /* Readable only by the bindery      */
#define BS_ANY_WRITE     0x00   /* Writeable by anyone               */
#define BS_LOGGED_WRITE  0x10   /* Must be logged in to write        */
#define BS_OBJECT_WRITE  0x20   /* Writeable by same object or super */
#define BS_SUPER_WRITE   0x30   /* Writeable only by the supervisor  */
#define BS_BINDERY_WRITE 0x40   /* Writeable by the bindery only     */

NWCCODE NWAPI NWVerifyObjectPassword(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR password);

NWCCODE NWAPI NWDisallowObjectPassword(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR disallowedPassword);

NWCCODE NWAPI NWChangeObjectPassword(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR oldPassword,
  char NWPTR newPassword);

NWCCODE NWAPI NWReadPropertyValue(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  BYTE segmentNumber,
  BYTE NWPTR segmentData,
  BYTE NWPTR moreSegments,
  BYTE NWPTR flags);

NWCCODE NWAPI NWWritePropertyValue(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  BYTE segmentNumber,
  BYTE NWPTR segmentData,
  BYTE moreSegments);

NWCCODE NWAPI NWAddObjectToSet(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  char NWPTR memberName,
  WORD memberType);

NWCCODE NWAPI NWDeleteObjectFromSet(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  char NWPTR memberName,
  WORD memberType);

NWCCODE NWAPI NWIsObjectInSet(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  char NWPTR memberName,
  WORD memberType);

NWCCODE NWAPI NWScanProperty(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR searchPropertyName,
  DWORD NWPTR sequenceNumber,
  char NWPTR propertyName,
  BYTE NWPTR propertyFlags,
  BYTE NWPTR propertySecurity,
  BYTE NWPTR hasValueFlag,
  BYTE NWPTR moreFlag);

NWCCODE NWAPI NWGetObjectID(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  DWORD NWPTR objectID);

NWCCODE NWAPI NWGetObjectDiskSpaceLeft(
  NWCONN_HANDLE conn,
  DWORD objectID,
  DWORD NWPTR systemElapsedTime,
  DWORD NWPTR unusedDiskBlocks,
  BYTE NWPTR restrictionEnforced);


NWCCODE NWAPI NWGetObjectName(
  NWCONN_HANDLE conn,
  DWORD objectID,
  char NWPTR objectName,
  WORD NWPTR objectType);

NWCCODE NWAPI NWScanObject(
  NWCONN_HANDLE conn,
  char NWPTR searchName,
  WORD searchType,
  DWORD NWPTR objectID,
  char NWPTR objectName,
  WORD NWPTR objectType,
  BYTE NWPTR hasPropertiesFlag,
  BYTE NWPTR objectFlags,
  BYTE NWPTR objectSecurity);

NWCCODE NWAPI NWGetBinderyAccessLevel(
  NWCONN_HANDLE conn,
  BYTE NWPTR accessLevel,
  DWORD NWPTR objectID);

NWCCODE NWAPI NWCreateProperty(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  BYTE propertyFlags,
  BYTE propertySecurity);

NWCCODE NWAPI NWDeleteProperty(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName);

NWCCODE NWAPI NWChangePropertySecurity(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR propertyName,
  BYTE newPropertySecurity);

NWCCODE NWAPI NWCreateObject(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  BYTE objectFlags,
  BYTE objectSecurity);

NWCCODE NWAPI NWDeleteObject(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType);

NWCCODE NWAPI NWRenameObject(
  NWCONN_HANDLE conn,
  char NWPTR oldObjectName,
  char NWPTR newObjectName,
  WORD objectType);

NWCCODE NWAPI NWChangeObjectSecurity(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  BYTE newObjectSecurity);

NWCCODE NWAPI NWOpenBindery(NWCONN_HANDLE conn);
NWCCODE NWAPI NWCloseBindery(NWCONN_HANDLE conn);

NWCCODE NWAPI NWScanObjectTrusteePaths(
  NWCONN_HANDLE conn,
  DWORD objectID,
  WORD volNumber,
  WORD NWPTR sequenceNumber,
  BYTE NWPTR accessRights,
  char NWPTR dirPath);

NWCCODE NWAPI NWGetObjectEffectiveRights(
  NWCONN_HANDLE conn,
  DWORD objectID,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  WORD NWPTR rightsMask);

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
$Header:   K:\archives\src\sdkinc\nwbindry.h_v   1.1   09 Aug 1993 12:04:34   JWOODBUR  $
*/
