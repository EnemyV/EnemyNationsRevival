#ifndef __RWDEFS_H__
#define __RWDEFS_H__

/*
 * Common definitions
 *
 * $Id: defs.h,v 6.10 1994/07/14 21:04:32 jims Exp $
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
 ***************************************************************************
 *
 * $Log: defs.h,v $
 * Revision 6.10  1994/07/14  21:04:32  jims
 * Use RWGExport for generic collection classes
 *
 * Revision 6.9  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.8  1994/07/12  17:56:52  vriezen
 * Updated copyright.
 *
 * Revision 6.7  1994/07/05  15:58:37  jims
 * Move #include for rw/generic.h down below #define for "rwexport" (necessary
 * due to mutual inclusion relationship between rw/defs.h and rw/generic.h)
 *
 * Revision 6.6  1994/06/22  00:24:50  vriezen
 * For RWPRECONDITION and friends, use != 0 on condition
 * to accomodate compilers where casting pointer to int
 * might lose data and/or change truth value.  Bug# 539
 *
 * Revision 6.6  1994/06/22  00:22:03  vriezen
 * When forwarding from RWPRECONDITION, etc. to assert, use !=
 * to accomodate compilers where (int) pointer might lose data
 * and/or change truth value.
 *
 * Revision 6.5  1994/06/16  04:13:42  vriezen
 * Add generic.h include
 *
 * Revision 6.4  1994/06/16  02:49:13  vriezen
 * Added RW_DECLARE_HOME and RW_DEFINE_HOME macros.
 *
 * Revision 6.3  1994/05/13  20:43:05  jims
 * Remove #include for rw/generic.h (left over from 6.2)
 *
 * Revision 6.2  1994/04/15  21:12:29  vriezen
 * Backed out (until next release) RW_DEFINE_HOME and RW_DECLARE_HOME support.
 *
 * Revision 6.1  1994/04/15  19:09:47  vriezen
 * Move all files to 6.1
 *
 * Revision 2.27  1994/04/01  18:56:06  vriezen
 * Remove extraneous ;
 *
 * Revision 2.26  1994/03/23  20:42:45  vriezen
 * Change RW_DECLARE_HOME to take classname arg
 *
 * Revision 2.25  1994/03/23  18:25:06  vriezen
 * Add RW_DECLARE_HOME and RW_DEFINE_HOME macros for pure-virt and inline only classes.
 *
 * Revision 2.24  1994/02/05  02:02:17  jims
 * Add huge pointer support for character reads and writes
 *
 * Revision 2.23  1993/09/14  18:18:54  alv
 * rwUninitialized moved to here from Math.h++
 *
 * Revision 2.22  1993/09/10  17:33:32  jims
 * Don't define near, far, etc. for Metaware (HIGHC),
 * Add define for RW_RCSID, change $Header to $Id
 *
 * Revision 2.21  1993/08/05  11:45:43  jims
 * Check for __WIN16__ or __WIN32__ instead of _Windows
 *
 * Revision 2.20  1993/06/23  01:07:41  keffer
 * RW_MULTI_THREAD now turned on by USL _REENTRANT hook.
 *
 * Revision 2.19  1993/05/01  18:23:19  keffer
 * Added overloaded version of rwmin and rwmax for unsigned long.
 *
 * Revision 2.18  1993/04/12  12:31:32  jims
 * Added #define for RWGUARD and forward declaration for RWMutex
 *
 * Revision 2.17  1993/04/09  17:53:10  keffer
 * Changed type of RWSize_T.
 *
 * Revision 2.16  1993/04/06  00:59:14  keffer
 * Added forward declaration for ios.
 *
 * Revision 2.14  1993/03/13  02:11:45  keffer
 * ClassID -> RWClassID; removed typedef for fileDescTy
 *
 * Revision 2.13  1993/03/13  01:21:17  keffer
 * Added DOS definitiosn for rwnear, rwfar, rwhuge.
 *
 * Revision 2.12  1993/03/13  01:03:17  keffer
 * Moved PRECONDITION definition to backward.h
 *
 * Revision 2.11  1993/03/13  00:38:51  keffer
 * near->rwnear; far->rwfar; huge->rwhuge
 *
 * Revision 2.10  1993/02/18  17:01:53  keffer
 * Added RWBITSPERBYTE
 *
 * Revision 2.9  1993/02/17  21:03:26  keffer
 * Constants no longer declared static.
 *
 * Revision 2.8  1993/02/14  05:25:40  myersn
 * add RW_NPOS, the "bad index" for string searches (was int val < 0).
 *
 * Revision 2.7  1993/02/11  18:14:28  keffer
 * Removed no-op definition for RWPRECONDITION and friends
 * to suppress warnings from xlC compiler.
 *
 * Revision 2.6  1993/02/07  21:30:27  keffer
 * Moved the definition for _RWCLASSTYPE from rwwind.h to
 * defs.h; this allows the library to be used with far virtual
 * tables by non-Windows code.
 *
 * Revision 2.5  1993/01/26  21:38:49  alv
 * rcsid now const char [] to stop sun warnings
 *
 * Revision 2.3  1992/11/26  20:11:25  myersn
 * declared value() members of RWCS_T and RWSize_T as const functions
 *
 * Revision 2.2  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.1  1992/11/16  04:06:52  keffer
 * Removed support for old-style V1.2 type streams.
 * Added wrapper classes RWSize_T and RWCS_T.
 * Now includes <stddef.h>
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.14   29 Sep 1992 11:18:22   KEFFER
 * Removed #define for Boolean and NIL.
 * 
 *    Rev 1.12   28 May 1992 16:33:08   KEFFER
 * Introduced RWPRECONDITION2
 * 
 *    Rev 1.11   28 May 1992 11:02:50   KEFFER
 * 
 *    Rev 1.10   24 Apr 1992 16:07:38   KEFFER
 * Added RWASSERT() macro.
 * 
 *    Rev 1.8   17 Mar 1992 19:21:10   KEFFER
 * Changed BOUNDS_CHECK to RWBOUNDS_CHECK
 * 
 *    Rev 1.7   17 Mar 1992 11:35:22   KEFFER
 * Introduced rwnil, RWPRECONDITION, RWPOSTCONDITION, RWDEBUG.
 * MS-DOS hacks now done here instead of compiler.h.
 * 
 *    Rev 1.6   21 Feb 1992 12:30:36   KEFFER
 * Debug flag is now "RWDEBUG" instead of "DEBUG".
 * 
 *    Rev 1.5   12 Nov 1991 13:14:14   keffer
 * Added _RWCLASSTYPE modifier.
 * 
 *    Rev 1.4   17 Oct 1991 09:12:44   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.2   27 Jul 1991 21:27:38   keffer
 * No longer includes <stddef.h>.  Other changes allow it to be used in C files.
 * 
 *    Rev 1.1   24 Jul 1991 13:06:42   keffer
 * Added pvcs keywords
 *
 */

