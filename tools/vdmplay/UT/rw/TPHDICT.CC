
/*
 * Template definitions for RWTPtrHashDictionary<KP,VP> and RWTPtrHashDictionaryIterator<KP,VP>
 *
 * $Id: tphdict.cc,v 6.3 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tphdict.cc,v $
 * Revision 6.3  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.2  1994/06/21  00:42:37  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 1.8  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 1.7  1993/11/08  12:17:42  jims
 * Port to ObjectStore
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.3  1993/02/17  19:07:23  keffer
 * Now uses RWTPtrVector<T> instead of RWTValVector<T> for the buckets
 *
 * Revision 1.2  1993/02/07  22:32:54  keffer
 * Added copy constructor and assignment operator for iterator
 *
 * Revision 1.1  1993/01/29  20:46:34  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */


template <class KP, class VP>
RWTPtrHashDictionary<KP,VP>::RWTPtrHashDictionary
                             (
			       unsigned (*hashFun)(const KP&),
			       size_t size
			     )
  :  table_(size ? size : 2, rwnil),
     nitems_(0),
     hashFun_(hashFun)
{;}

template <class KP, class VP>
RWTPtrHashDictionary<KP,VP>::RWTPtrHashDictionary
                             (
			       const RWTPtrHashDictionary<KP,VP>& v
			     )
  :  table_(v.table_.length(), rwnil),
     nitems_(v.nitems_),
     hashFun_(v.hashFun_)
{
  size_t N = table_.length();
  for (size_t i=0; i<N; i++)
  {
    // Use copy constructor:
    if (v.table_(i))
      table_(i) = new RWTPtrSlistDictionary<KP,VP>(*v.table_(i));
  }
}

template <class KP, class VP> 
RWTPtrHashDictionary<KP,VP>::~RWTPtrHashDictionary() { clear(); }

template <class KP, class VP> RWTPtrHashDictionary<KP,VP>&
RWTPtrHashDictionary<KP,VP>::operator=(const RWTPtrHashDictionary<KP,VP>& v)
{
  if( this != &v)
  {
    clear();
    hashFun_ = v.hashFun_;	// Transfer hashing function
    size_t N;
    table_.reshape(N = v.table_.length());
    for (size_t i=0; i<N; i++)	// Fill the table as needed
    {
      table_(i) = v.table_(i) ? new RWTPtrSlistDictionary<KP,VP>(*v.table_(i)) : rwnil;
    }
    nitems_ = v.nitems_;	// Fix up counter
  }
  return *this;
}

template <class KP, class VP> VP*&
RWTPtrHashDictionary<KP,VP>::operator[](KP* key)
{
  size_t idx = hashIndex(key);
  RWTPtrSlistDictionary<KP,VP>* chain = table_(idx);

  if (chain==rwnil)
    table_(idx) = chain = new RWTPtrSlistDictionary<KP,VP>;

  size_t N = chain->entries();

  VP*& val = chain->operator[](key);
  if (N!=chain->entries()) nitems_++;	// Ugly, I know.

  return val;
}

template <class KP, class VP> void
RWTPtrHashDictionary<KP,VP>::applyToKeyAndValue(void (*applyFun)(KP*,VP*&,void*), void* a)
{
  size_t N = table_.length();
  for (size_t i=0; i<N; i++)
  {
    if (table_(i))
      table_(i)->applyToKeyAndValue(applyFun, a);
  }
}

template <class KP, class VP> void
RWTPtrHashDictionary<KP,VP>::clear()
{
  size_t i = table_.length();
  while (i--)
  {
    delete table_(i);		// Always ok to delete nil
    table_(i) = rwnil;
  }

  nitems_ = 0;
}

template <class KP, class VP> void
RWTPtrHashDictionary<KP,VP>::clearAndDestroy()
{
  size_t i = table_.length();
  while (i--)
  {
    if (table_(i))
    {
      table_(i)->clearAndDestroy();
      delete table_(i);
      table_(i) = rwnil;
    }
  }

  nitems_ = 0;
}

template <class KP, class VP> RWBoolean
RWTPtrHashDictionary<KP,VP>::contains(const KP* key) const
{
  RWTPtrSlistDictionary<KP,VP>* chain = table_(hashIndex(key));
  return chain ? chain->contains(key) : FALSE;
}

template <class KP, class VP> KP*
RWTPtrHashDictionary<KP,VP>::find(const KP* key) const
{
  RWTPtrSlistDictionary<KP,VP>* chain = table_(hashIndex(key));
  return chain ? chain->find(key) : rwnil;
}

template <class KP, class VP> VP*
RWTPtrHashDictionary<KP,VP>::findValue(const KP* key) const
{
  RWTPtrSlistDictionary<KP,VP>* chain = table_(hashIndex(key));
  return chain ? chain->findValue(key) : rwnil;
}

