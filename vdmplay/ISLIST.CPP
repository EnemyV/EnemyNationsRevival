/*
 * RWIsvSlist: Definitions for intrusive singly-linked lists.
 *
 * $Id: islist.cpp,v 2.12 1993/09/10 03:59:57 keffer Exp $
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
 * $Log: /vdmplay/ISLIST.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 2.12  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 2.11  1993/05/31  21:46:32  keffer
 * New messaging architecture for localization
 *
 * Revision 2.10  1993/05/18  00:48:57  keffer
 * Introduced new exception handling classes
 *
 * Revision 2.9  1993/05/14  20:50:24  dealys
 * removed iterator code
 *
 * Revision 2.8  1993/04/09  20:57:09  keffer
 * Indexing now done using size_t
 *
 * Revision 2.7  1993/03/24  23:49:27  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.6  1993/02/17  22:18:00  keffer
 * RWIsvSlist::removeReference() now checks for nil pointer.
 *
 * Revision 2.5  1993/02/05  23:06:09  keffer
 * Corrected problem in removeReference().
 *
 * Revision 2.4  1993/01/29  20:09:52  keffer
 * Added precondition in removeReference().
 *
 * Revision 2.3  1993/01/23  00:34:51  keffer
 * Performance enhancements; simplified; flatter inheritance tree.
 *
 *
 * Revision 2.1  1992/11/19  05:06:42  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:36:08  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   29 May 1992 09:46:50   KEFFER
 * Introduced CPP_ANSI_RECURSION macro
 * 
 *    Rev 1.3   28 May 1992 16:24:14   KEFFER
 * Introduced RWPRECONDITION2 macro
 * 
 *    Rev 1.2   27 May 1992 18:09:06   KEFFER
 * RWMEMCK compatible.
 * 
 *    Rev 1.1   25 May 1992 15:38:14   KEFFER
 * 
 *    Rev 1.0   15 Apr 1992 19:38:32   KEFFER
 * Initial revision.
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
#include "rw/rwerr.h"
#include "rw/toolerr.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: islist.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#ifndef RW_NO_CPP_RECURSION
# define new rwnew
#endif

/********************************************************
*							*
*		RWIsvSlist definitions			*
*							*
********************************************************/


/*
 * Construct a list with one link, given by "a".  In the "debug" version,
 * the "next" pointer in "a" must be nil, signalling that it is not being
 * used in any other list.
 */
RWIsvSlist::RWIsvSlist(RWIsvSlink* a)
{
  RWPRECONDITION2(a, "RWIsvSlist::RWIsvSlist(RWIsvSlink*): Attempt to construct with nil pointer");
  RWPRECONDITION2(a->next_==rwnil, "RWIsvSlist::RWIsvSlist(RWIsvSlink*): Attempt to add intrusive link to more than one list");

  head_.next_ = last_       = a;
  a->next_    = tail_.next_ = &tail_;
  nitems_     = 1;
}

/*
 * Find link "i".  The index "i" must be in range.
 */
RWIsvSlink*
RWIsvSlist::at(size_t i) const
{
  if(i>=entries())
    RWTHROW( RWBoundsErr(RWMessage(RWTOOL_INDEX, (unsigned)i, (unsigned)entries()-1) ));

  register RWIsvSlink* link = head_.next_;
  while (i--) link = link->next_;
  return link;
}

/*
 * Clear the list.  The debug implementation walks the list, setting each
 * "next" pointer to nil.  This allows other routines to detect whether
 * a link is being inserted into more than one collection.
 */
void   
RWIsvSlist::clear()
{
#ifdef RWDEBUG
  register RWIsvSlink* link = first();
  while (link)
  {
    link = link->clear();
  }
#endif
  init();
}

RWBoolean
RWIsvSlist::containsReference(const RWIsvSlink* a) const
{
  RWPRECONDITION2(a, "RWIsvSlist::containsReference(const RWISvSlink*): nil pointer");
  RWPRECONDITION(head_.next_ != rwnil);

  if (a)
  {
    const RWIsvSlink* cursor = head_.next_;
    // Cast away constness to suppress unwarranted cfront warning:
    while (cursor != &((RWIsvSlist*)this)->tail_)
    {
      if (cursor == a) return TRUE;
      cursor = cursor->next_;
    }
  }
  return FALSE;
}


/*
 * Put link "a" at position "i".  The link formerly at
 * position "i" becomes link "i+1".
 */
void
RWIsvSlist::insertAt(size_t i, RWIsvSlink* a)
{
  RWPRECONDITION2(a, "RWIsvSlist::insertAt(size_t,RWIsvSlink*): nil pointer");
  RWPRECONDITION2(a->next_==rwnil, "RWIsvSlist::insertAt(size_t,RWIsvSlink*): Attempt to add intrusive link to more than one list");

  if(i>entries())
    RWTHROW( RWBoundsErr(RWMessage(RWTOOL_INDEX, (unsigned)i, (unsigned)entries()-1) ));

  RWIsvSlink* prev = i ? at(i-1) : &head_;

  RWASSERT(prev);

  insertAfterLink(prev, a);
}


