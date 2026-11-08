/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWFSE_INC
#define NWFSE_INC

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
  DWORD currentServerTime;
  BYTE vconsoleVersion;
  BYTE vconsoleRevision;
} SERVER_AND_VCONSOLE_INFO;

/* Get Cache Information */

typedef struct
{
  DWORD readExistingBlockCount;
  DWORD readExistingWriteWaitCount;
  DWORD readExistingPartialReadCount;
  DWORD readExistingReadErrorCount;
  DWORD writeBlockCount;
  DWORD writeEntireBlockCount;
  DWORD getDiskCount;
  DWORD getDiskNeedToAllocCount;
  DWORD getDiskSomeoneBeatMeCount;
  DWORD getDiskPartialReadCount;
  DWORD getDiskReadErrorCount;
  DWORD getAsyncDiskCount;
  DWORD getAsyncDiskNeedToAlloc;
  DWORD getAsyncDiskSomeoneBeatMe;
  DWORD errorDoingAsyncReadCount;
  DWORD getDiskNoReadCount;
  DWORD getDiskNoReadAllocCount;
  DWORD getDiskNoReadSomeoneBeatMeCount;
  DWORD diskWriteCount;
  DWORD diskWriteAllocCount;
  DWORD diskWriteSomeoneBeatMeCount;
  DWORD writeErrorCount;
  DWORD waitOnSemaphoreCount;
  DWORD allocBlockWaitForSomeoneCount;
  DWORD allocBlockCount;
  DWORD allocBlockWaitCount;
} CACHE_COUNTERS;

typedef struct
{
  DWORD originalNumOfCacheBuffers;
  DWORD currentNumOfCacheBuffers;
  DWORD cacheDirtyBlockThreshold;
  DWORD waitNodeCount;
  DWORD waitNodeAllocFailureCount;
  DWORD moveCacheNodeCount;
  DWORD moveCacheNodeFromAvailCount;
  DWORD accelerateCacheNodeWriteCount;
  DWORD removeCacheNodeCount;
  DWORD removeCacheNodeFromAvailCount;
} CACHE_MEM_COUNTERS;

typedef struct
{
  DWORD numCacheChecks;
  DWORD numCacheHits;
  DWORD numDirtyCacheChecks;
  DWORD numDirtyCacheHits;
  DWORD cacheUsedWhileChecking;
  DWORD waitForDirtyBlocksDecreaseCount;
  DWORD allocBlockFromAvailCount;
  DWORD allocBlockFromLRUCount;
  DWORD allocBlockAlreadyWaiting;
  DWORD LRUSittingTime;
} CACHE_TREND_COUNTERS;

typedef struct
{
  DWORD maxByteCount;
  DWORD minNumOfCacheBuffers;
  DWORD minCacheReportThreshold;
  DWORD allocWaitingCount;
  DWORD numDirtyBlocks;
  DWORD cacheDirtyWaitTime;
  DWORD cacheMaxConcurrentWrites;
  DWORD maxDirtyTime;
  DWORD numOfDirCacheBuffers;
  DWORD cacheByteToBlockShiftFactor;
} CACHE_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  CACHE_COUNTERS cacheCounters;
  CACHE_MEM_COUNTERS cacheMemCounters;
  CACHE_TREND_COUNTERS cacheTrendCounters;
  CACHE_INFO cacheInformation;
} NWFSE_CACHE_INFO;

/* Get File Server Information */

typedef struct
{
  DWORD replyCanceledCount;
  DWORD writeHeldOffCount;
  DWORD writeHeldOffWithDupRequest;
  /* writeHeldOffWithDuplicateRequest */
  DWORD invalidRequestTypeCount;
  DWORD beingAbortedCount;
  DWORD alreadyDoingReallocCount;
  DWORD deAllocInvalidSlotCount;
  DWORD deAllocBeingProcessedCount;
  DWORD deAllocForgedPacketCount;
  DWORD deAllocStillTransmittingCount;
  DWORD startStationErrorCount;
  DWORD invalidSlotCount;
  DWORD beingProcessedCount;
  DWORD forgedPacketCount;
  DWORD stillTransmittingCount;
  DWORD reExecuteRequestCount;
  DWORD invalidSequenceNumCount;
  DWORD duplicateIsBeingSentAlreadyCnt;
  DWORD sentPositiveAcknowledgeCount;
  DWORD sentDuplicateReplyCount;
  DWORD noMemForStationCtrlCount;
  DWORD noAvailableConnsCount;
  DWORD reallocSlotCount;
  DWORD reallocSlotCameTooSoonCount;
} FSE_SERVER_INFO;

typedef struct
{
  WORD tooManyHops;
  WORD unknownNetwork;
  WORD noSpaceForService;
  WORD noReceiveBuffers;
  WORD notMyNetwork;
  DWORD netBIOSProgatedCount;
  DWORD totalPacketsServiced;
  DWORD totalPacketsRouted;
} FILE_SERVER_COUNTERS;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD NCPStationsInUseCount;
  DWORD NCPPeakStationsInUseCount;
  DWORD numOfNCPRequests;
  DWORD serverUtilization;
  FSE_SERVER_INFO ServerInfo;
  FILE_SERVER_COUNTERS fileServerCounters;
} NWFSE_FILE_SERVER_INFO;

/* Netware File Systems Information */

typedef struct
{
  DWORD FATMovedCount;
  DWORD FATWriteErrorCount;
  DWORD someoneElseDidItCount0;
  DWORD someoneElseDidItCount1;
  DWORD someoneElseDidItCount2;
  DWORD iRanOutSomeoneElseDidItCount0;
  DWORD iRanOutSomeoneElseDidItCount1;
  DWORD iRanOutSomeoneElseDidItCount2;
  DWORD turboFATBuildScrededUpCount;
  DWORD extraUseCountNodeCount;
  DWORD extraExtraUseCountNodeCount;
  DWORD errorReadingLastFATCount;
  DWORD someoneElseUsingThisFileCount;
} FSE_FILE_SYSTEM_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  FSE_FILE_SYSTEM_INFO fileSystemInfo;
} NWFSE_FILE_SYSTEM_INFO;

/* User Information */

/* status */
#define FSE_LOGGED_IN                   0x00000001
#define FSE_BEING_ABORTED               0x00000002
#define FSE_AUDITED                     0x00000004
#define FSE_NEEDS_SECURITY_CHANGE       0x00000008
#define FSE_MAC_STATION                 0x00000010
#define FSE_AUTHENTICATED_TEMPORARY     0x00000020
#define FSE_AUDIT_CONNECTION_RECORDED   0x00000040
#define FSE_DSAUDIT_CONNECTION_RECORDED 0x00000080

/* fileWriteFlags */
#define FSE_WRITE          1
#define FSE_WRITE_ABORTED  2

/* fileWriteState */
#define FSE_NOT_WRITING          0
#define FSE_WRITE_IN_PROGRESS    1
#define FSE_WRITE_BEING_STOPPED  2

