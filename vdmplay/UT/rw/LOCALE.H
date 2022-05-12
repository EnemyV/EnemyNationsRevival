#ifndef __RWLOCALE_H__
#define __RWLOCALE_H__

/*
 * Declarations for the RWLocale class
 *
 * $Id: locale.h,v 6.4 1994/07/12 23:58:57 nevis Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 * Voice: (503) 754-3010	FAX: (503) 757-6650
 * email: support@roguewave.com
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
 * Classes RWLocale and RWLocaleSnapshot support encapsulation of local
 * formatting conventions.  RWLocaleDefault provides the same interface for
 * access to the global process state "non-encapsulation" of the same
 * functions.
 *
 ***************************************************************************
 *
 * $Log: locale.h,v $
 * Revision 6.4  1994/07/12  23:58:57  nevis
 * Moved undefined member functions to the end of the class
 * declaration to avoid vtable problems in MetaWare.
 *
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/24  21:20:35  myersn
 * don't include <locale.h> if it is not available.
 *
 * Revision 6.1  1994/04/15  19:10:26  vriezen
 * Move all files to 6.1
 *
 * Revision 1.19  1993/10/22  20:54:40  griswolf
 * make stringToMoney not a pure virtual when RW_GLOBAL_ENUMS is defined
 *
 * Revision 1.18  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.17  1993/08/05  05:51:05  jims
 * Add RWExport to forward declaration of RWLocaleDefault
 *
 * Revision 1.16  1993/07/30  03:25:20  jims
 * MS C7 insists that forward decl of RWTimeFmtList be "struct" to match def.
 *
 * Revision 1.15  1993/07/28  21:48:34  myersn
 * more fixes to support japanese dates.
 *
 * Revision 1.14  1993/07/13  02:29:08  myersn
 * add CurrSymbol argument to stringToMoney.
 *
 * Revision 1.13  1993/05/27  18:50:16  keffer
 * Now scopes enum CurrSymbol as RWLocale::CurrSymbol
 *
 * Revision 1.12  1993/04/20  23:37:47  myersn
 * make asString(struct tm* ...) argument const.
 *
 * Revision 1.11  1993/04/15  02:17:29  myersn
 * add RWLocale::imbue(), guard against entire lack of ios::xalloc() support.
 *
 * Revision 1.10  1993/04/09  23:18:33  myersn
 * declare RWLocale::imbue() const.
 *
 * Revision 1.9  1993/04/06  20:35:33  myersn
 * add default value for RWLocale::moneyAsString() CurrSym argument.
 *
 * Revision 1.8  1993/04/06  17:12:17  myersn
 * add RWLocale::of(ios&) and RWLocale::imbue(ios&), and money conversions.
 *
 * Revision 1.7  1993/03/31  02:18:47  myersn
 * add constructor RWLocaleSnapshot(const char* locName = 0) to temporarily
 * substitute the named locale during construction. Note that "" is allowed.
 *
 * Revision 1.6  1993/02/17  03:10:23  keffer
 * Changed const notation to follow style guide
 *
 * Revision 1.5  1992/12/04  05:03:59  myersn
 * make RWLocale::global() and RWZone::local() more flexible.
 *
 * Revision 1.4  1992/12/01  04:01:55  myersn
 * changed RWLocaleFluid to RWLocaleDefault because RWLocaleFluid
 * is too hard to implement.
 *
 * Revision 1.3  1992/11/27  21:05:46  myersn
 * add RWZone interface
 *
 * Revision 1.2  1992/11/20  02:58:36  myersn
 * changes in support of time zones, RWLocale, and struct tm
 *
 * Revision 1.1  1992/11/14  00:08:56  myersn
 * Initial revision
 *
 * 
 */

#ifndef __RWCOMPILER_H__
#include "rw/compiler.h"	/* for STARTWRAP */
#endif
STARTWRAP
#if !defined(RW_NO_LOCALE)
#include <locale.h>
#endif
#include <time.h>    		/* for struct tm and time_t */
ENDWRAP
#include "rw/defs.h"            /* for RWExport */
#include "rw/cstring.h"    	/* for RWCString */
#include "rw/zone.h"            /* for RWZone::local() */

struct RWTimeFmtList;    // internal type
class  RWExport RWLocaleDefault;

// RWLocale is an abstract base class defining an interface to
//  locale-dependent programming facilities.

