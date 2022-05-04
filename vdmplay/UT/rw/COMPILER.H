#ifndef __RWCOMPILER_H__
#define __RWCOMPILER_H__ 1

/*
 * Compiler and system related foibles and directives
 *
 * $Id: compiler.in,v 6.18 1994/07/18 03:07:36 jims Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 * Voice: (503) 754-3010	FAX: (503) 757-6650
 *
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 Rogue Wave Software, Inc.
 * ALL RIGHTS RESERVED
 *
 * The software and information contained herein are proprietary to, and
 * comprise valuable trade secrets of, Rogue Wave Software, Inc., which
 * intends to preserve as trade secrets such software and information.
 * This software is furnished pursuant to a written license agreement and
 * may be used, copied, transmitted, and stored only in accordance with
 * the terms of such license and with the inclusion of the above copyright
 * notice.  This software and information or any other copies thereof may
 * not be provided or otherwise made available to any other person.
 *
 * Notwithstanding any other lease or license that may pertain to, or
 * accompany the delivery of, this computer software and information, the
 * rights of the Government regarding its use, reproduction and disclosure
 * are as set forth in Section 52.227-19 of the FARS Computer
 * Software-Restricted Rights clause.
 * 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause at DFARS 52.227-7013.
 * 
 * This computer software and information is distributed with "restricted
 * rights."  Use, duplication or disclosure is subject to restrictions as
 * set forth in NASA FAR SUP 18-52.227-79 (April 1985) "Commercial
 * Computer Software-Restricted Rights (April 1985)."  If the Clause at
 * 18-52.227-74 "Rights in Data General" is specified in the contract,
 * then the "Alternate III" clause applies.
 *
 ***************************************************************************
 *
 *
 */


/****************************************************************
 ****************************************************************
 *								*
 *		U S E R   T U N A B L E   S E C T I O N		*
 *								*
 ****************************************************************
 ****************************************************************/

/*
 * This section has various preprocessor constants that can
 * be set to reflect the properties of your compiler.  For most
 * compilers (particularly, MS-DOS compilers) there is no need
 * to do anything --- most settings can be autodetected.
 *
 * For many Unix compilers you may have to tune the settings below.
 * This is most easily done by running the "config" shell script
 * which will try various test programs to discover the properties
 * of your compiler.
 *
 *       THIS IS FAR EASIER THAN SETTING THESE BY HAND!
 */




/*
 *                   AT&T "CFRONT" USERS
 */

/* 
 * Most compilers have a built in "manifest constant".
 * For the following compilers you must supply one by uncommenting
 * an appropriate line:
 *
 *   AT&T cfront V2.X:       __ATT2__
 *   AT&T cfront V3.0:       __ATT3__
 */

/* #define __ATT2__ 1 */
/* #define __ATT3__ 1 */




/*
 *                    GLOCKENSPIEL USERS
 *
 * IN ADDITION to defining __ATT2__ or __ATT3__, you must
 * uncomment the following line:
 *
 */ 

/* #define __GLOCK__ 1 */




/**
 **                     *** ALL USERS ***
 **/


/******************** COMPILER WORD SIZES, ETC ********************/

/*
 * Uncomment the following and set to the number of bytes per
 * computer word.  If you do nothing, the default will be 4.
 */

/* #define RW_BYTES_PER_WORD 4  */


/*
 * Uncomment the following and set to the pointer size in bytes.
 * If you do nothing, the default will be 4.
 */

/* #define RW_BYTES_PER_PTR 4 */

/*
 * Uncomment the following and set to the number of decimal digits
 * of precision for type double.
 * If you do nothing, the default will be 16.
 */

/* #define RW_DEFAULT_PRECISION 16 */


/*************** COMPILER QUIRKS AND LIMITATIONS ******************/

/*
 * Uncomment the following line if your compiler has old V2.0 style
 * nested enums (an enum declared inside a class has global scope,
 * not class scope).
 */

/* #define RW_GLOBAL_ENUMS 1 */


/*
 * Uncomment the following if your "C" (not C++) compiler understands 
 * K&R style prototypes only.  The most common situation is Unix machines
 * using non-ANSI "C" compilers.
 */

/* #define RW_KR_ONLY 1 */


/*
 * Uncomment the following if your compiler has trouble with
 * base class access adjustments using the "::baseFunctionName;" 
 * construct (early versions of cfront ca. Summer '89). These
 * are rare.
 */

