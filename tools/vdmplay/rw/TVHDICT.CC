
/*
 * Template definitions for RWTValHashDictionary<K,V> and RWTValHashDictionaryIterator<K,V>
 *
 * $Id: tvhdict.cc,v 6.3 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tvhdict.cc,v $
 * Revision 6.3  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.2  1994/06/21  00:48:01  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 6.1  1994/04/15  19:48:18  vriezen
 * Move all files to 6.1
 *
 * Revision 1.8  1993/11/08  14:57:02  jims
 * Port to ObjectStore
 *
 * Revision 1.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.6  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.5  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.4  1993/02/17  19:46:03  keffer
 * Now uses RWTPtrVector<T> rather than RWTValVector<T*>
 *
 * Revision 1.3  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
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

template <class K, class V>
RWTValHashDictionary<K,V>::RWTValHashDictionary
                           (
			     unsigned (*hashFun)(const K&),
			     size_t size
			   )
  :  table_(size ? size : 2, rwnil), // Minimum of two buckets
     nitems_(0),
     hashFun_(hashFun)
{;}

template <class K, class V>
RWTValHashDictionary<K,V>::RWTValHashDictionary(const RWTValHashDictionary<K,V>& v)
  :  table_(v.table_.length(), rwnil),
     nitems_(v.nitems_),
     hashFun_(v.hashFun_)
{
  size_t N = table_.length();
  for (size_t i=0; i<N; i++)
  {
    // Use copy constructor:
    if (v.table_(i))
      table_(i) = new RWTValSlistDictionary<K,V>(*v.table_(i));
  }
}

template <class K, class V>
RWTValHashDictionary<K,V>::~RWTValHashDictionary() { clear(); }

template <class K, class V> RWTValHashDictionary<K,V>&
RWTValHashDictionary<K,V>::operator=(const RWTValHashDictionary<K,V>& v)
{
  if (this != &v)
  {
    clear();
    hashFun_ = v.hashFun_;	// Transfer hashing function
    size_t N;
    table_.reshape(N = v.table_.length());
    for (size_t i=0; i<N; i++)	// Fill the table as needed
    {
      table_(i) = v.table_(i) 
	              ? new RWTValSlistDictionary<K,V>(*v.table_(i)) : rwnil;
    }
    nitems_ = v.nitems_;	// Fix up counter
  }
  return *this;
}

template <class K, class V> V&
RWTValHashDictionary<K,V>::operator[](const K& key)
{
  size_t idx = hashIndex(key);
  RWTValSlistDictionary<K,V>* chain = table_(idx);

  if (chain==rwnil)
    table_(idx) = chain = new RWTValSlistDictionary<K,V>;

  size_t N = chain->entries();

  V& val = chain->operator[](key);
  if (N!=chain->entries()) nitems_++;	// Ugly, I know.

  return val;
}

template <class K, class V> void
RWTValHashDictionary<K,V>::applyToKeyAndValue(void (*applyFun)(const K&, V&, void*), void* a)
{
  size_t N = table_.length();
  for (size_t i=0; i<N; i++)
  {
    if (table_(i))
      table_(i)->applyToKeyAndValue(applyFun, a);
  }
}

template <class K, class V> void
RWTValHashDictionary<K,V>::clear()
{
  size_t i = table_.length();
  while (i--)
  {
    delete table_(i);
    table_(i) = rwnil;
  }

  nitems_ = 0;
}

template <class K, class V> RWBoolean
RWTValHashDictionary<K,V>::contains(const K& key) const
{
  RWTValSlistDictionary<K,V>* chain = table_(hashIndex(key));
  return chain ? chain->contains(key) : FALSE;
}

template <class K, class V> RWBoolean
RWTValHashDictionary<K,V>::find(const K& key, K& retKey) const
{
  RWTValSlistDictionary<K,V>* chain = table_(hashIndex(key));
  return chain ? chain->find(key, retKey) : FALSE;
}

template <class K, class V> RWBoolean
RWTValHashDictionary<K,V>::findValue(const K& key, V& retVal) const
{
  RWTValSlistDictionary<K,V>* chain = table_(hashIndex(key));
  return chain ? chain->findValue(key, retVal) : FALSE;
}

template <class K, class V> RWBoolean
RWTValHashDictionary<K,V>::findKeyAndValue(const K& key, K& retKey, V& retVal) const
{
  RWTValSlistDictionary<K,V>* chain = table_(hashIndex(key));
  return chain ? chain->findKeyAndValue(key, retKey, retVal) : FALSE;
}

template <class K, class V> RWBoolean
RWTValHashDictionary<K,V>::remove(const K& key)
{
  RWTValSlistDictionary<K,V>* chain = table_(hashIndex(key));
  if (chain)
  {
    if (chain->remove(key)) {nitems_--; return TRUE;}
  }
  return FALSE;
}

template <class K, class V> void
RWTValHashDictionary<K,V>::resize(size_t N)
{
 if (table_.length() == N) return; // save the effort

#ifdef RWDEBUG
  size_t oldNitems = nitems_;
#endif

  // Save old values -- NB: this involves a copy
  RWTPtrVector<RWTValSlistDictionary<K,V> > tempTable = table_;

  table_.reshape(N);		// Resize internal table
  table_  = rwnil;		// Zero it
  nitems_ = 0;

  // Now iterate through the old collection, inserting each item
  for (size_t i=0; i<tempTable.length(); i++)
  {
    if (tempTable(i))
    {
      RWTValSlistDictionaryIterator<K,V> next(*tempTable(i));
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
 *     Definitions for RWTValHashDictionaryIterator<K,V>	*
 *								*
 ****************************************************************/

