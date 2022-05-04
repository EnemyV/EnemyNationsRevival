/*--------------------------------------------------------------------------
   Copyright (c) 1991, Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWSERVER_INC
#define NWSERVER_INC

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

#define LNS_CHECK 0

#ifndef NW_SHORT_NAME_SERVER
# define NW_SHORT_NAME_SERVER 0
#endif

#ifndef NW_LONG_NAME_SERVER
#define NW_LONG_NAME_SERVER 1
#endif

#ifndef NW_ENCP_SERVER
#define NW_ENCP_SERVER 1
#endif

#ifndef NW_EXTENDED_NCP_SERVER
#define NW_EXTENDED_NCP_SERVER 1
#endif

#define VERSION_CHECK 1
# define NW_2X  0
# define NW_30  1
# define NW_311 2
# define NW_32  3
# define NW_40  4

typedef struct
{
   BYTE networdAddress[4];
   BYTE hostAddress[6];
   BYTE boardInstalled;
   BYTE optionNumber;
   BYTE configurationText1[80];
   BYTE configurationText2[80];
} NWLAN_CONFIG;

typedef struct
{
   DWORD systemElapsedTime;
   WORD maxRoutingBuffersAvail;
   WORD maxRoutingBuffersUsed;
   WORD routingBuffersInUse;
   DWORD totalFileServicePackets;
   WORD fileServicePacketsBuffered;
   WORD invalidConnPacketCount;
   WORD badLogicalConnCount;
   WORD packetsRcvdDuringProcCount;
   WORD reprocessedRequestCount;
   WORD badSequenceNumberPacketCount;
   WORD duplicateReplyCount;
   WORD acknowledgementsSent;
   WORD badRequestTypeCount;
   WORD attachDuringProcCount;
   WORD attachWhileAttachingCount;
   WORD forgedDetachRequestCount;
   WORD badConnNumberOnDetachCount;
   WORD detachDuringProcCount;
   WORD repliesCanceledCount;
   WORD hopCountDiscardCount;
   WORD unknownNetDiscardCount;
   WORD noDGroupBufferDiscardCount;
   WORD outPacketNoBufferDiscardCount;
   WORD IPXNotMyNetworkCount;
   DWORD NetBIOSPropagationCount;
   DWORD totalOtherPackets;
   DWORD totalRoutedPackets;
}  SERVER_LAN_IO_STATS;


typedef struct
{
   DWORD systemElapsedTime;
   BYTE diskChannel;
   BYTE diskRemovable;
   BYTE driveType;
   BYTE controllerDriveNumber;
   BYTE controllerNumber;
   BYTE controllerType;
   DWORD driveSize;            /* in 4096 byte blocks */
   WORD driveCylinders;
   BYTE driveHeads;
   BYTE sectorsPerTrack;
   BYTE driveDefinition[64];
   WORD IOErrorCount;
   DWORD hotFixStart;          /* only meaningful with SFT I or greater */
   WORD hotFixSize;           /* only meaningful with SFT I or greater */
   WORD hotFixBlockAvailable; /* only meaningful with SFT I or greater */
   BYTE hotFixDisabled;       /* only meaningful with SFT I or greater */
} PHYS_DSK_STATS;

typedef struct
{
   DWORD systemElapsedTime;
   WORD channelState;
   WORD channelSyncState;
   BYTE driverType;
   BYTE driverMajorVersion;
   BYTE driverMinorVersion;
   BYTE driverDescription[65];
   WORD IOAddr1;
   WORD IOAddr1Size;
   WORD IOAddr2;
   WORD IOAddr2Size;
   BYTE sharedMem1Seg[3];
   WORD sharedMem1Ofs;
   BYTE sharedMem2Seg[3];
   WORD sharedMem2Ofs;
   BYTE interrupt1Used;
   BYTE interrupt1;
   BYTE interrupt2Used;
   BYTE interrupt2;
   BYTE DMAChannel1Used;
   BYTE DMAChannel1;
   BYTE DMAChannel2Used;
   BYTE DMAChannel2;
   WORD reserved2;
   BYTE configDescription[80];
}  DSK_CHANNEL_STATS;

typedef struct
{
   DWORD systemElapsedTime;
   WORD cacheBufferCount;
   WORD cacheBufferSize;
   WORD dirtyCacheBuffers;
   DWORD cacheReadRequests;
   DWORD cacheWriteRequests;
   DWORD cacheHits;
   DWORD cacheMisses;
   DWORD physicalReadRequests;
   DWORD physicalWriteRequests;
   WORD physicalReadErrors;
   WORD physicalWriteErrors;
   DWORD cacheGetRequests;
   DWORD cacheFullWriteRequests;
   DWORD cachePartialWriteRequests;
   DWORD backgroundDirtyWrites;
   DWORD backgroundAgedWrites;
   DWORD totalCacheWrites;
   DWORD cacheAllocations;
   WORD thrashingCount;
   WORD LRUBlockWasDirtyCount;
   WORD readBeyondWriteCount;
   WORD fragmentedWriteCount;
   WORD cacheHitOnUnavailCount;
   WORD cacheBlockScrappedCount;
} DSK_CACHE_STATS;

