/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWCALDEF_INC
#define NWCALDEF_INC

/*************************************************************************
                            Basic Defines

 The only defines you may need to modify are bracketed here. The rest of
 the defines in this header are based on these.

 To compile in for 32 bit OS/2 which does the far thunking for you, the
 general rule of thumb is to change the following:

   #define NWFAR    __far16
   all instances of 'int' to 'short'

   you may also need to change pascal to __pascal

 The above changes will work for Borland C++ for OS/2.
 IBM C/2 requires much more extensive modifications, especially concerning
 the data pointers. Apparently C/2 wants pointers to be defined as:

   void * _Seg16 (rather than void _Seg16 *)

 To help with this a pointer modifer is used: NWPTR
 For C/2 redefine NWPTR to be:

   #define NWPTR * _Far16

 Also, functions in C/2 use a slightly different modifier: _Far16

   #define NWAPI _Far16 pascal

*************************************************************************/

#ifndef NWPASCAL
#define NWPASCAL pascal
#endif

#ifndef NWFAR
#define NWFAR far
#endif

#ifndef NWAPI
#define NWAPI NWFAR NWPASCAL
#endif

#ifndef NWPTR
#define NWPTR NWFAR *
#endif

#ifndef NWINT16
#define NWINT16 int
#endif

#ifndef INT8
#define INT8 char
#endif

#ifndef INT16
#define INT16 short
#endif

#ifndef INT32
#define INT32 long
#endif

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned long
#endif

/**************************************************************************/
#ifndef NWCONN_HANDLE
#define NWCONN_HANDLE     unsigned int
#endif
#define NWCONN_ID         unsigned int
#define NWCONN_NUM        unsigned int
#define NWCCODE           unsigned int
#define NWFILE_HANDLE     unsigned int
#define NWFILE_HANDLEINT  int
#define NWWORD            unsigned int

#define NWCONN_ID_BYTE    unsigned char
#define NWCONN_NUM_DWORD  unsigned long
#define NWCONN_NUM_BYTE   unsigned char
#define NWDIR_HANDLE      unsigned char
/**************************************************************************/

#ifndef TRUE
#define TRUE     1
#endif

#ifndef FALSE
#define FALSE    0
#endif

/* the following is for model detection for data model */
#if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__) || \
    defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)       || \
    defined(_M_I86TM) || defined(_M_I86SM) || defined(_M_I86MM)
#define NW_NEAR_DATA 1
#endif

/* concerning NULLs: these are here just in case they are not defined by
   the headers included prior to nwcaldef.h. Since NWCalls uses far (32 bit)
   pointers for everything 0L should always be used. NWNULL is provided for
   this use.
*/

#ifndef  NULL
#ifdef NW_NEAR_DATA
#define  NULL 0
#else
#define  NULL 0L
#endif
#endif

#ifndef NWNULL
#define NWNULL   0L
#endif


/*************************************************************************
                       End of Basic Defines
*************************************************************************/

#ifndef _PINT8
#define _PINT8 INT8 NWPTR
#endif

#ifndef _PUINT8
#define _PUINT8 UINT8 NWPTR
#endif

#ifndef _PINT16
#define _PINT16 INT16 NWPTR
#endif

#ifndef _PUINT16
#define _PUINT16 UINT16 NWPTR
#endif

#ifndef _PINT32
#define _PINT32 INT32 NWPTR
#endif

#ifndef _PUINT32
#define _PUINT32 UINT32 NWPTR
#endif

#ifndef _PVOID
#define _PVOID void NWPTR
#endif

/* __WINDOWS_H          borland windows.h
   _INC_WINDOWS         microsoft windows.h
   ___WIN386_INCLUDED__ watcom 32-bit
   LOWORD               windows.h 3.0 */

#if !defined(__WINDOWS_H) &&         \
    !defined(_INC_WINDOWS) &&        \
    !defined(__WIN386_INCLUDED__) && \
    !defined(LOWORD)

#if !defined(BYTE) && !defined(OS2DEF_INCLUDED)
#define BYTE UINT8
#endif

#ifndef WORD
#define WORD unsigned NWINT16
#endif

#ifndef DWORD
#define DWORD UINT32
#endif

#ifndef LONG
#define LONG INT32
#endif

#endif /* windows detection */

