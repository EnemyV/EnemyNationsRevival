/*
  +************************************************************************+
  | Copyright (c) 1994 Novell, Inc.  All rights reserved.                  |
  |                                                                        |
  | THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND      |
  | TREATIES.                                                              |
  |                                                                        |
  | NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED, COPIED,        |
  | DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED,       |
  | EXPANDED, COLLECTED, COMPILED, LINKED, RECAST, TRANSFORMED OR ADAPTED  |
  | WITHOUT THE PRIOR WRITTEN CONSENT OF NOVELL, INC.                      |
  |                                                                        |
  | ANY USE OR EXPLOITATION OF THIS WORK WITHOUT AUTHORIZATION COULD       |
  | SUBJECT THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.               |
  +************************************************************************+
  | Source module name:  ntypes.h                                          |
  |            Version:  1.01                                              |
  +************************************************************************+
*/

/*
  +************************************************************************+
  | Source control information:                                            |
  +************************************************************************+
  |
  | Source Control:
  | ---------------
  | $Workfile:   ntypes.h  $
  | $Revision:   1.24  $
  | $Author:   DINO  $
  | $Date:   15 Jun 1994 18:28:24  $
  |
  | UNIX:
  | ----
  | $Id:         $
  | $Source:     $
  |
  +************************************************************************+
  | Description:                                                           |
  +************************************************************************+
  |
  |     Novell common definitions and macros.
  |
  +************************************************************************+
*/

#if ! defined( NTYPES_H )
#define NTYPES_H

/*
   This file is broken up into three sections. The first defines some
   items to help predict compilers better. The second section defines a some
   items according to the platform. The third section defines everything
   else.

   This file is guarenteed NOT to include any other files. It should be
   placed before any NetWare include files, but after any standard C or
   platform specific files, such as "windows.h".

   For compatibility purposes a platform may be defined in multiple ways.
   Some of these methods are defacto standards for their appropriate
   platforms. Note that AppWare considers the N_PLAT_xxxx method to be the
   default. 

   (Some of the following are trademarks of their respective companies)

      Microsoft Windows Family   - N_PLAT_MSW
      IBM OS/2                   - N_PLAT_OS2
      DOS                        - N_PLAT_DOS, NWDOS
      Novell NetWare NLM         - N_PLAT_NLM
      Macintosh                  - N_PLAT_MAC
      Unix                       - N_PLAT_UNIX
   
   One of the following architectures will always be defined:

      N_ARCH_16                  - 16 bit word size
      N_ARCH_32                  - 32 bit word size
      N_ARCH_64                  - 64 bit word size
   
   Typical combinations are as follows.  The preferred defines are the first
   one in each of the following lines:

      Microsoft Windows (16 bit) - N_PLAT_MSW and N_ARCH_16, NWWIN
      Microsoft Windows (32 bit) - N_PLAT_MSW and N_ARCH_32, WIN32
      IBM OS/2 (16 bit)          - N_PLAT_OS2 and N_ARCH_16, NWOS2
      IBM OS/2 (32 bit)          - N_PLAT_OS2 and N_ARCH_32
      DOS                        - N_PLAT_DOS, NWDOS
      Novell NetWare NLM         - N_PLAT_NLM and N_ARCH_32
      Macintosh                  - N_PLAT_MAC and N_ARCH_32 
      Unix                       - N_PLAT_UNIX and N_ARCH_32 or N_ARCH_64


   Note that the following constants are assumed to be defined for the
   given circumstances:

      _INC_WINDOWS        - Microsoft's WINDOWS.H file has been included.
      RC_INVOKED          - Microsoft's RC resource compiler is currently
                            processing the file.
*/

#ifdef NWOS2
   #ifndef N_PLAT_OS2
      #define N_PLAT_OS2
   #endif
   #ifndef N_ARCH_16
      #define N_ARCH_16
   #endif
#elif defined(NWWIN)
   #ifndef N_PLAT_MSW
      #define N_PLAT_MSW
   #endif
   #ifndef N_ARCH_16
      #define N_ARCH_16
   #endif
#elif defined(NWDOS)
   #ifndef N_PLAT_DOS
      #define N_PLAT_DOS
   #endif
#elif defined(WIN32)
   #ifndef N_PLAT_MSW
      #define N_PLAT_MSW
   #endif
   #ifndef N_ARCH_32
      #define N_ARCH_32
   #endif
#endif

#if defined(N_PLAT_MSW) && !defined(N_ARCH_32) && !defined(N_ARCH_16)
	#define N_ARCH_16
#endif

/* section 1a - compiler specific defines */

#ifdef _MSC_VER
   #if(_MSC_VER < 700)
      #define N_MSC_OLD
   #else
      #define N_MSC
   #endif
#endif /* section 1 */
                                            /* Resource compiler define */
#if defined( RC_INVOKED )
   #if !defined( N_RC_INVOKED )
      #define N_RC_INVOKED
   #endif  /* !defined( N_RC_INVOKED ) */
#endif  /* defined( RC_INVOKED ) */
                                            /* section 2 - platforms */
