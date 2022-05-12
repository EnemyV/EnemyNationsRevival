#ifndef __RWDATE_H__
#define __RWDATE_H__

/*
 * Declarations for RWDate Class
 *
 * $Id: rwdate.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * An RWDate can be tested for validity using the member function isValid().
 *
 ***************************************************************************
 *
 * $Log: rwdate.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/16  18:11:49  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:10:44  vriezen
 * Move all files to 6.1
 *
 * Revision 2.27  1993/09/10  02:03:24  keffer
 * Added explicit copy constructors and assignment operators to
 * support g++ which otherwise generates non-optimized versions.
 *
 * Revision 2.26  1993/08/20  04:22:16  keffer
 * Made Julian day constructor public.
 *
 * Revision 2.25  1993/07/19  20:45:26  keffer
 * friend classes now use elaborated-type-specifier (ARM Sec. 11.4)
 *
 * Revision 2.24  1993/06/06  00:51:41  keffer
 * Arithmetic operators now take unsigned longs, allowing
 * arithmetic of over 64k days under DOS.
 *
 * Revision 2.23  1993/04/12  22:15:49  keffer
 * clobber() is now public.
 *
 * Revision 2.22  1993/04/12  19:26:24  keffer
 * Added static member function RWDate::now()
 *
 * Revision 2.21  1993/04/09  02:50:00  keffer
 * Added support for operator<< and >> for virtual streams and RWFile.
 *
 * Revision 2.20  1993/04/07  00:46:14  myersn
 * change interfaces to month and weekday name functions.
 *
 * Revision 2.18  1993/04/06  17:49:30  keffer
 * Logical operators now global instead of friends.
 * Exported operator>>().
 *
 * Revision 2.17  1993/04/06  17:12:17  myersn
 * remove inline operator >> to rwdateio.cpp, make it use RWLocale::of(s).
 *
 * Revision 2.16  1993/04/05  15:34:11  keffer
 * Arithmetic assignment operators now return *this.
 *
 * Revision 2.15  1993/03/31  02:54:58  myersn
 * add constructor from RWCString and RWLocale.
 *
 * Revision 2.14  1993/03/30  21:06:10  myersn
 * inline the static members dayOfWeek and indexOfMonth, t
 * to delegate to RWLocale.
 *
 * Revision 2.13  1993/03/25  06:03:27  myersn
 * replace monthTy etc. with unsigned.
 *
 * Revision 2.12  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.11  1993/02/17  03:10:23  keffer
 * Changed const notation to follow style guide
 *
 * Revision 2.10  1993/02/06  04:02:32  keffer
 * Added rwexport tag to operator<<().
 *
 * Revision 2.9  1993/02/03  00:21:35  jims
 * Removed #include directive for procinit.h
 *
 * Revision 2.8  1993/01/29  21:51:05  myersn
 * extend asString() interface.
 *
 * Revision 2.6  1992/12/04  05:03:59  myersn
 * make RWLocale::global() and RWZone::local() more flexible.
 *
 * Revision 2.5  1992/12/01  05:29:03  myersn
 * remove zone argument from RWDate::parseFrom()
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
 *    Rev 1.8   07 Jun 1992 16:06:58   KEFFER
 * Introduced HAS_POSTFIX macro
 * 
 *    Rev 1.7   17 Mar 1992 11:59:16   KEFFER
 * Pre- and post-increment and decrement operators now have a return type.
 * 
 *    Rev 1.6   04 Mar 1992 10:21:20   KEFFER
 * Instance manager used in multi-thread situation.
 * 
 *    Rev 1.3   13 Nov 1991 11:10:24   keffer
 * Static variables maintained by a manager for multi-thread version
 * 
 *    Rev 1.2   29 Oct 1991 13:57:32   keffer
 * Added postfix ++ and --.
 * 
 *    Rev 1.1   28 Oct 1991 09:08:20   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 */

#include "rw/tooldefs.h"
#include "rw/zone.h"
#include "rw/locale.h"

class RWExport RWTime;
class RWExport RWCString;
class RWExport RWLocale;

const unsigned long  rwJul1901 = 2415386L;	// Julian day for 1/1/1901

