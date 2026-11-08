#ifndef __RWTVSLDICT_H__
#define __RWTVSLDICT_H__

/*
 * RWTValSlistDictionary<K,V>: A dictionary of keys of type K, values of type V,
 *   implemented as a singly-linked list
 *
 * $Id: tvsldict.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * Stores a *copy* of the inserted items into the collection.
 *
 * Assumes that K and V have:
 *   - well-defined copy constructor (T::T(const T&) or equiv.);
 *   - well-defined assignment operator (T::operator=(const T&) or equiv.);
 *
 * Assumes that K has:
 *   - well-defined equality semantics (T::operator==(const T&));
 *
 * Assumes that V has:
 *   - a default constructor.
 *
 * Note that while these are automatically defined for builtin types
 * (such as "int", "double", or any pointer), you may need to provide
 * appropriate operators for your own classes, particularly those with
 * constructors and/or pointers to other objects.
 *
 ***************************************************************************
 *
 * $Log: tvsldict.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/07/05  21:56:47  nevis
 * Fixes bug #117 by defining explicitly a copy constructor.
 *
 * Revision 6.1.7.2  1994/07/05  20:57:30  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.2  1994/07/05  20:57:27  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.1  1994/04/15  19:48:32  vriezen
 * Move all files to 6.1
 *
 * Revision 2.10  1993/11/08  21:14:01  jims
 * Port to ObjectStore
 *
 * Revision 2.9  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.8  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.7  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.6  1993/02/17  22:21:30  keffer
 * Changed name of header file tvasslnk.h to tasslnk.h
 *
 * Revision 2.5  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.4  1993/02/12  00:18:50  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.3  1993/01/29  20:07:44  keffer
 * Added comment that V must have default constructor.
 *
 * Revision 2.2  1993/01/28  21:11:49  keffer
 * Ported to cfront V3.0
 *
 *    Rev 1.0   02 Mar 1992 16:10:54   KEFFER
 * Initial revision.
 */

#include "rw/tislist.h"
#include "rw/tasslnk.h"

template <class K, class V> class RWTExport RWTValSlistDictionaryIterator;

/****************************************************************
 *								*
 *	Declarations for RWTValSlistDictionary<K,V>		*
 *								*
 ****************************************************************/

template <class K, class V>
class RWTExport RWTValSlistDictionary 
                            : private RWTIsvSlist< RWTValAssocLink<K, V> >
{

public:

  RWTValSlistDictionary() {;}
  RWTValSlistDictionary(const RWTValSlistDictionary<K,V>&);
  ~RWTValSlistDictionary()
	{RWTIsvSlist<RWTValAssocLink<K,V> >::clearAndDestroy();}

  // Operators:
  RWTValSlistDictionary<K,V> &
			operator=(const RWTValSlistDictionary<K,V>&);

  // Look up key, add if not there:
  V&		operator[](const K& key);

  // Member functions:
  void		applyToKeyAndValue(void (*applyFun)(const K&, V& ,void*), void*);

  void		clear()
	{RWTIsvSlist<RWTValAssocLink<K,V> >::clearAndDestroy();}

  RWBoolean	contains(const K&) const;	// Contain key?

  size_t	entries() const
	{return RWTIsvSlist<RWTValAssocLink<K,V> >::entries();}

  RWBoolean	isEmpty() const
	{return RWTIsvSlist<RWTValAssocLink<K,V> >::isEmpty();}

  RWBoolean	find(const K& key, K& retKey) const;
  RWBoolean	findValue(const K& key, V& retVal) const;
  RWBoolean	findKeyAndValue(const K& key, K& retKey, V& retVal) const;

  void		insertKeyAndValue(const K& key, const V& value)
	{(*this)[key] = value;}

  RWBoolean	remove(const K& key);
  RWBoolean	remove(const K& key, K& retKey);

protected:

  RWTValAssocLink<K,V>*	findLink(const K&) const;
  RWTValAssocLink<K,V>*	removeLink(const K&);

private:

friend class RWTExport RWTValSlistDictionaryIterator<K,V>;

};

/****************************************************************
 *								*
 *     Declarations for RWTValSlistDictionaryIterator<K,V>	*
 *								*
 ****************************************************************/

template <class K, class V>
class RWTExport RWTValSlistDictionaryIterator :
	private RWTIsvSlistIterator<RWTValAssocLink<K,V> >
{

public:

  // Constructor:
  RWTValSlistDictionaryIterator(RWTValSlistDictionary<K,V>& d) 
	: RWTIsvSlistIterator<RWTValAssocLink<K,V> >(d) {;}

  RWTValSlistDictionaryIterator(const RWTValSlistDictionaryIterator<K,V>&);
     // Copy ctor added to address BUG#117

  // Operators:
  RWBoolean	operator++()
	{advance(); return cursor()!=container()->tailLink();}
  RWBoolean	operator+=(size_t n)
	{return RWTIsvSlistIterator<RWTValAssocLink<K,V> >::operator+=(n)!=rwnil;}
  RWBoolean	operator()()
	{advance(); return cursor()!=container()->tailLink();}

  RWTValSlistDictionary<K,V>*	container() const
	{return (RWTValSlistDictionary<K,V>*)RWTIsvSlistIterator<RWTValAssocLink<K,V> >::container();}

  K		key() const	{return cursor()->key_;}

  void		reset()
	{RWTIsvSlistIterator<RWTValAssocLink<K,V> >::reset();}
  void		reset(RWTValSlistDictionary<K,V>& s)
	{RWTIsvSlistIterator<RWTValAssocLink<K,V> >::reset(s);}

  V		value() const	{return cursor()->value_;}

private:

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);
};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvsldict.cc"
#endif

#endif	/* __RWTVSLDICT_H__ */

