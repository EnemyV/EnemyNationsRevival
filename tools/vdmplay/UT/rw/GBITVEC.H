#ifndef __RWGBITVEC_H__
#define __RWGBITVEC_H__

/*
 * Generic bit vector.
 *
 * $Id: gbitvec.h,v 6.3 1994/07/14 21:04:32 jims Exp $
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
 * To use this class you must declare and implement a bit vector of the length
 * you want.
 *
 * Example: To declare and implement a bit vector of length 22:
 *
 *   #include <rw/gbitvec.h>
 *
 *   declare(RWGBitVec,22)
 *
 *   // In one and only one .cpp file you must put the following:
 *   implement(RWGBitVec,22)
 *
 *   // To use:
 *   void func(){
 *     RWGBitVec(22) bv;
 *     bv(0) = TRUE;
 *     bv(3) = FALSE;	// etc.
 *   }
 *
 ***************************************************************************
 *
 * $Log: gbitvec.h,v $
 * Revision 6.3  1994/07/14  21:04:32  jims
 * Use RWGExport for generic collection classes
 *
 * Revision 6.2  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:58  vriezen
 * Move all files to 6.1
 *
 * Revision 2.12  1994/04/01  21:33:30  vriezen
 * Use RW_INLINE_FRIEND symbol for inline friends
 *
 * Revision 2.11  1994/03/10  23:22:58  jims
 * Use "friend inline" or just "friend" as appropriate
 *
 * Revision 2.10  1994/02/21  21:51:14  jims
 * Port to DEC C++ under OSF/1 on AXP
 *
 * Revision 2.9  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.8  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.7  1993/03/25  03:50:41  keffer
 * Added prefix RW to class name
 *
 * Revision 2.6  1993/03/25  01:07:41  keffer
 * Corrected comments.
 *
 * Revision 2.5  1993/03/25  00:24:28  keffer
 * Corrected problem in operator==().
 * Made operator==() a friend function.
 *
 * Revision 2.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.3  1993/01/27  19:18:15  keffer
 * Now use RWor, RWand, and RWxor always.
 *
 * Revision 2.1  1992/11/28  00:14:22  myersn
 * moved inline friends out of class declarations
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   27 May 1992 12:13:34   KEFFER
 * Changed formal parameter of RWor, etc, to size_t
 * 
 *    Rev 1.3   18 Feb 1992 19:22:32   KEFFER
 * Now includes "rw/generic.h".
 * Class tag is now RWExport instead of huge.
 * 
 *    Rev 1.2   28 Oct 1991 09:08:14   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   09 Oct 1991 18:34:32   keffer
 * Ported to Zortech V3.0
 * 
 *    Rev 1.0   28 Jul 1991 08:11:18   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/bitref.h"
#include "rw/generic.h"
STARTWRAP
#include <assert.h>
#ifdef RW_NON_ANSI_HEADERS
#  include <memory.h>		/* Looking for memcpy() */
#else
#  include <string.h>
#endif
ENDWRAP


#define RWGBitVec(sz) name2(RWGBitVec,sz)

#define RWGBitVecdeclare(sz)							\
class RWGExport RWGBitVec(sz) {							\
  RWByte	vec_[ (sz+7) >> 3 ];						\
public:										\
  RWGBitVec(sz)() { memset(vec_,0,sizeof(vec_)); }				\
  RWGBitVec(sz)(RWBoolean f) { memset(vec_, f ? ~0 : 0,sizeof(vec_)); }		\
										\
  RWGBitVec(sz)&		operator=(const RWGBitVec(sz)& v)		\
    { memcpy(vec_,v.vec_,sizeof(vec_)); return *this; }				\
  RWGBitVec(sz)&		operator=(RWBoolean f)				\
    { memset(vec_,f ? ~0 : 0,sizeof(vec_)); return *this; }			\
  RWGBitVec(sz)&		operator&=(const RWGBitVec(sz)& v)		\
    { RWand(vec_,v.vec_,sizeof(vec_)); return *this; }				\
  RWGBitVec(sz)&		operator^=(const RWGBitVec(sz)& v)		\
    { RWxor(vec_,v.vec_,sizeof(vec_)); return *this; }				\
  RWGBitVec(sz)&		operator|=(const RWGBitVec(sz)& v)		\
    { RWor (vec_,v.vec_,sizeof(vec_)); return *this; }				\
										\
  RWBitRef		operator[](size_t i)					\
    { assert(i<sz); return RWBitRef(vec_,i); }					\
  RWBitRef		operator()(size_t i)					\
    { return RWBitRef(vec_,i); }						\
  friend RWBoolean	operator==(const RWGBitVec(sz)& v1,const RWGBitVec(sz)& v2);	\
  friend RWBoolean	operator!=(const RWGBitVec(sz)& v1,const RWGBitVec(sz)& v2)	\
    { return !(v1==v2); }								\
											\
  RW_INLINE_FRIEND RWGBitVec(sz) operator&(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2);	\
  RW_INLINE_FRIEND RWGBitVec(sz) operator^(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2);	\
  RW_INLINE_FRIEND RWGBitVec(sz) operator|(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2);	\
										\
  void			clearBit(size_t i)					\
    { vec_[i>>3] &= ~(1 << (7&i)); }						\
  const RWByte*		data() const						\
    {return vec_;}								\
  void			setBit(size_t i)					\
    { vec_[i>>3] |=  (1 << (7&i)); }						\
  RWBoolean		testBit(size_t i) const				\
    { return (  vec_[i>>3] & ( 1 << (7&i) )  ) != 0; }				\
};                                                                              \
inline RWGBitVec(sz)	operator&(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2)	\
    { RWGBitVec(sz) temp(v1); temp &= v2; return temp; }				\
inline RWGBitVec(sz)	operator^(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2)	\
    { RWGBitVec(sz) temp(v1); temp ^= v2; return temp; }				\
inline RWGBitVec(sz)	operator|(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2)	\
    { RWGBitVec(sz) temp(v1); temp |= v2; return temp; }


#define RWGBitVecimplement(sz)							\
RWBoolean operator==(const RWGBitVec(sz)& v1, const RWGBitVec(sz)& v2)		\
{										\
  size_t nfull = sz>>3;								\
  if (memcmp(v1.vec_, v2.vec_, nfull)) return FALSE;				\
  if (nfull==sizeof(v1.vec_)) return TRUE;					\
  RWByte mask = (1 << ((size_t)sz&7)) - 1;					\
  return (v1.vec_[nfull] & mask) == (v2.vec_[nfull] & mask);			\
}


#endif /* __RWGBITVEC_H__ */