class RWExport RWLocale {
 public:
    virtual ~RWLocale(); 

    virtual RWCString asString(long) const = 0;
    virtual RWCString asString(unsigned long) const = 0;
    virtual RWCString asString(
      double f, int precision = 6, RWBoolean showpoint = 0) const = 0;

    // asString(tm*, format): format is one of [aAbBcdHIjmMpSUwWxXyYZ],
    //                        as in ANSI C Library function strftime().
    virtual RWCString asString(const struct tm* tmbuf, char format,
                               const RWZone& = RWZone::local()) const = 0;

    enum CurrSymbol { NONE, LOCAL, INTL };

    virtual RWCString moneyAsString(double,
#ifdef RW_GLOBAL_ENUMS
				    CurrSymbol=LOCAL) const = 0;
#else
				    RWLocale::CurrSymbol=LOCAL) const = 0;
#endif

    virtual RWBoolean stringToNum  (const RWCString&, double*) const = 0;
    virtual RWBoolean stringToNum  (const RWCString&, long*) const = 0;
    virtual RWBoolean stringToDate (const RWCString&, struct tm*) const = 0;
    virtual RWBoolean stringToTime (const RWCString&, struct tm*) const = 0;

    virtual RWBoolean stringToMoney(const RWCString&, double*,
#ifdef RW_GLOBAL_ENUMS
                                    CurrSymbol=LOCAL) const = 0;
#else
                                    RWLocale::CurrSymbol=LOCAL) const = 0;
#endif

    // returns [1..12] (1 for January), 0 for error
    virtual int monthIndex(const RWCString&) const = 0;
    // returns 1 for Monday equivalent, 7 for Sunday, 0 for error.
    virtual int weekdayIndex(const RWCString&) const = 0;

    static const RWLocale& global();  // the default locale for most functions
    static const RWLocale* global(const RWLocale*);  // a function to set it.

#ifndef RW_IOS_XALLOC_BROKEN

    const RWLocale*   imbue(ios&) const;  // assign this locale to a stream
    static const RWLocale*  unimbue(ios&);  // remove the locale from a stream

#endif /* RW_IOS_XALLOC_BROKEN */

    static const RWLocale&  of(ios&);     // retrieve imbued locale, if any

};


// RWLocaleDefault is used when you do not want an encapsulated locale
//  object, but need an RWLocale for the interface.  There is only one
//  instance of RWLocaleDefault, initially returned by RWLocale::global(),
//  used as the default value for RWLocale& arguments.

class RWExport RWLocaleDefault : public RWLocale {
 public:
    RWLocaleDefault();

    virtual ~RWLocaleDefault(); 

    virtual RWCString asString(long) const;
    virtual RWCString asString(unsigned long) const;
    virtual RWCString asString(double f, int precision = 6,
                               RWBoolean showpoint = 0) const;
    virtual RWCString asString(const struct tm* tmbuf, char format,
                               const RWZone& = RWZone::local()) const;

#ifdef RW_GLOBAL_ENUMS
    virtual RWCString moneyAsString(double, CurrSymbol=LOCAL) const;
#else
    virtual RWCString moneyAsString(double, RWLocale::CurrSymbol=LOCAL) const;
#endif

    virtual RWBoolean stringToNum  (const RWCString&, double*) const;
    virtual RWBoolean stringToNum  (const RWCString&, long*) const;
    virtual RWBoolean stringToDate (const RWCString&, struct tm*) const;
    virtual RWBoolean stringToTime (const RWCString&, struct tm*) const;

    virtual RWBoolean stringToMoney(const RWCString&, double*,
#ifdef RW_GLOBAL_ENUMS
                                    CurrSymbol=LOCAL) const;
#else
                                    RWLocale::CurrSymbol=LOCAL) const;
#endif

    // monthIndex() returns [1..12], 0 for error
    virtual int monthIndex(const RWCString&) const;

    // weekdayIndex() returns 1 for Monday, 7 for Sunday, 0 for error.
    virtual int weekdayIndex(const RWCString&) const;
};

// RWLocaleSnapshot is the real working locale object.  When constructed
// it copies all necessary data from the environment into itself, for use
// later by its member functions.

class RWExport RWLocaleSnapshot : public RWLocale {
 public:
    RWCString locale_name_;  // Name of the locale "snapped"

