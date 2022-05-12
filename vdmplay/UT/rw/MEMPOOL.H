#ifndef __RWMEMPOOL_H__
#define __RWMEMPOOL_H__

/*
 * RWMemoryPool: Manages small object pool via inheritance
 *
 * $Id: mempool.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: mempool.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/12  18:05:49  vriezen
 * Adjusted #if in .h and .cpp file to test same conditions. Bug #416
 *
 * Revision 6.1  1994/04/15  19:10:28  vriezen
 * Move all files to 6.1
 *
 * Revision 2.9  1994/03/23  00:56:26  vriezen
 * Add trailing #endif
 *
 * Revision 2.8  1994/03/22  21:40:23  vriezen
 * Added RW_DONT_USE_MEMORY_POOL option
 *
 * Revision 2.7  1993/11/08  09:11:13  jims
 * Port to ObjectStore
 *
 * Revision 2.6  1993/09/03  02:08:13  keffer
 * Macro _CLASSDLL is now named _RWTOOLSDLL
 *
 * Revision 2.5  1993/08/05  11:49:12  jims
 * Distinguish between using a WIN16 DLL from a WIN32 DLL by
 * checking for __WIN16__
 *
 * Revision 2.4  1993/03/17  18:41:25  keffer
 * TCC_DELETE_SIZE_BUG -> RW_TCC_DELETE_SIZE_BUG
 *
 * Revision 2.3  1993/02/18  17:00:47  keffer
 * Constants no longer declared static.
 *
 * Revision 2.2  1993/01/28  01:54:00  myersn
 * define macro RWMemoryPool_OPTION for MT-safe use
 *
 * Revision 2.1  1993/01/15  22:23:49  keffer
 * Tools.h++ V5.2 alpha
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.9   22 Jun 1992 17:52:28   KEFFER
 * Static data no longer visible when compiling or using the Tools.h++ DLL.
 * 
 *    Rev 1.8   28 May 1992 10:38:10   KEFFER
 * Now compatible with RWMEMCK.
 * 
 *    Rev 1.5   17 Oct 1991 09:13:00   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.3   31 Aug 1991 20:40:28   keffer
 * Now includes <stddef.h>, with a STARTWRAP wrapper
 * 
 *    Rev 1.1   24 Jul 1991 13:06:44   keffer
 * Added pvcs keywords
 *
 */

/*
 * This class maintains a pool of free memory allocations for small objects.
 * Objects inheriting from it will be managed by it.  It can greatly improve
 * the performance of objects that are rapidly allocated and deleted, such
 * as nodes in a linked list or binary tree.  It can also help memory
 * fragmentation.
 *
 * Two static variables affect its performance:
 *
 *  RWMAXPOOLS:		The number of different sizes managed.  The
 *			default (5) will manage objects with sizes
 *			from 1 to 10 bytes big.  Objects bigger than this
 * 			will be handled by the operating system.
 *
 *  RWPOOLSIZE:		The maximum number of objects retained within a size class.
 *			Excess objects are returned to the operating system.
 */



#ifdef RW_DONT_USE_MEMORY_POOL
#  define RWMemoryPool_OPTION
#else



#ifndef __RWDEFS_H__
#  include "rw/defs.h"
#endif
STARTWRAP
#include <stddef.h>	/* Looking for size_t*/
ENDWRAP

const int RWPOOLSIZE = 5;
const int RWMAXPOOLS = 5;

class RWExport RWMemoryPool {
// If not a 16bitDLL and not memcheck and not debug
// If following changes, also change #if in mempool.cpp
#if !(defined(_RWTOOLSDLL) && defined(__WIN16__)) && !defined(RWMEMCK) && !defined(RWDEBUG) 
  static void*		stash[RWMAXPOOLS][RWPOOLSIZE];	// The private stash
  static short		nstash[RWMAXPOOLS];		// Number of free objects in each size class.
#endif
public:
#ifdef RW_TCC_DELETE_SIZE_BUG
  ~RWMemoryPool() { }	// Superfluous destructor required for Borland bug
#endif
  void			operator delete(void*, size_t);
  void*			operator new(size_t);
#ifdef RWMEMCK
  void*			operator new(size_t size, const char* filename, int line);
#endif
};

// Multi-threaded programs can't afford conflicts in the static
// RWMemoryPool free-block pool.  We inherit differently because
// otherwise every derived object would grow by (up to) 4 bytes,
// for no reason.

#ifdef RW_MULTI_THREAD
#define RWMemoryPool_OPTION
#else
#define RWMemoryPool_OPTION   : public RWMemoryPool
#endif

#endif /* RW_DONT_USE_MEMORY_POOL */

#endif	/* __RWMEMPOOL_H__ */
