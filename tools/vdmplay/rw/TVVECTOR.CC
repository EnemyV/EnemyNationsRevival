
/*
 * Template definitions for RWTValVector<T>
 *
 * $Id: tvvector.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tvvector.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:48:44  vriezen
 * Move all files to 6.1
 *
 * Revision 1.7  1993/11/08  21:40:06  jims
 * Port to ObjectStore
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/06/18  23:34:44  keffer
 * New messaging architecture for localization
 *
 * Revision 1.4  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.3  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.2  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 1.1  1993/01/28  02:02:26  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

#include "rw/toolerr.h"
#include "rw/rwerr.h"

template<class T>
RWTValVector<T>::RWTValVector(register size_t n, const T& ival)
{
  register T* dst = array_ = new T[npts_=n];
  while (n--) *dst++ = ival;
}

template<class T>
RWTValVector<T>::RWTValVector(const RWTValVector<T>& a)
{
  register size_t i= npts_ = a.npts_;
  register T* dst = array_ = new T[i];
  register T* src = a.array_;
  while (i--) *dst++ = *src++;
}

template<class T> RWTValVector<T>&
RWTValVector<T>::operator=(const RWTValVector<T>& a)
{
  if(array_ != a.array_){
    RWVECTOR_DELETE(npts_) array_; /* Disconnect from old array_ */
    register size_t i = npts_ = a.npts_;
    register T* dst = array_ = new T[i];
    register T* src = a.array_;
    while (i--) *dst++ = *src++;
  }
  return *this;
}
										
template<class T> RWTValVector<T>&
RWTValVector<T>::operator=(const T& val)
{
  for (register size_t i=0; i<npts_; i++)
    array_[i] = val;
  return *this;
}

template<class T> void
RWTValVector<T>::reshape(size_t N)
{
  if (N==npts_) return;
  T* newArray = new T[N];
  register size_t i = (N<=npts_) ? N:npts_;
  register T* src = array_;
  register T* dst = newArray;
  while (i--) *dst++ = *src++;
  RWVECTOR_DELETE(npts_) array_;
  array_ = newArray;
  npts_ = N;
}

template<class T> void
RWTValVector<T>::boundsCheck(size_t i) const
{
  if (i>=npts_)
    RWTHROW( RWBoundsErr(RWMessage(RWTOOL_INDEX, (unsigned)i, (unsigned)npts_) ));
}
