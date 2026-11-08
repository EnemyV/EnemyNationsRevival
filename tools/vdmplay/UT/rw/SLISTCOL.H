#ifndef  __RWSLISTCOL_H__
#define  __RWSLISTCOL_H__

/*
 * Declarations for Singly linked list of RWCollectables.
 *
 * $Id: slistcol.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: slistcol.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  02:22:57  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:10:59  vriezen
 * Move all files to 6.1
 *
 * Revision 2.9  1994/03/04  01:04:16  jims
 * Override isEqual member function from RWCollectable to return
 * TRUE or FALSE based on operator==
 *
 * Revision 2.8  1993/11/10  23:17:26  jims
 * Change order of inheritance in ObjectStore version
 *
 * Revision 2.7  1993/11/08  09:15:47  jims
 * Port to ObjectStore
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.3  1993/01/25  18:12:13  keffer
 * RW_NO_CONST_OVERLOADS->RW_NO_CONST_OVERLOAD
 *
 * Revision 2.1  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   04 Aug 1992 19:00:36   KEFFER
 * Added "virtual" keyword to destructor for documentation purposes.
 * 
 *    Rev 1.3   22 May 1992 17:04:16   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.2   18 Feb 1992 09:54:46   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:24   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:17:10   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/slist.h"
#include "rw/seqcltn.h"
#include "rw/iterator.h"

/****************************************************************
 *								*
 *			RWSlistCollectables			*
 *								*
 ****************************************************************/

class RWExport RWSlistCollectables : public RWSequenceable, public RWSlist {

  friend class RWExport RWSlistCollectablesIterator;
  RWDECLARE_COLLECTABLE(RWSlistCollectables)

public:	 
  RWSlistCollectables();
  RWSlistCollectables(RWCollectable* a) : RWSlist(a) { }
  virtual ~RWSlistCollectables();

  RWBoolean		operator==(const RWSlistCollectables&) const;

  /***************  Member virtual functions ***********************/
  virtual RWCollectable*	append(RWCollectable* a)	// Add at tail of list.
    {return (RWCollectable*)RWSlist::append(a);}
  virtual void			apply(RWapplyCollectable ap, void* x) 
    { RWSlist::apply((RWapplyGeneric)ap, x); }
  virtual RWCollectable*&	at(size_t i)
    { return (RWCollectable*&)RWSlist::at(i); }
#ifndef RW_NO_CONST_OVERLOAD
  virtual const RWCollectable*	at(size_t i) const
    { return (const RWCollectable*)RWSlist::at(i); }
#endif
  virtual void 			clear() 
    { RWSlist::clear(); }
//virtual void			clearAndDestroy();
  virtual RWBoolean		contains(const RWCollectable* a) const
    {return RWCollection::contains(a);}
  virtual size_t		entries() const 
    {return RWSlist::entries();}
  virtual RWCollectable*	find(const RWCollectable* a) const
    { return (RWCollectable*)RWSlist::find(rwIsEqualFun, a); }
  virtual RWCollectable* 	first() const		// Return value at head of list.
    {return (RWCollectable*) RWSlist::first();}
  virtual size_t		index(const RWCollectable* a) const
    { return RWSlist::index(rwIsEqualFun, a); }
  virtual RWCollectable*	insert(RWCollectable* a)
    { return (RWCollectable*)RWSlist::append(a); }
  virtual RWCollectable*	insertAt(size_t i, RWCollectable* a)
    { return (RWCollectable*)RWSlist::insertAt(i,a); }
  virtual RWBoolean		isEmpty() const 
    {return RWSlist::isEmpty();}
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual RWCollectable*	last() const	// Return value at tail of list.
    {return (RWCollectable*) RWSlist::last();}
  virtual size_t		occurrencesOf(const RWCollectable* a) const
    { return RWSlist::occurrencesOf(rwIsEqualFun, a); }
  virtual RWCollectable*	prepend(RWCollectable* a)
    { return (RWCollectable*)RWSlist::prepend(a); }
  virtual RWCollectable*	remove(const RWCollectable* a)
    { return (RWCollectable*)RWSlist::remove(rwIsEqualFun, a); }
  //virtual void		removeAndDestroy(const RWCollectable* a);
  
/****************  (Redefined) Special Functions *****************/
  RWBoolean 			containsReference(const RWCollectable* a) const
    { return RWSlist::containsReference(a); }
  RWCollectable*		findReference(const RWCollectable* a) const 
    { return (RWCollectable*)RWSlist::findReference(a); }
  RWCollectable*		get()		// Return value and delete link at head of list.
    {return (RWCollectable*) RWSlist::get();}
  size_t	 		occurrencesOfReference(const RWCollectable* a) const
    { return RWSlist::occurrencesOfReference(a); }
  RWCollectable*		removeReference(const RWCollectable* a)
    { return (RWCollectable*)RWSlist::removeReference(a); }
/*****************************************************************/

  // For backwards compatiblity:
  virtual RWCollectable*	insertAfter(int i, RWCollectable* a)
    { return (RWCollectable*)RWSlist::insertAfter(i,a); }

};


/****************************************************************
 *								*
 *		RWSlistCollectablesIterator			*
 *								*
 ****************************************************************/

/*
 * Iterator for singly-linked list (moves iterator through the list);
 * starts by pointing at last link.
 */
class RWExport RWSlistCollectablesIterator 
                   : public RWIterator, public RWSlistIterator {
public:
  RWSlistCollectablesIterator(RWSlistCollectables& s);
  virtual ~RWSlistCollectablesIterator();
  
/******************** (Redefined) Standard Iterator functions *********************/
  virtual RWCollectable*	findNext(const RWCollectable* a)
    { return (RWCollectable*)RWSlistIterator::findNext(rwIsEqualFun, a); }
  virtual RWCollectable*	key() const				// Return current item
    { return (RWCollectable*)RWSlistIterator::key(); }
  virtual RWCollectable*	operator()()				// Advance iterator
    { return (RWCollectable*)RWSlistIterator::operator()(); }
  virtual void			reset()
    { RWSlistIterator::reset(); }
  
/****************** (Redefined) Special RWSlistIterator functions ********************/

  RWCollectable*		findNextReference(const RWCollectable* a)
    { return (RWCollectable*)RWSlistIterator::findNextReference(a); }
  RWCollectable*		insertAfterPoint(RWCollectable* a)	// Insert item behind iterator.
    { return (RWCollectable*)RWSlistIterator::insertAfterPoint(a); }
  RWCollectable*		remove()				// Remove current item
    { return (RWCollectable*)RWSlistIterator::remove(); }
  RWCollectable*		removeNext(const RWCollectable* a)
    { return (RWCollectable*)RWSlistIterator::removeNext(rwIsEqualFun, a); }
  RWCollectable*		removeNextReference(const RWCollectable* a)
    { return (RWCollectable*)RWSlistIterator::removeNextReference(a); }
/*****************************************************************/
};

#endif  /* __RWSLISTCOL_H__ */
