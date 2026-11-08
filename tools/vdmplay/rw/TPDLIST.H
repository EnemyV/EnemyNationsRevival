#ifndef __RWTPDLIST_H__
#define __RWTPDLIST_H__

/*
 * RWTPtrDlist<TP>: A doubly-linked list of pointers to objects of type TP.
 *
 * $Id: tpdlist.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * Stores a *pointer* to the inserted item into the collection.
 *
 * Assumes that TP has:
 *   - well-defined equality semantics (TP::operator==(const TP&)).
 *
 ***************************************************************************
 *
 * $Log: tpdlist.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:47:40  vriezen
 * Move all files to 6.1
 *
 * Revision 2.13  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 2.12  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.11  1993/12/10  19:40:11  jims
 * ObjectStore version: remove private postfix operators
 *
 * Revision 2.10  1993/11/06  02:29:18  jims
 * Fix ppp directive
 *
 * Revision 2.9  1993/11/06  02:08:54  jims
 * Port to ObjectStore
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.6  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.5  1993/02/12  23:33:45  keffer
 * Ported to g++ v2.3
 *
 * Revision 2.4  1993/02/12  00:18:50  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.3  1993/01/28  01:33:50  keffer
 * Added clearAndDestroy(); corrected syntax error with removeAt(int).
 *
 * Revision 2.2  1993/01/27  21:39:41  keffer
 * Now uses separate definitions file
 *
 *    Rev 1.3   09 Sep 1992 19:36:52   KEFFER
 * Pointer iterators now return TP* where appropriate
 * 
 *    Rev 1.1   04 Sep 1992 12:54:10   KEFFER
 * remove taking a tester function now returns a TP*, instead of RWBoolean.
 * 
 *    Rev 1.0   25 May 1992 15:59:04   KEFFER
 * Initial revision.
 * 
 */

#include "rw/tidlist.h"

template <class TP> class RWTExport RWTPtrDlistIterator;

/****************************************************************
 *								*
 *		Declarations for RWTPtrDlink<TP>		*
 *								*
 ****************************************************************/

/*
 * This is the actual link that is stored in the linked list.
 * It includes data of type "TP".
 */
template <class TP> class RWTExport RWTPtrDlink : public RWIsvDlink
{

public:

  RWTPtrDlink(TP* a) : info_(a) {;}
  RWTPtrDlink<TP>*	next() const
    { return (RWTPtrDlink<TP>*)RWIsvDlink::next();}
  RWTPtrDlink<TP>*	prev() const
    { return (RWTPtrDlink<TP>*)RWIsvDlink::prev();}

  TP*		info_;
};

/****************************************************************
 *								*
 *	Declarations for RWTPtrDlist<TP>			*
 *								*
 ****************************************************************/