typedef struct
{
  DWORD connNum;
  DWORD useCount;
  BYTE connServiceType;
  BYTE loginTime[ 7 ];
  DWORD status;
  DWORD expirationTime;
  DWORD objType;
  BYTE transactionFlag;
  BYTE logicalLockThreshold;
  BYTE recordLockThreshold;
  BYTE fileWriteFlags;         /* Includes active and stop bits */
  BYTE fileWriteState;
  BYTE filler;
  WORD fileLockCount;
  WORD recordLockCount;
  BYTE totalBytesRead[ 6 ];
  BYTE totalBytesWritten[ 6 ];
  DWORD totalRequests;
  DWORD heldRequests;
  BYTE heldBytesRead[ 6 ];
  BYTE heldBytesWritten[ 6 ];
} USER_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  USER_INFO userInfo;
} NWFSE_USER_INFO;

/* Packet Burst Information */

typedef struct
{
  DWORD bigInvalidSlotCount;
  DWORD bigForgedPacketCount;
  DWORD bigInvalidPacketCount;
  DWORD bigStillTransmittingCount;
  DWORD stillDoingTheLastRequestCount;
  DWORD invalidCtrlRequestCount;
  DWORD ctrlInvalidMessageNumCount;
  DWORD ctrlBeingTornDownCount;
  DWORD bigRepeatTheFileReadCount;
  DWORD bigSendExtraCCCount;
  DWORD bigReturnAbortMessageCount;
  DWORD bigReadInvalidMessageNumCount;
  DWORD bigReadDoItOverCount;
  DWORD bigReadBeingTornDownCount;
  DWORD previousCtrlPacketCount;
  DWORD sendHoldOffMessageCount;
  DWORD bigReadNoDataAvailableCount;
  DWORD bigReadTryingToReadTooMuchCount;
  DWORD asyncReadErrorCount;
  DWORD bigReadPhysicalReadErrorCount;
  DWORD ctrlBadACKFragmentListCount;
  DWORD ctrlNoDataReadCount;
  DWORD writeDuplicateRequestCount;
  DWORD shouldntBeACKingHereCount;
  DWORD writeInconsistentPktLengthsCnt;
  DWORD firstPacketIsntAWriteCount;
  DWORD writeTrashedDuplicateRequestCnt;
  DWORD bigWriteInvalidMessageNumCount;
  DWORD bigWriteBeingTornDownCount;
  DWORD bigWriteBeingAbortedCount;
  DWORD zeroACKFragmentCountCount;
  DWORD writeCurrentlyTransmittingCount;
  DWORD tryingToWriteTooMuchCount;
  DWORD writeOutOfMemForCtrlNodesCount;
  DWORD writeDidntNeedThisFragmentCount;
  DWORD writeTooManyBuffsCheckedOutCnt;
  /* writeTooManyBuffersCheckedOutCount */
  DWORD writeTimeOutCount;
  DWORD writeGotAnACKCount;
  DWORD writeGotAnACKCount1;
  DWORD pollerAbortedTheConnCount;
  DWORD maybeHadOutOfOrderWritesCount;
  DWORD hadAnOutOfOrderWriteCount;
  DWORD movedTheACKBitDownCount;
  DWORD bumpedOutOfOrderWriteCount;
  DWORD pollerRemovedOldOutOfOrderCount;
  DWORD writeDidntNeedButRequestACKCnt;
  /* writeDidntNeedButRequestedACTCount */
  DWORD writeTrashedPacketCount;
  DWORD tooManyACKFragmentsCount;
  DWORD savedAnOutOfOrderPacketCount;
  DWORD connBeingAbortedCount;
} PACKET_BURST_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  PACKET_BURST_INFO packetBurstInfo;
} NWFSE_PACKET_BURST_INFO;

/* IPX SPX Information */

typedef struct
{
  DWORD IPXSendPacketCount;
  WORD IPXMalformPacketCount;
  DWORD IPXGetECBRequestCount;
  DWORD IPXGetECBFailCount;
  DWORD IPXAESEventCount;
  WORD IPXPostponedAESCount;
  WORD IPXMaxConfiguredSocketCount;
  WORD IPXMaxOpenSocketCount;
  WORD IPXOpenSocketFailCount;
  DWORD IPXListenECBCount;
  WORD IPXECBCancelFailCount;
  WORD IPXGetLocalTargetFailCount;
} IPX_INFO;

typedef struct
{
  WORD SPXMaxConnsCount;
  WORD SPXMaxUsedConns;
  WORD SPXEstConnReq;
  WORD SPXEstConnFail;
  WORD SPXListenConnectReq;
  WORD SPXListenConnectFail;
  DWORD SPXSendCount;
  DWORD SPXWindowChokeCount;
  WORD SPXBadSendCount;
  WORD SPXSendFailCount;
  WORD SPXAbortedConn;
  DWORD SPXListenPacketCount;
  WORD SPXBadListenCount;
  DWORD SPXIncomingPacketCount;
  WORD SPXBadInPacketCount;
  WORD SPXSuppressedPackCount;
  WORD SPXNoSesListenECBCount;
  WORD SPXWatchDogDestSesCount;
} SPX_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  IPX_INFO IPXInfo;
  SPX_INFO SPXInfo;
} NWFSE_IPXSPX_INFO;

/* Garbage Collection Information */

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD failedAllocRequestCount;
  DWORD numOfAllocs;
  DWORD noMoreMemAvailableCount;
  DWORD numOfGarbageCollections;
  DWORD garbageFoundSomeMem;
  DWORD garbageNumOfChecks;
} NWFSE_GARBAGE_COLLECTION_INFO;

/* CPU Information */

#define FSE_CPU_STR_MAX            16
#define FSE_COPROCESSOR_STR_MAX    48
#define FSE_BUS_STR_MAX            32

typedef struct
{
  DWORD pageTableOwnerFlag;
  DWORD CPUTypeFlag;
  DWORD coProcessorFlag;
  DWORD busTypeFlag;
  DWORD IOEngineFlag;
  DWORD FSEngineFlag;
  DWORD nonDedicatedFlag;
} CPU_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD numOfCPUs;
  CPU_INFO CPUInfo;
} NWFSE_CPU_INFO;

/* Volume Switch Information */

