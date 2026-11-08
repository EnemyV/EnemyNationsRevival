#ifndef __RWWSTRING_H__
#define __RWWSTRING_H__

/*
 * Declarations for RWWString --- wide character strings.
 *
 * $Id: wstring.h,v 6.9 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: wstring.h,v $
 * Revision 6.9  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.8  1994/06/23  19:07:33  vriezen
 * Bug# 277. Added signed and unsigned char ctors.  Not clear
 * if simply char and wchar_t ctors would produce desired
 * ambiguity for numeric arguments of other types, which were
 * probably meant for the RWSize_T ctor.
 *
 * Revision 6.7  1994/06/23  01:32:31  myersn
 * fix up ctors from wchar_t and char.
 *
 * Revision 6.6  1994/06/22  21:37:24  vriezen
 * Added private, non implemented RWWString(char) ctor to
 * force ambiguity for implicit conversions on numeric types.
 *
 * Revision 6.5  1994/06/21  03:09:04  myersn
 * make RWWString(wchar_t) ctor private/undefined to help catch errors.
 *
 * Revision 6.4  1994/06/17  07:31:08  jims
 * Change RWWString::pref_ to data_ for easier debugging
 *
 * Revision 6.3  1994/06/15  01:24:47  myersn
 * eliminate constructor from wchar_t.
 *
 * Revision 6.2  1994/05/16  18:09:28  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:48:53  vriezen
 * Move all files to 6.1
 *
 * Revision 2.26  1994/04/06  23:27:10  vriezen
 * Added caseCompare to subString function.
 *
 * Revision 2.25  1994/04/01  21:33:30  vriezen
 * Use RW_INLINE_FRIEND symbol for inline friends
 *
 * Revision 2.24  1994/01/04  21:01:48  jims
 * ObjectStore version: add #include <ostore/ostore.hh>
 *
 * Revision 2.23  1993/11/22  11:44:17  jims
 * Rename unlink to "unLink" to avoid #defines in some of the DOS/WIN compilers
 *
 * Revision 2.22  1993/11/16  07:26:53  myersn
 * fix use of RW_NO_CONST_OVERLOAD.
 *
 * Revision 2.21  1993/11/15  00:38:56  keffer
 * Corrected declaration for getRep() in ObjectStore version
 *
 * Revision 2.20  1993/11/15  00:37:38  keffer
 * Introduced member function clobber()
 *
 * Revision 2.19  1993/11/14  22:19:43  keffer
 * Introduced m.f. unlink()
 *
 * Revision 2.18  1993/11/13  22:54:35  keffer
 * Added const version of strip()
 *
 * Revision 2.17  1993/11/09  20:57:29  griswolf
 * add inline declaration for op==, for those compilers that want it
 *
 * Revision 2.16  1993/11/08  21:44:56  jims
 * Port to ObjectStore
 *
 * Revision 2.15  1993/11/02  01:16:11  keffer
 * Added missing default values to readToDelim() and strip()
 *
 * Revision 2.14  1993/09/14  00:05:27  randall
 * removed mention of RWWiden and RWWidenAscii
 *
 * Revision 2.13  1993/09/12  21:10:43  keffer
 * Now no longer includes wcsutil.h, greatly speeding compilation
 * under Win32.
 *
 * Revision 2.12  1993/09/12  18:52:49  keffer
 * Added comment about RWWiden interface being obsolete.
 *
 * Revision 2.11  1993/09/09  02:51:34  keffer
 * Added constructors allowing MB to wide character conversion
 * of RWCStrings.
 *
 * Revision 2.10  1993/09/03  02:08:13  keffer
 * Macro _CLASSDLL is now named _RWTOOLSDLL
 *
 * Revision 2.9  1993/09/01  03:37:38  myersn
 * remove dependency on RWMemoryPool.
 *
 * Revision 2.8  1993/08/26  00:25:15  myersn
 * replace RW?StringRef::hashCase() with hash() and hashFoldCase().
 *
 * Revision 2.7  1993/08/21  21:09:16  keffer
 * Added conversion constructors taking enum multiByte_ and ascii_.
 * Deprecated old "RWWiden" interface.
 *
 * Revision 2.6  1993/08/06  20:40:40  keffer
 * Removed default argument from from 4-argument member index().
 * Removed private function initNull().
 *
 * Revision 2.4  1993/08/05  11:49:12  jims
 * Distinguish between using a WIN16 DLL from a WIN32 DLL by
 * checking for __WIN16__
 *
 * Revision 2.3  1993/08/04  19:57:12  keffer
 * Substrings now reference their cstring by a pointer rather than a reference
 * to work around a Symantec bug.
 *
 * Revision 2.2  1993/07/30  20:52:57  randall
 * added inline definition for index(RWWString&,size_t,size_t, caseCompare)
 *
 * Revision 2.1  1993/07/29  04:07:44  keffer
 * New architecture using variable lengthed RWWStringRef.
 *
 * Revision 1.37  1993/07/19  20:48:50  keffer
 * Added RWExport keyword to friend declaration in RWWiden definition.
 *
 * Revision 1.36  1993/06/13  21:39:29  jims
 * Include wcsutil.h if we must provide wide-char string functions
 *
 * Revision 1.35  1993/05/25  18:36:14  keffer
 * Added "skipWhite" flag to RWWStringRef::readToDelim()
 *
 * Revision 1.34  1993/05/15  03:50:34  myersn
 * fix += and embedded nulls.
 *
 * Revision 1.33  1993/05/14  21:39:10  myersn
 * fix append() and prepend() for strings with embedded nulls.
 *
 * Revision 1.32  1993/05/14  00:18:27  myersn
 * replace RWWString(..., widenFrom) constructors with
 * RWWString(const RWWiden&) and RWWString(const RWWidenAscii&) ctors,
 * and declare RWWiden and RWWidenAscii conversion types.  Also make
 * isNull() tolerate embedded nulls.
 *
 * Revision 1.31  1993/05/01  18:23:39  keffer
 * No longer includes "rw/wchar.h".
 *
 * Revision 1.30  1993/04/12  16:35:34  keffer
 * Added Log keyword; added rwexport keyword to operator==() globals.
 *
 * Revision 1.21  1993/02/18  17:50:39  keffer
 * Improved Rabin-Karp algorithm.
 *
 * Revision 1.20  1993/02/17  04:42:40  myersn
 * eliminate bogus RWWidenAscii class, add enum widenFrom arg to RWWString
 * constructor, and change names of members ascii(), multiByte() to
 * toAscii() and toMultiByte().
 *
 * Revision 1.19  1993/02/17  03:10:23  keffer
 * Changed const notation to follow style guide
 *
 * Revision 1.18  1993/02/15  23:33:31  myersn
 * remove references to RWCString member functions, for include cleanliness.
 *
 * Revision 1.17  1993/02/15  02:47:08  myersn
 * replaced RWMBString with RWWidenAscii, & corresponding RWWString ctor.
 *
 * Revision 1.16  1993/02/14  05:25:40  myersn
 * Made comparison operators global.
 *
 * Revision 1.15  1993/02/06  02:58:39  myersn
 * simplify ascii-widening constructors, move implementation to strngcv.cpp.
 *
 * Revision 1.14  1993/02/05  23:18:23  myersn
 * delete widen() function -- RWWString constructor is sufficient.
 *
 * Revision 1.13  1993/02/04  01:12:28  myersn
 * add new RWMBString class for mbs->ws conversion constructor.
 *
 * Revision 1.12  1993/02/03  00:19:32  jims
 * Removed #include directive for procinit.h
 *
 * Revision 1.11  1993/01/29  20:26:17  myersn
 * add MT-safe reference-counting.
 *
 * Revision 1.10  1993/01/28  21:53:14  myersn
 * add RWCStringRef::readFile() member for new RWCString::readFile() semantics.
 *
 * Revision 1.9  1993/01/28  19:57:55  myersn
 * remove references to RWWRegexp
 *
 * Revision 1.8  1993/01/28  01:53:04  myersn
 * derive from RWMemoryPool via the macro RWMemoryPool_OPTION for MT-safety
 *
 * Revision 1.7  1993/01/27  21:15:13  myersn
 * add multibyte i/o functions readToDelim etc.
 *
 * Revision 1.5  1992/12/01  22:15:40  myersn
 * change sensitive, insensitive to exact, ignoreCase for clarity.
 *
 * Revision 1.4  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 1.3  1992/11/17  21:37:05  keffer
 * Put in wide-character string persistence.
 *
 * Revision 1.2  1992/11/16  04:37:31  keffer
 * operator()(unsigned) is now inline
 *
 * Revision 1.1  1992/11/16  04:05:01  keffer
 * Initial revision
 *
 * 
 */

