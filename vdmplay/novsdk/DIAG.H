/*      (c) COPYRIGHT 1988-1991 by Novell, Inc.  All Rights Reserved.    */

#ifndef _DIAG_H
#define _DIAG_H

#include <nwcaldef.h>

#ifndef _NXT_H
    #include <nxtd.h>
#endif /* _NXT_H */

/* ------------------- Defines --------------------*/
/* Limits */
#define MAX_NETWORKS    120
#define MAX_SERVERS     150
#define MAX_NODES       120
#define MAX_EXCLUSIONS  80

#define MAX_LOCAL_NETWORKS  4
#define MAX_ROUTES          47

#define MAX_IPX_PACKET_SIZE  576

#define MAX_SERVER_NAME_LENGTH   16
#define WORKGROUP_NAME_LENGTH    16
#define OEM_NAME_LENGTH          16

/* Component IDs */
#define IPX_SPX_COMPONENT               0
#define BRIDGE_DRIVER_COMPONENT         1
#define SHELL_DRIVER_COMPONENT          2
#define SHELL_COMPONENT                 3
#define VAP_SHELL_COMPONENT             4
#define BRIDGE_COMPONENT                5
#define FILE_SERVER_COMPONENT           6
#define NONDEDICATED_IPX_SPX_COMPONENT  7
#define IPX_ONLY                        8
#define GNMA_DOS_COMPONENT              9
#define GNMA_OS2_COMPONENT             10
#define NWLITE_SERVER_COMPONENT        11
#define NWLITE_CACHE_COMPONENT         12

#define MAX_COMPONENTS                 13


/* Error Codes */
#define NO_ERRORS                           0x00
#define COULD_NOT_GET_LOCAL_TARGET          0xFF
#define COULD_NOT_OPEN_SOCKET               0xFE
#define COULD_NOT_BEGIN_CONNECTION          0xFD
#define COULD_NOT_ESTABLISH_CONNECTION      0xFC
#define COULD_NOT_TERMINATE_CONNECTION      0xFB
#define BAD_CONNECTION_ID                   0xFA
#define COULD_NOT_SEND_REQUEST              0xF9
#define RECEIVED_REPLY_IN_ERROR             0xF8
#define NETWORK_NOT_FOUND                   0xEF
#define NO_RESPONSE_FROM_DESTINATION        0xEE
#define RESPONSE_COMPLETION_CODE_BAD        0xED
#define LIST_SIZE_TOO_SMALL                 0xEC
#define INTERNAL_LIST_ERROR                 0xEB
#define NODE_NOT_FOUND_OR_NO_RESPONSE       0xEA
#define MEMORY_ALLOCATION_ERROR             0xDF
#define VERSION_DOES_NOT_SUPPORT            0xDE


/* ------------------- Structures --------------------*/
typedef struct IPXAddress BeginDiagnosticStruct;

typedef struct StructIPXPacket
{
    BYTE data[MAX_IPX_PACKET_SIZE - sizeof(IPXHeader)];
} IPXPacket;

typedef struct StructSPXPacket
{
    BYTE data[MAX_IPX_PACKET_SIZE - sizeof(SPXHeader)];
} SPXPacket;

typedef struct StructExclusionList
{
    BYTE nodeAddress[6];
} ExclusionListStruct;

typedef struct StructExclusionPacket
{
    BYTE numberOfExclusions;
    ExclusionListStruct structureArray[MAX_EXCLUSIONS];
} ExclusionPacketStructure;

typedef struct StructAddr
{
    BYTE network[4];
    BYTE node[6];
} AddrStruct;

struct StructDriver
{
    BYTE localNetworkType;
    BYTE network[4];
    BYTE node[6];
};

typedef struct StructBridge
{
    BYTE numberOfNets;
    struct StructDriver bridge[MAX_LOCAL_NETWORKS];
} BridgeStruct;

typedef struct StructNode
{
    char name[12];
    BeginDiagnosticStruct target;
    BYTE numberOfComponents;
    BYTE componentID[MAX_COMPONENTS];
    BridgeStruct *bridgePtr;
} NodeStruct;