typedef struct
{
  DWORD  readFile;
  DWORD  writeFile;
  DWORD  deleteFile;
  DWORD  renMove;
  DWORD  openFile;
  DWORD  createFile;
  DWORD  createAndOpenFile;
  DWORD  closeFile;
  DWORD  scanDeleteFile;
  DWORD  salvageFile;
  DWORD  purgeFile;
  DWORD  migrateFile;
  DWORD  deMigrateFile;
  DWORD  createDir;
  DWORD  deleteDir;
  DWORD  directoryScans;
  DWORD  mapPathToDirNum;
  DWORD  modifyDirEntry;
  DWORD  getAccessRights;
  DWORD  getAccessRightsFromIDs;
  DWORD  mapDirNumToPath;
  DWORD  getEntryFromPathStrBase;
  DWORD  getOtherNSEntry;
  DWORD  getExtDirInfo;
  DWORD  getParentDirNum;
  DWORD  addTrusteeR;
  DWORD  scanTrusteeR;
  DWORD  delTrusteeR;
  DWORD  purgeTrust;
  DWORD  findNextTrustRef;
  DWORD  scanUserRestNodes;
  DWORD  addUserRest;
  DWORD  deleteUserRest;
  DWORD  rtnDirSpaceRest;
  DWORD  getActualAvailDskSp;
  DWORD  cntOwnedFilesAndDirs;
  DWORD  migFileInfo;
  DWORD  volMigInfo;
  DWORD  readMigFileData;
  DWORD  getVolusageStats;
  DWORD  getActualVolUsageStats;
  DWORD  getDirUsageStats;
  DWORD  NMFileReadsCount;
  DWORD  NMFileWritesCount;
  /* DWORD  mapPathToDirectoryNumberOrPhantom; */
  DWORD  mapPathToDirNumOrPhantom;
  /* DWORD  stationHasAccessRightsGrantedBelow; */
  DWORD  stationHasAccessRgtsGntedBelow;
  /* DWORD  getDataStreamLengthsFromPathStringBase; */
  DWORD  gtDataStreamLensFromPathStrBase;
  DWORD  checkAndGetDirectoryEntry;
  DWORD  getDeletedEntry;
  DWORD  getOriginalNameSpace;
  DWORD  getActualFileSize;
  DWORD  verifyNameSpaceNumber;
  DWORD  verifyDataStreamNumber;
  DWORD  checkVolumeNumber;
  DWORD  commitFile;
  DWORD  VMGetDirectoryEntry;
  DWORD  createDMFileEntry;
  DWORD  renameNameSpaceEntry;
  DWORD  logFile;
  DWORD  releaseFile;
  DWORD  clearFile;
  DWORD  setVolumeFlag;
  DWORD  clearVolumeFlag;
  DWORD  getOriginalInfo;
  DWORD  createMigratedDir;
  DWORD  F3OpenCreate;
  DWORD  F3InitFileSearch;
  DWORD  F3ContinueFileSearch;
  DWORD  F3RenameFile;
  DWORD  F3ScanForTrustees;
  DWORD  F3ObtainFileInfo;
  DWORD  F3ModifyInfo;
  DWORD  F3EraseFile;
  DWORD  F3SetDirHandle;
  DWORD  F3AddTrustees;
  DWORD  F3DeleteTrustees;
  DWORD  F3AllocDirHandle;
  DWORD  F3ScanSalvagedFiles;
  DWORD  F3RecoverSalvagedFiles;
  DWORD  F3PurgeSalvageableFile;
  DWORD  F3GetNSSpecificInfo;
  DWORD  F3ModifyNSSpecificInfo;
  DWORD  F3SearchSet;
  DWORD  F3GetDirBase;
  DWORD  F3QueryNameSpaceInfo;
  DWORD  F3GetNameSpaceList;
  DWORD  F3GetHugeInfo;
  DWORD  F3SetHugeInfo;
  DWORD  F3GetFullPathString;
  DWORD  F3GetEffectiveDirectoryRights;
} VOLUME_SWITCH_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD totalLFSCounters;
  DWORD CurrentLFSCounters;
  DWORD LFSCounters[ 128 ]; /* 512 / sizeof(DWORD) */
  /* VOLUME_SWITCH_INFO volumeSwitchInfo; */ /* Cant return all counters */
} NWFSE_VOLUME_SWITCH_INFO;

/* Get NLM Loaded List */

#define FSE_NLM_NUMS_RETURNED_MAX 128

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD numberNLMsLoaded;
  DWORD NLMsInList;
  DWORD NLMNums[ FSE_NLM_NUMS_RETURNED_MAX ];
} NWFSE_NLM_LOADED_LIST;

/* NLM Information */

/* 1 is added for the NULL */

#define FSE_NLM_FILENAME_LEN_MAX   37
#define FSE_NLM_NAMELEN_MAX       129
#define FSE_NLM_COPYRIGHTLEN_MAX  256

typedef struct {
  DWORD identificationNum;
  DWORD flags;
  DWORD type;
  DWORD parentID;
  DWORD majorVersion;
  DWORD minorVersion;
  DWORD revision;
  DWORD year;
  DWORD month;
  DWORD day;
  DWORD allocAvailableBytes;
  DWORD allocFreeCount;
  DWORD lastGarbageCollection;
  DWORD messageLanguage;
  DWORD numOfReferencedPublics;
} NLM_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  NLM_INFO NLMInfo;
} NWFSE_NLM_INFO;

/* Get Directory Cache Information */

typedef struct {
  DWORD minTimeSinceFileDelete;
  DWORD absMinTimeSinceFileDelete;
  DWORD minNumOfDirCacheBuffers;
  DWORD maxNumOfDirCacheBuffers;
  DWORD numOfDirCacheBuffers;
  DWORD dCMinNonReferencedTime;
  DWORD dCWaitTimeBeforeNewBuffer;
  DWORD dCMaxConcurrentWrites;
  DWORD dCDirtyWaitTime;
  DWORD dCDoubleReadFlag;
  DWORD mapHashNodeCount;
  DWORD spaceRestrictionNodeCount;
  DWORD trusteeListNodeCount;
  DWORD percentOfVolumeUsedByDirs;
} DIR_CACHE_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DIR_CACHE_INFO dirCacheInfo;
} NWFSE_DIR_CACHE_INFO;

/* Get Operating System Version Information */

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  BYTE OSMajorVersion;
  BYTE OSMinorVersion;
  BYTE OSRevisionNum;
  BYTE accountingVersion;
  BYTE VAPVersion;
  BYTE queueingVersion;
  BYTE securityRestrictionsLevel;
  BYTE bridgingSupport;
  DWORD maxNumOfVolumes;
  DWORD numOfConnSlots;
  DWORD maxLoggedInConns;
  DWORD maxNumOfNameSpaces;
  DWORD maxNumOfLans;
  DWORD maxNumOfMediaTypes;
  DWORD maxNumOfProtocols;
  DWORD maxMaxSubdirTreeDepth;
  DWORD maxNumOfDataStreams;
  DWORD maxNumOfSpoolPrinters;
  DWORD serialNum;
  WORD applicationNum;
} NWFSE_OS_VERSION_INFO;

/* Get Active Connection List by Type */

/* Connection service type */
/* NOTE: type 1 is reserved by CLIB for backward compatability */