#ifndef __RWTOOLDEFS_H__
# include "rw/tooldefs.h"
#endif
#ifndef __RWREF_H__
# include "rw/ref.h"
#endif

STARTWRAP
#include <string.h>
ENDWRAP

class RWExport RWWString;
class RWExport RWWSubString;
class RWExport RWCString;

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWWStringRef                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * This is the dynamically allocated part of a RWWString.
 * It maintains a reference count.
 * There are no public member functions.
 */

class RWExport RWWStringRef : public RWReference
{

  static RWWStringRef*	getRep(size_t capac, size_t nchar);
  void		unLink();	// Disconnect from a stringref, maybe delete it

  size_t	length   () const {return nchars_;}
  size_t	capacity () const {return capacity_;}
  wchar_t*	data     () const {return (wchar_t*)(this+1);}

  wchar_t&	operator[](size_t i)       {return ((wchar_t*)(this+1))[i];}
  wchar_t	operator[](size_t i) const {return ((wchar_t*)(this+1))[i];}

  int		collate(const wchar_t*) const;
  size_t	first    (wchar_t       ) const;
  size_t	first    (const wchar_t*) const;
  unsigned	hash     (              ) const;
  unsigned	hashFoldCase (          ) const;
  size_t	last     (wchar_t       ) const;