template <class KP, class VP> KP*
RWTPtrHashDictionary<KP,VP>::findKeyAndValue(const KP* key, VP*& retVal) const
{
  RWTPtrSlistDictionary<KP,VP>* chain = table_(hashIndex(key));
  return chain ? chain->findKeyAndValue(key, retVal) : rwnil;
}

template <class KP, class VP> KP*
RWTPtrHashDictionary<KP,VP>::remove(const KP* key)
{
  RWTPtrSlistDictionary<KP,VP>* chain = table_(hashIndex(key));
  KP* ret = chain ? chain->remove(key) : rwnil;
  if (ret) nitems_--;
  return ret;
}

template <class KP, class VP> void
RWTPtrHashDictionary<KP,VP>::resize(size_t N)
{
 if (table_.length() == N) return; // save the effort

#ifdef RWDEBUG
  size_t oldNitems = nitems_;
#endif

  // Save old values -- NB: this involves a copy
  RWTPtrVector<RWTPtrSlistDictionary<KP,VP> > tempTable = table_;

  table_.reshape(N);		// Resize internal table
  table_  = rwnil;		// Zero it
  nitems_ = 0;

  // Now iterate through the old collection, inserting each item
  for (size_t i=0; i<tempTable.length(); i++)
  {
    if (tempTable(i))
    {
      RWTPtrSlistDictionaryIterator<KP,VP> next(*tempTable(i));
      while ( ++next )
	insertKeyAndValue(next.key(), next.value());
      delete tempTable(i);
    }
  }
#ifdef RWDEBUG
  assert(nitems_==oldNitems);	// Make sure we got 'em all.
#endif
}

/****************************************************************
 *								*
 *     Definitions for RWTPtrHashDictionaryIterator<KP,VP>	*
 *								*
 ****************************************************************/

template <class KP, class VP>
RWTPtrHashDictionaryIterator<KP,VP>::RWTPtrHashDictionaryIterator
(
 RWTPtrHashDictionary<KP,VP>& d
)
  :  myDict_(&d),
     idx_(RW_NPOS),
     iterator_(rwnil)
{
  reset();
}

// Copy constructor:
template <class KP, class VP>
RWTPtrHashDictionaryIterator<KP,VP>::RWTPtrHashDictionaryIterator
(
 const RWTPtrHashDictionaryIterator<KP,VP>& h
)
  :  myDict_(h.myDict_),
     idx_(h.idx_)
{
  iterator_ = h.iterator_ ? new RWTPtrSlistDictionaryIterator<KP,VP>(*h.iterator_) : rwnil;
}

// Assignment operator:
template <class KP, class VP> RWTPtrHashDictionaryIterator<KP,VP>&
RWTPtrHashDictionaryIterator<KP,VP>::operator=
(
 const RWTPtrHashDictionaryIterator<KP,VP>& h
)
{
  if (this != &h)
  {
    delete iterator_;
    myDict_  = h.myDict_;
    idx_     = h.idx_;
    iterator_ = h.iterator_ ? new RWTPtrSlistDictionaryIterator<KP,VP>(*h.iterator_) : rwnil;
  }
  return *this;
}

template <class KP, class VP> RWBoolean
RWTPtrHashDictionaryIterator<KP,VP>::operator++()
{
  while (iterator_ && ++(*iterator_)==FALSE)
  {
    nextChain();
  }
  return iterator_!=rwnil;
}

template <class KP, class VP> void
RWTPtrHashDictionaryIterator<KP,VP>::reset()
{
  delete iterator_;
  iterator_ = rwnil;

  // Advance to the first non-empty bucket:
  for (idx_=0; idx_ < myDict_->table_.length(); idx_++)
  {
    if (myDict_->table_(idx_))
    {
      iterator_ = new RWTPtrSlistDictionaryIterator<KP,VP>(*myDict_->table_(idx_));
      return;
    }
  }
}

template <class KP, class VP> void
RWTPtrHashDictionaryIterator<KP,VP>::reset(RWTPtrHashDictionary<KP,VP>& t)
{
  myDict_ = &t;
  reset();
}

// Advance the iterator to work on the next chain.
template <class KP, class VP> void
RWTPtrHashDictionaryIterator<KP,VP>::nextChain()
{
  while (++idx_ < myDict_->table_.length())
  {
    if (myDict_->table_(idx_))
    {
      if (iterator_) 
	iterator_->reset(*myDict_->table_(idx_));
      else           
	iterator_ = new RWTPtrSlistDictionaryIterator<KP,VP>(*myDict_->table_(idx_));
      return;
    }
  }

  delete iterator_;		// Invalidate old iterator
  iterator_ = rwnil;

}

