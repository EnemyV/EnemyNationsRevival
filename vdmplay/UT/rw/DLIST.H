#ifndef  __RWDLIST_H__
#define  __RWDLIST_H__

/*
 * Declarations for Doubly-linked lists.
 *
 * $Id: dlist.h,v 6.5 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: dlist.h,v $
 * Revision 6.5  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.4  1994/06/28  19:23:46  nevis
 * Fixed out of line virtual dtor.
 *
 * Revision 6.3  1994/06/20  16:52:07  nevis
 * Added out of line virtual dtor.
 *
 * Revision 6.2  1994/06/02  16:59:13  foote
 * Port to Symantec 7.0 on Macintosh
 *
 * Revision 6.1  1994/04/15  19:09:51  vriezen
 * Move all files to 6.1
 *
 * Revision 2.8  1993/11/08  08:58:51  jims
 * Port to ObjectStore
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
 * Revision 2.4  1993/02/13  21:49:01  keffer
 * Ported to Zortech; required public derivation.
 *
 * Revision 2.3  1993/02/11  22:47:20  keffer
 * Ported to IBM xlC compiler
 *
 * Revision 2.2  1993/01/25  18:21:06  keffer
 * Simplified, improving performance
 *
 *    Rev 1.3   25 May 1992 15:49:44   KEFFER
 * inline -> Inline.
 * 
 *    Rev 1.2   18 Feb 1992 09:54:18   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:12   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:14:22   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/idlist.h"

/*
 * This is the actual link which includes a pointer
 */
class RWExport RWPDlink : public RWIsvDlink
{

public:

  RWPDlink(void* a) : info_(a) {;}
  RWPDlink*	next() const  {return (RWPDlink*)next_;}

  void*		info_;

};

/****************************************************************
 *								*
 *		Declarations for RWDlist			*
 *								*
 ****************************************************************/

#if defined(__ZTC__) && !defined(THINK_CPLUS) 
// This derivation is formally intended to be private, but a bug
// in Zortech requires it to be public:
class RWExport RWDlist : public RWIsvDlist
#else
class RWExport RWDlist : private RWIsvDlist
#endif
{

public:

  RWDlist()		{;}
  RWDlist(void* a) 
    : RWIsvDlist(new RWPDlink(a)) {;}
  RWDlist(const RWDlist&);
  ~RWDlist()		{clear();} 

  // Operators:
  RWDlist&		operator=(const RWDlist&);

  // Member functions:
  void*			append(void* a);
  void			apply(RWapplyGeneric, void*);
  void			applyReference(void (*apRef)(void*&, void*), void*);
  void*&		at(size_t n)	{return ((RWPDlink*)RWIsvDlist::at(n))->info_;}
#ifndef RW_NO_CONST_OVERLOAD
  const void*		at(size_t n) const	{return ((RWPDlink*)RWIsvDlist::at(n))->info_;}
#endif
  void			clear();
  RWBoolean		contains(RWtestGeneric tfun, const void* c) const
	{return (find(tfun,c) != rwnil);}
  RWBoolean		containsReference(const void* c) const
	{return (findReference(c) != rwnil);}
#ifdef RW_NO_ACCESS_ADJUSTMENT
  size_t		entries() const { return RWIsvDlist::entries(); }
#else
  RWIsvDlist::entries;
#endif
  void* 		find(RWtestGeneric, const void*) const;
  void* 		findReference(const void*) const; // First occurrence
  void*	 		first() const;
  void*	 		get()	{return peel((RWPDlink*)RWIsvDlist::get());}
  size_t		index(RWtestGeneric, const void*) const;
  void*			insert(void* a) { return append(a); }
  void*			insertAfter(int, void*); // For backwards compatiblity
  void*			insertAt(size_t, void*);
#ifdef RW_NO_ACCESS_ADJUSTMENT
  RWBoolean		isEmpty() const { return RWIsvDlist::isEmpty(); }
#else
  RWIsvDlist::isEmpty;
#endif
  void*	 		last() const;
  size_t		occurrencesOf(RWtestGeneric, const void*) const;
  size_t		occurrencesOfReference(const void*) const;
  void*			prepend(void* a);
  void*			remove(RWtestGeneric, const void*);
  void* 		removeReference(const void*);

protected:

  RWPDlink*		headLink()  const;
  RWPDlink*		tailLink()  const;
  RWPDlink*		firstLink() const {return (RWPDlink*)head_.next_;}
  RWPDlink*		lastLink()  const {return (RWPDlink*)tail_.prev_;}

  // Type safe interface to base class:
  void			insertAfterLink(RWPDlink* p, RWPDlink* v)
	{RWIsvDlist::insertAfterLink(p,v);}

  static void*		peel(RWPDlink*);

private:

friend class RWExport RWDlistIterator;

};


