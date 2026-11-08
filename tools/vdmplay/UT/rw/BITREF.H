#ifndef	__RWBITREF_H__
#define	__RWBITREF_H__

/*
 * A reference to a bit in a bit vector.
 *
 * $Id: bitref.h,v 6.2 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: bitref.h,v $
 * Revision 6.2  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:25  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1993/12/29  00:06:54  keffer
 * Tightened up type of constants, to avoid compiler warnings.
 *
 * Revision 2.10  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.9  1993/07/31  01:37:52  keffer
 * Added declarations for rwFirstTrue() and rwFirstFalse().
 *
 * Revision 2.8  1993/07/19  20:45:26  keffer
 * friend classes now use elaborated-type-specifier (ARM Sec. 11.4)
 *
 * Revision 2.7  1993/07/07  00:26:04  keffer
 * Copy constructor is now public to accommodate RWGBitVec(sz).
 *
 * Revision 2.6  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/02/08  18:19:23  keffer
 * RWor, RWand, and RWxor now exported from DLL.
 *
 * Revision 2.3  1993/01/28  01:53:04  myersn
 * derive from RWMemoryPool via the macro RWMemoryPool_OPTION for MT-safety
 *
 * Revision 2.2  1993/01/27  19:18:15  keffer
 * Now use RWor, RWand, and RWxor always.
 *
 *
 */

/*
 * Although this class has two public constructors, it is not intended
 * to be used by itself, but rather as an lvalue to the BitVec 
 * and GBitVec(sz) classes.  Do not "construct" a RWBitRef directly.
 * The constructor must be public to accomodate GBitVec(sz) whose name
 * is not known in advance, hence it can't be made a friendly class.
 */

#include "rw/tooldefs.h"
#include "rw/mempool.h"

class RWExport RWBitVec;

class RWExport RWBitRef   RWMemoryPool_OPTION {
public:

  RWBitRef(RWByte* p, size_t j)
    { pt_ = (j>>3) + p;   msk_ = ((RWByte)1) << (7&j); }

  RWBitRef(const RWBitRef& br)
    { pt_=br.pt_; msk_=br.msk_; }

  operator	RWBoolean() const
    { return ((*pt_ & msk_) != 0); }

  RWBoolean	operator=(RWBoolean i)
    { if(i) *pt_ |= msk_;
    else    *pt_ &= ~msk_;
    return i; }

  void		operator&=(RWBoolean i) {if (!i) *pt_ &= ~msk_;}
  void		operator|=(RWBoolean i) {if  (i) *pt_ |=  msk_;}
  void		operator^=(RWBoolean i) {if  (i) *pt_ ^=  msk_;}

private:

  RWByte*	pt_;	// Pointer to the referenced byte
  RWByte	msk_;	// Mask for the bit within the byte

friend class RWExport RWBitVec;
};

// Useful declarations:
extern void rwexport RWor  (RWByte*, const RWByte*, size_t);
extern void rwexport RWxor (RWByte*, const RWByte*, size_t);
extern void rwexport RWand (RWByte*, const RWByte*, size_t);
extern size_t rwexport rwFirstTrue (const RWByte*, size_t);
extern size_t rwexport rwFirstFalse(const RWByte*, size_t);

#endif /* __RWBITREF_H__ */
