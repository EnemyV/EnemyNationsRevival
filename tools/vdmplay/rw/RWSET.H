#ifndef  __RWSET_H__
#define  __RWSET_H__

/*
 * Declarations for RWSet --- hash table lookup.
 *
 * $Id: rwset.h,v 6.5 1994/07/12 19:58:19 vriezen Exp $
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
 * Duplicates are not allowed.
 * Hash table look up with chaining: derived from RWHashTable
 *
 * $Log: rwset.h,v $
 * Revision 6.5  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.4  1994/06/18  00:52:14  myersn
 * add explicit out-of-line dtor.
 *
 * Revision 6.3  1994/06/16  03:34:55  vriezen
 * Added RW_DECLARE_HOME
 *
 * Revision 6.2  1994/04/15  21:12:29  vriezen
 * Backed out (until next release) RW_DEFINE_HOME and RW_DECLARE_HOME support.
 *
 * Revision 6.1  1994/04/15  19:10:50  vriezen
 * Move all files to 6.1
 *
 * Revision 2.8  1994/03/23  20:42:45  vriezen
 * Add RW_DECLARE_HOME macro to RWSetIterator
 *
 * Revision 2.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.6  1993/07/19  20:45:26  keffer
 * friend classes now use elaborated-type-specifier (ARM Sec. 11.4)
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.3  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.1  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.6   25 May 1992 15:50:40   KEFFER
 * Optimized, reducing size.
 * 
 *    Rev 1.5   22 May 1992 17:04:16   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   29 Apr 1992 14:51:20   KEFFER
 * Hashing now uses chaining to resolve collisions
 *
 */

#include "rw/hashtab.h"
#include "rw/iterator.h"

class RWExport RWSetIterator;

/****************************************************************
 *								*
 *			RWSet					*
 *								*
 ****************************************************************/

class RWExport RWSet : public RWHashTable {

  friend class RWExport RWSetIterator;
  RWDECLARE_COLLECTABLE(RWSet)

public:

  RWSet(size_t N = RWCollection::DEFAULT_CAPACITY);
  RWSet (const RWSet& s) :  RWHashTable(s) { }
  virtual ~RWSet ();

  /******************** Member operators ****************************/
  void				operator=(const RWSet& s) { RWHashTable::operator=(s);}
  RWBoolean 			operator<=(const RWSet&) const;
//RWBoolean 			operator==(const RWSet&) const;
//RWBoolean 			operator!=(const RWSet&) const;

  /****************** Virtual member functions *******************/
//virtual void			apply(RWapplyCollectable, void*);
//virtual RWspace		binaryStoreSize() const;
//virtual void			clear();
  virtual void			clearAndDestroy();
//virtual int			compareTo(const RWCollectable*) const;
//virtual RWBoolean		contains(const RWCollectable*) const;
//virtual size_t		entries() const;
//virtual RWCollectable*	find(const RWCollectable*) const;
//virtual unsigned		hash() const;
  virtual RWCollectable*	insert(RWCollectable*);
//virtual RWBoolean		isEmpty() const		{return items==0;}
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual size_t		occurrencesOf(const RWCollectable* a) const;
//virtual RWCollectable*	remove(const RWCollectable*);
//virtual void			removeAndDestroy(const RWCollectable*); 
//virtual void			restoreGuts(RWvistream&);
//virtual void			restoreGuts(RWFile&);
//virtual void			saveGuts(RWvostream&) const;
//virtual void			saveGuts(RWFile&) const;

/********************** Special functions **********************************/
#ifdef RDEBUG
  // Print status of hash table:
  friend ostream&		operator<<(ostream&, const RWSet&);
#endif

protected:

};

/****************************************************************
 *								*
 *			RWSetIterator				*
 *								*
 ****************************************************************/

/*
 * RWSetIterator is an exact clone of RWHashTableIterator --
 */

class RWExport RWSetIterator : public RWHashTableIterator {
  RW_DECLARE_HOME(RWSetIterator)
public:

  RWSetIterator(RWSet& h) : RWHashTableIterator(h) { }
#ifdef RW_NO_ACCESS_ADJUSTMENT
  void				reset() { RWHashTableIterator::reset(); }
#else
  RWHashTableIterator::reset;
#endif

};

#endif /* __RWSET_H__ */
