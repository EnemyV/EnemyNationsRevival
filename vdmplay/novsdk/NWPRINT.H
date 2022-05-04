/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWPRINT_INC
#define NWPRINT_INC

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

/*
**  Call NWGetMaxPrinters() to see how many of these are valid.
*/

#define LPT1 1
#define LPT2 2
#define LPT3 3
#define LPT4 4
#define LPT5 5
#define LPT6 6
#define LPT7 7
#define LPT8 8
#define LPT9 9

#define START_CAPTURE             1
#define END_CAPTURE               2
#define CANCEL_CAPTURE            3
#define GET_PRINT_JOB_FLAGS       4
#define SET_PRINT_JOB_FLAGS       5
#define GET_BANNER_USER_NAME      6
#define SET_BANNER_USER_NAME      7
#define GET_PRINTER_SETUP_STRING  8
#define SET_PRINTER_SETUP_STRING  9
#define GET_PRINTER_RESET_STRING  10
#define SET_PRINTER_RESET_STRING  11

typedef struct
{
  BYTE  clientStation;
  BYTE  clientTask;
  DWORD clientID;
  DWORD targetServerID;
  BYTE  targetExecutionTime[6];
  BYTE  jobEntryTime[6];
  WORD  jobNumber;
  WORD  formType;
  BYTE  jobPosition;
  BYTE  jobControlFlags;
  BYTE  jobFileName[14];
  BYTE  jobFileHandle[6];
  BYTE  servicingServerStation;
  BYTE  servicingServerTask;
  DWORD servicingServerID;
  BYTE  jobDescription[50];
  BYTE  clientJobInfoVer;
  BYTE  tabSize;
  WORD  numberCopies;
  WORD  printFlags;
  WORD  maxLines;
  WORD  maxChars;
  BYTE  formName[16];
  BYTE  reserved[6];    /* must be set to zeros */
  BYTE  bannerUserName[13];
  BYTE  bannerFileName[13];
  BYTE  bannerHeaderFileName[14];
  BYTE  filePathName[80];
} PrintJobStruct;

typedef struct
{
  DWORD clientStation;
  DWORD clientTask;
  DWORD clientID;
  DWORD targetServerID;
  BYTE  targetExecutionTime[6];
  BYTE  jobEntryTime[6];
  DWORD jobNumber;
  WORD  formType;
  WORD  jobPosition;
  WORD  jobControlFlags;
  BYTE  jobFileName[14];
  DWORD jobFileHandle;
  DWORD servicingServerStation;
  DWORD servicingServerTask;
  DWORD servicingServerID;
  BYTE  jobDescription[50];
  BYTE  clientJobInfoVer;
  BYTE  tabSize;
  WORD  numberCopies;
  WORD  printFlags;
  WORD  maxLines;
  WORD  maxChars;
  BYTE  formName[16];
  BYTE  reserved[6];      /* must be set to zeros */
  BYTE  bannerUserName[13];
  BYTE  bannerFileName[13];
  BYTE  bannerHeaderFileName[14];
  BYTE  filePathName[80];
} NWPrintJobStruct;

typedef struct PRINTER_STATUS
{
  BYTE  printerHalted;
  BYTE  printerOffline;
  BYTE  currentFormType;
  BYTE  redirectedPrinter;
} PRINTER_STATUS;

typedef struct
{
  BYTE      jobDescription[ 50 ];   /* OS/2, VLM only                         */
                                    /* VLM returns or sets only 12 characters */
                                    /* plus the NULL -- a total of 13 BYTEs   */
  BYTE      jobControlFlags;        /* OS/2, VLM only */
  BYTE      tabSize;
  WORD      numCopies;
  WORD      printFlags;
  WORD      maxLines;
  WORD      maxChars;
  BYTE      formName[ 13 ];
  BYTE      reserved[ 9 ];
  WORD      formType;
  BYTE      bannerText[ 13 ];
  BYTE      reserved2;
  WORD      flushCaptureTimeout;    /* DOS/WIN only */
  BYTE      flushCaptureOnClose;    /* DOS/WIN only */
} NWCAPTURE_FLAGSRW;

#define NWCAPTURE_FLAGS1 NWCAPTURE_FLAGSRW

typedef struct
{
  NWCONN_HANDLE connID;
  DWORD     queueID;
  WORD      setupStringMaxLen;
  WORD      resetStringMaxLen;
  BYTE      LPTCaptureFlag;         /* DOS/WIN only */
  BYTE      fileCaptureFlag;        /* DOS/WIN only */
  BYTE      timingOutFlag;          /* DOS/WIN only */
  BYTE      inProgress;             /* DOS/WIN only */
  BYTE      printQueueFlag;         /* DOS/WIN only */
  BYTE      printJobValid;          /* DOS/WIN only */
  char      queueName[ 65 ];        /* VLM only     */
} NWCAPTURE_FLAGSRO;

