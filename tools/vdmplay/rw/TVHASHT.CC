
/*
 * Template definitions for RWTValHashTable<T> and RWTValHashTableIterator<T>
 *
 * $Id: tvhasht.cc,v 6.3 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tvhasht.cc,v $
 * Revision 6.3  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.2  1994/06/21  00:47:57  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 6.1  1994/04/15  19:48:13  vriezen
 * Move all files to 6.1
 *
 * Revision 1.10  1993/11/08  14:57:02  jims
 * Port to ObjectStore
 *
 * Revision 1.9  1993/09/15  18:44:44  keffer
 * Casts away constness in (safe) call to slist's apply().
 *
 * Revision 1.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.7  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.6  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.5  1993/02/17  19:46:03  keffer
 * Now uses RWTPtrVector<T> rather than RWTValVector<T*>
 *
 * Revision 1.4  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 1.3  1993/02/10  18:29:23  keffer
 * Added assertions.
 *
 * Revision 1.2  93/02/07  22:32:54  22:32:54  keffer (Tom Keffer)
 * Added copy constructor and assignment operator for iterator
 * 
 * Revision 1.1  1993/01/29  01:19:41  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

template <class T>
RWTValHashTable<T>::RWTValHashTable
                    (
		      unsigned (*hashFun)(const T&),
		      size_t size
		    )
  :  table_(size ? size : 2, rwnil), // Minimum of two buckets
     nitems_(0),
     hashFun_(hashFun)
{;}

template <class T>
RWTValHashTable<T>::RWTValHashTable(const RWTValHashTable<T>& v)
  :  table_(v.table_.length(), rwnil),
     nitems_(v.nitems_),
     hashFun_(v.hashFun_)
{
  size_t N = table_.length();
  for (size_t i=0; i<N; i++)
  {
    if (v.table_(i))
      table_(i) = new RWTValSlist<T>(*v.table_(i)); //Use list copy constructor
  }
}

template <class T> 
RWTValHashTable<T>::~RWTValHashTable() { clear(); }

template <class T> RWTValHashTable<T>&
RWTValHashTable<T>::operator=(const RWTValHashTable<T>& v)
{
  if(&v != this)
  {
    clear();	    
    hashFun_ = v.hashFun_;	// get the hashfunction
    size_t N;
    table_.reshape(N = v.table_.length()); // get the size, and use it
    for (size_t i=0; i<N; i++)		   // fill the table as needed
    {
      table_(i) = v.table_(i) ? new RWTValSlist<T>(*v.table_(i)) : rwnil;
    }
    nitems_ = v.nitems_;	// fix up counter
  }
  return *this;
}

template <class T> void
RWTValHashTable<T>::apply(void (*applyFun)(const T&, void*), void* a)
{
  size_t N = table_.length();
  for (size_t i=0; i<N; i++)
  {
    if (table_(i))
      table_(i)->apply((void (*)(T&, void*))applyFun,a);
  }
}

template <class T> void
RWTValHashTable<T>::clear()
{
  register size_t i = table_.length();
  while (i--)
  {
    delete table_(i);		// Always ok to delete nil
    table_(i) = rwnil;
  }
  nitems_ = 0;
}

template <class T> RWBoolean
RWTValHashTable<T>::contains(const T& val) const
{
  RWTValSlist<T>* p = table_(hashIndex(val));
  return p ? p->contains(val) : FALSE;
}

template <class T> RWBoolean
RWTValHashTable<T>::find(const T& val, T& retVal) const
{
  RWTValSlist<T>* p = table_(hashIndex(val));
  return p ? p->find(val, retVal) : FALSE;
}

template <class T> void
RWTValHashTable<T>::insert(const T& val)
{
  size_t idx = hashIndex(val);
  RWASSERT(idx<table_.length());

  // Allocate a chain if one doesn't already exist:
  if (table_(idx)==rwnil)
    table_(idx) = new RWTValSlist<T>;

  table_(idx)->insert(val);	// Insert into the chain

  nitems_++;			// Update count
}

template <class T> size_t
RWTValHashTable<T>::occurrencesOf(const T& val) const
{
  RWTValSlist<T>* p = table_(hashIndex(val));
  return  p ? p->occurrencesOf(val) : 0;
}

template <class T> RWBoolean
RWTValHashTable<T>::remove(const T& val)
{
  RWTValSlist<T>* p = table_(hashIndex(val));
  RWBoolean ret = p ? p->remove(val) : FALSE;
  if (ret) nitems_--;
  return ret;
}

template <class T> size_t
RWTValHashTable<T>::removeAll(const T& val)
{
  RWTValSlist<T>* p = table_(hashIndex(val));
  size_t count = p ? p->removeAll(val) : 0;
  nitems_ -= count;
  return count;
}

template <class T> void
RWTValHashTable<T>::resize(size_t N)
{
  if( table_.length() == N) return; // save the effort

  // Save old values -- NB: this involves a copy
  RWTPtrVector<RWTValSlist<T> > tempTable = table_;

  table_.reshape(N);		// Resize internal table
  table_  = rwnil;		// Zero it
  nitems_ = 0;

  // Now iterate through the old collection, inserting each item
  for (size_t i=0; i<tempTable.length(); i++)
  {
    if (tempTable(i))
    {
      RWTValSlistIterator<T> next(*tempTable(i));
      while ( ++next )
	insert(next.key());
      delete tempTable(i);
    }
  }
}

// protected methods

// Insert a value only if it doesn't already exist in the collection
// (this function is useful for implementing sets).
template <class T> RWBoolean
RWTValHashTable<T>::insertOnce(const T& val)
{
  size_t idx = hashIndex(val);
  RWASSERT(idx<table_.length());

  if (table_(idx)==rwnil)
    table_(idx) = new RWTValSlist<T>;

  else if (table_(idx)->contains(val))
     return FALSE;

  table_(idx)->insert(val);
  nitems_++;
  return TRUE;
}

/****************************************************************
 *								*
 *	Definitions for RWTValHashTableIterator<T>		*
 *								*
 ****************************************************************/

