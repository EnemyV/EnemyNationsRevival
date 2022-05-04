#ifndef __RWZONE_H__
#define __RWZONE_H__ 1

/*
 * Declarations for class RWTime.
 *
 * $Id: zone.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: zone.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:57  vriezen
 * Move all files to 6.1
 *
 * Revision 1.19  1993/09/13  01:38:43  keffer
 * Prefixed #include wrapper with 'RW'
 *
 * Revision 1.18  1993/09/10  17:20:46  jims
 * Add RWExport to RWZoneSimple class
 *
 * Revision 1.17  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.16  1993/08/06  17:46:02  keffer
 * Changed name ZoneDefault to ZoneSimple for clarity.
 *
 * Revision 1.14  1993/07/03  02:19:31  myersn
 * allow support for mainland China DST rules (April 12 - September 12).
 *
 * Revision 1.13  1993/04/13  08:38:33  myersn
 * add RWZoneSimple(StdZone, DstRule) constructor.
 *
 * Revision 1.12  1993/04/12  21:53:49  myersn
 * change names of some time zones in StdZone.
 *
 * Revision 1.11  1993/04/10  19:52:48  keffer
 * Changed argument name tzname to tz_name to avoid ambiguity with
 * global variable of the same name.
 *
 * Revision 1.10  1993/04/09  23:17:57  myersn
 * generalize and simplify RWZoneSimple interface.
 *
 * Revision 1.9  1993/03/25  06:01:13  myersn
 * eliminate RWDaylightRule::mustFree_ member.
 *
 * Revision 1.8  1993/03/12  02:21:44  myersn
 * make usRule DST rules available to users defining zones.
 *
 * Revision 1.7  1993/02/17  03:10:23  keffer
 * Changed const notation to follow style guide
 *
 * Revision 1.6  1993/01/18  22:56:23  myersn
 * add observed_ field to RWDaylightRule to allow no-DST years
 *
 * Revision 1.4  1992/12/16  00:53:52  myersn
 * improvements to RWZoneSimple interface
 *
 * Revision 1.3  1992/12/04  05:03:59  myersn
 * make RWLocale::global() and RWZone::local() more flexible.
 *
 * Revision 1.2  1992/11/27  21:05:46  myersn
 * build up separate RWZone class for time zone & daylight encapsulation.
 *
 * Revision 1.1  1992/11/20  02:58:36  myersn
 * Initial revision
 *
 *
 */

#include "rw/compiler.h"
#include "rw/defs.h"
#include "rw/cstring.h"
STARTWRAP
#include <time.h>		 /* System time management. */
ENDWRAP

struct RWExport RWDaylightBoundary {
  // this struct uses <time.h> struct tm conventions:
  int month_;    // [0..11]
  int week_;     // [0..4], 4=last week in month [or, -1=use month day]
  int weekday_;  // [0..6], 0=Sunday             [or, 1..31 if week_==-1]
  int minute_;   // [0..1439]  (Usually 2 AM, = 120)

  void date(struct tm*) const;
  RWBoolean after(const struct tm*) const;
};
  
struct RWExport RWDaylightRule {
  RWDaylightRule const* next_;
  short firstYear_;
  char observed_;
  RWDaylightBoundary begin_;   // begin may be after end
  RWDaylightBoundary end_;

  RWDaylightRule& operator= (const RWDaylightRule&);
//  RWDaylightRule(const RWDaylightRule&);  // do not declare constructors.
  RWBoolean isDaylight(const struct tm*) const;
};

class RWExport RWZone {
 public:
  virtual ~RWZone();

  virtual int       timeZoneOffset  () const = 0;   // seconds west of UTC
  virtual int       altZoneOffset   () const = 0;
  virtual RWBoolean daylightObserved() const = 0;
  virtual RWCString timeZoneName    () const = 0;
  virtual RWCString altZoneName     () const = 0;

  // isDaylight() may use all members of tmbuf except tm_isdst
  virtual RWBoolean isDaylight(const struct tm* tmbuf) const = 0;

  // The following functions use the tm_year member of struct tm, and
  //   set all the remaining members.
  virtual void getBeginDaylight(struct tm*) const = 0;
  virtual void getEndDaylight(struct tm*) const = 0;

  static const RWZone& local();  
  static const RWZone& standard();
  static const RWZone& utc();

  static const RWZone* local(const RWZone*);     // set local()
  static const RWZone* standard(const RWZone*);  // set standard()

  enum StdZone {
	NewZealand=-12,         CarolineIslands,	MarianaIslands,
	Japan,			China, 			Java,
	Kazakh,			Pakistan, 		CaspianSea,
 	Ukraine,		Nile,			Europe,
	Greenwich,		Azores, 		Oscar,
	Greenland, 		Atlantic, 		USEastern,
	USCentral,		USMountain, 		USPacific,
	Yukon,			Hawaii, 		Bering
  };

  enum DstRule {    // supported Daylight Savings Time jurisdictions:
    NoDST,              // Daylight Savings Time never observed
    NoAm,		// North America (US, Canada)
    WeEu		// Western Europe, except UK
  };
  static const RWDaylightRule* dstRule(DstRule=NoAm);

  // RWZoneSimple implements DST rules simple enough to represent
  // as static instances of RWDaylightRule.
  // DST Jurisdictions too complex to represent in table form
  //  require deriving from RWZone to implement necessary semantics.
};

class RWExport RWZoneSimple : public RWZone {
 public: 
  const RWDaylightRule* rule_;
  RWBoolean       daylightObserved_;
  long            timeZoneOffset_;
  RWCString       timeZoneName_;
  long            altZoneOffset_;
  RWCString       altZoneName_;

  RWZoneSimple(const RWDaylightRule* rule,
	        long tzoff,  const RWCString& tz_name,
		long altoff, const RWCString& altname)
    : rule_(rule), daylightObserved_(rule != 0)
    , timeZoneOffset_(tzoff), timeZoneName_(tz_name)
    , altZoneOffset_(altoff), altZoneName_(altname)
    { }

  RWZoneSimple(long tzoff, const RWCString& tz_name);
  RWZoneSimple(StdZone zone, const RWDaylightRule* rule);
  RWZoneSimple(StdZone zone, DstRule = NoAm);
  
  virtual ~RWZoneSimple();

  virtual int       timeZoneOffset  () const;   // seconds west of UTC
  virtual int       altZoneOffset   () const;
  virtual RWBoolean daylightObserved() const;
  virtual RWCString timeZoneName    () const;
  virtual RWCString altZoneName     () const;

  // isDaylight() may use all members of tmbuf except tm_isdst
  virtual RWBoolean isDaylight(const struct tm* tmbuf) const;

  // The following functions use the tm_year member of struct tm, and
  //   set all the remaining members.
  virtual void getBeginDaylight(struct tm*) const;
  virtual void getEndDaylight(struct tm*) const;
};

#endif /* __RWZONE_H__ */