#define _PSTR    INT8     NWPTR
#define _PBYTE   UINT8    NWPTR
#define _PINT    NWINT16  NWPTR
#define _PWORD unsigned NWINT16 NWPTR
#define _PDWORD  UINT32   NWPTR
#define _PLONG   INT32    NWPTR

/* add compiler detection here if #pragma pack(1) is appropriate for your compiler.
   otherwise, modify each header with the appropriate pragma or use a command line
   switch */

#ifndef __N_USE_PRAGMA_PACK__
#if defined(_MSC_VER) || defined(__WATCOMC__) || defined(__EXPRESSC__)
#define __N_USE_PRAGMA_PACK__
#endif
#endif

#ifndef FA_READ_ONLY
#define FA_NORMAL         0x00
#define FA_READ_ONLY      0x01
#define FA_HIDDEN         0x02
#define FA_SYSTEM         0x04
#define FA_EXECUTE_ONLY   0x08
#define FA_DIRECTORY      0x10
#define FA_NEEDS_ARCHIVED 0x20
#define FA_SHAREABLE      0x80

/* Extended file attributes */
#define FA_TRANSACTIONAL  0x10
#define FA_INDEXED        0x20
#define FA_READ_AUDIT     0x40
#define FA_WRITE_AUDIT    0x80
#endif

/* the following is a the correct attribute mask list */
/* The difference between these and the FA_ constants above is that these
   are in the correct positions. The last four attributes above are 8 bits
   off. (They need to be shifted 8 bits to the left.) */
#ifndef A_NORMAL
#define A_NORMAL             0x00000000L
#define A_READ_ONLY          0x00000001L
#define A_HIDDEN             0x00000002L
#define A_SYSTEM             0x00000004L
#define A_EXECUTE_ONLY       0x00000008L
#define A_DIRECTORY          0x00000010L
#define A_NEEDS_ARCHIVED     0x00000020L
#define A_SHAREABLE          0x00000080L
#define A_DONT_SUBALLOCATE   0x00000800L
#define A_TRANSACTIONAL      0x00001000L
#define A_INDEXED            0x00002000L /* not in the NCP book */
#define A_READ_AUDIT         0x00004000L
#define A_WRITE_AUDIT        0x00008000L
#define A_IMMEDIATE_PURGE    0x00010000L
#define A_RENAME_INHIBIT     0x00020000L
#define A_DELETE_INHIBIT     0x00040000L
#define A_COPY_INHIBIT       0x00080000L
#define A_FILE_MIGRATED      0x00400000L
#define A_DONT_MIGRATE       0x00800000L
#define A_IMMEDIATE_COMPRESS 0x02000000L
#define A_FILE_COMPRESSED    0x04000000L
#define A_DONT_COMPRESS      0x08000000L
#define A_CANT_COMPRESS      0x20000000L
#endif

/* access rights attributes */
#ifndef AR_READ_ONLY
#define AR_READ           0x0001
#define AR_WRITE          0x0002
#define AR_READ_ONLY      0x0001
#define AR_WRITE_ONLY     0x0002
#define AR_DENY_READ      0x0004
#define AR_DENY_WRITE     0x0008
#define AR_COMPATIBILITY  0x0010
#define AR_WRITE_THROUGH  0x0040
#define AR_OPEN_COMPRESSED 0x0100
#endif

/* search attributes */
#ifndef SA_HIDDEN
#define SA_NORMAL         0x0000
#define SA_HIDDEN         0x0002
#define SA_SYSTEM         0x0004
#define SA_SUBDIR_ONLY    0x0010
#define SA_SUBDIR_FILES   0x8000
#define SA_ALL            0x8006
#endif

#define MAX_VOL_LEN 17        /* this includes a byte for null  */


#ifndef USE_NW_WILD_MATCH
#define USE_NW_WILD_MATCH   0
#endif

#ifndef USE_DOS_WILD_MATCH
#define USE_DOS_WILD_MATCH  1
#endif

/* Scope specifiers */
#define GLOBAL       0
#define PRIVATE      1
#define MY_SESSION   2
#define ALL_SESSIONS 3

#endif

/*
$Header:   K:\archives\src\sdkinc\nwcaldef.h_v   1.5   04 Aug 1994 13:56:00   DJHARRIS  $
*/