#if !defined( N_RC_INVOKED )

   #if defined(N_ARCH_16) && defined(N_PLAT_MSW)

      #ifndef NWWIN
         #define NWWIN
      #endif

      #if defined(N_MSC_OLD)
         #define N_NEAR   near
         #define N_FAR    far
         #define N_HUGE   _huge
         #define N_PASCAL pascal
         #define N_CDECL  cdecl
         #define N_EXPORT _export
      #else
         #define N_NEAR   __near
         #define N_FAR    __far
         #define N_HUGE   _huge
         #define N_PASCAL __pascal
         #define N_CDECL  __cdecl
         #define N_EXPORT _export
      #endif

      #define N_API              N_FAR N_PASCAL
      #define N_API_VARARGS      N_FAR N_CDECL
      #define N_CALLBACK         N_FAR N_PASCAL
      #define N_CALLBACK_VARARGS N_FAR N_CDECL

      #undef  N_INT_ENDIAN_HI_LO
      #undef  N_INT_STRICT_ALIGNMENT

   #elif defined(N_ARCH_32) && defined(N_PLAT_MSW)

      #ifndef WIN32
         #define WIN32
      #endif

      #define N_NEAR
      #define N_FAR
      #define N_HUGE	
      #define N_PASCAL
      #define N_CDECL __cdecl
      #define N_EXPORT 

      #define N_API              __stdcall
      #define N_API_VARARGS      N_CDECL
      #define N_VARARGS          N_CDECL
      #define N_CALLBACK         __stdcall
      #define N_CALLBACK_VARARGS N_CDECL

      #undef  N_INT_ENDIAN_HI_LO
      #undef  N_INT_STRICT_ALIGNMENT

   #elif defined(N_ARCH_16) && defined(N_PLAT_OS2)

      #if defined(N_MSC_OLD)
         #define N_NEAR   near
         #define N_FAR    far
         #define N_PASCAL pascal
         #define N_CDECL  cdecl
      #else
         #define N_NEAR   __near
         #define N_FAR    __far
         #define N_PASCAL __pascal
         #define N_CDECL  __cdecl
      #endif

      #define N_API              N_FAR N_PASCAL
      #define N_API_VARARGS      N_FAR N_CDECL
      #define N_CALLBACK         N_FAR N_PASCAL
      #define N_CALLBACK_VARARGS N_FAR N_CDECL

      #undef  N_INT_ENDIAN_HI_LO
      #undef  N_INT_STRICT_ALIGNMENT

   #elif defined(N_ARCH_32) && defined(N_PLAT_OS2)

      #define N_NEAR
      #define N_FAR
      #define N_PASCAL _Pascal
      #define N_CDECL

      #define N_API              N_PASCAL
      #define N_API_VARARGS      N_CDECL
      #define N_CALLBACK         N_PASCAL
      #define N_CALLBACK_VARARGS N_CDECL

      #undef  N_INT_ENDIAN_HI_LO
      #undef  N_INT_STRICT_ALIGNMENT

   #elif defined(N_PLAT_DOS)

      #if defined(N_MSC_OLD)
         #define N_NEAR   near
         #define N_FAR    far
         #define N_PASCAL pascal
         #define N_CDECL  cdecl
      #else
         #define N_NEAR   __near
         #define N_FAR    __far
         #define N_PASCAL __pascal
         #define N_CDECL  __cdecl
      #endif

      #ifndef NWDOS
         #define NWDOS
      #endif

      #define N_API              N_FAR N_PASCAL
      #define N_API_VARARGS      N_FAR N_CDECL
      #define N_CALLBACK         N_FAR N_PASCAL
      #define N_CALLBACK_VARARGS N_FAR N_CDECL

      #undef  N_INT_ENDIAN_HI_LO
      #undef  N_INT_STRICT_ALIGNMENT

   #elif defined(N_PLAT_NLM)

      #define N_NEAR
      #define N_FAR
      #define N_PASCAL  pascal
      #define N_CDECL   cdecl

      #define N_API              N_PASCAL
      #define N_API_VARARGS      N_CDECL
      #define N_CALLBACK         N_PASCAL
      #define N_CALLBACK_VARARGS N_CDECL

      #undef  N_INT_ENDIAN_HI_LO
      #undef  N_INT_STRICT_ALIGNMENT

   #elif defined(N_PLAT_MAC)

      #define N_NEAR
      #define N_FAR
      #define N_PASCAL  pascal
      #define N_CDECL

      #define N_API              N_PASCAL
      #define N_API_VARARGS      N_CDECL
      #define N_CALLBACK         N_PASCAL
      #define N_CALLBACK_VARARGS N_CDECL

      #define N_INT_ENDIAN_HI_LO
      #if defined( mc68020 )
         #undef  N_INT_STRICT_ALIGNMENT
      #else
         #define N_INT_STRICT_ALIGNMENT
      #endif

   #elif defined(N_PLAT_UNIX)

      #define N_NEAR
      #define N_FAR
      #define N_PASCAL
      #define N_CDECL

      #define N_API              N_PASCAL
      #define N_API_VARARGS      N_CDECL
      #define N_CALLBACK         N_PASCAL
      #define N_CALLBACK_VARARGS N_CDECL

      #if defined(BYTE_ORDER) && defined(BIG_ENDIAN)
         #if (BYTE_ORDER == BIG_ENDIAN)
            #define N_INT_ENDIAN_HI_LO
         #endif
      #elif defined(vax) || defined(ns32000) || defined(sun386) || \
            defined(MIPSEL) || defined(BIT_ZERO_ON_RIGHT)
         /* do nothing (little endian) */
      #elif defined(sel) || defined(pyr) || defined(mc68000) || \
            defined(sparc) || defined(is68k) || defined(tahoe) || \
            defined(ibm032) || defined(ibm370) || defined(MIPSEB) || \
            defined(__hpux) || defined (BIT_ZERO_ON_LEFT)
         #define N_INT_ENDIAN_HI_LO
      #endif

      /* For now, we assume strict alignment for all Unix platforms since it
         is the worst case (safe); we could optimize for certain platforms
         in the future */

      #define N_INT_STRICT_ALIGNMENT

   #else    /* no platform constant */

      #error A platform must be defined

   #endif   /* platforms */

#endif  /* !defined( N_RC_INVOKED ) */

#if !defined(NULL) && defined(__cplusplus)
   #define NULL   0
#endif

#if defined( N_RC_INVOKED )
	#define NULL 0
#endif

#ifndef NULL
   #ifdef N_MSC
      #define NULL ((void *) 0)
   #else
      #if defined(M_I86S) || defined(M_I86SM) || defined(M_I86C) || \
          defined(M_I86CM) || \
          defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__)

         #define NULL   0
      #else
         #define NULL   0L
      #endif
   #endif
#endif /* NULL */

/*
------------------------------------------------------------------------------
Constants:

Define the standard constant values
------------------------------------------------------------------------------
*/

/*
    Standard constants.
*/

#define N_ALWAYS        1
#define N_SUCCESS       0
#define N_FAILURE       (-1)
#define N_YES           1
#define N_NO            0
#define N_FALSE     0
#define N_TRUE          1
#define N_UNKNOWN       (N_TRUE + 1)

/*
    System constants.
*/

#define N_SYS_USER          0
#define N_SYS_NETWARE       1
#define N_SYS_NAWF          2
#define N_SYS_MAX_COUNT     20
#define N_SYS_NAME_MAX_LEN  31

/*
    Historical constant synonyms.
*/

#if ! defined( N_INC_NO_OLD_CONSTANTS )

    #if ! defined( TRUE )
        #define TRUE        N_TRUE
    #endif

    #if ! defined( FALSE )
        #define FALSE       N_FALSE
    #endif

#endif

/* N_BITSPERBYTE is currently only used internally */
#if defined BITSPERBYTE
   #define N_BITSPERBYTE  BITSPERBYTE
#else
   #define N_BITSPERBYTE  8
#endif

/*
------------------------------------------------------------------------------
Basic Machine Dependent Types:

Define the basic machine dependent type definitions
------------------------------------------------------------------------------
*/