/*
 * Remove and return the link at position "i".
 */
RWIsvSlink*
RWIsvSlist::removeAt(size_t i)
{
  if(i>=entries())
    RWTHROW( RWBoundsErr(RWMessage(RWTOOL_INDEX, (unsigned)i, (unsigned)entries()-1) ));

  RWIsvSlink* prev = i ? at(i-1) : &head_;

  RWASSERT(prev);
  return removeRight(prev);
}


/*
 * Remove and return the link with address "a".  Returns nil if the
 * link is not on the list.  Because this is a non-const function,
 * you can optimize slightly by setting a sentinel, then searching
 * for it.  This eliminates a conditional in the "while()" loop.
 */

RWIsvSlink*
RWIsvSlist::removeReference(RWIsvSlink* a)
{
  if (!a) return rwnil;

  RWPRECONDITION2(a != &head_ && a != &tail_, "RWIsvSlist::removeReference(RWIsvSlink*): Internal error");

  tail_.next_ = a;		// Set sentinel

  // Search for the link to the left of "a"
  RWIsvSlink* cursor = &head_;

  while (cursor->next_ != a)	// Search for link or sentinel
    cursor = cursor->next_;

  if (cursor == &tail_)
  {
    a = rwnil;			// Not found
  }
  else
  {
    cursor->next_ = cursor->next_->next_;
    a->next_ = rwnil;
    if (a==last_)
      last_ = cursor;		// Correct last_ if necessary
    --nitems_;
  }

  tail_.next_ = &tail_;		// Reset sentinel
  return a;
}

/********************************************************
*							*
*		RWIsvSlist protected functions		*
*							*
********************************************************/

/*
 * Protected function to find the link to the left of the argument.  
 * This necessitates searching the list.  Returns nil if the argument
 * is nil or not in the list.
 *
 * Because this is a const function, you cannot set tail_ to
 * be a sentinel.  Hence, the "while()" loop includes two
 * conditionals.
 */

RWIsvSlink*
RWIsvSlist::findLeftIsv(const RWIsvSlink* a) const
{
  RWPRECONDITION(head_.next_ != rwnil);

  if (a)
  {
    // Search for the link to the left of "a".

    const RWIsvSlink* cursor = &head_;

    // Cast away constness to suppress unwarranted cfront warning:
    while (cursor->next_ != &((RWIsvSlist*)this)->tail_)
    {
      if (cursor->next_ == a)
	return (RWIsvSlink*)cursor; // Found it.
      cursor = cursor->next_;
    }
  }
  return rwnil;
}

void
RWIsvSlist::init()
{
  head_.next_ = tail_.next_ = &tail_;
  last_       = &head_;
  nitems_     = 0;
}


/*
 * Protected function to insert an object. 
 * Link "a" is inserted to the right of link "sl".
 * Neither "a" nor "sl" can be nil.
 */
void
RWIsvSlist::insertAfterLink(RWIsvSlink* sl, RWIsvSlink* a)
{
  RWPRECONDITION2(sl!=rwnil, "RWIsvSlist::insertAfterLink(RWIsvSlink*, RWIsvSlink*): Internal error: attempt to add link after nil link");
  RWPRECONDITION2(a!=rwnil, "RWIsvSlist::insertAfterLink(RWIsvSlink*, RWIsvSlink*): Attempt to add nil link");
  RWPRECONDITION2(a->next_==rwnil, "RWIsvSlist::insertAfterLink(RWIsvSlink*, RWIsvSlink*): Attempt to add intrusive link to more than one list");

  a->next_  = sl->next_;
  sl->next_ = a;
  if (sl==last_) last_ = a;	// Update end of list if appending.
  ++nitems_;

  RWASSERT(last_->next_ == &tail_);
}


/*
 * Remove and return the link after the argument.
 * The argument MUST occur in the list and cannot be the last link in
 * the list.
 */
RWIsvSlink*
RWIsvSlist::removeRight(RWIsvSlink* prev)
{
  RWPRECONDITION2(prev, "RWIsvSlist::removeRight(RWIsvSlink*): Internal error");
  RWPRECONDITION2(prev == &head_ || containsReference(prev),"RWIsvSlist::removeRight(RWIsvSlink*): attempt to remove link not in list");
  RWPRECONDITION2(prev!=last_, "RWIsvSlist::removeRight(RWIsvSlink*): Attempt to remove link after last");

  if (!prev) return rwnil;
  RWIsvSlink* target = prev->next_;
  prev->next_ = target->next_;		// Point around the victim.
  if (target==last_) last_ = prev;	// Correct last_ if necessary.
  --nitems_;
#ifdef RWDEBUG
  target->next_ = rwnil;
#endif
  return target;
}

