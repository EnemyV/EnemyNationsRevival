/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWMISC_INC
#define NWMISC_INC

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

#ifndef NW_SHORT_NAME_SERVER
# define NW_SHORT_NAME_SERVER 0
#endif

#ifndef NW_LONG_NAME_SERVER
# define NW_LONG_NAME_SERVER 1
#endif

#ifndef NW_ENCP_SERVER
#define NW_ENCP_SERVER 1
#endif

#ifndef NW_EXTENDED_NCP_SERVER
#define NW_EXTENDED_NCP_SERVER 1
#endif

#ifndef _NETX_COM
#define _NETX_COM     0x0001
#define _NETX_VLM     0x0002
#define _REDIR_LOADED 0x4000
#define _VLM_LOADED   0x8000
#endif

typedef struct
{
  BYTE day;
  BYTE month;
  WORD year;
} NW_DATE;

/* hours is a WORD so that this structure will be the same length as a dword */
typedef struct
{
  BYTE seconds;
  BYTE minutes;
  WORD hours;
} NW_TIME;

typedef enum
{
  NW_LONG_NAME_REQUESTER,
  NW_SHORT_NAME_REQUESTER,
  NW_ERROR_ON_REQUESTER_TYPE
} NW_REQUESTER_TYPE;

#ifndef NW_FRAGMENT_DEFINED
#define NW_FRAGMENT_DEFINED
typedef struct
{
  void NWPTR fragAddress;
#if defined(NWNLM)
  DWORD fragSize;
#else
  WORD fragSize;
#endif
} NW_FRAGMENT;
#endif

typedef struct
{
  WORD taskNumber;
  BYTE taskState;
} CONN_TASK;

typedef struct
{
  WORD  serverVersion;    /* use NW_ constants from nwserver.h */
  BYTE  lockState;
  WORD  waitingTaskNumber;
  DWORD recordStart;
  DWORD recordEnd;
  BYTE  volNumber;
  DWORD dirEntry;         /* this field is only valid in 3.11 */
  BYTE  nameSpace;        /* this field is only valid in 3.11 */
  WORD  dirID;            /* this field is only valid in 2.x  */
  char  lockedName[256];
  BYTE  taskCount;
  CONN_TASK tasks[256];
} CONN_TASK_INFO;

typedef struct
{
  BYTE  volNumber;
  DWORD dirEntry;
} DIR_ENTRY;

void NWAPI NWUnpackDateTime(
  DWORD dateTime,
  NW_DATE NWPTR sDate,
  NW_TIME NWPTR sTime);

void NWAPI NWUnpackDate(WORD date, NW_DATE NWPTR sDate);
void NWAPI NWUnpackTime(WORD time, NW_TIME NWPTR sTime);

DWORD NWAPI NWPackDateTime( NW_DATE NWPTR sDate, NW_TIME NWPTR sTime);
WORD NWAPI NWPackDate(NW_DATE NWPTR sDate);
WORD NWAPI NWPackTime(NW_TIME NWPTR sTime);

/* Avoid using the following three NWConvert{Date/Time} functions,
   they just call the NWUnpack{Date/Time} functions. They are here for
   compatibility reasons only. */
void NWAPI NWConvertDateTime(
  DWORD dateTime,
  NW_DATE NWPTR sDate,
  NW_TIME NWPTR sTime);

void NWAPI NWConvertDate(WORD date, NW_DATE NWPTR sDate);
void NWAPI NWConvertTime(WORD time, NW_TIME NWPTR sTime);

NWCCODE NWAPI NWRequest(
  NWCONN_HANDLE conn,
  WORD function,
  WORD numReqFrags,
  NW_FRAGMENT NWPTR reqFrags,
  WORD numReplyFrags,
  NW_FRAGMENT NWPTR replyFrags);

NWCCODE NWAPI NWRequest2(
  NWCONN_HANDLE conn,
  WORD function,
  WORD numReqFrags,
  NW_FRAGMENT NWPTR reqFrags,
  WORD numReplyFrags,
  NW_FRAGMENT NWPTR replyFrags,
  WORD NWFAR *actualReplyLen);

NWCCODE NWAPI _NWGetRequesterType(NW_REQUESTER_TYPE NWPTR type);

