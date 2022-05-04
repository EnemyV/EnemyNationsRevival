#ifndef _UNICODE_HEADER_
#define _UNICODE_HEADER_
/****************************************************************************
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written consent of
 * Novell, Inc.
 ****************************************************************************/

#ifdef __BORLANDC__
#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned size_t;
#  endif
#elif defined(NWNLM)
#  include <stddef.h>
#elif defined(__WATCOMC__)
#  ifndef _SIZE_T_DEFINED_
#    define _SIZE_T_DEFINED_
     typedef unsigned size_t;
#  endif
#else
#  ifndef _SIZE_T_DEFINED
#    define _SIZE_T_DEFINED
     typedef unsigned int size_t;
#  endif
#endif

#ifndef NWNLM
#ifndef NWAPI
# define NWAPI far pascal
#endif

#if defined(MEDIUM_MODEL)
# undef NWFAR
# define NWFAR
#endif

#ifndef NWFAR
# define NWFAR far
#endif

#else /* NWNLM */
# undef NWAPI
# define NWAPI
# undef NWFAR
# define NWFAR
#endif

/****************************************************************************/

/*
    Definition of a Unicode character - Must be 16 bits wide
*/
typedef unsigned short unicode;

/****************************************************************************/

/*
    True and False
*/
#ifndef TRUE
# define TRUE    1
#endif
#ifndef FALSE
# define FALSE   0
#endif

/*
    Error codes
*/
#define UNI_ALREADY_LOADED   -489  /* Already loaded another country or code page */
#define UNI_FUTURE_OPCODE    -490  /* Rule table has unimplimented rules*/
#define UNI_NO_SUCH_FILE     -491  /* No such file or directory */
#define UNI_TOO_MANY_FILES   -492  /* Too many files already open */
#define UNI_NO_PERMISSION    -493  /* Permission denied on file open */
#define UNI_NO_MEMORY        -494  /* Not enough memory */
#define UNI_LOAD_FAILED      -495  /* NWLoadRuleTable failed, don't know why */
#define UNI_HANDLE_BAD       -496  /* Rule table handle was bad */
#define UNI_HANDLE_MISMATCH  -497  /* Rule table handle doesn't match operation*/
#define UNI_RULES_CORRUPT    -498  /* Rule table is corrupt */
#define UNI_NO_DEFAULT       -499  /* No default rule and no 'No map' character*/
#define UNI_INSUFFICIENT_BUFFER -500
#define UNI_OPEN_FAILED      -501  /* Open failed in NWLoadRuleTable */
#define UNI_NO_LOAD_DIR      -502  /* Load directory could not be determined */

/****************************************************************************/

/*
  Functions in Unicode.Lib that have no counterpart in string.h
*/

#ifdef __cplusplus
    extern "C" {
#endif

/*
  Memory models with near returns require different libc calls
     NWInitUnicodeTables()
     NWFreeUnicodeTables()
     NWLoadRuleTable()
     NWUnloadRuleTable()
  MSC uses M_I86TM, M_I86SM and M_I86CM for tiny, small and compact.
  Borland, Zortech and Watcom all use __TINY__ and __SMALL__ __COMPACT__.
*/

#if (defined(NWDOS) && \
      (defined(M_I86SM) || defined(M_I86TM) || defined(M_I86CM) || \
      defined(__SMALL__) || defined(__TINY__) || defined(__COMPACT__)))

#define NWInitUnicodeTables     NWNInitUnicodeTables
int NWAPI NWNInitUnicodeTables(
   int            countryCode, 
   int            codePage);

#define NWFreeUnicodeTables     NWNFreeUnicodeTables
int NWAPI NWNFreeUnicodeTables(
   void);

#define NWLoadRuleTable         NWNLoadRuleTable
int NWAPI NWNLoadRuleTable(          /* Load a rule table                   */
   char     NWFAR *ruleTableName,   /* Name of the rule table              */
   void NWFAR * NWFAR *ruleHandle); /* Where to put the rule table handle  */

