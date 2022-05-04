/*
 * Template definitions for RWTIsvSlist<TL> and RWTIsvSlistIterator<TL>
 *
 * $Id: tislist.cc,v 6.2 1994/07/12 20:05:34 vriezen Exp $
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
 * $Log: tislist.cc,v $
 * Revision 6.2  1994/07/12  20:05:34  vriezen
 * Updated Copyright
 *
 * Revision 6.1  1994/04/15  19:11:12  vriezen
 * Move all files to 6.1
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
 * Revision 1.2  1993/02/05  23:09:47  keffer
 * Corrected problem in RWTIsvSlist<TL>::remove(RWBoolean (*testFun)(const TL*, void*), void* d);
 *
 * Revision 1.1  1993/01/27  21:38:32  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

template <class TL> void
RWTIsvSlist<TL>::apply(void (*applyFun)(TL*, void*), void* d)
{
  TL* link = firstLink();
  while (link != tailLink())
  {
    applyFun(link, d);		// Apply the function
    link = (TL*)link->next();	// Advance
  }
}

template <class TL> void
RWTIsvSlist<TL>::clearAndDestroy()
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
RWTIsvSlist<TL>::find(RWBoolean (*testFun)(const TL*, void*), void* d) const
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
RWTIsvSlist<TL>::index(RWBoolean (*testFun)(const TL*, void*), void* d) const
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
RWTIsvSlist<TL>::occurrencesOf(RWBoolean (*testFun)(const TL*, void*), void* d) const
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

/*
 * Remove and return the first link for which the tester function
 * returns TRUE.
 */
template <class TL> TL*
RWTIsvSlist<TL>::remove(RWBoolean (*testFun)(const TL*, void*), void* d)
{
  RWPRECONDITION(testFun!=rwnil);

  TL* link = headLink();
  while (link != lastLink())
  {
    if (testFun((const TL*)link->next(), d))
      return removeRight(link);
    link = (TL*)link->next();
  }
  return rwnil;
}

/****************************************************************
 *								*
 *	Definitions for RWTIsvSlistIterator<TL>			*
 *								*
 ****************************************************************/

/*
 * Return first occurrence where the tester returns true.
 */
template <class TL> TL*
RWTIsvSlistIterator<TL>::findNext(RWBoolean (*testFun)(const TL*, void*), void* d)
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
 * Remove first occurrence where the tester returns true.
 * To get any decent efficiency out of this function with
 * a singly-linked list, we must remember the previous link.
 */
template <class TL> TL*
RWTIsvSlistIterator<TL>::removeNext(RWBoolean (*testFun)(const TL*, void*), void* d)
{
  while (cursor()->next() != container()->tailLink())
  {
    if (testFun((const TL*)cursor()->next(), d))
    {
      return container()->removeRight(cursor());
    }
    advance();
  }
  return rwnil;
}

