#ifndef __RWVREF_H__
#define __RWVREF_H__

/*
 * RWVirtualRef: Reference counted virtual array reference.
 *
 * $Id: vref.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: vref.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:48  vriezen
 * Move all files to 6.1
 *
 * Revision 2.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.4  1993/04/12  12:35:17  jims
 * Now uses RWMutex class for locking
 *
 * Revision 2.3  1993/02/06  02:09:37  keffer
 * Made findLocation(), lock() and unlock() public.
 *
 * Revision 2.2  1993/01/29  20:26:17  myersn
 * add MT-safe reference-counting.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 * 
 *    Rev 1.2   04 Aug 1992 19:04:50   KEFFER
 * Destructor is now virtual.
 * 
 *    Rev 1.1   04 Jun 1992 14:49:22   KEFFER
 * Ported to Glock under Unix.
 * 
 *    Rev 1.0   11 Mar 1992 14:10:00   KEFFER
 * Initial revision.
 */

#ifndef __RWREF_H__
#  include "rw/ref.h"
#endif
#include "rw/vpage.h"
STARTWRAP
#include <stdlib.h>
ENDWRAP
#ifdef RW_MULTI_THREAD
#include "rw/mutex.h"
#endif

typedef unsigned RWPageSlot;

class RWExport RWVirtualRef : public RWReference {

public:

  RWVirtualRef(long length, size_t elementSize, RWVirtualPageHeap* h);
  RWVirtualRef(const RWVirtualRef&);	// Deep copy
  virtual ~RWVirtualRef();

  long			capacity() const	{return nSlots_*nPerPage_;}
  RWVirtualPageHeap*	heap() const		{return myHeap_;}
  long			length() const		{return length_;}
  void			slide(long start, long delta);
  void			setSlice(long start1, long extent1,
				RWVirtualRef& v, long start2, long extent2);
  void			reshape(long newLength);

public:

  // The following three functions are formally intended to be
  // protected, but to avoid many friend functions have been made part
  // of the public interface.

  void			findLocation(long, RWPageSlot&, unsigned&) const;
  void*			lock(RWPageSlot p)	{return myHeap_->lock(handles_[p]);}
  void			unlock(RWPageSlot p)	{myHeap_->unlock(handles_[p]);}

protected:

  // Neither augmentLeft nor augmentRight change the vector length.
  // They just add capacity to one side or the other.
  void			augmentLeft(long);	// Add capacity on left
  void			augmentRight(long);	// Add capacity on right
  void			init();
  unsigned		pages()			{return nSlots_;}
  unsigned		pageSize()		{return myHeap_->pageSize();}
  void			dirty(RWPageSlot p)	{myHeap_->dirty(handles_[p]);}

  // Supplied by specializing class:
  virtual void		conformalCopy(long start1, RWVirtualRef& v2, long start2, long N) = 0;

protected:

  // Protected data.
  // Note that the length of the vector handles_ is nSlots_.

  long			length_;	// Total number of elements
  long			baseIndex_;	// Start of actual data
  unsigned		nPerPage_;	// Number of elements per page
  RWVirtualPageHeap*	myHeap_;	// The heap I'm getting my pages from
  RWHandle*		handles_;	// Array of handles
  RWPageSlot		nSlots_;	// Length of handles_.

#ifdef RW_MULTI_THREAD
public:
  void addReference() { RWReference::addReference(mutexLock_); }
  unsigned removeReference()
    { return RWReference::removeReference(mutexLock_); }
private:
  RWMutex mutexLock_;
#endif

};

#endif	/* __RWVREF_H__ */
