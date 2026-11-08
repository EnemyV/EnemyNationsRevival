#ifndef __RWORDCLTN_H__
#define __RWORDCLTN_H__

/*
 * RWOrdered --- Ordered Collection
 *
 * $Id: ordcltn.h,v 6.4 1994/07/12 19:58:19 vriezen Exp $
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
 * The copy constructor and assignment operator use memberwise
 * initialization and assignment, respectively.
 *
 ***************************************************************************
 *
 * $Log: ordcltn.h,v $
 * Revision 6.4  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/18  02:22:57  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.2  1994/06/18  01:48:36  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:10:33  vriezen
 * Move all files to 6.1
 *
 * Revision 2.10  1994/03/04  01:04:16  jims
 * Override isEqual member function from RWCollectable to return
 * TRUE or FALSE based on operator==
 *
 * Revision 2.9  1994/01/13  04:35:39  jims
 * Move member function removeAt to the public interface
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.6  1993/04/01  00:50:34  myersn
 * change GVector to RWGVector.
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
 * Revision 2.1  1992/11/15  00:03:02  keffer
 * Added explicit "this->" dereference to suppress unfounded
 * cfront 3.0 warnings.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.8   22 May 1992 17:04:14   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.7   17 Mar 1992 19:21:10   KEFFER
 * Changed BOUNDS_CHECK to RWBOUNDS_CHECK
 * 
 *    Rev 1.6   04 Mar 1992 09:03:52   KEFFER
 * nil changed to rwnil
 * 
 *    Rev 1.5   18 Feb 1992 09:54:32   KEFFER
 * 
 *    Rev 1.4   05 Nov 1991 13:52:06   keffer
  GVector(RWCollectableP)	vec;		// An array of pointers to objects.
 * Now declares GVector of RWCollectableP, instead of relying on collect.h
 * 
 *    Rev 1.3   28 Oct 1991 09:08:18   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.2   20 Aug 1991 18:53:12   keffer
 * pop() now returns nitems-1
 * 
 *    Rev 1.1   29 Jul 1991 14:13:56   keffer
 * Added member function data().
 * 
 *    Rev 1.0   28 Jul 1991 08:15:54   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/seqcltn.h"
#include "rw/iterator.h"
#include "rw/gvector.h"
declare(RWGVector,RWCollectableP)

/****************************************************************
 *								*
 *			RWOrdered				*
 *								*
 ****************************************************************/

class RWExport RWOrdered : public RWSequenceable {

  friend class RWExport RWOrderedIterator;
  RWDECLARE_COLLECTABLE(RWOrdered)

public:

  RWOrdered(size_t size = RWCollection::DEFAULT_CAPACITY);
  virtual ~RWOrdered();

  /******************** Member operators ****************************/
  RWBoolean			operator==(const RWOrdered&) const;

  /****************** Virtual member functions *******************/
  virtual RWCollectable*	append(RWCollectable* a);
  virtual void			apply(RWapplyCollectable, void*);
  virtual RWCollectable*&	at(size_t);	// Can use as lvalue
#ifndef RW_NO_CONST_OVERLOAD
  virtual const RWCollectable*	at(size_t) const;	// Cannot use as lvalue.
#endif
  virtual void			clear();
//virtual void			clearAndDestroy();
//virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const {return nitems;}
  virtual RWCollectable*	find(const RWCollectable*) const;	// First occurrence
  virtual RWCollectable*	first() const;
  virtual size_t		index(const RWCollectable*) const; // Returns RW_NPOS if not found.
  virtual RWCollectable*	insert(RWCollectable*);		 // Appends.
  virtual RWCollectable*	insertAt(size_t, RWCollectable*);
  virtual RWBoolean		isEmpty() const {return nitems==0;}
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual RWCollectable*	last() const;
  virtual size_t		occurrencesOf(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable*);	// Remove first occurrence
//virtual void			removeAndDestroy(const RWCollectable*);
  virtual RWCollectable*	prepend(RWCollectable*);

  /*********************** Special functions ******************************/
  RWCollectable*&		operator[](size_t);	// With bounds checking
  RWCollectable*&		operator()(size_t);	// Optional bounds checking
#ifndef RW_NO_CONST_OVERLOAD
  RWCollectable*		operator[](size_t) const;	// With bounds checking
  RWCollectable*		operator()(size_t) const;	// Optional bounds checking
#endif
  const RWCollectableP*		data() const;		// Use with care.
  void				push(RWCollectable*);	// Alternative stack.
  RWCollectable*		pop();
  RWCollectable*		removeAt(size_t);
  void				resize(size_t);	// Cannot shrink below population
  RWCollectable*		top() const;

  // For backwards compatiblity:
  virtual RWCollectable*	insertAfter(int, RWCollectable*);

protected:

  size_t			nitems;
  RWGVector(RWCollectableP)	vec;		// An array of pointers to objects.

  void				boundsCheck(size_t) const;

};

/****************************************************************
 *								*
 *			RWOrderedIterator			*
 *								*
 ****************************************************************/

class RWExport RWOrderedIterator : public RWIterator {
public:
  RWOrderedIterator(const RWOrdered& ord) { theCollection=&ord; here=RW_NPOS;}
  virtual ~RWOrderedIterator();
  virtual RWCollectable*	findNext(const RWCollectable*); // Find next matching item
  virtual RWCollectable*	key() const;		  // Return current key
  virtual RWCollectable*	operator()();		  // Advance iterator
  virtual void			reset() {here=RW_NPOS;}
private:
  const RWOrdered*		theCollection;
  size_t			here;
};

//////////////////////////////////////////////////////////////////////////
//									//
//			INLINES						//
//									//
//////////////////////////////////////////////////////////////////////////

inline RWCollectable*&
RWOrdered::operator[](size_t i)
{ boundsCheck(i); return vec(i); }

inline RWCollectable*&
RWOrdered::operator()(size_t i)
{
#ifdef RWBOUNDS_CHECK
  boundsCheck(i);
#endif
  return vec(i);
}

#ifndef RW_NO_CONST_OVERLOAD
inline RWCollectable*
RWOrdered::operator[](size_t i) const
{ boundsCheck(i); return this->vec(i); }

inline RWCollectable*
RWOrdered::operator()(size_t i) const
{
#ifdef RWBOUNDS_CHECK
  boundsCheck(i);
#endif
  return this->vec(i);
}
#endif	/* RW_NO_CONST_OVERLOAD */

inline const RWCollectableP*
RWOrdered::data() const
{
  return vec.data();
}

inline void
RWOrdered::push(RWCollectable* c)
{ insert(c); }

inline RWCollectable*
RWOrdered::pop()
{ return nitems>0 ? removeAt(nitems-1) : rwnil; }

inline RWCollectable*
RWOrdered::top() const
{ return nitems>0 ? this->vec(nitems-1) : rwnil; }

#endif /* __RWORDCLTN_H__ */