#ifndef __RWCOMPILER_H__
# include "rw/compiler.h"	/* Set compiler-specific flags */
#endif

STARTWRAP
#include <stddef.h>		/* Looking for size_t */
ENDWRAP

/*
 * In rare instances, the following few lines may have to be reworked
 * to deal with naming conflicts.
 */

#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

typedef int RWBoolean;
#define rwnil	0
#define	RWNIL	-1L

/*************************************************************************
**************************************************************************
**									**
**		From here on, it's pretty much boilerplate		**
**		and rarely requires any tuning.				**
**									**
**************************************************************************
**************************************************************************/

/*
 *     D E B U G G I N G
 *
 * Use -DRWDEBUG to compile a version of the libraries to debug
 * the user's code.  This will perform pre- and post-condition checks
 * upon entering routines, but will be larger and run more slowly.
 *
 * Use -DRWMEMCK to add memory checking software.
 *
 * VERY IMPORTANT!  *All* code must be compiled with the same flag.
 */

#if defined(RDEBUG) && !defined(RWDEBUG)
#  define RWDEBUG 1
#endif

#if defined(RWDEBUG)
#  ifndef RWBOUNDS_CHECK
#    define RWBOUNDS_CHECK 1	/* Turn on bounds checking when debugging. */
#  endif
STARTWRAP
#  include <assert.h>
ENDWRAP
#  define RWPRECONDITION(a)	assert( (a) != 0 ) /* Check pre- and post-conditions */
#  define RWPOSTCONDITION(a)	assert( (a) != 0 )
#ifdef RW_NO_NESTED_QUOTES
#  define RWPRECONDITION2(a,b)	assert( (a) != 0 )
#  define RWPOSTCONDITION2(a,b)	assert( (a) != 0 )
#else
#  define RWPRECONDITION2(a,b)	assert((b, (a) !=0))
#  define RWPOSTCONDITION2(a,b)	assert((b, (a) !=0))
#endif
#  define RWASSERT(a)		assert( (a) != 0 )
#else
#  define RWPRECONDITION(a)
#  define RWPOSTCONDITION(a)
#  define RWPRECONDITION2(a,b)
#  define RWPOSTCONDITION2(a,b)
#  define RWASSERT(a)
#endif