  size_t	capacity_;	// Max string length (excluding null)
  size_t	nchars_;	// String length (excluding terminating null)

friend class RWExport RWWString;
friend class RWExport RWWSubString;
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWWSubString                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * The RWWSubString class allows selected elements to be addressed.
 * There are no public constructors.
 */

class RWExport RWWSubString
{
public:
  RWWSubString(const RWWSubString& SP)
    : str_(SP.str_), begin_(SP.begin_), extent_(SP.extent_) {;}

  RWWSubString&	operator=(const wchar_t*);	// Assignment to wchar_t*
  RWWSubString&	operator=(const RWWString&);	// Assignment to RWWString
  wchar_t&  	operator()(size_t i);		// Index with optional bounds checking
  wchar_t&  	operator[](size_t i);		// Index with bounds checking
#ifndef RW_NO_CONST_OVERLOAD
  wchar_t  	operator()(size_t i) const;	// Index with optional bounds checking
  wchar_t  	operator[](size_t i) const;	// Index with bounds checking
#endif
  const wchar_t*data() const;
  size_t	length() const		{return extent_;}
  size_t	start() const		{return begin_;}
  void		toLower();		// Convert self to lower-case
  void		toUpper();		// Convert self to upper-case

  // For detecting null substrings:
  RWBoolean	isNull() const		{return begin_==RW_NPOS;}
  int		operator!() const	{return begin_==RW_NPOS;}

protected:

  void		subStringError(size_t, size_t, size_t) const;
  void		assertElement(size_t i) const;	// Verifies i is valid index

private:

  // NB: the only constructor is private:
  RWWSubString(const RWWString & s, size_t start, size_t len);

  RWWString*  	str_;		// Referenced string
  size_t	begin_;		// Index of starting wchar_tacter
  size_t	extent_;	// Length of RWWSubString

friend rwexport 
RWBoolean operator==(const RWWSubString& s1, const RWWSubString& s2);
friend rwexport 
RWBoolean operator==(const RWWSubString& s1, const RWWString& s2);
friend rwexport
RWBoolean operator==(const RWWSubString& s1, const wchar_t* s2);
friend class RWExport RWWString;
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              RWWString                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


class RWExport RWWString
{

public:

  enum stripType   {leading = 0x1, trailing = 0x2, both = 0x3};
  enum caseCompare {exact, ignoreCase};
  enum multiByte_  {multiByte};	// Convert from multibyte
  enum ascii_      {ascii};	// Convert from ASCII

  RWWString();			// Null string
  RWWString(RWSize_T ic);	// Suggested capacity
  RWWString(const RWWString& S)	// Copy constructor
#ifndef RW_MULTI_THREAD
  { data_ = S.data_; pref()->addReference(); }
#else
  ;
#endif
  RWWString(const wchar_t * a);			// Copy to embedded null
  RWWString(const wchar_t * a, size_t N);	// Copy past any embedded nulls
  RWWString(wchar_t);
  RWWString(char c) {initChar(c);}              // widen a char
#ifndef RW_NO_OVERLOAD_UCHAR
  RWWString(unsigned char c) {initChar(char(c));}     // widen a char
#endif
#ifndef RW_NO_OVERLOAD_SCHAR
  RWWString(signed char c) {initChar(char(c));}       // widen a char
#endif
  RWWString(wchar_t, size_t N);
  
