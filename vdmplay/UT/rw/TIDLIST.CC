/*
 * Template definitions for RWTIsvDlist<TL> and RWTIsvDlistIterator<TL>
 *
 * $Id: tidlist.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tidlist.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:11:09  vriezen
 * Move all files to 6.1
 *
 * Revision 1.4  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.3  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.2  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.1  1993/01/27  21:38:32  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

template <class TL> void
RWTIsvDlist<TL>::apply(void (*applyFun)(TL*, void*), void* p)
{
  TL* link = firstLink();
  while (link != tailLink())
  {
    (*applyFun)(link, p);	// Apply the function
    link = (TL*)link->next();	// Advance
  }
}
template <class TL> void
RWTIsvDlist<TL>::clearAndDestroy()
{
  TL* n;
  TL* link = firstLink();
  while (link != tailLink())
  {
    n = (TL*)link->next();
    delete link;
    link = n;
  }
  init();
}
  
template <class TL> TL*
RWTIsvDlist<TL>::find(RWBoolean (*testFun)(const TL*, void*), void* d) const
{
  TL* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link, d))
      return link;
    link = (TL*)link->next();
  }
  return rwnil;
}


template <class TL> size_t
RWTIsvDlist<TL>::index(RWBoolean (*testFun)(const TL*, void*), void* d) const
{
  size_t count = 0;
  const TL* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link, d))
      return count;
    link = (const TL*)link->next();
    ++count;
  }
  return RW_NPOS;
}

template <class TL> size_t
RWTIsvDlist<TL>::occurrencesOf(RWBoolean (*testFun)(const TL*, void*), void* d) const
{
  size_t count = 0;
  const TL* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link, d))
      ++count;
    link = (const TL*)link->next();
  }
  return count;
}

template <class TL> TL*
RWTIsvDlist<TL>::remove(RWBoolean (*testFun)(const TL*, void*), void* d)
{
  TL* link = firstLink();
  while (link != tailLink())
  {
    if (testFun(link, d))
      return removeReference(link);
    link = (TL*)link->next();
  }
  return rwnil;
}

/****************************************************************
 *								*
 *	Definitions for RWTIsvDlistIterator<TL>			*
 *								*
 ****************************************************************/


/*
 * Return first occurrence where the tester returns true.
 */
template <class TL> TL*
RWTIsvDlistIterator<TL>::findNext(RWBoolean (*testFun)(const TL*, void*), void* d)
{
  TL* p;
  while ( (p= (*this)()) != 0 )
  {
    if (testFun(p, d))
      return p;
  }
  return rwnil;
}

/*
 * Remove and return first occurrence where the tester returns true.
 * In a doubly-linked list, we are unencumbered by having to remember
 * the previous link.
 */
template <class TL> TL*
RWTIsvDlistIterator<TL>::removeNext(RWBoolean (*testFun)(const TL*, void*), void* d)
{
  return findNext(testFun, d) ? remove() : rwnil;
}