typedef struct StructConfigurationResponse
{
    AddrStruct address;
    struct
    {
        BYTE majorVersion;
        BYTE minorVersion;
        WORD SPXDiagnosticSocket;
        BYTE numberOfComponents;
        BYTE componentStructure[MAX_IPX_PACKET_SIZE -
            sizeof(IPXHeader) - sizeof(AddrStruct) - 5];
    } packet;
} ConfigurationResponseStruct;

typedef struct StructAllResp
{
    BYTE completionCode;
    long intervalMarker;
} AllResponseData;

typedef struct StructIPXSPXVersion
{
    BYTE IPXMajorVersion;
    BYTE IPXMinorVersion;
    BYTE SPXMajorVersion;
    BYTE SPXMinorVersion;
} IPXSPXVersion;

typedef struct StructIPXStatistics
{
    long sendPacketCount;
    WORD malformedPacketCount;
    long getECBRequestCount;
    long getECBFailureCount;
    long AESEventCount;
    WORD postponedAESEventCount;
    WORD maxConfiguredSocketsCount;
    WORD maxOpenSocketsCount;
    WORD openSocketFailureCount;
    long listenECBCount;
    WORD ECBCancelFailureCount;
    WORD findRouteFailureCount;
} IPXStatisticsStruct;

typedef struct StructSPXStatistics
{
    WORD maxConnectionsCount;
    WORD maxUsedConnectionsCount;
    WORD establishConnectionRequest;
    WORD establishConnectionFailure;
    WORD listenConnectionRequestCount;
    WORD listenConnectionFailureCount;
    long sendPacketCount;
    long windowChokeCount;
    WORD badSendPacketCount;
    WORD sendFailureCount;
    WORD abortConnectionCount;
    long listenPacketCount;
    WORD badListenPacketCount;
    long incomingPacketCount;
    WORD badIncomingPacketCount;
    WORD suppressedPacketCount;
    WORD noSessionListenECBCount;
    WORD watchdogDestroySessionCount;
} SPXStatisticsStruct;

typedef struct SPReq
{
    BeginDiagnosticStruct target;
    BYTE immediateAddress[6];
    WORD numberOfPackets;
    BYTE timerTickInterval;
    BYTE packetsPerTickInterval;
    WORD packetSize;
    WORD changeSize;
} SendPacketsRequestStruct;

typedef struct SPResp
{
    WORD numberOfTransmitErrors;
} SendPacketsResponseStruct;

typedef struct StructStartCountingPackets
{
    WORD destinationSocket;
} StartCountingPacketsStruct;

typedef struct StructReturnReceivedPacket
{
    WORD packetsReceived;
} ReturnReceivedPacketStruct;

typedef BYTE status;

typedef struct StructBridgeDriverStatus
{
    status LANBoard[4];
} BridgeDriverStatusStruct;

typedef struct StructDriverConf
{
    BYTE networkAddress[4];
    BYTE nodeAddress[6];
    BYTE LANMode;
    BYTE nodeAddressType;
    WORD maxDataSize;
    WORD reserved1;
    BYTE reserved4;
    WORD transportTime;
    BYTE reserved2[11];
    BYTE majorVersion;
    BYTE minorVersion;
    BYTE ethernetFlagBits;
    BYTE selectedConfiguration;
    BYTE LANDescription[80];
    WORD IOAddress1;
    WORD IODecodeRange1;
    WORD IOAddress2;
    WORD IODecodeRange2;
    BYTE memoryAddress1[3];
    WORD memoryDecodeRange1;
    BYTE memoryAddress2[3];
    WORD memoryDecodeRange2;
    BYTE interruptIsUsed1;
    BYTE interruptLine1;
    BYTE interruptIsUsed2;
    BYTE interruptLine2;
    BYTE DMAIsUsed1;
    BYTE DMALine1;
    BYTE DMAIsUsed2;
    BYTE DMALine2;
    BYTE microChannelFlagBits;
    BYTE reserved3;
    BYTE textDescription[80];
} DriverConfigurationStruct;

