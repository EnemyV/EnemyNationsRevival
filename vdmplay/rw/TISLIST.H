#ifndef __RWTISLIST_H__
#define __RWTISLIST_H__

/*
 * RWTIsvSlist<T>: Parameterized intrusive list of Ts (which must derive from RWIsvSlink)
 *
 * $Id: tislist.h,v 6.2 1994/07/18 19:03:55 jims Exp $
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
 * $Log: tislist.h,v $
 * Revision 6.2  1994/07/18  19:03:55  jims
 * Add copyright
 *
 * Revision 6.1  1994/04/15  19:11:14  vriezen
 * Move all files to 6.1
 *
 * Revision 2.10  1994/01/03  23:33:28  jims
 * ObjectStore version: Add #include <ostore/ostore.hh>
 *
 * Revision 2.9  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.8  1993/11/08  10:55:24  jims
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
 * Revision 2.4  1993/01/28  01:32:36  keffer
 * Adjusted access of RWIsvSlist::init.
 *
 * Revision 2.3  1993/01/27  21:39:40  keffer
 * Now uses separate definitions file
 *
 * Revision 2.2  1993/01/23  00:32:27  keffer
 * Performance enhancements; simplified; flatter inheritance tree.
 *
 * 
 *    Rev 1.0   02 Mar 1992 16:10:52   KEFFER
 * Initial revision.
 *
 ***************************************************************************
 *
 * Declares the parameterized class RWTIsvSlist<T>, an intrusive
 * singly-linked list of links of type T, which MUST inherit from RWIsvSlink.
 */

#include "rw/islist.h"
// forward declaration of the iterator:
template <class TL> class RWTExport RWTIsvSlistIterator;

/****************************************************************
 *								*
 *		Declarations for RWTIsvSlist<T>			*
 *								*
 ****************************************************************/

template <class TL> class RWTExport RWTIsvSlist : private RWIsvSlist
{

public:

  RWTIsvSlist()				{;}
  RWTIsvSlist(TL* a) : RWIsvSlist(a)	{;}

  /********************* Member functions **************************/

  RWIsvSlist::clear;
  RWIsvSlist::entries;
  RWIsvSlist::isEmpty;

  void		append(TL* a)
	{RWIsvSlist::append(a);}

  void		apply(void (*applyFun)(TL*, void*), void*);

  TL*		at(size_t i) const
	{return (TL*)RWIsvSlist::at(i);}

  void		clearAndDestroy();

  RWBoolean	contains(RWBoolean (*testFun)(const TL*, void*), void* d) const
	{return find(testFun,d)!=rwnil;}

  RWBoolean	containsReference(const TL* a) const
	{return RWIsvSlist::containsReference(a);}

  TL*		find(RWBoolean (*testFun)(const TL*, void*), void*) const;

  TL*		first() const
	{return (TL*)RWIsvSlist::first();}

  TL*		get()
	{return (TL*)RWIsvSlist::get();}

  size_t	index(RWBoolean (*testFun)(const TL*, void*), void*) const;

  void		insert(TL* a)
	{RWIsvSlist::insert(a);}

  void		insertAt(size_t i, TL* a)
	{RWIsvSlist::insertAt(i, a);}

  TL*		last() const
	{return (TL*)RWIsvSlist::last();}

  size_t	occurrencesOf(RWBoolean (*testFun)(const TL*, void*), void*) const;

  size_t	occurrencesOfReference(const TL* a) const
	{return RWIsvSlist::occurrencesOfReference(a);}

  void		prepend(TL* a)
	{RWIsvSlist::prepend(a);}

  TL*		remove(RWBoolean (*testFun)(const TL*, void*), void*);

  TL*		removeAt(size_t i)
	{return (TL*)RWIsvSlist::removeAt(i);}

  TL*		removeFirst()
	{return (TL*)RWIsvSlist::removeFirst();}

  TL*		removeLast()
	{return (TL*)RWIsvSlist::removeLast();}

  TL*		removeReference(TL* a)
	{return (TL*)RWIsvSlist::removeReference(a);}

protected:

  TL*		headLink()  const {return (TL*)&head_;}
  TL*		tailLink()  const {return (TL*)&tail_;}
  TL*		firstLink() const {return (TL*)head_.next_;}
  TL*		lastLink()  const {return (TL*)last_;}

  TL*		removeRight(TL* a)	// Remove and return link after the argument
	{return (TL*)RWIsvSlist::removeRight(a);}

  RWIsvSlist::init;

private:

friend class RWTExport RWTIsvSlistIterator<TL>;

};

/****************************************************************
 *								*
 *		Declarations for RWTIsvSlistIterator<TL>	*
 *								*
 ****************************************************************/

template <class TL> class RWTExport RWTIsvSlistIterator : 
				private RWIsvSlistIterator {

public:

  // Constructor: starts with iterator positioned at headLink
  RWTIsvSlistIterator(RWTIsvSlist<TL>& s) : RWIsvSlistIterator(s) {;}

  // Operators:
  TL*	operator++()          {return (TL*)RWIsvSlistIterator::operator++(); }
  TL*	operator+=(size_t n)  {return (TL*)RWIsvSlistIterator::operator+=(n);}
  TL*	operator()()          {return (TL*)RWIsvSlistIterator::operator()(); }

  // Member functions:
  RWTIsvSlist<TL>* container() const
	{return (RWTIsvSlist<TL>*)slist_;}

  TL*		findNext(RWBoolean (*testFun)(const TL*, void*), void*);

  void		insertAfterPoint(TL* a)
	{RWIsvSlistIterator::insertAfterPoint(a);}

  TL*		key() const
	{return (TL*)RWIsvSlistIterator::key();}

  TL*		remove()	// Relatively slow
	{return (TL*)RWIsvSlistIterator::remove();}

  TL*		removeNext(RWBoolean (*testFun)(const TL*, void*), void*);

  void		reset()
	{RWIsvSlistIterator::reset();}

  void		reset(RWTIsvSlist<TL>& s)
	{RWIsvSlistIterator::reset(s);}

protected:
  RWIsvSlistIterator::advance;
  RWIsvSlistIterator::isActive;

  TL*		cursor() const {return (TL*)shere_;}

private:

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  TL*		operator++(int);

};


#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tislist.cc"
#endif

#endif	/* __RWTISLIST_H__ */