template <class T>
RWTValHashTableIterator<T>::RWTValHashTableIterator(RWTValHashTable<T>& t)
  :  hashTable_(&t),
     idx_(RW_NPOS),
     iterator_(rwnil)
{
  reset();
}

template <class T>
RWTValHashTableIterator<T>::RWTValHashTableIterator(const RWTValHashTableIterator<T>& h)
  : hashTable_(h.hashTable_),
    idx_(h.idx_)
{
  iterator_ = h.iterator_ ? new RWTValSlistIterator<T>(*h.iterator_) : rwnil;
}

template <class T> RWTValHashTableIterator<T>&
RWTValHashTableIterator<T>::operator=(const RWTValHashTableIterator<T>& h)
{
  if (this != &h)
  {
    delete iterator_;
    hashTable_ = h.hashTable_;
    idx_       = h.idx_;
    iterator_ = h.iterator_ ? new RWTValSlistIterator<T>(*h.iterator_) : rwnil;
  }
  return *this;
}

template <class T> RWBoolean
RWTValHashTableIterator<T>::operator++()
{
  // either spin over empty slots or advance into a non-empty one
  while ( iterator_ && ++(*iterator_)==FALSE )
  {
    nextIterator();
  }
  return iterator_!=rwnil;
}

template <class T> void
RWTValHashTableIterator<T>::reset()
{
  delete iterator_;
  iterator_ = rwnil;

  // Advance to the first non-empty bucket:
  for (idx_=0; idx_ < hashTable_->table_.length(); idx_++)
  {
    if (hashTable_->table_(idx_))
    {
      iterator_ = new RWTValSlistIterator<T>(*hashTable_->table_(idx_));
      return;
    }
  }
}

template <class T> void
RWTValHashTableIterator<T>::reset(RWTValHashTable<T>& t)
{
  hashTable_ = &t;
  reset();
}


// private function ---

template <class T> void
RWTValHashTableIterator<T>::nextIterator()
{
  while (++idx_ < hashTable_->table_.length())
  {
    if (hashTable_->table_(idx_))
    {
      if (iterator_) 
	iterator_->reset(*hashTable_->table_(idx_));
      else           
	iterator_ = new RWTValSlistIterator<T>(*hashTable_->table_(idx_));
      return;
    }
  }

  delete iterator_;		// Invalidate old iterator
  iterator_ = rwnil;

}