typedef struct StructDriverStat
{
    BYTE driverVersion[2];
    BYTE statisticsVersion[2];
    long totalTxPacketCount;
    long totalRxPacketCount;
    WORD noECBAvailableCount;
    WORD packetTxTooBigCount;
    WORD packetTxTooSmallCount;
    WORD packetRxOverflowCount;
    WORD packetRxTooBigCount;
    WORD packetRxTooSmallCount;
    WORD packetTxMiscErrorCount;
    WORD packetRxMiscErrorCount;
    WORD retryTxCount;
    WORD checksumErrorCount;
    WORD hardwareRxMismatchCount;
    WORD numberOfCustomVariables;
    BYTE variableData[495];
    /*  BYTE variableData[1]; */
} DriverStatisticsStruct;

typedef struct StructOSVersion
{
    BYTE machineID;
    BYTE versionData[41];
} OSVersionStruct;

typedef struct IPXAddress ShellAddressStruct;

typedef struct StructShellStatistics
{
    long shellRequestsCount;
    WORD operatorAbortsCount;
    WORD operatorRetriesCount;
    WORD timeoutsCount;
    WORD writeErrorCount;
    WORD invalidReplyHeaderCount;
    WORD invalidSlotCount;
    WORD invalidSequenceNumberCount;
    WORD errorReceivingCount;
    WORD noRouterFoundCount;
    WORD beingProcessedCount;
    WORD unknownErrorCount;
    WORD invalidServerSlotCount;
    WORD networkGoneCount;
    WORD reserved1;
    WORD allocateCannotFindRouteCount;
    WORD allocateNoSlotsAvailableCount;
    WORD allocateServerIsDownCount;
} ShellStatisticsStruct;

typedef struct StructAddressTable
{
    BYTE serverUsed;
    BYTE orderNumber;
    BYTE serverNetwork[4];
    BYTE serverNode[6];
    WORD serverSocket;
    WORD receivedTimeOut;
    BYTE immediateNode[6];
    BYTE sequenceNumber;
    BYTE connectionNumber;
    BYTE connectionOK;
    WORD maximumTimeOut;
    BYTE reserved[5];
} AddressTableStruct;

typedef struct StructServerAddressTable
{
    AddressTableStruct addressTable[8];
} ServerAddressTableStruct;

typedef struct StructNameTable
{
    BYTE name[48];
} NameTableStruct;

typedef struct StructServerNameTable
{
    NameTableStruct nameTable[8];
} ServerNameTableStruct;

typedef struct StructPrimaryServer
{
    BYTE number;
} PrimaryServerStruct;

typedef struct StructShellVersion
{
    BYTE minor;
    BYTE major;
    BYTE rev;
} ShellVersionStruct;

typedef struct StructBridgeStatistics
{
    WORD tooManyHopsCount;
    WORD unknownNetworkCount;
    WORD noSpaceForServiceCount;
    WORD noReceiveBuffersCount;
    WORD notMyNetwork;
    long netBIOSPropogateCount;
    long totalPacketsServiced;
    long totalPacketsRouted;
} BridgeStatisticsStruct;

typedef struct StructNumber
{
    BYTE number[4];
} NumberStruct;

typedef struct StructNodeAddress
{
    BYTE address[6];
    BYTE reserved[2];
} NodeAddressStruct;

typedef struct StructLocalTables
{
    NumberStruct localNetworkNumber[16];
    NodeAddressStruct localNodeAddress[16];
} LocalTablesStruct;

typedef struct StructNetworkAddress
{
    BYTE address[4];
} NetworkAddressStruct;

typedef struct StructAllKnownNetworks
{
    WORD numberOfNetworkAddresses;
    NetworkAddressStruct networkAddress[128];
} AllKnownNetworksStruct;

typedef struct StructRoutingInfo
{
    BYTE routerForwardingAddress[6];
    BYTE routerBoardNumber;
    BYTE reserved[2];
    BYTE routeHops;
    WORD routeTime;
} RoutingInfoStruct;

typedef struct StructSpecificNetInfo
{
    BYTE networkAddress[4];
    BYTE hopsToNet;
    BYTE reservedA[7];
    WORD routeTimeToNet;
    WORD numberOfKnownRouters;
    RoutingInfoStruct routingInfo[MAX_ROUTES];
} SpecificNetworkInfoStruct;

