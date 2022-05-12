#ifndef __RWPVECTOR_H__
#define __RWPVECTOR_H__

/*
 * RWPtrVector: Vector of pointers
 *
 * $Id: pvector.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: pvector.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:36  vriezen
 * Move all files to 6.1
 *
 * Revision 1.9  1993/11/04  13:15:09  jims
 * Add ppp version header
 *
 * Revision 1.8  1993/11/04  12:44:22  jims
 * Port to ObjectStore
 *
 * Revision 1.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.6  1993/05/16  21:14:00  keffer
 * Header file "rw/toolerr.h" no longer included.
 *
 * Revision 1.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.3  1993/03/15  02:28:02  keffer
 * Corrected error in RWVECTOR_DELETE
 *
 * Revision 1.2  1993/03/14  20:47:26  keffer
 * Removed m.f. ref(); return type of data() is now "void* const*".
 *
 * Revision 1.1  1993/02/17  18:27:09  keffer
 * Initial revision
 *
 */


#ifndef __RWTOOLDEFS_H__
#  include "rw/tooldefs.h"
#endif

class RWExport RWPtrVector
{

public:

  RWPtrVector()           : npts_(0), array_(0)        {;}
  RWPtrVector(size_t n) : npts_(n), array_(new RWvoid[n]) {;}
  RWPtrVector(size_t n, void* ival);
  RWPtrVector(const RWPtrVector&);
  ~RWPtrVector()		{ RWVECTOR_DELETE(npts_) array_; }

  RWPtrVector&	operator=(const RWPtrVector&);
  RWPtrVector&	operator=(void* p);               // All elements initialized to p
  void* const*	data() const     {return array_;} // Cannot change vector elements
  size_t	length() const   {return npts_;}
  void		reshape(size_t);
  void		resize(size_t);
  void*&	operator()(size_t i){
#ifdef RWBOUNDS_CHECK
    boundsCheck(i);
#endif
    return array_[i]; }
  void*			operator()(size_t i) const {
#ifdef RWBOUNDS_CHECK
    boundsCheck(i);
#endif
    return array_[i]; }
  void*&		operator[](size_t i)       { boundsCheck(i); return array_[i];}
  void*			operator[](size_t i) const { boundsCheck(i); return array_[i];}

protected:

  void			boundsCheck(size_t i) const;

  size_t		npts_;
  RWvoid*		array_;

};

#endif /* __RWPVECTOR_H__ */
