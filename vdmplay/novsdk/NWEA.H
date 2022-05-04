/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWEA_INC
#define NWEA_INC

#ifndef NWCALDEF_INC
# include <nwcaldef.h>
#endif

#ifndef NWNAMSPC_INC
# include <nwnamspc.h>
#endif

#ifdef __N_USE_PRAGMA_PACK__
#pragma pack(1)
#elif defined(__BORLANDC__)
#pragma option -a-
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define EA_EOF                  1
#define EA_DONE                 1

#define EA_READWRITE            0
#define EA_CREATE               1

#define MISSING_EA_KEY             200  /* 0xC8 */
#define EA_NOT_FOUND               201  /* 0xC9 */
#define INVALID_EA_HANDLE_TYPE     202  /* 0xCA */
#define EA_NO_KEY_NO_DATA          203  /* 0xCB */
#define EA_NUMBER_MISMATCH         204  /* 0xCC */
#define EXTENT_NUMBER_OUT_OF_RANGE 205  /* 0xCD */
#define EA_BAD_DIR_NUM             206  /* 0xCE */
#define INVALID_EA_HANDLE          207  /* 0xCF */
#define EA_POSITION_OUT_OF_RANGE   208  /* 0xD0 */
#define EA_ACCESS_DENIED           209  /* 0xD1 */
#define DATA_PAGE_ODD_SIZE         210  /* 0xD2 */
#define EA_VOLUME_NOT_MOUNTED      211  /* 0xD3 */
#define BAD_PAGE_BOUNDARY          212  /* 0xD4 */
#define INSPECT_FAILURE            213  /* 0xD5 */
#define EA_ALREADY_CLAIMED         214  /* 0xD6 */
#define ODD_BUFFER_SIZE            215  /* 0xD7 */
#define NO_SCORECARDS              216  /* 0xD8 */
#define BAD_EDS_SIGNATURE          217  /* 0xD9 */
#define EA_SPACE_LIMIT             218  /* 0xDA */
#define EA_KEY_CORRUPT             219  /* 0xDB */
#define EA_KEY_LIMIT               220  /* 0xDC */
#define TALLY_CORRUPT              221  /* 0xDD */

typedef struct
{
  NWCONN_HANDLE connID;
  DWORD rwPosition;
  DWORD EAHandle;
  DWORD volNumber;
  DWORD dirBase;
  BYTE  keyUsed;
  WORD  keyLength;
  BYTE  key[256];
} NW_EA_HANDLE;

typedef struct
{
  NWCONN_HANDLE connID;
  WORD  nextKeyOffset;
  WORD  nextKey;
  DWORD numKeysRead;
  DWORD totalKeys;
  DWORD EAHandle;
  WORD  sequence;
  WORD  numKeysInBuffer;
  BYTE  enumBuffer[512];
} NW_EA_FF_STRUCT;

NWCCODE NWAPI NWCloseEA(NW_EA_HANDLE NWPTR EAHandle);

NWCCODE NWAPI NWFindFirstEA(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_EA_FF_STRUCT NWPTR ffStruct,
  NW_EA_HANDLE NWPTR EAHandle,
  char NWPTR EAName);

NWCCODE NWAPI NWFindNextEA(
  NW_EA_FF_STRUCT NWPTR ffStruct,
  NW_EA_HANDLE NWPTR EAHandle,
  char NWPTR EAName);

NWCCODE NWAPI NWReadEA(
  NW_EA_HANDLE NWPTR EAHandle,
  DWORD bufferSize,
  BYTE NWPTR buffer,
  DWORD NWPTR totalEASize,
  DWORD NWPTR amountRead);

NWCCODE NWAPI NWWriteEA(
  NW_EA_HANDLE NWPTR EAHandle,
  DWORD totalWriteSize,
  DWORD bufferSize,
  BYTE NWPTR buffer,
  DWORD NWPTR amountWritten);

NWCCODE NWAPI NWGetEAHandleStruct(
  NWCONN_HANDLE conn,
  char NWPTR EAName,
  NW_IDX NWPTR idxStruct,
  NW_EA_HANDLE NWPTR EAHandle);

NWCCODE NWAPI NWOpenEA(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE dirHandle,
  char NWPTR path,
  char NWPTR EAName,
  BYTE nameSpace,
  NW_EA_HANDLE NWPTR EAHandle);

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
$Header:   K:\archives\src\sdkinc\nwea.h_v   1.1   09 Aug 1993 12:04:44   JWOODBUR  $
*/