class RWExport RWDate
{

public:

  RWDate() : julnum(RWDate::currentDate()) {;} // Default constructor: today's date
  RWDate(unsigned long j) : julnum(j) {;}      // Julian day; beware of type conversion

  /* Construct a RWDate with a given day of the year and a given year.
     The base date for this computation is Dec. 31 of the previous year.  
     If year == 0, Construct a RWDate with Jan. 1, 1901 as the "day zero".  
        i.e., RWDate(-1) = Dec. 31, 1900 and RWDate(1) = Jan. 2, 1901. */
  RWDate(unsigned day, unsigned year);

  //  Construct a RWDate for the given day, month, and year.
  RWDate(unsigned, const char* month, unsigned year,
         const RWLocale& = RWLocale::global()); // for month names
  RWDate(unsigned d, unsigned m, unsigned);
  RWDate(istream& s, const RWLocale& l = RWLocale::global())
                        {parseFrom(s, l);}  // Read date from stream.
  RWDate(const RWCString& str, const RWLocale& = RWLocale::global());
  RWDate(const RWTime&, const RWZone& = RWZone::local());
  RWDate(const struct tm*);  // from members: tm_year, tm_mon, tm_mday

  // The following two should not be necessary, but are required to
  // support feeble compilers:
  RWDate(const RWDate& d) : julnum(d.julnum) {;}
  RWDate& operator=(const RWDate& d) {julnum=d.julnum; return *this;}

  RWCString		asString(char format = 'x',
				 const RWLocale& = RWLocale::global()) const;
  RWBoolean 		between(const RWDate& d1, const RWDate& d2) const
			{ return julnum >= d1.julnum && julnum <= d2.julnum; }
  int 			compareTo(const RWDate*) const;
  unsigned 	    	day() const;		// 1-365
  unsigned 		dayOfMonth() const;	// 1-31
  void                  extract(struct tm*) const; // set all tm members
  unsigned	    	firstDayOfMonth(unsigned mon) const;
  unsigned 	    	firstDayOfMonth() const
                           { return firstDayOfMonth(month());}
  unsigned		hash() const;
  RWBoolean		isValid() const { return julnum>0; }
  RWBoolean 		leap() const { return leapYear(year());}  // leap year?
  RWDate		max(const RWDate& dt) const;
  RWDate		min(const RWDate& dt) const;
  unsigned	    	month() const;
  RWCString		monthName(const RWLocale& =RWLocale::global()) const;
                        // Return date of previous dayName:
  RWDate    		previous(const char* dayName,
                                 const RWLocale& =RWLocale::global()) const;
                        // Same as above, but use numeric day of week
  RWDate    		previous(unsigned day) const; 		 
  unsigned 	    	weekDay() const;
  RWCString		weekDayName(const RWLocale& =RWLocale::global()) const;
  unsigned 	    	year() const;

  // Arithmetic operators:
  RWDate		operator++()	       { return RWDate(++julnum); }
  RWDate 		operator--()	       { return RWDate(--julnum); }
#ifndef RW_NO_POSTFIX
  RWDate		operator++(int)	       { return RWDate(julnum++); }
  RWDate		operator--(int)	       { return RWDate(julnum--); }
#endif
  RWDate& 		operator+=(unsigned long dd) { julnum += dd; return *this; }
  RWDate& 		operator-=(unsigned long dd) { julnum -= dd; return *this; }

  // Read or write dates:
#ifndef RW_TRAILING_RWEXPORT
  friend rwexport ostream& operator<<(ostream&, const RWDate&);
  friend rwexport istream& operator>>(istream& s, RWDate& d);
#else
  friend ostream& rwexport operator<<(ostream&, const RWDate&);
  friend istream& rwexport operator>>(istream& s, RWDate& d);
#endif
  RWspace		binaryStoreSize() const {return sizeof(julnum);}
  void			restoreFrom(RWvistream& s);
  void	  		restoreFrom(RWFile&);
  void			saveOn(RWvostream& s) const;
  void			saveOn(RWFile&) const;

