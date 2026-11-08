/*
 * Definitions for RWMemoryPool
 *
 * $Id: mempool.cpp,v 2.2 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1989, 1990, 1991. This software is subject to copyright 
 * protection under the laws of the United States and other countries.
 *
 ***************************************************************************
 *
 * This class maintains a stash of small objects.  
 *
 * $Log: /vdmplay/MEMPOOL.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 2.2  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 2.1  1993/08/05  11:47:26  jims
 * Distinguish between WIN16 DLLs and WIN32 DLLs by checking
 * for __WIN16__
 *
 * Revision 2.0  1992/10/23  03:36:08  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.8   14 Jul 1992 17:30:56   KEFFER
 * RWMemoryPool::operator delete() rounds size up to match operator new().
 * 
 *    Rev 1.7   13 Jul 1992 18:18:58   KEFFER
 * Memory pool now disabled if compiled with RWDEBUG.
 * 
 *    Rev 1.6   28 May 1992 11:01:40   KEFFER
 * Moved the special version of RWMemoryPool::new to memck.cpp.
 * 
 *    Rev 1.5   27 May 1992 18:19:08   KEFFER
 * Now compatible with RWMEMCK utility.
 * 
 *    Rev 1.4   13 Nov 1991 11:49:48   keffer
 * DLL version does not maintain a stash.
 * 
 *    Rev 1.3   17 Oct 1991 09:15:20   keffer
 * Changed include path to <rw/xxx.h>
 * 
 */

#include "rw/mempool.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: mempool.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

/*
 * The memory pool cannot be used in a Win16-DLL because one application might
 * allocate some memory, return it to the pool, then it might be recycled
 * to a different application, resulting in a fault. We could maintain separate
 * pools, one for each application, but the resulting overhead of retrieving
 * the appropriate pool would negate the advantages of maintaining the pool
 * in the first place.  Hence, it's simpler just to go to the operating
 * system for every allocation.
 */

#if (defined(__DLL__) && defined(__WIN16__)) || defined(RWMEMCK) || defined(RWDEBUG)

  void* RWMemoryPool::operator new(size_t sz)           { return ::new char[sz]; }
  void  RWMemoryPool::operator delete(void* p, size_t)  { ::delete p; }

#else

/*
 * The following two static member data are guaranteed to be initialized
 * to zero by the language.  This is crucial for "nstash".
 */
void* RWMemoryPool::stash[RWMAXPOOLS][RWPOOLSIZE];
short RWMemoryPool::nstash[RWMAXPOOLS];

void*
RWMemoryPool::operator new(size_t sz)
{
  // Round the size up to an even number:
  sz = (sz+1) & ~0x1;
  // Divide by two, then subtract one to get the slot number:
  int slot = (sz >> 1) - 1;

  // If this object is small enough for us to handle and there is an available
  // allocation, return it.  Otherwise call the global new operator:
  return slot < RWMAXPOOLS && nstash[slot] ? stash[slot][--nstash[slot]] : ::new char[sz];
}

void
RWMemoryPool::operator delete(void* p, size_t sz)
{
  // enforce sz is even, to match operator new(), as required for some
  // compilers.
   sz = (sz+1) & ~0x1;
  // Divide by two and subtract one to get the slot number:
  int slot = (sz >> 1) - 1;

  if(slot >= RWMAXPOOLS || nstash[slot] >= RWPOOLSIZE)
    ::delete p;
  else
    stash[slot][nstash[slot]++] = p;
}

#endif