#if ! defined(N_RC_INVOKED)

typedef signed char     nint8;
#if !defined( N_PLAT_UNIX )
typedef unsigned char   nuint8;
#else
    /*
        WARNING:  THE NEXT STATEMENT PRESUMES CHAR IS UNSIGNED!
    */
    #ifndef __GNUC__
    typedef unsigned char nuint8;
    #else
    typedef char nuint8;
    #endif
#endif

typedef signed short    nint16;
typedef unsigned short  nuint16;
#if defined(__alpha)
typedef signed int nint32;
typedef unsigned int nuint32;
typedef signed long nint64;
typedef unsigned long nuint64;
#define nint64_type
#else
typedef signed long     nint32;
typedef unsigned long   nuint32;
#endif
typedef float           nreal32;
typedef double          nreal64;

#ifndef N_PLAT_MAC
   typedef signed int      nint;
   typedef unsigned int    nuint;
   typedef unsigned int    nbool;
   #ifndef N_PLAT_WIN32
      #if ! defined(__alpha) && ! defined(_AIX)
          typedef long double     nreal80;
          #define nreal80_type
        /* VC++ for NT does not support nreal80's by default. Check readme for
           instructions on how to enable them */
      #endif
   #endif

   typedef double          nreal;
#else
   typedef signed long     nint;
   typedef unsigned long   nuint;
   typedef unsigned char   nbool;
   #ifndef __TYPES__
      #include <Types.h>		/* for the definition of extended */
   #endif
	#if defined( powerc ) || defined( __powerc ) || defined( THINK_C )
		typedef	long double nreal80;
		typedef	double		nreal;
	#else
		typedef extended	nreal80;
		typedef extended	nreal;
	#endif
   #define nreal80_type
#endif

/*
------------------------------------------------------------------------------
Basic Derived Types:

Define the basic derived type definitions
------------------------------------------------------------------------------
*/

#if defined(__alpha)
typedef nuint64 nparam;
#else
typedef nuint32 nparam;
#endif
typedef nparam nhdl;
typedef nparam nid;
typedef nuint8  nflag8;
typedef nuint16 nflag16;
typedef nuint32 nflag32;

typedef nuint8  nbool8;

typedef nint32  nfixed;
#if defined(N_USE_UNSIGNED_CHAR)
typedef nuint8  nstr8;
#else
typedef char  nstr8;
#endif
typedef nuint16 nstr16;
#if defined(N_USE_STR_16)
   typedef  nstr16 nstr;
#else
   typedef  nstr8  nstr;
#endif
typedef nstr    nchar;
typedef nstr8   nchar8;
typedef nstr16  nchar16;
typedef nchar16 nwchar;

#define NWRCODE  nint32

typedef void      N_FAR *  nptr;
typedef void      N_FAR *  npproc;

/*
   Pointers to scalars.
*/

typedef nint8     N_FAR *  pnint8;
typedef nuint8    N_FAR *  pnuint8;
typedef nint16    N_FAR *  pnint16;
typedef nuint16   N_FAR *  pnuint16;
typedef nint32    N_FAR *  pnint32;
typedef nuint32   N_FAR *  pnuint32;
#if defined( nint64_type )
   typedef nint64    N_FAR *  pnint64;
   typedef nuint64   N_FAR *  pnuint64;
#endif
typedef nint      N_FAR *  pnint;
typedef nuint     N_FAR *  pnuint;
typedef nflag8    N_FAR *  pnflag8;
typedef nflag16   N_FAR *  pnflag16;
typedef nflag32   N_FAR *  pnflag32;
typedef nbool     N_FAR *  pnbool;
typedef nbool8    N_FAR *  pnbool8;
typedef nfixed    N_FAR *  pnfixed;
typedef nstr      N_FAR *  pnstr;
typedef pnstr     N_FAR *  ppnstr;
typedef nstr16    N_FAR *  pnstr16;
typedef pnstr16   N_FAR *  ppnstr16;
typedef nstr8     N_FAR *  pnstr8;
typedef pnstr8    N_FAR *  ppnstr8;
typedef nchar     N_FAR *  pnchar;
typedef pnchar    N_FAR *  ppnchar;
typedef nchar16   N_FAR *  pnchar16;
typedef pnchar16  N_FAR *  ppnchar16;
typedef nchar8    N_FAR *  pnchar8;
typedef pnchar8   N_FAR *  ppnchar8;
typedef nwchar    N_FAR *  pnwchar;
typedef pnwchar   N_FAR *  ppnwchar;
typedef nreal     N_FAR *  pnreal;
typedef nreal32   N_FAR *  pnreal32;
typedef nreal64   N_FAR *  pnreal64;
#if defined( nreal80_type )
   typedef nreal80   N_FAR *  pnreal80;
#endif
typedef nid       N_FAR *  pnid;
typedef nhdl      N_FAR *  pnhdl;
typedef nparam    N_FAR *  pnparam;
typedef nptr      N_FAR *  pnptr;
typedef npproc    N_FAR *  pnpproc;

/*
	 Structures.
*/

#if defined( _XLIB_H_ )	/* Only use XPoint if xlib.h is included */
typedef XPoint npt16;
#elif defined( N_PLAT_MAC )
typedef struct tagnpt16
{
    nint16      y;
    nint16      x;
} npt16;
#elif defined( N_PLAT_OS2 )
typedef POINTS npt16;
#elif defined( N_PLAT_DOS ) || defined( N_PLAT_MSW ) || defined( N_PLAT_UNIX )
typedef struct tagnpt16
{
	nint16       x;
	nint16       y;
} npt16;
#elif defined( N_PLAT_NLM )
typedef struct tagnpt16       /*  This could be done with an else instead */
{                             /*  of with an elif as is the npt32 below.  */
	nint16       x;
	nint16       y;
} npt16;
#endif

#if defined( N_PLAT_MAC )
typedef struct tagnpt32
{
	 nint32      y;
	 nint32      x;
} npt32;
#elif defined( N_PLAT_OS2 )
typedef POINTL npt32;
#else
typedef struct tagnpt32
{
	 nint32      x;
	 nint32      y;
} npt32;
#endif

#if defined ( N_PLAT_OS2 )
#pragma pack(1) /* force structure alignment packing */
#endif

#if defined( __QUICKDRAW__ )
typedef Point npthv16;
#elif defined( N_PLAT_MAC )
typedef struct tagnpthv16
{
    nint16      v;
    nint16      h;
} npthv16;
#else
typedef struct tagnpthv16
{
	 nint16      h;
	 nint16      v;
} npthv16;
#endif

