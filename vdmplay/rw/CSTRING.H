#ifndef __RWCSTRING_H__
#define __RWCSTRING_H__

/*
 * Declarations for RWCString --- C strings
 *
 * $Id: cstring.h,v 6.9 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: cstring.h,v $
 * Revision 6.9  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.8  1994/06/23  19:06:13  vriezen
 * Bug# 277. Change three variants of char ctor to inline calls to
 * initChar.
 *
 * Revision 6.7  1994/06/22  21:36:10  vriezen
 * Added #ifndef's and RWCString(signed char) ctor
 *
 * Revision 6.6  1994/06/22  08:49:38  myersn
 * make RWCString(char) public again, add RWCString(unsigned char) so
 * that improper conversions will be ambiguous.
 *
 * Revision 6.5  1994/06/21  03:05:10  myersn
 * Make RWCString(char) ctor private/undefined, to help catch errors.
 *
 * Revision 6.4  1994/06/17  07:30:41  jims
 * Change RWCString::pref_ to data_ for easier debugging
 *
 * Revision 6.3  1994/06/15  01:22:05  myersn
 * Eliminate constructor from char.
 *
 * Revision 6.2  1994/05/16  18:00:43  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  18:41:43  vriezen
 * Move all files to 6.1
 *
 * Revision 3.23  1994/04/01  22:24:45  vriezen
 * Added caseCompare parm to RWCString::subString function.
 *
 * Revision 3.22  1994/04/01  21:33:30  vriezen
 * Use RW_INLINE_FRIEND symbol for inline friends
 *
 * Revision 3.21  1993/11/22  11:44:17  jims
 * Rename unlink to "unLink" to avoid #defines in some of the DOS/WIN compilers
 *
 * Revision 3.20  1993/11/16  07:22:08  myersn
 * fix uses of RW_NO_CONST_OVERLOAD[S]
 *
 * Revision 3.19  1993/11/14  22:01:20  keffer
 * Introduced clobber()
 *
 * Revision 3.18  1993/11/14  19:39:22  keffer
 * Introduced m.f. unlink() to delete RWCStringRef
 *
 * Revision 3.17  1993/11/13  22:54:35  keffer
 * Added const version of strip()
 *
 * Revision 3.16  1993/11/08  08:23:48  jims
 * Port to ObjectStore
 *
 * Revision 3.15  1993/11/02  01:16:11  keffer
 * Added missing default values to readToDelim() and strip()
 *
 * Revision 3.13  1993/10/22  23:43:34  griswolf
 * add inline declaration for op==, for those compilers that want it
 *
 * Revision 3.12  1993/09/03  02:08:13  keffer
 * Macro _CLASSDLL is now named _RWTOOLSDLL
 *
 * Revision 3.11  1993/09/01  03:37:38  myersn
 * remove dependency on RWMemoryPool.
 *
 * Revision 3.10  1993/08/26  00:25:15  myersn
 * replace RW?StringRef::hashCase() with hash() and hashFoldCase().
 *
 * Revision 3.9  1993/08/06  20:41:03  keffer
 * Removed private function initNull().
 *
 * Revision 3.8  1993/08/06  20:24:29  myersn
 * remove default argument from from 4-argument member index().
 *
 * Revision 3.6  1993/08/05  11:49:12  jims
 * Distinguish between using a WIN16 DLL from a WIN32 DLL by
 * checking for __WIN16__
 *
 * Revision 3.5  1993/08/04  19:57:12  keffer
 * Substrings now reference their cstring by a pointer rather than a reference
 * to work around a Symantec bug.
 *
 * Revision 3.4  1993/08/03  00:01:32  myersn
 * guard RWCString::mbLength() against RW_NO_LOCALE.
 *
 * Revision 3.3  1993/07/30  21:04:00  randall
 * added inline definition for index(RWCString&,size_t,size_t,caseCompare)
 *
 * Revision 3.2  1993/07/29  00:38:20  keffer
 * Removed constructors taking signed and unsigned char.
 *
 * Revision 3.1  1993/07/28  22:33:27  keffer
 * New architecture using variable lengthed RWCStringRef.
 *
 * Revision 2.32.1.1  1993/07/27  23:58:31  keffer
 * Now uses variable length RWCStringRef object.
 *
 * Revision 2.32  1993/05/15  03:50:34  myersn
 * fix += and embedded nulls.
 *
 * Revision 2.31  1993/05/14  21:38:14  myersn
 * fix append() and prepend() for strings with embedded nulls.
 *
 * Revision 2.30  1993/05/14  00:16:20  myersn
 * fix null-intolerance in member isNull().
 *
 * Revision 2.29  1993/04/13  20:41:03  myersn
 * add RWCString::isAscii().
 *
 * Revision 2.28  1993/04/12  16:21:25  keffer
 * Added Log keyword
 *
 * Revision 2.17  1993/02/17  03:10:23  keffer
 * Changed const notation to follow style guide
 *
 * Revision 2.16  1993/02/17  00:39:55  myersn
 * add RWCString::mbLength().
 *
 * Revision 2.15  1993/02/14  05:25:40  myersn
 * Made comparison operators global.
 *
 * Revision 2.14  1993/02/03  00:19:09  jims
 * Removed #include directive for procinit.h
 *
 * Revision 2.13  1993/01/29  20:26:17  myersn
 * add MT-safe reference-counting.
 *
 * Revision 2.12  1993/01/28  21:53:14  myersn
 * add RWCStringRef::readFile() member for new RWCString::readFile() semantics.
 *
 * Revision 2.11  1993/01/28  01:53:04  myersn
 * derive from RWMemoryPool via the macro RWMemoryPool_OPTION for MT-safety
 *
 * Revision 2.10  1993/01/26  22:28:19  myersn
 * remove widen() member
 *
 * Revision 2.9  1993/01/26  21:34:43  myersn
 * add skipWhite default arg for RWCString::readLine
 *
 * Revision 2.7  1992/12/01  22:15:40  myersn
 * change sensitive, insensitive to exact, ignoreCase for clarity.
 *
 * Revision 2.6  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.4  1992/11/16  04:37:31  keffer
 * operator()(size_t) is now inline
 *
 * Revision 2.3  1992/11/16  04:06:09  keffer
 * Now analogous to wide-character strings.
 *
 * 
 */