template <class K, class V>
RWTValHashDictionaryIterator<K,V>::RWTValHashDictionaryIterator(RWTValHashDictionary<K,V>& d) :
  myDict_(&d),
  idx_(RW_NPOS),
  iterator_(rwnil)
{
  nextChain();	// Advance to the first chain
}

// Copy constructor:
template <class K, class V>
RWTValHashDictionaryIterator<K,V>::RWTValHashDictionaryIterator
(
 const RWTValHashDictionaryIterator<K,V>& h
)
  :  myDict_(h.myDict_),
     idx_(h.idx_)
{
  iterator_ = h.iterator_ 
              ? new RWTValSlistDictionaryIterator<K,V>(*h.iterator_) : rwnil;
}

// Assignment operator:
template <class K, class V> RWTValHashDictionaryIterator<K,V>&
RWTValHashDictionaryIterator<K,V>::operator=
(
 const RWTValHashDictionaryIterator<K,V>& h
)
{
  if (this != &h)
  {
    delete iterator_;
    myDict_  = h.myDict_;
    idx_     = h.idx_;
    iterator_ = h.iterator_ 
                ? new RWTValSlistDictionaryIterator<K,V>(*h.iterator_) : rwnil;
  }
  return *this;
}

template <class K, class V> RWBoolean
RWTValHashDictionaryIterator<K,V>::operator++()
{
  while (iterator_ && ++(*iterator_)==FALSE)
  {
    nextChain();
  }
  return iterator_!=rwnil;
}

template <class K, class V> void
RWTValHashDictionaryIterator<K,V>::reset()
{
  idx_ = RW_NPOS;
  nextChain();
}

template <class K, class V> void
RWTValHashDictionaryIterator<K,V>::reset(RWTValHashDictionary<K,V>& t)
{
  myDict_ = &t;
  idx_ = RW_NPOS;
  nextChain();
}

// Advance the iterator to work on the next chain.
template <class K, class V> void
RWTValHashDictionaryIterator<K,V>::nextChain()
{
  while (++idx_ < myDict_->table_.length())
  {
    if (myDict_->table_(idx_))
    {
      if (iterator_) 
	iterator_->reset(*myDict_->table_(idx_));
      else           
	iterator_ = new 
	            RWTValSlistDictionaryIterator<K,V>(*myDict_->table_(idx_));
      return;
    }
  }
  delete iterator_;
  iterator_ = rwnil;
}