#if defined( N_PLAT_MAC )
typedef struct tagnpthv32
{
	 nint32      v;
	 nint32      h;
} npthv32;
#else
typedef struct tagnpthv32
{
	 nint32      h;
	 nint32      v;
} npthv32;
#endif

#if defined( __QUICKDRAW__ )
typedef Rect nrect16;
#elif defined( N_PLAT_UNIX ) || defined( N_PLAT_DOS )  || defined( N_PLAT_MSW ) || defined( N_PLAT_NLM )
typedef struct tagnrect16
{
	 nint16      left;
	 nint16      top;
	 nint16      right;
	 nint16      bottom;
} nrect16;
#elif defined( N_PLAT_MAC )
typedef struct tagnrect16
{
	 nint16      top;
	 nint16      left;
	 nint16      bottom;
	 nint16      right;
} nrect16;
#elif defined( N_PLAT_OS2 )
typedef struct tagnrect16
{
    nint16      left;
    nint16      bottom;
    nint16      right;
    nint16      top;
} nrect16;
#endif

#if defined( N_PLAT_MSW ) || defined( N_PLAT_UNIX ) || defined( N_PLAT_DOS ) || defined( N_PLAT_NLM )
typedef struct tagnrect32
{
    nint32      left;
    nint32      top;
    nint32      right;
    nint32      bottom;
} nrect32;
#elif defined( N_PLAT_MAC )
typedef struct tagnrect32
{
    nint32      top;
    nint32      left;
    nint32      bottom;
    nint32      right;
} nrect32;
#elif defined( N_PLAT_OS2 )
typedef struct tagnrect32
{
    nint32      left;
    nint32      bottom;
    nint32      right;
    nint32      top;
} nrect32;
#endif

#if defined( N_PLAT_MSW ) || defined( N_PLAT_UNIX ) || defined( N_PLAT_DOS ) || defined( N_PLAT_NLM )
typedef struct tagnrectmm16
{
	 nint16      xMin;
	 nint16      yMin;
	 nint16      xMax;
	 nint16      yMax;
} nrectmm16;
#elif defined( N_PLAT_MAC )
typedef struct tagnrectmm16
{
	 nint16      yMin;
	 nint16      xMin;
	 nint16      yMax;
	 nint16      xMax;
} nrectmm16;
#elif defined( N_PLAT_OS2 )
typedef struct tagnrectmm16
{
	 nint16      xMin;
	 nint16      yMax;
	 nint16      xMax;
	 nint16      yMin;
} nrectmm16;
#endif

#if defined( N_PLAT_MSW ) || defined( N_PLAT_UNIX ) || defined( N_PLAT_DOS ) || defined( N_PLAT_NLM )
typedef struct tagnrectmm32
{
	 nint32      xMin;
	 nint32      yMin;
	 nint32      xMax;
	 nint32      yMax;
} nrectmm32;
#elif defined( N_PLAT_MAC )
typedef struct tagnrectmm32
{
	 nint32      yMin;
	 nint32      xMin;
	 nint32      yMax;
	 nint32      xMax;
} nrectmm32;
#elif defined( N_PLAT_OS2 )
typedef struct tagnrectmm32
{
	 nint32      xMin;
	 nint32      yMax;
	 nint32      xMax;
	 nint32      yMin;
} nrectmm32;
#endif

#if defined ( N_PLAT_OS2 )
#pragma pack()  /* restore packing to default */
#endif  /* #if defined ( N_PLAT_OS2 ) */

/*
    Pointers to structures.
*/

typedef npt16       N_FAR * pnpt16;
typedef npt32       N_FAR * pnpt32;
typedef npthv16 N_FAR * pnpthv16;
typedef npthv32 N_FAR * pnpthv32;
typedef nrect16 N_FAR * pnrect16;
typedef nrect32 N_FAR * pnrect32;
typedef nrectmm16   N_FAR * pnrectmm16;
typedef nrectmm32   N_FAR * pnrectmm32;

#endif /* #if ! defined(N_RC_INVOKED) */

#if ! defined(N_RC_INVOKED)

#if defined( N_PLAT_MSW )

typedef unsigned char N_HUGE *   neptr;

#elif defined( N_PLAT_MAC )

typedef unsigned char *     neptr;

#elif defined( N_PLAT_UNIX )

typedef unsigned char *     neptr;

#elif defined( N_PLAT_OS2 )

typedef unsigned char _huge *   neptr;

#elif defined( N_PLAT_DOS )

typedef unsigned char _huge *   neptr;

#elif defined( N_PLAT_NLM ) 
                           /* We just need to include NLM in our thinking */
typedef unsigned char *     neptr;

#else
    HELP SYSTEM_ARCHITECTURE_IS_UNDEFINED;
#endif   /* MSW or MAC or UNIX or OS2 undefined */

#endif /* #if ! defined(N_RC_INVOKED) */

/*
------------------------------------------------------------------------------
Basic Derived Types:

Define the basic derived type definitions
------------------------------------------------------------------------------
*/

#if ! defined(N_RC_INVOKED)

/*
    Pointers to scalars.
*/

typedef neptr       N_FAR * pneptr;

#endif  /* ! defined(N_RC_INVOKED) */

/*
------------------------------------------------------------------------------
Macros:

------------------------------------------------------------------------------
*/

#if ! defined(N_RC_INVOKED)

#if defined( __cplusplus )

 /* Special 'extern' for C++ to avoid name mangling... */

#define _N_EXTERN           extern "C"

#else
#define _N_EXTERN           extern
#endif /* __cplusplus */

#define N_UNUSED_VAR( x )

#define N_REG1              register
#define N_REG2              register
#define N_REG3              register
#define N_REG4              register

#define N_STATIC_VAR        static
#define N_INTERN_VAR        static
#define N_GLOBAL_VAR
#define N_EXTERN_VAR        extern

