/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NCPEXT_INC
#define NCPEXT_INC

#ifndef NWCALDEF_INC
# include <nwcaldef.h>
#endif

#ifndef NWMISC_INC
# include <nwmisc.h>
#endif

#ifdef __N_USE_PRAGMA_PACK__
#pragma pack(1)
#elif defined(__BORLANDC__)
#pragma option -a-
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NW_NCPX_BEGIN_SCAN 0xFFFFFFFF

NWCCODE NWAPI NWGetNCPExtensionInfo(
  NWCONN_HANDLE conn,
  DWORD NCPExtensionID,
  char  NWPTR NCPExtensionName,
  BYTE  NWPTR majorVersion,
  BYTE  NWPTR minorVersion,
  BYTE  NWPTR revision,
  BYTE  NWPTR queryData);

NWCCODE NWAPI NWNCPExtensionRequest(
  NWCONN_HANDLE conn,
  DWORD NCPExtensionID,
  void  NWPTR requestData,
  WORD requestDataLen,
  void  NWPTR replyData,
  WORD NWPTR replyDataLen);

NWCCODE NWAPI NWFragNCPExtensionRequest(
  NWCONN_HANDLE conn,
  DWORD NCPExtensionID,
  WORD reqFragCount,
  NW_FRAGMENT NWPTR reqFragList,
  WORD replyFragCount,
  NW_FRAGMENT NWPTR replyFragList);

NWCCODE NWAPI NWScanNCPExtensions(
  NWCONN_HANDLE conn,
  DWORD NWPTR NCPExtensionID,
  char  NWPTR NCPExtensionName,
  BYTE  NWPTR majorVersion,
  BYTE  NWPTR minorVersion,
  BYTE  NWPTR revision,
  BYTE  NWPTR queryData);

NWCCODE NWAPI NWGetNCPExtensionInfoByName(
  NWCONN_HANDLE conn,
  char  NWPTR NCPExtensionName,
  DWORD NWPTR NCPExtensionID,
  BYTE  NWPTR majorVersion,
  BYTE  NWPTR minorVersion,
  BYTE  NWPTR revision,
  BYTE  NWPTR queryData);

NWCCODE NWAPI NWGetNCPExtensionsList(
  NWCONN_HANDLE conn,
  DWORD NWPTR startNCPExtensionID,
  WORD  NWPTR itemsInList,
  DWORD NWPTR NCPExtensionIDList);

NWCCODE NWAPI NWGetNumberNCPExtensions(
  NWCONN_HANDLE conn,
  DWORD NWPTR numNCPExtensions);

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
$Header:   K:\archives\src\sdkinc\nwncpext.h_v   1.1   09 Aug 1993 12:04:46   JWOODBUR  $
*/