  RWWString(const RWWSubString& SS);

  // Constructors used for MB to wide character conversions:
  RWWString(const char*, multiByte_);		// Convert from multibyte
  RWWString(const char*, ascii_    );		// Convert from ASCII
  RWWString(const char*, size_t N, multiByte_);	// Convert N characters from MB
  RWWString(const char*, size_t N, ascii_    );	// Convert N characters from ASCII
  RWWString(const RWCString&, multiByte_);	// Convert from multibyte
  RWWString(const RWCString&, ascii_    );	// Convert from ASCII

  ~RWWString();

  // Type conversion:
#ifndef RW_ZTC_TYPE_CONVERSION_BUG  
   		operator const wchar_t*() const {return data_;}
#endif

  // Assignment:
  RWWString&	operator=(const wchar_t*);	// Replace string
  RWWString&	operator=(const RWWString&);	// Replace string
  RWWString&	operator+=(const wchar_t*);	// Append string.
  RWWString&	operator+=(const RWWString& s);


  // Indexing operators:
  wchar_t&	operator[](size_t);		// Indexing with bounds checking
  wchar_t&	operator()(size_t);		// Indexing with optional bounds checking
  RWWSubString	operator()(size_t start, size_t len);	// Sub-string operator
  RWWSubString	subString(const wchar_t* pat, size_t start=0, caseCompare=exact);		
#ifndef RW_NO_CONST_OVERLOAD
  wchar_t		operator[](size_t) const;
  wchar_t		operator()(size_t) const;
  const RWWSubString	operator()(size_t start, size_t len) const;
  const RWWSubString	subString(const wchar_t* pat, size_t start=0, caseCompare=exact) const;
  const RWWSubString	strip(stripType s=trailing, wchar_t c=(wchar_t)' ') const;
#endif
  
      // Non-static member functions:
  RWWString&	append(const wchar_t* cs);
  RWWString&	append(const wchar_t* cs, size_t N);
  RWWString&	append(const RWWString& s);
  RWWString&	append(const RWWString& s, size_t N);
  RWWString&	append(wchar_t c, size_t rep=1);	// Append c rep times
  RWspace	binaryStoreSize() const		{return length()*sizeof(wchar_t)+sizeof(size_t);}
  size_t	capacity() const		{return pref()->capacity();}
  size_t	capacity(size_t N);
  int		collate(const wchar_t* cs) const {return pref()->collate(cs);}
  int		collate(const RWWString& st) const;
  int		compareTo(const wchar_t* cs,   caseCompare cmp = exact) const;
  int		compareTo(const RWWString& st, caseCompare cmp = exact) const;
  RWBoolean	contains(const wchar_t* pat,   caseCompare cmp = exact) const;
  RWBoolean	contains(const RWWString& pat, caseCompare cmp = exact) const;
  RWWString    	copy() const;
  const wchar_t* data() const {return data_;}
  size_t	first(wchar_t c) const			{return pref()->first(c);}
  size_t	first(const wchar_t* cs) const		{return pref()->first(cs);}
  unsigned	hash(caseCompare cmp = exact) const;
  size_t	index(const wchar_t* pat, size_t i=0, caseCompare cmp = exact) const;
  size_t	index(const RWWString& s, size_t i=0, caseCompare cmp = exact) const;
  size_t	index(const wchar_t* pat, size_t patlen, size_t i,
		      caseCompare cmp) const;
  size_t	index(const RWWString& s, size_t patlen, size_t i,
		      caseCompare cmp) const;
  RWWString&	insert(size_t pos, const wchar_t*);
  RWWString&	insert(size_t pos, const wchar_t*, size_t extent);
  RWWString&	insert(size_t pos, const RWWString&);
  RWWString&	insert(size_t pos, const RWWString&, size_t extent);
  RWBoolean	isAscii() const;
  RWBoolean	isNull() const			{return pref()->nchars_ == 0;}
  size_t	last(wchar_t c) const		{return pref()->last(c);}
  size_t  	length() const			{return pref()->nchars_;}