typedef struct StrSrvrInfo
{
    WORD serverType;
    BYTE serverName[48];
} ServerInfoStruct;

typedef struct StructAllKnownServers
{
    WORD numberOfServers;
    ServerInfoStruct serverInfo[10];
} AllKnownServersStruct;

typedef struct StructRouteSourceInfo
{
    BYTE routeSourceAddress[6];
    WORD routeHopsToSource;
    BYTE reserved[2];
} RouteSourceInfoStruct;

typedef struct StrSpecSrvrInfo
{
    ServerInfoStruct serverInfo;
    BYTE serverAddress[12];
    WORD hopsToServer;
    BYTE reserved1[2];
    WORD numberOfRoutes;
    RouteSourceInfoStruct routeSourceInfo[MAX_ROUTES];
} SpecificServerInfoStruct;

typedef struct StructConnectionStatusBlock
{
    BYTE connectionState;
    BYTE connectionFlags;
    BYTE sourceConnectionID[2];             /* hi-lo */
    BYTE destinationConnectionID[2];        /* hi-lo */
    BYTE sequenceNumber[2];                 /* hi-lo */
    BYTE acknowledgeNumber[2];              /* hi-lo */
    BYTE allocationNumber[2];               /* hi-lo */
    BYTE remoteAcknowledgeNumber[2];        /* hi-lo */
    BYTE remoteAllocationNumber[2];         /* hi-lo */
    WORD connectionSocket;                  /* hi-lo */
    BYTE immediateAddress[6];               /* hi-lo */
    struct IPXAddress destination;
    BYTE retransmissionCount[2];            /* hi-lo */
    BYTE estimatedRoundTripDelay[2];        /* hi-lo */
    BYTE retransmittedPackets[2];           /* hi-lo */
    BYTE suppressedPackets[2];              /* hi-lo */
} ConnectionStatusBlock;


/* Personal NetWare Server Information */
typedef struct StructNLServerInfo
{
  WORD  StructVersion;
  BYTE  WorkgroupName[WORKGROUP_NAME_LENGTH];
  DWORD WorkgroupID;
  BYTE  ServerName[MAX_SERVER_NAME_LENGTH];
  DWORD ServerID;
  BYTE  OEMString[OEM_NAME_LENGTH];
  WORD  OEMID;
  BYTE  MajorVersion;
  BYTE  MinorVersion;
  BYTE  BetaVersion;
  BYTE  StartMinutes;
  BYTE  StartHours;
  BYTE  StartSeconds;
  WORD  StartYear;            /* Relative to 1980 */
  BYTE  StartDay;
  BYTE  StartMonth;
  BYTE  ServerAddressType;    /* 0=IPX */
  BYTE  ServerNetAddr[4];
  BYTE  ServerNodeAddr[6];
  BYTE  ServerSocket[2];
  BYTE  ServerDescription[50];
  WORD  CfgConnections;
  WORD  CfgOpenFiles;
  WORD  CfgClientTasks;
  WORD  CfgDirBuffers;
  WORD  CfgServiceExtensions;
  WORD  CurServiceExtensions;
  WORD  CfgResources;
  WORD  CurResources;
  WORD  CfgDirs;
  WORD  CurDirs;
  WORD  CfgPrns;
  BYTE  CurPrns;
  WORD  SizePrintBuffers;
  WORD  NumReceiveBuffers;
  WORD  RealSizeReceiveBuffers;
  WORD  SizeReceiveBuffers;
  WORD  NumIOBuffers;  
  WORD  SizeIOBuffers;
  WORD  ServerTSRSize;
  BYTE  AuditingFlag;         /* Zero/non-zero */
  BYTE  ServerFlags;          /* See SERVER_FLAG_* */
  BYTE  OperatingSystem;      /* 1=DOS */
  BYTE  DosMajorVersion;
  BYTE  DosMinorVersion;
  BYTE  IsShareLoaded;        /* Zero/non-zero */
  BYTE  ServerLoadType;       /* 0 = No load, 1 = Load Conventional */
  DWORD  ModuleFlag;          /* See SERVER_MODULE_* */
  DWORD  ScheduleControl;
  DWORD  ServerDiskVector;    /* Bit 0 = drive1 which is A */
  DWORD  RemovableDiskVector;
} NLServerInfoStruct;


