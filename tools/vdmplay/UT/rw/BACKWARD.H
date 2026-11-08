#ifndef __RWBACKWARD_H__
#define __RWBACKWARD_H__

/*
 * Declarations for backwards compatibility with earlier versions
 * of Tools.h++.
 *
 * $Id: backward.h,v 6.2 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: backward.h,v $
 * Revision 6.2  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:21  vriezen
 * Move all files to 6.1
 *
 * Revision 1.8  1993/10/29  06:04:27  myersn
 * add some more obsolete declarations.
 *
 * Revision 1.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.6  1993/04/15  19:58:16  keffer
 * Added RWClassID macros for old generics
 *
 * Revision 1.5  1993/03/26  21:18:19  myersn
 * add rwdate.h & rwtime.h typedefs (dayTy etc.)
 *
 * Revision 1.4  1993/03/26  19:57:23  keffer
 * *** empty log message ***
 *
 * Revision 1.3  1993/03/16  01:58:27  keffer
 * *** empty log message ***
 *
 * Revision 1.1  1992/11/16  04:09:50  keffer
 * Initial revision
 *
 *
 */

#if defined(RWTOOLS4_COMPATIBLE) && !defined(RWTOOLS5_COMPATIBLE)
#  define RWTOOLS5_COMPATIBLE 1
#endif

#ifdef RWTOOLS4_COMPATIBLE
  typedef RWCollectable*	(*userCreator)();
  typedef long			storedValue;
  typedef long			nodeOffset;

# define DEFINE_COLLECTABLE(className)				\
  RWDEFINE_COLLECTABLE1(className)				\
  RWInit(className)::RWInit(className)(){			\
    className temp;						\
    rwAddToFactory(rwCreateFN(className), temp.isA());		\
  }								\
  RWInit(className)::~RWInit(className)(){			\
    className temp;						\
    rwRemoveFromFactory(temp.isA());				\
  }

#endif

#ifdef RWTOOLS5_COMPATIBLE
#  if !defined(RW_STD_TYPEDEFS) && !defined(RWNO_STD_TYPEDEFS)
#    define RW_STD_TYPEDEFS 1
#  endif
#endif

#define DEFINE_COLLECTABLE(a,b) RWDEFINE_COLLECTABLE(a,b)

/*********************  Old generic.h names *******************/

#define GBitVec(sz)          RWGBitVec(sz)
#define GDlist(type)         RWGDlist(type)
#define GDlistIterator(type) RWGDlistIterator(type)
#define GOrderedVector(val)  RWGOrderedVector(val)
#define GQueue(type)         RWGQueue(type)
#define GSlist(type)         RWGSlist(type)
#define GSlistdeclare(type)  RWGSlistdeclare(type)
#define GSlistIterator(type) RWGSlistIterator(type)
#define GSortedVector(type)  RWGSortedVector(type)
#define GStack(type)         RWGStack(type)
#define GVector(val)         RWGVector(val)

#define __GDLIST                      0x8014
#define __GDLISTITERATOR              0x8015
#define __GQUEUE                      0x8016
#define __GSTACK                      0x8017
#define __GVECTOR                     0x8018

/*****************  Old <rw/compiler.h> macros ****************/

#ifndef RW_GLOBAL_ENUMS
#  define SCOPE_NESTED_ENUMS 1
#endif

#ifdef RW_KR_ONLY
#  define KR_ONLY 1
#endif

#ifdef RW_NO_ACCESS_ADJUSTMENT
#  define NO_ACCESS_ADJUSTMENT 1
#endif

#ifndef RW_NO_CONST_OVERLOAD
#  define CONST_OVERLOADS 1
#endif

#ifndef RW_NO_MEMMOVE
#  define HAS_MEMMOVE 1
#endif

#ifndef RW_NO_POSTFIX
#  define HAS_POSTFIX 1
#endif

/******************** From <rw/defs.h> ***************************/

#ifndef PRECONDITION
/* The following group of lines can be removed if they cause naming conflicts: */
#define PRECONDITION(a)  RWPRECONDITION(a)	/* For backwards portability */
#define POSTCONDITION(a) RWPOSTCONDITION(a)
#if defined(BOUNDS_CHECK) && !defined(RWBOUNDS_CHECK)
#  define RWBOUNDS_CHECK 1			/* For backwards portability */
#endif
#endif

typedef int fileDescTy;	/* Type of file descriptors */
typedef int ClassID;

/********************* From <rw/rstream.h> *************************/

#define OPENOSTREAM(filename, streamname)  ofstream streamname(filename);
#define OPENISTREAM(filename, streamname)  ifstream streamname(filename);

/********************* From <rw/rwdate.h> *************************/

typedef unsigned dayTy;
typedef unsigned monthTy;
typedef unsigned yearTy;
typedef unsigned long julTy;

#define jul1901 rwJul1901

/********************* From <rw/rwtime.h> *************************/

typedef unsigned hourTy;
typedef unsigned minuteTy;
typedef unsigned secondTy;
typedef unsigned long clockTy;

#define secFrom_Jan_1_1901_to_Jan_1_1970 rwEpoch

#endif	/* __RWBACKWARD_H__ */

