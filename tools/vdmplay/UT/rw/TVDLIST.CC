
/*
 * Template definitions for RWTValDlist<T> and RWTValDlistIterator<T>
 *
 * $Id: tvdlist.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tvdlist.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:48:09  vriezen
 * Move all files to 6.1
 *
 * Revision 1.7  1993/11/08  13:49:36  jims
 * Port to ObjectStore
 *
 * Revision 1.6  1993/09/10  02:18:28  keffer
 * apply() now takes a function with a non-const reference, allowing
 * modification of the contained value.
 *
 * Revision 1.5  1993/07/05  21:20:25  randall
 * fixed memory link in remove( RWBoolean (*testFun) .... )
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

template <class T> RWTValDlist<T>::RWTValDlist(const RWTValDlist<T>& s)
{
  // Construct an iterator, casting away "constness"
  // (which we promise to honor anyway):
  RWTValDlistIterator<T> next((RWTValDlist<T>&)s);
  while (++next) append(next.key());
  RWPOSTCONDITION(s.entries()==entries());
}

template <class T> RWTValDlist<T>&
RWTValDlist<T>::operator=(const RWTValDlist<T>& s)
{
  if (this!=&s)
  {
    clear();
    // Construct an iterator, casting away "constness"
    // (which we promise to honor anyway):
    RWTValDlistIterator<T> next((RWTValDlist<T>&)s);
    while (++next) append(next.key());
  }
  RWPOSTCONDITION(s.entries()==entries());
  return *this;
}

template <class T> void
RWTValDlist<T>::append(const T& a)
{
  RWTIsvDlist<RWTValDlink<T> >::append(new RWTValDlink<T>(a));
}

template <class T> void
RWTValDlist<T>::apply(void (*applyFun)(T&, void*), void* d)
{
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    applyFun(link->info_, d);	// Apply the function
    link = link->next();	// Advance
  }
}

template <class T> RWBoolean
RWTValDlist<T>::contains(const T& val) const
{
  return findVal(val)!=rwnil;
}

template <class T> RWBoolean
RWTValDlist<T>::contains(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  return findFun(testFun, d)!=rwnil;
}

template <class T> RWBoolean
RWTValDlist<T>::find(const T& val, T& ret) const
{
  RWTValDlink<T>* link = findVal(val);
  return link ? (ret = link->info_, TRUE) : FALSE;
}

template <class T> RWBoolean
RWTValDlist<T>::find(RWBoolean (*testFun)(const T&, void*), void* d, T& ret) const
{
  RWPRECONDITION(testFun!=rwnil);

  RWTValDlink<T>* link = findFun(testFun, d);

  return link ? (ret = link->info_, TRUE) : FALSE;
}

template <class T> size_t
RWTValDlist<T>::index(const T& val) const
{
  size_t count = 0;
  RWTValDlink<T>* link = firstLink();
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
RWTValDlist<T>::index(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_,d))
      return count;
    ++count;
    link = link->next();	// Advance
  }
  return RW_NPOS;
}

template <class T> void
RWTValDlist<T>::insert(const T& a)
{
  RWTIsvDlist<RWTValDlink<T> >::insert(new RWTValDlink<T>(a)); 
}

template <class T> void
RWTValDlist<T>::insertAt(size_t i, const T& a)
{
  RWTIsvDlist<RWTValDlink<T> >::insertAt(i, new RWTValDlink<T>(a)); 
}

template <class T> size_t
RWTValDlist<T>::occurrencesOf(const T& val) const
{
  size_t count = 0;
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (link->info_ == val)
      ++count;
    link = link->next();	// Advance
  }
  return count;
}

template <class T> size_t
RWTValDlist<T>::occurrencesOf(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_, d))
      ++count;
    link = link->next();	// Advance
  }
  return count;
}

template <class T> void
RWTValDlist<T>::prepend(const T& a)
{
  RWTIsvDlist<RWTValDlink<T> >::prepend(new RWTValDlink<T>(a));
}

template <class T> RWBoolean
RWTValDlist<T>::remove(const T& val)
{
  RWPRECONDITION(lastLink()->next()==tailLink());

  // Set up a sentinel:
  RWTValDlink<T> newTail(val);
  lastLink()->next_ = &newTail;

  // Now search for it:
  RWTValDlink<T>* link = firstLink();
  while (!(link->info_ == val))
    link = link->next();

  // Reset the pointer in the last link:
  lastLink()->next_ = tailLink();

  if (link != &newTail)
  {
    RWPOSTCONDITION(link->info_ == val);
    delete removeReference(link);
    return TRUE;
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValDlist<T>::remove(RWBoolean (*testFun)(const T&, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  RWTValDlink<T>* link = removeReference(findFun(testFun, d));
  if (link)
  {
    RWPOSTCONDITION( testFun(link->info_, d) );
    delete link;
    return TRUE;
  }
  return FALSE;
}

template <class T> size_t
RWTValDlist<T>::removeAll(const T& val)
{
  size_t count = 0;
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (link->info_ == val)
    {
      RWTValDlink<T>* next = link->next();
      delete removeReference(link);
      link = next;
      count++;
    }
    else
      link = link->next();
  }
  return count;
}

template <class T> size_t
RWTValDlist<T>::removeAll(RWBoolean (*testFun)(const T&, void*), void* d)
{
  RWPRECONDITION(testFun != rwnil);

  size_t count = 0;
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_ ,d))
    {
      RWTValDlink<T>* next = link->next();
      delete removeReference(link);
      link = next;
      count++;
    }
    else
      link = link->next();
  }
  return count;
}

/***********************************************************
 *							   *
 *	    RWTValDlist<T> protected functions		   *
 *							   *
 ***********************************************************/