#include "rw/tooldefs.h"
#include "rw/ref.h"
STARTWRAP
#include <string.h>
ENDWRAP


class RWExport RWCRegexp;
class RWExport RWCString;
class RWExport RWCSubString;


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWCStringRef                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * This is the dynamically allocated part of a RWCString.
 * It maintains a reference count.
 * There are no public member functions.
 */

class RWExport RWCStringRef : public RWReference
{

  static RWCStringRef*	getRep(size_t capac, size_t nchar);
  void		unLink(); // disconnect from a stringref, maybe delete it

  size_t	length   () const {return nchars_;}
  size_t	capacity () const {return capacity_;}
  char*		data     () const {return (char*)(this+1);}

  char&		operator[](size_t i)       {return ((char*)(this+1))[i];}
  char		operator[](size_t i) const {return ((char*)(this+1))[i];}

  size_t	first    (char       ) const;
  size_t	first    (const char*) const;
  unsigned	hash     (           ) const;
  unsigned	hashFoldCase (       ) const;
  size_t	last     (char       ) const;

#ifndef RW_NO_LOCALE
  int		collate(const char*) const;
#endif

  size_t	capacity_;	// Max string length (excluding null)
  size_t	nchars_;	// String length (excluding terminating null)

friend class RWExport RWCString;
friend class RWExport RWCSubString;
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWCSubString                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * The RWCSubString class allows selected elements to be addressed.
 * There are no public constructors.
 */

class RWExport RWCSubString
{
public:
  RWCSubString(const RWCSubString& SP)
    : str_(SP.str_), begin_(SP.begin_), extent_(SP.extent_) {;}

  RWCSubString&	operator=(const char*);		// Assignment to char*
  RWCSubString&	operator=(const RWCString&);	// Assignment to RWCString
  char&  	operator()(size_t i);		// Index with optional bounds checking
  char&  	operator[](size_t i);		// Index with bounds checking
#ifndef RW_NO_CONST_OVERLOAD
  char  	operator()(size_t i) const;	// Index with optional bounds checking
  char  	operator[](size_t i) const;	// Index with bounds checking
#endif
  const char*	data() const;
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
  RWCSubString(const RWCString & s, size_t start, size_t len);

  RWCString*  	str_;		// Referenced string
  size_t	begin_;		// Index of starting character
  size_t	extent_;	// Length of RWCSubString

friend rwexport 
RWBoolean operator==(const RWCSubString& s1, const RWCSubString& s2);
friend rwexport 
RWBoolean operator==(const RWCSubString& s1, const RWCString& s2);
friend rwexport 
RWBoolean operator==(const RWCSubString& s1, const char* s2);

friend class RWExport RWCString;
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              RWCString                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


class RWExport RWCString
{
public:

