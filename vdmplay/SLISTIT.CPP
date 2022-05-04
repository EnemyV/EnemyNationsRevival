/*
 * RWSlistIterator
 *
 * $Id: slistit.cpp,v 1.2 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1990, 1991, 1992, 1992, 1993.
 * This software is subject to copyright protection under the laws
 * of the United States and other countries.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/SLISTIT.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 1.2  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 1.1  1993/05/14  21:20:30  dealys
 * Initial revision
 *
 * 
 */

#include "rw/slist.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: slistit.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#ifndef RW_NO_CPP_RECURSION
# define new rwnew
#endif


/********************************************************
*							*
*	     RWSlistIterator definitions		*
*							*
********************************************************/

void*
RWSlistIterator::operator+=(size_t n)
{
  void* p = key();
  while (n--)
    p = ++(*this);
  return p;
}

void*
RWSlistIterator::findNext(RWtestGeneric tst, const void* x)
{
  RWPRECONDITION( tst!=rwnil );

  void* p;
  while ( (p = (*this)()) != 0 )
    if ( (*tst)(p, x) )
      return p;
  return rwnil;
}

void*
RWSlistIterator::findNextReference(const void* a)
{
  void* p;
  while ( (p = (*this)()) != 0 )	// This will advance iterator.
    if ( p==a )				// Is this the victim?
      return p;
  return rwnil;
}

void*
RWSlistIterator::insertAfterPoint(void* a)
{
  RWIsvSlistIterator::insertAfterPoint(new RWPSlink(a));
  return a;
}

void*
RWSlistIterator::removeNext(RWtestGeneric testFun, const void* x)
{
  RWPRECONDITION( testFun!=rwnil );

  while (cursor()->next() != container()->tailLink())
  {
    if ( testFun(cursor()->next()->info_, x) ) // hit?
    {
      return RWSlist::peel(container()->removeRight(cursor()));
    }
    advance();
  }
  return rwnil;
}

/*
 * Remove first occurrence of the pointer "a".
 */
void*
RWSlistIterator::removeNextReference(const void* a)
{
  while (cursor()->next() != container()->tailLink())
  {
    if ( cursor()->next()->info_ == a ) // hit?
    {
      return RWSlist::peel(container()->removeRight(cursor()));
    }
    advance();
  }
  return rwnil;
}