#if defined( N_PLAT_MSW )
#define N_INTERN_FUNC( retType )                static retType N_NEAR
#define N_INTERN_FUNC_C( retType )          static retType N_NEAR
#define N_INTERN_FUNC_PAS( retType )        static retType N_NEAR N_PASCAL
#define N_GLOBAL_FUNC( retType )                retType
#define N_EXTERN_FUNC( retType )                _N_EXTERN retType
#define N_GLOBAL_FUNC_C( retType )          retType
#define N_EXTERN_FUNC_C( retType )          _N_EXTERN retType
#define N_GLOBAL_FUNC_PAS( retType )        retType N_PASCAL
#define N_EXTERN_FUNC_PAS( retType )        _N_EXTERN retType N_PASCAL
#elif defined( N_PLAT_MAC )
#define N_INTERN_FUNC( retType )         static retType
#define N_INTERN_FUNC_C( retType )       static retType
#define N_INTERN_FUNC_PAS( retType )     static pascal retType
#define N_GLOBAL_FUNC( retType )         retType
#define N_EXTERN_FUNC( retType )         _N_EXTERN retType
#define N_GLOBAL_FUNC_C( retType )       retType
#define N_EXTERN_FUNC_C( retType )       _N_EXTERN retType
#define N_GLOBAL_FUNC_PAS( retType )     pascal retType
#define N_EXTERN_FUNC_PAS( retType )     _N_EXTERN pascal retType
#elif defined( N_PLAT_UNIX )
#define N_INTERN_FUNC( retType )                static retType
#define N_INTERN_FUNC_C( retType )          static retType
#define N_INTERN_FUNC_PAS( retType )        static retType
#define N_GLOBAL_FUNC( retType )                retType
#define N_EXTERN_FUNC( retType )                _N_EXTERN retType
#define N_GLOBAL_FUNC_C( retType )          retType
#define N_EXTERN_FUNC_C( retType )          _N_EXTERN retType
#define N_GLOBAL_FUNC_PAS( retType )        retType
#define N_EXTERN_FUNC_PAS( retType )        _N_EXTERN retType
#elif defined( N_PLAT_OS2 )
#define _N_PASCAL           _Pascal
#define N_INTERN_FUNC( retType )                static retType
#define N_INTERN_FUNC_C( retType )          static retType
#define N_INTERN_FUNC_PAS( retType )        static retType  _N_PASCAL
#define N_GLOBAL_FUNC( retType )                retType
#define N_EXTERN_FUNC( retType )                _N_EXTERN retType
#define N_GLOBAL_FUNC_C( retType )          retType
#define N_EXTERN_FUNC_C( retType )          _N_EXTERN retType
#define N_GLOBAL_FUNC_PAS( retType )        retType _N_PASCAL
#define N_EXTERN_FUNC_PAS( retType )        _N_EXTERN retType  _N_PASCAL
#elif defined( N_PLAT_DOS )
#define N_INTERN_FUNC( retType )                static retType N_NEAR
#define N_INTERN_FUNC_C( retType )          static retType N_NEAR
#define N_INTERN_FUNC_PAS( retType )        static retType N_NEAR PASCAL
#define N_GLOBAL_FUNC( retType )                retType
#define N_EXTERN_FUNC( retType )                _N_EXTERN retType
#define N_GLOBAL_FUNC_C( retType )          retType
#define N_EXTERN_FUNC_C( retType )          _N_EXTERN retType
#define N_GLOBAL_FUNC_PAS( retType )        retType PASCAL
#define N_EXTERN_FUNC_PAS( retType )        _N_EXTERN retType PASCAL
#elif defined( N_PLAT_NLM )
#define N_INTERN_FUNC( retType )                static retType
#define N_INTERN_FUNC_C( retType )          static retType
#define N_INTERN_FUNC_PAS( retType )        static retType
#define N_GLOBAL_FUNC( retType )                retType
#define N_EXTERN_FUNC( retType )                _N_EXTERN retType
#define N_GLOBAL_FUNC_C( retType )          retType
#define N_EXTERN_FUNC_C( retType )          _N_EXTERN retType
#define N_GLOBAL_FUNC_PAS( retType )        retType
#define N_EXTERN_FUNC_PAS( retType )        _N_EXTERN retType
#endif

#if ! defined( N_PLAT_MAC )
#define N_GLOBAL_LIBRARY( retType )       retType N_API
#define N_EXTERN_LIBRARY( retType )       _N_EXTERN retType N_API
#define N_GLOBAL_LIBRARY_C( retType )     retType N_API_VARARGS
#define N_EXTERN_LIBRARY_C( retType )     _N_EXTERN retType N_API_VARARGS
#else
#define N_GLOBAL_LIBRARY( retType )      pascal retType
#define N_EXTERN_LIBRARY( retType )      _N_EXTERN pascal retType
#define N_GLOBAL_LIBRARY_C( retType )    retType
#define N_EXTERN_LIBRARY_C( retType )    _N_EXTERN retType
#endif

#if defined( N_PLAT_MSW )

#define N_GLOBAL_LIBRARY_PAS( retType ) retType N_FAR N_PASCAL
#define N_EXTERN_LIBRARY_PAS( retType ) _N_EXTERN retType N_FAR N_PASCAL

#define N_GLOBAL_CALLBACK( retType )        retType N_FAR N_PASCAL N_EXPORT
#define N_EXTERN_CALLBACK( retType )        _N_EXTERN retType N_FAR N_PASCAL \
                                            N_EXPORT
#define N_GLOBAL_CALLBACK_C( retType )      retType N_FAR __cdecl N_EXPORT
#define N_EXTERN_CALLBACK_C( retType )      _N_EXTERN retType N_FAR __cdecl \
                                            N_EXPORT
#define N_GLOBAL_CALLBACK_PAS( retType )    retType N_FAR N_PASCAL N_EXPORT
#define N_EXTERN_CALLBACK_PAS( retType )    _N_EXTERN retType N_FAR N_PASCAL \
                                            N_EXPORT

#define N_TYPEDEF_FUNC( retType, typeName ) \
                typedef retType (*typeName)

#define N_TYPEDEF_FUNC_C( retType, typeName ) \
                typedef retType (*typeName)

#define N_TYPEDEF_FUNC_PAS( retType, typeName ) \
                typedef retType (N_PASCAL *typeName)

#define N_TYPEDEF_LIBRARY( retType, typeName ) \
                typedef retType (N_FAR N_PASCAL *typeName)

#define N_TYPEDEF_LIBRARY_C( retType, typeName ) \
                typedef retType (N_FAR __cdecl *typeName)

#define N_TYPEDEF_LIBRARY_PAS( retType, typeName ) \
                typedef retType (N_FAR N_PASCAL *typeName)

#define N_TYPEDEF_CALLBACK( retType, typeName ) \
                typedef retType (N_FAR N_PASCAL N_EXPORT *typeName)

#define N_TYPEDEF_CALLBACK_C( retType, typeName ) \
                typedef retType (N_FAR __cdecl N_EXPORT *typeName)

#define N_TYPEDEF_CALLBACK_PAS( retType, typeName ) \
                typedef retType (N_FAR N_PASCAL N_EXPORT *typeName)

