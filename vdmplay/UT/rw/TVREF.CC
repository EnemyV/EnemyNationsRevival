/*
 * Template definitions for RWTVirtualRef<T>
 *
 * $Id: tvref.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tvref.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:48:26  vriezen
 * Move all files to 6.1
 *
 * Revision 1.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.3  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 1.2  1993/02/06  02:05:21  keffer
 * Removed unused variable.
 *
 * Revision 1.1  1993/01/29  20:46:20  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

template <class T>
RWTVirtualRef<T>::RWTVirtualRef(long size, RWVirtualPageHeap* heap)
  :  RWVirtualRef(size, sizeof(T), heap)
{
}

// Copy constructor: use copy semantics
template <class T>
RWTVirtualRef<T>::RWTVirtualRef(RWTVirtualRef<T>& v)
  :  RWVirtualRef(v)
{
  // Painful, but unavoidable w. copy semantics!
  conformalCopy(0, (RWVirtualRef&)v, 0, length_);
}

// Return the value at index idx
template <class T> T
RWTVirtualRef<T>::val(long idx)
{
  RWPageSlot slot;
  unsigned rem;
  findLocation(idx, slot, rem);
  T* p = (T*)lock(slot);
  T ret = p[rem];
  unlock(slot);
  return ret;
}

// Set the item at index idx to "val"
template <class T> void
RWTVirtualRef<T>::set(long idx, const T& val)
{
  RWPageSlot slot;
  unsigned rem;
  findLocation(idx, slot, rem);
  T* p = (T*)lock(slot);
  p[rem] = val;
  dirty(slot);
  unlock(slot);
}

// Set "extent" items starting at index "start" to the value "val":
template <class T> void
RWTVirtualRef<T>::set(long start, long extent, const T& val)
{
  long stop = start + extent - 1L;
  RWPageSlot startSlot, stopSlot;
  unsigned startRem, stopRem;
  findLocation(start, startSlot, startRem);
  findLocation(stop,  stopSlot,  stopRem);

  for (RWPageSlot slot = startSlot; slot <= stopSlot; slot++)
  {
    unsigned startIdx = slot==startSlot ? startRem : 0;
    unsigned endIdx   = slot==stopSlot  ? stopRem  : nPerPage_-1;
    T* p = (T*)lock(slot);
    for (unsigned i = startIdx; i<=endIdx; i++)
    {
      p[i] = val;
    }
    dirty(slot);
    unlock(slot);
  }
}

/*
 * Copies the data in v2 starting at index start2 into self,
 * starting at index start1.
 * Logically:
 *
 *   for(i=0; i<N; i++)
 *     (*this)[i+start1] = v2[i+start2];
 *
 * although the copy is actually done by minimizing the number of page
 * locks and, hence, page swaps.
 *
 */
template <class T> void
RWTVirtualRef<T>::conformalCopy(long start1,
				RWVirtualRef& v2,
				long start2,
				long N)
{
  RWPageSlot startSlot1, stopSlot1, lockedSlot1, lockedSlot2, slot2;
  unsigned   startRem1,  stopRem1,  lockedRem2,  rem2;
  long stop1 = start1 + N - 1L;
  findLocation(start1, startSlot1, startRem1);
  findLocation(stop1,  stopSlot1,  stopRem1);

  long idx2 = start1 > start2 ? start2 : start2 + N - 1L;

  v2.findLocation(idx2, lockedSlot2, lockedRem2);
  T* p2 = (T*)v2.lock(lockedSlot2);

  if (start1 > start2)
  {

    // Work upwards (from left to right)
    for(lockedSlot1=startSlot1; lockedSlot1<=stopSlot1; lockedSlot1++)
    {
      unsigned leftIdx  = lockedSlot1==startSlot1 ? startRem1 : 0;
      unsigned rightIdx = lockedSlot1==stopSlot1  ? stopRem1  : nPerPage_-1;
      T* p1 = (T*)lock(lockedSlot1);
      for (unsigned i = leftIdx; i<=rightIdx; i++)
      {
        v2.findLocation(idx2++, slot2, rem2);
        if (lockedSlot2 != slot2){
          v2.unlock(lockedSlot2);
          p2 = (T*)v2.lock(lockedSlot2=slot2);
        }
        p1[i] = p2[rem2];
      }
      dirty(lockedSlot1);
      unlock(lockedSlot1);
    }
    v2.unlock(lockedSlot2);
  }
  else
  {

    unsigned countSlot = stopSlot1 - startSlot1 + 1;

    // Work downwards (from right to left)
    while (countSlot--)
    {
      lockedSlot1 = startSlot1 + countSlot;
      unsigned leftIdx  = lockedSlot1==startSlot1 ? startRem1 : 0;
      unsigned rightIdx = lockedSlot1==stopSlot1  ? stopRem1  : nPerPage_-1;
      unsigned countIdx = rightIdx - leftIdx + 1;
      T* p1 = (T*)lock(lockedSlot1);
      while (countIdx--)
      {
        v2.findLocation(idx2--, slot2, rem2);
        if (lockedSlot2 != slot2)
	{
          v2.unlock(lockedSlot2);
          p2 = (T*)v2.lock(lockedSlot2=slot2);
        }
        p1[rightIdx--] = p2[rem2];
      }
      dirty(lockedSlot1);
      unlock(lockedSlot1);
    }
    v2.unlock(lockedSlot2);
  }
}

