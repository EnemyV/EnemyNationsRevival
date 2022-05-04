/*
 * Template definitions for RWTPtrDlist<T> and RWTPtrDlistIterator<T>
 *
 * $Id: tpdlist.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tpdlist.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:47:38  vriezen
 * Move all files to 6.1
 *
 * Revision 1.7  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 1.6  1993/11/05  22:38:26  jims
 * Port to objectStore
 *
 * Revision 1.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.4  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.3  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.2  1993/01/28  01:35:03  keffer
 * Corrected error in removeNext()
 *
 * Revision 1.1  1993/01/27  21:38:32  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

template <class TP> 
RWTPtrDlist<TP>::RWTPtrDlist(const RWTPtrDlist<TP>& s)
{
  // Construct an iterator, casting away "constness"
  // (which we promise to honor anyway):
  RWTPtrDlistIterator<TP> next((RWTPtrDlist<TP>&)s);
  TP* p;

  while ((p = next())!=rwnil)
    append(p);

  RWPOSTCONDITION(s.entries()==entries());
}

template <class TP> RWTPtrDlist<TP>&
RWTPtrDlist<TP>::operator=(const RWTPtrDlist<TP>& s)
{
  if (this!=&s)
  {
    clear();
    // Construct an iterator, casting away "constness"
    // (which we promise to honor anyway):
    RWTPtrDlistIterator<TP> next((RWTPtrDlist<TP>&)s);
    TP* p;

    while ((p = next())!=rwnil)
      append(p);
  }

  RWPOSTCONDITION(s.entries()==entries());
  return *this;
}

template <class TP> void
RWTPtrDlist<TP>::append(TP* a)	
{
  RWTIsvDlist<RWTPtrDlink<TP> >::append(new RWTPtrDlink<TP>(a)); 
}

template <class TP> void
RWTPtrDlist<TP>::apply(void (*applyFun)(TP*, void*), void* d)
{
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    applyFun(link->info_, d);	// Apply the function
    link = link->next();	// Advance
  }
}

template <class TP> void
RWTPtrDlist<TP>::clearAndDestroy()
{
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    RWTPtrDlink<TP>* next = link->next();
    delete link->info_;
    delete link;
    link = next;
  }
  init();
}

template <class TP> RWBoolean
RWTPtrDlist<TP>::contains(const TP* p) const
{
  return findVal(p)!=rwnil;
}

template <class TP> RWBoolean
RWTPtrDlist<TP>::contains(RWBoolean (*testFun)(TP*, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  return findFun(testFun, d)!=rwnil;
}

template <class TP> TP*
RWTPtrDlist<TP>::find(const TP* p) const
{
  RWPRECONDITION(p != rwnil);

  RWTPtrDlink<TP>* link = firstLink();
  while (link!=tailLink())
  {
    if (*link->info_ == *p)
    {
      return link->info_;
    }
    link = link->next();	// Advance
  }
  return rwnil;
}

template <class TP> TP*
RWTPtrDlist<TP>::find(RWBoolean (*testFun)(TP*, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  RWTPtrDlink<TP>* link = firstLink();
  while (link!=tailLink())
  {
    if (testFun(link->info_, d))
    {
      return link->info_;
    }
    link = link->next();	// Advance
  }
  return rwnil;
}

template <class TP> size_t
RWTPtrDlist<TP>::index(const TP* p) const
{
  RWPRECONDITION(p != rwnil);

  size_t count = 0;
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (*link->info_ == *p)
      return count;
    ++count;
    link = link->next();	// Advance
  }
  return RW_NPOS;
}

template <class TP> size_t
RWTPtrDlist<TP>::index(RWBoolean (*testFun)(TP*, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_,d))
      return count;
    ++count;
    link = link->next();	// Advance
  }
  return RW_NPOS;
}


template <class TP> void
RWTPtrDlist<TP>::insert(TP* a)
{
  RWTIsvDlist<RWTPtrDlink<TP> >::insert(new RWTPtrDlink<TP>(a)); 
}

template <class TP> void
RWTPtrDlist<TP>::insertAt(size_t i, TP* a)
{
  RWTIsvDlist<RWTPtrDlink<TP> >::insertAt(i, new RWTPtrDlink<TP>(a)); 
}

template <class TP> size_t
RWTPtrDlist<TP>::occurrencesOf(const TP* p) const
{
  RWPRECONDITION(p != rwnil);

  size_t count = 0;
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (*link->info_ == *p)
      ++count;
    link = link->next();	// Advance
  }
  return count;
}

template <class TP> size_t
RWTPtrDlist<TP>::occurrencesOf(RWBoolean (*testFun)(TP*, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  size_t count = 0;
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_, d))
      ++count;
    link = link->next();	// Advance
  }
  return count;
}

template <class TP> void
RWTPtrDlist<TP>::prepend(TP* a)
{
  RWTIsvDlist<RWTPtrDlink<TP> >::prepend(new RWTPtrDlink<TP>(a)); 
}

template <class TP> TP*
RWTPtrDlist<TP>::remove(const TP* p)
{
  RWPRECONDITION(p != rwnil);
  RWPRECONDITION(lastLink()->next()==tailLink());

  TP* ret = rwnil;

  // Set up a sentinel (casting away const: DO NOT MODIFY *newTail.info_):
  RWTPtrDlink<TP> newTail((TP*) p);
  lastLink()->next_ = &newTail;

  // Now search for it:
  RWTPtrDlink<TP>* link = firstLink();
  while (!(*link->info_ == *p))
    link = link->next();

  // Reset the pointer in the last link:
  lastLink()->next_ = tailLink();

  if (link != &newTail)
  {
    RWTPtrDlink<TP>* victim = removeReference(link);
    RWASSERT(victim == link);
    ret = victim->info_;
    delete victim;
  }
  RWPOSTCONDITION(ret==rwnil || *ret == *p);
  return ret;
}

template <class TP> TP*
RWTPtrDlist<TP>::remove(RWBoolean (*testFun)(TP*, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  TP* ret = peel(removeReference(findFun(testFun, d)));

  RWPOSTCONDITION( ret==rwnil || testFun(ret, d) );

  return ret;
}

template <class TP> size_t
RWTPtrDlist<TP>::removeAll(const TP* p)
{
  RWPRECONDITION(p != rwnil);

  size_t count = 0;
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (*link->info_ == *p)
    {
      RWTPtrDlink<TP>* next = link->next();
      delete removeReference(link);
      link = next;
      count++;
    }
    else
      link = link->next();
  }
  return count;
}

template <class TP> size_t
RWTPtrDlist<TP>::removeAll(RWBoolean (*testFun)(TP*, void*), void* d)
{
  RWPRECONDITION(testFun != rwnil);

  size_t count = 0;
  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link->info_, d))
    {
      RWTPtrDlink<TP>* next = link->next();
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
 *	    RWTPtrDlist<TP> protected functions		   *
 *							   *
 ***********************************************************/