typedef struct StructNLServerStats
{
  DWORD    TotalPacketsReceived;
  WORD     BadPackets;
  WORD     PacketsNotProcessedImmediately;
  WORD     LostResponses;
  WORD     PeakConnectionsUsed;
  WORD     PeakOpenFiles;
  WORD     PeakClientTasks;
  WORD     WatchdogPacketsSent;
  WORD     ClientsWatchdogged;
  WORD     ECBRepostsWithNoBuffers;
  WORD     SendPacketWithECBActive;
  WORD     TotalSlistRequests;
  WORD     ServerBusyPackets ;
  WORD     ServerBusyPacketsNoBuffers;
  WORD     UnknownRequests;
  WORD     WriteBehindMisses;
  DWORD    ReadCacheHits;
  DWORD    ReadCacheMisses;
  WORD     ReadsTooLarge;
  WORD     CriticalErrors;
  DWORD    SavedLargeDOSArea;
  DWORD    SavedSmallDOSArea;
  WORD     StarvationCounter;
  WORD     WriteBehindHits;
  WORD     CacheBlocksInUse;
  DWORD    Reserved;
  DWORD    PacketQueueRuns;
  DWORD    IdleLoopWaitHits;
  DWORD    Int21NotOurs;
  DWORD    TotalInt21Calls;
  WORD     PasswordFailures;
  WORD     Reserved0;
} NLServerStatsStruct;


typedef struct StructNLServerCurrentState
{
  WORD     CurConnections;
  WORD     CurOpenFiles;
  WORD     CurClientTasks;
  WORD     CurNumFreeBuffers;
  WORD     CurNumSemaphores;
} NLServerCurrentStateStruct;

typedef struct StructNLCacheStats
{
  BYTE     NLCacheMajorVersion;  /*binary decimal */
  BYTE     NLCacheMinorVersion;  /*binary decimal */
  WORD     TypeOfMemory;         /* 0=unsupported, */
                                 /* 1=Base memory being used */
                                 /* 2=Extended memory being used */
                                 /* 3=Expanded memory being used */
  DWORD    ReadRequests;
  DWORD    ActualReadsToDisk;
  DWORD    WriteRequests;
  DWORD    ActualWritesToDisk;
  WORD     NumberOfDiskErrors;
  WORD     NumberOfMemoryErrors;
  WORD     CurrentSizeInElements;
  WORD     MaxNumElements;
  WORD     ElementSize;          /* in bytes */
  WORD     MinNumElements;
} NLCacheStatsStruct;

typedef struct StructResponderType
{
   WORD     ResponderType;
   BYTE     ResponderMajorVersion;
   BYTE     ResponderMinorVersion;
} ResponderTypeStruct;

typedef struct StructGNMAQueryRequest
{
   BYTE     ComponentIDIndex;
   BYTE     RequestType;
   WORD     ResponderType;
   WORD     ResponderRequest;
   WORD     OtherData;
} GNMAQueryRequestStruct;

typedef struct StructGNMAInfo
{
   BYTE                    GNMAMajorVersion;
   BYTE                    GNMAMinorVersion;
   WORD                    GNMANumberOfResponders;
   ResponderTypeStruct     Type[131];
} GNMAInfoStruct;

