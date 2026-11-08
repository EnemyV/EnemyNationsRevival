/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWVOL_INC
#define NWVOL_INC

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

/* define volume types  */

#define VINetWare386    0
#define VINetWare286    1
#define VINetWare386v30 2
#define VINetWare386v31 3

/*    define the extended volume information status flag bits  */

#define NWSubAllocEnabledBit    0x01
#define NWCompressionEnabledBit 0x02
#define NWMigrationEnabledBit   0x04
#define NWAuditingEnabledBit    0x08
#define NWReadOnlyEnabledBit    0x10

typedef struct
{
  DWORD objectID;
  DWORD restriction;
} NWOBJ_REST;

typedef struct
{
  BYTE  numberOfEntries;
  struct
  {
    DWORD objectID;
    DWORD restriction;
  } resInfo[12];
} NWVolumeRestrictions;

typedef struct
{
  BYTE  numberOfEntries;
  struct
  {
    DWORD objectID;
    DWORD restriction;
  } resInfo[16];
} NWVOL_RESTRICTIONS;

typedef struct
{
  INT32 systemElapsedTime;
  BYTE volumeNumber;
  BYTE logicalDriveNumber;
  WORD sectorsPerBlock;
  WORD startingBlock;
  WORD totalBlocks;
  WORD availableBlocks;
  WORD totalDirectorySlots;
  WORD availableDirectorySlots;
  WORD maxDirectorySlotsUsed;
  BYTE isHashing;
  BYTE isCaching;
  BYTE isRemovable;
  BYTE isMounted;
  char volumeName[16];
} VOL_STATS;


typedef struct ExtendedVolInfo_tag
{
  DWORD volType;
  DWORD statusFlag;
  DWORD sectorSize;
  DWORD sectorsPerCluster;
  DWORD volSizeInClusters;
  DWORD freeClusters;
  DWORD subAllocFreeableClusters;
  DWORD freeableLimboSectors;
  DWORD nonfreeableLimboSectors;
  DWORD availSubAllocSectors;            /* non freeable */
  DWORD nonuseableSubAllocSectors;
  DWORD subAllocClusters;
  DWORD numDataStreams;
  DWORD numLimboDataStreams;
  DWORD oldestDelFileAgeInTicks;
  DWORD numCompressedDataStreams;
  DWORD numCompressedLimboDataStreams;
  DWORD numNoncompressibleDataStreams;
  DWORD precompressedSectors;
  DWORD compressedSectors;
  DWORD numMigratedDataStreams;
  DWORD migratedSectors;
  DWORD clustersUsedByFAT;
  DWORD clustersUsedByDirs;
  DWORD clustersUsedByExtDirs;
  DWORD totalDirEntries;
  DWORD unusedDirEntries;
  DWORD totalExtDirExtants;
  DWORD unusedExtDirExtants;
  DWORD extAttrsDefined;
  DWORD extAttrExtantsUsed;
  DWORD DirectoryServicesObjectID;
  DWORD volLastModifiedDateAndTime;
} NWVolExtendedInfo;


NWCCODE NWAPI NWGetDiskUtilization(
  NWCONN_HANDLE conn,
  DWORD objectID,
  BYTE volumeNumber,
  WORD NWPTR usedDirectories,
  WORD NWPTR usedFiles,
  WORD NWPTR usedBlocks);

NWCCODE NWAPI NWGetObjDiskRestrictions(
  NWCONN_HANDLE conn,
  BYTE  volNumber,
  DWORD objectID,
  DWORD NWPTR restriction,
  DWORD NWPTR inUse);

NWCCODE NWAPI NWScanVolDiskRestrictions(
  NWCONN_HANDLE conn,
  BYTE volNumber,
  DWORD NWPTR sequence,
  NWVolumeRestrictions NWPTR volInfo);

NWCCODE NWAPI NWScanVolDiskRestrictions2(
  NWCONN_HANDLE conn,
  BYTE volNum,
  DWORD NWPTR seq,
  NWVOL_RESTRICTIONS NWPTR volInfo);

NWCCODE NWAPI NWRemoveObjectDiskRestrictions(
  NWCONN_HANDLE conn,
  BYTE volNumber,
  DWORD objectID);

NWCCODE NWAPI NWSetObjectVolSpaceLimit(
  NWCONN_HANDLE conn,
  WORD volNumber,
  DWORD objectID,
  DWORD restriction);

NWCCODE NWAPI NWGetVolumeInfoWithHandle(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR volName,
  WORD NWPTR totalBlocks,
  WORD NWPTR sectorsPerBlock,
  WORD NWPTR availableBlocks,
  WORD NWPTR totalDirEntries,
  WORD NWPTR availableDirEntries,
  WORD NWPTR volIsRemovableFlag);

NWCCODE NWAPI NWGetVolumeInfoWithNumber(
  NWCONN_HANDLE conn,
  WORD volNumber,
  char NWPTR volName,
  WORD NWPTR totalBlocks,
  WORD NWPTR sectorsPerBlock,
  WORD NWPTR availableBlocks,
  WORD NWPTR totalDirEntries,
  WORD NWPTR availableDirEntries,
  WORD NWPTR volIsRemovableFlag);

NWCCODE NWAPI NWGetVolumeName(
  NWCONN_HANDLE conn,
  WORD volNum,
  char NWPTR volName);

NWCCODE NWAPI NWGetVolumeNumber(
  NWCONN_HANDLE conn,
  char NWPTR volName,
  WORD NWPTR volNumber);

NWCCODE NWAPI NWGetVolumeStats(
  NWCONN_HANDLE conn,
  BYTE volumeNumber,
  VOL_STATS NWPTR volInfo );

NWCCODE NWAPI NWGetExtendedVolumeInfo(
  NWCONN_HANDLE conn,
  WORD volNumber,
  NWVolExtendedInfo NWPTR volInfo);

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
$Header:   K:\archives\src\sdkinc\nwvol.h_v   1.2   10 Aug 1993 12:50:38   JWOODBUR  $
*/
