#ifndef __RWMEMCK_H__
#define __RWMEMCK_H__

/*
 * RWMemoryCheck: for sensing wild pointers and memory leaks
 *
 * $Id: memck.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
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
 * Objects of type RWMemCheck are pushed onto an internal stack
 * when they go into scope, and popped back off when they go out of
 * scope.  If the flag "checkDestroy" was set, then when they go out of
 * scope they will report on any outstanding allocations, thus giving you
 * a sense of any memory leaks.  You can also ask the object to walk
 * its internal list of outstanding allocations by calling "walkList()" at
 * any time.
 *
 * To use, compile your program and any libraries with the flag
 * RWMEMCK defined, include this file, and allocate one of these objects
 * in a scope you wish to test.  If the flag RWMEMCK is not defined, then
 * effect of the class will be a no-op.
 *
 * Here's an example:
 *
 *   #define RWMEMCK 1
 *   #include <rw/memck.h>
 *   #define new rwnew			// Allows line numbers to be reported
 *
 *   main() {
 *
 *     RWMemoryCheck check;		// Set a scope for checking memory
 *   
 *     double* d = new double[10];	// Do an allocation
 *     d[10] = 2.0;			// Wild pointer
 *   
 *     double* e = new double[50];
 *   
 *     delete d;			// Delete "d"
 *   
 *     return 0;			// NB: "e" never deleted.
 *   }
 *
 * This program will generate the following output:
 *
 *   MEMCK: Wild pointer.
 *          Memory block at 0x1ea6001a; allocated in file example.cpp line 9
 *   
 *   MEMCK: Outstanding allocation.
 *          Memory block at 0x1eae001a; allocated in file example.cpp line 12
 *
 *
 ***************************************************************************
 *
 * $Log: memck.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:27  vriezen
 * Move all files to 6.1
 *
 * Revision 2.4  1993/08/21  21:42:02  keffer
 * Fixed to handle older compilers that do not scope their enums.
 *
 * Revision 2.3  1993/07/14  21:15:49  randall
 * added cast to RWMemoryCheck constructor for enumerated type
 *
 * Revision 2.2  1993/06/21  21:11:37  keffer
 * Added no-op walkList() if RWMEMCK is not defined.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.1   08 Jun 1992 12:16:28   KEFFER
 * Ported to Liant under Unix.
 * 
 *    Rev 1.0   28 May 1992 09:09:46   KEFFER
 * Initial revision.
 */


#ifndef __RWTOOLDEFS_H__
# include "rw/tooldefs.h"
#endif

#if defined(RWMEMCK)	/* Proceed only if doing memory checks */
#include "rw/rstream.h"

class RWMemAlloc;

struct RWMemLink {
  RWMemAlloc*		next;
};

class RWExport RWMemoryCheck {

public:

  enum memckType {
    retainFree = 0x01,	// Do not return free blocks to OS (can check use after delete)
    checkEvery = 0x02,	// Check & report on heap state on every allocation
    checkDestroy=0x04	// Report outstanding allocations when destructor is called
  };

#ifdef __LPI__
  RWMemoryCheck
    (
      RWMemoryCheck::memckType c = 
	(RWMemoryCheck::memckType)
	(RWMemoryCheck::checkEvery | RWMemoryCheck::checkDestroy),
      ostream& str=cout				// Report stream
    );
#else
#  ifdef RW_GLOBAL_ENUMS
  RWMemoryCheck
    (
      memckType c = (memckType)(checkEvery | checkDestroy),	
      ostream& str=cout				// Report stream
    );
#  else
  RWMemoryCheck
    (
      memckType c = (RWMemoryCheck::memckType)(checkEvery | checkDestroy),	
      ostream& str=cout				// Report stream
    );
#  endif
#endif
  ~RWMemoryCheck();

  // Walk the free and outstanding lists, checking integrity.
  // If argument is TRUE, then report outstanding allocations too.
  void			walkList(RWBoolean reportAlloc = FALSE);

private:

  RWMemLink		allocList_;	// List of outstanding allocations
  RWMemLink		freeList_;	// List of free blocks
  ostream&		rpstream_;	// Report stream
  memckType		checks_;	// Type of check

  void*			allocate(size_t, const char*, int);
  void			deallocate(void*);
  void			releaseFree();
  RWMemAlloc*		removeFromAllocList(RWMemAlloc*);
  RWBoolean		onFreeList(RWMemAlloc*);

friend void* operator new(size_t size, const char* filename, int line);
friend void* operator new(size_t);
friend void  operator delete(void*);
};


#else	/* Not doing memory checks */


class ostream;

class RWMemoryCheck {

public:

  enum memckType {
    retainFree = 0x01,	// Do not return free blocks to OS (can check use after delete)
    checkEvery = 0x02,	// Check & report on heap state on every allocation
    checkDestroy=0x04	// Report outstanding allocations when destructor is called
  };

  // This approach avoids including <iostream.h> :
  RWMemoryCheck()                    { /* No-op */ }
  RWMemoryCheck(memckType)           { /* No-op */ }
  RWMemoryCheck(memckType, ostream&) { /* No-op */ }

  void			walkList(RWBoolean reportAlloc = FALSE) { /* No-op */ }

};

#endif

#endif	/*__RWMEMCK_H__ */