#define N_TYPEDEF_INTERN_FUNC( retType, typeName ) \
                typedef retType (N_NEAR *typeName)

#define N_TYPEDEF_INTERN_FUNC_C( retType, typeName ) \
                typedef retType (N_NEAR *typeName)

#define N_TYPEDEF_INTERN_FUNC_PAS( retType, typeName ) \
                typedef retType (N_NEAR N_PASCAL *typeName)

#elif defined( N_PLAT_MAC )

#define N_GLOBAL_LIBRARY_PAS( retType )  pascal retType
#define N_EXTERN_LIBRARY_PAS( retType )  _N_EXTERN pascal retType

#define N_GLOBAL_CALLBACK( retType )     pascal retType
#define N_EXTERN_CALLBACK( retType )     _N_EXTERN pascal retType
#define N_GLOBAL_CALLBACK_C( retType )   retType
#define N_EXTERN_CALLBACK_C( retType )   _N_EXTERN retType
#define N_GLOBAL_CALLBACK_PAS( retType ) pascal retType
#define N_EXTERN_CALLBACK_PAS( retType ) _N_EXTERN pascal retType

#define N_TYPEDEF_FUNC( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_FUNC_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_FUNC_PAS( retType, typeName ) \
                    typedef pascal retType (*typeName)
#define N_TYPEDEF_LIBRARY( retType, typeName ) \
						  typedef pascal retType (*typeName)
#define N_TYPEDEF_LIBRARY_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_LIBRARY_PAS( retType, typeName ) \
                    typedef pascal retType (*typeName)
#define N_TYPEDEF_CALLBACK( retType, typeName ) \
                    typedef pascal retType (*typeName)
#define N_TYPEDEF_CALLBACK_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_CALLBACK_PAS( retType, typeName ) \
                    typedef pascal retType (*typeName)
#define N_TYPEDEF_INTERN_FUNC( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_INTERN_FUNC_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_INTERN_FUNC_PAS( retType, typeName ) \
                    typedef pascal retType (*typeName)

#elif defined( N_PLAT_UNIX )

#define N_GLOBAL_LIBRARY_PAS( retType ) retType
#define N_EXTERN_LIBRARY_PAS( retType ) _N_EXTERN retType

#define N_GLOBAL_CALLBACK( retType )        retType
#define N_EXTERN_CALLBACK( retType )        _N_EXTERN retType
#define N_GLOBAL_CALLBACK_C( retType )      retType
#define N_EXTERN_CALLBACK_C( retType )      _N_EXTERN retType
#define N_GLOBAL_CALLBACK_PAS( retType )    retType
#define N_EXTERN_CALLBACK_PAS( retType )    _N_EXTERN retType

#define N_TYPEDEF_FUNC( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_FUNC_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_FUNC_PAS( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_LIBRARY( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_LIBRARY_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_LIBRARY_PAS( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_CALLBACK( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_CALLBACK_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_CALLBACK_PAS( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_INTERN_FUNC( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_INTERN_FUNC_C( retType, typeName ) \
                    typedef retType (*typeName)
#define N_TYPEDEF_INTERN_FUNC_PAS( retType, typeName ) \
                    typedef retType (*typeName)

#elif defined( N_PLAT_OS2)

#define N_PRAGMA_ONCE
#define _N_PASCAL           _Pascal

#define N_GLOBAL_LIBRARY_PAS( retType )               retType  _N_PASCAL
#define N_EXTERN_LIBRARY_PAS( retType )     _N_EXTERN retType  _N_PASCAL

#define N_GLOBAL_CALLBACK( retType )                  retType  _N_PASCAL  _export
#define N_EXTERN_CALLBACK( retType )        _N_EXTERN retType  _N_PASCAL
#define N_GLOBAL_CALLBACK_NATIVE( retType )           retType  EXPENTRY   _export
#define N_EXTERN_CALLBACK_NATIVE( retType ) _N_EXTERN retType  EXPENTRY
#define N_GLOBAL_CALLBACK_C( retType )                retType             _export
#define N_EXTERN_CALLBACK_C( retType )      _N_EXTERN retType
#define N_GLOBAL_CALLBACK_PAS( retType )              retType  _N_PASCAL  _export
#define N_EXTERN_CALLBACK_PAS( retType )    _N_EXTERN retType  _N_PASCAL

#define N_TYPEDEF_FUNC( retType, typeName ) \
                typedef retType (*typeName)

#define N_TYPEDEF_FUNC_C( retType, typeName ) \
                typedef retType (*typeName)

#define N_TYPEDEF_FUNC_PAS( retType, typeName ) \
                typedef retType (* _N_PASCAL typeName)

#define N_TYPEDEF_LIBRARY( retType, typeName ) \
                typedef retType ( * _N_PASCAL typeName)

#define N_TYPEDEF_LIBRARY_C( retType, typeName ) \
                typedef retType ( *typeName)

#define N_TYPEDEF_LIBRARY_PAS( retType, typeName ) \
                typedef retType (*  _N_PASCAL typeName)

#define N_TYPEDEF_CALLBACK( retType, typeName ) \
                typedef  retType (  _export * _N_PASCAL typeName)

#define N_TYPEDEF_CALLBACK_NATIVE( retType, typeName ) \
                typedef  retType (  _export * EXPENTRY  typeName)

#define N_TYPEDEF_CALLBACK_C( retType, typeName ) \
                typedef retType ( _export *typeName)

#define N_TYPEDEF_CALLBACK_PAS( retType, typeName ) \
                typedef retType ( _export  *  _N_PASCAL typeName)

#define N_TYPEDEF_INTERN_FUNC( retType, typeName ) \
                typedef retType ( *typeName)

#define N_TYPEDEF_INTERN_FUNC_C( retType, typeName ) \
                typedef retType ( *typeName)

#define N_TYPEDEF_INTERN_FUNC_PAS( retType, typeName ) \
                typedef retType (* _N_PASCAL typeName)

#elif defined( N_PLAT_DOS )

#define N_GLOBAL_LIBRARY_PAS( retType ) retType N_FAR PASCAL
#define N_EXTERN_LIBRARY_PAS( retType ) _N_EXTERN retType N_FAR PASCAL

#define N_GLOBAL_CALLBACK( retType )        retType N_FAR PASCAL _export
#define N_EXTERN_CALLBACK( retType )        _N_EXTERN retType N_FAR PASCAL \
                                            _export
#define N_GLOBAL_CALLBACK_C( retType )      retType N_FAR _export
#define N_EXTERN_CALLBACK_C( retType )      _N_EXTERN retType N_FAR _export
#define N_GLOBAL_CALLBACK_PAS( retType )    retType N_FAR PASCAL _export
#define N_EXTERN_CALLBACK_PAS( retType )    _N_EXTERN retType N_FAR PASCAL \
                                            _export