#define FSE_NCP_CONNECTION_TYPE        2
#define FSE_NLM_CONNECTION_TYPE        3
#define FSE_AFP_CONNECTION_TYPE        4
#define FSE_FTAM_CONNECTION_TYPE       5
#define FSE_ANCP_CONNECTION_TYPE       6

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  BYTE activeConnBitList[ 512 ];
} NWFSE_ACTIVE_CONN_LIST;

/* Get NLM's Resource Tag List */

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD totalNumOfResourceTags;
   DWORD packetResourceTags;
   BYTE resourceTagBuf[ 512 ];
   /* (LONG,LONG,BYTE[])[] - BYTE[] is a null-terminated string.
   ** This packed structure consisting of:
   **
   ** DWORD number,
   ** DWORD signature,
   ** DWORD count,
   ** BYTE name[] */
} NWFSE_NLMS_RESOURCE_TAG_LIST;

/* Active LAN Board List --- 20 */

#define FSE_MAX_NUM_OF_LANS 64

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD MaxNumOfLANs;
  DWORD LANLoadedCount;
  DWORD boardNums[ FSE_MAX_NUM_OF_LANS ];
} NWFSE_ACTIVE_LAN_BOARD_LIST;

/* LAN Configuration Information */

typedef struct
{
   BYTE  DriverCFG_MajorVersion;
   BYTE  DriverCFG_MinorVersion;
   BYTE  DriverNodeAddress[ 6 ];
   WORD  DriverModeFlags;
   WORD  DriverBoardNum;
   WORD  DriverBoardInstance;
   DWORD  DriverMaxSize;
   DWORD  DriverMaxRecvSize;
   DWORD  DriverRecvSize;
   DWORD  Reserved1[3];
   WORD  DriverCardID;
   WORD  DriverMediaID;
   WORD  DriverTransportTime;
   BYTE  DriverReserved[ 16 ];
   BYTE  DriverMajorVersion;
   BYTE  DriverMinorVersion;
   WORD  DriverFlags;
   WORD  DriverSendRetries;
   DWORD  DriverLink;
   WORD  DriverSharingFlags;
   WORD  DriverSlot;
   WORD  DriverIOPortsAndLengths[ 4 ];
   DWORD  DriverMemDecode0;
   WORD  DriverLength0;
   DWORD  DriverMemDecode1;
   WORD  DriverLength1;
   BYTE  DriverInterrupt[ 2 ];
   BYTE  DriverDMAUsage[ 2 ];
   DWORD Reserved2[3];
   BYTE  DriverLogicalName[ 18 ];
   DWORD  DriverLinearMem[ 2 ];
   WORD  DriverChannelNum;
   BYTE  DriverIOReserved[ 6 ];
} LAN_CONFIG_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  LAN_CONFIG_INFO LANConfigInfo;
} NWFSE_LAN_CONFIG_INFO;

/* LAN Common Counters Information */

typedef struct
{
  DWORD notSupportedMask;
  DWORD totalTxPAcketCount;
  DWORD totalRxPacketCount;
  DWORD noECBAvailableCount;
  DWORD packetTxTooBigCount;
  DWORD packetTxTooSmallCount;
  DWORD packetRxOverflowCount;
  DWORD packetRxTooBigCount;
  DWORD packetRxTooSmallCount;
  DWORD packetTxMiscErrorCount;
  DWORD packetRxMiscErrorCount;
  DWORD retryTxCount;
  DWORD checksumErrorCount;
  DWORD hardwareRxMismatchCount;
  DWORD reserved[50];
} LAN_COMMON_INFO;


typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  BYTE statisticsMajorVersion;
  BYTE statisticsMinorVersion;
  DWORD numberOfGenericCounters;
  DWORD numberOfCounterBlocks;
  DWORD customVariableCount;
  DWORD NextCounterBlock;
   LAN_COMMON_INFO LANCommonInfo;
} NWFSE_LAN_COMMON_COUNTERS_INFO;

/* LAN Custom Counters Information */

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD numCustomVar;
  BYTE customInfo[ 512 ];   /* (LONG,BYTE[])[] - BYTE[] is a length preceded
                             ** non-null terminated string. */
} NWFSE_LAN_CUSTOM_INFO;

/* LSL Information */

typedef struct
{
  DWORD rxBufs;
  DWORD rxBufs75PerCent;
  DWORD rxBufsCheckedOut;
  DWORD rxBufMaxSize;
  DWORD maxPhysicalSize;
  DWORD lastTimeRxBufAllocated;
  DWORD maxNumsOfProtocols;
  DWORD maxNumsOfMediaTypes;
  DWORD totalTXPackets;
  DWORD getECBBfrs;
  DWORD getECBFails;
  DWORD AESEventCounts;
  DWORD postponedEvents;
  DWORD ECBCxlFails;
  DWORD validBfrsReused;
  DWORD enqueuedSendCount;
  DWORD totalRXPackets;
  DWORD unclaimedPackets;
  BYTE StatisticsTableMajorVersion;
  BYTE StatisticsTableMinorVersion;
} LSL_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  LSL_INFO LSLInfo;
} NWFSE_LSL_INFO;

/* LSL Logical Board Statistics */

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved0;
  DWORD LogTtlTxPackets;
  DWORD LogTtlRxPackets;
  DWORD LogUnclaimedPackets;
  DWORD reserved1;
} NWFSE_LSL_LOGICAL_BOARD_STATS;

/* Get Media Manager Object Information */

/* objtype */

#define FSE_ADAPTER_OBJECT         0
#define FSE_CHANGER_OBJECT         1
#define FSE_DEVICE_OBJECT          2
#define FSE_MEDIA_OBJECT           4
#define FSE_PARTITION_OBJECT       5
#define FSE_SLOT_OBJECT            6
#define  FSE_HOTFIX_OBJECT         7
#define  FSE_MIRROR_OBJECT         8
#define  FSE_PARITY_OBJECT         9
#define  FSE_VOLUME_SEG_OBJECT     10
#define  FSE_VOLUME_OBJECT         11
#define  FSE_CLONE_OBJECT          12
#define  FSE_MAGAZINE_OBJECT       14
#define  FSE_VIRTUAL_DEVICE_OBJECT 15

#define FSE_UNKNOWN_OBJECT         0xFFFF

/* mediatype */

#define  FSE_HARD_DISK      0
#define FSE_CDROM_DISK      1
#define FSE_WORM_DISK       2
#define FSE_TAPE_DEVICE     3
#define FSE_MAGNETO_OPTICAL 4

/* cartridgetype */

#define  FSE_FIXED_MEDIA    0x00000000
#define  FSE_FLOPPY_5_25    0x00000001
#define  FSE_FLOPPY_3_5     0x00000002
#define  FSE_OPTICAL_5_25   0x00000003
#define  FSE_OPTICAL_3_5    0x00000004
#define  FSE_TAPE_0_5       0x00000005
#define  FSE_TAPE_0_25      0x00000006
#define  FSE_TAPE_8_MM      0x00000007
#define  FSE_TAPE_4_MM      0x00000008
#define  FSE_BERNOULLI_DISK 0x00000009