#define NWCAPTURE_FLAGS2 NWCAPTURE_FLAGSRO

#ifdef NWOS2

#define NWAPIPIPE                 "\\PIPE\\NWSPOOL\\API"  /*IPC to API*/
#define NET_SPOOL_SEG             "\\sharemem\\nwspool\\seg1"
#define NET_SPOOL_SEM1            "\\sem\\nwspool\\sem1"
#define NET_SPOOL_SEM2            "\\sem\\nwspool\\sem2"
#define NET_SPOOL_SEM3            "\\sem\\nwspool\\sem3"

typedef struct
{
  DWORD targetServerID;
  BYTE  targetExecutionTime[6];
  BYTE  jobDescription[50];
  BYTE  jobControlFlags;
  BYTE  tabSize;
  WORD  numberCopies;
  WORD  printFlags;
  WORD  maxLines;
  WORD  maxChars;
  BYTE  formName[16];
  BYTE  reserved1[6];  /* must be set to zeros */
  WORD  formType;
  BYTE  bannerFileName[13];
  BYTE  reserved2;    /* must be set to zero */

  /* The following fields can be gotten, but not set */
  NWCONN_HANDLE connID;
  DWORD queueID;
  WORD  setupStringMaxLength;
  WORD  resetStringMaxLength;
} SpoolFlagsStruct;

typedef struct _NWPipeStruct
{
  WORD  fwCommand;
  WORD  idSession;
  DWORD idQueue;
  WORD  idConnection;
  WORD  idDevice;
  WORD  fwMode;
  WORD  fwScope;
  WORD  cbBufferLength;
  BYTE  fbValidBuffer;
  SpoolFlagsStruct  nwsSpoolFlags;
  BYTE  szBannerUserName[13];
  WORD  rc;
} NWPipeStruct;

NWCCODE NWAPI NWSpoolGetPrintJobFlags(
  WORD deviceID,
  SpoolFlagsStruct NWPTR flagsBuffer,
  WORD mode,
  WORD NWPTR scope);

NWCCODE NWAPI NWSpoolSetPrintJobFlags(
  WORD deviceID,
  SpoolFlagsStruct NWPTR flagsBuffer,
  WORD unused);

NWCCODE NWAPI NWSpoolGetPrinterSetupString(
  WORD deviceID,
  WORD NWPTR bufferLen,
  char NWPTR buffer,
  WORD mode,
  WORD NWPTR scope);

NWCCODE NWAPI NWSpoolSetPrinterSetupString(
  WORD deviceID,
  WORD bufferLen,
  char NWPTR buffer,
  WORD scope);

NWCCODE NWAPI NWSpoolGetPrinterResetString(
  WORD deviceID,
  WORD NWPTR bufferLen,
  char NWPTR buffer,
  WORD mode,
  WORD NWPTR scope);

NWCCODE NWAPI NWSpoolSetPrinterResetString(
  WORD deviceID,
  WORD bufferLen,
  char NWPTR buffer,
  WORD scope);

#else

typedef struct
{
  BYTE  status;
  BYTE  flags;
  BYTE  tabSize;
  BYTE  serverPrinter;
  BYTE  numberCopies;
  BYTE  formType;
  BYTE  reserved;
  BYTE  bannerText[13];
  BYTE  reserved2;
  BYTE  localLPTDevice;
  WORD  captureTimeOutCount;
  BYTE  captureOnDeviceClose;
} CaptureFlagsStruct;

typedef struct
{
  BYTE  status;
  BYTE  flags;
  BYTE  tabSize;
  BYTE  serverPrinter;
  BYTE  numberCopies;
  BYTE  formType;
  BYTE  reserved;
  BYTE  bannerText[13];
  BYTE  reserved2;
  BYTE  localLPTDevice;
  WORD  captureTimeOutCount;
  BYTE  captureOnDeviceClose;
  WORD  maxLines;
  WORD  maxChars;
  BYTE  formName[13];
  BYTE  LPTCaptureFlag;
  BYTE  fileCaptureFlag;
  BYTE  timingOutFlag;
  char NWPTR printerSetupBuffAddr;
  char NWPTR printerResetBuffAddr;
  NWCONN_ID_BYTE connID;
  BYTE  captureInProgress;
  BYTE  printQueueFlag;
  BYTE  printJobValid;
  DWORD queueID;
  WORD  printJobNumber;
} FlagBufferStruct;

