#ifndef __RWTPHASHT_H__
#define __RWTPHASHT_H__

/*
 * RWTPtrHashTable<TP>:  A Bag of pointers to objects of type TP,
 *   implemented using a hashed lookup
 *
 * $Id: tphasht.h,v 6.5 1994/07/12 19:58:19 vriezen Exp $
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
 * This class implements a parameterized hash table of pointers to types TP.
 * It uses chaining to resolve hash collisions.
 *
 * Example use of this class:
 *
 *   #include <rw/cstring.h>
 *   #include <rw/tphasht.h>
 * 
 *   unsigned myHash(const RWCString& s){ return s.hash(); }
 *   
 *   RWTPtrHashTable<RWCString> bag(myHash);	// Bag of ptrs to RWCStrings
 *   
 *   bag.insert(new RWCString("a string"));
 *   bag.insert(new RWCString("another string"));
 *
 *   RWCString key("a string");
 *   bag.contains(&key);	// Returns true.
 *
 *
 * Note that the constructor for RWTPtrHashTable<TP> takes a function with
 * prototype
 *
 *   unsigned hashFun(const TP& s);
 *
 * The argument is a reference to a const object of type TP.
 * This function should return a suitable hashing value for an instance 
 * of class TP.
 *
 * Usually, the definition for such a function is trivial because hashing
 * functions have been defined for all Rogue Wave supplied classes.
 *
 ***************************************************************************
 *
 * $Log: tphasht.h,v $
 * Revision 6.5  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.4  1994/06/21  00:34:02  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 6.3  1994/06/17  17:08:52  vriezen
 * Added virtual dtor to make class more usable as base and quite some compilers
 *
 * Revision 6.2  1994/06/15  21:21:33  myersn
 * add buckets() for use with resize(size_t).
 *
 * Revision 6.1  1994/04/15  19:47:42  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 2.10  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.9  1993/11/08  11:10:39  jims
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
 * Revision 2.5  1993/02/17  18:37:06  keffer
 * Now uses RWTPtrVector<T> to hold buckets
 *
 * Revision 2.4  1993/02/12  00:46:24  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.3  1993/02/07  22:32:54  keffer
 * Added copy constructor and assignment operator for iterator
 *
 * Revision 2.2  1993/01/29  01:19:28  keffer
 * Ported to cfront v3.0
 *
 *    Rev 1.1   09 Sep 1992 13:37:02   KEFFER
 * Changed return type of RWTPtrHashTableIterator::operator().
 * 
 *    Rev 1.0   25 May 1992 15:59:04   KEFFER
 * Initial revision.
 * 
 */

#include "rw/tpslist.h"
#include "rw/tpvector.h"

template <class TP> class RWTExport RWTPtrHashTableIterator;

/****************************************************************
 *								*
 *		Declarations for RWTPtrHashTable<TP>		*
 *								*
 ****************************************************************/

template <class TP> class RWTExport RWTPtrHashTable
{

public:

  // Constructors, destructors, etc:
  RWTPtrHashTable
  (
    unsigned (*hashFun)(const TP&), 
    size_t size = RWDEFAULT_CAPACITY
  );

  RWTPtrHashTable
  (
    const RWTPtrHashTable<TP>&
  );

  virtual ~RWTPtrHashTable();

  RWTPtrHashTable<TP> &	operator=(const RWTPtrHashTable<TP>&);

  // Member functions:
  void		apply(void (*applyFun)(TP*, void*), void*);

  void		clear();

  void		clearAndDestroy();

  RWBoolean	contains(const TP* val) const;

  size_t	entries() const
	{return nitems_;}

  TP*		find(const TP* p) const;

  virtual void	insert(TP* val);

  RWBoolean	isEmpty() const
	{return nitems_==0;}

  size_t	occurrencesOf(const TP* val) const;

  virtual TP*		remove(const TP* val);
  virtual size_t	removeAll(const TP* val);

  void		resize(size_t);  // Change # of buckets
  size_t        buckets() const  // report # of buckets
        { return table_.length(); }

protected:  //methods

  size_t	hashIndex(const TP* val) const
	{return (size_t)(*hashFun_)(*val) % table_.length();}

  RWBoolean	insertOnce(TP* val);	// Useful for Sets

protected:  //data

  RWTPtrVector<RWTPtrSlist<TP> >	table_;
  size_t				nitems_;
  unsigned				(*hashFun_)(const TP&);

private:

friend class RWTPtrHashTableIterator<TP>;

};


/****************************************************************
 *								*
 *	Declarations for RWTPtrHashTableIterator<TP>		*
 *								*
 ****************************************************************/

template <class TP> class RWTExport RWTPtrHashTableIterator
{

public:

  RWTPtrHashTableIterator(RWTPtrHashTable<TP>& t);
  RWTPtrHashTableIterator(const RWTPtrHashTableIterator<TP>& h);
  ~RWTPtrHashTableIterator() {delete iterator_;}

  RWTPtrHashTableIterator<TP>&	operator=(const RWTPtrHashTableIterator<TP>& h);

  RWBoolean	operator++();		// Advance and test
  TP*		operator()()
	{return ++(*this) ? key() : rwnil;}

  RWTPtrHashTable<TP>*	container() const
	{return hashTable_;}

  TP*		key() const
	{return iterator_->key();}

  void			reset();
  void			reset(RWTPtrHashTable<TP>& t);

private:

  RWTPtrHashTable<TP>*		hashTable_;
  size_t			idx_;
  RWTPtrSlistIterator<TP>*	iterator_;

  void			nextIterator();
};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tphasht.cc"
#endif

#endif	/* __RWTPHASHT_H__ */
