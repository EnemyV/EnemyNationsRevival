#ifndef __RWBTRDICT_H__
#define __RWBTRDICT_H__

/*
 * RWBTreeDictionary --- Dictionary implemented as an in memory B-Tree
 *
 * $Id: btrdict.h,v 6.2 1994/07/12 18:45:56 vriezen Exp $#
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
 * $Log: btrdict.h,v $
 * Revision 6.2  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:29  vriezen
 * Move all files to 6.1
 *
 * Revision 2.3  1994/03/04  01:04:16  jims
 * Override isEqual member function from RWCollectable to return
 * TRUE or FALSE based on operator==
 *
 * Revision 2.2  1993/09/09  02:38:31  keffer
 * Added copy constructor and assignment operator.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   04 Aug 1992 18:55:48   KEFFER
 * Added "virtual" keyword to destructor for documentation purposes.
 * 
 *    Rev 1.3   22 May 1992 17:04:08   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.2   18 Feb 1992 09:54:08   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:08   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:12:36   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

/*
 * For the storage and retrieval of (key, value) pairs.
 */

#include "rw/btree.h"
#include "rw/colclass.h"

/****************************************************************
 *								*
 *			RWBTreeDictionary			*
 *								*
 ****************************************************************/

class RWExport RWBTreeDictionary : public RWBTree {

  RWDECLARE_COLLECTABLE(RWBTreeDictionary)

public:

  RWBTreeDictionary();
  RWBTreeDictionary(const RWBTreeDictionary&);
  void			operator=(const RWBTreeDictionary&);

  virtual ~RWBTreeDictionary();

  void				applyToKeyAndValue(RWapplyKeyAndValue, void*);
  virtual void			clear();
  virtual void			clearAndDestroy();
  virtual RWCollectable*	find(const RWCollectable* key) const;  // Returns value 
  RWCollectable*		findKeyAndValue(const RWCollectable* key, RWCollectable*& value) const;
  RWCollectable*		findValue(const RWCollectable*) const;	// Returns value
  RWCollectable*		findValue(const RWCollectable*, RWCollectable*) const; // Replace value.
  RWCollectable*		insertKeyAndValue(RWCollectable* key, RWCollectable* val);
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable* key); // Returns value
  virtual void			removeAndDestroy(const RWCollectable*); 
  RWCollectable*		removeKeyAndValue(const RWCollectable* key, RWCollectable*& value);
};

#endif /* __RWBTRDICT_H__ */