/* type */
/* same as defined below for object types */

/* status bits */

#define  FSE_OBJECT_ACTIVATED          0x00000001
#define  FSE_OBJECT_CREATED            0x00000002
#define  FSE_OBJECT_SCRAMBLED          0x00000004
#define  FSE_OBJECT_RESERVED           0x00000010
#define  FSE_OBJECT_BEING_IDENTIFIED   0x00000020
#define  FSE_OBJECT_MAGAZINE_LOADED    0x00000040
#define  FSE_OBJECT_FAILURE            0x00000080
#define  FSE_OBJECT_REMOVABLE          0x00000100
#define  FSE_OBJECT_READ_ONLY          0x00000200
#define  FSE_OBJECT_IN_DEVICE          0x00010000
#define  FSE_OBJECT_ACCEPTS_MAGAZINES  0x00020000
#define  FSE_OBJECT_IS_IN_A_CHANGER    0x00040000
#define  FSE_OBJECT_LOADABLE           0x00080000
#define  FSE_OBJECT_BEING_LOADED       0x00080000
#define  FSE_OBJECT_DEVICE_LOCK        0x01000000
#define  FSE_OBJECT_CHANGER_LOCK       0x02000000
#define  FSE_OBJECT_REMIRRORING        0x04000000
#define  FSE_OBJECT_SELECTED           0x08000000

/* functionmask */

#define  FSE_RANDOM_READ               0x0001
#define  FSE_RANDOM_WRITE              0x0002
#define  FSE_RANDOM_WRITE_ONCE         0x0004
#define  FSE_SEQUENTIAL_READ           0x0008
#define  FSE_SEQUENTIAL_WRITE          0x0010
#define  FSE_RESET_END_OF_TAPE         0x0020
#define  FSE_SINGLE_FILE_MARK          0x0040
#define  FSE_MULTIPLE_FILE_MARK        0x0080
#define  FSE_SINGLE_SET_MARK           0x0100
#define  FSE_MULTIPLE_SET_MARK         0x0200
#define  FSE_SPACE_DATA_BLOCKS         0x0400
#define  FSE_LOCATE_DATA_BLOCKS        0x0800
#define  FSE_POSITION_PARTITION        0x1000
#define  FSE_POSITION_MEDIA            0x2000

/* controlmask */

#define  FSE_ACTIVATE_DEACTIVE  0x0001
#define  FSE_MOUNT_DISMOUNT     0x0002
#define  FSE_SELECT_UNSELECT    0x0004
#define  FSE_LOCK_UNLOCK        0x0008
#define  FSE_EJECT              0x0010
#define  FSE_MOVE               0x0020

typedef struct
{
  BYTE label[ 64 ];
  DWORD identificationtype;
  DWORD identificationtimestamp;
} MEDIA_INFO_DEF;

typedef struct
{
  MEDIA_INFO_DEF MediaInfo;
  DWORD mediaType;
  DWORD cartridgeType;
  DWORD unitSize;
  DWORD blockSize;
  DWORD capacity;
  DWORD preferredUnitSize;
  BYTE name[ 64 ];
  DWORD type;
  DWORD status;
  DWORD functionMask;
  DWORD controlMask;
  DWORD parentCount;
  DWORD siblingCount;
  DWORD childCount;
  DWORD specificInfoSize;
  DWORD objectUniqueID;
  DWORD mediaSlot;
} FSE_MM_OBJ_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  FSE_MM_OBJ_INFO fseMMObjInfo;
} NWFSE_MEDIA_MGR_OBJ_INFO;

/* Get Media Manager Objects List
   Get Media Manager Object Children's List  */

#define FSE_MAX_OBJECTS 128

/* object types */

#define  FSE_ADAPTER_OBJECT        0
#define  FSE_CHANGER_OBJECT        1
#define  FSE_DEVICE_OBJECT         2
#define  FSE_MEDIA_OBJECT          4
#define  FSE_PARTITION_OBJECT      5
#define  FSE_SLOT_OBJECT           6
#define  FSE_HOTFIX_OBJECT         7
#define  FSE_MIRROR_OBJECT         8
#define  FSE_PARITY_OBJECT         9
#define  FSE_VOLUME_SEG_OBJECT     10
#define  FSE_VOLUME_OBJECT         11
#define  FSE_CLONE_OBJECT          12
#define  FSE_MAGAZINE_OBJECT       14
#define  FSE_VIRTUAL_DEVICE_OBJECT 15
#define  FSE_UNKNOWN_OBJECT_TYPE   0xFFFF

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD nextStartObjNum;
  DWORD objCount;
  DWORD objs[ FSE_MAX_OBJECTS ];
} NWFSE_MEDIA_MGR_OBJ_LIST;

/* Get Volume Segment List */

#define FSE_MAX_NUM_SEGS_RETURNED 43

typedef struct
{
  DWORD volumeSegmentDeviceNum;
  DWORD volumeSegmentOffset;
  DWORD volumeSegmentSize;
} VOLUME_SEGMENT;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD numOfVolumeSegments; /* segment info follows */
  VOLUME_SEGMENT volumeSegment[ 42 ]; /* VOLUME_SEGMENT structures are packed */
} NWFSE_VOLUME_SEGMENT_LIST;

/* Volume Information by Level */

typedef struct {
  DWORD volumeType;
  DWORD statusFlagBits;
  DWORD sectorSize;
  DWORD sectorsPerCluster;
  DWORD volumeSizeInClusters;
  DWORD freedClusters;
  DWORD subAllocFreeableClusters;
  DWORD freeableLimboSectors;
  DWORD nonFreeableLimboSectors;
  DWORD nonFreeableAvailSubAllocSectors;
  DWORD notUsableSubAllocSectors;
  DWORD subAllocClusters;
  DWORD dataStreamsCount;
  DWORD limboDataStreamsCount;
  DWORD oldestDeletedFileAgeInTicks;
  DWORD compressedDataStreamsCount;
  DWORD compressedLimboDataStreamsCount;
  DWORD unCompressableDataStreamsCount;
  DWORD preCompressedSectors;
  DWORD compressedSectors;
  DWORD migratedFiles;
  DWORD migratedSectors;
  DWORD clustersUsedByFAT;
  DWORD clustersUsedByDirectories;
  DWORD clustersUsedByExtendedDirs;
  DWORD totalDirectoryEntries;
  DWORD unUsedDirectoryEntries;
  DWORD totalExtendedDirectoryExtants;
  DWORD unUsedExtendedDirectoryExtants;
  DWORD extendedAttributesDefined;
  DWORD extendedAttributeExtantsUsed;
  DWORD directoryServicesObjectID;
  DWORD volumeLastModifiedDateAndTime;
} VOLUME_INFO_BY_LEVEL_DEF;

