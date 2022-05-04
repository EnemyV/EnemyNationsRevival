#ifndef  __RWIDENSET_H__
#define  __RWIDENSET_H__

/*
 * Declarations for RWIdentitySet (i.e., an Identity Set).
 *
 * $Id: idenset.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: idenset.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  01:54:59  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:10:19  vriezen
 * Move all files to 6.1
 *
 * Revision 2.4  1994/03/04  01:04:16  jims
 * Override isEqual member function from RWCollectable to return
 * TRUE or FALSE based on operator==
 *
 * Revision 2.3  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.2  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.7   28 May 1992 16:46:28   KEFFER
 * Ported to Metaware High-C
 * 
 *    Rev 1.6   28 May 1992 15:36:02   KEFFER
 * Introduced RWhashAddress() for identity hashes
 * 
 *    Rev 1.5   25 May 1992 15:50:38   KEFFER
 * Optimized, reducing size.
 * 
 *    Rev 1.4   22 May 1992 17:04:14   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   29 Apr 1992 14:51:20   KEFFER
 * Hashing now uses chaining to resolve collisions
 * 
 *    Rev 1.2   18 Feb 1992 09:54:24   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:18   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:15:24   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

/*
 * For the storage and retrieval of items using the address of the item
 */

#include "rw/rwset.h"

/****************************************************************
 *								*
 *			RWIdentitySet				*
 *								*
 ****************************************************************/

class RWExport RWIdentitySet : public RWSet {

  RWDECLARE_COLLECTABLE(RWIdentitySet)

public:

  RWIdentitySet(size_t N = RWCollection::DEFAULT_CAPACITY);
  virtual ~RWIdentitySet();

  // Redefined methods use identity semantics instead of value semantics:
  virtual RWCollectable*	find(const RWCollectable*) const;
  virtual RWCollectable*	insert(RWCollectable*);
  virtual RWCollectable*	remove(const RWCollectable*);
  virtual RWBoolean		isEqual(const RWCollectable*) const;

protected:

  size_t			hashIndexReference(const RWCollectable* p) const;

};    

/****************************************************************
 *								*
 *			INLINES					*
 *								*
 ****************************************************************/

inline size_t
RWIdentitySet::hashIndexReference(const RWCollectable* a) const
{
  return RWhashAddress((void*)a) % buckets();
}

#endif /* __RWIDENSET_H__ */