  // Global friends:
friend RWDate		operator+(const RWDate& dt, unsigned long dd)
				{ return RWDate(dt.julnum + dd); }
friend RWDate		operator+(unsigned long dd, const RWDate& dt)
				{ return RWDate(dt.julnum + dd); }
friend RWDate		operator-(const RWDate& dt, unsigned long dd)
				{ return RWDate(dt.julnum - dd); }
friend unsigned long	operator-(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum - d2.julnum; }
friend RWBoolean	operator<(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum < d2.julnum; }
friend RWBoolean	operator<=(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum <= d2.julnum; }
friend RWBoolean	operator>(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum > d2.julnum; }
friend RWBoolean	operator>=(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum >= d2.julnum; }
friend RWBoolean	operator==(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum == d2.julnum; }
friend RWBoolean	operator!=(const RWDate& d1, const RWDate& d2)
				{ return d1.julnum != d2.julnum; }

  // Static member functions:
  static  void		clobber(struct tm*);
  static  unsigned 	dayOfWeek(const char* dayName,
                                  const RWLocale& = RWLocale::global());
  static  unsigned     	daysInYear(unsigned year);
  static  RWBoolean 	dayWithinMonth(unsigned mon, unsigned day, unsigned yr);
   
  static  unsigned 	indexOfMonth(const char* monName,
                                     const RWLocale& = RWLocale::global());
  static  unsigned long	jday(unsigned mon, unsigned day, unsigned year);
  static  RWBoolean	leapYear(unsigned year);
  static  RWCString     nameOfMonth(unsigned month,
				    const RWLocale& =RWLocale::global());
  static  RWDate	now();			// return today's date.
  static  RWCString	weekDayName(unsigned day,
				    const RWLocale& =RWLocale::global());

  
  // static const data
  static const unsigned char daysInMonth[12];          // [1..31]
  static const unsigned      firstDayOfEachMonth[12];  // [0..334]

  // For functions that returned char*, use RWLocale facilities for new code:
  // for member nameOfDay()
  //    use member weekDayName(const RWLocale&)
  // for member nameOfMonth()
  //    use member monthName(const RLocale&)
  // for static member dayName(unsigned)
  //    use static member weekDayName(unsigned, const RWLocale&)
  // for static member monthName(unsigned)
  //    use static member nameOfMonth(unsigned, const RWLocale&)

protected:

  static RWBoolean 	assertWeekDayNumber(unsigned d)
			{return d>=1 && d<=7;}
  static RWBoolean 	assertIndexOfMonth(unsigned m)
			{return m>=1 && m<=12;}
private:

  friend class RWExport RWTime;
  void 			parseFrom(istream&, const RWLocale&);
  void rwnear		mdy(unsigned& m, unsigned& d, unsigned& y) const;
  static unsigned long	currentDate();

  unsigned long		julnum;	// Julian Day Number (not same as Julian date).

};

// inline members
inline RWCString RWDate::monthName(const RWLocale& locale) const
		{ return nameOfMonth(month(), locale); }
inline RWCString RWDate::weekDayName(const RWLocale& locale) const
		{ return weekDayName(weekDay(), locale); }

// inline statics

// Return the number, [1..7], of the day of the week named, or 0 for failure
inline unsigned RWDate::dayOfWeek(const char* dayName, const RWLocale& locale)
                { return (unsigned) locale.weekdayIndex(RWCString(dayName)); }

// Return the number, [1..12], of the month named, or 0 for failure
inline unsigned RWDate::indexOfMonth(const char* mon, const RWLocale& locale)
                { return (unsigned) locale.monthIndex(RWCString(mon)); }

// inline globals

inline RWvistream& operator>>(RWvistream& str,       RWDate& date)
		{ date.restoreFrom(str);  return str;  }
inline RWFile&     operator>>(RWFile& file,          RWDate& date)
		{ date.restoreFrom(file); return file; }
inline RWvostream& operator<<(RWvostream& str, const RWDate& date)
		{ date.saveOn(str);       return str;  }
inline RWFile&     operator<<(RWFile& file,    const RWDate& date)
		{ date.saveOn(file);      return file; }

#endif	 /* __RWDATE_H__ */