/* MACHINE CONFIGURATION */
typedef struct StructMachineStaticInfo
{
   DWORD    StartDate;
   DWORD    StartTime;
   BYTE     OSVerMajor;
   BYTE     OSVerMinor;
   WORD     OSVersionType;
   BYTE     NumberOfSerialPorts;
   BYTE     NumberOfParallelPorts;
   BYTE     FloppyDriveCount;
   WORD     FloppyDriveType;
   BYTE     HardDriveCount;
   BYTE     KeyboardType;
   BYTE     BootVideoType;
   DWORD    DisplayMemory;
   BYTE     MathCoprocessor;
   BYTE     CoprocessorType;
   BYTE     CPUType;
   BYTE     Reserved;
   BYTE     ModelType;
   WORD     SerialAddress[4];
   WORD     ParallelAddress[4];
   BYTE     PointerDevice;
   WORD     SystemMemory;
   BYTE     MemoryTypes;
   BYTE     BIOSType;
   BYTE     BIOSRev[2];
   BYTE     EISA_MCASlots;
   BYTE     EISA_MCAIRQ[16];
   BYTE     EISA_CardName;
   BYTE     BusType;
   BYTE     HardDriveType[4];
   BYTE     MemTypeFlag;
   WORD     TotalXMS;
   WORD     AvailableXMS;
   WORD     TotalEMS;
   WORD     AvailableEMS;
   BYTE     Reserved0[22];
} MachineStaticInfoStruct;

typedef struct StructNonremoveableDrives
{
   BYTE     DriveNumber;
   DWORD    SectorsPerCluster;
   DWORD    TotalNumberOfClustersPerDrive;
   DWORD    NumberOfAvailableClusters;
   WORD     BytesPerSector;
} NonremoveableDrivesStruct;

typedef struct StructMachineDynamicInfo
{
   DWORD                      CurrentDate;
   DWORD                      CurrentTime;
   BYTE                       MouseDriver;
   BYTE                       Reserved[37];
   BYTE                       NonremoveableDriveCount;
   NonremoveableDrivesStruct  Drive[31];
} MachineDynamicInfoStruct;

/* PROTOCOL STACK INFORMATION */
typedef struct StructPStackID
{
   WORD          StackId;
   BYTE          Name[18];
} PStackIDStruct;

typedef struct StructProtocolStackList
{
   BYTE             NumberOfStacks;
   PStackIDStruct   PSID[20];
} ProtocolStackListStruct;

typedef struct StructProtocolStackConfig
{
   BYTE     ConfigMajorVersion;
   BYTE     ConfigMinorVersion;
   BYTE     ProtocolLongName[42];
   BYTE     ProtocolShortName[17];
   BYTE     StackMajorVersion;
   BYTE     StackMinorVersion;
   BYTE     Reserved[16];
} ProtocolStackConfigStruct;

typedef struct StructProtocolStackStats
{
   BYTE     StatMajorVersion;
   BYTE     StatMinorVersion;
   WORD     GenericCounters;
   DWORD    ValidCountersMask;
   DWORD    TotalTxPackets;
   DWORD    TotalRxPackets;
   DWORD    IgnoredRxPackets;
   BYTE     OtherData[508];
} ProtocolStackStatsStruct;

/* MLID INFORMATION */
typedef struct StructMLIDID
{
   WORD          MLIDId;
   BYTE          Name[18];
} MLIDIDStruct;

typedef struct StructMLIDList
{
   BYTE          NumberOfMLIDs;
   MLIDIDStruct  MLID[20];
} MLIDListStruct;

typedef struct StructMLIDConfiguration
{
   BYTE     Signature[26];
   BYTE     ConfigTableMajorVersion;
   BYTE     ConfigTableMinorVersion;
   BYTE     NodeAddress[6];
   WORD     ModeFlags;
   WORD     BoardNumber;
   WORD     BoardInstance;
   WORD     MaxPacketSize;
   WORD     BestDataSize;
   WORD     WorstDataSize;
   BYTE     NICLongName[42];
   BYTE     NICShortName[10];
   BYTE     FrameTypeString[42];
   WORD     Reserved;
   WORD     FrameTypeID;
   WORD     TransportTime;
   DWORD    SourceRouteHandler;
   WORD     LookAheadSize;
   WORD     LineSpeed;
   WORD     QueueDepth;
   BYTE     Reserved0[6];
   BYTE     DriverMajorVersion;
   BYTE     DriverMinorVersion;
   BYTE     Flags[2];
   WORD     SendRetries;
   DWORD    ConfigTableLink;
   BYTE     SharingFlags[2];
   WORD     Slot;
   WORD     IOAddress1;
   WORD     IORange1;
   WORD     IOAddress2;
   WORD     IORange2;
   DWORD    MemoryAddress1;
   WORD     MemorySize1;
   DWORD    MemoryAddress2;
   WORD     MemorySize2;
   BYTE     IntLine1;
   BYTE     IntLine2;
   BYTE     DMALine1;
   BYTE     DMALine2;
   BYTE     OtherData[328];
} MLIDConfigurationStruct;

