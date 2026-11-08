#ifndef	__RWBITVEC_H__
#define	__RWBITVEC_H__

/*
 * Ar`bit'rarily long bit vector.
 *
 * $Id: bitvec.h,v 6.3 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: bitvec.h,v $
 * Revision 6.3  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/16  17:59:52  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:09:26  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.10  1993/07/31  01:43:04  keffer
 * Added member functions firstFalse() and firstTrue().
 *
 * Revision 2.9  1993/05/18  00:44:51  keffer
 * Added rwexport qualifier to overloaded shift operators.
 *
 * Revision 2.8  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.7  1993/04/09  02:50:00  keffer
 * Added support for operator<< and >> for virtual streams and RWFile.
 *
 * Revision 2.6  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.5  1993/01/28  01:53:04  myersn
 * derive from RWMemoryPool via the macro RWMemoryPool_OPTION for MT-safety
 *
 * Revision 2.4  1993/01/27  19:18:15  keffer
 * Now use RWor, RWand, and RWxor always.
 *
 * Revision 2.3  1993/01/25  18:12:13  keffer
 * RW_NO_CONST_OVERLOADS->RW_NO_CONST_OVERLOAD
 *
 * Revision 2.1  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.6   03 May 1992 09:58:18   KEFFER
 * Added RWExport qualifier to RWBitVec.
 * 
 *    Rev 1.5   17 Mar 1992 19:21:08   KEFFER
 * Changed BOUNDS_CHECK to RWBOUNDS_CHECK
 * 
 *    Rev 1.4   04 Mar 1992 09:03:02   KEFFER
 * nil changed to rwnil
 * 
 *    Rev 1.3   18 Feb 1992 19:21:42   KEFFER
 * Added m.f. nfull()
 * 
 *    Rev 1.2   28 Oct 1991 09:08:08   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   10 Oct 1991 09:35:10   keffer
 * Exported global operators and ::sum().
 * 
 *    Rev 1.0   28 Jul 1991 08:12:16   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

/*
 * Defining the preprocessor directive RWBOUNDS_CHECK will
 * cause bounds checking on the subscripting operator.
 */

#include "rw/bitref.h"

class RWExport RWBitVec   RWMemoryPool_OPTION
{

public:

  RWBitVec() : vec_(rwnil), npts_(0) {;}
  RWBitVec(size_t N);
  RWBitVec(size_t N, RWBoolean initVal);
  RWBitVec(const RWByte*, size_t N);
  RWBitVec(const RWBitVec&);
  ~RWBitVec();

  // Assignment operators
  RWBitVec&		operator=(const RWBitVec&);
  RWBitVec&		operator=(RWBoolean);
  RWBitVec&		operator&=(const RWBitVec& V)
	{RWand(vec_, V.vec_, nbytes()); return *this;}
  RWBitVec&		operator^=(const RWBitVec& V)
	{RWxor(vec_, V.vec_, nbytes()); return *this;}
  RWBitVec&		operator|=(const RWBitVec& V)
	{RWor(vec_, V.vec_, nbytes()); return *this;}

  // Indexing operators
  RWBitRef		operator[](size_t i);	// Can be used as lvalue
  RWBitRef		operator()(size_t i);	//       ditto
#ifndef RW_NO_CONST_OVERLOAD
  RWBoolean		operator[](size_t i) const; // Can't be used as lvalue
  RWBoolean		operator()(size_t i) const; //       ditto
#endif

  // Other operators ---
  RWBoolean		operator==(const RWBitVec& u) const {return  isEqual(u);}
  RWBoolean		operator!=(const RWBitVec& u) const {return !isEqual(u);}
  RWBoolean		operator==(RWBoolean b) const;
  RWBoolean		operator!=(RWBoolean b) const {return !operator==(b);}