typedef struct
{
  DWORD volumeActiveCount;
  DWORD volumeUseCount;
  DWORD mACRootIDs;
  DWORD volumeLastModifiedDateAndTime;
  DWORD volumeReferenceCount;
  DWORD compressionLowerLimit;
  DWORD outstandingIOs;
  DWORD outstandingCompressionIOs;
  DWORD compressionIOsLimit;
} VOLUME_INFO_BY_LEVEL_DEF2;

typedef union
{
   VOLUME_INFO_BY_LEVEL_DEF   volInfoDef;
   VOLUME_INFO_BY_LEVEL_DEF2  volInfoDef2;
} VOLUME_INFO_BY_LEVEL;

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverAndVConsoleInfo;
   WORD  reserved;
   DWORD infoLevel;
   VOLUME_INFO_BY_LEVEL volumeInfo;
} NWFSE_VOLUME_INFO_BY_LEVEL;


/* Active Protocol Stacks */

#define FSE_MAX_NUM_OF_STACKINFO 25

typedef struct
{
  DWORD StackNum;
  BYTE StackShortName[ 16 ];
} STACK_INFO;

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD maxNumOfStacks;
  DWORD stackCount;
  DWORD nextStartNum;
  STACK_INFO stackInfo[ FSE_MAX_NUM_OF_STACKINFO ];
} NWFSE_ACTIVE_STACKS;

/* Get Protocol Stack Configuration Information */

#define FSE_STK_FULL_NAME_STR_LEN_MAX 256

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  BYTE configMajorVersionNum;
  BYTE configMinorVersionNum;
  BYTE stackMajorVersionNum;
  BYTE stackMinorVersionNum;
  BYTE stackShortName[ 16 ];
} NWFSE_PROTOCOL_STK_CONFIG_INFO;

/* Get Protocol Stack Statistics Information  */

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  BYTE statMajorVersionNum;
  BYTE statMinorVersionNum;
  WORD commonCounters;           /* always set to 3? */
  DWORD validCountersMask;
  DWORD totalTxPackets;
  DWORD totalRxPackets;
  DWORD ignoredRxPackets;
  WORD numCustomCounters;
} NWFSE_PROTOCOL_STK_STATS_INFO;

/* Get Protocol Stack Custom Information */

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved0;
  DWORD customCount;
  BYTE customStruct[ 512 ]; /* (LONG,BYTE[])[] - BYTE[] is a length preceded
                             ** non-null terminated string. */
} NWFSE_PROTOCOL_CUSTOM_INFO;

/* Get Protocol Stack Numbers by Media     Number
   Get Protocol Stack Numbers by Lan Board Number */

/* Currently 16 */
#define FSE_STACK_IDS_MAX 128

/* mediaNum */

#define FSE_NO_FRAME_ID_MAC            0
#define FSE_APPLE_LOCALTALK            1
#define FSE_ETHERNETII_DEC             2
#define FSE_ETHERNET_802_3_USING_802_2 3
#define FSE_TRING_802_5_USING_802_2    4
#define FSE_IPX_802_3                  5
#define FSE_TOKEN_PASSING_BUS          6
#define FSE_IBM_PC_NETWORK_II          7
#define FSE_GATEWAY_GNET               8
#define FSE_PROTEON_PRONET             9
#define FSE_ENET_802_3_USING_802_2_SNAP 10
#define FSE_TRING_802_5_USE_802_2_SNAP 11
#define FSE_RACORE_FRAME               12
#define FSE_ISDN_FRAME                 13
#define FSE_NOVELL_ARCNET              14
#define FSE_IBM_PCN2_USING_802_2       15
#define FSE_IBM_PCN2_USING_802_2_SNAP  16
#define FSE_CORVUS_FRAME               17
#define FSE_HARRIS_ADACOM_FRAME        18
#define FSE_IP_TUNNEL_FRAME            19
#define FSE_FDDI_USING_802_2           20
#define FSE_COMMTEX_FRAME              21
#define FSE_DATACO_FRAME               22
#define FSE_FDDI_USING_802_2_SMAP      23
#define FSE_SDLC_TUNNEL                24
#define FSE_PC_OFFICE_FRAME            25
#define FSE_HYPERCOMMUNICATIONS        26
#define FSE_NOVELL_FRAME               27

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
  DWORD stackIDCount;
  DWORD stackIDs[ FSE_STACK_IDS_MAX ];
} NWFSE_PROTOCOL_ID_NUMS;

/* Get Media Name by Media Number */

#define FSE_MEDIA_NAME_LEN_MAX 81

typedef struct
{
  SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
  WORD reserved;
} NWFSE_MEDIA_NAME_LIST;

/* Get Loaded Media Number List */

#define FSE_MEDIA_LIST_MAX 32

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD maxMediaTypes;
   DWORD mediaListCount;
   DWORD mediaList[ FSE_MEDIA_LIST_MAX ];
} NWFSE_LOADED_MEDIA_NUM_LIST;

/* Get General Router And SAP Information */

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD internalRIPSocket;
   DWORD internalRouterDownFlag;
   DWORD trackOnFlag;
   DWORD externalRouterActiveFlag;
   DWORD internalSAPSocketNumber;
   DWORD replyToNearestServerFlag;
} NWFSE_GENERAL_ROUTER_SAP_INFO;

/* Get Network Router Information */

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD NetIDNumber;
   WORD HopsToNet;
   WORD NetStatus;
   WORD TimeToNet;
} NWFSE_NETWORK_ROUTER_INFO;

/* Get Network Routers Information */

typedef struct
{
   BYTE nodeAddress[ 6 ];
   DWORD connectedLAN;
   WORD routeHops;
   WORD routeTime;
} ROUTERS_INFO;

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD NumberOfEntries;
   ROUTERS_INFO routersInfo[ 36 ]; /* 512 / sizeof( ROUTERS_INFO ) */
} NWFSE_NETWORK_ROUTERS_INFO;

/* Get Known Networks Information */

/* netStatus */

#define FSE_LOCALBIT       0x01
#define FSE_NETSTARBIT     0x02
#define FSE_NETRELIABLEBIT 0x04
#define FSE_NETWANBIT      0x10

typedef struct
{
   DWORD netIDNumber;
   WORD  hopsToNet;
   WORD  netStatus;
   WORD  timeToNet;
} KNOWN_NET_INFO;

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD  reserved;
   DWORD numberOfEntries;
   KNOWN_NET_INFO knownNetInfo[ 51 ];  /* 512 / sizeof( KNOWN_NET_INFO ) */
} NWFSE_KNOWN_NETWORKS_INFO;

/* Get Server Information */

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   BYTE serverAddress[ 12 ];
   WORD hopsToServer;
} NWFSE_SERVER_INFO;

/* Get Server Sources Information */

