#ifndef __RWTPSLIST_H__
#define __RWTPSLIST_H__

/*
 * RWTPtrSlist<TP>: A singly-linked list of pointers to objects of type TP.
 *
 * $Id: tpslist.h,v 6.3 1994/07/18 19:04:25 jims Exp $
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
 * $Log: tpslist.h,v $
 * Revision 6.3  1994/07/18  19:04:25  jims
 * Add copyright
 *
 * Revision 6.2  1994/07/05  21:56:42  nevis
 * Fixes bug #117 by defining explicitly a copy constructor.
 *
 * Revision 6.3  1994/07/05  21:05:30  nevis
 * Fixed typo.
 *
 * Revision 6.2  1994/07/05  20:57:22  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.1  1994/04/15  19:48:02  vriezen
 * Move all files to 6.1
 *
 * Revision 2.12  1994/01/12  03:36:46  jims
 * Add const to parameter of findLeftVal
 *
 * Revision 2.11  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 2.10  1993/12/31  00:25:26  jims
 * Left justify pre-preprocessor directives
 *
 * Revision 2.9  1993/11/08  13:37:24  jims
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
 * Revision 2.2  1993/01/27  21:39:43  keffer
 * Now uses separate definitions file
 *
 * 
 */

#include "rw/tislist.h"

template <class TP> class RWTExport RWTPtrSlistIterator;

/****************************************************************
 *								*
 *		Declarations for RWTPtrSlink<TP>		*
 *								*
 ****************************************************************/

/*
 * This is the actual link that is stored in the linked list.
 * It includes data of type "TP".
 */
template <class TP> class RWTExport RWTPtrSlink : public RWIsvSlink
{

public:

  RWTPtrSlink(TP* a) : info_(a) {;}
  RWTPtrSlink<TP>*	next() const	{return (RWTPtrSlink<TP>*)next_;}

  TP*		info_;

};

/****************************************************************
 *								*
 *	Declarations for RWTPtrSlist<TP>			*
 *								*
 ****************************************************************/

template <class TP> class RWTExport RWTPtrSlist 
                                 : private RWTIsvSlist< RWTPtrSlink<TP> >
{
public:

  RWTPtrSlist() {;}
  RWTPtrSlist(const RWTPtrSlist<TP>&);
  ~RWTPtrSlist() { clear(); }

  // Operators:
  RWTPtrSlist<TP>&	operator=(const RWTPtrSlist<TP>&);

  TP*&		operator[](size_t i)
	{return RWTIsvSlist<RWTPtrSlink<TP> >::at(i)->info_;}
  TP*		operator[](size_t i) const
	{return RWTIsvSlist<RWTPtrSlink<TP> >::at(i)->info_;}
  
  // Member functions:
  void		append(TP* a);

  void		apply(void (*applyFun)(TP*, void*), void*);

  TP*&		at(size_t i)
	{return RWTIsvSlist<RWTPtrSlink<TP> >::at(i)->info_;}
  TP*		at(size_t i) const
	{return RWTIsvSlist<RWTPtrSlink<TP> >::at(i)->info_;}

  void		clear()
	{RWTIsvSlist<RWTPtrSlink<TP> >::clearAndDestroy();}

  void		clearAndDestroy();

  RWBoolean	contains(const TP* a) const;
  RWBoolean	contains(RWBoolean (*testFun)(TP*, void*), void*) const;

  size_t	entries() const
	{return RWTIsvSlist<RWTPtrSlink<TP> >::entries();}

  TP*		find(const TP* p) const;
  TP*		find(RWBoolean (*testFun)(TP*, void*), void* d) const;

  TP*		first() const
	{return isEmpty() ? rwnil : firstLink()->info_;}

  TP*		get()
	{return peel(RWTIsvSlist<RWTPtrSlink<TP> >::get());}

  size_t	index(const TP* a) const;
  size_t	index(RWBoolean(*testFun)(TP*, void*), void*) const;

  void		insert(TP* a);

  void		insertAt(size_t i, TP* a);

  RWBoolean	isEmpty() const
	{return RWTIsvSlist<RWTPtrSlink<TP> >::isEmpty();}

  TP*		last() const
	{return isEmpty() ? rwnil : lastLink()->info_;}

  size_t	occurrencesOf(const TP* a) const;
  size_t	occurrencesOf(RWBoolean (*testFun)(TP*, void*), void*) const;

  void		prepend(TP* a);

  TP*		remove(const TP* p);
  TP*		remove(RWBoolean (*testFun)(TP*, void*), void* d);

  TP*		removeAt(size_t i)
	{return peel(RWTIsvSlist<RWTPtrSlink<TP> >::removeAt(i));}

  TP*		removeFirst()
	{return peel(RWTIsvSlist<RWTPtrSlink<TP> >::removeFirst());}

  TP*		removeLast()
	{return peel(RWTIsvSlist<RWTPtrSlink<TP> >::removeLast()); }

  size_t	removeAll(const TP* a);
  size_t	removeAll(RWBoolean (*testFun)(TP*, void*), void*);

protected:

  RWTPtrSlink<TP>*	findLeftVal(const TP*) const;
  RWTPtrSlink<TP>*	findLeftFun(RWBoolean (*testFun)(TP*, void*), void*) const;

  static TP*		peel(RWTPtrSlink<TP>* link);	// return "info_"; delete link

private:

friend class RWTExport RWTPtrSlistIterator<TP>;

};


/****************************************************************
 *								*
 *	Declarations for RWTPtrSlistIterator<TP>		*
 *								*
 ****************************************************************/

template <class TP> class RWTExport RWTPtrSlistIterator :
	            private RWTIsvSlistIterator<RWTPtrSlink<TP> >
{

public:

  RWTPtrSlistIterator(RWTPtrSlist<TP>& s) :
	RWTIsvSlistIterator<RWTPtrSlink<TP> >(s) {;}

  RWTPtrSlistIterator(const RWTPtrSlistIterator<TP>&);
	// Copy ctor added for BUG#117

  // Operators:
  RWBoolean	operator++()
	{advance(); return cursor()!=container()->tailLink();}
  RWBoolean	operator+=(size_t n)
	{return RWTIsvSlistIterator<RWTPtrSlink<TP> >::operator+=(n)!=rwnil;}
  TP*		operator()()
	{advance(); return cursor()==container()->tailLink() ? rwnil : cursor()->info_;}

  // Methods
  RWTPtrSlist<TP>*	container() const
	{return (RWTPtrSlist<TP>*)RWTIsvSlistIterator<RWTPtrSlink<TP> >::container();}

  TP*		findNext(const TP* p);
  TP*		findNext(RWBoolean (*testFun)(TP*, void*), void* d);

  void		insertAfterPoint(TP* a);

  TP*		key() const
	{return isActive() ? cursor()->info_ : rwnil;}

  TP*		remove();

  TP*		removeNext(const TP*);
  TP*		removeNext(RWBoolean (*testFun)(TP*, void*), void*);

  void		reset()
	{RWTIsvSlistIterator<RWTPtrSlink<TP> >::reset();}
  void		reset(RWTPtrSlist<TP>& s)
	{RWTIsvSlistIterator<RWTPtrSlink<TP> >::reset(s);}

private:

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);
};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tpslist.cc"
#endif

#endif	/* __RWTPSLIST_H__ */