/* #define RW_NO_ACCESS_ADJUSTMENT 1 */


/*
 * Uncomment the following if your compiler cannot overload
 * member functions on a const "this" pointer (relatively rare).
 */

/* #define RW_NO_CONST_OVERLOAD 1 */


/*
 * Uncomment the following if your compiler does not support
 * exceptions.
 */

/* #define RW_NO_EXCEPTIONS 1 */


/*
 * Uncomment the following if your compiler does not support
 * throwing of exceptions from a shared library.
 */

/* #define RW_NO_THROW_WITH_SHARED 1 */


/*
 * Uncomment the following if your compiler does not support xmsg
 * and xalloc exception classes.
 */

/* #define RW_NO_XMSG 1 */


/*
 * Uncomment the following if your compiler cannot differentiate
 * between a formal argument of type "char" and "unsigned char"
 * (early Oregon compilers).
 */

/* #define RW_NO_OVERLOAD_UCHAR 1 */


/*
 * Uncomment the following if your compiler does not support
 * "signed char" (V2.0 cfront with non-ANSI backends)
 */

/* #define RW_NO_SCHAR 1 */


/*
 * Uncomment the following if your compiler cannot differentiate
 * between a formal argument of type "char" and "signed char"
 */

/* #define RW_NO_OVERLOAD_SCHAR 1 */


/*
 * Uncomment the following if your compiler cannot differentiate between
 * the prefix and postfix increment operator (V2.0 cfront):
 */

/* #define RW_NO_POSTFIX 1 */


/*
 * Uncomment the following if your compiler supports inlined 80x86 assembly.
 */

/* #define RW_INLINE86_ASSEMBLY 1 */


/************************** PREPROCESSOR ********************************/

/*
 * Uncomment the following if your preprocessor does not detect
 * recursions properly.
 */

/* #define RW_NO_CPP_RECURSION 1 */


/*
 * Uncomment the following if your preprocessor does not understand 
 * nested quotes.
 */

/* #define RW_NO_NESTED_QUOTES 1 */


/*************************** TEMPLATES **********************************/

/*
 * Uncomment the following if your compiler does not understand templates
 * at all.
 */

/* #define RW_NO_TEMPLATES 1 */


/*
 * Uncomment the following if your compiler understands
 * only simple templates (that is, at the level of AT&T cfront V3.0).
 */

/* #define RW_BROKEN_TEMPLATES 1 */


/*
 * Uncomment the following if your compiler does template
 * instantiation at compile time.
 */

/* #define RW_COMPILE_INSTANTIATE 1 */


/*************************** STRINGS ****************************/

/*
 * Uncomment the following if your sprintf() does not
 * return the size of the buffer as an int, as ANSI C requires.
 */

/* #define RW_NO_ANSI_SPRINTF 1 */


/*
 * Uncomment the following if your compiler does not have the
 * function stricmp() (case-independent comparisons).
 */

/* #define RW_NO_STRICMP 1 */


/*
 * Uncomment the following if your compiler does not have the
 * function strnicmp() (limited case-independent comparisons).
 */

/* #define RW_NO_STRNICMP 1 */


/*
 * Uncomment the following if your compiler does not have the
 * ANSI C function strstr() (string search).
 */

/* #define RW_NO_STRSTR 1 */


/*
 * Uncomment the following if your compiler does not have the
 * ANSI C function memmove().
 */

/* #define RW_NO_MEMMOVE 1 */


/*
 * Uncomment the following if the prototype for function memcpy()
 * is found in <memory.h> on your machine, rather than the
 * ANSI-required <string.h>.
 */

/* #define RW_NON_ANSI_HEADERS 1 */



/****************** INTERNATIONALIZATION ************************/

/*
 * Uncomment the following if your compiler does not support
 * wide characters strings (e.g., functions wslen(), etc.).
 */

/* #define RW_NO_WSTR 1 */


/*
 * Uncomment the following if the type wchar_t is just a typedef,
 * instead of a distinct type.
 */

/* #define RW_NO_OVERLOAD_WCHAR 1 */


/*
 * Uncomment the following if your compiler does not support
 * the ANSI C locale facility fully, or if it does not support
 * it at all (in particular, uncomment if setlocale(), strxform(),
 * or strcoll() are not present or don't work).
 */

