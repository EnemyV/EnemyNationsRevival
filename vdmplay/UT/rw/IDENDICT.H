#ifndef  __RWIDENDICT_H__
#define  __RWIDENDICT_H__

/*
 * Declarations for RWIdentityDictionary (Hashed Identity Dictionary)
 *
 * $Id: idendict.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: idendict.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  01:59:54  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:10:18  vriezen
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
 *    Rev 1.5   25 May 1992 15:50:38   KEFFER
 * Optimized, reducing size.
 * 
 *    Rev 1.4   22 May 1992 17:04:12   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   29 Apr 1992 14:51:18   KEFFER
 * Hashing now uses chaining to resolve collisions
 * 
 *    Rev 1.1   28 Oct 1991 09:08:18   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:15:20   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/hashdict.h"

/****************************************************************
 *								*
 *			RWIdentityDictionary			*
 *								*
 ****************************************************************/

/*
 * For the storage and retrieval of (key, value) pairs, using the identity
 * (i.e., the address) of the key.
 */

class RWExport RWIdentityDictionary : public RWHashDictionary {

  RWDECLARE_COLLECTABLE(RWIdentityDictionary)

public:

  RWIdentityDictionary(size_t N = RWCollection::DEFAULT_CAPACITY);
  virtual ~RWIdentityDictionary();

  virtual RWBoolean		isEqual(const RWCollectable*) const;

protected:

  virtual RWCollectableAssociation*
  				findAssociation(const RWCollectable* key) const;
  virtual RWCollectableAssociation*	
  				newAssociation(RWCollectable*, RWCollectable*) const;
  virtual RWCollectableAssociation*
  				removeAssociation(const RWCollectable* key);

};

#endif /* __RWIDENDICT_H__ */
