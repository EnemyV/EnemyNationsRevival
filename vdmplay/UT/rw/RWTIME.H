#ifndef __RWTIME_H__
#define __RWTIME_H__

/*
 * Declarations for class RWTime.
 *
 * $Id: rwtime.h,v 6.5 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwtime.h,v $
 * Revision 6.5  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.4  1994/06/17  05:24:50  myersn
 * add extractGMT(struct tm*), for symmetry with hourGMT() etc.
 *
 * Revision 6.3  1994/06/07  13:53:16  vriezen
 * Change rwEpoch constant to 2177452800UL, was 2177452800L
 *
 * Revision 6.2  1994/05/16  18:11:21  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:10:52  vriezen
 * Move all files to 6.1
 *
 * Revision 2.29  1994/04/01  21:33:30  vriezen
 * Use RW_INLINE_FRIEND symbol for inline friends
 *
 * Revision 2.28  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.27  1993/09/10  02:03:24  keffer
 * Added explicit copy constructors and assignment operators to
 * support g++ which otherwise generates non-optimized versions.
 *
 * Revision 2.26  1993/07/29  11:17:47  jims
 * #define away the inline keyword for compilers that  eschew "friend inline"
 *
 * Revision 2.25  1993/07/15  02:16:16  myersn
 * add default arguments for beginDST() and endDST() static members.
 *
 * Revision 2.24  1993/05/01  01:15:19  keffer
 * Now #includes rwlocale.h instead of rwdate.h.
 *
 * Revision 2.23  1993/04/13  19:23:25  keffer
 * Corrected error in inlined constructor.
 *
 * Revision 2.22  1993/04/12  19:26:24  keffer
 * Added static member function RWTime::now().
 *
 * Revision 2.21  1993/04/09  02:50:00  keffer
 * Added support for operator<< and >> for virtual streams and RWFile.
 *
 * Revision 2.20  1993/04/07  02:32:06  myersn
 * redeclare friend operators, prevent compiler warnings.
 *
 * Revision 2.19  1993/04/06  18:12:08  keffer
 * Removed operator-(long, RWTime).
 *
 * Revision 2.17  1993/04/05  00:09:29  keffer
 * Logical operators are now global friends rather than member functions.
 * Postfix and prefix operators now return values.
 * Arithmetic assignments now return self.
 *
 * Revision 2.16  1993/04/04  23:30:16  keffer
 * Added member function reset().
 *
 * Revision 2.15  1993/03/31  02:54:58  myersn
 * add constructor from RWCString and RWLocale.
 *
 * Revision 2.14  1993/03/25  06:02:52  myersn
 * replace hourTy etc. with unsigned.
 *
 * Revision 2.13  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.12  1993/02/17  03:10:23  keffer
 * Changed const notation to follow style guide
 *
 * Revision 2.11  1993/02/06  04:06:18  keffer
 * Added rwexport tag to operator<<().
 *
 * Revision 2.10  1993/02/03  00:20:51  jims
 * Removed #include directive for procinit.h
 *
 * Revision 2.9  1993/01/29  20:28:20  myersn
 * extend RWTime::asString() interface.
 *
 * Revision 2.8  1993/01/23  01:38:41  keffer
 * Corrected polarity of RW_NO_POSTFIX macro.
 *
 *
 * Revision 2.6  1992/12/04  05:03:59  myersn
 * make RWLocale::global() and RWZone::local() more flexible.
 *
 * Revision 2.5  1992/12/01  04:00:40  myersn
 * deleted RWTime::hour() (no-argument version)
 *
 * Revision 2.4  1992/11/28  00:14:52  myersn
 * moved inline friends out of class declaration
 *
 * Revision 2.3  1992/11/27  21:05:46  myersn
 * add RWZone interface
 *
 * Revision 2.2  1992/11/20  02:58:36  myersn
 * changes in support of time zones, RWLocale, and struct tm
 *
 * Revision 2.1  1992/11/14  00:08:10  myersn
 * added struct tm and RWLocale interface
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.7   07 Jun 1992 16:06:58   KEFFER
 * Introduced HAS_POSTFIX macro
 * 
 *    Rev 1.6   04 Mar 1992 10:21:36   KEFFER
 * Instance manager used in multi-thread situation.
 * 
 *    Rev 1.4   13 Nov 1991 11:10:26   keffer
 * Static variables maintained by a manager for multi-thread version
 *
 *    Rev 1.3   29 Oct 1991 13:57:04   keffer
 * Added 24hour clock print option.
 * 
 *    Rev 1.2   28 Oct 1991 09:08:22   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   22 Aug 1991 10:20:16   keffer
 * Simplified construction of times by defining m.f. buildFrom()
 * 
 *    Rev 1.0   28 Jul 1991 08:16:44   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/locale.h"
STARTWRAP
#include <time.h>		 /* System time management. */
ENDWRAP