  enum stripType {leading = 0x1, trailing = 0x2, both = 0x3};
  enum caseCompare {exact, ignoreCase};

  RWCString();			// Null string
  RWCString(RWSize_T ic);	// Suggested capacity
  RWCString(const RWCString& S)	// Copy constructor
#ifndef RW_MULTI_THREAD
  { data_ = S.data_; pref()->addReference(); }
#else
  ;
#endif

  RWCString(const char * a);		// Copy to embedded null
  RWCString(const char * a, size_t N);	// Copy past any embedded nulls
  RWCString(char c) {initChar(c);} 

#ifndef RW_NO_OVERLOAD_UCHAR
  RWCString(unsigned char c) {initChar(char(c));}
#endif
#ifndef RW_NO_OVERLOAD_SCHAR
  RWCString(signed char c) {initChar(char(c));} 
#endif

  RWCString(char, size_t N);
  
  RWCString(const RWCSubString& SS);

  ~RWCString();

  // Type conversion:
#ifndef RW_ZTC_TYPE_CONVERSION_BUG  
   		operator const char*() const {return data_;}
#endif

  // Assignment:
  RWCString&	operator=(const char*);		// Replace string
  RWCString&	operator=(const RWCString&);	// Replace string
  RWCString&	operator+=(const char*);	// Append string.
  RWCString&	operator+=(const RWCString& s);


  // Indexing operators:
  char&		operator[](size_t);		// Indexing with bounds checking
  char&		operator()(size_t);		// Indexing with optional bounds checking
  RWCSubString	operator()(size_t start, size_t len);		// Sub-string operator
  RWCSubString	operator()(const RWCRegexp& re);		// Match the RE
  RWCSubString	operator()(const RWCRegexp& re, size_t start);	// Match the RE
  RWCSubString	subString(const char* pat, size_t start=0, caseCompare=exact);		
#ifndef RW_NO_CONST_OVERLOAD
  char		operator[](size_t) const;
  char		operator()(size_t) const;
  const RWCSubString	operator()(size_t start, size_t len) const;
  const RWCSubString	operator()(const RWCRegexp& pat) const;	                // Match the RE
  const RWCSubString	operator()(const RWCRegexp& pat, size_t start) const;	// Match the RE
  const RWCSubString	subString(const char* pat, size_t start=0, caseCompare=exact) const;   
  const RWCSubString	strip(stripType s=trailing, char c=' ') const;
#endif
  