typedef struct StructMLIDStatistics
{
   BYTE     DriverStatMajorVersion;
   BYTE     DriverStatMinorVersion;
   WORD     NumberGenericCounters;
   DWORD    ValidCountersMask;
   DWORD    TotalTxRequest;
   DWORD    TotalRxRequest;
   DWORD    NoECBAvailableCount;
   DWORD    TooBigTxRequest;
   DWORD    TooSmallTxRequest;
   DWORD    RxOverflowCount;
   DWORD    RxTooBigCount;
   DWORD    RxTooSmallCount;
   DWORD    TxMiscCount;
   DWORD    RxMiscCount;
   DWORD    TxRetryCount;
   DWORD    RxChecksumErrorCount;
   DWORD    RxMismatchCount;
   BYTE     OtherData[468];
} MLIDStatisticsStruct;

/* LSL INFORMATION */
typedef struct StructLSLStatistics
{
   BYTE     StatMajorVersion;
   BYTE     StatMinorVersion;
   WORD     GenericCounters;
   DWORD    ValidCountersMask;
   DWORD    TotalTxPackets;
   DWORD    GetECBRequests;
   DWORD    GetECBFailures;
   DWORD    AESEventsCount;
   DWORD    PostponedEvents;
   DWORD    CancelAESFailures;
   DWORD    Reserved0;
   DWORD    Reserved1;
   DWORD    TotalRxPackets;
   DWORD    UnclaimedPackets;
   BYTE     OtherData[480];
} LSLStatisticsStruct;

typedef struct StructLSLConfiguration
{
   BYTE     ConfigMajorVersion;
   BYTE     ConfigMinorVersion;
   DWORD    Reserved;
   DWORD    Reserved0;
   BYTE     LSLMajorVersion;
   BYTE     LSLMinorVersion;
   WORD     MaxBoardsNumber;
   WORD     MaxStacksNumber;
   BYTE     Reserved1[12];
} LSLConfigurationStruct;

typedef struct StructLSLNetCfgPath
{
   BYTE     NetCfgPath[128];
} LSLNetCfgPathStruct;


