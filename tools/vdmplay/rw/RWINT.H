#ifndef __RWINT_H__
#define __RWINT_H__

/*
 * Declarations for an integer class.
 *
 * $Id: rwint.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwint.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:48  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1993/09/10  02:03:24  keffer
 * Added explicit copy constructors and assignment operators to
 * support g++ which otherwise generates non-optimized versions.
 *
 * Revision 2.6  1993/06/06  00:35:01  keffer
 * operator<<(ostream&, const RWInteger&) is no longer a friend.
 *
 * Revision 2.5  1993/04/09  02:50:00  keffer
 * Added support for operator<< and >> for virtual streams and RWFile.
 *
 * Revision 2.4  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.3  1993/03/13  05:32:27  keffer
 * ZTC_TYPE_CONVERSION_BUG->RW_ZTC_TYPE_CONVERSION_BUG
 *
 *    Rev 1.1   28 Oct 1991 09:08:20   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:16:28   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/tooldefs.h"
#include "rw/rstream.h"

class RWExport RWInteger{
  int 			intBase;
public:
  RWInteger(int i=0) 	{intBase = i;}

  // The following two should not be necessary, but are required to
  // support feeble compilers:
  RWInteger(const RWInteger& i) : intBase(i.intBase) {;}
  RWInteger& operator=(const RWInteger& i) {intBase=i.intBase; return *this;}

  RWBoolean		operator==(const RWInteger& i) const {return intBase==i.intBase;}

#ifndef RW_ZTC_TYPE_CONVERSION_BUG
  operator		int() const {return intBase;}
#endif
  RWspace		binaryStoreSize() const		{return sizeof(int);}
  void			restoreFrom(RWvistream& s);
  void			restoreFrom(RWFile& f);
  void			saveOn(RWvostream& s) const;
  void			saveOn(RWFile& f) const;

  int			value() const {return intBase;}
  int			value(int newval) {int temp=intBase; intBase=newval; return temp;}

friend istream& operator>>(istream& i, RWInteger& x)
		{ return i >> x.intBase;}
};

inline ostream& operator<<(ostream& o, const RWInteger& x)
		{ return o << x.value(); }
inline RWvistream& operator>>(RWvistream& str,       RWInteger& x)
		{ x.restoreFrom(str);  return str;  }
inline RWFile&     operator>>(RWFile& file,          RWInteger& x)
		{ x.restoreFrom(file); return file; }
inline RWvostream& operator<<(RWvostream& str, const RWInteger& x)
		{ x.saveOn(str);       return str;  }
inline RWFile&     operator<<(RWFile& file,    const RWInteger& x)
		{ x.saveOn(file);      return file; }

#endif /* __RWINT_H__ */