      // Non-static member functions:
  RWCString&	append(const char* cs);
  RWCString&	append(const char* cs, size_t N);
  RWCString&	append(const RWCString& s);
  RWCString&	append(const RWCString& s, size_t N);
  RWCString&	append(char c, size_t rep=1);	// Append c rep times
  RWspace	binaryStoreSize() const		{return length()+sizeof(size_t);}
  size_t	capacity() const		{return pref()->capacity();}
  size_t	capacity(size_t N);
#ifndef RW_NO_LOCALE
  int		collate(const char* cs) const	{return pref()->collate(cs);}
  int		collate(const RWCString& st) const;
#endif
  int		compareTo(const char* cs,      caseCompare cmp = exact) const;
  int		compareTo(const RWCString& st, caseCompare cmp = exact) const;
  RWBoolean	contains(const char* pat,      caseCompare cmp = exact) const;
  RWBoolean	contains(const RWCString& pat, caseCompare cmp = exact) const;
  RWCString    	copy() const;
  const char*  	data() const {return data_;}
  size_t	first(char c) const		{return pref()->first(c);}
  size_t	first(const char* cs) const	{return pref()->first(cs);}
  unsigned	hash(caseCompare cmp = exact) const;
  size_t	index(const char* pat, size_t i=0, caseCompare cmp = exact)
		      const;
  size_t	index(const RWCString& s, size_t i=0, caseCompare cmp = exact)
		      const;
  size_t	index(const char* pat, size_t patlen, size_t i,
		      caseCompare cmp) const;
  size_t	index(const RWCString& s, size_t patlen, size_t i,
		      caseCompare cmp) const;
  size_t	index(const RWCRegexp& pat, size_t i=0) const;
  size_t	index(const RWCRegexp& pat, size_t* ext, size_t i=0) const;
  RWCString&	insert(size_t pos, const char*);
  RWCString&	insert(size_t pos, const char*, size_t extent);
  RWCString&	insert(size_t pos, const RWCString&);
  RWCString&	insert(size_t pos, const RWCString&, size_t extent);
  RWBoolean	isAscii() const;
  RWBoolean	isNull() const			{return pref()->nchars_ == 0;}
  size_t	last(char c) const		{return pref()->last(c);}
  size_t  	length() const			{return pref()->nchars_;}
#ifndef RW_NO_LOCALE
  size_t	mbLength() const;	// multibyte length, or RW_NPOS on error
#endif
  RWCString&	prepend(const char*);			// Prepend a character string
  RWCString&	prepend(const char* cs, size_t N);
  RWCString&	prepend(const RWCString& s);
  RWCString&	prepend(const RWCString& s, size_t N);
  RWCString&	prepend(char c, size_t rep=1);	// Prepend c rep times
  istream&	readFile(istream&);			// Read to EOF or null character.
  istream&	readLine(istream&,
          	         RWBoolean skipWhite = TRUE);	// Read to EOF or newline.
  istream&	readString(istream&);			// Read to EOF or null character.
  istream&	readToDelim(istream&, char delim='\n');	// Read to EOF or delimitor.
  istream&	readToken(istream&);			// Read separated by white space.
  RWCString&	remove(size_t pos);			// Remove pos to end of string
  RWCString&	remove(size_t pos, size_t n);		// Remove n chars starting at pos
  RWCString&	replace(size_t pos, size_t n, const char*);
  RWCString&	replace(size_t pos, size_t n, const char*, size_t);
  RWCString&	replace(size_t pos, size_t n, const RWCString&);
  RWCString&	replace(size_t pos, size_t n, const RWCString&, size_t);
  void		resize(size_t N);	 		// Truncate or add blanks as necessary.
  void		restoreFrom(RWvistream&);		// Restore from ASCII store
  void		restoreFrom(RWFile&);			// Restore string
  void		saveOn(RWvostream& s) const;
  void		saveOn(RWFile& f) const;
  RWCSubString	strip(stripType s=trailing, char c=' ');
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
  RWCString(const char* a1, size_t N1, const char* a2, size_t N2);
  void			assertElement(size_t) const;	// Index in range
  void			clobber(size_t nc);		// Remove old contents
  void			cow();				// Do copy on write as needed
  void			cow(size_t nc);			// Do copy on write as needed
  static size_t		adjustCapacity(size_t nc);
  void                  initChar(char);                 // Initialize from char
private:

  void			clone();          // Make self a distinct copy
  void			clone(size_t nc); // Make self a distinct copy w. capacity nc

#if !defined(_RWTOOLSDLL) || !defined(__WIN16__)
  /* If not compiling for a shared address space, then use static data */
  static size_t	initialCapac;		// Initial allocation Capacity
  static size_t	resizeInc;		// Resizing increment
  static size_t	freeboard;		// Max empty space before reclaim
#endif

