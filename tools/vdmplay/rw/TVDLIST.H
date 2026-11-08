#ifndef __RWTVDLIST_H__
#define __RWTVDLIST_H__

/*
 * RWTValDlist<T>: Doubly-linked list of values of type T.
 *
 * $Id: tvdlist.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * Stores a *copy* of the inserted item into the collection.
 *
 * Assumes that T has:
 *   - well-defined copy constructor (T::T(const T&) or equiv.);
 *   - well-defined assignment operator (T::operator=(const T&) or equiv.);
 *   - well-defined equality semantics (T::operator==(const T&)).
 *
 ***************************************************************************
 *
 * $Log: tvdlist.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:11  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.10  1993/11/08  13:49:36  jims
 * Port to ObjectStore
 *
 * Revision 2.9  1993/09/10  02:18:28  keffer
 * apply() now takes a function with a non-const reference, allowing
 * modification of the contained value.
 *
 * Revision 2.8  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.7  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.6  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.5  1993/02/12  23:33:45  keffer
 * Ported to g++ v2.3
 *
 * Revision 2.4  1993/02/12  00:18:50  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.3  1993/01/27  21:39:45  keffer
 * Now uses separate definitions file
 *
 * Revision 2.2  1993/01/23  00:32:27  keffer
 * Performance enhancements; simplified; flatter inheritance tree.
 *
 * 
 */

#include "rw/tidlist.h"

template <class T> class RWTExport RWTValDlistIterator;

/****************************************************************
 *								*
 *		 Declarations for RWTValDlink<T>		*
 *								*
 ****************************************************************/

/*
 * This is the actual link that is stored in the linked list.
 * It includes data of type "T".
 */
template <class T> class RWTExport RWTValDlink : public RWIsvDlink
{

public:

  RWTValDlink(const T& a) : info_(a) {;}
  RWTValDlink<T>* next() const {return (RWTValDlink<T>*)next_;}
  RWTValDlink<T>* prev() const {return (RWTValDlink<T>*)prev_;}

  T		info_;
};

/****************************************************************
 *								*
 *		Declarations for RWTValDlist<T>			*
 *								*
 ****************************************************************/

template <class T> class RWTExport RWTValDlist 
                             : private RWTIsvDlist< RWTValDlink<T> >
{

public:

  RWTValDlist() {;}
  RWTValDlist(const RWTValDlist<T>&);
  ~RWTValDlist() {clear();}

  // Operators:
  RWTValDlist<T>&	operator=(const RWTValDlist<T>&);

  T&		operator[](size_t i)
	{return RWTIsvDlist<RWTValDlink<T> >::at(i)->info_;}
  T		operator[](size_t i) const
	{return RWTIsvDlist<RWTValDlink<T> >::at(i)->info_;}
  
  // Member functions:
  void		append(const T& a);

  void		apply(void (*applyFun)(T&, void*), void*);

  T&		at(size_t i)
	{return RWTIsvDlist<RWTValDlink<T> >::at(i)->info_;}
  T		at(size_t i) const
	{return RWTIsvDlist<RWTValDlink<T> >::at(i)->info_;}

  void		clear()
	{RWTIsvDlist<RWTValDlink<T> >::clearAndDestroy();}

  RWBoolean	contains(const T& a) const;
  RWBoolean	contains(RWBoolean (*testFun)(const T&, void*), void*) const;

  size_t	entries() const
	{return RWTIsvDlist<RWTValDlink<T> >::entries();}

  RWBoolean	find(const T& p, T& ret) const; // Find first; return in "ret"
  RWBoolean	find(RWBoolean (*testFun)(const T&, void*), void* d, T& ret) const;

  T		first() const
	{return firstLink()->info_;}

  T		get()
	{return peel(RWTIsvDlist<RWTValDlink<T> >::get());}

  size_t	index(const T& a) const;
  size_t	index(RWBoolean(*testFun)(const T&, void*), void*) const;

  void		insert(const T& a);
  void		insertAt(size_t i, const T& a);

  RWBoolean	isEmpty() const
	{return RWTIsvDlist<RWTValDlink<T> >::isEmpty();}

  T		last() const
	{return lastLink()->info_;}

  size_t	occurrencesOf(const T& a) const;
  size_t	occurrencesOf(RWBoolean (*testFun)(const T&, void*), void*) const;

  void		prepend(const T& a);

  RWBoolean	remove(const T& a);
  RWBoolean	remove(RWBoolean (*testFun)(const T&, void*), void* d);

  size_t	removeAll(const T& a);
  size_t	removeAll(RWBoolean (*testFun)(const T&, void*), void*);

  T		removeAt(size_t i)
	{return peel(RWTIsvDlist<RWTValDlink<T> >::removeAt(i));}

  T		removeFirst()
	{return peel(RWTIsvDlist<RWTValDlink<T> >::removeFirst());}

  T		removeLast()
	{return peel(RWTIsvDlist<RWTValDlink<T> >::removeLast());}

protected:

  RWTValDlink<T>*	findVal(const T&) const;
  RWTValDlink<T>*	findFun(RWBoolean (*testFun)(const T&, void*), void*) const;

  static T		peel(RWTValDlink<T>* link);

private:

friend class RWTExport RWTValDlistIterator<T>;  

};

template <class T> class RWTExport RWTValDlistIterator :
                   private RWTIsvDlistIterator<RWTValDlink<T> >
{

public:

  RWTValDlistIterator(RWTValDlist<T>& s) :
	RWTIsvDlistIterator<RWTValDlink<T> > (s) {;}

  // Operators:
  RWBoolean	operator++()
	{advance(); return cursor()!=container()->tailLink();}
  RWBoolean	operator--()
	{backup(); return cursor()!=container()->headLink();}
  RWBoolean	operator+=(size_t n)
	{return RWTIsvDlistIterator<RWTValDlink<T> >::operator+=(n)!=rwnil;}
  RWBoolean	operator-=(size_t n)
	{return RWTIsvDlistIterator<RWTValDlink<T> >::operator-=(n)!=rwnil;}
  RWBoolean	operator()()
	{advance(); return cursor()!=container()->tailLink();}

  // Methods:
  RWTValDlist<T>*	container() const
	{return (RWTValDlist<T>*)RWTIsvDlistIterator<RWTValDlink<T> >::container();}

  RWBoolean		findNext(const T& p);
  RWBoolean		findNext(RWBoolean (*testFun)(const T&, void*), void* d);

  void			insertAfterPoint(const T& a);

  T			key() const
	{return cursor()->info_;}

  RWBoolean		remove();	// Remove item at cursor

  RWBoolean		removeNext(const T& p);
  RWBoolean		removeNext(RWBoolean (*testFun)(const T&, void*), void* d);

  void			reset()
	{RWTIsvDlistIterator<RWTValDlink<T> >::reset();}

  void			reset(RWTValDlist<T>& s)
	{RWTIsvDlistIterator<RWTValDlink<T> >::reset(s);}

private:

  // Disallow postfix increment & decrement.  Unless we hide it, some 
  // compilers will substitute the prefix operator in its place.
  RWBoolean		operator++(int);
  RWBoolean		operator--(int);
};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvdlist.cc"
#endif

#endif	/* __RWTVDLIST_H__ */
