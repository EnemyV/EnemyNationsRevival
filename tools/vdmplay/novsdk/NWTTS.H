/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWTTS_INC
#define NWTTS_INC

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
  DWORD systemElapsedTime;
  BYTE  TTS_Supported;
  BYTE  TTS_Enabled;
  WORD  TTS_VolumeNumber;
  WORD  TTS_MaxOpenTransactions;
  WORD  TTS_MaxTransactionsOpened;
  WORD  TTS_CurrTransactionsOpen;
  DWORD TTS_TotalTransactions;
  DWORD TTS_TotalWrites;
  DWORD TTS_TotalBackouts;
  WORD  TTS_UnfilledBackouts;
  WORD  TTS_DiskBlocksInUse;
  DWORD TTS_FATAllocations;
  DWORD TTS_FileSizeChanges;
  DWORD TTS_FilesTruncated;
  BYTE  numberOfTransactions;
  struct
  {
    BYTE connNumber;
    BYTE taskNumber;
  } connTask[235];
} TTS_STATS;

NWCCODE NWAPI NWTTSAbortTransaction(NWCONN_HANDLE conn);
NWCCODE NWAPI NWTTSBeginTransaction(NWCONN_HANDLE conn);
NWCCODE NWAPI NWTTSIsAvailable(NWCONN_HANDLE conn);

NWCCODE NWAPI NWTTSGetControlFlags(NWCONN_HANDLE conn, BYTE NWPTR controlFlags);
NWCCODE NWAPI NWTTSSetControlFlags(NWCONN_HANDLE conn, BYTE controlFlags);

NWCCODE NWAPI NWTTSEndTransaction(NWCONN_HANDLE conn, DWORD NWPTR transactionNum);
NWCCODE NWAPI NWTTSTransactionStatus(NWCONN_HANDLE conn, DWORD transactionNum);

NWCCODE NWAPI NWTTSGetProcessThresholds(
  NWCONN_HANDLE conn,
  BYTE NWPTR logicalLockLevel,
  BYTE NWPTR physicalLockLevel);

NWCCODE NWAPI NWTTSSetProcessThresholds(
  NWCONN_HANDLE conn,
  BYTE logicalLockLevel,
  BYTE physicalLockLevel);

NWCCODE NWAPI NWTTSGetConnectionThresholds(
  NWCONN_HANDLE conn,
  BYTE NWPTR logicalLockLevel,
  BYTE NWPTR physicalLockLevel);

NWCCODE NWAPI NWTTSSetConnectionThresholds(
  NWCONN_HANDLE conn,
  BYTE logicalLockLevel,
  BYTE physicalLockLevel);

NWCCODE NWAPI NWEnableTTS(NWCONN_HANDLE conn);

NWCCODE NWAPI NWDisableTTS(NWCONN_HANDLE conn);

NWCCODE NWAPI NWGetTTSStats(
  NWCONN_HANDLE conn,
  TTS_STATS NWPTR ttsStats);

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
$Header:   K:\archives\src\sdkinc\nwtts.h_v   1.1   09 Aug 1993 12:04:32   JWOODBUR  $
*/