/* #define RW_NO_LOCALE 1 */

 
/*
 * Uncomment the following if your compiler does not have
 * the %C directive to strftime().
 */

/* #define RW_NO_STRFTIME_CAPC 1 */


#define RW_NOMSG    0x00
#define RW_CATGETS  0x01
#define RW_GETTEXT  0x02
#define RW_DGETTEXT 0x03

/*
 * Set RW_MESSAGE to the type of messaging facility you want:
 *   RW_NOMSG	 No messaging facility
 *   RW_CATGETS  Use catgets()
 *   RW_GETTEXT  Use gettext()
 *   RW_DGETTEXT Use dgettext()
 */

#define RW_MESSAGE RW_NOMSG

/************************** TIME ********************************/

/*
 * Uncomment the following if your compiler does not declare
 * the ANSI C function "clock()" in either <time.h> or <stdlib.h>.
 */

/* #define RW_NO_CLOCK 1 */


/*
 * Uncomment the following if your compiler does not have global
 * variables "_daylight", "_timezone", and "_tzname", or corresponding
 * variables without a leading underscore (generally
 * this is true only for pure Berkeley systems).
 */

/* #define RW_NO_GLOBAL_TZ 1 */


/*
 * Uncomment the following if your system supplies a global variable
 * named "daylight" instead of the nominally more correct "_daylight".
 */

/* #define RW_NO_LEADING_UNDERSCORE 1 */


/*
 * If your system does not have global variables "daylight" and
 * "timezone" (see directive immediately above) and does not have
 * the Berkeley function gettimeofday() either, then uncomment
 * the following:
 */

/* #define RW_NO_GETTIMEOFDAY 1 */


/*
 * If the struct tm defined in your <time.h> has extra member data
 * "tm_zone" and "tm_gmtoff" (this is true for SunOs 4.X), then you
 * should uncomment the following:
 */

/* #define RW_STRUCT_TM_TZ 1 */


/************************** STREAMS ********************************/

/*
 * If your system's iostream functions ios::xalloc() and ios::pword()
 * fail to initialize the xalloc()'d storage to 0, this prevents the
 * RWLocale::imbue(ios&) feature from working, so uncomment the following:
 */

/* #define RW_IOS_XALLOC_BROKEN 1 */


/*
 * Uncomment the following if your compiler / system does not support
 * XDR streams:
 */

/* #define RW_NO_XDR 1 */


/*************************************************************************
**************************************************************************
**									**
**		From here on, it's pretty much boilerplate		**
**		and rarely requires any tuning.				**
**									**
**************************************************************************
**************************************************************************/


/************************ Cfront derivatives ******************************/

/* Any of these defines a cfront style compiler: */
#if defined(__ATT1__) || defined(__ATT2__) || defined(__ATT3__)
#  define __ATT__ 1
#endif

#if defined(__ATT3__) && !defined(RW_BROKEN_TEMPLATES)
#  define RW_BROKEN_TEMPLATES 1
#endif

/****************** Various Glockenspiel foibles. ***********************/

#if defined(__GLOCK__)

   /* Glock has bug in base class access adjustments: */
#  ifndef RW_NO_ACCESS_ADJUSTMENT
#    define RW_NO_ACCESS_ADJUSTMENT 1
#  endif

   /* Glock emits "const structs", then attempts to assign to them: */
#  define RW_CONST_EMIT_BUG 1

   /*
    * Glockenspiel was too lazy to provide type-safe linkage include
    * files, preferring to use the backend C compiler include files.
    * These require a "C" wrapper:
    */
#  ifdef __cplusplus
#    define STARTWRAP	extern "C" {
#    define ENDWRAP	}
#  endif

#else	/* Not Glock */

#  define STARTWRAP
#  define ENDWRAP

#endif

/*************************** Symantec *******************************/

/*
 * No longer supports Zortech.  Must have Symantec V6.0 or greater.
 */
#if defined(__SC__)
/*************************** For Macs *******************************/
#  if defined(macintosh)
#    define RW_CRLF_CONVENTION       1
#    define RW_NO_EXCEPTIONS         1
#    define RW_NO_GETTIMEOFDAY       1
#    define RW_NO_FRIEND_INLINE_DECL 1
#    define RW_NO_GLOBAL_TZ          1
#    define RW_NO_LOCALE             1
#    define RW_NO_OVERLOAD_WCHAR     1
#    define RW_NO_STRICMP            1
#    define RW_NO_TEMPLATES          1
#    define RW_NO_XDR                1
#    define RW_NO_XMSG               1
#    define RW_NO_WSTR               1
/*************************** For PC's *******************************/
#  else
#    define __MSDOS__                1
#    define RW_COMPILE_INSTANTIATE   1
#    define RW_NO_XMSG               1

