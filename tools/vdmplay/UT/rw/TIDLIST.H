#ifndef __RWTIDLIST_H__
#define __RWTIDLIST_H__

/*
 * RWTIsvDlist<T>: Parameterized intrusive list of Ts (which must derive from RWIsvDlink)
 *
 * $Id: tidlist.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: tidlist.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:11:10  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1994/03/22  23:26:13  vriezen
 * Added operator--(int) to private.
 *
 * Revision 2.10  1994/01/03  23:42:40  jims
 * ObjectStore version: add #include <ostore/ostore.hh>
 *
 * Revision 2.9  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.8  1993/11/06  04:30:01  jims
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
 * Revision 2.4  1993/01/28  01:32:12  keffer
 * Adjusted access of RWIsvDlist::init.
 *
 * Revision 2.3  1993/01/27  21:39:38  keffer
 * Now uses separate definitions file
 *
 * Revision 2.2  1993/01/23  00:32:27  keffer
 * Performance enhancements; simplified; flatter inheritance tree.
 *
 * 
 *    Rev 1.4   29 May 1992 11:50:46   KEFFER
 * 0u ->0
 * 
 *    Rev 1.3   28 May 1992 20:10:20   KEFFER
 * Ported to Metaware High-C.
 * 
 *    Rev 1.2   25 May 1992 15:53:46   KEFFER
 * Optimized by using an internal link
 * 
 *    Rev 1.0   02 Mar 1992 16:10:50   KEFFER
 * Initial revision.
 *
 ***************************************************************************
 *
 * Declares the parameterized class RWTIsvDlist<T>, an intrusive
 * doubly-linked list.  Items of type T are inserted into the list.  
 * They must inherit from RWIsvSlink.
 */

#include "rw/idlist.h"

// forward declaration of the iterator:
template <class TL> class RWTExport RWTIsvDlistIterator;

/****************************************************************
 *								*
 *		Declarations for RWTIsvDlist<T>			*
 *								*
 ****************************************************************/

template <class TL> class RWTExport RWTIsvDlist : private RWIsvDlist
{

public:

  RWTIsvDlist()                        {;}
  RWTIsvDlist(TL* a) : RWIsvDlist(a)   {;}

  /********************* Member functions **************************/

  RWIsvDlist::clear;
  RWIsvDlist::entries;
  RWIsvDlist::isEmpty;

  void		append(TL* a)
	{RWIsvDlist::append(a);}

  void		apply(void (*applyFun)(TL*, void*), void*);

  TL*		at(size_t i) const
	{return (TL*)RWIsvDlist::at(i);}

  void		clearAndDestroy();

  RWBoolean	contains(RWBoolean (*testFun)(const TL*, void*), void* d) const
	{return find(testFun,d)!=rwnil;}

  RWBoolean	containsReference(const TL* a) const
	{return RWIsvDlist::containsReference(a);}

  TL*		find(RWBoolean (*testFun)(const TL*, void*), void*) const;

  TL*		first() const
	{return (TL*)RWIsvDlist::first();}

  TL*		get()
	{return (TL*)RWIsvDlist::get();}

  size_t	index(RWBoolean (*testFun)(const TL*, void*), void*) const;

  void		insert(TL* a)
	{RWIsvDlist::insert(a);}

  void		insertAt(size_t i, TL* a)
	{RWIsvDlist::insertAt(i,a);}

  TL*		last() const
	{return (TL*)RWIsvDlist::last();}

  size_t	occurrencesOf(RWBoolean (*testFun)(const TL*, void*), void*) const;

  size_t	occurrencesOfReference(const TL* a) const
	{return RWIsvDlist::occurrencesOfReference(a);}

  void		prepend(TL* a)
	{RWIsvDlist::prepend(a);}

  TL*		remove(RWBoolean (*testFun)(const TL*, void*), void*);

  TL*		removeAt(size_t i)
	{return (TL*)RWIsvDlist::removeAt(i);}

  TL*		removeFirst()
	{return (TL*)RWIsvDlist::removeFirst();}

  TL*		removeLast()
	{return (TL*)RWIsvDlist::removeLast();}

  TL*		removeReference(TL* a)
	{return (TL*)RWIsvDlist::removeReference(a);}

protected:

  TL*		headLink() const	{return (TL*)&head_;}
  TL*		tailLink() const	{return (TL*)&tail_;}
  TL*		firstLink() const	{return (TL*)head_.next_;}
  TL*		lastLink() const	{return (TL*)tail_.prev_;}

  RWIsvDlist::init;

private:

  friend class RWTExport RWTIsvDlistIterator<TL>;

};

/****************************************************************
 *								*
 *	Declarations for RWTIsvDlistIterator<TL>		*
 *								*
 ****************************************************************/

template <class TL> class RWTExport RWTIsvDlistIterator : 
				private RWIsvDlistIterator {

public:

  // Constructor: starts with iterator positioned at headLink
  RWTIsvDlistIterator(RWTIsvDlist<TL>& s) : RWIsvDlistIterator(s) {;}

  // Operators:
  TL*	operator++()          {return (TL*)RWIsvDlistIterator::operator++(); }
  TL*	operator--()          {return (TL*)RWIsvDlistIterator::operator--(); }
  TL*	operator+=(size_t n)  {return (TL*)RWIsvDlistIterator::operator+=(n);}
  TL*	operator-=(size_t n)  {return (TL*)RWIsvDlistIterator::operator-=(n);}
  TL*	operator()()          {return (TL*)RWIsvDlistIterator::operator()(); }


  // Member functions:
  RWTIsvDlist<TL>* container() const
	{return (RWTIsvDlist<TL>*)dlist_;}

  TL*		findNext(RWBoolean (*testFun)(const TL*, void*), void*);

  void		insertAfterPoint(TL* a)
	{RWIsvDlistIterator::insertAfterPoint(a);}

  TL*		key() const
	{return (TL*)RWIsvDlistIterator::key();}

  TL*		remove()
	{return (TL*)RWIsvDlistIterator::remove();}

  TL*		removeNext(RWBoolean (*testFun)(const TL*, void*), void*);

  void		reset()
	{RWIsvDlistIterator::reset();}

  void		reset(RWTIsvDlist<TL>& s)
	{RWIsvDlistIterator::reset(s);}

protected:

  TL*		cursor() const {return (TL*)dhere_;}
  RWIsvDlistIterator::advance;
  RWIsvDlistIterator::backup;
  RWIsvDlistIterator::isActive;

private:

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);
  RWBoolean		operator--(int);
};


#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tidlist.cc"
#endif

#endif	/* __RWTIDLIST_H__ */
