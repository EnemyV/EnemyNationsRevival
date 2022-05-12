#ifndef __RWCOLLDATE_H__
#define __RWCOLLDATE_H__

/*
 * RWCollectableDate --- RWCollectable Dates.
 *
 * $Id: colldate.h,v 6.4 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: colldate.h,v $
 * Revision 6.4  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/18  03:11:05  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.2  1994/05/26  20:23:07  myersn
 * Add conversion from julian date, to match RWDate constructor
 *
 * Revision 6.1  1994/04/15  19:09:37  vriezen
 * Move all files to 6.1
 *
 * Revision 2.9  1993/11/08  07:50:15  jims
 * Port to ObjectStore
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/04/07  02:55:10  myersn
 * eliminate member operator==.
 *
 * Revision 2.6  1993/03/31  02:54:58  myersn
 * add constructor from RWCString and RWLocale.
 *
 * Revision 2.5  1993/03/31  02:14:50  myersn
 * typo
 *
 * Revision 2.4  1993/03/31  01:53:08  myersn
 * fold in RWZone & RWLocale arguments supported by base classes RWDate & RWTime
 *
 * Revision 2.3  1993/03/26  21:16:25  myersn
 * replace dayTy etc. with unsigned.
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
 *    Rev 1.3   18 Feb 1992 09:54:14   KEFFER
 * 
 *    Rev 1.2   28 Oct 1991 09:08:10   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   09 Oct 1991 18:32:56   keffer
 * Added RWCollectableDate(const RWDate&) constructor.
 * 
 *    Rev 1.0   28 Jul 1991 08:13:32   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/rwdate.h"
#include "rw/collect.h"
#include "rw/locale.h"

/****************************************************************
 *								*
 *			RWCollectableDate			*
 *								*
 ****************************************************************/

class RWExport RWCollectableDate : public RWCollectable, public RWDate {

  RWDECLARE_COLLECTABLE(RWCollectableDate)

public:
  RWCollectableDate();
  RWCollectableDate(unsigned long j)            : RWDate(j)           { }
  RWCollectableDate(unsigned d, unsigned y)	: RWDate(d, y)        { }
  RWCollectableDate(unsigned d, const char* month, unsigned y,
		    const RWLocale& loc = RWLocale::global())
						: RWDate(d, month, y, loc) { }
  RWCollectableDate(unsigned d, unsigned m, unsigned y)
						: RWDate(d, m, y)     { }
  RWCollectableDate(istream& s, const RWLocale& locale = RWLocale::global())
						: RWDate(s, locale)   { }
  RWCollectableDate(const RWCString& str,
                    const RWLocale& locale = RWLocale::global())
                                                : RWDate(str, locale) { }
  RWCollectableDate(const RWTime& t, const RWZone& zone = RWZone::local())
						: RWDate(t, zone)     { }
  RWCollectableDate(const RWDate& d)            : RWDate(d)           { }
  RWCollectableDate(const struct tm* tmb)	: RWDate(tmb)         { }
  virtual ~RWCollectableDate();

  /* Virtual functions inherited from RWCollectable */
  virtual RWspace		binaryStoreSize() const {return RWDate::binaryStoreSize();}
  virtual int			compareTo(const RWCollectable*) const;
  virtual unsigned		hash() const;
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual void			restoreGuts(RWvistream&);
  virtual void			restoreGuts(RWFile&);
  virtual void			saveGuts(RWvostream&) const;
  virtual void			saveGuts(RWFile&) const;
};	  

#endif /* __RWCOLLECTABLE_DATE__ */