    // lconv members
    RWCString decimal_point_;
    RWCString thousands_sep_;
    RWCString grouping_;
    RWCString int_curr_symbol_;
    RWCString currency_symbol_;
    RWCString mon_decimal_point_;
    RWCString mon_thousands_sep_;
    RWCString mon_grouping_;
    RWCString positive_sign_;
    RWCString negative_sign_;
    char int_frac_digits_;
    char frac_digits_;
    char p_cs_precedes_;
    char p_sep_by_space_;
    char n_cs_precedes_;
    char n_sep_by_space_;
    char p_sign_posn_;
    char n_sign_posn_;

 private:
    RWCString weekDays_[7];	// names of days of the week
    RWCString weekAbbs_[7];	// abbreviated week days
    RWCString monthNames_[12];	// names of months
    RWCString monthAbbrs_[12];	// abbreviated months
    RWCString am_;
    RWCString pm_;
    int monthNameHasDigits_;    // aargh!

    RWCString      timeFmt_;
    RWTimeFmtList* timeFmtList_;
    RWCString      dateFmt_;
    RWTimeFmtList* dateFmtList_;
    RWCString      dateTimeFmt_;
    RWTimeFmtList* dateTimeFmtList_;
#ifndef RW_NO_STRFTIME_CAPC
    RWCString      dateLongFmt_;
    RWTimeFmtList* dateLongFmtList_;
#endif
    const char* positiveMoneyFormat_;
    const char* negativeMoneyFormat_;
    double localDivisor_;
    double intlDivisor_;

 public:
    enum RWDateOrder { DMY, MDY, YDM, YMD };
    RWDateOrder dateOrder_; 	// to help parse input

 private:
    // number output formatting
    int fmt(char** bufpp, long) const;
    int fmt(char** bufpp, unsigned long) const;
    int fmt(char** bufpp, double, int precision, int showdot,
	    const RWCString& dec_point, const RWCString& thou_sep,
	    const RWCString& grouping) const;

    // date output formatting
    int formatComponent(char*, const struct tm*, int, const RWZone&) const;
    int fillFormat(char*, const char*, RWTimeFmtList*,
                   const struct tm*, const RWZone&)           const;
    int fmt(char* buf, const struct tm*, char format, const RWZone&) const;

    // money output formatting
    size_t fmt(char** buf, double cents, const RWCString& curr_sym,
	    const char* format, const RWCString& sign, int sep_by_space,
	    int frac_digits, double divisor) const ;

 public:
    RWLocaleSnapshot(const char* localeName = 0); // if 0, snap current locale
    virtual ~RWLocaleSnapshot();

    virtual RWCString asString(long) const;
    virtual RWCString asString(unsigned long) const;
    virtual RWCString asString(
      double f, int precision = 6, RWBoolean showpoint = 0) const;

    // asString(tm*, format): format is one of [aAbBcdHIjmMpSUwWxXyYZ],
    //                        as in ANSI C Library function strftime().
    virtual RWCString asString(const struct tm* tmbuf, char format,
                               const RWZone& = RWZone::local()) const;

#ifdef RW_GLOBAL_ENUMS
    virtual RWCString moneyAsString(double, CurrSymbol=LOCAL) const;
#else
    virtual RWCString moneyAsString(double, RWLocale::CurrSymbol=LOCAL) const;
#endif

    virtual RWBoolean stringToDate (const RWCString&, struct tm*) const;
    virtual RWBoolean stringToTime (const RWCString&, struct tm*) const;
    virtual RWBoolean stringToNum  (const RWCString&, double*) const;
    virtual RWBoolean stringToNum  (const RWCString&, long*) const;

    virtual RWBoolean stringToMoney(const RWCString&, double*,
#ifdef RW_GLOBAL_ENUMS
                                    CurrSymbol=LOCAL) const;
#else
                                    RWLocale::CurrSymbol=LOCAL) const;
#endif

    // returns [1..12], 0 for error
    virtual int monthIndex(const RWCString&) const;
    // returns 1 for Monday, 7 for Sunday, 0 for error.
    virtual int weekdayIndex(const RWCString&) const;

    friend class RWLocaleStream;
  private:
    // These functions migrated to the end of the class
    // because MetaWare 3.2 beta for OS/2 will not generate
    // the vtable when they are at the top...
    RWLocaleSnapshot(RWLocaleSnapshot&);  // not defined
    void operator= (RWLocaleSnapshot&);   // not defined
};

#endif

//////////////////////////////////////////////////////////
