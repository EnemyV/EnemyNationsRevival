#ifndef __RWCOLLTIME_H__
#define __RWCOLLTIME_H__

/*
 * RWCollectableTime --- collectable times.
 *
 * $Id: colltime.h,v 6.3 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: colltime.h,v $
 * Revision 6.3  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  03:11:05  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:09:43  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1993/11/08  07:53:42  jims
 * Port to ObjectStore
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/03/31  02:54:58  myersn
 * add constructor from RWCString and RWLocale.
 *
 * Revision 2.4  1993/03/31  01:53:08  myersn
 * fold in RWZone & RWLocale arguments supported by base classes RWDate & RWTime
 *
 * Revision 2.3  1993/03/25  05:32:01  myersn
 * eliminate hourTy, monthTy, etc. references.
 *
 * Revision 2.2  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   22 May 1992 17:04:10   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   18 Feb 1992 09:54:16   KEFFER
 * 
 *    Rev 1.2   28 Oct 1991 09:08:12   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   09 Oct 1991 18:28:56   keffer
 * Provided RWCollectableTime(const RWTime&) constructor.
 * 
 *    Rev 1.0   28 Jul 1991 08:14:00   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/rwtime.h"
#include "rw/collect.h"
#include "rw/zone.h"

/****************************************************************
 *								*
 *			RWCollectableTime			*
 *								*
 ****************************************************************/

class RWExport RWCollectableTime : public RWCollectable, public RWTime {

  RWDECLARE_COLLECTABLE(RWCollectableTime)

public:

  RWCollectableTime();
  RWCollectableTime(unsigned long s)       : RWTime(s) { }
  RWCollectableTime(const RWTime& t) : RWTime(t) { }
  RWCollectableTime(unsigned h, unsigned m, unsigned s = 0,
		    const RWZone& zone = RWZone::local())
    : RWTime(h, m, s, zone) { }
  RWCollectableTime(const RWDate& d, unsigned h=0, unsigned m=0, unsigned s=0,
		    const RWZone& zone = RWZone::local())
    : RWTime(d, h, m, s, zone)   { }
  RWCollectableTime(const RWDate& d, const RWCString& str,
                    const RWZone& z = RWZone::local(),
                    const RWLocale& l = RWLocale::global())
    : RWTime(d, str, z, l) {}
  RWCollectableTime(const struct tm* tmb, const RWZone& zone = RWZone::local())
    : RWTime(tmb, zone) { }

  virtual ~RWCollectableTime();
  /* Virtual functions inherited from RWCollectable */
  virtual RWspace		binaryStoreSize() const {return RWTime::binaryStoreSize();}
  virtual int			compareTo(const RWCollectable*) const;
  virtual unsigned		hash() const;
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual void			restoreGuts(RWvistream&);
  virtual void			restoreGuts(RWFile&);
  virtual void			saveGuts(RWvostream&) const;
  virtual void			saveGuts(RWFile&) const;
};

#endif /* __RWCOLLTIME_H__ */
