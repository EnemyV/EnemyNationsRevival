#ifndef __RWTVHASHT_H__
#define __RWTVHASHT_H__

/*
 * RWTValHashTable<T>:  A Bag of values T, implemented using a hashed lookup
 *
 * $Id: tvhasht.h,v 6.5 1994/07/12 19:58:19 vriezen Exp $
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
 * This class implements a parameterized hash table of types T.
 * It uses chaining to resolve hash collisions.
 *
 * Example use of this class:
 *
 *   #include <rw/cstring.h>
 *   #include <rw/tvhasht.h>
 *   
 *   unsigned myHash(const RWCString& s){ return s.hash(); }
 *   
 *   RWTValHashTable<RWCString> bag(myHash);	// A bag of RWCStrings
 *   
 *   bag.insert("a string");	// Type conversion: char* to RWCString happens
 *   bag.insert("another string");
 *   bag.contains("a string");	// Returns true.
 *
 *
 * Note that the constructor for RWTValHashTable<T> takes a function with
 * prototype
 *
 *   unsigned hashFun(const T&);
 *
 * The argument is a reference to a const object of type T.
 * This function should return a suitable hashing value for an instance
 * of class T.
 *
 * Usually, the definition for such a function is trivial because hashing
 * functions have been defined for all Rogue Wave supplied classes.
 *
 ***************************************************************************
 *
 * $Log: tvhasht.h,v $
 * Revision 6.5  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.4  1994/06/21  00:34:14  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 6.3  1994/06/17  17:21:42  vriezen
 * Added virtual dtor or change dtor to virtual to enhance derivability and quiet some compilers.
 *
 * Revision 6.2  1994/06/15  21:25:23  myersn
 * add buckets() member for use with resize(size_t).
 *
 * Revision 6.1  1994/04/15  19:48:14  vriezen
 * Move all files to 6.1
 *
 * Revision 2.12  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.11  1993/12/10  19:41:42  jims
 * ObjectStore version: fix typo
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
 * Revision 2.2  1993/01/29  01:19:28  keffer
 * Ported to cfront v3.0
 *
 *    Rev 1.0   02 Mar 1992 16:10:52   KEFFER
 * Initial revision.
 */

#include "rw/tvslist.h"
#include "rw/tpvector.h"

template <class T> class RWTExport RWTValHashTableIterator;

/****************************************************************
 *								*
 *		Declarations for RWTValHashTable<T>		*
 *								*
 ****************************************************************/

template <class T> class RWTExport RWTValHashTable
{

public:

    // Constructors, destructors, etc:
  RWTValHashTable
  (
    unsigned (*hashFun)(const T&),	// Hashing function
    size_t size = RWDEFAULT_CAPACITY	// No. of buckets
  );

  RWTValHashTable
  (
    const RWTValHashTable<T>&
  );

  virtual ~RWTValHashTable();

  RWTValHashTable<T>&	operator=(const RWTValHashTable<T>&);

    // Member functions:
  void		apply(void (*applyFun)(const T&, void*), void*);

  void		clear();

  RWBoolean	contains(const T& val) const;

  size_t	entries() const
	{return nitems_;}

  RWBoolean	find(const T& a, T& k) const;  // First occurrence

  virtual void	insert(const T& val);

  RWBoolean	isEmpty() const
	{return nitems_==0;}

  size_t	occurrencesOf(const T& val) const;

  virtual RWBoolean	remove(const T& val);
  virtual size_t	removeAll(const T& val);

  void		resize(size_t); // Change # of buckets
  size_t        buckets() const  // report # of buckets
        { return table_.length(); }


protected:  //methods

  size_t	hashIndex(const T& val) const
	{ return (size_t)(*hashFun_)(val) % table_.length(); }

  RWBoolean	insertOnce(const T& val);	// Useful for Sets

protected:  //data

  RWTPtrVector<RWTValSlist<T> >	table_;
  size_t			nitems_;
  unsigned			(*hashFun_)(const T&);
private:

friend class RWTValHashTableIterator<T>;

};

/****************************************************************
 *								*
 *	Declarations for RWTValHashTableIterator<T>		*
 *								*
 ****************************************************************/

template <class T> class RWTExport RWTValHashTableIterator
{ 

public:

  RWTValHashTableIterator(RWTValHashTable<T>& t);
  RWTValHashTableIterator(const RWTValHashTableIterator<T>& h);
  ~RWTValHashTableIterator() {delete iterator_;}

  RWTValHashTableIterator<T>&	operator=(const RWTValHashTableIterator<T>& h);

  RWBoolean	operator++();		// Advance and test
  RWBoolean	operator()()
	{return ++(*this);}

  RWTValHashTable<T>* container() const
	{return hashTable_;}

  T		key() const
	{return iterator_->key();}

  void		reset();
  void		reset(RWTValHashTable<T>& t);

private:

  RWTValHashTable<T>*		hashTable_;
  int				idx_;
  RWTValSlistIterator<T>*	iterator_;

  void			nextIterator();
};


#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvhasht.cc"
#endif

#endif	/* __RWTVHASHT_H__ */