class RWExport RWCString;
class RWExport RWDate;

// typedef 	unsigned	 	hourTy;
// typedef 	unsigned	 	minuteTy;
// typedef 	unsigned	 	secondTy;
// typedef 	unsigned long	  	clockTy;
const 	unsigned long rwEpoch = 2177452800UL;  // conversion from to Unix time

class RWExport RWTime
{

public:

  RWTime() : sec(RWTime::currentTime()) {;}	// Default constructor: current time
  RWTime(unsigned long s) : sec(s) {;}		// Seconds since 00:00:00 Jan 1, 1901
  RWTime(unsigned h, unsigned m, unsigned s=0,	// today
         const RWZone& = RWZone::local());
  RWTime(const RWDate&, unsigned h=0, unsigned m=0, unsigned s=0,
         const RWZone& = RWZone::local());
  RWTime(const RWDate&, const RWCString& str,
         const RWZone& = RWZone::local(),
         const RWLocale& = RWLocale::global());

  // This constructor depends on the following members of the struct tm arg:
  //    tm_year, tm_mon, tm_mday, tm_hour, tm_min, and tm_sec.
  RWTime(const struct tm*, const RWZone& = RWZone::local());

  // The following two should not be necessary, but are required to
  // support feeble compilers:
  RWTime(const RWTime& t) : sec(t.sec) {;}
  RWTime& operator=(const RWTime& t) {sec = t.sec; return *this;}

  // this function sets all members of its struct tm argument:
  void		extract(struct tm*, const RWZone& = RWZone::local()) const;
  void		extractGMT(struct tm*) const;

  RWCString	asString(char format = '\0',
			 const RWZone& = RWZone::local(),
			 const RWLocale& = RWLocale::global()) const;
  RWBoolean	between(const RWTime& a, const RWTime& b) const
			{ return *this >= a && *this <= b;}
  RWspace	binaryStoreSize() const {return sizeof(unsigned long);} 
  int 		compareTo(const RWTime*) const;
  unsigned 	hash()	  const;
  unsigned   	hour(const RWZone& = RWZone::local())  const;
  unsigned   	hourGMT() const;	// hour: UTC
  RWBoolean 	isDST(const RWZone& = RWZone::local()) const;
  RWBoolean	isValid() const { return sec != 0; }
  RWTime 	max(const RWTime& t) const;
  RWTime 	min(const RWTime& t) const;
  unsigned 	minute(const RWZone& = RWZone::local()) const;
  unsigned 	minuteGMT() 	const;	// minute: UTC
  void		restoreFrom(RWFile&);
  void		restoreFrom(RWvistream&);
  void		saveOn(RWFile&) const;
  void		saveOn(RWvostream&) const;
  unsigned 	second() const;		// second: local time or UTC 
  unsigned long seconds() const {return sec;}
#ifndef RW_TRAILING_RWEXPORT
  friend rwexport ostream& operator<<(ostream&, const RWTime&);
#else
  friend ostream& rwexport operator<<(ostream&, const RWTime&);
#endif