#define NWUnloadRuleTable       NWNUnloadRuleTable
int NWAPI NWNUnloadRuleTable(        /* Unload a rule table                 */
   void     NWFAR *ruleHandle);     /* Rule table handle                   */

#else  /* small model */

int NWAPI NWInitUnicodeTables(
   int            countryCode, 
   int            codePage);

int NWAPI NWFreeUnicodeTables(
   void);

int NWAPI NWLoadRuleTable(          /* Load a rule table                   */
   char     NWFAR *ruleTableName,   /* Name of the rule table              */
   void NWFAR * NWFAR *ruleHandle); /* Where to put the rule table handle  */

int NWAPI NWUnloadRuleTable(        /* Unload a rule table                 */
   void     NWFAR *ruleHandle);     /* Rule table handle                   */
#endif   /* small model */


int NWAPI NWLocalToUnicode(         /* Convert local to Unicode            */
   void     NWFAR *ruleHandle,      /* Rule table handle                   */
   unicode  NWFAR *dest,            /* Buffer for resulting Unicode        */
   size_t         maxLen,           /* Size of results buffer              */
#ifndef MACINTOSH
   unsigned char     NWFAR *src,    /* Buffer with source local code       */
#else
   void           *src,
#endif
   unicode        noMap,            /* No map character                    */
   size_t   NWFAR *len);            /* Number of unicode chars in output   */

int NWAPI NWUnicodeToLocal(         /* Convert Unicode to local code       */
   void     NWFAR *ruleHandle,      /* Rule table handle                   */
#ifndef MACINTOSH
   unsigned char     NWFAR *dest,            /* Buffer for resulting local code     */
#else
   void           *dest,
#endif
   size_t         maxLen,           /* Size of results buffer              */
   unicode  NWFAR *src,             /* Buffer with source Unicode          */
   char           noMap,            /* No Map character                    */
   size_t   NWFAR *len);            /* Number of bytes in output           */

int NWAPI NWUnicodeToCollation(     /* Convert Unicode to collation        */
   void     NWFAR *ruleHandle,      /* Rule table handle                   */
   unicode  NWFAR *dest,            /* Buffer for resulting Unicode weights*/
   size_t         maxLen,           /* Size of results buffer              */
   unicode  NWFAR *src,             /* Buffer with source Unicode          */
   unicode        noMap,            /* No map character                    */
   size_t   NWFAR *len);            /* Number of unicode chars in output   */

int NWAPI NWUnicodeCompare(         /* Compare two unicode characters      */
   void     NWFAR *ruleHandle,      /* Rule table handle                   */
   unicode        chr1,             /* 1st character                       */
   unicode        chr2);            /* 2nd character                       */

int NWAPI NWUnicodeToMonocase(      /* Convert Unicode to collation        */
   void     NWFAR *ruleHandle,      /* Rule table handle                   */
   unicode  NWFAR *dest,            /* Buffer for resulting Unicode weights*/
   size_t         maxLen,           /* Size of results buffer              */
   unicode  NWFAR *src,             /* Buffer with source Unicode          */
   size_t   NWFAR *len);            /* Number of unicode chars in output   */

int NWAPI NWGetUnicodeToLocalHandle(
   void NWFAR * NWFAR *handle);

int NWAPI NWGetLocalToUnicodeHandle(
   void NWFAR * NWFAR *handle);

int NWAPI NWGetMonocaseHandle(
   void NWFAR * NWFAR *handle);

int NWAPI NWGetCollationHandle(
   void NWFAR * NWFAR *handle);

/****************************************************************************/

/*
    Functions in Unicode.Lib that work like those in string.h
*/
unicode NWFAR * NWAPI unicat(    /* Corresponds to strcat               */
   unicode  NWFAR *s1,           /* Original string                     */
   unicode  NWFAR *s2);          /* String to be appended               */