/*
 * Declaration for iterator for doubly-linked list.
 */
class RWExport RWDlistIterator : private RWIsvDlistIterator
{

public:

  // Constructor: cast needed for Glock V2.0b:
  RWDlistIterator(RWDlist& s) : RWIsvDlistIterator((RWIsvDlist&)s) {;}
  virtual ~RWDlistIterator();

  void*			operator++();
  void*			operator--();
  void*			operator+=(size_t n);
  void*			operator-=(size_t n);
  void*	 		operator()();

#ifdef RW_NO_ACCESS_ADJUSTMENT
  RWBoolean		atFirst() const	{return RWIsvDlistIterator::atFirst();}
  RWBoolean		atLast() const	{return RWIsvDlistIterator::atLast(); }
  void			toFirst()	{RWIsvDlistIterator::toFirst();       }
  void			toLast()	{RWIsvDlistIterator::toLast();        }
#else
  RWIsvDlistIterator::atFirst;
  RWIsvDlistIterator::atLast;
  RWIsvDlistIterator::toFirst;
  RWIsvDlistIterator::toLast;
#endif

  // Methods:
  RWDlist*		container() const	{return (RWDlist*)dlist_;}
  void*	 		findNext(RWtestGeneric, const void*);
  void*	 		findNextReference(const void*);
  void*			insertAfterPoint(void*);
  void*	 		key() const
	{return isActive() ? cursor()->info_ : rwnil;}
  void*			remove()
	{return RWDlist::peel((RWPDlink*)RWIsvDlistIterator::remove());}
  void*			removeNext(RWtestGeneric, const void*);
  void*			removeNextReference(const void*);
  void			reset()			{RWIsvDlistIterator::reset();}
  void			reset(RWDlist& sl)	{RWIsvDlistIterator::reset((RWIsvDlist&)sl);}

protected:

  RWPDlink*		cursor() const		{return (RWPDlink*)dhere_;}

#ifndef RW_NO_POSTFIX
  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place. (or worse)
  void			operator++(int);
  void			operator--(int);
#endif  

};


//////////////////////////////////////////////////////////
//							//
//			INLINES				//
//							//
//////////////////////////////////////////////////////////

inline void*
RWDlist::first() const	{
  return isEmpty() ? rwnil : firstLink()->info_;
}

inline void*
RWDlist::last() const	{
  return isEmpty() ? rwnil : lastLink()->info_;
}

inline RWPDlink*
RWDlist::headLink() const {
  // Cast away constness to suppress unfounded cfront warning:
  return (RWPDlink*)&((RWDlist*)this)->head_;
}

inline RWPDlink*
RWDlist::tailLink() const {
  // Cast away constness to suppress unfounded cfront warning:
  return (RWPDlink*)&((RWDlist*)this)->tail_;
}

inline void*
RWDlistIterator::operator++()
{
  // Make sure tail link points to itself:
  RWPRECONDITION(container()->tailLink()->next()==container()->tailLink());

  advance();
  return cursor() == container()->tailLink() ? rwnil : cursor()->info_;
}

inline void*
RWDlistIterator::operator--()
{
  // Make sure head link points to itself:
  RWPRECONDITION(container()->headLink()->prev()==container()->headLink());

  backup();
  return cursor() == container()->headLink() ? rwnil : cursor()->info_;
}

inline void*
RWDlistIterator::operator()()
{
  return ++(*this);
}

#endif  /* __RWDLIST_H__ */
