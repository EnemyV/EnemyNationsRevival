/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef REDIR_INC
#define REDIR_INC

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

#if !defined(NWOS2)

#ifndef REDIR_SIGNATURE
#define REDIR_SIGNATURE 'WN'

typedef struct tNW_REDIR_ENTRY
{
  WORD index;
  BYTE deviceStatus;
  BYTE deviceType;
  WORD storedValue;
  char localName[16];
  char networkName[128];
} NW_REDIR_ENTRY;
#endif

/* obsolete
NWCCODE NWAPI NWRedirLogin(
  char NWPTR serverName,
  char NWPTR userName,
  char NWPTR password);
*/

NWCCODE NWAPI NWRedirLogout(char NWPTR serverName);
NWCCODE NWAPI NWRedirectDevice(char NWPTR uncPath, char device);
NWCCODE NWAPI NWCancelRedirection(char device);
NWCCODE NWAPI NWGetRedirectionEntry(NW_REDIR_ENTRY NWPTR entry);

#endif

NWCCODE NWAPI NWParseUNCPath(
  char NWPTR uncPath,
  NWCONN_HANDLE NWPTR conn,
  char NWPTR serverName,
  char NWPTR volName,
  char NWPTR path,
  char NWPTR nwPath);

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
$Header:   K:\archives\src\sdkinc\nwredir.h_v   1.2   08 Oct 1993 18:10:54   JWOODBUR  $
*/