/*
 * Find the link containing the indicated value.
 */
template <class TP> RWTPtrDlink<TP>*
RWTPtrDlist<TP>::findVal(const TP* p) const
{
  RWPRECONDITION(p != rwnil);

  RWTPtrDlink<TP>* link = firstLink();
  while (link != tailLink())
  {
    if (*link->info_ == *p)
      return link;
    link = link->next();
  }
  return rwnil;
}

/*
 * Find the link that contains info_ that tests true.
 */
template <class TP> RWTPtrDlink<TP>*
RWTPtrDlist<TP>::findFun(RWBoolean (*testFun)(TP*, void*), void* d) const
{
  RWPRECONDITION(testFun!=rwnil);

  RWTPtrDlink<TP>* link = firstLink();
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
template <class TP> TP*
RWTPtrDlist<TP>::peel(RWTPtrDlink<TP>* link)
{
  if (!link) return rwnil;

  TP* ret = link->info_;
  delete link;
  return ret;
}

/****************************************************************
 *								*
 *	Definitions for RWTPtrDlistIterator<TP>			*
 *								*
 ****************************************************************/

template <class TP> TP*
RWTPtrDlistIterator<TP>::findNext(const TP* p)
{
  RWPRECONDITION(p != rwnil);

  TP* a;
  while ( (a = (*this)()) != rwnil)
  {
    if (*a == *p)
      return a;
  }
  return rwnil;
}

template <class TP> TP*
RWTPtrDlistIterator<TP>::findNext(RWBoolean (*testFun)(TP*, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  TP* a;
  while ( (a = (*this)()) != rwnil)
  {
    if (testFun(a, d))
      return a;
  }
  return rwnil;
}

template <class TP> void
RWTPtrDlistIterator<TP>::insertAfterPoint(TP* a)
{
  RWTIsvDlistIterator<RWTPtrDlink<TP> >::insertAfterPoint(new RWTPtrDlink<TP>(a));
}

template <class TP> TP*
RWTPtrDlistIterator<TP>::remove()
{
  return RWTPtrDlist<TP>::peel(RWTIsvDlistIterator<RWTPtrDlink<TP> >::remove());
}

template <class TP> TP*
RWTPtrDlistIterator<TP>::removeNext(const TP* p)
{
  RWPRECONDITION(p != rwnil);

  while (cursor()->next() != container()->tailLink())
  {
    if (*cursor()->next()->info_ == *p)
    {
      return RWTPtrDlist<TP>::peel(container()->removeReference(cursor()->next()));
    }
    advance();
  }
  return rwnil;
}

template <class TP> TP*
RWTPtrDlistIterator<TP>::removeNext(RWBoolean (*testFun)(TP*, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  while (cursor()->next() != container()->tailLink())
  {
    if (testFun(cursor()->next()->info_, d))
    {
      return RWTPtrDlist<TP>::peel(container()->removeReference(cursor()->next()));
    }
    advance();
  }
  return rwnil;
}