  RWWString&	prepend(const wchar_t*);			// Prepend a wchar_tacter string
  RWWString&	prepend(const wchar_t* cs, size_t N);
  RWWString&	prepend(const RWWString& s);
  RWWString&	prepend(const RWWString& s, size_t N);
  RWWString&	prepend(wchar_t c, size_t rep=1);	// Prepend c rep times
  istream&	readFile(istream&);			// Read to EOF or null wchar_tacter.
  istream&	readLine(istream&,
          	         RWBoolean skipWhite = TRUE);	// Read to EOF or newline.
  istream&	readString(istream&);			// Read to EOF or null wchar_tacter.
  istream&	readToDelim(istream&, wchar_t delim=(wchar_t)'\n');	// Read to EOF or delimiter.
  istream&	readToken(istream&);			// Read separated by white space.
  RWWString&	remove(size_t pos);			// Remove pos to end of string
  RWWString&	remove(size_t pos, size_t n);		// Remove n wchar_t's starting at pos
  RWWString&	replace(size_t pos, size_t n, const wchar_t*);
  RWWString&	replace(size_t pos, size_t n, const wchar_t*, size_t);
  RWWString&	replace(size_t pos, size_t n, const RWWString&);
  RWWString&	replace(size_t pos, size_t n, const RWWString&, size_t);
  void		resize(size_t N);	 		// Truncate or add blanks as necessary.
  void		restoreFrom(RWvistream&);		// Restore from ASCII store
  void		restoreFrom(RWFile&);			// Restore string
  void		saveOn(RWvostream& s) const;
  void		saveOn(RWFile& f) const;
  RWWSubString	strip(stripType s=trailing, wchar_t c=(wchar_t)' ');
  RWCString     toAscii() const;			// strip high bytes
  RWCString     toMultiByte() const;			// use wcstombs()
  void		toLower();				// Change self to lower-case
  void		toUpper();				// Change self to upper-case

  // Static member functions:
  static size_t		initialCapacity(size_t ic = 15);	// Initial allocation Capacity
  static size_t		maxWaste(size_t mw = 15);		// Max empty space before reclaim
  static size_t		resizeIncrement(size_t ri = 16);	// Resizing increment

#if defined(_RWTOOLSDLL) && defined(__WIN16__)
  // Just declarations --- static data must be retrieved from the instance manager.
  static size_t		getInitialCapacity();
  static size_t		getResizeIncrement();
  static size_t		getMaxWaste();
#else
  static size_t		getInitialCapacity()	{return initialCapac;}
  static size_t		getResizeIncrement()	{return resizeInc;}
  static size_t		getMaxWaste()		{return freeboard;}
#endif

protected:

  // Special concatenation constructor:
  RWWString(const wchar_t* a1, size_t N1, const wchar_t* a2, size_t N2);
  void			assertElement(size_t) const;	// Index in range
  void			clobber(size_t nc);		// Remove old contents
  void			cow();				// Do copy on write as needed
  void			cow(size_t nc);			// Do copy on write as needed
  istream&		readToDelim(istream&, wchar_t delim, RWBoolean skipWhite);
  static size_t		adjustCapacity(size_t nc);
  void			initMB(const char*, size_t); // Initialize from multibyte
  void                  initChar(char);              // Init from char

private:
  void			clone();          // Make self a distinct copy
  void			clone(size_t nc); // Make self a distinct copy w. capacity nc

#if !defined(_RWTOOLSDLL) || !defined(__WIN16__)
  /* If not compiling for shared address space, then use static data */
  static size_t	initialCapac;		// Initial allocation Capacity
  static size_t	resizeInc;		// Resizing increment
  static size_t	freeboard;		// Max empty space before reclaim
#endif

