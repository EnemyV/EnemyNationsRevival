#ifndef __RWGORDVEC_H__
#define __RWGORDVEC_H__

/*
 * RWGOrderedVector(val): <generic.h> style Ordered Vector of type val
 *
 * $Id: gordvec.h,v 6.3 1994/07/14 21:04:32 jims Exp $
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
 * To use this class you must declare and implement the 
 * base class RWGVector(val), as well as the ordered collection itself.
 *
 * Example: To declare and implement an ordered vector of doubles:
 *
 *   #include <rw/gordvec.h>
 *   
 *   declare(RWGVector,double)		// Declare the base class
 *   declare(RWGOrderedVector,double)	// Declare the ordered vector
 *
 *   // In one and only one .cpp file you must put the following:
 *   implement(RWGVector,double)	// Implement the base class
 *   implement(RWGOrderedVector,double)	// Implement the ordered vector
 *
 *   // To use:
 *   void func(){
 *     RWGOrderedVector(double) ord;
 *     ord.insert(22.1);
 *     ord.insert(5.0);		// etc.
 *   }
 *
 ***************************************************************************
 *
 * $Log: gordvec.h,v $
 * Revision 6.3  1994/07/14  21:04:32  jims
 * Use RWGExport for generic collection classes
 *
 * Revision 6.2  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:03  vriezen
 * Move all files to 6.1
 *
 * Revision 2.13  1994/03/02  19:19:33  jims
 * New message INDEXERR shows invalid index and number of
 * items in collection
 *
 * Revision 2.12  1994/01/20  02:38:55  keffer
 * Replaced a couple of "int"s with "size_t".
 *
 * Revision 2.11  1993/12/28  23:56:11  keffer
 * Added member function data(): to be used with care
 *
 * Revision 2.10  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.9  1993/08/03  21:07:15  dealys
 * Ported to MPW C++ 3.3 - RW_BROKEN_TOKEN_PASTE
 *
 * Revision 2.8  1993/05/31  21:45:26  keffer
 * New messaging architecture for localization
 *
 * Revision 2.7  1993/05/18  00:41:46  keffer
 * Now uses new exception handling facility
 *
 * Revision 2.6  1993/04/12  16:07:49  keffer
 * Now uses RW error facility instead of "generic" error facility.
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/25  03:50:41  keffer
 * Added prefix RW to class name
 *
 * Revision 2.3  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.2  1993/02/08  00:24:55  keffer
 * Changed member data names
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   14 May 1992 11:53:22   KEFFER
 * Changed name of RWGVector::resize() to reshape().
 * 
 *    Rev 1.2   13 May 1992 15:37:42   KEFFER
 * Added push() and pop().
 * 
 *    Rev 1.0   04 Mar 1992 18:57:56   KEFFER
 * Initial revision.
 *
 */

#include "rw/tooldefs.h"
#include "rw/gvector.h"

#ifdef RW_BROKEN_TOKEN_PASTE
#  define RWGOrderedVector(val) val##RWGOrderedVector                    
#else
#  define RWGOrderedVector(val) name2(val,RWGOrderedVector)
#endif

#define RWGOrderedVectordeclare(val)						\
class RWGExport RWGOrderedVector(val) : private RWGVector(val) {			\
public:										\
  RWGOrderedVector(val)(size_t capac=RWDEFAULT_CAPACITY)			\
    : RWGVector(val)(capac), nitems_(0) {;}					\
  val&		operator()(size_t i)       {return array_[i];}			\
  val		operator()(size_t i) const {return array_[i];}			\
  val&		operator[](size_t i)       {boundsCheck(i); return array_[i];}	\
  val		operator[](size_t i) const {boundsCheck(i); return array_[i];}	\
  void		clear()                    {nitems_=0;}				\
  const val*	data() const               {return RWGVector(val)::data();}	\
  size_t	entries() const            {return nitems_;}			\
  size_t	index(val) const;						\
  void		insert(val item);						\
  void		insertAt(size_t indx, val item);				\
  RWBoolean	isEmpty() const            {return nitems_==0;}			\
  size_t	length() const             {return nitems_;}			\
  val		pop()                      {return array_[--nitems_];}		\
  void		push(val item)             {insert(item);}			\
  void		removeAt(size_t);						\
  void		resize(size_t N);						\
protected:									\
  void		boundsCheck(size_t) const;					\
  size_t	nitems_;							\
};

#define RWGOrderedVectorimplement(val)						\
size_t RWGOrderedVector(val)::index(val item) const				\
{										\
  for(size_t i=0; i<nitems_; i++) if (array_[i]==item) return i;		\
  return RW_NPOS;								\
}										\
										\
void RWGOrderedVector(val)::insert(val item)					\
{										\
  if(nitems_>=RWGVector(val)::length())		/* Check for overflow */	\
    resize(nitems_ + RWDEFAULT_RESIZE);						\
  array_[nitems_++] = item;							\
}										\
										\
void RWGOrderedVector(val)::insertAt(size_t ipt, val item)			\
{										\
  if(nitems_>=RWGVector(val)::length())		/* Check for overflow */	\
    resize(nitems_ + RWDEFAULT_RESIZE);						\
  										\
  for(size_t i=nitems_; i>ipt; i--)	/* Slide right (could be expensive) */	\
    array_[i] = array_[i-1];							\
										\
  nitems_++;									\
  array_[ipt] = item;								\
}										\
										\
void RWGOrderedVector(val)::removeAt(size_t ipt)				\
{										\
  /* Slide left (could be very expensive): */					\
  for(size_t i=ipt; i<nitems_-1; i++) array_[i] = array_[i+1];			\
  nitems_--;									\
}										\
										\
void RWGOrderedVector(val)::resize(size_t N)					\
{  if(N>nitems_) RWGVector(val)::reshape(N); }					\
										\
void RWGOrderedVector(val)::boundsCheck(size_t i) const				\
{  if (i>=nitems_)								\
     RWTHROW(RWBoundsErr(RWMessage(RWTOOL_INDEXERR,				\
				   (unsigned)i, (unsigned)nitems_)) );	\
}

#endif

