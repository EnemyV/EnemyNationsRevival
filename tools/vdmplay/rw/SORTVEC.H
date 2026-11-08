#ifndef __RWSORTVEC_H__
#define __RWSORTVEC_H__

/*
 * RWSortedVector -- sorted vector; uses insertion sort.
 *
 * $Id: sortvec.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: sortvec.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  01:22:18  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:11:00  vriezen
 * Move all files to 6.1
 *
 * Revision 2.8  1994/03/04  01:04:16  jims
 * Override isEqual member function from RWCollectable to return
 * TRUE or FALSE based on operator==
 *
 * Revision 2.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.6  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.5  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.4  1993/01/25  18:12:13  keffer
 * RW_NO_CONST_OVERLOADS->RW_NO_CONST_OVERLOAD
 *
 * Revision 2.2  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.1  1992/11/15  00:32:38  keffer
 * Added explicit "this->" dereference to suppress unfounded
 * cfront 3.0 warnings.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   22 May 1992 17:04:16   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   17 Mar 1992 19:21:10   KEFFER
 * Changed BOUNDS_CHECK to RWBOUNDS_CHECK
 * 
 *    Rev 1.2   18 Feb 1992 09:54:46   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:24   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:17:18   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

/*
 * This collection inherits from RWOrdered.  A few member functions
 * must be disallowed because the insertion order is determined internally
 * rather than by function calls.  Examples are "insertAfter()", or 
 * "prepend()".  A few others can be done more efficiently because we can
 * take advantage of the internal sorting of objects.  Examples are "index()",
 * "occurrencesOf()", etc.
 */

#include "rw/ordcltn.h"

// OrderedCollection iterator is used for SortedVector:
typedef RWOrderedIterator RWSortedVectorIterator;

/****************************************************************
 *								*
 *			RWSortedVector				*
 *								*
 ****************************************************************/

class RWExport RWSortedVector : public RWOrdered {

  RWDECLARE_COLLECTABLE(RWSortedVector)

public:

  RWSortedVector(size_t size = RWCollection::DEFAULT_CAPACITY);
  virtual ~RWSortedVector();

  RWBoolean			operator==(const RWSortedVector& c) const {return RWOrdered::operator==(c);}

  /****************** Redefined from RWOrdered *******************/

#ifndef RW_NO_CONST_OVERLOAD
  virtual const RWCollectable*	at(size_t) const;	// Cannot use as lvalue.
#endif
  virtual size_t		index(const RWCollectable* p) const;
  virtual RWCollectable*	insert(RWCollectable*);
  virtual size_t		occurrencesOf(const RWCollectable* a) const;
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  const RWCollectable*		operator[](size_t i) const		// With bounds checking
    { boundsCheck(i); return this->vec(i); }
  const RWCollectable*		operator()(size_t i) const		// Optional bounds checking
    {
#ifdef RWBOUNDS_CHECK
      boundsCheck(i);
#endif
      return this->vec(i);
    }

  /****************** Inherited from RWOrdered *******************/
//virtual void			apply(RWapplyCollectable, void*);
//virtual void			clear();
//virtual void			clearAndDestroy();
//virtual RWBoolean		contains(const RWCollectable*) const;
//virtual size_t		entries() const {return nitems;}
//virtual RWCollectable*	find(const RWCollectable* p) const;
//virtual RWCollectable*	first() const;
//virtual RWBoolean		isEmpty() const {return nitems==0;}
//virtual RWCollectable*	last() const;
//virtual RWCollectable*	remove(const RWCollectable*);
//virtual void			removeAndDestroy(const RWCollectable*);

//void				resize(size_t);	// Cannot shrink below population

  /****************** Disallowed from RWOrdered *******************/
private:
  virtual RWCollectable*&	at(size_t);	// No lvalues allowed for sorted vector
  virtual RWCollectable*	append(RWCollectable* a);
  virtual RWCollectable*	insertAt(size_t, RWCollectable*);
  virtual RWCollectable*	prepend(RWCollectable*);
  void				push(RWCollectable*);
  RWCollectable*		pop();
  RWCollectable*		top() const;

  // For backwards compatiblity:
  virtual RWCollectable*	insertAfter(int, RWCollectable*);
};

#endif /* __RWSORTVEC_H__ */