typedef struct
{
   BYTE serverNode[ 6 ];
   DWORD connectedLAN;
   WORD sourceHops;
} SERVERS_SRC_INFO;

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD numberOfEntries;
   SERVERS_SRC_INFO serversSrcInfo[ 42 ]; /* 512 / sizeof( SERVERS_SRC_INFO ) */
} NWFSE_SERVER_SRC_INFO;

/* Get Known Servers Information */

/*
** NWFSE_KNOWN_SERVER_INFO.data is packed with as many variable length
**  structures of the following type as possible:
**
**  {
**    BYTE serverAddress[ 12 ];
**    WORD hopsToServer;
**    BYTE serverNameLen;
**    BYTE serverName[ serverNameLen ];
**  }
**
**  servername is not NULL terminated.
*/

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD numberOfEntries;
   BYTE data[ 512 ];
} NWFSE_KNOWN_SERVER_INFO;

/* Get Server Set Commands Information */

/* setCmdType */

#define  FSE_TYPE_NUMBER            0
#define FSE_TYPE_BOOLEAN            1
#define  FSE_TYPE_TICKS             2
#define FSE_TYPE_BLOCK_SHIFT        3  /* 512 * number */
#define  FSE_TYPE_TIME_OFFSET       4  /* [+|-]hh:mm:ss converted to seconds */
#define  FSE_TYPE_STRING            5
#define  FSE_TYPE_TRIGGER           6  /* The following show the types of triggers */
#define  FSE_TYPE_TRIGGER_OFF       0x00
#define FSE_TYPE_TRIGGER_ON         0x01
#define  FSE_TYPE_TRIGGER_PENDING   0x10
#define FSE_TYPE_TRIGGER_SUCCESS    0x20
#define FSE_TYPE_TRIGGER_FAILED     0x30

/* setCmdFlags */

#define FSE_STARTUP_ONLY         0x01
#define FSE_HIDE                 0x02
#define FSE_ADVANCED             0x04
#define FSE_STARTUP_OR_LATER     0x08
#define FSE_NOT_SECURED_CONSOLE  0x10  /* Can't be performed on secured console*/

/* setCmdCategory    */

#define FSE_COMMUNICATIONS       0
#define FSE_MEMORY               1
#define FSE_FILE_CACHE           2
#define FSE_DIR_CACHE            3
#define FSE_FILE_SYSTEM          4
#define FSE_LOCKS                5
#define FSE_TRANSACTION_TRACKING 6
#define FSE_DISK                 7
#define FSE_TIME                 8
#define FSE_NCP                  9
#define FSE_MISCELLANEOUS        10
#define FSE_ERRORS               11

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD numberOfSetCommands;
   DWORD nextSequenceNumber;
   DWORD setCmdType;
   DWORD setCmdCategory;
   DWORD setCmdFlags;
   /*  The setNameAndValueInfo contains ASCIIZ strings in the following layout:
   **    BYTE setCmdName[ ];
   **    BYTE setCmdValue[ ]; */
   BYTE setNameAndValueInfo[ 500 ];
} NWFSE_SERVER_SET_CMDS_INFO;

typedef struct
{
   SERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
   WORD reserved;
   DWORD numberOfSetCategories;
   DWORD nextSequenceNumber;
   BYTE categoryName[ 512 ];  /* Len preceded string which is not NULL terminated */
} NWFSE_SERVER_SET_CATEGORIES;

NWCCODE NWAPI NWGetCacheInfo(
  NWCONN_HANDLE conn,
  NWFSE_CACHE_INFO NWPTR fseCacheInfo);

NWCCODE NWAPI NWGetFileServerInfo(
  NWCONN_HANDLE conn,
  NWFSE_FILE_SERVER_INFO NWPTR fseFileServerInfo);

NWCCODE NWAPI NWGetNetWareFileSystemsInfo(
  NWCONN_HANDLE conn,
  NWFSE_FILE_SYSTEM_INFO NWPTR fseFileSystemInfo);

NWCCODE NWAPI NWGetUserInfo(
  NWCONN_HANDLE conn,
  DWORD connNum,
  char NWPTR userName,
  NWFSE_USER_INFO NWPTR fseUserInfo);

NWCCODE NWAPI NWGetPacketBurstInfo(
  NWCONN_HANDLE conn,
  NWFSE_PACKET_BURST_INFO NWPTR fsePacketBurstInfo);

NWCCODE NWAPI NWGetIPXSPXInfo(
  NWCONN_HANDLE conn,
  NWFSE_IPXSPX_INFO NWPTR fseIPXSPXInfo);

NWCCODE NWAPI NWGetGarbageCollectionInfo(
  NWCONN_HANDLE conn,
  NWFSE_GARBAGE_COLLECTION_INFO NWPTR fseGarbageCollectionInfo);

NWCCODE NWAPI NWGetCPUInfo(
  NWCONN_HANDLE conn,
  DWORD CPUNum,
  char NWPTR CPUString,
  char NWPTR numericCoprocessorString,
  char NWPTR busString,
  NWFSE_CPU_INFO NWPTR fseCPUInfo);

NWCCODE NWAPI NWGetVolumeSwitchInfo(
  NWCONN_HANDLE conn,
  DWORD startNum,
  NWFSE_VOLUME_SWITCH_INFO NWPTR fseVolumeSwitchInfo);

NWCCODE NWAPI NWGetNLMLoadedList(
  NWCONN_HANDLE conn,
  DWORD startNum,
  NWFSE_NLM_LOADED_LIST NWPTR fseNLMLoadedList);

NWCCODE NWAPI NWGetNLMInfo(
  NWCONN_HANDLE conn,
  DWORD NLMNum,
  char NWPTR fileName,
  char NWPTR name,
  char NWPTR copyright,
  NWFSE_NLM_INFO NWPTR fseNLMInfo);

NWCCODE NWAPI NWGetDirCacheInfo(
  NWCONN_HANDLE conn,
  NWFSE_DIR_CACHE_INFO NWPTR fseDirCacheInfo);

NWCCODE NWAPI NWGetOSVersionInfo(
  NWCONN_HANDLE conn,
  NWFSE_OS_VERSION_INFO NWPTR fseOSVersionInfo);

NWCCODE NWAPI NWGetActiveConnListByType(
  NWCONN_HANDLE conn,
  DWORD startConnNum,
  DWORD connType,
  NWFSE_ACTIVE_CONN_LIST NWPTR fseActiveConnListByType);

NWCCODE NWAPI NWGetNLMsResourceTagList(
  NWCONN_HANDLE conn,
  DWORD NLMNum,
  DWORD startNum,
  NWFSE_NLMS_RESOURCE_TAG_LIST NWPTR fseNLMsResourceTagList);

NWCCODE NWAPI NWGetActiveLANBoardList(
  NWCONN_HANDLE conn,
  DWORD startNum,
  NWFSE_ACTIVE_LAN_BOARD_LIST NWPTR fseActiveLANBoardList);