#    if (__SC__ <= 0x610)
#      define RW_NO_EXCEPTIONS       1
#      define RW_NO_GETTIMEOFDAY     1
#      define RW_NO_GLOBAL_TZ        1
#      define RW_NO_OVERLOAD_WCHAR   1
#      define RW_NO_WSTR             1
#    endif
#  endif
#endif

/********************** Borland's Turbo C++ **************************/

#if defined(__TURBOC__)

#  if defined(__MSDOS__) && defined(_Windows)
#    define __WIN16__ 1
#  endif

/* 
 * For Borland, the __export keyword in a function declaration must 
 * come after the return type: 
 */
#  define RW_TRAILING_RWEXPORT 1
   /*
    * Turbo and Borland won't inline code that contains
    * "while" or "for" loops or that generates temporaries
    * requiring destructors:
    */
#  define RW_NO_INLINED_WHILES           1
#  define RW_NO_INLINED_FORS             1
#  define RW_NO_INLINED_TEMP_DESTRUCTORS 1

#  define RW_NO_LEADING_UNDERSCORE       1

   /* The Borland compilers left out this definition: */
#  define name2 _Paste2

   /* Turbo C++ V1.00 forgets the segment address when passing
      a class as a far reference if the class has not been defined. */
#  if __TURBOC__ <= 0x0295
#    define RW_UNDEFINED_REFERENCE_BUG	1
#  endif

#  if __TURBOC__ >=0x200

     /*
      * Borland C++ and later Turbo C++ have inlined assembly.  So do
      * earlier Turbo C++ *professional* versions.
      * In this case, you should uncomment the
      * directive for "RW_INLINE86_ASSEMBLY" above.
      * No harm done if you don't: things will just be slower.
      */

     /*
      * Borland C++ Version 2.0:
      * Calls destructor of temporaries in inlines of inlines incorrectly.
      * Increments pointer to structs incorrectly.
      * Passes the wrong size to an overloaded delete
      */
#    if __TURBOC__ < 0x300
#      define RW_BCC_INLINE_DESTRUCTOR_BUG 1
#      define RW_BCC_STRUCT_POINTER_BUG    1
#      define RW_TCC_DELETE_SIZE_BUG	1
#    endif	/* end Borland C++ V2.0 */
/*
 *   No templates before 3.0
 */  
#    if __TURBOC__ < 0x400     
#      define RW_NO_TEMPLATES 1
#    endif
/*
 *  In Borland C++ versions previous to 4.0, wchar_t is not a
 *  built-in type and there are no exceptions
 */
#    if __TURBOC__ < 0x451
#      define RW_NO_EXCEPTIONS         1
#      define RW_NO_FRIEND_INLINE_DECL 1
#      define RW_NO_OVERLOAD_WCHAR     1
#      define RW_NO_OVERLOAD_SCHAR     1
#      define RW_NO_XMSG               1
#    else  
/*
 *   Borland 4.0: 
 *   While Borland does provide these, they don't seem to consult the
 *   current locale when comparing.  Now that Borland provides working
 *   locales other than "C", we better roll our own: 
 */
#      define RW_NO_STRICMP  1
#      define RW_NO_STRNICMP 1
/*   Borland has an xmsg class, but for now, we're not compatible */
#      define RW_NO_XMSG 1	
#    endif
/*
 *   If the win32 API is available we can supply the WC string stuff
 *   for Borland (except for wcsxfrm for now)
 */
#    ifdef __WIN32__
#      define RW_SUPPLY_WSTR 1
#      define RW_NO_WCSXFRM  1
#    else
#      define RW_NO_WSTR     1
#    endif

#    ifdef __MT__
#      define RW_MULTI_THREAD 1
#    endif

#  endif	/* end post Turbo C++ V1.01 section */
#endif	/* __TURBOC__ */

/************************ Microsoft C/C++ *****************************/

