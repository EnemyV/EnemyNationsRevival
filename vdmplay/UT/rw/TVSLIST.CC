
/*
 * Template definitions for RWTValSlist<T> and RWTValSlistIterator<T>
 *
 * $Id: tvslist.cc,v 6.4 1994/07/26 19:11:28 jims Exp $
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
 * $Log: tvslist.cc,v $
 * Revision 6.4  1994/07/26  19:11:28  jims
 * Fix copy ctor to use passed in argument instead of *this
 *
 * Revision 6.3  1994/07/12  20:05:34  vriezen
 * Updated Copyright
 *
 * Revision 6.2  1994/07/05  21:56:52  nevis
 * Fixes bug #117 by defining explicitly a copy constructor.
 *
 * Revision 6.2  1994/07/05  20:57:31  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.1  1994/04/15  19:48:33  vriezen
 * Move all files to 6.1
 *
 * Revision 1.6  1993/11/08  21:14:01  jims
 * Port to ObjectStore
 *
 * Revision 1.5  1993/09/10  02:18:28  keffer
 * apply() now takes a function with a non-const reference, allowing
 * modification of the contained value.
 *
 * Revision 1.4  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.3  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.2  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 1.1  1993/01/27  21:38:32  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */


// Copy constructor:
template <class T> RWTValSlist<T>::RWTValSlist(const RWTValSlist<T>& s)
{
  // Construct an iterator, casting away "constness"
  // (which we promise to honor anyway):
  RWTValSlistIterator<T> next((RWTValSlist<T>&)s);

  while (++next)
    append(next.key());

  RWPOSTCONDITION(s.entries()==entries());
}

template <class T> RWTValSlist<T>&
RWTValSlist<T>::operator=(const RWTValSlist<T>& s)
{
  if (this!=&s)
  {
    clear();
    // Construct an iterator, casting away "constness"
    // (which we promise to honor anyway):
    RWTValSlistIterator<T> next((RWTValSlist<T>&)s);

    while (++next)
      append(next.key());

  }
  RWPOSTCONDITION(s.entries()==entries());
  return *this;
}

template <class T> void
RWTValSlist<T>::append(const T& a)
{
  RWTIsvSlist<RWTValSlink<T> >::append(new RWTValSlink<T>(a));
}

template <class T> void
RWTValSlist<T>::apply(void (*applyFun)(T&, void*), void* d)
{
  RWTValSlink<T>* link = firstLink();
  while (link != tailLink())
  {
    applyFun(link->info_, d);	// Apply the function
    link = link->next();	// Advance
  }
}

template <class T> RWBoolean
RWTValSlist<T>::contains(const T& val) const
{
  return findLeftVal(val)!=rwnil;
}

template <class T> RWBoolean
RWTValSlist<T>::contains(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  return findLeftFun(testFun, d)!=rwnil;
}
  
template <class T> RWBoolean
RWTValSlist<T>::find(const T& val, T& k) const
{
  RWTValSlink<T>* link = firstLink();
  while (link!=tailLink())
  {
    if (link->info_ == val)
    {
      k = link->info_;
      return TRUE;
    }
    link = link->next();	// Advance
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValSlist<T>::find(RWBoolean (*testFun)(const T&, void*), void* d, T& k) const
{
  RWPRECONDITION(testFun!=rwnil);
  RWTValSlink<T>* link = firstLink();
  while (link!=tailLink())
  {
    if (testFun(link->info_, d))
    {
      k = link->info_;
      return TRUE;
    }
    link = link->next();	// Advance
  }
  return FALSE;
}

template <class T> size_t
RWTValSlist<T>::index(const T& val) const
{
  size_t count = 0;
  RWTValSlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (link->info_ == val)
      return count;
    ++count;
    link = link->next();	// Advance
  }
  return RW_NPOS;
}

template <class T> size_t
RWTValSlist<T>::index(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTValSlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_, d))
      return count;
    ++count;
    link = link->next();	// Advance
  }
  return RW_NPOS;
}

template <class T> void
RWTValSlist<T>::insert(const T& a)
{
  RWTIsvSlist<RWTValSlink<T> >::insert(new RWTValSlink<T>(a));
}

template <class T> void
RWTValSlist<T>::insertAt(size_t i, const T& a)
{
  RWTIsvSlist<RWTValSlink<T> >::insertAt(i, new RWTValSlink<T>(a));
}

template <class T> size_t
RWTValSlist<T>::occurrencesOf(const T& val) const
{
  size_t count = 0;
  RWTValSlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (link->info_ == val)
      ++count;
    link = link->next();	// Advance
  }
  return count;
}

template <class T> size_t
RWTValSlist<T>::occurrencesOf(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTValSlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_, d))
      ++count;
    link = link->next();	// Advance
  }
  return count;
}