/* ------------------- Prototypes --------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

extern int AbortSendingPackets(
               WORD        connectionID,
               BYTE        componentNumber);

extern int BeginDiagnostics(
               BeginDiagnosticStruct *destination,
               WORD        *connectionID,
               BYTE        *componentList);

extern int EndDiagnostics(
               WORD        connectionID);

extern BYTE FindComponentOffset(
               BYTE        *componentList,
               BYTE        componentID);

extern int GetAllKnownNetworks(
               WORD        connectionID,
               BYTE        componentNumber,
               WORD        nextNetworkOffset,
               AllResponseData *Response,
               AllKnownNetworksStruct *ResponseData);

extern int GetAllKnownServers(
               WORD        connectionID,
               BYTE        componentNumber,
               WORD        numberServersToSkip,
               AllResponseData *Response,
               AllKnownServersStruct *ResponseData);

extern int GetBridgeDriverConfiguration(
               WORD        connectionID,
               BYTE        componentNumber,
               BYTE        LANBoardNumber,
               AllResponseData *Response,
               DriverConfigurationStruct *ResponseData);

extern int GetBridgeDriverStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               BYTE        LANBoardNumber,
               AllResponseData *Response,
               DriverStatisticsStruct *ResponseData);

extern int GetBridgeDriverStatus(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               BridgeDriverStatusStruct *ResponseData);

extern int GetBridgeStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               BridgeStatisticsStruct *ResponseData);

extern int GetDiagnosticResponse(
               WORD        connectionID,
               BYTE        *buffer1,
               WORD        size1,
               BYTE        *buffer2,
               WORD        size2);

extern int GetDiagnosticStatus(
               WORD        connectionID);

extern int GetGNMAInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               GNMAInfoStruct *ResponseData);

extern int GetIPXSPXVersion(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               IPXSPXVersion *ResponseData);

extern int GetIPXStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               IPXStatisticsStruct *ResponseData);

extern int GetLocalTables(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               LocalTablesStruct *ResponseData);

extern int GetLSLConfiguration(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               LSLConfigurationStruct *ResponseData);

extern int GetLSLStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               LSLStatisticsStruct *ResponseData);

extern int GetMachineStaticInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               MachineStaticInfoStruct *ResponseData);

extern int GetMachineDynamicInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               MachineDynamicInfoStruct *ResponseData);

extern int GetMLIDConfiguration(
               WORD        connectionID,
               BYTE        componentNumber,
               WORD        MlidId,
               AllResponseData *Response,
               MLIDConfigurationStruct *ResponseData);

extern int GetMLIDList(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               MLIDListStruct *ResponseData);

extern int GetMLIDStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               WORD        MlidId,
               AllResponseData *Response,
               MLIDStatisticsStruct *ResponseData);

extern int GetNetCfgPath(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               LSLNetCfgPathStruct *ResponseData);

extern int GetNLCacheStats(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               NLCacheStatsStruct *ResponseData);

extern int GetNLServerCurrentState(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               NLServerCurrentStateStruct *ResponseData);

extern int GetNLServerInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               NLServerInfoStruct *ResponseData);

extern int GetNLServerStats(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               NLServerStatsStruct *ResponseData);

extern int GetOSVersionInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               OSVersionStruct *ResponseData);

extern int GetPrimaryServerNumber(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               PrimaryServerStruct *ResponseData);

extern int GetProtocolStackConfiguration(
               WORD        connectionID,
               BYTE        componentNumber,
               WORD        stackID,
               AllResponseData *Response,
               ProtocolStackConfigStruct *ResponseData);

extern int GetProtocolStackList(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ProtocolStackListStruct *ResponseData);

extern int GetProtocolStackStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               WORD        stackID,
               AllResponseData *Response,
               ProtocolStackStatsStruct *ResponseData);

extern int GetRemoteSPXSocket(
               BeginDiagnosticStruct *destination,
               BYTE        *cList);

extern int GetServerAddressTable(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ServerAddressTableStruct *ResponseData);

extern int GetServerNameTable(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ServerNameTableStruct *ResponseData);

extern int GetShellAddress(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ShellAddressStruct *ResponseData);

extern int GetShellDriverConfiguration(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               DriverConfigurationStruct *ResponseData);

extern int GetShellDriverStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               DriverStatisticsStruct *ResponseData);

extern int GetShellStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ShellStatisticsStruct *ResponseData);

extern int GetShellVersionInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ShellVersionStruct *ResponseData);

extern int GetSpecificNetworkInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               BYTE        *networkAddress,
               AllResponseData *Response,
               SpecificNetworkInfoStruct *ResponseData);

extern int GetSpecificServerInfo(
               WORD        connectionID,
               BYTE        componentNumber,
               ServerInfoStruct *Server,
               AllResponseData *Response,
               SpecificServerInfoStruct *ResponseData);

extern int GetSPXStatistics(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               SPXStatisticsStruct *ResponseData);

extern int SendSPXPacket(
               WORD        connectionID,
               BYTE        *buffer1,
               WORD        size1);

extern int StartCountingPkts(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               StartCountingPacketsStruct *ResponseData);

extern int StartSendingPktsTimed(
               WORD        connectionID,
               BYTE        componentNumber,
               SendPacketsRequestStruct *RequestData,
               AllResponseData *Response,
               SendPacketsResponseStruct *ResponseData,
               WORD        Ticks);

extern int ReinitializeRouterTables(
               WORD        connectionID,
               BYTE        componentNumber,
               BYTE        *authorizationCode,
               AllResponseData *Response);

extern int ReturnReceivedPacketCount(
               WORD        connectionID,
               BYTE        componentNumber,
               AllResponseData *Response,
               ReturnReceivedPacketStruct *ResponseData);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif /* _DIAG_H */