typedef struct
{
   DWORD systemElapsedTime;
   WORD maxOpenFiles;
   WORD maxFilesOpened;
   WORD currOpenFiles;
   DWORD totalFilesOpened;
   DWORD totalReadRequests;
   DWORD totalWriteRequests;
   WORD currChangedFATSectors;
   DWORD totalChangedFATSectors;
   WORD FATWriteErrors;
   WORD fatalFATWriteErrors;
   WORD FATScanErrors;
   WORD maxIndexFilesOpened;
   WORD currOpenIndexedFiles;
   WORD attachedIndexFiles;
   WORD availableIndexFiles;
} FILESYS_STATS;


typedef struct
{
   DWORD systemElapsedTime;
   BYTE SFTSupportLevel;
   BYTE logicalDriveCount;
   BYTE physicalDriveCount;
   BYTE diskChannelTable[5];
   WORD pendingIOCommands;
   BYTE driveMappingTable[32];
   BYTE driveMirrorTable[32];
   BYTE deadMirrorTable[32];
   BYTE reMirrorDriveNumber;
   BYTE reserved;
   DWORD reMirrorCurrentOffset;
   WORD SFTErrorTable[60];
}  DRV_MAP_TABLE;


typedef struct
{
   BYTE serverName[48];
   BYTE fileServiceVersion;
   BYTE fileServiceSubVersion;
   WORD maximumServiceConnections;
   WORD connectionsInUse;
   WORD maxNumberVolumes;
   BYTE revision;
   BYTE SFTLevel;
   BYTE TTSLevel;
   WORD maxConnectionsEverUsed;
   BYTE accountVersion;
   BYTE VAPVersion;
   BYTE queueVersion;
   BYTE printVersion;
   BYTE virtualConsoleVersion;
   BYTE restrictionLevel;
   BYTE internetBridge;
   BYTE reserved[60];
}  VERSION_INFO;

/* Defines that are used for the NWCheckNetWareVersion call for values
   that can be returned in the compatibilityFlag BYTE.  */
#define COMPATIBLE               0x00
#define VERSION_NUMBER_TOO_LOW   0x01
#define SFT_LEVEL_TOO_LOW        0x02
#define TTS_LEVEL_TOO_LOW        0x04

/* structures for NWGetFileServerMiscInfo (2.2 only) */
typedef struct tNW_MEM_AREAS
{
  DWORD total;    /* total amount of memory in dynamic memory area */
  DWORD max;      /* amount of memory in dynamic memory area that has been in use since server was brought up */
  DWORD cur;      /* amount of memory in dynamic memory area currently in use */
} NW_DYNAMIC_MEM;

typedef struct tNW_FS_MISC
{
  DWORD upTime;        /* how long file server's been up in 1/18 ticks (wraps at 0xffffffff) */
  BYTE processor;      /* 1 = 8086/8088, 2 = 80286       */
  BYTE reserved;
  BYTE numProcs;       /* number processes that handle incoming service requests */
  BYTE utilization;    /* server utilization percentage (0-100), updated once/sec */
  WORD configuredObjs; /* max number of bindery objects file server will track - 0=unlimited & next 2 fields have no meaning */
  WORD maxObjs;        /* max number of bindery objects that have been used concurrently since file server came up */
  WORD curObjs;        /* actual number of bindery objects currently in use on server */
  WORD totalMem;       /* total amount of memory (in K) installed on server */
  WORD unusedMem;      /* amount of memory server has determined it is not using */
  WORD numMemAreas;    /* number of dynamic memory areas (1-3) */
  NW_DYNAMIC_MEM dynamicMem[3];
} NW_FS_INFO;


NWCCODE NWAPI NWGetPhysicalDiskStats(
  NWCONN_HANDLE conn,
  BYTE physicalDiskNumber,
  PHYS_DSK_STATS NWPTR statBuffer);

NWCCODE NWAPI NWGetFileSystemStats(
  NWCONN_HANDLE conn,
  FILESYS_STATS NWPTR statBuffer);

NWCCODE NWAPI NWGetDiskChannelStats(
  NWCONN_HANDLE conn,
  BYTE channelNumber,
  DSK_CHANNEL_STATS NWPTR statBuffer);

NWCCODE NWAPI NWGetDiskCacheStats(
  NWCONN_HANDLE conn,
  DSK_CACHE_STATS NWPTR statBuffer);

NWCCODE NWAPI NWGetFSDriveMapTable(
  NWCONN_HANDLE conn,
  DRV_MAP_TABLE NWPTR tableBuffer);

