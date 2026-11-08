#ifndef __RWGVECTOR_H__
#define __RWGVECTOR_H__

/*
 * <generic.h> style macro for a generic vector.
 *
 * $Id: gvector.h,v 6.3 1994/07/14 21:04:32 jims Exp $
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
 * $Log: gvector.h,v $
 * Revision 6.3  1994/07/14  21:04:32  jims
 * Use RWGExport for generic collection classes
 *
 * Revision 6.2  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:12  vriezen
 * Move all files to 6.1
 *
 * Revision 2.17  1994/03/02  19:19:33  jims
 * New message INDEXERR shows invalid index and number of
 * items in collection
 *
 * Revision 2.16  1993/11/09  04:05:15  jims
 * Port to ObjectStore
 *
 * Revision 2.15  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.14  1993/08/03  21:23:30  dealys
 * Ported to MPW C++ 3.3 - RW_BROKEN_TOKEN_PASTE
 *
 * Revision 2.13  1993/08/02  18:07:52  jims
 * Still cannot initialize array_ to new val[n] using initializer list
 * syntax Microsoft
 *
 * Revision 2.12  1993/05/31  21:45:26  keffer
 * New messaging architecture for localization
 *
 * Revision 2.11  1993/05/18  00:41:46  keffer
 * Now uses new exception handling facility
 *
 * Revision 2.10  1993/04/12  16:07:49  keffer
 * Now uses RW error facility instead of "generic" error facility.
 *
 * Revision 2.9  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.8  1993/03/25  03:50:41  keffer
 * Added prefix RW to class name
 *
 * Revision 2.7  1993/03/24  03:44:27  keffer
 * Removed unnecessary checks for an unsigned less than zero.
 *
 * Revision 2.6  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.5  1993/02/09  17:59:59  keffer
 * Cannot initialize array_ using initializer list syntax
 * with Microsoft C7.
 *
 * Revision 2.4  1993/02/07  17:28:40  keffer
 * Now allows assignment to a value.
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
 *    Rev 1.8   30 May 1992 12:14:04   KEFFER
 * Introduced RWVECTOR_DELETE macro to rationalize V2.0 vs V2.1 deletes.
 * 
 *    Rev 1.7   28 May 1992 13:34:46   KEFFER
 * 
 *    Rev 1.6   14 May 1992 11:53:20   KEFFER
 * Changed name of RWGVector::resize() to reshape().
 * 
 *    Rev 1.4   18 Mar 1992 11:27:52   KEFFER
 * Implementation is now V2.1 aware.
 * 
 *    Rev 1.3   18 Feb 1992 19:23:04   KEFFER
 * Now includes "rw/generic.h".
 * Class tag is now RWExport instead of huge.
 * 
 *    Rev 1.2   28 Oct 1991 09:08:16   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   09 Oct 1991 18:34:38   keffer
 * Ported to Zortech V3.0
 * 
 *    Rev 1.0   28 Jul 1991 08:11:24   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/generic.h"
#include "rw/rwerr.h"
#include "rw/toolerr.h"

#ifdef RW_BROKEN_TOKEN_PASTE
#  define RWGVector(val) val##RWGVector
#else
#  define RWGVector(val) name2(val,RWGVector)
#endif

/************************************************
 ************************************************
 *						*
 *		RWGVector declaration		*
 *						*
 ************************************************
 ************************************************/

#ifdef RW_NO_CONST_OVERLOAD

#define RWGVectordeclare(val)							\
class RWExport RWGVector(val) {							\
public:										\
  RWGVector(val)()         : npts_(0), array_(0)          {;}			\
  RWGVector(val)(size_t n) : npts_(n) { array_ = new val[n]; }			\
  RWGVector(val)(size_t n, val v);						\
  ~RWGVector(val)()	{ RWVECTOR_DELETE(npts_) array_; }			\
  RWGVector(val)(const RWGVector(val)&);					\
  RWGVector(val)&		operator=(const RWGVector(val)&);		\
  RWGVector(val)&		operator=(val);					\
  const val*	data() const		{return array_;}			\
  size_t	length() const		{return npts_;}				\
  void		resize(size_t N)	{reshape(N);}				\
  void		reshape(size_t);						\
  val&		operator()(size_t i)	{return array_[i];}			\
  val&		operator[](size_t i)	{boundsCheck(i); return array_[i];}	\
protected:									\
  void		boundsCheck(size_t i) const;					\
  size_t	npts_;								\
  val*		array_;								\
};