  friend rwexport RWBitVec 	operator!(const RWBitVec&);
  friend rwexport RWBitVec  	operator&(const RWBitVec&,const RWBitVec&);
  friend rwexport RWBitVec  	operator^(const RWBitVec&,const RWBitVec&);
  friend rwexport RWBitVec  	operator|(const RWBitVec&,const RWBitVec&);
#ifndef RW_TRAILING_RWEXPORT
  friend rwexport ostream& 	operator<<(ostream&, const RWBitVec&);
  friend rwexport istream& 	operator>>(istream&, RWBitVec&);
#else
  friend ostream& rwexport 	operator<<(ostream&, const RWBitVec&);
  friend istream& rwexport 	operator>>(istream&, RWBitVec&);
#endif

  // Member functions
  void			clearBit(size_t i);		// Clear bit i
  const RWByte*		data() const	{ return vec_; }
  size_t		firstFalse() const;		// Find first OFF bit
  size_t		firstTrue() const;		// Find first ON  bit
  unsigned		hash() const;
  RWBoolean		isEqual(const RWBitVec&) const;
  size_t		length() const	{return npts_;}
  ostream&		printOn(ostream&) const;
  void			restoreFrom(RWvistream&);
  void			restoreFrom(RWFile&);
  void			resize(size_t);
  void			saveOn(RWvostream&) const;
  void			saveOn(RWFile&) const;
  istream&		scanFrom(istream&);
  void			setBit(size_t i);			// Set bit i
  RWBoolean		testBit(size_t i) const;		// Return value of bit i

  // Other friend functions:
  friend size_t rwexport	sum(const RWBitVec&);		// Total # of bits set

private:

  void			indexRangeErr(size_t) const;
  void			lengthErr(size_t, size_t) const;
  size_t		nbytes() const	{ return (npts_+7) >> 3; }
  size_t		nfull()  const  { return npts_ >> 3; }

  RWByte*		vec_;		// The bit data, packed within a byte vector
  size_t		npts_;		// Length of vector in bits

};

inline RWvistream& operator>>(RWvistream& str,       RWBitVec& vec)
  { vec.restoreFrom(str);  return str; }
inline RWFile&     operator>>(RWFile& file,          RWBitVec& vec)
  { vec.restoreFrom(file); return file; }
inline RWvostream& operator<<(RWvostream& str, const RWBitVec& vec)
  { vec.saveOn(str);       return str; }
inline RWFile&     operator<<(RWFile& file,    const RWBitVec& vec)
  { vec.saveOn(file);      return file; }

inline size_t RWBitVec::firstFalse() const
  { return rwFirstFalse(vec_, npts_); }
inline size_t RWBitVec::firstTrue() const
  { return rwFirstTrue(vec_, npts_); }

// This macro isolates bit 'i'
#define RWBIT(i) (*(((i)>>3) + (vec_)) & (1<<(7&(i))))

inline RWBitRef
RWBitVec::operator[](size_t i)
{
  if (i >= npts_) indexRangeErr(i);
  return RWBitRef(vec_,i);
}
	
inline RWBitRef
RWBitVec::operator()(size_t i)
{
#ifdef RWBOUNDS_CHECK
  if (i >= npts_) indexRangeErr(i);
#endif
  return RWBitRef(vec_,i);
}

#ifndef RW_NO_CONST_OVERLOAD
	
inline RWBoolean
RWBitVec::operator[](size_t i) const
{
  if (i >= npts_) indexRangeErr(i);
  return RWBIT(i) != 0;
}
	
inline RWBoolean
RWBitVec::operator()(size_t i) const
{
#ifdef RWBOUNDS_CHECK
  if (i >= npts_) indexRangeErr(i);
#endif
  return RWBIT(i) != 0;
}

#endif	/* RW_NO_CONST_OVERLOAD */

inline void
RWBitVec::clearBit(size_t i)
{ (*(vec_ + (i>>3))) &= ~(1<<(7&i)); }

inline void
RWBitVec::setBit(size_t i)
{ (*(vec_ + (i>>3))) |= (1<<(7&i)); }

inline RWBoolean
RWBitVec::testBit(size_t i) const
{ return RWBIT(i) != 0; }

#endif /* __RWBITVEC_H__ */