  RWCStringRef* pref() const { return (((RWCStringRef*) data_) - 1); }
  char*		data_;		// ref. counted data (RWCStringRef is in front)

friend rwexport RWCString operator+(const RWCString& s1, const RWCString& s2);
friend rwexport RWCString operator+(const RWCString& s,  const char* cs);
friend rwexport RWCString operator+(const char* cs, const RWCString& s);
RW_INLINE_FRIEND rwexport 
RWBoolean operator==(const RWCString& s1, const RWCString& s2);
friend rwexport RWBoolean operator==(const RWCString& s1, const char* s2);
friend class RWExport RWCSubString;
friend class RWExport RWCStringRef;

};

// Related global functions:
#ifndef RW_TRAILING_RWEXPORT
rwexport istream&  operator>>(istream& str   ,       RWCString& cstr);
rwexport ostream&  operator<<(ostream& str   , const RWCString& cstr);
#else
istream& rwexport  operator>>(istream& str   ,       RWCString& cstr);
ostream& rwexport  operator<<(ostream& str   , const RWCString& cstr);
#endif

inline RWvistream& operator>>(RWvistream& str,       RWCString& cstr)
		{ cstr.restoreFrom(str);  return str; }
inline RWFile&     operator>>(RWFile& file,          RWCString& cstr)
		{ cstr.restoreFrom(file); return file; }
inline RWvostream& operator<<(RWvostream& str, const RWCString& cstr)
		{ cstr.saveOn(str);       return str; }
inline RWFile&     operator<<(RWFile& file,    const RWCString& cstr)
		{ cstr.saveOn(file);      return file; }

RWCString rwexport toLower(const RWCString&);	// Return lower-case version of argument.
RWCString rwexport toUpper(const RWCString&);	// Return upper-case version of argument.
inline    unsigned rwhash(const RWCString& s) { return s.hash(); }
inline    unsigned rwhash(const RWCString* s) { return s->hash(); }
#ifndef RW_NO_LOCALE
RWCString rwexport strXForm(const RWCString&);	// strxfrm() interface
#endif


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                               Inlines                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

inline void RWCString::cow()
{ if (pref()->references() > 1) clone(); }

inline void RWCString::cow(size_t nc)
{ if (pref()->references() > 1  || capacity() < nc) clone(nc); }

inline RWCString& RWCString::append(const char* cs)
{ return replace(length(), 0, cs, strlen(cs)); }

inline RWCString& RWCString::append(const char* cs, size_t N)
{ return replace(length(), 0, cs, N); }

inline RWCString& RWCString::append(const RWCString& s)
{ return replace(length(), 0, s.data(), s.length()); }

inline RWCString& RWCString::append(const RWCString& s, size_t N)
{ return replace(length(), 0, s.data(), rwmin(N, s.length())); }

inline RWCString& RWCString::operator+=(const char* cs)
{ return append(cs, strlen(cs)); }  

inline RWCString& RWCString::operator+=(const RWCString& s)
{ return append(s.data(),s.length()); }

#ifndef RW_NO_LOCALE
inline int RWCString::collate(const RWCString& st) const
{ return pref()->collate(st.data()); }
#endif

inline RWBoolean RWCString::contains(const RWCString& pat, caseCompare cmp) const
{ return index(pat.data(), pat.length(), (size_t)0, cmp) != RW_NPOS; }

inline RWBoolean RWCString::contains(const char* s, caseCompare cmp) const
{ return index(s, strlen(s), (size_t)0, cmp) != RW_NPOS; }

inline size_t RWCString::index(const char* s, size_t i, caseCompare cmp) const
{ return index(s, strlen(s), i, cmp); }

inline size_t RWCString::index(const RWCString& s, size_t i, caseCompare cmp) const
{ return index(s.data(), s.length(), i, cmp); }

inline size_t RWCString::index(const RWCString& pat, size_t patlen, size_t i, caseCompare cmp) const
{ return index(pat.data(), patlen, i, cmp); }

inline RWCString& RWCString::insert(size_t pos, const char* cs)
{ return replace(pos, 0, cs, strlen(cs)); }

inline RWCString& RWCString::insert(size_t pos, const char* cs, size_t N)
{ return replace(pos, 0, cs, N); }

inline RWCString& RWCString::insert(size_t pos, const RWCString& cstr)
{ return replace(pos, 0, cstr.data(), cstr.length()); }

inline RWCString& RWCString::insert(size_t pos, const RWCString& cstr, size_t N)
{ return replace(pos, 0, cstr.data(), rwmin(N, cstr.length())); }

inline RWCString& RWCString::prepend(const char* cs)
{ return replace(0, 0, cs, strlen(cs)); }

inline RWCString& RWCString::prepend(const char* cs, size_t N)
{ return replace(0, 0, cs, N); }

inline RWCString& RWCString::prepend(const RWCString& s)
{ return replace(0, 0, s.data(), s.length()); }

inline RWCString& RWCString::prepend(const RWCString& s, size_t N)
{ return replace(0, 0, s.data(), rwmin(N, s.length())); }

inline RWCString& RWCString::remove(size_t pos)
{ return replace(pos, length()-pos, rwnil, 0); }

inline RWCString& RWCString::remove(size_t pos, size_t n)
{ return replace(pos, n, rwnil, 0); }

inline RWCString& RWCString::replace(size_t pos, size_t n, const char* cs)
{ return replace(pos, n, cs, strlen(cs)); }

inline RWCString& RWCString::replace(size_t pos, size_t n, const RWCString& cstr)
{ return replace(pos, n, cstr.data(), cstr.length()); }

inline RWCString& RWCString::replace(size_t pos, size_t n1, const RWCString& cstr, size_t n2)
{ return replace(pos, n1, cstr.data(), rwmin(cstr.length(),n2)); }

inline char& RWCString::operator()(size_t i)
{ 
#ifdef RWBOUNDS_CHECK
  assertElement(i); 
#endif
  cow();
  return data_[i];
}

#ifndef RW_NO_CONST_OVERLOAD
inline char RWCString::operator[](size_t i) const
{ assertElement(i); return data_[i]; }

inline char RWCString::operator()(size_t i) const
{ 
#ifdef RWBOUNDS_CHECK    
  assertElement(i); 
#endif
  return data_[i];
}
#endif

inline const char* RWCSubString::data() const
{ return str_->data() + begin_; }

// Access to elements of sub-string with bounds checking
#ifndef RW_NO_CONST_OVERLOAD
inline char RWCSubString::operator[](size_t i) const
{ assertElement(i); return str_->data_[begin_+i]; }

inline char RWCSubString::operator()(size_t i) const
{ 
#ifdef RWBOUNDS_CHECK    
   assertElement(i);
#endif
   return str_->data_[begin_+i];
}
#endif

// String Logical operators:
inline RWBoolean	operator==(const RWCString& s1, const RWCString& s2)
				  { return ((s1.length() == s2.length()) &&
				    !memcmp(s1.data(), s2.data(), s1.length())); }
inline RWBoolean	operator!=(const RWCString& s1, const RWCString& s2)
				  { return !(s1 == s2); }
inline RWBoolean	operator< (const RWCString& s1, const RWCString& s2)
				  { return s1.compareTo(s2)< 0;}
inline RWBoolean	operator> (const RWCString& s1, const RWCString& s2)
				  { return s1.compareTo(s2)> 0;}
inline RWBoolean	operator<=(const RWCString& s1, const RWCString& s2)
				  { return s1.compareTo(s2)<=0;}
inline RWBoolean	operator>=(const RWCString& s1, const RWCString& s2)
				  { return s1.compareTo(s2)>=0;}

//     RWBoolean	operator==(const RWCString& s1, const char* s2);
inline RWBoolean	operator!=(const RWCString& s1, const char* s2)
				  { return !(s1 == s2); }
inline RWBoolean	operator< (const RWCString& s1, const char* s2)
				  { return s1.compareTo(s2)< 0; }
inline RWBoolean	operator> (const RWCString& s1, const char* s2)
				  { return s1.compareTo(s2)> 0; }
inline RWBoolean	operator<=(const RWCString& s1, const char* s2)
                                  { return s1.compareTo(s2)<=0; }
inline RWBoolean	operator>=(const RWCString& s1, const char* s2)
				  { return s1.compareTo(s2)>=0; }

inline RWBoolean	operator==(const char* s1, const RWCString& s2)
				  { return (s2 == s1); }
inline RWBoolean	operator!=(const char* s1, const RWCString& s2)
				  { return !(s2 == s1); }
inline RWBoolean	operator< (const char* s1, const RWCString& s2)
				  { return s2.compareTo(s1)> 0; }
inline RWBoolean	operator> (const char* s1, const RWCString& s2)
				  { return s2.compareTo(s1)< 0; }
inline RWBoolean	operator<=(const char* s1, const RWCString& s2)
                                  { return s2.compareTo(s1)>=0; }
inline RWBoolean	operator>=(const char* s1, const RWCString& s2)
				  { return s2.compareTo(s1)<=0; }

// SubString Logical operators:
//     RWBoolean operator==(const RWCSubString& s1, const RWCSubString& s2);
//     RWBoolean operator==(const RWCSubString& s1, const char* s2);
//     RWBoolean operator==(const RWCSubString& s1, const RWCString& s2);
inline RWBoolean operator==(const RWCString& s1,    const RWCSubString& s2)
			   { return (s2 == s1); }
inline RWBoolean operator==(const char* s1,         const RWCSubString& s2)
			   { return (s2 == s1); }
inline RWBoolean operator!=(const RWCSubString& s1, const char* s2)
			   { return !(s1 == s2); }
inline RWBoolean operator!=(const RWCSubString& s1, const RWCString& s2)
			   { return !(s1 == s2); }
inline RWBoolean operator!=(const RWCSubString& s1, const RWCSubString& s2)
			   { return !(s1 == s2); }
inline RWBoolean operator!=(const RWCString& s1,    const RWCSubString& s2)
			   { return !(s2 == s1); }
inline RWBoolean operator!=(const char* s1,         const RWCSubString& s2)
			   { return !(s2 == s1); }

#endif /* __RWCSTRING_H__ */