WORD NWAPI NWWordSwap(WORD swapWord);
DWORD NWAPI NWLongSwap(DWORD swapLong);

NWINT16 NWAPI NWInitDBCS(void);

NWCCODE NWAPI NWConvertPathToDirEntry(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  DIR_ENTRY NWPTR dirEntry);

NWCCODE NWAPI NWGetTaskInformationByConn(
  NWCONN_HANDLE conn,
  NWCONN_NUM connNum,
  CONN_TASK_INFO NWPTR taskInfo);

NWCCODE NWAPI NWGetRequesterVersion(
  BYTE NWPTR majorVer,
  BYTE NWPTR minorVer,
  BYTE NWPTR revision);

NWCCODE NWAPI NWIsLNSSupportedOnVolume(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path);

NWCCODE NWAPI _NWConvertHandle(
  NWCONN_HANDLE conn,
  BYTE accessMode,
  BYTE NWPTR NWHandle,
  DWORD fileSize,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWConvertFileHandle(
  NWFILE_HANDLEINT fileHandle,
  WORD handleType,
  BYTE NWPTR NWHandle,
  NWCONN_HANDLE NWPTR conn);

void NWAPI _NWConvert4ByteTo6ByteHandle(
  BYTE NWPTR NW4ByteHandle,
  BYTE NWPTR NW6ByteHandle);

NWCCODE NWAPI NWEndOfJob(void);

NWCCODE NWAPI NWCallsInit(void NWPTR in, void NWPTR out);

WORD NWAPI __NWGetNWCallsState(void);

NWCCODE NWAPI NWSetNetWareErrorMode(
  BYTE errorMode,
  BYTE NWPTR prevMode);

NWCCODE NWAPI NWSetEndOfJobStatus(
  BYTE endOfJobStatus,
  BYTE NWPTR prevStatus);

void NWAPI NWGetNWCallsVersion(
  BYTE NWPTR majorVer,
  BYTE NWPTR minorVer,
  BYTE NWPTR revLevel,
  BYTE NWPTR betaLevel);

NWCCODE NWAPI NWConvertHandle(
  NWCONN_HANDLE conn,
  BYTE accessMode,
  void NWPTR NWHandle,
  WORD handleSize,
  DWORD fileSize,
  NWFILE_HANDLE NWPTR fileHandle);

NWCCODE NWAPI NWCloseFile(
  NWCONN_HANDLE conn,
  void NWPTR NWHandle,
  WORD handleSize);

char NWPTR  NWAPI NWStripServerOffPath(
  char NWPTR path,
  char NWPTR server);

/* The stuff below this line may NOT be documented. Use with care. */
#if !defined(NWOS2)
#ifndef _REGISTERS_DEF
#define _REGISTERS_DEF

typedef struct
{
  WORD si;
  WORD ds;
  WORD di;
  WORD es;
  BYTE al, ah;
  BYTE bl, bh;
  BYTE cl, ch;
  BYTE dl, dh;
} BYTE_REGISTERS;

typedef struct
{
  WORD si;
  WORD ds;
  WORD di;
  WORD es;
  WORD ax;
  WORD bx;
  WORD cx;
  WORD dx;
  WORD bp;
  WORD flags;
} WORD_REGISTERS;

typedef struct
{
  void NWPTR requestBuffer;
  void NWPTR replyBuffer;
} PTR_REGISTERS;

typedef struct
{
  void NWPTR ds_si;
  void NWPTR es_di;
} SEG_OFF_REGISTERS;

typedef union
{
  WORD_REGISTERS w;
  BYTE_REGISTERS b;
  PTR_REGISTERS  p;
  SEG_OFF_REGISTERS s;
} REGISTERS;
#endif

#ifndef USE_DS
#define USE_DS  1
#define USE_ES  2
#define USE_DOS 0x80
#endif

int  NWAPI NWShellRequest(REGISTERS NWPTR , WORD);
WORD NWAPI NWVLMRequest(
  WORD callerID,
  WORD destID,
  WORD destFunc,
  REGISTERS NWPTR regs,
  WORD mask);
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
$Header:   K:\archives\src\sdkinc\nwmisc.h_v   1.4   10 Mar 1994 16:50:40   DOCOX  $
*/
