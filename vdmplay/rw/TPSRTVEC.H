#ifndef __RWTPSRTVEC_H__
#define __RWTPSRTVEC_H__

/*
 * Sorted pointer-based vector.  Inserts pointers to values using an insertion sort.
 *
 * $Id: tpsrtvec.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * Stores a pointer to the inserted item into the collection according
 * to an ordering determined by the less-than (<) operator.
 *
 * Assumes that TP has:
 *   - well-defined equality semantics (TP::operator==(const TP&));
 *   - well-defined less-than semantics (TP::operator<(const TP&)).
 *
 * This class uses binary searches for efficient value-based retrievals.
 *
 ***************************************************************************
 *
 * $Log: tpsrtvec.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:04  vriezen
 * Move all files to 6.1
 *
 * Revision 2.9  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 2.8  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.6  1993/06/06  01:29:31  keffer
 * Added RWExport keyword to class declaration
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.3  1993/02/12  01:25:07  keffer
 * Changed name of definition file.
 *
 * Revision 2.2  1993/01/29  03:07:03  keffer
 * Increased code reuse between ordered and sorted vectors
 *
 *    Rev 1.0   25 May 1992 15:59:24   KEFFER
 * Initial revision.
 *
 */

#include "rw/tpordvec.h"

template <class TP> class RWTExport RWTPtrSortedVector : public RWTPtrOrderedVector<TP>
{

public:

  //Constructors
  RWTPtrSortedVector
  (
    size_t capac = RWDEFAULT_CAPACITY
  ) :  RWTPtrOrderedVector<TP>(capac) {;}

  RWTPtrSortedVector
  (
    const RWTPtrSortedVector<TP>& c
  ) : RWTPtrOrderedVector<TP>(c) { }

  // Overridden virtual functions
  virtual size_t	index(const TP* p) const;
  virtual void		insert(TP* p);
  virtual size_t	occurrencesOf(const TP* p) const;
  virtual TP*		remove(const TP* p);
  virtual size_t	removeAll(const TP* p);

protected:

  RWBoolean		bsearch(const TP*, size_t&) const; // binary search routine
  size_t		indexSpan(const TP*, size_t&) const;
#ifdef RWDEBUG
  RWBoolean		isSorted() const;
#endif

};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tpsrtvec.cc"
#endif

#endif	/* __RWTPSRTVEC_H__ */