unicode NWFAR * NWAPI unichr(    /* Corresponds to strchr               */
   unicode  NWFAR *s,            /* String to be scanned                */
   int            c);            /* Character to be found               */

unicode NWFAR * NWAPI unicpy(    /* Corresponds to strcpy               */
   unicode  NWFAR *s1,           /* Destination string                  */
   unicode  NWFAR *s2);          /* Source string                       */

size_t NWAPI unicspn(            /* Corresponds to strcspn              */
   unicode  NWFAR *s1,           /* String to be scanned                */
   unicode  NWFAR *s2);          /* Character set                       */

size_t NWAPI unilen(             /* Corresponds to strlen               */
   unicode  NWFAR *s);           /* String to determine length of       */

unicode NWFAR * NWAPI unincat(   /* Corresponds to strncat              */
   unicode  NWFAR *s1,           /* Original string                     */
   unicode  NWFAR *s2,           /* String to be appended               */
   size_t         n);            /* Maximum characters to be appended   */

unicode NWFAR * NWAPI unincpy(   /* Corresponds to strncpy              */
   unicode  NWFAR *s1,           /* Destination string                  */
   unicode  NWFAR *s2,           /* Source string                       */
   size_t         n);            /* Maximum length                      */

unicode NWFAR * NWAPI uninset(   /* Corresponds to strnset              */
   unicode  NWFAR *s,            /* String to be modified               */
   int            c,             /* Fill character                      */
   size_t         n);            /* Maximum length                      */

unicode NWFAR * NWAPI unipbrk(   /* Corresponds to strpbrk              */
   unicode  NWFAR *s1,           /* String to be scanned                */
   unicode  NWFAR *s2);          /* Character set                       */

unicode NWFAR * NWAPI unipcpy(   /* Corresponds to strpcpy              */
   unicode  NWFAR *s1,           /* Destination string                  */
   unicode  NWFAR *s2);          /* Source string                       */

unicode NWFAR * NWAPI unirchr(   /* Corresponds to strrchr              */
   unicode  NWFAR *s,            /* String to be scanned                */
   int            c);            /* Character to be found               */

unicode NWFAR * NWAPI unirev(    /* Corresponds to strrev               */
   unicode  NWFAR *s);           /* String to be reversed               */

unicode NWFAR * NWAPI uniset(    /* Corresponds to strset               */
   unicode  NWFAR *s,            /* String to modified                  */
   int            c);            /* Fill character                      */

size_t NWAPI unispn(             /* Corresponds to strspn               */
   unicode  NWFAR *s1,           /* String to be tested                 */
   unicode  NWFAR *s2);          /* Character set                       */

unicode NWFAR * NWAPI unistr(    /* Corresponds to strstr               */
   unicode  NWFAR *s1,           /* String to be scanned                */
   unicode  NWFAR *s2);          /* String to be located                */

unicode NWFAR * NWAPI unitok(    /* Corresponds to strtok               */
   unicode  NWFAR *s1,           /* String to be parsed                 */
   unicode  NWFAR *s2);          /* Delimiter values                    */

int NWAPI uniicmp(               /* Corresponds to stricmp              */
   unicode  NWFAR *s1,           /* 1st string to be compared           */
   unicode  NWFAR *s2);          /* 2nd string to be compared           */

int NWAPI uninicmp(              /* Corresponds to strnicmp             */
   unicode  NWFAR *s1,           /* 1st string to be compared           */
   unicode  NWFAR *s2,           /* 2nd string to be compared           */
   size_t         n);            /* Maximum length                      */

#ifndef MACINTOSH
size_t NWAPI unisize(            /* Corresponds to sizeof               */
   unicode  NWFAR *s);
#else
# define unisize(uS)  ((unilen((unicode *) uS) + 1) * sizeof(unicode))
#endif

#ifdef __cplusplus
    }
#endif

/****************************************************************************/
/****************************************************************************/
#endif
