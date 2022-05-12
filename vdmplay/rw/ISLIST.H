#ifndef  __RWISLIST_H__
#define  __RWISLIST_H__

/*
 * RWIsvSlist: An intrusive singly-linked list.
 *
 * $Id: islist.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 * Voice: (503) 754-3010	FAX: (503) 757-6650
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
 * See Stroustrup II, Section 8.3.1 for a guide to intrusive lists.
 *
 ***************************************************************************
 *
 * $Log: islist.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:23  vriezen
 * Move all files to 6.1
 *
 * Revision 2.8  1993/10/04  18:16:19  keffer
 * RWIsv[SD]link now does not inherit from RWMemoryPool in
 * multi-threaded case.
 *
 * Revision 2.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.6  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.5  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.4  1993/02/11  22:47:20  keffer
 * Ported to IBM xlC compiler
 *
 * Revision 2.3  1993/01/23  00:34:10  keffer
 * Performance enhancements; simplified; flatter inheritance tree.
 *
 *    Rev 1.4   07 Jun 1992 16:06:58   KEFFER
 * Introduced HAS_POSTFIX macro
 * 
 *    Rev 1.3   28 May 1992 17:38:12   KEFFER
 * Moved reset() to eliminate forward reference of an inline.
 * 
 *    Rev 1.2   28 May 1992 11:03:58   KEFFER
 * Added RWExport tag where appropriate
 * 
 *    Rev 1.1   25 May 1992 15:53:46   KEFFER
 * Optimized by using an internal link
 * 
 *    Rev 1.0   11 Mar 1992 14:10:44   KEFFER
 * Initial revision.
 */



#ifndef __RWTOOLDEFS_H__
# include "rw/tooldefs.h"
#endif
#ifndef __RWMEMPOOL_H__
# include "rw/mempool.h"
#endif


/****************************************************************
 *								*
 *		Declaration for RWIsvSlink			*
 *								*
 ****************************************************************/

class RWExport RWIsvSlink RWMemoryPool_OPTION
{

public:

#ifdef RWDEBUG
  RWIsvSlink() : next_(rwnil) {;}
#else
  RWIsvSlink() {;}
#endif

  RWIsvSlink*		next() const	{return next_;}
  RWIsvSlink*		clear()		{RWIsvSlink* ret = next_; next_=rwnil; return ret;}

  RWIsvSlink*		next_;		// Pointer to next link.

};


/****************************************************************
 *								*
 *		Declarations for RWIsvSlist			*
 *								*
 ****************************************************************/

class RWExport RWIsvSlist
{

public:

  RWIsvSlist()		{init();}
  RWIsvSlist(RWIsvSlink* a);
  ~RWIsvSlist()		{clear();}

  /********************* Member functions **************************/
  void			append(RWIsvSlink* a)	{insertAfterLink(last_,a);}
  RWIsvSlink*		at(size_t i) const;	// Index must be in range
  void			clear();
  RWBoolean		containsReference(const RWIsvSlink*) const;
  size_t		entries() const		{return nitems_;}
  RWIsvSlink*		first() const		{return isEmpty() ? rwnil: head_.next_;}
  RWIsvSlink*		get()			{return removeFirst();}
  void			insert(RWIsvSlink* a)	{append(a);}
  void			insertAt(size_t, RWIsvSlink*);
  RWBoolean		isEmpty() const		{return nitems_ == 0;}
  RWIsvSlink*		last() const		{return isEmpty() ? rwnil : last_;}
  size_t		occurrencesOfReference(const RWIsvSlink* a) const
    { return containsReference(a) ? 1u : 0;}
  void			prepend(RWIsvSlink* a)	{insertAfterLink(&head_, a);}
  RWIsvSlink*		removeAt(size_t);		// Relatively slow
  RWIsvSlink*	 	removeFirst()		{return isEmpty() ? rwnil : removeRight(&head_);}
  RWIsvSlink*		removeLast()		{return isEmpty() ? rwnil : removeReference(last_);}
  RWIsvSlink*		removeReference(RWIsvSlink* a);	// Return and remove link w. address "a"

protected:

  RWIsvSlink*		findLeftIsv(const RWIsvSlink*) const;
  void			init();
  void			insertAfterLink(RWIsvSlink*, RWIsvSlink*);
  RWIsvSlink*		removeRight(RWIsvSlink*);  // Remove and return link after the argument

  RWIsvSlink		head_;		// Sentinel
  RWIsvSlink		tail_;		// Sentinel
  RWIsvSlink*		last_;		// Points to last link
  size_t		nitems_;	// Number of entries in the list

private:

friend class RWExport RWIsvSlistIterator;
friend class RWExport RWSlistIterator;

};


/****************************************************************
 *								*
 *		Declarations for RWIsvSlistIterator		*
 *								*
 ****************************************************************/

// Iterator for singly linked list:

class RWExport RWIsvSlistIterator
{

public:

  RWIsvSlistIterator(RWIsvSlist& s) : slist_(&s), shere_(&s.head_) {;}

  // Operators:
  RWIsvSlink*		operator++();		// Advance and return.
  RWIsvSlink*		operator+=(size_t n);	// Advance n links and return.
  RWIsvSlink*		operator()();		// Advance and return.  For lateral compatibility

  RWBoolean		atFirst() const;
  RWBoolean		atLast() const;
  void			toFirst();
  void			toLast();

  RWIsvSlist*		container() const	{return slist_;}
  void			insertAfterPoint(RWIsvSlink*);// Insert item after position.
  RWIsvSlink*		key() const;		// Return current item
  RWIsvSlink*	 	remove();		// Remove current item (slow)
  void			reset();
  void			reset(RWIsvSlist&);	// Reset container to be interated over.

protected:

  void			advance()	{shere_ = shere_->next_;}
  RWBoolean		isActive() const;

  RWIsvSlist*		slist_;			// The list over which we are iterating
  RWIsvSlink*		shere_;			// Iterator position

private:

#ifndef RW_NO_POSTFIX
  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);
#endif

};


//////////////////////////////////////////////////////////
//							//
//			INLINES				//
//							//
//////////////////////////////////////////////////////////

/*
 * Prefix operator: advance iterator one link then return new link.
 * Returns nil if we were at the last link or if the list is empty.
 */
inline RWIsvSlink*
RWIsvSlistIterator::operator++()
{
  RWPRECONDITION(slist_->tail_.next_ == &slist_->tail_);
  advance();
  return shere_ == &slist_->tail_ ? rwnil : shere_;
}

inline RWIsvSlink*
RWIsvSlistIterator::operator()()
{
  return ++(*this);
}

inline RWBoolean
RWIsvSlistIterator::isActive() const
{
  return shere_ != &slist_->head_ && shere_ !=&slist_->tail_;
}

/*
 * Return the current item.  This will be nil if the iterator
 * is out of range: Either past the end, or before the beginning
 * of the list.
 */
inline RWIsvSlink*
RWIsvSlistIterator::key() const
{
  return isActive() ? shere_ : rwnil;
}

/*
 * Reset iterator: position it to the phantom link at the head of the list
 */
inline void
RWIsvSlistIterator::reset()
{
  shere_ = &slist_->head_;
}


#endif  /* __RWISLIST_H__ */
