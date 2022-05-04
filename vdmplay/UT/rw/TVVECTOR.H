#ifndef __RWTVVECTOR_H__
#define __RWTVVECTOR_H__

/*
 * RWTValVector: Parameterized vectors of values
 *
 * $Id: tvvector.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * Stores a *copy* of the inserted item into the collection.
 *
 * Assumes that T has:
 *   - well-defined copy semantics (T::T(const T&) or equiv.);
 *   - well-defined assignment semantics (T::operator=(const T&) or equiv.);
 *
 * Note that while these are automatically defined for builtin types
 * (such as "int", "double", or any pointer), you may need to provide
 * appropriate operators for your own classes, particularly those with
 * constructors and/or pointers to other objects.
 *
 ***************************************************************************
 *
 * $Log: tvvector.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:45  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.10  1993/11/16  03:19:22  jims
 * Add RWExport modifier
 *
 * Revision 2.9  1993/11/08  21:40:06  jims
 * Port to ObjectStore
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/06/18  23:33:59  keffer
 * Removed unneeded #include "toolerr.h"
 *
 * Revision 2.6  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.5  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.4  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.3  1993/02/12  00:35:29  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.2  1993/01/28  02:01:20  keffer
 * Put definitions in a separate tvvector.cc
 *
 *    Rev 1.4   04 Sep 1992 12:59:58   KEFFER
 * Added comment about requiring less-than semantics
 * 
 *    Rev 1.3   17 Mar 1992 19:21:10   KEFFER
 * Changed BOUNDS_CHECK to RWBOUNDS_CHECK
 * 
 *    Rev 1.0   02 Mar 1992 16:10:56   KEFFER
 * Initial revision.
 */

#ifndef __RWTOOLDEFS_H__
#  include "rw/tooldefs.h"
#endif

template <class T> class RWTExport RWTValVector
{

public:

  RWTValVector()           : npts_(0), array_(0)        {;}
  RWTValVector(size_t n) : npts_(n), array_(new T[n]) {;}
  RWTValVector(size_t n, const T& ival);
  RWTValVector(const RWTValVector<T> &);
  ~RWTValVector()		{ RWVECTOR_DELETE(npts_) array_; }

  RWTValVector<T> &	operator=(const RWTValVector<T> &);
  RWTValVector<T> &	operator=(const T& ival);
  const T*		data() const     {return array_;}
  size_t		length() const   {return npts_;}
  const T&		ref(size_t i) const {return array_[i];}
  void			reshape(size_t);
  T&			operator()(size_t i){
#ifdef RWBOUNDS_CHECK
    boundsCheck(i);
#endif
    return array_[i]; }
  T			operator()(size_t i) const {
#ifdef RWBOUNDS_CHECK
    boundsCheck(i);
#endif
    return array_[i]; }
  T&			operator[](size_t i)	{ boundsCheck(i); return array_[i];}
  T			operator[](size_t i) const	{ boundsCheck(i); return array_[i];}

protected:

  void			boundsCheck(size_t i) const;

  size_t		npts_;
  T*			array_;

};


#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvvector.cc"
#endif

#endif /* __RWTVVECTOR_H__ */