/*
 * Find the link containing the indicated value.
 */
template <class T> RWTValDlink<T>*
RWTValDlist<T>::findVal(const T& val) const
{
  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (link->info_ == val)
      return link;
    link = link->next();
  }
  return rwnil;
}

/*
 * Find the link that contains info_ that tests true.
 */
template <class T> RWTValDlink<T>*
RWTValDlist<T>::findFun(RWBoolean (*testFun)(const T&, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  RWTValDlink<T>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_, d))
      return link;
    link = link->next();
  }
  return rwnil;
}

/*
 * Extracts the value out of a link then throws the link away:
 */
template <class T> T
RWTValDlist<T>::peel(RWTValDlink<T>* link)
{
  RWPRECONDITION2(link, "RWTValDlist<T>::peel(RWTValDlink<T>*): nil link");

  T ret = link->info_;
  delete link;
  return ret;
}

/****************************************************************
 *								*
 *	Definitions for RWTValDlistIterator<T>			*
 *								*
 ****************************************************************/

template <class T> RWBoolean
RWTValDlistIterator<T>::findNext(const T& val)
{
  while (++(*this))
  {
    if (key()==val)
      return TRUE;
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValDlistIterator<T>::findNext(RWBoolean (*testFun)(const T&, void*), void* d)
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
RWTValDlistIterator<T>::insertAfterPoint(const T& a)
{
 RWTIsvDlistIterator<RWTValDlink<T> >::insertAfterPoint(new RWTValDlink<T>(a));
}

template <class T> RWBoolean
RWTValDlistIterator<T>::remove()
{
  RWTValDlink<T>* link = RWTIsvDlistIterator<RWTValDlink<T> >::remove();
  return link ? (delete link, TRUE) : FALSE;
}

template <class T> RWBoolean
RWTValDlistIterator<T>::removeNext(const T& val)
{
  while (cursor()->next() != container()->tailLink())
  {
    if (cursor()->next()->info_ == val)
    {
      delete container()->removeReference(cursor()->next());
      return TRUE;
    }
    advance();
  }
  return FALSE;
}

template <class T> RWBoolean
RWTValDlistIterator<T>::removeNext(RWBoolean (*testFun)(const T&, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  while (cursor()->next() != container()->tailLink())
  {
    if (testFun(cursor()->next()->info_ , d))
    {
      delete container()->removeReference(cursor()->next());
      return TRUE;
    }
    advance();
  }
  return FALSE;
}


