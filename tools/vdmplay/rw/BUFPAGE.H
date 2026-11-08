#ifndef __RWBUFPAGE_H__
#define __RWBUFPAGE_H__

/*
 * RWBufferedPageHeap: A virtual page heap accessed through a buffer
 *
 * $Id: bufpage.h,v 6.2 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: bufpage.h,v $
 * Revision 6.2  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:31  vriezen
 * Move all files to 6.1
 *
 * Revision 2.4  1994/02/15  18:37:29  jims
 * Change return types from unsigned to size_t
 *
 * Revision 2.3  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.2  1993/03/23  02:45:12  keffer
 * Changed variable names; eliminated int to unsigned conversions.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.0   11 Mar 1992 14:09:58   KEFFER
 * Initial revision.
 */

#include "rw/vpage.h"
STARTWRAP
#include <stddef.h>
ENDWRAP

class RWExport RWBufferedPageHeap : public RWVirtualPageHeap {
  unsigned		nBuffers_;	// Number of buffers (each is pageSize() big)
  RWvoid*		buffers_;	// Points to an array of pointers to buffers
  RWHandle*		handles_;	// Page handle associated with each buffer
  short*		lockCounts_;	// Lock count for each buffer
  unsigned*		age_;		// Age of buffer since accessed
  RWBoolean*		dirty_;		// Whether this buffer has changed since swap in
protected:
  size_t		ageAndFindHandle(RWHandle);	// Find slot for given handle and age all slots
  size_t		findHandle(RWHandle);		// Find slot for given handle
  size_t		findUnusedSlot();		// Find an unused slot
  size_t		swapPageIn(RWHandle);		// Swap in page with given handle
  size_t		swapOutLRUSlot();		// Swap out the Least Recently Used page
  virtual RWBoolean	swapIn(RWHandle, void*)  = 0;	// Supplied by specializing class
  virtual RWBoolean	swapOut(RWHandle, void*) = 0;
public:
  RWBufferedPageHeap(unsigned pgsize, unsigned nbufs=10);
  virtual		~RWBufferedPageHeap();

  RWBoolean		isValid()		{return buffers_!=NULL;}

  // Inherited from RWVirtualPageHeap:
  virtual RWHandle	allocate()           = 0;	// Allocate a page
  virtual void		deallocate(RWHandle);		// Deallocate it
  virtual void		dirty(RWHandle);		// Declare page as dirty
  virtual void*		lock(RWHandle);			// Lock a page
  virtual void		unlock(RWHandle);		// Unlock a page
};

#endif	/* __RWBUFPAGE_H__ */