#else /* !RW_NO_CONST_OVERLOAD */


#define RWGVectordeclare(val)						\
class RWGExport RWGVector(val) {					\
public:									\
  RWGVector(val)()         : npts_(0), array_(0)   {;}			\
  RWGVector(val)(size_t n) : npts_(n) { array_ = new val[n]; }		\
  RWGVector(val)(size_t n, val v);					\
  ~RWGVector(val)()	{ RWVECTOR_DELETE(npts_) array_; }		\
  RWGVector(val)(const RWGVector(val)&);				\
  RWGVector(val)&		operator=(const RWGVector(val)&);	\
  RWGVector(val)&		operator=(val);				\
  val&	operator()(size_t i)       {return array_[i];}		        \
  val	operator()(size_t i) const {return array_[i];}		        \
  val&	operator[](size_t i)	   {boundsCheck(i); return array_[i];}  \
  val	operator[](size_t i) const {boundsCheck(i); return array_[i];}	\
  const val*	data() const               {return array_;}		\
  size_t	length() const             {return npts_; }		\
  void		resize(size_t N)           {reshape(N);   }		\
  void		reshape(size_t);					\
protected:								\
  void		boundsCheck(size_t i) const;				\
  size_t	npts_;							\
  val*		array_;							\
};


#endif /* RW_NO_CONST_OVERLOAD */


/************************************************
 ************************************************
 *						*
 *	RWGVector implementation		*
 *						*
 ************************************************
 ************************************************/


#define RWGVectorimplement(val)						\
RWGVector(val)::RWGVector(val)(size_t n, val ival)			\
{									\
  register size_t i = npts_ = n;					\
  register val* dst = array_ = new val[i];				\
  while(i--) *dst++ = ival;						\
}									\
									\
RWGVector(val)::RWGVector(val)(const RWGVector(val)& a)			\
{									\
  register size_t i= npts_ = a.npts_;					\
  register val* dst = array_ = new val[i];				\
  register val* src = a.array_;						\
  while (i--) *dst++ = *src++;						\
}									\
									\
RWGVector(val)& RWGVector(val)::operator=(const RWGVector(val)& a)	\
{									\
  if(this != &a){							\
    RWVECTOR_DELETE(npts_) array_;	/* Disconnect from old array */	\
    register size_t i = npts_ = a.npts_;				\
    register val* dst = array_ = new val[i];				\
    register val* src = a.array_;					\
    while (i--) *dst++ = *src++;					\
  }									\
  return *this;								\
}									\
									\
RWGVector(val)& RWGVector(val)::operator=(val v)			\
{									\
  register size_t i = npts_;						\
  while (i--) array_[i] = v;						\
  return *this;								\
}									\
									\
void RWGVector(val)::reshape(size_t N)					\
{									\
  if(N==npts_)return;							\
  val* newArray = new val[N];						\
  register size_t i = (N<=npts_) ? N:npts_;				\
  register val* src = array_;						\
  register val* dst = newArray;						\
  while (i--) *dst++ = *src++;						\
  RWVECTOR_DELETE(npts_) array_;					\
  array_ = newArray;							\
  npts_ = N;								\
}									\
									\
void RWGVector(val)::boundsCheck(size_t i) const			\
{ if (i>=npts_)								\
    RWTHROW(RWBoundsErr(RWMessage(RWTOOL_INDEXERR,			\
				  (unsigned)i, (unsigned)npts_) ) );	\
}


#endif /* __RWGVECTOR_H__ */