#define N_TYPEDEF_FUNC( retType, typeName ) \
            typedef retType (*typeName)

#define N_TYPEDEF_FUNC_C( retType, typeName ) \
            typedef retType (*typeName)

#define N_TYPEDEF_FUNC_PAS( retType, typeName ) \
            typedef retType (PASCAL *typeName)

#define N_TYPEDEF_LIBRARY( retType, typeName ) \
            typedef retType (N_FAR PASCAL *typeName)

#define N_TYPEDEF_LIBRARY_C( retType, typeName ) \
            typedef retType (N_FAR *typeName)

#define N_TYPEDEF_LIBRARY_PAS( retType, typeName ) \
            typedef retType (N_FAR PASCAL *typeName)

#define N_TYPEDEF_CALLBACK( retType, typeName ) \
            typedef retType (N_FAR PASCAL _export *typeName)

#define N_TYPEDEF_CALLBACK_C( retType, typeName ) \
            typedef retType (N_FAR _export *typeName)

#define N_TYPEDEF_CALLBACK_PAS( retType, typeName ) \
            typedef retType (N_FAR PASCAL _export *typeName)

#define N_TYPEDEF_INTERN_FUNC( retType, typeName ) \
            typedef retType (N_NEAR *typeName)

#define N_TYPEDEF_INTERN_FUNC_C( retType, typeName ) \
            typedef retType (N_NEAR *typeName)

#define N_TYPEDEF_INTERN_FUNC_PAS( retType, typeName ) \
            typedef retType (N_NEAR PASCAL *typeName)

#endif /* N_PLAT_DOS */

/*
    Untyped logical macros.
*/

#define NMin(a,b)               ((a) < (b) ? (a) : (b))
#define NMax(a,b)               ((a) > (b) ? (a) : (b))

#endif /* ! defined( N_RC_INVOKED ) */

/*
    Integer construction macros.
*/

#if ! defined( N_RC_INVOKED )

/* NMakePtrParam is defined here since it can't be used in a resource file */

#define NMakePtrParam(p)    ((nparam) ((nptr) (p)))

#endif /* not N_RC_INVOKED */

#define NMake32(a,b)       ((nuint32) (((nuint16)(a)) | \
                                    (((nuint32)(nuint16)(b)) << 16)))
#define NGetLo16(a32)      ((nuint16)((nuint32)(a32) & 0xFFFF))
#define NGetHi16(a32)      ((nuint16)((nuint32)(a32) >> 16))
#define NMake16(a,b)       ((nuint16) (((nuint8)(a)) | \
                           (((nuint16)((nuint8)(b))) << 8)))
#define NGetLo8(a16)       ((nuint8)((nuint16)(a16) & 0xFF))
#define NGetHi8(a16)       ((nuint8)((nuint16)(a16) >> 8))


#define NMakePair32         NMake32
#define NGetFirst16(a)      ((nint16) NGetLo16(a))
#define NGetSecond16(a)     ((nint16) NGetHi16(a))

#define NMakePair16         NMake16
#define NGetFirst8(a)       ((nint8) NGetLo8(a))
#define NGetSecond8(a)      ((nint8) NGetHi8(a))

#define NMakeFixed32(i,f)   NMake32(f,i)
#define NGetFixedInt16(a)   ((nint16) NGetHi16(a))
#define NGetFixedFrac16(a)  ((nint16) NGetLo16(a))

/* Swap, Copy, Get and Set macros */

#define NSwap32( x ) ((nuint32) ( \
         (((nuint32)(x) & 0x000000FFL) << 24) | \
         (((nuint32)(x) & 0x0000FF00L) <<  8) | \
         (((nuint32)(x) & 0x00FF0000L) >>  8) | \
         (((nuint32)(x) & 0xFF000000L) >> 24) ))

#define NSwap16( x ) ((nuint16) ( \
         (((nuint16)(x) & 0x00FF) << 8) | \
         (((nuint16)(x) & 0xFF00) >> 8) ))

#define NCopySwap32( pDest, pSrc )  \
      (((pnuint8) (pDest))[0] = ((pnuint8) (pSrc))[3], \
       ((pnuint8) (pDest))[1] = ((pnuint8) (pSrc))[2], \
       ((pnuint8) (pDest))[2] = ((pnuint8) (pSrc))[1], \
       ((pnuint8) (pDest))[3] = ((pnuint8) (pSrc))[0])

#define NCopySwap16( pDest, pSrc )  \
      (((pnuint8) (pDest))[0] = ((pnuint8) (pSrc))[1], \
       ((pnuint8) (pDest))[1] = ((pnuint8) (pSrc))[0])

#if defined( N_INT_ENDIAN_HI_LO )

   #define NSwapHiLo32( x )      ((nuint32) (x))
   #define NSwapHiLo16( x )      ((nuint16) (x))
   #define NSwapLoHi32( x )      NSwap32( x )
   #define NSwapLoHi16( x )      NSwap16( x )

#else

   #define NSwapLoHi32( x )      ((nuint32) (x))
   #define NSwapLoHi16( x )      ((nuint16) (x))
   #define NSwapHiLo32( x )      NSwap32( x )
   #define NSwapHiLo16( x )      NSwap16( x )

#endif

#if defined( N_INT_STRICT_ALIGNMENT )

   #define NCopy32( pDest, pSrc )   \
         (((pnuint8) (pDest))[0] = ((pnuint8) (pSrc))[0], \
          ((pnuint8) (pDest))[1] = ((pnuint8) (pSrc))[1], \
          ((pnuint8) (pDest))[2] = ((pnuint8) (pSrc))[2], \
          ((pnuint8) (pDest))[3] = ((pnuint8) (pSrc))[3])

   #define NCopy16( pDest, pSrc )   \
         (((pnuint8) (pDest))[0] = ((pnuint8) (pSrc))[0], \
          ((pnuint8) (pDest))[1] = ((pnuint8) (pSrc))[1])

   #define NSet32( pDest, src) \
         (((pnuint8) (pDest))[0] = ((pnuint8) (&src))[0], \
          ((pnuint8) (pDest))[1] = ((pnuint8) (&src))[1], \
          ((pnuint8) (pDest))[2] = ((pnuint8) (&src))[2], \
          ((pnuint8) (pDest))[3] = ((pnuint8) (&src))[3])


   #define NSet16( pDest, src) \
         (((pnuint8) (pDest))[0] = ((pnuint8) (&src))[0], \
          ((pnuint8) (pDest))[1] = ((pnuint8) (&src))[1])

   #define NGet32( pSrc ) \
         (((pnuint8) (pSrc))[0]         ||  \
          (((pnuint8) (pSrc))[1] << 8)  ||  \
          (((pnuint8) (pSrc))[2] << 16) ||  \
          (((pnuint8) (pSrc))[3] << 24))

   #define NGet16( pSrc ) \
         (((pnuint8) (pSrc))[0]         ||  \
          (((pnuint8) (pSrc))[1] << 8))

