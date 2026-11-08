#ifndef __RWTVREF_H__
#define __RWTVREF_H__

/*
 * RWTVirtualRef<T>: A typed interface to RWVirtualRef
 *
 * $Id: tvref.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * Although this array could be used by the end user it is really intended
 * to be the reference part of a reference counted array.  It uses
 * pure copy semantics and hence is relatively slow.  
 *
 * It inherits from RWReference and hence contains a reference count.
 * This is used to implement copy-on-write reference counting as was
 * done with class RWTValVirtualArray<T>.
 *
 * Assumes that T has:
 *   - well-defined copy constructor (T::T(const T&) or equiv.);
 *   - well-defined assignment operator (T::operator=(const T&) or equiv.).
 *
 * Note that the location in physical memory of a particular index 
 * of the array can change from invocation to invocation.  Hence,
 * the address of an element should never be taken. 
 *
 ***************************************************************************
 *
 * $Log: tvref.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:47:57  vriezen
 * Move all files to 6.1
 *
 * Revision 2.5  1993/11/08  20:32:10  jims
 * Port to ObjectStore
 *
 * Revision 2.4  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.3  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.2  1993/01/29  20:46:00  keffer
 * *** empty log message ***
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.2   09 Sep 1992 12:51:42   KEFFER
 * RWTVirtualRef<T> is now marked with "RWExport" for use in Windows.
 * 
 *    Rev 1.1   17 Mar 1992 19:41:40   KEFFER
 * 
 *    Rev 1.0   11 Mar 1992 14:10:00   KEFFER
 * Initial revision.
 */

#include "rw/vref.h"
#include "rw/toolerr.h"

template <class T> class RWTExport RWTVirtualRef : public RWVirtualRef
{

public:

  RWTVirtualRef(long size, RWVirtualPageHeap* h);
  RWTVirtualRef(RWTVirtualRef<T>&);		// Takes deep copy.

  T			val(long i);
  void			set(long i, const T& v);
  void			set(long start, long extent, const T& v);
  virtual void		conformalCopy(long start1, RWVirtualRef& v2, long start2, long N);
};


#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvref.cc"
#endif

#endif	/* __RWTVREF_H__ */