#if defined(_MSC_VER)
#  if (_MSC_VER >= 900)
#    define RW_MSC_BACKEND       1
#    define RW_NO_XMSG           1
#    define RW_COMPILE_INSTANTIATE 1
#    define RW_NO_OVERLOAD_WCHAR 1
#  endif
#  if (_MSC_VER < 900) 
#    define RW_MSC_BACKEND       1
#    define RW_NO_EXCEPTIONS     1
#    define RW_NO_OVERLOAD_WCHAR 1
#    define RW_NO_TEMPLATES      1
#    define RW_NO_XMSG           1
#  endif

#  if defined(_MSDOS) && !defined(WIN32) && !defined(_WIN32)
#    define __MSDOS__   1
#    if defined(_WINDOWS)
#      define __WIN16__ 1
#    endif
#  endif
#  if defined(WIN32) || defined(_WIN32)
#    define __WIN32__
#    define RW_TOLOWER_SIGN_EXTENDS_RESULT_BUG 1
#  else
#    define RW_NO_WSTR 1
#  endif

#  ifdef _RWBUILDDLL
#    define __DLL__ 1
#  endif

#  ifdef _M_I86SM
#    define __SMALL__ 1
#  endif
#  ifdef _M_I86CM
#    define __COMPACT__ 1
#  endif
#  ifdef _M_I86MM
#    define __MEDIUM__ 1
#  endif
#  ifdef _M_I86LM
#    define __LARGE__ 1
#  endif
#  ifdef _MT
#    define RW_MULTI_THREAD 1
#  endif
#endif

/************************** WATCOM C/C++ ******************************/

#ifdef __WATCOMC__
#  define RW_NO_XMSG 1
#  define RW_NO_WSTR 1
#  define RW_NO_LEADING_UNDERSCORE 1
#  define RW_NO_NESTED_QUOTES 1
/* 
 * Note: Watcom cannot handle high precision (RW_DEFAULT_PRECISON)
 * correctly in all circumstances.  Use relatively low value:
 */
#  define RW_DEFAULT_PRECISION 10
#  if defined(__DOS__) && !defined(__MSDOS__)
#    define __MSDOS__ 1
#  endif
#  if defined(__WINDOWS__)
#    if defined(__WINDOWS_386__)
#      define __WIN32__ 1
#    else
#      define __WIN16__ 1
#    endif
#  endif
#endif


/********************** Metaware High C/C++ ***************************/

#if defined(__HIGHC__)
#  define RW_NO_OVERLOAD_WCHAR 1
#  define RW_NO_XMSG           1
#  if defined(_MSDOS)
#    define __MSDOS__ 1
#    define RW_HIGHC_INLINE_BUG  1
#    define RW_NO_EXCEPTIONS     1
#    define RW_NO_WSTR 1
#  endif

#  if defined(_OS2)
#    undef __OS2__
#    define __OS2__ 1
#    define RW_SUPPLY_WSTR 1
#    define RW_NO_WCSXFRM  1
#    ifdef _REENTRANT
#      define RW_MULTI_THREAD 1
#    endif
#  endif

#  if defined(_MSNT)
#    undef __WIN32__
#    define __WIN32__ 1
#    define RW_NO_WSTR 1
#    if defined(_REENTRANT) || defined(_MT)
#      define RW_MULTI_THREAD 1
#    endif
#  endif

#endif  /* __HIGHC__ */

/************************** MPW *************************************/

#if defined(applec)
#  define RW_BROKEN_TOKEN_PASTE    1
#  define RW_REVERSED_CR_AND_LF    1
#  define RW_NATIVE_EXTENDED       1
#  define RW_NO_CPP_RECURSION      1
#  define RW_NO_EXCEPTIONS         1
#  define RW_NO_GETTIMEOFDAY       1
#  define RW_NO_GLOBAL_TZ          1
#  define RW_NO_LEADING_UNDERSCORE 1
#  define RW_NO_OVERLOAD_WCHAR     1
#  define RW_NO_POSTFIX            1
#  define RW_NO_STRICMP            1
#  define RW_NO_STRNICMP           1
#  define RW_NO_TEMPLATES          1
#  define RW_NO_WSTR               1
#  define RW_NO_XMSG	           1
#endif

/********************** IBM C/Set++   *********************************/

#ifdef __IBMCPP__
#  define RW_IOS_XALLOC_BROKEN 1
#  define RW_NO_WSTR           1
#  define RW_NO_XMSG           1
#  ifdef __MULTI__
#    define RW_MULTI_THREAD 1
#  endif
#endif

