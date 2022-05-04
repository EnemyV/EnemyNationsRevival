/*
 * RWIsvSlistIterator
 *
 * $Id: islistit.cpp,v 1.2 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1989, 1990, 1991, 1992, 1993.
 * This software is subject to copyright protection under the laws of
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/ISLISTIT.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 1.2  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 1.1  1993/05/14  20:50:24  dealys
 * Initial revision
 *
 * 
 */

/*
 * Member data head_ is a link that points to the actual start of
 * the list, or tail_ if there are no members of the list.  
 *
 * Member data last_ points to the last item in the list, or
 * to head_ if there are no items in the list.
 * Hence, appending a link is always done by attaching it after
 * the link pointed to by last_.
 *
 * Member data tail_ is a sentinel that can be used to find the
 * end of a list.  It always points to itself.
 */

#include "rw/islist.h"
#include "rw/toolerr.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: islistit.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#ifndef RW_NO_CPP_RECURSION
# define new rwnew
#endif


/********************************************************
*							*
*		RWIsvSlistIterator definitions		*
*							*
********************************************************/

/*
 * The variable "shere_" points to the "current position" of
 * the iterator (sometimes called "point" or the "cursor 
 * position").  When the iterator is constructed, shere_
 * points to the "head_" link of the list.  This link
 * points to the actual first link of the list.
 * Each call to operator++() advances the cursor one position.
 *
 */


/*
 * Advance iterator n links then return.
 */

RWIsvSlink*
RWIsvSlistIterator::operator+=(size_t n)
{
  RWIsvSlink* p = key();
  while (n--)
    p = ++(*this);
  return p;
}

RWBoolean
RWIsvSlistIterator::atFirst() const
{
  return isActive() && shere_ == slist_->head_.next_;
}

RWBoolean
RWIsvSlistIterator::atLast() const
{
  return isActive() && shere_ == slist_->last_;
}

void
RWIsvSlistIterator::toFirst()
{
  shere_ = slist_->head_.next_;
}

void
RWIsvSlistIterator::toLast()
{
  shere_ = slist_->isEmpty() ? &slist_->tail_ : slist_->last_;
}


/*
 * Insert the link pointed to by a after the current link.  There must
 * be a current link (that is, the iterator must still be active).
 */
void
RWIsvSlistIterator::insertAfterPoint(RWIsvSlink* a)
{
  RWPRECONDITION2(a, "RWIsvSlistIterator::insertAfterPoint(RWIsvSlink*): nil pointer");
  RWPRECONDITION2(a->next_==rwnil, "RWIsvSlistIterator::insertAfterPoint(RWIsvSlink*): Attempt to add intrusive link to more than one list");
  RWPRECONDITION2(shere_ != &slist_->tail_, "RWIsvSlistIterator::insertAfterPoint(RWIsvSlink*): Attempt to add link while iterator is inactive");

  slist_->insertAfterLink(shere_, a);
}	       

/*
 * Remove the link pointed to by the iterator.  The iterator must
 * be active.  This routine is not terribly efficient because in a 
 * singly linked list we do not have the link to the left.  
 * Hence, we must search through the list for it.
 *
 * The iterator is left pointing to the *previous* item in the list.
 */

RWIsvSlink*
RWIsvSlistIterator::remove()
{
  // Check for inactive iterator:
  if (!isActive()) return rwnil;

  // Backup the iterator to the previous link:
  shere_ = slist_->findLeftIsv(shere_);

  // Now remove the link after the new cursor position:
  return slist_->removeRight(shere_);
}

// Reset the iterator to work with a new collection
void
RWIsvSlistIterator::reset(RWIsvSlist& s)
{
  slist_ = &s;
  reset();
}