  RWWStringRef*	pref() const    { return ((RWWStringRef*)data_) - 1; }
  wchar_t*      data_;		// ref. counted data (RWWStringRef is in front)

friend rwexport RWWString operator+(const RWWString& s1, const RWWString& s2);
friend rwexport RWWString operator+(const RWWString& s,  const wchar_t* cs);
friend rwexport RWWString operator+(const wchar_t* cs, const RWWString& s);
RW_INLINE_FRIEND rwexport RWBoolean operator==(const RWWString& s1, const RWWString& s2);
friend rwexport RWBoolean operator==(const RWWString& s1, const wchar_t* s2);
friend class RWExport RWWSubString;
friend class RWExport RWWStringRef;

};

// Related global functions:
#ifndef RW_TRAILING_RWEXPORT
rwexport istream&  operator>>(istream& str   ,       RWWString& wcstr);
rwexport ostream&  operator<<(ostream& str   , const RWWString& wcstr);
#else
istream& rwexport  operator>>(istream& str   ,       RWWString& wcstr);
ostream& rwexport  operator<<(ostream& str   , const RWWString& wcstr);
#endif

inline RWvistream& operator>>(RWvistream& str,       RWWString& wcstr)
		{ wcstr.restoreFrom(str);  return str; }
inline RWFile&     operator>>(RWFile& file,          RWWString& wcstr)
		{ wcstr.restoreFrom(file); return file; }
inline RWvostream& operator<<(RWvostream& str, const RWWString& wcstr)
		{ wcstr.saveOn(str);       return str; }
inline RWFile&     operator<<(RWFile& file,    const RWWString& wcstr)
		{ wcstr.saveOn(file);      return file; }

RWWString rwexport toLower(const RWWString&); // Ret lower-case version of arg
RWWString rwexport toUpper(const RWWString&); // Ret upper-case version of arg
inline    unsigned rwhash(const RWWString& s) { return s.hash(); }
inline    unsigned rwhash(const RWWString* s) { return s->hash(); }
#ifndef RW_NO_WCSXFRM
RWWString rwexport strXForm(const RWWString&);	// wsxfrm() interface
#endif /* RW_NO_WCSXFRM */


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                               Inlines                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

inline void RWWString::cow()
{ if (pref()->references() > 1) clone(); }

inline void RWWString::cow(size_t nc)
{ if (pref()->references() > 1  || capacity() < nc) clone(nc); }

inline RWWString& RWWString::append(const wchar_t* cs, size_t N)
{ return replace(length(), 0, cs, N); }

inline RWWString& RWWString::append(const RWWString& s)
{ return replace(length(), 0, s.data(), s.length()); }

inline RWWString& RWWString::append(const RWWString& s, size_t N)
{ return replace(length(), 0, s.data(), rwmin(N, s.length())); }

inline RWWString& RWWString::operator+=(const RWWString& s)
{ return append(s.data(),s.length()); }

inline int RWWString::collate(const RWWString& st) const
{ return pref()->collate(st.data()); }

inline RWBoolean RWWString::contains(const RWWString& pat, caseCompare cmp) const
{ return index(pat.data(), pat.length(), (size_t)0, cmp) != RW_NPOS; }

inline size_t RWWString::index(const RWWString& s, size_t i, caseCompare cmp) const
{ return index(s.data(), s.length(), i, cmp); }

inline size_t RWWString::index(const RWWString& pat, size_t plen, size_t i, caseCompare cmp) const
{ return index(pat.data(), plen, i, cmp); }

inline RWWString& RWWString::insert(size_t pos, const wchar_t* cs, size_t N)
{ return replace(pos, 0, cs, N); }

inline RWWString& RWWString::insert(size_t pos, const RWWString& wcstr)
{ return replace(pos, 0, wcstr.data(), wcstr.length()); }

inline RWWString& RWWString::insert(size_t pos, const RWWString& wcstr, size_t N)
{ return replace(pos, 0, wcstr.data(), rwmin(N, wcstr.length())); }

inline RWWString& RWWString::prepend(const wchar_t* cs, size_t N)
{ return replace(0, 0, cs, N); }

inline RWWString& RWWString::prepend(const RWWString& s)
{ return replace(0, 0, s.data(), s.length()); }

inline RWWString& RWWString::prepend(const RWWString& s, size_t N)
{ return replace(0, 0, s.data(), rwmin(N, s.length())); }

inline RWWString& RWWString::remove(size_t pos)
{ return replace(pos, length()-pos, rwnil, 0); }

inline RWWString& RWWString::remove(size_t pos, size_t n)
{ return replace(pos, n, rwnil, 0); }

inline RWWString& RWWString::replace(size_t pos, size_t n, const RWWString& wcstr)
{ return replace(pos, n, wcstr.data(), wcstr.length()); }

inline RWWString& RWWString::replace(size_t pos, size_t n1, const RWWString& wcstr, size_t n2)
{ return replace(pos, n1, wcstr.data(), rwmin(wcstr.length(),n2)); }

inline wchar_t& RWWString::operator()(size_t i)
{ 
#ifdef RWBOUNDS_CHECK
  assertElement(i); 
#endif
  cow();
  return data_[i];
}

#ifndef RW_NO_CONST_OVERLOAD
inline wchar_t RWWString::operator[](size_t i) const
{ assertElement(i); return data_[i]; }

inline wchar_t RWWString::operator()(size_t i) const
{ 
#ifdef RWBOUNDS_CHECK    
  assertElement(i); 
#endif
  return data_[i];
}
#endif

inline const wchar_t* RWWSubString::data() const
{ return str_->data() + begin_; }

// Access to elements of sub-string with bounds checking
#ifndef RW_NO_CONST_OVERLOAD
inline wchar_t RWWSubString::operator[](size_t i) const
{ assertElement(i); return str_->data_[begin_+i]; }

inline wchar_t RWWSubString::operator()(size_t i) const
{ 
#ifdef RWBOUNDS_CHECK    
   assertElement(i);
#endif
   return str_->data_[begin_+i];
}
#endif

// String Logical operators:
inline RWBoolean	operator==(const RWWString& s1, const RWWString& s2)
				  { return ((s1.length() == s2.length()) &&
				    !memcmp(s1.data(), s2.data(), s1.length()*sizeof(wchar_t))); }
inline RWBoolean	operator!=(const RWWString& s1, const RWWString& s2)
				  { return !(s1 == s2); }
inline RWBoolean	operator< (const RWWString& s1, const RWWString& s2)
				  { return s1.compareTo(s2)< 0;}
inline RWBoolean	operator> (const RWWString& s1, const RWWString& s2)
				  { return s1.compareTo(s2)> 0;}
inline RWBoolean	operator<=(const RWWString& s1, const RWWString& s2)
				  { return s1.compareTo(s2)<=0;}
inline RWBoolean	operator>=(const RWWString& s1, const RWWString& s2)
				  { return s1.compareTo(s2)>=0;}

//     RWBoolean	operator==(const RWWString& s1, const wchar_t* s2);
inline RWBoolean	operator!=(const RWWString& s1, const wchar_t* s2)
				  { return !(s1 == s2); }
inline RWBoolean	operator< (const RWWString& s1, const wchar_t* s2)
				  { return s1.compareTo(s2)< 0; }
inline RWBoolean	operator> (const RWWString& s1, const wchar_t* s2)
				  { return s1.compareTo(s2)> 0; }
inline RWBoolean	operator<=(const RWWString& s1, const wchar_t* s2)
                                  { return s1.compareTo(s2)<=0; }
inline RWBoolean	operator>=(const RWWString& s1, const wchar_t* s2)
				  { return s1.compareTo(s2)>=0; }

inline RWBoolean	operator==(const wchar_t* s1, const RWWString& s2)
				  { return (s2 == s1); }
inline RWBoolean	operator!=(const wchar_t* s1, const RWWString& s2)
				  { return !(s2 == s1); }
inline RWBoolean	operator< (const wchar_t* s1, const RWWString& s2)
				  { return s2.compareTo(s1)> 0; }
inline RWBoolean	operator> (const wchar_t* s1, const RWWString& s2)
				  { return s2.compareTo(s1)< 0; }
inline RWBoolean	operator<=(const wchar_t* s1, const RWWString& s2)
                                  { return s2.compareTo(s1)>=0; }
inline RWBoolean	operator>=(const wchar_t* s1, const RWWString& s2)
				  { return s2.compareTo(s1)<=0; }

// SubString Logical operators:
//     RWBoolean operator==(const RWWSubString& s1, const RWWSubString& s2);
//     RWBoolean operator==(const RWWSubString& s1, const wchar_t* s2);
//     RWBoolean operator==(const RWWSubString& s1, const RWWString& s2);
inline RWBoolean operator==(const RWWString& s1,    const RWWSubString& s2)
			   { return (s2 == s1); }
inline RWBoolean operator==(const wchar_t* s1,      const RWWSubString& s2)
			   { return (s2 == s1); }
inline RWBoolean operator!=(const RWWSubString& s1, const wchar_t* s2)
			   { return !(s1 == s2); }
inline RWBoolean operator!=(const RWWSubString& s1, const RWWString& s2)
			   { return !(s1 == s2); }
inline RWBoolean operator!=(const RWWSubString& s1, const RWWSubString& s2)
			   { return !(s1 == s2); }
inline RWBoolean operator!=(const RWWString& s1,    const RWWSubString& s2)
			   { return !(s2 == s1); }
inline RWBoolean operator!=(const wchar_t* s1,      const RWWSubString& s2)
			   { return !(s2 == s1); }

#endif /* __RWWSTRING_H__ */

