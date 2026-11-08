#ifndef __RWTBITVEC_H__
#define __RWTBITVEC_H__

/*
 * RWTBitVec<N>: Parameterized bit vector of length N
 *
 * $Id: tbitvec.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: tbitvec.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:11:07  vriezen
 * Move all files to 6.1
 *
 * Revision 2.15  1994/01/04  02:27:39  jims
 * ObjectStore version: add #include for <ostore/ostore.hh>
 *
 * Revision 2.14  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.13  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.12  1993/07/31  01:37:31  keffer
 * Added member functions firstTrue() and firstFalse().
 *
 * Revision 2.11  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.10  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.9  1993/02/12  20:19:03  keffer
 * Ported to g++ v2.3
 *
 * Revision 2.8  1993/02/12  00:23:42  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.7  1993/02/06  22:13:47  keffer
 * Ported to cfront V3.0
 *
 * Revision 2.6  1993/01/27  19:18:15  keffer
 * Now use RWor, RWand, and RWxor always.
 *
 * Revision 2.5  1993/01/16  02:38:44  keffer
 * Changed to new <rw/compiler.h> macros.
 *
 *    Rev 1.1   27 May 1992 12:13:34   KEFFER
 * Changed formal parameter of RWor, etc, to size_t
 * 
 *    Rev 1.0   02 Mar 1992 16:10:50   KEFFER
 * Initial revision.
 */

/*
 * Assumes 8 bits per byte.
 *
 * To use:
 *
 *   main() {
 *     RWTBitVec<17> vc;	// Defines a bit vector 17 bits long
 *     vc(3) = TRUE;		// Set bit 3 to TRUE
 *     // ... etc.
 *   }
 *
 */

#include "rw/bitref.h"

STARTWRAP
#include <assert.h>
#ifdef RW_NON_ANSI_HEADERS
#  include <memory.h>		/* Looking for memcpy() */
#else
#  include <string.h>
#endif
ENDWRAP


template <int N> class RWTExport RWTBitVec
{

public:

  RWTBitVec()		{ memset(vec_, 0, sizeof(vec_)); }
  RWTBitVec(RWBoolean f){ memset(vec_, f ? ~0 : 0, sizeof(vec_)); }

  RWTBitVec<N>&		operator=(const RWTBitVec<N>& v)
    { memcpy(vec_,v.vec_,sizeof(vec_)); return *this; }
  RWTBitVec<N>&		operator=(RWBoolean f)
    { memset(vec_,f ? ~0 : 0,sizeof(vec_)); return *this; }
  RWTBitVec<N>&		operator&=(const RWTBitVec<N>& v)
    { RWand(vec_,v.vec_,sizeof(vec_)); return *this; }
  RWTBitVec<N>&		operator^=(const RWTBitVec<N>& v)
    { RWxor(vec_,v.vec_,sizeof(vec_)); return *this; }
  RWTBitVec<N>&		operator|=(const RWTBitVec<N>& v)
    { RWor (vec_,v.vec_,sizeof(vec_)); return *this; }

  RWBitRef		operator[](size_t i)
    { assert(i<(size_t)N); return RWBitRef(vec_,i); }
  RWBitRef		operator()(size_t i)
    { return RWBitRef(vec_,i); }
  RWBoolean		operator==(RWBoolean b) const;
  RWBoolean		operator!=(RWBoolean b) const
    { return !operator==(b); }
  RWBoolean		operator==(const RWTBitVec<N>& v) const;
  RWBoolean		operator!=(const RWTBitVec<N>& v) const
    { return !operator==(v); }

   RWTBitVec<N>	operator&(const RWTBitVec<N>& v2)
    { RWTBitVec<N> temp(*this); temp &= v2; return temp; }
   RWTBitVec<N>	operator^(const RWTBitVec<N>& v2)
    { RWTBitVec<N> temp(*this); temp ^= v2; return temp; }
   RWTBitVec<N>	operator|(const RWTBitVec<N>& v2)
    { RWTBitVec<N> temp(*this); temp |= v2; return temp; }

  void			clearBit(size_t i)
    { vec_[i>>3] &= ~(1 << (7&i)); }
  const RWByte*		data() const
    {return vec_;}
  void			setBit(size_t i)
    { vec_[i>>3] |=  (1 << (7&i)); }
  RWBoolean		testBit(size_t i) const
    { return (  vec_[i>>3] & ( 1 << (7&i) )  ) != 0; }

  size_t		firstTrue() const
    { return rwFirstTrue(vec_, (size_t)N); }
  size_t		firstFalse() const
    { return rwFirstFalse(vec_, (size_t)N); }

private:

  RWByte		vec_[ (N+7) >> 3 ];

#if !defined(RW_BROKEN_TEMPLATES) && !defined(__GNUC__)
  static const size_t	nfull_;
  static const RWByte	mask_;
#endif

};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tbitvec.cc"
#endif

#endif /* __RWTBITVEC_H__ */
