#ifndef __RWTPHDICT_H__
#define __RWTPHDICT_H__

/*
 * RWTPtrHashDictionary: A parameterized hashing dictionary using pointers
 *                       to keys of type K and pointers to values of type V.
 *
 * $Id: tphdict.h,v 6.4 1994/07/12 19:58:19 vriezen Exp $
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
 * Assumes that KP has:
 *
 *   - well-defined equality semantics (KP::operator==(const KP&)).
 *
 ***************************************************************************
 *
 * $Log: tphdict.h,v $
 * Revision 6.4  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/21  00:34:06  myersn
 * move virtual dtor out-of-line.
 *
 * Revision 6.2  1994/06/15  21:29:52  myersn
 * add buckets() member for use with resize(size_t).
 *
 * Revision 6.1  1994/04/15  19:47:50  vriezen
 * Move all files to 6.1
 *
 * Revision 2.13  1994/01/13  12:08:58  jims
 * KP* val ---> const KP* val   in hashIndex
 *
 * Revision 2.12  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 2.11  1994/01/03  23:33:28  jims
 * ObjectStore version: Add #include <ostore/ostore.hh>
 *
 * Revision 2.10  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.9  1993/11/08  12:17:42  jims
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
 * Revision 2.5  1993/02/17  19:07:23  keffer
 * Now uses RWTPtrVector<T> instead of RWTValVector<T> for the buckets
 *
 * Revision 2.4  1993/02/12  00:59:40  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.3  1993/02/07  22:32:54  keffer
 * Added copy constructor and assignment operator for iterator
 *
 * Revision 2.2  1993/01/29  20:02:12  keffer
 * Ported to cfront V3.0
 *
 *    Rev 1.0   02 Mar 1992 16:10:54   KEFFER
 * Initial revision.
 */

#include "rw/tpsldict.h"
#include "rw/tpvector.h"

template <class KP, class VP> class RWTExport RWTPtrHashDictionaryIterator;

template <class KP, class VP> class RWTExport RWTPtrHashDictionary
{

public:

  // Constructors:
  RWTPtrHashDictionary
  (
    unsigned (*hashKey)(const KP&),	// Hashing function
    size_t size = RWDEFAULT_CAPACITY	// No. of buckets
  );

  RWTPtrHashDictionary
  (
    const RWTPtrHashDictionary<KP,VP>&
  );

  ~RWTPtrHashDictionary();

  // Operators:
  RWTPtrHashDictionary<KP,VP> &	operator=(const RWTPtrHashDictionary<KP,VP>&);

  // Look up key, add if not there:
  VP*&			operator[](KP* key);

  // Member functions:
  void			applyToKeyAndValue(void (*applyFun)(KP*,VP*&,void*), void*);

  void			clear();
  void			clearAndDestroy();

  RWBoolean		contains(const KP*) const;	// Contain key?

  size_t		entries() const		{return nitems_;}

  RWBoolean		isEmpty() const		{return nitems_==0;}

  KP*			find(const KP* key) const;
  VP*			findValue(const KP* key) const;
  KP*			findKeyAndValue(const KP* key, VP*& retVal) const;

  void			insertKeyAndValue(KP* key, VP* value)
	{ (*this)[key] = value;}

  KP*			remove(const KP* k);

  void			resize(size_t); // Change # of buckets
  size_t        	buckets() const // report # of buckets
        { return table_.length(); }


protected:

  size_t		hashIndex(const KP* val) const
	{ return (size_t)(*hashFun_)(*val) % table_.length(); }

  RWTPtrVector<RWTPtrSlistDictionary<KP,VP> > table_;
  size_t		nitems_;
  unsigned		(*hashFun_)(const KP&);

private:

friend class RWTPtrHashDictionaryIterator<KP,VP>;

};

/****************************************************************
 *								*
 *	Declarations for RWTPtrHashDictionaryIterator<KP,VP>	*
 *								*
 ****************************************************************/

template <class KP, class VP>
class RWTExport RWTPtrHashDictionaryIterator
{

public:

  RWTPtrHashDictionaryIterator(RWTPtrHashDictionary<KP,VP>& s);
  RWTPtrHashDictionaryIterator(const RWTPtrHashDictionaryIterator<KP,VP>& h);
  ~RWTPtrHashDictionaryIterator() {delete iterator_;}

  RWTPtrHashDictionaryIterator<KP,VP>&
		operator=(const RWTPtrHashDictionaryIterator<KP,VP>& h);

  RWBoolean		operator++();		// Advance and test
  KP*			operator()()
    { return ++(*this)? iterator_->key() : rwnil; }

  RWTPtrHashDictionary<KP,VP>* container() const	{return myDict_;}

  KP*			key() const		{return iterator_->key();}

  void			reset();
  void			reset(RWTPtrHashDictionary<KP,VP>& s);

  VP*			value() const		{return iterator_->value();}

private:

  RWTPtrHashDictionary<KP,VP>* myDict_;
  size_t idx_;
  RWTPtrSlistDictionaryIterator<KP,VP>*	iterator_;

  void			nextChain();		// Advance to the next chain

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);

};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tphdict.cc"
#endif

#endif	/* __RWTPHDICT_H__ */

