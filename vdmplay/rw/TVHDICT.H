#ifndef __RWTVHDICT_H__
#define __RWTVHDICT_H__

/*
 * RWTValHashDictionary: A parameterized hashing dictionary of keys K and values V.
 *
 * $Id: tvhdict.h,v 6.4 1994/07/12 19:58:19 vriezen Exp $
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
 * Note that the constructor takes a function with prototype
 *
 *   unsigned hashFun(const K&);
 *
 * The argument is a reference to a const object of type K.
 * This function should return a suitable hashing value for an instance
 * of class K.
 *
 ***************************************************************************
 *
 * $Log: tvhdict.h,v $
 * Revision 6.4  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/21  00:34:18  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 6.2  1994/06/15  21:33:05  myersn
 * add buckets() member for use with resize(size_t).
 *
 * Revision 6.1  1994/04/15  19:48:19  vriezen
 * Move all files to 6.1
 *
 * Revision 2.12  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.11  1993/11/11  00:39:32  jims
 * Fix type of hashFun in ObjectStore version
 *
 * Revision 2.10  1993/11/08  14:57:02  jims
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
 * Revision 2.6  1993/02/17  19:46:03  keffer
 * Now uses RWTPtrVector<T> rather than RWTValVector<T*>
 *
 * Revision 2.5  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.4  1993/02/12  00:18:50  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.3  1993/02/07  22:32:54  keffer
 * Added copy constructor and assignment operator for iterator
 *
 * Revision 2.2  1993/01/29  20:06:50  keffer
 * Ported to cfront V3.0
 *
 *    Rev 1.0   02 Mar 1992 16:10:54   KEFFER
 * Initial revision.
 */


#include "rw/tvsldict.h"
#include "rw/tpvector.h"

template <class K, class V> class RWTExport RWTValHashDictionaryIterator;

template <class K, class V> class RWTExport RWTValHashDictionary
{

public:

  // Constructors:
  RWTValHashDictionary
  (
    unsigned (*hashKey)(const K&),	// Hashing function
    size_t size = RWDEFAULT_CAPACITY	// No. of buckets
   );

  RWTValHashDictionary
  (
    const RWTValHashDictionary<K,V>&
  );

  ~RWTValHashDictionary();

  // Operators:
  RWTValHashDictionary<K,V> &	operator=(const RWTValHashDictionary<K,V>&);

  // Look up key, add if not there:
  V&			operator[](const K& key);

  // Member functions:
  void			applyToKeyAndValue(void (*applyFun)(const K&, V&, void*), void*);

  void			clear();

  RWBoolean		contains(const K&) const; // Contain key?

  size_t		entries() const		{return nitems_;}

  RWBoolean		isEmpty() const		{return nitems_==0;}

  RWBoolean		find(const K& key, K& retKey) const;
  RWBoolean		findValue(const K& key, V& retVal) const;
  RWBoolean		findKeyAndValue(const K& key, K& retKey, V& retVal) const;

  void			insertKeyAndValue(const K& key, const V& value)
	{ (*this)[key] = value;}

  RWBoolean		remove(const K& k);

  void			resize(size_t); // Change # of buckets
  size_t        	buckets() const // report # of buckets
        { return table_.length(); }

protected:

  size_t		hashIndex(const K& val) const
	{ return (size_t)(*hashFun_)(val) % table_.length(); }

  RWTPtrVector<RWTValSlistDictionary<K,V> > table_;
  size_t		nitems_;
  unsigned		(*hashFun_)(const K&);

private:

friend class RWTValHashDictionaryIterator<K,V>;

};

/****************************************************************
 *								*
 *	Declarations for RWTValHashDictionaryIterator<K,V>	*
 *								*
 ****************************************************************/

template <class K, class V>
class RWTExport RWTValHashDictionaryIterator
{

public:

  RWTValHashDictionaryIterator(RWTValHashDictionary<K,V>& s);
  RWTValHashDictionaryIterator(const RWTValHashDictionaryIterator<K,V>& h);
  ~RWTValHashDictionaryIterator() {delete iterator_;}

  RWTValHashDictionaryIterator<K,V> &
		operator=(const RWTValHashDictionaryIterator<K,V>& h);

  RWBoolean		operator++();		// Advance and test
  RWBoolean		operator()()		{ return ++(*this); }

  RWTValHashDictionary<K,V>* container() const	{return myDict_;}

  K			key() const		{return iterator_->key();}

  void			reset();
  void			reset(RWTValHashDictionary<K,V>& s);

  V			value() const		{return iterator_->value();}

private:

  RWTValHashDictionary<K,V>* myDict_;
  size_t idx_;
  RWTValSlistDictionaryIterator<K,V>* iterator_;

  void			nextChain();		// Advance to the next chain

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);
};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvhdict.cc"
#endif

#endif	/* __RWTVHDICT_H__ */

