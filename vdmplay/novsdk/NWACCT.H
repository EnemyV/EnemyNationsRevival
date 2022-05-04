/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NOACCT_INC
#define NOACCT_INC

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
  DWORD objectID;
  INT32 amount;
} HOLDS_INFO;

typedef struct
{
  WORD holdsCount;
  HOLDS_INFO holds[16];
} HOLDS_STATUS;

NWCCODE NWAPI NWGetAccountStatus(
  NWCONN_HANDLE conn,
  WORD objectType,
  char NWPTR objectName,
  INT32 NWPTR balance,
  INT32 NWPTR limit,
  HOLDS_STATUS NWPTR holds);

NWCCODE NWAPI NWQueryAccountingInstalled(
  NWCONN_HANDLE conn,
  BYTE NWPTR installed);

NWCCODE NWAPI NWSubmitAccountCharge(
  NWCONN_HANDLE conn,
  WORD objectType,
  char NWPTR objectName,
  WORD serviceType,
  INT32 chargeAmount,
  INT32 holdCancelAmount,
  WORD noteType,
  char NWPTR note);

NWCCODE NWAPI NWSubmitAccountHold(
  NWCONN_HANDLE conn,
  WORD objectType,
  char NWPTR objectName,
  INT32 holdAmount);

NWCCODE NWAPI NWSubmitAccountNote(
  NWCONN_HANDLE conn,
  WORD objectType,
  char NWPTR objectName,
  WORD serviceType,
  WORD noteType,
  char NWPTR note);
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
$Header:   K:\archives\src\sdkinc\nwacct.h_v   1.2   08 Oct 1993 18:10:04   JWOODBUR  $
*/