NWCCODE NWAPI NWGetFSLANDriverConfigInfo(
  NWCONN_HANDLE conn,
   BYTE lanBoardNumber,
  NWLAN_CONFIG NWPTR lanConfig);

NWCCODE NWAPI NWGetFileServerLANIOStats(
  NWCONN_HANDLE conn,
  SERVER_LAN_IO_STATS NWPTR statBuffer);

NWCCODE NWAPI NWCheckConsolePrivileges(NWCONN_HANDLE conn);

NWCCODE NWAPI NWDownFileServer(
  NWCONN_HANDLE conn,
  BYTE forceFlag);

NWCCODE NWAPI NWGetFileServerDateAndTime(
  NWCONN_HANDLE conn,
  BYTE NWPTR dateTimeBuffer);

NWCCODE NWAPI NWSetFileServerDateAndTime(
  NWCONN_HANDLE conn,
  BYTE year,
  BYTE month,
  BYTE day,
  BYTE hour,
  BYTE minute,
  BYTE second);

NWCCODE NWAPI NWCheckNetWareVersion(
  NWCONN_HANDLE conn,
  WORD minimumVersion,
  WORD minimumSubVersion,
  WORD minimumRevision,
  WORD minimumSFT,
  WORD minimumTTS,
  BYTE NWPTR compatibilityFlag );

NWCCODE NWAPI NWGetFileServerVersionInfo(
  NWCONN_HANDLE conn,
  VERSION_INFO NWPTR versBuffer);

NWCCODE NWAPI NWGetFileServerInformation(
  NWCONN_HANDLE conn,
  char NWPTR serverName,
  BYTE NWPTR majorVersion,
  BYTE NWPTR minorVersion,
  BYTE NWPTR revision,
  WORD NWPTR maxConnections,
  WORD NWPTR maxConnectionsUsed,
  WORD NWPTR connectionsInUse,
  WORD NWPTR numVolumes,
  BYTE NWPTR SFTLevel,
  BYTE NWPTR TTSLevel);

NWCCODE NWAPI NWGetFileServerExtendedInfo(
  NWCONN_HANDLE conn,
  BYTE NWPTR accountingVersion,
  BYTE NWPTR VAPversion,
  BYTE NWPTR queueingVersion,
  BYTE NWPTR printServerVersion,
  BYTE NWPTR virtualConsoleVersion,
  BYTE NWPTR securityVersion,
  BYTE NWPTR internetBridgeVersion);

NWCCODE NWAPI _NWGetFileServerType(
  NWCONN_HANDLE conn,
  WORD typeFlag,
  WORD NWPTR serverType);

NWCCODE NWAPI NWAttachToFileServer(
  char NWPTR serverName,
  WORD scopeFlag,
  NWCONN_HANDLE NWPTR newConnID);

NWCCODE NWAPI NWGetFileServerLoginStatus(
  NWCONN_HANDLE conn,
  BYTE NWPTR loginEnabledFlag);

NWCCODE NWAPI NWDetachFromFileServer(NWCONN_HANDLE conn);

NWCCODE NWAPI NWGetFileServerName(NWCONN_HANDLE conn, char NWPTR serverName);

NWCCODE NWAPI NWLogoutFromFileServer(NWCONN_HANDLE conn);

NWCCODE NWAPI NWLoginToFileServer(
  NWCONN_HANDLE conn,
  char NWPTR objectName,
  WORD objectType,
  char NWPTR password);

NWCCODE NWAPI NWEnableFileServerLogin(NWCONN_HANDLE conn);

NWCCODE NWAPI NWDisableFileServerLogin(NWCONN_HANDLE conn);

NWCCODE NWAPI NWGetFileServerDescription(
  NWCONN_HANDLE conn,
  char NWPTR companyName,
  char NWPTR revision,
  char NWPTR revisionDate,
  char NWPTR copyrightNotice);

NWCCODE NWAPI NWGetFileServerVersion(
  NWCONN_HANDLE conn,
  WORD NWPTR serverVersion);

NWCCODE NWAPI NWAttachToFileServerByConn(
  NWCONN_HANDLE conn,
  char NWPTR serverName,
  WORD scopeFlag,
  NWCONN_HANDLE NWPTR newConnID);

NWCCODE NWAPI NWGetNetworkSerialNumber(
  NWCONN_HANDLE conn,
  DWORD NWFAR *serialNum,
  WORD NWFAR *appNum);

NWCCODE NWAPI NWIsManager(
  NWCONN_HANDLE conn);

/* this function is 2.2 specific */
NWCCODE NWAPI NWGetFileServerMiscInfo(
  NWCONN_HANDLE conn,
  NW_FS_INFO NWFAR *fsInfo);

#ifdef NWOS2
NWCCODE NWAPI NWLogoutWithLoginID(DWORD citrixLoginID);
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
$Header:   K:\archives\src\sdkinc\nwserver.h_v   1.4   08 Oct 1993 18:10:10   JWOODBUR  $
*/