/* Check for USL hook for multi-thread mode: */
#if defined(_REENTRANT) && !defined(RW_MULTI_THREAD)
# define RW_MULTI_THREAD 1
#endif

#ifdef RW_MULTI_THREAD
#define RWGUARD(mutex)  RWGuard rwdummyguard(mutex)
#else
#define RWGUARD(mutex)
#endif
  
/*
 *     W I N D O W S - S P E C I F I C   C O D E
 *
 * Enable or disable, as necessary, for Microsoft Windows
 */
#if defined(__WIN16__) || defined(__WIN32__)
#  include "rw/rwwind.h"
#else
   /* Disable Windows hacks if we are not compiling for Windows: */
#  define RWExport
#  define rwexport
#endif

#ifndef _MSC_VER
#define RWGExport RWExport
#define RWTExport RWExport
#else
#define RWGExport
#define RWTExport
#endif

/*
 * The following allows getting the declaration for RTL classes
 * right without having to include the appropriate header file
 * (in particular, istream & ostream).
 */
#ifdef __TURBOC__
#  include <_defs.h>	/* Looking for _CLASSTYPE */
#  define _RWCLASSTYPE _CLASSTYPE
#else
#  define _RWCLASSTYPE
#endif

/* No RCS for MS-DOS (it has enough memory problems already!): */
#ifdef __MSDOS__
#define RCSID(a)
#define RW_RCSID(a)
#else
#define RCSID(a) static const char rcsid[] = a
#define RW_RCSID(a) static const char rcsid[] = a
#endif

/* Enable near/far pointers if we are using segmented architecture: */
#if (defined(__MSDOS__) || defined(I8086)) && !defined(__HIGHC__)
#  define RW_SEGMENTED_ARCHITECTURE
#  define rwnear near
#  define rwfar  far
#  define rwhuge huge
#else
#  define rwnear
#  define rwfar
#  define rwhuge
#endif

typedef unsigned char  RWByte;    /* Bitflag atomics. */
typedef unsigned short RWClassID; /* Class ID tag. */
typedef unsigned short RWErrNo;   /* Error number */

#ifdef __cplusplus

/*
 *   C + +   S P E C I F I C   D E F I N I T I O N S
 */

/*
 * Wrapper classes to avoid unwanted type conversion.
 */
class RWSize_T
{
public:
  RWSize_T(size_t n) : N_(n) {;}
  size_t value() const           {return N_;}
private:
  size_t N_;
};