/* Used by VLM code */
typedef struct
{
  DWORD nameNDSID;
  WORD  connHandle;
  char  queueName[ 65 ];
  DWORD targetServerID;
  BYTE  targetExecutionTime[ 6 ];
  WORD  jobControlFlags;
  BYTE  bannerName[ 13 ];
  BYTE  jobDescription[ 13 ];
  DWORD reserved;
} ExtendedCaptureFlags;

NWCCODE NWAPI NWGetPrinterDefaults(
  BYTE NWPTR status,
  BYTE NWPTR flags,
  BYTE NWPTR tabSize,
  BYTE NWPTR serverPrinter,
  BYTE NWPTR numberCopies,
  BYTE NWPTR formType,
  char NWPTR bannerText,
  BYTE NWPTR localLPTDevice,
  WORD NWPTR captureTimeOutCount,
  BYTE NWPTR captureOnDeviceClose);

NWCCODE NWAPI NWSetPrinterDefaults(
  BYTE flags,
  BYTE tabSize,
  BYTE serverPrinter,
  BYTE numberCopies,
  BYTE formType,
  char NWPTR bannerText,
  BYTE localLPTDevice,
  WORD captureTimeOutCount,
  BYTE captureOnDeviceClose);

NWCCODE NWAPI NWStartLPTCapture(WORD);
NWCCODE NWAPI NWGetLPTCaptureStatus(WORD NWPTR );

NWCCODE NWAPI NWStartFileCapture(
  NWCONN_HANDLE conn,
  BYTE LPTDevice,
  NWDIR_HANDLE dirhandle,
  char NWPTR filePath
);

#endif

NWCCODE NWAPI NWStartFileCapture2(
  NWCONN_HANDLE conn,
  BYTE LPTDevice,
  NWDIR_HANDLE dirHandle,
  char NWFAR *filePath,
  WORD flags);

NWCCODE NWAPI NWSpoolStartCapture(
  WORD deviceID,
  DWORD queueID,
  NWCONN_HANDLE conn,
  WORD scope);

NWCCODE NWAPI NWSpoolEndCapture(
  WORD deviceID,
  WORD scope);

NWCCODE NWAPI NWSpoolCancelCapture(
  WORD deviceID,
  WORD scope);

NWCCODE NWAPI NWSpoolGetBannerUserName(
  char NWPTR username,
  WORD mode,
  WORD NWPTR scope);

NWCCODE NWAPI NWSpoolSetBannerUserName(
  char NWPTR username,
  WORD scope);

NWCCODE NWAPI NWGetPrinterStatus(
  NWCONN_HANDLE conn,
  WORD printerNumber,
  PRINTER_STATUS NWPTR status);

NWCCODE NWAPI NWStartQueueCapture(
  NWCONN_HANDLE conn,
  BYTE LPTDevice,
  DWORD queueID,
  char NWPTR queueName);

NWCCODE NWAPI NWGetCaptureStatus(
  BYTE LPTDevice);

NWCCODE NWAPI NWFlushCapture(
  BYTE LPTDevice);

NWCCODE NWAPI NWEndCapture(
  BYTE LPTDevice);

NWCCODE NWAPI NWCancelCapture(
  BYTE LPTDevice);

NWCCODE NWAPI NWGetBannerUserName(
  char NWPTR userName);

NWCCODE NWAPI NWSetBannerUserName(
  char NWPTR userName);

NWCCODE NWAPI NWGetCaptureFlags(
  BYTE LPTDevice,
  NWCAPTURE_FLAGS1 NWPTR captureFlags1,
  NWCAPTURE_FLAGS2 NWPTR captureFlags2);

NWCCODE NWAPI NWSetCaptureFlags(
  NWCONN_HANDLE conn,
  BYTE LPTDevice,
  NWCAPTURE_FLAGS1 NWPTR captureFlags1);

NWCCODE NWAPI NWGetPrinterStrings(
  BYTE LPTDevice,
  WORD NWPTR setupStringLen,
  char NWPTR setupString,
  WORD NWPTR resetStringLen,
  char NWPTR resetString);

NWCCODE NWAPI NWSetPrinterStrings(
  BYTE LPTDevice,
  WORD setupStringLen,
  char NWPTR setupString,
  WORD resetStringLen,
  char NWPTR resetString);

NWCCODE NWAPI NWGetMaxPrinters(
  WORD NWPTR numPrinters);

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
$Header:   K:\archives\src\sdkinc\nwprint.h_v   1.4   08 Oct 1993 18:10:06   JWOODBUR  $
*/