/********************** IBM xlC   *********************************/

#if defined(_AIX) && !defined(unix)
#define unix 1
#endif

/********************** Miscellaneous *********************************/

/*
 * These compilers instantiate templates at compile-time;
 * others at link-time.
 */
#if defined(__TURBOC__) || defined(__HIGHC__) || defined(__xlC__) || defined(__GNUG__) || defined(__WATCOMC__) || defined(__IBMCPP__) || defined(__os_cplusplus)
# define RW_COMPILE_INSTANTIATE 1 
#endif

/* No Pi for these compilers: */
#if defined(RW_MSC_BACKEND) || defined(__OREGON__) || defined(__HIGHC__) || defined(applec) || defined(CII) || defined(__WATCOMC__)
#  ifndef M_PI
#    define M_PI 3.14159265358979323846
#  endif
#endif

#ifdef RW_GLOBAL_ENUMS
   /* V2.0 syntax */
#  define RWSCOPE(a)
#  define RWVECTOR_DELETE(i) delete[i]
#else
   /* V2.1 syntax */
#  define RWSCOPE(a) a::
#  define RWVECTOR_DELETE(i) delete[]
#endif

/*
 * Only Sun defines strftime("%C", ...)
 */
#if !defined(RW_NO_STRFTIME_CAPC) && !defined(sun)
#define RW_NO_STRFTIME_CAPC 1
#endif

/********************** Environment *********************************/
/*
 * This is the section for setting things which depend on the properties
 * of the operating systems rather than specific compilers.  It follows
 * the compiler section so we have the chance to rationalize the different
 * preprocessor constants (e.g. _MSDOS vs. __MSDOS__,  _M_I86LM vs. __LARGE__)
 */

#ifndef RW_BYTES_PER_PTR
# if defined(__SMALL__) || defined(__MEDIUM__)
#   define RW_BYTES_PER_PTR 2
# else
#   define RW_BYTES_PER_PTR 4	/* Assume 32 bit pointers */
# endif
#endif

#ifndef RW_BYTES_PER_WORD
# if defined(__SMALL__) || defined(__COMPACT__) || defined(__MEDIUM__) || defined(__LARGE__)
#   define RW_BYTES_PER_WORD 2
# else
#   define RW_BYTES_PER_WORD 4	/* Assume 32 bit words */
# endif
#endif

#ifndef RW_DEFAULT_PRECISION
#   define RW_DEFAULT_PRECISION 16	/* Assume standard IEEE format */
#endif

/*
 * Most (but not all) non-unix systems convert new line to carriage
 * return / line feed on output:
 */
#if defined(__MSDOS__) || defined(__OS2__) || defined(__WIN32__) || defined(__NT__) || defined(__WINDOWS__)
#  define RW_CRLF_CONVENTION 1
#endif


