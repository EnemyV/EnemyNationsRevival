#ifndef __RWCOLLASS_H__
#define __RWCOLLASS_H__

/*
 * Declarations for RWCollectable Associations, used in dictionaries.
 *
 * $Id: collass.h,v 6.4 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: collass.h,v $
 * Revision 6.4  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/20  16:34:41  nevis
 * Added out of line virtual destructor to IDAssoc to match
 * ctass.C...
 *
 * Revision 6.2  1994/06/18  03:11:05  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:09:36  vriezen
 * Move all files to 6.1
 *
 * Revision 2.4  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.3  1993/07/03  23:55:20  keffer
 * Removed the declaration for the no-longer-needed function rwDestroyAssociation()
 *
 * Revision 2.2  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.5   25 May 1992 15:47:12   KEFFER
 * 
 *    Rev 1.4   22 May 1992 17:04:08   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   29 Apr 1992 14:51:18   KEFFER
 * Hashing now uses chaining to resolve collisions
 * 
 *    Rev 1.2   18 Feb 1992 09:54:12   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:10   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:13:26   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

/*
 * Binds a "key" and a "value" together.  The key and value must be RWCollectable.
 */

#include "rw/collect.h"

/****************************************************************
 *								*
 *			RWCollectableAssociation		*
 *								*
 ****************************************************************/

class RWExport RWCollectableAssociation : public RWCollectable {

  RWDECLARE_COLLECTABLE(RWCollectableAssociation)

public:

  RWCollectableAssociation();
  RWCollectableAssociation(RWCollectable* k, RWCollectable* v){ky=k; val=v;}
  virtual ~RWCollectableAssociation();

  RWCollectable*		key()   const			{return ky;}
  RWCollectable*		value() const			{return val;}
  RWCollectable*		value(RWCollectable*);
  void				setValue(RWCollectable* v)	{val=v;}

  /* Inherited from class RWCollectable: */
  virtual RWspace		binaryStoreSize() const;
  virtual int			compareTo(const RWCollectable*) const;
  virtual unsigned		hash() const;
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual void			restoreGuts(RWvistream&);
  virtual void			restoreGuts(RWFile&);
  virtual void			saveGuts(RWvostream&) const;
  virtual void			saveGuts(RWFile&) const;

protected:

  RWCollectable*		ky;
  RWCollectable*		val;
};


/****************************************************************
 *								*
 *			RWCollectableIDAssociation		*
 *								*
 ****************************************************************/

class RWExport RWCollectableIDAssociation : public RWCollectableAssociation {

  RWDECLARE_COLLECTABLE(RWCollectableIDAssociation)

public:
   RWCollectableIDAssociation();
   RWCollectableIDAssociation(RWCollectable *k, RWCollectable*v) :
      RWCollectableAssociation(k,v) { }
  virtual ~RWCollectableIDAssociation();

   /*** Override these methods in order to use addresses, not values ***/

   virtual unsigned		hash() const;
   virtual int			compareTo(const RWCollectable*) const;
   virtual RWBoolean		isEqual(const RWCollectable* c) const;
};
      
#endif /* __RWCOLLASS_H__ */
