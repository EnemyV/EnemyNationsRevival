/*
 * Template definitions for RWTBitVec<N>
 *
 * $Id: tbitvec.cc,v 6.2 1994/07/12 20:04:35 vriezen Exp $
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
 * $Log: tbitvec.cc,v $
 * Revision 6.2  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.1  1994/04/15  19:11:06  vriezen
 * Move all files to 6.1
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 1.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 1.3  1993/02/12  20:19:03  keffer
 * Ported to g++ v2.3
 *
 * Revision 1.2  1993/02/06  22:13:47  keffer
 * Ported to cfront V3.0
 *
 * Revision 1.1  1993/01/27  21:38:32  keffer
 * Initial revision
 *
 *
 ***************************************************************************
 */

// Neither cfront v3.0 nor Gnu g++ can handle template static data;
// Borland and Metaware can.

#if defined(RW_BROKEN_TEMPLATES) || defined(__GNUC__)
# define nfull_ ((size_t)N>>3)
# define mask_ ((RWByte)((1 << ((size_t)N&7)) - 1))
#else
  template <int N> const size_t RWTBitVec<N>::nfull_ = (size_t)N >> 3;
  template <int N> const RWByte   RWTBitVec<N>::mask_  = (1 << ((size_t)N&7)) - 1;
#endif

template <int N> RWBoolean
RWTBitVec<N>::operator==(RWBoolean b) const
{
  register RWByte val = b ? ~0 : 0;

  // Check the full bytes:
  for (register size_t i=0; i<nfull_; i++) if (vec_[i] != val) return FALSE;
  
  // Check the last (partially full) byte, if any:
  return (nfull_==sizeof(vec_)) ? TRUE : (val & mask_) == (vec_[nfull_] & mask_);
}

template <int N> RWBoolean
RWTBitVec<N>::operator==(const RWTBitVec<N>& u) const
{
  // Check the full bytes:
  for (register size_t i=0; i<nfull_; i++)
    if (vec_[i] != u.vec_[i])
      return FALSE;

  // Check the last (partially full) byte, if any:
  return (nfull_==sizeof(vec_)) ? TRUE : (vec_[nfull_] & mask_) == (u.vec_[nfull_] & mask_);
}

#if defined(RW_BROKEN_TEMPLATES) || defined(__GNUC__)
# undef nfull_
# undef mask_
#endif
