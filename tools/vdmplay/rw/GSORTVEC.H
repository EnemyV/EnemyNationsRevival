#ifndef __RWGSORTVEC_H__
#define __RWGSORTVEC_H__

/*
 * <generic.h> style sorted vector --- uses insertion sort
 *
 * $Id: gsortvec.h,v 6.3 1994/07/14 21:04:32 jims Exp $
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
 * Declares a generic sorted vector.  This means that an item is inserted
 * so as to be after items "less than" itself, and before items 
 * "greater than" itself.  
 *
 * To use this class you must declare and implement the 
 * base class RWGVector(val), as well as the sorted vector itself.
 *
 *   #include <rw/gsortvec.h>
 *   declare(RWGVector,int)		// Declare the base class
 *   declare(RWGSortedVector,int)		// Declares a sorted vector of ints
 *
 *   // In one and only one .cpp file you must put the following:
 *   implement(RWGVector,int)		// Implement the base class
 *   implement(RWGSortedVector,int)	// Implement the sorted vector
 *
 * An instance of this vector could be used as follows:
 *
 *   int compFun(const int* a, const int* b)	// Define a comparison function
 *	{ return *a - *b; }
 *   RWGSortedVector(compFun, int) avec;		// Create a sorted vector
 *   avec.insert(3);				// Insert an item
 *
 ***************************************************************************
 *
 * $Log: gsortvec.h,v $
 * Revision 6.3  1994/07/14  21:04:32  jims
 * Use RWGExport for generic collection classes
 *
 * Revision 6.2  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:08  vriezen
 * Move all files to 6.1
 *
 * Revision 2.12  1994/03/02  19:19:33  jims
 * New message INDEXERR shows invalid index and number of
 * items in collection
 *
 * Revision 2.11  1993/09/16  04:18:06  keffer
 * Changed function prototype for RWbsearch to satisfy CSet++
 *
 * Revision 2.10  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.9  1993/08/03  21:17:57  dealys
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
 *    Rev 1.5   14 May 1992 11:53:06   KEFFER
 * Changed name of RWGVector::resize() to reshape().
 * 
 *    Rev 1.3   04 Mar 1992 18:57:22   KEFFER
 * Now requires the user to declare and implement the base class RWGVector(val)
 * 
 *    Rev 1.2   18 Feb 1992 19:23:02   KEFFER
 * Now includes "rw/generic.h".
 * Class tag is now RWExport instead of huge.
 * 
 *    Rev 1.1   28 Oct 1991 09:08:16   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:11:22   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/tooldefs.h"
#include "rw/gvector.h"

#ifdef RW_BROKEN_TOKEN_PASTE
#  define RWGSortedVector(val) val##RWGSortedVector 
#else
#  define RWGSortedVector(val) name2(val,RWGSortedVector)
#endif

#define RWGSortedVectordeclare(val)						\
class RWGExport RWGSortedVector(val) : public RWGVector(val) {			\
public:										\
  RWGSortedVector(val)(int(*f)(const val*,const val*))				\
  : RWGVector(val)(RWDEFAULT_CAPACITY), nitems_(0), cf(f) {;}			\
  RWGSortedVector(val)(int(*f)(const val*,const val*),size_t capac)		\
  : RWGVector(val)(capac),              nitems_(0), cf(f) {;}			\
  val		operator()(size_t i) const {return array_[i];}			\
  val		operator[](size_t i) const {boundsCheck(i); return array_[i];}	\
  size_t	entries() const            {return nitems_;}			\
  size_t	index(val);							\
  RWBoolean	insert(val);							\
  size_t	length() const {return nitems_;}				\
  void		removeAt(size_t);						\
  void		resize(size_t newCapacity);					\
protected:									\
  void		boundsCheck(size_t) const;					\
  size_t	nitems_; /* Number of slots in use */				\
  int		(*cf)(const val*, const val*); /* Comparison function */	\
};

#define RWGSortedVectorimplement(val)						\
size_t RWGSortedVector(val)::index(val item)					\
{										\
  size_t idx;									\
  if(!RWbsearch(&item, array_, nitems_, sizeof(val), (RWcompare)cf, idx))	\
    return RW_NPOS;								\
  while (idx && (*cf)(array_+idx-1, &item)==0) --idx;				\
  return idx;									\
}										\
										\
RWBoolean RWGSortedVector(val)::insert(val item)				\
{										\
  size_t idx;									\
  if(RWbsearch(&item, array_, nitems_, sizeof(val), (RWcompare)cf, idx))	\
    while (idx<nitems_ && (*cf)(array_+idx, &item) == 0) ++idx;			\
  else										\
    while (idx<nitems_ && (*cf)(array_+idx, &item) <  0) ++idx;			\
										\
  if( nitems_==npts_ ) resize(npts_+RWDEFAULT_RESIZE);				\
  for(size_t j=nitems_; j>idx; j--) array_[j] = array_[j-1]; /* Slide right */	\
  array_[idx] = item;								\
  nitems_++;									\
  return TRUE;									\
}										\
										\
void RWGSortedVector(val)::removeAt(size_t ipt)					\
{										\
  /* Slide left (could be very expensive): */					\
  for(register i=ipt; i<nitems_-1; i++) array_[i] = array_[i+1];		\
  nitems_--;									\
}										\
										\
void RWGSortedVector(val)::resize(size_t N)					\
{  if(N>nitems_) RWGVector(val)::reshape(N); }					\
										\
void RWGSortedVector(val)::boundsCheck(size_t i) const				\
{  if (i>=nitems_)								\
     RWTHROW(RWBoundsErr(RWMessage(RWTOOL_INDEXERR,				\
				   (unsigned)i, (unsigned)nitems_) ));	\
}

#endif

