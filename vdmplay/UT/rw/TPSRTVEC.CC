/*
 * Template definitions for RWTPtrSortedVector<TP>
 *
 * $Id: tpsrtvec.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tpsrtvec.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:48:03  vriezen
 * Move all files to 6.1
 *
 * Revision 1.6  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 1.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.4  1993/07/08  03:42:18  keffer
 * Corrected logic error in RWDEBUG version of bsearch().
 *
 * Revision 1.3  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.1  1993/01/29  03:06:18  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */


template <class TP> size_t
RWTPtrSortedVector<TP>::index(const TP* item) const
{
  size_t idx;
  if(! bsearch(item,idx))  // Return RW_NPOS if item is not in collection:
    return RW_NPOS;

  // The item is in the collection.
  // Search downwards looking for the first instance:

  while (idx && *(*this)(idx-1) == *item)
    --idx;

  RWPOSTCONDITION(*(*this)(idx) == *item);
  RWPOSTCONDITION(idx>=0 && idx<nitems_);

  return idx;
}

template <class TP> void
RWTPtrSortedVector<TP>::insert(TP* p)
{
  // This algorithm was written such that only
  // the equality and less-than operator are used.

#ifdef RWDEBUG
  size_t count = occurrencesOf(p);
#endif

  size_t idx;
  if (bsearch(p,idx))
  {

    // A matching item was found.  Insert after the
    // last equal item.
    while (idx<nitems_ && *(*this)(idx) == *p)
      ++idx;
  }
  else
  {
    // No matching item found.  Search upward
    // for the first item greater than the value
    // and insert before it.
    while (idx<nitems_ && *(*this)(idx) < *p)
      ++idx;
  }

  insertAt(idx, p);

  RWPOSTCONDITION(isSorted() && occurrencesOf(p) == count+1);
}

template <class TP> size_t
RWTPtrSortedVector<TP>::occurrencesOf(const TP* p) const
{
  size_t iend;
  size_t istart = indexSpan(p, iend);
  return istart == RW_NPOS ? 0 : iend-istart+1;
}

/*
 * Remove and return the first occurrence of an object with the
 * same value as the object pointed to by "p".
 */
template <class TP> TP*
RWTPtrSortedVector<TP>::remove(const TP* p)
{
  size_t idx = index(p);
  return idx == RW_NPOS ? rwnil : removeAt(idx);
}

template <class TP> size_t
RWTPtrSortedVector<TP>::removeAll(const TP* p)
{
  size_t iend;
  size_t istart = indexSpan(p, iend);

  if (istart == RW_NPOS) return 0;
  iend++;
  size_t nremoved = iend-istart;

  // Do a "solid body" slide left of the remaining items in the collection:
  while (iend<nitems_)
    (*this)(istart++) = (*this)(iend++);

  nitems_ -= nremoved;
  RWPOSTCONDITION(!contains(p));
  return nremoved;
}

/****************************************************************
 *								*
 *	protected members of RWTPtrSortedVector<TP>		*
 *								*
 ****************************************************************/

template <class TP> RWBoolean
RWTPtrSortedVector<TP>::bsearch(const TP* key, size_t& idx) const
{
  idx = 0;
  if (entries())
  {
    size_t top = entries() - 1;
    size_t bottom = 0;

    while (top>bottom)
    {
      idx = (top+bottom) >> 1;
      // It is important that we use only the equality and less-than
      // operators:
      if (*key == *(*this)(idx))
        return TRUE;
      else if(*key < *(*this)(idx))
        top    = idx ? idx - 1 : 0u;
      else
        bottom = idx + 1;
    }
    return *key == *(*this)(idx=bottom);
  }
  // Not found:
  return FALSE;
}

template <class TP> size_t
RWTPtrSortedVector<TP>::indexSpan(const TP* key, size_t& end) const
{
  // Do a binary search to find the first match:
  size_t istart = index(key);

  if (istart != RW_NPOS)
  {
    // Found one.  Do a linear search, looking for the last match:
    end = istart;
    while ( end+1 < entries() && *((*this)(end+1)) == *key)
     ++end;
  }

  return istart;
}

#ifdef RWDEBUG
template <class TP> RWBoolean
RWTPtrSortedVector<TP>::isSorted() const
{
  for(size_t idx=0; idx<nitems_-1; idx++)
  {
    // Not sorted if the item at this index is not less
    // than and not equal to the item at the next index
    // (i.e., it is greater than).
    if(!(*(*this)(idx) < *(*this)(idx+1)) && !(*(*this)(idx) == *(*this)(idx+1)))
      return FALSE;
  }
  return TRUE;
}
#endif