  RWTime	operator++()		{ return RWTime(++sec); }
  RWTime	operator--()		{ return RWTime(--sec); }
#ifndef RW_NO_POSTFIX
  RWTime	operator++(int)		{ return RWTime(sec++); }
  RWTime	operator--(int)		{ return RWTime(sec--); }
#endif
  RWTime&	operator+=(long s)	{ sec += s; return *this; }
  RWTime& 	operator-=(long s)	{ sec -= s; return *this; }

  // Static member functions:
  static RWTime	beginDST(unsigned year, const RWZone& = RWZone::local());
  static RWTime	endDST  (unsigned year, const RWZone& = RWZone::local());
  static RWTime	now();		// Return present time

  // Logical operators.
RW_INLINE_FRIEND RWBoolean	operator< (const RWTime& t1, const RWTime& t2);
RW_INLINE_FRIEND RWBoolean	operator<=(const RWTime& t1, const RWTime& t2);
RW_INLINE_FRIEND RWBoolean	operator> (const RWTime& t1, const RWTime& t2);
RW_INLINE_FRIEND RWBoolean	operator>=(const RWTime& t1, const RWTime& t2);
RW_INLINE_FRIEND RWBoolean	operator==(const RWTime& t1, const RWTime& t2);
RW_INLINE_FRIEND RWBoolean	operator!=(const RWTime& t1, const RWTime& t2);

  // Add or subtract seconds.
RW_INLINE_FRIEND RWTime		operator+(const RWTime& t, long s         );
RW_INLINE_FRIEND RWTime		operator-(const RWTime& t, long s         );
RW_INLINE_FRIEND RWTime		operator+(long s         , const RWTime& t);

  
protected:

  static RWBoolean 	assertDate(const RWDate&);

private:

  static unsigned long	buildFrom(const RWDate&, unsigned h, unsigned m, unsigned s,
				  const RWZone& = RWZone::local());
  static unsigned long	currentTime();

  unsigned long 	sec;		// Seconds since 1/1/1901.

};

inline RWBoolean	operator< (const RWTime& t1, const RWTime& t2)
		{ return t1.sec <  t2.sec; }
inline RWBoolean 	operator<=(const RWTime& t1, const RWTime& t2)
		{ return t1.sec <= t2.sec; }
inline RWBoolean	operator> (const RWTime& t1, const RWTime& t2)
		{ return t1.sec >  t2.sec; }
inline RWBoolean	operator>=(const RWTime& t1, const RWTime& t2)
		{ return t1.sec >= t2.sec; }
inline RWBoolean	operator==(const RWTime& t1, const RWTime& t2)
		{ return t1.sec == t2.sec; }
inline RWBoolean	operator!=(const RWTime& t1, const RWTime& t2)
		{ return t1.sec != t2.sec; }

inline RWTime		operator+(const RWTime& t, long s         )
		{ return RWTime(t.sec+s); }
inline RWTime		operator-(const RWTime& t, long s         )
		{ return RWTime(t.sec-s); }
inline RWTime		operator+(long s         , const RWTime& t)
		{ return RWTime(t.sec+s); }

inline RWvistream&	operator>>(RWvistream& str,       RWTime& t)
		{ t.restoreFrom(str);  return str;  }
inline RWFile&		operator>>(RWFile& file,          RWTime& t)
		{ t.restoreFrom(file); return file; }
inline RWvostream&	operator<<(RWvostream& str, const RWTime& t)
		{ t.saveOn(str);       return str;  }
inline RWFile&		operator<<(RWFile& file,    const RWTime& t)
		{ t.saveOn(file);      return file; }

inline RWTime::RWTime(const RWDate& date, unsigned h, unsigned m, unsigned s,
                      const RWZone& zone)
    { sec = buildFrom(date, h, m, s, zone); }

#endif  /* __RWTIME_H__ */
