#ifndef  __RWHASHTAB_H__
#define  __RWHASHTAB_H__

/*
 * Declarations for RWHashTable --- hash table lookup.
 *
 * $Id: hashtab.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 * Voice: (503) 754-3010	FAX: (503) 757-6650
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
 * Hash table look up with chaining
 * Duplicates are kept as distinct entries.
 * 
 ***************************************************************************
 *
 * $Log: hashtab.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/15  03:05:28  myersn
 * make RWHashTable::buckets() public, for use with resize().
 *
 * Revision 6.1  1994/04/15  19:10:15  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1994/03/22  23:04:52  vriezen
 * Cast entire expression to size_t in hashIndex, not just lhs.
 * Fixes problem on SunOS where size_t is signed, and hashIndex
 * returned negative number.
 *
 * Revision 2.10  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.9  1993/07/19  20:45:26  keffer
 * friend classes now use elaborated-type-specifier (ARM Sec. 11.4)
 *
 * Revision 2.8  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.7  1993/04/01  00:32:13  myersn
 * add RW prefix to GVector mention.
 *
 * Revision 2.6  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.5  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.4  1993/02/07  21:31:32  keffer
 * Provided copy constructor for iterator
 *
 * Revision 2.3  1993/01/29  21:52:56  keffer
 * *** empty log message ***
 *
 * Revision 2.2  1993/01/28  22:40:22  keffer
 * Optimized insertions slightly.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.5   04 Aug 1992 18:59:52   KEFFER
 * Added keyword "virtual" to destructor for documentation purposes.
 * 
 *    Rev 1.4   25 May 1992 15:50:36   KEFFER
 * Optimized, reducing size.
 * 
 *    Rev 1.3   22 May 1992 17:02:32   KEFFER
 * Simplified algorithms
 * 
 *    Rev 1.2   22 May 1992 10:41:04   KEFFER
 * Uses RWDECLARE_COLLECTABLE.
 * 
 *    Rev 1.1   29 Apr 1992 15:51:26   KEFFER
 * Removed default argument from copyOld()
 * 
 *    Rev 1.0   29 Apr 1992 14:51:18   KEFFER
 * Hashing now uses chaining to resolve collisions
 */

#include "rw/colclass.h"
#include "rw/iterator.h"
#include "rw/gvector.h"

class RWExport RWSlistCollectables;
class RWExport RWSlistCollectablesIterator;
class RWExport RWHashTableIterator;
// Declare a vector of pointers to RWSlistCollectables:
typedef RWSlistCollectables* RWSlistCollectablesP;
declare(RWGVector,RWSlistCollectablesP)

/****************************************************************
 *								*
 *			RWHashTable				*
 *								*
 ****************************************************************/

class RWExport RWHashTable : public RWCollection {

  friend class RWExport RWHashTableIterator;
  RWDECLARE_COLLECTABLE(RWHashTable)

public:

  RWHashTable(size_t N = RWCollection::DEFAULT_CAPACITY);
  RWHashTable (const RWHashTable&);
  virtual ~RWHashTable();

  /******************** Member operators ****************************/
  RWHashTable&			operator=(const RWHashTable&);
  RWBoolean 			operator<=(const RWHashTable&) const;
  RWBoolean 			operator==(const RWHashTable&) const;
  RWBoolean 			operator!=(const RWHashTable&) const;

  /****************** Virtual member functions *******************/
  virtual void			apply(RWapplyCollectable, void*);
//virtual RWspace		binaryStoreSize() const;
  virtual void			clear();
//virtual void			clearAndDestroy();
//virtual int			compareTo(const RWCollectable*) const;
//virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const		{return nitems_;}
  virtual RWCollectable*	find(const RWCollectable*) const;
//virtual unsigned		hash() const;
  virtual RWCollectable*	insert(RWCollectable*);
  virtual RWBoolean		isEmpty() const		{return nitems_==0;}
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual size_t		occurrencesOf(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable*);
//virtual void			removeAndDestroy(const RWCollectable*); 
//virtual void			restoreGuts(RWvistream&);
//virtual void			restoreGuts(RWFile&);
//virtual void			saveGuts(RWvostream&) const;
//virtual void			saveGuts(RWFile&) const;

/********************** Special functions **********************************/
  virtual void			resize(size_t n = 0);
  size_t			buckets() const {return table_.length();}

protected:

  RWGVector(RWSlistCollectablesP) table_;  // Table of pointers to chains.
  size_t			nitems_;   // Total number of stored objects.

protected:

  size_t			hashIndex(const RWCollectable* p) const
     { return (size_t)(p->hash() % buckets()); }
  RWCollectable*		insertIndex(size_t, RWCollectable*);
};



/****************************************************************
 *								*
 *		RWHashTableIterator				*
 *								*
 ****************************************************************/

class RWExport RWHashTableIterator : public RWIterator {

public:

  RWHashTableIterator(RWHashTable& h);
  RWHashTableIterator(const RWHashTableIterator&);
  virtual ~RWHashTableIterator();

  RWHashTableIterator&	operator=(const RWHashTableIterator&);

/*********** Virtual functions inherited from class RWIterator ***********/
  virtual RWCollectable*	findNext(const RWCollectable*);	// Find next matching item
  virtual RWCollectable*	key() const;			// Return current item
  virtual RWCollectable*	operator()();			// Advance iterator
  virtual void			reset();
/******************* Special iterator functions *******************************/
  RWCollectable*		remove();			// Remove current item
  RWCollectable*   		removeNext(const RWCollectable*);	// Remove next matching item

protected:

  void				nextIterator();

private:

  RWHashTable*			myHash_;
  size_t			idx_;		// which Slist is current
  RWSlistCollectablesIterator*	iterator_;	// an iterator for that Slist

};


#endif /* __RWHASHTAB_H__ */