class RWCS_T
{
public:
  RWCS_T(const char * str) : str_(str) {;}
  const char* value() const	{return str_;}
private:
  const char* str_;
};

enum RWUninitialized { rwUninitialized };
enum RWSeverity {RWWARNING, RWDEFAULT, RWFATAL};

const RWErrNo  RWSUCCESS        = 0;
const size_t RWDEFAULT_CAPACITY = 64; /* Default collection class capacity */
const size_t RWDEFAULT_RESIZE   = 64; /* Default collection class resize */
const size_t RWBITSPERBYTE      = 8;  /* Number of bits per byte */
const size_t RW_NPOS            = ~(size_t)0;

inline double        rwmax(double        a, double        b) {return a>b? a : b;}
inline double        rwmin(double        a, double        b) {return a<b? a : b;}
inline int           rwmax(int           a, int           b) {return a>b? a : b;}
inline int           rwmin(int           a, int           b) {return a<b? a : b;}
inline unsigned      rwmax(unsigned      a, unsigned      b) {return a>b? a : b;}
inline unsigned      rwmin(unsigned      a, unsigned      b) {return a<b? a : b;}
inline unsigned long rwmax(unsigned long a, unsigned long b) {return a>b? a : b;}
inline unsigned long rwmin(unsigned long a, unsigned long b) {return a<b? a : b;}

class _RWCLASSTYPE istream;
class _RWCLASSTYPE ostream;
class _RWCLASSTYPE ios;
class RWExport RWvistream;
class RWExport RWvostream;
class RWExport RWFile;
class RWExport RWErrObject;
#ifdef RW_MULTI_THREAD
class RWMutex;
#endif

void rwexport RWThrow(RWErrObject a ...);	/* Raise an exception/error */
/*  For backwards compatibility: */
void RWError(RWSeverity, const char*, const char*);

/*
 * Class ID definitions for Core.h++ classes:
 */
#define __GLOBAL			0xf000
#define __RWBISTREAM			0xf001
#define __RWBOSTREAM			0xf002
#define __RWCLIPSTREAMBUF		0xf003
#define __RWDDESTREAMBUF		0xf004
#define __RWERROBJECT			0xf005
#define __RWFIXEDMEMORY			0xf006
#define __RWPISTREAM			0xf007
#define __RWPOSTREAM			0xf008
#define __RWVISTREAM			0xf009
#define __RWVOSTREAM			0xf00a
/* For historical reasons: */
#define __RWFILE			0x8020

#ifdef RW_NO_FRIEND_INLINE_DECL
#  define RW_INLINE_FRIEND friend
#else
#  define RW_INLINE_FRIEND inline friend
#endif

/* 
 * RW_DECLARE_HOME is used at the start of some classes to declare a
 * non-inline virtual function where there would otherwise be none.  This
 * allows many compilers to establish a "home" for the virtual function
 * table.  Otherwise, a virtual function table is built in every
 * compilation unit which includes the class declaration.  RW_DEFINE_HOME
 * is used to generate the implementation of the function.
 * 
 * The purpose of using macros is to hide the fact that these are
 * functions, since they are of no functional use.  Also, it
 * automatically includes the name of the class in the function name
 * which resolves name conflicts when a class inherits from two classes, both
 * with RW_DECLARE_HOME in them. (i.e RWXDRistream)
 */

// Do not move to top of file; uses "rwexport" from above:
#include "rw/generic.h"  

#ifdef RW_BROKEN_TOKEN_PASTE
#  define RW_DECLARE_HOME(classname) virtual void __RWhome##classname();
#  define RW_DEFINE_HOME(classname)   void classname::__RWhome##classname() {}
#else
#  define RW_DECLARE_HOME(classname) virtual void name2(__RWhome,classname)();
#  define RW_DEFINE_HOME(classname)   void classname::name2(__RWhome,classname)() {}
#endif /* RW_BROKEN_TOKEN_PASTE */


#endif /* if C++ */

#endif /* __RWDEFS_H__ */