#else

   #define NCopy32( pDest, pSrc )   \
         (*((pnuint32) (pDest)) = *((pnuint32) (pSrc)))

   #define NCopy16( pDest, pSrc )   \
         (*((pnuint16) (pDest)) = *((pnuint16) (pSrc)))

   #define NSet32( pDest, src) \
         (*((pnuint32) (pDest)) = src)

   #define NSet16( pDest, src) \
         (*((pnuint16) (pDest)) = src)

   #define NGet32( pSrc ) \
         (*((pnuint32) (pSrc)))

   #define NGet16( pSrc ) \
         (*((pnuint16) (pSrc)))

#endif

#if defined( N_INT_ENDIAN_HI_LO )

   #define NCopyHiLo32        NCopy32
   #define NCopyHiLo16        NCopy16
   #define NCopyLoHi32        NCopySwap32
   #define NCopyLoHi16        NCopySwap16

#else

   #define NCopyHiLo32        NCopySwap32
   #define NCopyHiLo16        NCopySwap16
   #define NCopyLoHi32        NCopy32
   #define NCopyLoHi16        NCopy16

#endif


#define NPad32( s )           ((s + 3) & ~3)
#define NPad16( s )           (((s) + 1) & ~1)

#define NAlign32(s)  *(s) = (void N_FAR *) (((nuint32)*(s) + 3) & ~3);

#define NSetPtXY( pptPoint, x, y )	\
	(( pptPoint )->x = ( x ), \
	 ( pptPoint )->y = ( y ))

#define NSetPtYX( pptPoint, y, x )	\
	(( pptPoint )->x = ( x ), \
	 ( pptPoint )->y = ( y ))

#define NSetRectXY( prtRect, left, top, right, bottom )	\
	(( prtRect )->left = ( left ), \
	 ( prtRect )->top = ( top ),	\
	 ( prtRect )->right = ( right ), \
	 ( prtRect )->bottom = ( bottom ))

#define NSetRectYX( prtRect, top, left, bottom, right )	\
	(( prtRect )->left = ( left ), \
	 ( prtRect )->top = ( top ),	\
	 ( prtRect )->right = ( right ), \
	 ( prtRect )->bottom = ( bottom ))

#define NSetRectXYSize( prtRect, left, top, width, height )	\
	(( prtRect )->left = ( left ), \
	 ( prtRect )->top = ( top ),	\
	 ( prtRect )->right = (( left )+( width )), \
	 ( prtRect )->bottom = (( top )+( height )))

#define NSetRectYXSize( prtRect, top, left, height, width )	\
	(( prtRect )->left = ( left ), \
	 ( prtRect )->top = ( top ),	\
	 ( prtRect )->right = (( left )+( width )), \
	 ( prtRect )->bottom = (( top )+( height )))

#define NCopyPt( pptTo, pptFrom ) \
    ((pptTo)->x = (pptFrom)->x, \
    (pptTo)->y = (pptFrom)->y)

#define NCopyRect( prtTo, prtFrom ) \
    ((prtTo)->left = (prtFrom)->left, \
    (prtTo)->top = (prtFrom)->top, \
    (prtTo)->right = (prtFrom)->right, \
    (prtTo)->bottom = (prtFrom)->bottom)

#define NCopyRect32( prtlTo, prtFrom ) \
    ((prtlTo)->left = (nint32) (prtFrom)->left, \
    (prtlTo)->top = (nint32) (prtFrom)->top, \
    (prtlTo)->right = (nint32) (prtFrom)->right, \
    (prtlTo)->bottom = (nint32) (prtFrom)->bottom)

#define NCopyRect16( prtTo, prtlFrom ) \
    ((prtTo)->left = (nint16) (prtlFrom)->left, \
    (prtTo)->top = (nint16) (prtlFrom)->top, \
    (prtTo)->right = (nint16) (prtlFrom)->right, \
    (prtTo)->bottom = (nint16) (prtlFrom)->bottom)

#define NCopyPt32( pptlTo, pptFrom ) \
    ((pptlTo)->x = (nint32) (pptFrom)->x, \
    (pptlTo)->y = (nint32) (pptFrom)->y)

#define NCopyPt16( pptTo, pptlFrom ) \
    ((pptTo)->x = (nint16) (pptlFrom)->x, \
    (pptTo)->y = (nint16) (pptlFrom)->y)

/*
    Character and String macros.
*/

#if defined(N_USE_STR_16)
    #define NText(a)        L a
#else
    #define NText(a)        a
#endif

#if defined( N_PLAT_MSW ) || defined( N_PLAT_OS2 ) || defined( N_PLAT_DOS )
    #define N_NEWLINE       NText( "\r\n" )
#elif defined( N_PLAT_MAC )
	#if defined( THINK_C ) || defined( METROWERKS_C )
		#define N_NEWLINE		NText( "\r" )
	#else
		#define N_NEWLINE		NText( "\n" )
	#endif
#else
    #define N_NEWLINE       NText( "\n" )
#endif

/*
    Historical macro synonyms.
*/

#if ! defined( N_INC_NO_OLD_MACROS ) 
    #if !defined( FAR )
        #define FAR         N_FAR
    #endif
    #if !defined( NEAR )
        #define NEAR        N_NEAR
    #endif
    #if !defined( MIN )
        #define MIN         NMin
    #endif
    #if !defined( MAX )
        #define MAX         NMax
    #endif
    #if !defined( MAKELONG )
        #define MAKELONG    NMake32
    #endif
    #if !defined( HIWORD )
        #define HIWORD      NGetHi16
    #endif
    #if !defined( LOWORD )
        #define LOWORD      NGetLo16
    #endif
    #if !defined( MAKEWORD )
        #define MAKEWORD    NMake16
    #endif
    #if !defined( HIBYTE )
        #define HIBYTE      NGetHi8
    #endif
    #if !defined( LOBYTE )
        #define LOBYTE      NGetLo8
    #endif
#endif

#endif /* ! defined( NTYPES_H ) */