template <class T> void
RWTValSlist<T>::prepend(const T& a)
{
  RWTIsvSlist<RWTValSlink<T> >::prepend(new RWTValSlink<T>(a));
}


// Non-const function allows us to use a sentinel:
template <class T> RWBoolean
RWTValSlist<T>::remove(const T& val)
{
  RWPRECONDITION(lastLink()->next()==tailLink());

  // Set up a sentinel:
  RWTValSlink<T> newTail(val);
  lastLink()->next_ = &newTail;

  // Now search for it:
  RWTValSlink<T>* link = headLink();
  while (!(link->next()->info_ == val))
    link = link->next();

  // Reset the pointer in the last link:
  lastLink()->next_ = tailLink();

  if (link != lastLink())
  {
    RWPOSTCONDITION(link->next()->info_ == val);
    delete removeRight(link);
    return TRUE;
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValSlist<T>::remove(RWBoolean (*testFun)(const T&, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  RWTValSlink<T>* link = removeRight(findLeftFun(testFun, d));
  if (link)
  {
    RWPOSTCONDITION( testFun(link->info_, d) );
    delete link;
    return TRUE;
  }
  return FALSE;
}

template <class T> size_t
RWTValSlist<T>::removeAll(const T& val)
{
  size_t count = 0;
  RWTValSlink<T>* prev = headLink();
  while (prev != lastLink())
  {
    if (prev->next()->info_ == val)
    {
      // This will effectively advance the cursor--
      delete removeRight(prev);
      count++;
    }
    else
      prev = prev->next();
  }
  return count;
}

template <class T> size_t
RWTValSlist<T>::removeAll(RWBoolean (*testFun)(const T&, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTValSlink<T>* prev = headLink();
  while (prev != lastLink())
  {
    if (testFun(prev->next()->info_, d))
    {
      // This will effectively advance the cursor--
      delete removeRight(prev);
      count++;
    }
    else
    {
      prev = prev->next();
    }
  }
  return count;
}

/****************************************************************
 *								*
 *		RWTValSlist<T> protected functions		*
 *								*
 ****************************************************************/


/*
 * Find the link *before* the link containing the indicated value.
 */
template <class T> RWTValSlink<T>*
RWTValSlist<T>::findLeftVal(const T& val) const
{
  RWTValSlink<T>* link = headLink();
  while (link != lastLink())
  {
    if (link->next()->info_ == val)
      return link;
    link = link->next();
  }
  return rwnil;
}

/*
 * Find the link *before* the link that tests true.
 */
template <class T> RWTValSlink<T>*
RWTValSlist<T>::findLeftFun(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  RWTValSlink<T>* link = headLink();
  while (link != lastLink())
  {
    if (testFun(link->next()->info_, d))
      return link;
    link = link->next();
  }
  return rwnil;
}

/*
 * Extracts the value out of a link then throws the link away:
 */
template <class T> T
RWTValSlist<T>::peel(RWTValSlink<T>* link)
{
  RWPRECONDITION2(link, "RWTValSlist<T>::peel(RWTValSlink<T>*): nil link");

  T ret = link->info_;
  delete link;
  return ret;
}


/****************************************************************
 *								*
 *	Definitions for RWTValSlistIterator<T>			*
 *								*
 ****************************************************************/

template <class T> RWBoolean
RWTValSlistIterator<T>::findNext(const T& val)
{
  while (++(*this))
  {
    if (key()==val)
      return TRUE;
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValSlistIterator<T>::findNext(RWBoolean (*testFun)(const T&, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  while (++(*this))
  {
    if (testFun(key(),d))
      return TRUE;
  }
  return FALSE;
}

template <class T> void
RWTValSlistIterator<T>::insertAfterPoint(const T& a)
{
 RWTIsvSlistIterator<RWTValSlink<T> >::insertAfterPoint(new RWTValSlink<T>(a));
}

template <class T> RWBoolean
RWTValSlistIterator<T>::remove()
{
  RWTValSlink<T>* link = RWTIsvSlistIterator<RWTValSlink<T> >::remove();
  return link ? (delete link, TRUE) : FALSE;
}

template <class T> RWBoolean
RWTValSlistIterator<T>::removeNext(const T& val)
{
  while (cursor()->next() != container()->tailLink())
  {
    if (cursor()->next()->info_ == val)
    {
      delete container()->removeRight(cursor());
      return TRUE;
    }
    advance();
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValSlistIterator<T>::removeNext(RWBoolean (*testFun)(const T&, void*), void* d)
{
  while (cursor()->next() != container()->tailLink())
  {
    if (testFun(cursor()->next()->info_, d))
    {
      delete container()->removeRight(cursor());
      return TRUE;
    }
    advance();
  }
  return FALSE;
}

template <class T>
RWTValSlistIterator<T>::RWTValSlistIterator(const RWTValSlistIterator<T>& si)
  : RWTIsvSlistIterator<RWTValSlink<T> >(si)
{ ; }