#if 0
/*
 ****************************************************************************
 *
 * $Log: compiler.in,v $
 * Revision 6.18  1994/07/18  03:07:36  jims
 * Expand section for HIGHC to include OS/2 and NT settings
 *
 * Revision 6.17  1994/07/14  06:07:28  jims
 * Lower DEFAULT_PRECISION for Watcom to 10
 *
 * Revision 6.16  1994/07/13  17:40:55  nevis
 * Changed default precision to 16 match vriezen's change to
 * toolread.doc and to duck a Watcom bug.
 *
 * Revision 6.15  1994/07/12  23:11:07  nevis
 * MetaWare section now checks for OS/2.
 *
 * Revision 6.14  1994/07/12  21:19:30  nevis
 * Fixed typo in MetaWare section: WR_NO_WSTR instead of RW_NO_WSTR
 *
 * Revision 6.13  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.12  1994/07/07  22:15:36  nevis
 * Watcom cannot handle default precision of 17...changed to 16...
 *
 * Revision 6.11  1994/06/29  00:14:50  vriezen
 * Set RW_NO_OVERLOAD_SCHAR for Borland 3.1
 *
 * Revision 6.10  1994/06/22  21:35:07  vriezen
 * Add RW_NO_OVERLOAD_SCHAR
 *
 * Revision 6.9  1994/06/16  00:55:07  vriezen
 * Bug #54.  Enhance precision support for RWpostream
 *
 * Revision 6.8  1994/06/06  20:48:34  nevis
 * Added RW_NO_THROW_WITH_SHARED for compilers which cannot
 * throw exceptions from a shared library.
 *
 * Revision 6.7  1994/06/02  22:56:23  foote
 * Defined RW_NO_FRIEND_INLINE_DECL for Symantec 7.0 on Macintosh
 *
 * Revision 6.6  1994/06/02  17:58:04  foote
 * Fixed typo in last revision
 *
 * Revision 6.5  1994/06/02  17:45:10  foote
 * Port to Symantec 7.0 on Macintosh
 *
 * Revision 6.4  1994/05/16  18:06:58  jims
 * Port to Win32 DLL
 *
 * Revision 6.3  1994/05/09  20:02:50  vriezen
 * Added RW_DOUBLE_PRECISION
 *
 * Revision 6.2  1994/05/06  01:58:38  vriezen
 * Add RW_NO_NESTED_QUOTES to Watcom  Bug #385
 *
 * Revision 6.1  1994/04/15  18:40:32  vriezen
 * Move all files to 6.1
 *
 * Revision 2.58  1994/04/01  14:13:33  vriezen
 * Add RW_NO_WSTR to Metaware
 *
 * Revision 2.57  1994/03/10  23:22:58  jims
 * Add RW_IOS_XALLOC_BROKEN to IBMCPP (C Set ++)
 *
 * Revision 2.56  1994/03/04  07:41:30  jims
 * Set __WIN32__ / __WIN16__ for Watcom; make sure RW_CRLF_CONVENTION
 * defined for Watcom
 *
 * Revision 2.55  1994/01/11  05:05:54  myersn
 * make IBM xlC #define unix as do all other unices.
 *
 * Revision 2.54  1993/12/10  20:50:58  jims
 * Add ObjectStore compiler to those with compile-time instantiation
 *
 * Revision 2.53  1993/11/16  08:39:22  myersn
 * add OS/2 multithread flag
 *
 * Revision 2.52  1993/11/03  23:27:03  jims
 * define RW_REVERSED_CR_AND_LF for MPW
 * add __WATCOMC__ to list of compilers that don't define pi
 *
 * Revision 2.51  1993/09/23  21:33:50  alv
 * added CII (computer innovations) to list of compilers without M_PI
 *
 * Revision 2.50  1993/09/21  17:28:21  dealys
 * corrected MPW flags
 *
 * Revision 2.49  1993/09/16  18:48:13  keffer
 * Fine tuned support for Symantec V6.0.
 *
 * Revision 2.48  1993/09/16  05:53:10  keffer
 * MS Visual C++ does not need RW_NO_LEADING_UNDERSCORE
 *
 * Revision 2.47  1993/09/16  01:28:21  keffer
 * Corrected typo in comment.
 *
 * Revision 2.46  1993/09/16  00:36:56  keffer
 * Added RW_NO_XDR
 *
 * Revision 2.45  1993/09/15  20:47:16  keffer
 * Added RW_STRUCT_TM_TZ.
 *
 * Revision 2.44  1993/09/14  17:44:40  keffer
 * Added support for Symantec C/C++ V6.0
 *
 * Revision 2.43  1993/09/14  06:09:59  myersn
 * add flags for IBM C/Set++
 *
 * Revision 2.42  1993/09/13  16:38:41  keffer
 * Added support for WATCOM C/C++ 32 V9.5
 *
 * Revision 2.41  1993/09/13  09:17:40  myersn
 * add support for RW_NO_LEADING_UNDERSCORE.
 *
 * Revision 2.40  1993/09/10  22:38:56  jims
 * Adjust __TURBOC__ value to match latest release of Borland
 *
 * Revision 2.39  1993/09/10  17:46:02  jims
 * Add define for Microsoft tolower sign extension of result bug
 *
 * Revision 2.38  1993/09/07  18:17:37  jims
 * MSC now defines _WIN32 under Windows NT
 *
 * Revision 2.37  1993/08/31  19:47:19  keffer
 * Now detects __GNUG__ instead of __GNUC__.
 *
 * Revision 2.36  1993/08/20  03:15:32  keffer
 * The macro RW_COMPILE_INSTANTIATE is now set by config.
 *
 * Revision 2.35  1993/08/11  00:36:51  myersn
 * set RW_NO_STRFTIME_CAPC according to environment (i.e. Sun or not).
 *
 * Revision 2.34  1993/08/05  11:38:36  jims
 * Adjust Borland and Microsoft sections for DOS / Windows / NT
 *
 * Revision 2.33  1993/08/03  21:55:16  dealys
 * Ported to MPW C++ 3.3
 *
 * Revision 2.32  1993/08/03  21:44:29  jims
 * *** empty log message ***
 *
 * Revision 2.31  1993/07/30  03:20:34  jims
 * Port to MS C7
 *
 * Revision 2.30  1993/07/29  11:19:29  jims
 * Reorganize non-unix compiler section
 *
 * Revision 2.29  1993/07/15  10:37:40  jims
 * Set RW_NO_STR(N)ICMP for Borland 4.0
 *
 * Revision 2.28  1993/06/13  22:05:55  jims
 * Port to Borland 4.0 including check for __WIN32__
 *
 * Revision 2.27  1993/06/06  00:16:44  keffer
 * Introduced RW_BYTES_PER_PTR and RW_BYTES_PER_WORD
 *
 * Revision 2.26  1993/05/19  23:11:19  keffer
 * Added RW_NO_XMSG
 *
 * Revision 2.25  1993/04/13  03:17:35  myersn
 * add RW_IOS_XALLOC_BROKEN flag, restore RW_HIGHC_INLINE_BUG flag.
 *
 * Revision 2.25  1993/03/15  18:39:34  alv
 * added RW_HIGHC_INLINE_BUG
 *
 * Revision 2.24  1993/03/15  18:25:14  keffer
 * Added 'RW_' prefix to remaining macros.
 *
 * Revision 2.23  1993/03/01  17:53:00  alv
 * ported to Metaware High C++
 *
 * Revision 2.22  1993/02/13  23:07:51  keffer
 * Corrected syntax error.
 *
 * Revision 2.21  1993/02/13  22:21:26  keffer
 * Zortech V3.0
 *
 * Revision 2.20  1993/02/12  20:05:34  keffer
 * Added g++ to the list of compilers that do compile-time instantiation.
 *
 * Revision 2.19  1993/02/11  23:58:28  keffer
 * Added the IBM xlC compiler to the list of compilers that do
 * compile time instantiation.
 *
 * Revision 2.18  1993/01/29  22:30:26  alv
 * RW_CRLF_CONVENTION now true for OS2 as well as DOS
 *
 * Revision 2.17  1993/01/27  03:43:49  keffer
 * Added macro RW_COMPILE_INSTANTIATE for compilers that do
 * template instantiation at compile time.
 *
 * Revision 2.16  1993/01/26  23:55:49  alv
 * Wrapped Log in #if 0 ... #endif to avoid problems due to log
 * entries containing cpp macros or comments
 *
 * Revision 2.15  1993/01/26  23:30:45  keffer
 * The macro __ATT?__ now set.
 *
 * Revision 2.14  1993/01/26  02:05:57  keffer
 * RW_NO_OVERLOAD_WCHAR now defined to 1 instead of nothing.
 *
 * Revision 2.13  1993/01/25  22:04:11  keffer
 * Changed comments for RW_NO_CLOCK.
 *
 * Revision 2.12  1993/01/25  18:13:56  keffer
 * RW_NO_CONST_OVERLOADS->RW_NO_CONST_OVERLOAD
 *
 * Revision 2.10  1993/01/22  18:25:41  alv
 * Fixed so that macros only get #define'd, not #undef'd in the
 * bottom half of the file.
 *
 * Revision 2.8  1992/12/01  04:11:37  myersn
 * undefined RW_NO_OVERLOAD_WCHAR for Gnu gcc
 *
 * Revision 2.7  1992/11/30  23:13:36  myersn
 * change RW_NO_WCHAR_OVERLOAD to RW_NO_OVERLOAD_WCHAR
 *
 * Revision 2.6  1992/11/26  03:48:26  myersn
 * add __GNUC__ to list of compilers allowed to claim template support
 *
 * Revision 2.5  1992/11/20  02:35:01  keffer
 * Changed RW_NO_ANSI_PRINTF to RW_NO_ANSI_SPRINTF
 *         RW_NO_WCHAR_T     to RW_NO_WSTR
 *         RW_NO_WCHAR_TYPE  to RW_NO_WCHAR_OVERLOAD
 *
 * Revision 2.3  1992/11/19  04:13:08  keffer
 * Introduced new macro names.
 *
 * 
 */
#endif

#endif /* __RWCOMPILER_H__ */