template <class TP> class RWTExport RWTPtrDlist 
                                    : private RWTIsvDlist< RWTPtrDlink<TP> > 
{

public:

  RWTPtrDlist() {;}
  RWTPtrDlist(const RWTPtrDlist<TP>&);
  ~RWTPtrDlist() { clear(); }

  // Operators:
  RWTPtrDlist<TP>&	operator=(const RWTPtrDlist<TP>&);

  TP*&		operator[](size_t i)
	{return RWTIsvDlist<RWTPtrDlink<TP> >::at(i)->info_;}
  TP*		operator[](size_t i) const
	{return RWTIsvDlist<RWTPtrDlink<TP> >::at(i)->info_;}
  
  // Members
  void		append(TP* a);

  void		apply(void (*applyFun)(TP*, void*), void*);

  TP*&		at(size_t i)
	{return RWTIsvDlist<RWTPtrDlink<TP> >::at(i)->info_;}
  TP*		at(size_t i) const
	{return RWTIsvDlist<RWTPtrDlink<TP> >::at(i)->info_;}

  void		clear()
	{RWTIsvDlist<RWTPtrDlink<TP> >::clearAndDestroy();}

  void		clearAndDestroy();

  RWBoolean	contains(const TP* a) const;
  RWBoolean	contains(RWBoolean (*testFun)(TP*, void*), void*) const;

  size_t	entries() const
	{return RWTIsvDlist<RWTPtrDlink<TP> >::entries();}

  TP*		find(const TP* p) const;
  TP*		find(RWBoolean (*testFun)(TP*, void*), void* d) const;

  TP*		first() const
	{return isEmpty() ? rwnil : firstLink()->info_;}

  TP*		get()
	{return peel(RWTIsvDlist<RWTPtrDlink<TP> >::get());}

  size_t	index(const TP* a) const;
  size_t	index(RWBoolean(*testFun)(TP*, void*), void*) const;

  void		insert(TP* a);

  void		insertAt(size_t i, TP* a);

  RWBoolean	isEmpty() const
	{return RWTIsvDlist<RWTPtrDlink<TP> >::isEmpty();}

  TP*		last() const
	{return isEmpty() ? rwnil : lastLink()->info_;}

  size_t	occurrencesOf(const TP* a) const;
  size_t	occurrencesOf(RWBoolean (*testFun)(TP*, void*), void*) const;

  void		prepend(TP* a);

  TP*		remove(const TP* p);
  TP*		remove(RWBoolean (*testFun)(TP*, void*), void* d);

  TP*		removeAt(size_t i)
	{return peel(RWTIsvDlist<RWTPtrDlink<TP> >::removeAt(i));}

  TP*		removeFirst()
	{return peel(RWTIsvDlist<RWTPtrDlink<TP> >::removeFirst());}

  TP*		removeLast()
	{return peel(RWTIsvDlist<RWTPtrDlink<TP> >::removeLast()); }

  size_t	removeAll(const TP* a);
  size_t	removeAll(RWBoolean (*testFun)(TP*, void*), void*);

protected:
    
  RWTPtrDlink<TP>*	findVal(const TP*) const;
  RWTPtrDlink<TP>*	findFun(RWBoolean (*testFun)(TP*, void*), void*) const;
  static TP*		peel(RWTPtrDlink<TP>* link);

private:

friend class RWTExport RWTPtrDlistIterator<TP>;  

};

/****************************************************************
 *								*
 *	Declarations for RWTPtrDlistIterator<TP>		*
 *								*
 ****************************************************************/

template <class TP> class RWTExport RWTPtrDlistIterator : 
                    public RWTIsvDlistIterator<RWTPtrDlink<TP> >
{

public:

  RWTPtrDlistIterator(RWTPtrDlist<TP>& s) : 
		RWTIsvDlistIterator<RWTPtrDlink<TP> >(s) { }

  // Operators:
  RWBoolean	operator++()
	{advance(); return cursor()!=container()->tailLink();}
  RWBoolean	operator--()
	{backup(); return cursor()!=container()->headLink();}
  RWBoolean	operator+=(size_t n)
	{return RWTIsvDlistIterator<RWTPtrDlink<TP> >::operator+=(n)!=rwnil;}
  RWBoolean	operator-=(size_t n)
	{return RWTIsvDlistIterator<RWTPtrDlink<TP> >::operator-=(n)!=rwnil;}
  TP*		operator()()
	{advance(); return cursor()==container()->tailLink() ? rwnil : cursor()->info_;}

  // Methods:
  RWTPtrDlist<TP>* container() const
	{return (RWTPtrDlist<TP>*)RWTIsvDlistIterator<RWTPtrDlink<TP> >::container();}

  TP*		findNext(const TP* p);
  TP*		findNext(RWBoolean (*testFun)(TP*, void*), void* d);

  void		insertAfterPoint(TP* a);

  TP*		key() const
	{return isActive() ? cursor()->info_ : rwnil;}

  TP*		remove();

  TP*		removeNext(const TP* p);
  TP*		removeNext(RWBoolean (*testFun)(TP*, void*), void* d);

  void		reset()
	{RWTIsvDlistIterator<RWTPtrDlink<TP> >::reset();}
  void		reset(RWTPtrDlist<TP>& s)
	{RWTIsvDlistIterator<RWTPtrDlink<TP> >::reset(s);}

private:

  // Disallow postfix operators.  Unless we hide it, some compilers will
  // substitute the prefix operator in its place.
  RWBoolean		operator++(int);
  RWBoolean		operator--(int);
};


#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tpdlist.cc"
#endif

#endif	/* __RWTPDLIST_H__ */