NWCCODE NWAPI NWGetLANConfigInfo(
  NWCONN_HANDLE conn,
  DWORD boardNum,
  NWFSE_LAN_CONFIG_INFO NWPTR fseLANConfigInfo);

NWCCODE NWAPI NWGetLANCommonCountersInfo(
  NWCONN_HANDLE conn,
  DWORD boardNum,
  DWORD blockNum,
  NWFSE_LAN_COMMON_COUNTERS_INFO NWPTR fseLANCommonCountersInfo);

NWCCODE NWAPI NWGetLANCustomCountersInfo(
  NWCONN_HANDLE conn,
  DWORD boardNum,
  DWORD startingNum,
  NWFSE_LAN_CUSTOM_INFO NWPTR fseLANCustomInfo);

NWCCODE NWAPI NWGetLSLInfo(
  NWCONN_HANDLE conn,
  NWFSE_LSL_INFO NWPTR fseLSLInfo);

NWCCODE NWAPI NWGetLSLLogicalBoardStats(
  NWCONN_HANDLE conn,
  DWORD LANBoardNum,
  NWFSE_LSL_LOGICAL_BOARD_STATS NWPTR fseLSLLogicalBoardStats);

NWCCODE NWAPI NWGetMediaMgrObjInfo(
  NWCONN_HANDLE conn,
  DWORD objNum,
  NWFSE_MEDIA_MGR_OBJ_INFO NWPTR fseMediaMgrObjInfo);

NWCCODE NWAPI NWGetMediaMgrObjList(
  NWCONN_HANDLE conn,
  DWORD startNum,
  DWORD objType,
  NWFSE_MEDIA_MGR_OBJ_LIST NWPTR fseMediaMgrObjList);

NWCCODE NWAPI NWGetMediaMgrObjChildrenList(
  NWCONN_HANDLE conn,
  DWORD startNum,
  DWORD objType,
  DWORD parentObjNum,
  NWFSE_MEDIA_MGR_OBJ_LIST NWPTR fseMediaMgrObjList);

NWCCODE NWAPI NWGetVolumeSegmentList(
  NWCONN_HANDLE conn,
  DWORD volumeNum,
  NWFSE_VOLUME_SEGMENT_LIST NWPTR fseVolumeSegmentList);

NWCCODE NWAPI NWGetVolumeInfoByLevel(
  NWCONN_HANDLE conn,
  DWORD volumeNum,
  DWORD infoLevel,
  NWFSE_VOLUME_INFO_BY_LEVEL NWPTR fseVolumeInfo);

NWCCODE NWAPI NWGetActiveProtocolStacks(
  NWCONN_HANDLE conn,
  DWORD startNum,
  NWFSE_ACTIVE_STACKS NWPTR fseActiveStacks);

NWCCODE NWAPI NWGetProtocolStackConfigInfo(
  NWCONN_HANDLE conn,
  DWORD stackNum,
  char NWPTR stackFullNameString,
  NWFSE_PROTOCOL_STK_CONFIG_INFO NWPTR fseProtocolStkConfigInfo);

NWCCODE NWAPI NWGetProtocolStackStatsInfo(
  NWCONN_HANDLE conn,
  DWORD stackNum,
  NWFSE_PROTOCOL_STK_STATS_INFO NWPTR fseProtocolStkStatsInfo);

NWCCODE NWAPI NWGetProtocolStackCustomInfo(
  NWCONN_HANDLE conn,
  DWORD stackNum,
  DWORD customStartNum,
  NWFSE_PROTOCOL_CUSTOM_INFO NWPTR fseProtocolStackCustomInfo);

NWCCODE NWAPI NWGetProtocolStkNumsByMediaNum(
  NWCONN_HANDLE conn,
  DWORD mediaNum,
  NWFSE_PROTOCOL_ID_NUMS NWPTR fseProtocolStkIDNums);

NWCCODE NWAPI NWGetProtocolStkNumsByLANBrdNum(
  NWCONN_HANDLE conn,
  DWORD LANBoardNum,
  NWFSE_PROTOCOL_ID_NUMS NWPTR fseProtocolStkIDNums);

NWCCODE NWAPI NWGetMediaNamebyMediaNum(
  NWCONN_HANDLE conn,
  DWORD mediaNum,
  char NWPTR mediaName,
  NWFSE_MEDIA_NAME_LIST NWPTR fseMediaNameList);

NWCCODE NWAPI NWGetLoadedMediaNumList(
  NWCONN_HANDLE conn,
  NWFSE_LOADED_MEDIA_NUM_LIST NWPTR fseLoadedMediaNumList);

NWCCODE NWAPI NWGetGeneralRouterAndSAPInfo(
  NWCONN_HANDLE conn,
  NWFSE_GENERAL_ROUTER_SAP_INFO NWPTR  fseGeneralRouterSAPInfo );

NWCCODE NWAPI NWGetNetworkRouterInfo(
  NWCONN_HANDLE conn,
  DWORD networkNum,
  NWFSE_NETWORK_ROUTER_INFO NWPTR  fseNetworkRouterInfo );

NWCCODE NWAPI NWGetNetworkRoutersInfo(
  NWCONN_HANDLE conn,
  DWORD networkNum,
  DWORD startNum,
  NWFSE_NETWORK_ROUTERS_INFO NWPTR  fseNetworkRoutersInfo );

NWCCODE NWAPI NWGetKnownNetworksInfo(
  NWCONN_HANDLE conn,
  DWORD numberOfEntries,
  NWFSE_KNOWN_NETWORKS_INFO NWPTR  fseKnownNetworksInfo );

NWCCODE NWAPI NWGetServerInfo(
  NWCONN_HANDLE conn,
  DWORD serverType,
  char NWPTR serverName,
  NWFSE_SERVER_INFO NWPTR  fseServerInfo );

NWCCODE NWAPI NWGetServerSourcesInfo(
  NWCONN_HANDLE conn,
  DWORD startNum,
  DWORD serverType,
  char NWPTR serverName,
  NWFSE_SERVER_SRC_INFO NWPTR  fseServerSrcInfo );

NWCCODE NWAPI NWGetKnownServersInfo(
  NWCONN_HANDLE conn,
  DWORD startNum,
  DWORD serverType,
  NWFSE_KNOWN_SERVER_INFO NWPTR  fseKnownServerInfo );

NWCCODE NWAPI NWGetServerSetCommandsInfo(
  NWCONN_HANDLE conn,
  DWORD startNum,
  NWFSE_SERVER_SET_CMDS_INFO NWPTR  fseServerSetCmdsInfo );

NWCCODE NWAPI NWGetServerSetCategories(
  NWCONN_HANDLE conn,
  DWORD startNum,
  NWFSE_SERVER_SET_CATEGORIES NWPTR  fseServerSetCategories );

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
$Header:   K:\archives\src\sdkinc\nwfse.h_v   1.2   10 Aug 1993 12:50:50   JWOODBUR  $
*/
