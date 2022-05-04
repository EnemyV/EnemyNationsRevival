/*
 * Definitions for RWCString
 *
 * $Id: cstring.cpp,v 2.20 1993/09/14 04:19:20 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1989 - 1993.
 * This software is subject to copyright protection under the laws of
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/CSTRING.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 2.20  1993/09/14  04:19:20  keffer
 * Ported to Symantec, which does not allow a register
 * value to be passed by reference.
 *
 * Revision 2.19  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 2.18  1993/09/08  23:26:40  jims
 * Add work-around for Microsoft NT tolower bug
 *
 * Revision 2.17  1993/09/08  20:20:47  randall
 * corrected retrieving of data for second argument in RWCSubString operator==
 *
 * Revision 2.16  1993/09/07  17:35:52  jims
 * Remove "::" from "::toupper" to guard against macro implementation
 *
 * Revision 2.15  1993/09/02  23:49:39  randall
 * changed ret->refs_ 's value from 1 to 0
 *
 * Revision 2.14  1993/09/01  03:40:34  myersn
 * use new static init semantics of RWReference for default RW?String ctor.
 *
 * Revision 2.13  1993/08/26  00:24:12  myersn
 * fix up hashing, optimize case-folding hash.
 *
 * Revision 2.12  1993/08/06  21:10:30  myersn
 * minor cleanup.
 *
 * Revision 2.11  1993/08/06  20:41:45  keffer
 * Assignment or construction from a nil const char* is
 * now a violated precondition.
 *
 * Revision 2.10  1993/08/06  18:17:43  keffer
 * Reinstated multibyte functions.
 *
 * Revision 2.8  1993/08/05  23:19:51  keffer
 * Changed how a null string is handled to something more DLL friendly.
 *
 * Revision 2.7  1993/08/05  11:47:26  jims
 * Distinguish between WIN16 DLLs and WIN32 DLLs by checking
 * for __WIN16__
 *
 * Revision 2.6  1993/08/04  19:56:29  keffer
 * Substrings now reference their cstring by a pointer rather than a reference
 * to work around a Symantec bug.
 *
 * Revision 2.5  1993/08/03  00:17:00  myersn
 * move multibyte functions out to cstrmb.cpp, for better linking behavior.
 *
 * Revision 2.4  1993/07/30  02:05:44  jims
 * Guard against tolower being a macro (use *p, not *p++)
 *
 * Revision 2.3  1993/07/29  01:25:28  keffer
 * Corrected starting index error in RWCString::index().
 *
 * Revision 2.2  1993/07/29  00:37:58  keffer
 * Removed constructors taking signed and unsigned char.
 *
 * Revision 2.1  1993/07/28  22:34:38  keffer
 * New architecture using variable lengthed RWCStringRef.
 *
 * Revision 1.32.1.1  1993/07/27  23:59:20  keffer
 * Now uses variable length RWCStringRef object.
 *
 * Revision 1.32  1993/07/27  02:50:56  myersn
 * implement RWCString::append(const char*) !
 *
 * Revision 1.31  1993/07/20  00:14:42  myersn
 * shuffle functions from strngcv.o to not pull in wide string support.
 *
 * Revision 1.30  1993/05/31  21:46:32  keffer
 * New messaging architecture for localization
 *
 * Revision 1.29  1993/05/26  00:09:35  keffer
 * Ported to MS under NT
 *
 * Revision 1.28  1993/05/18  00:48:57  keffer
 * Introduced new exception handling classes
 *
 * Revision 1.27  1993/05/14  21:42:14  myersn
 * allow constructor to fill with nulls.
 *
 * Revision 1.26  1993/05/13  23:53:28  myersn
 * fix some 8-bit uncleanliness and null intolerance.
 *
 * Revision 1.25  1993/04/12  16:35:16  keffer
 * Indexing now uses size_t
 *
 * Revision 1.24  1993/04/12  11:57:04  jims
 * Locking now done with RWMutex class
 *
 * Revision 1.23  1993/03/25  05:45:05  myersn
 * add embedNulls arg to ctor, append, prepend; fix toupper/tolower bug
 * for Solaris/ISO charset.
 *
 * Revision 1.21  1993/03/12  00:33:55  myersn
 * make RWCString::index() return offset from the beginning of string, not
 * from the starting index.
 *
 * Revision 1.20  1993/03/01  18:03:37  keffer
 * Added text messages to PRECONDITION clauses.
 * Comparisons to null substrings now legal.
 *
 * Revision 1.19  1993/02/23  03:15:12  myersn
 * add #endif at end of file.
 *
 * Revision 1.15  1993/02/17  03:12:30  keffer
 * Changed const notation to follow style guide
 *
 * Revision 1.14  1993/02/15  16:24:07  keffer
 * Added rwexport tag where appropriate
 *
 * Revision 1.13  1993/02/14  05:18:57  myersn
 * Make comparison operators global, make
 * comparison operators faster, and treat nulls correctly too.
 *
 * Revision 1.12  1993/02/07  23:07:30  keffer
 * Every string now contains the null string.
 *
 * Revision 1.11  1993/02/03  20:02:09  myersn
 * fix clone() for MT case: must always check result of removeReference.
 *
 * Revision 1.10  1993/01/29  22:13:44  myersn
 * fix bug in prepend, which was inserting a null at the end of the prefix portion.
 *
 * Revision 1.9  1993/01/29  20:07:31  myersn
 * make MT-safe via different interface to RWReference base class
 *
 * Revision 1.8  1993/01/25  16:23:44  keffer
 * Cast away 'constness' to satisfy Sun cfront.
 *
 * Revision 1.6  1992/12/02  00:24:14  myersn
 * fixed static initialization of the null string
 *
 * Revision 1.5  1992/12/01  22:14:50  myersn
 * changed "sensitive, insensitive" to "exact, ignoreCase" for clarity.
 *
 * Revision 1.3  1992/11/19  05:06:42  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 1.2  1992/11/16  04:37:43  keffer
 * operator()(size_t) is now inline
 *
 * Revision 1.1  1992/11/16  03:22:21  keffer
 * Initial revision
 */

#include "rw/cstring.h"
#include "rw/rwerr.h"
#include "rw/toolerr.h"
#include "defmisc.h"
STARTWRAP
#if defined(RW_NON_ANSI_HEADERS)
#  include <memory.h>		/* Looking for memcpy() */
#endif
#include <stdlib.h>
#include <ctype.h>
ENDWRAP

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: cstring.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#ifdef RW_MULTI_THREAD

# include "rw/mutex.h"
  static RWMutex rwcstringLock; 
# define MULTITHREAD_LOCK (rwcstringLock)
  RWCString::RWCString(const RWCString& S)
  { pref_ = S.pref_; pref_->addReference(rwcstringLock); }

#else 
# define MULTITHREAD_LOCK /* nothing */

#endif /* RW_MULTI_THREAD */

#ifndef RW_NO_CPP_RECURSION
# define new rwnew
#endif

#ifdef RW_TOLOWER_SIGN_EXTENDS_RESULT_BUG
#define tolower(c) ((int)(unsigned char)tolower(c))
#endif

// Amount to shift hash values to avoid clustering

const unsigned RW_HASH_SHIFT = 5;

// This is the global null string representation, shared among all
// empty strings.  The space for it is in "nullref" which the
// loader will set to zero:
static long nullref[(sizeof(RWCStringRef)+1)/sizeof(long) + 1];
static RWCStringRef* const nullStringRef = (RWCStringRef*)nullref;

/*
 * In what follows, npts_ is the length of the underlying representation
 * vector.  Hence, the capacity for a null terminated string held in this 
 * vector is npts_-1.  The variable nchars_ is the length of the held
 * string, excluding the terminating null.
 *
 * The algorithms make no assumptions about whether internal strings
 * hold embedded nulls.  However, they do assume that any string
 * passed in as an argument that does not have a length count is null
 * terminated and therefore has no embedded nulls.
 *
 * The internal string is always null terminated.
 *
 */



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWCStringRef                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


/* static */ RWCStringRef*
RWCStringRef::getRep(size_t capacity, size_t nchar)
{
  RWCStringRef* ret = (RWCStringRef*)new char[capacity + sizeof(RWCStringRef) + 1];
  ret->capacity_ = capacity;
  ret->refs_     = 0;
  (*ret)[ret->nchars_ = nchar] = 0; // Terminating null
#if defined(RWDEBUG)
  ret->ptr_ = ret->data();
#endif /* */
  return ret;
}

// Find first occurrence of a character c:
size_t
RWCStringRef::first(char c) const
{
  const char* f = strchr(data(), c);
  return f ? f - data(): RW_NPOS;
}

// Find first occurrence of a character in wchr:
size_t
RWCStringRef::first(const char* cs) const
{
  RWPRECONDITION2(cs!=rwnil, "RWCStringRef::first(const char* cs)const: nil pointer");
  const char* f = strpbrk(data(), cs);
  return f ? f - data() : RW_NPOS;
}

inline static void mash(unsigned& hash, unsigned chars)
{
  hash = (chars ^
       ((hash << RW_HASH_SHIFT) |
        (hash >> (RWBITSPERBYTE*sizeof(unsigned) - RW_HASH_SHIFT))));
}

/*
 * Return a case-sensitive hash value.
 */
unsigned
RWCStringRef::hash() const
{
  unsigned hv       = (unsigned)length(); // Mix in the string length.
  unsigned i        = length()*sizeof(char)/sizeof(unsigned);
  const unsigned* p = (const unsigned*)data();
  {
    while (i--)
      mash(hv, *p++);			// XOR in the characters.
  }
  // XOR in any remaining characters:
  if ((i = length()*sizeof(char)%sizeof(unsigned)) != 0) {
    unsigned h = 0;
    const char* c = (const char*)p;
    while (i--) 
      h = ((h << RWBITSPERBYTE*sizeof(char)) | *c++);
    mash(hv, h);
  }
  return hv;
}

/*
 * Return a case-insensitive hash value.
 */
unsigned
RWCStringRef::hashFoldCase() const
{
  unsigned hv = (unsigned)length();    // Mix in the string length.
  unsigned i  = hv;
  const unsigned char* p = (const unsigned char*) data();
  while (i--) {
    mash(hv, toupper(*p));
    ++p;
  }
  return hv;
}

// Find last occurrence of a character c
size_t
RWCStringRef::last(char c) const
{
  const char* f = strrchr(data(), c);
  return f ? f - data() : RW_NPOS;
}



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              RWCString                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


RWCString::RWCString()
{
  // make sure the null string rep has not been clobbered:
  RWASSERT(nullStringRef->capacity()==0);
  RWASSERT(nullStringRef->length()==0);
  RWASSERT((*nullStringRef)[0]=='\0');
  pref_ = nullStringRef;
  pref_->addReference(MULTITHREAD_LOCK);
}

RWCString::RWCString(RWSize_T ic)
{
  pref_ = RWCStringRef::getRep(ic.value(), 0);
}

RWCString::RWCString(const char* cs)
{
  RWPRECONDITION(cs!=rwnil);
  size_t N = strlen(cs);
  pref_ = RWCStringRef::getRep(N, N);
  memcpy(pref_->data(), cs, N);
}

RWCString::RWCString(const char* cs, size_t N)
{
  RWPRECONDITION(cs!=rwnil);
  pref_ = RWCStringRef::getRep(N, N);
  memcpy(pref_->data(), cs, N);
}

RWCString::RWCString(char c)
{
  pref_ = RWCStringRef::getRep(getInitialCapacity(), 1);
  (*pref_)[0] = c;
}

RWCString::RWCString(char c, size_t N)
{
  pref_ = RWCStringRef::getRep(N, N);
  while (N--) (*pref_)[N] = c;
}

RWCString::RWCString(const RWCSubString& substr)
{
  size_t len = substr.isNull() ? 0 : substr.length();
  pref_ = RWCStringRef::getRep(adjustCapacity(len), len);
  memcpy(pref_->data(), substr.data(), len);
}

RWCString::~RWCString()
{
  if (pref_->removeReference(MULTITHREAD_LOCK) == 0) delete pref_;
}

RWCString&
RWCString::operator=(const char* cs)
{
  RWPRECONDITION(cs!=rwnil);
  return replace(0, length(), cs, strlen(cs));
}

// Assignment:  
RWCString&
RWCString::operator=(const RWCString& str)
{
  str.pref_->addReference(MULTITHREAD_LOCK);
  if (pref_->removeReference(MULTITHREAD_LOCK) == 0) delete pref_;
  pref_ = str.pref_;
  return *this;
}


/********************** Member Functions *************************/

RWCString&
RWCString::append(char c, size_t rep)
{
  size_t tot;
  cow(tot = length() + rep);
  char* p = pref_->data() + length();
  while (rep--)
    *p++ = c;

  (*pref_)[pref_->nchars_ = tot] = '\0';

  return *this;
}

// Change the string capacity, returning the new capacity
size_t
RWCString::capacity(size_t nc)
{
  if (nc > length() && nc != capacity())
    clone(nc);

  RWPOSTCONDITION(capacity() >= length());
  return capacity();
}

// String comparisons
int
RWCString::compareTo(const char* cs2, caseCompare cmp) const
{
  RWPRECONDITION2(cs2!=rwnil, "RWCString::compareTo(const char* cs2, caseCompare cmp) const: nil pointer");
  const char* cs1 = data();
  size_t len = length();
  size_t i = 0;
  if (cmp == exact) {
    for (; cs2[i]; ++i) {
      if (i == len) return -1;
      if (cs1[i] != cs2[i]) return ((cs1[i] > cs2[i]) ? 1 : -1);
    }
  } else {                  // ignore case
    for (; cs2[i]; ++i) {
      if (i == len) return -1;
      char c1 = tolower((unsigned char)cs1[i]);
      char c2 = tolower((unsigned char)cs2[i]);
      if (c1 != c2) return ((c1 > c2)? 1 : -1);
    }
  }
  return (i < len) ? 1 : 0;
}

int
RWCString::compareTo(const RWCString& str, caseCompare cmp) const
{
  const char* s1 = data();
  const char* s2 = str.data();
  size_t len = str.length();
  if (length() < len) len = length();
  if (cmp == exact) {
    int result = memcmp(s1, s2, len);
    if (result != 0) return result;
  } else {
    size_t i = 0;
    for (; i < len; ++i) {
      char c1 = tolower((unsigned char)s1[i]);
      char c2 = tolower((unsigned char)s2[i]);
      if (c1 != c2) return ((c1 > c2)? 1 : -1);
    }
  }
  // strings are equal up to the length of the shorter one.
  if (length() == str.length()) return 0;
  return (length() > str.length())? 1 : -1;
}


RWCString
RWCString::copy() const
{
  RWCString temp(*this);	// Has increased reference count
  temp.clone();			// Distinct copy
  return temp;
}

unsigned
RWCString::hash(caseCompare cmp) const
{
  return (cmp==exact) ? pref_->hash() : pref_->hashFoldCase();
}


static int
rwMemiEqual(const char* p, const char* q, size_t N)
{
  while (N--)
  {
    if (tolower((unsigned char)*p) != tolower((unsigned char)*q))
      return FALSE;
    p++; q++;
  }
  return TRUE;
}

// Pattern Matching:
size_t
RWCString::index(const char* pattern,	// Pattern to search for
		 size_t plen,		// Length of pattern
		 size_t startIndex,	// Starting index from which to start
		 caseCompare cmp) const	// What type of case-comparison
{
  RWPRECONDITION2(pattern!=rwnil,"RWCString::index(const char*, size_t, caseCompare) const: nil pointer");
  
  size_t slen = length();
  if (slen < startIndex + plen) return RW_NPOS;
  if (plen == 0) return startIndex;
  slen -= startIndex + plen;
  const char* sp = data() + startIndex;
  if (cmp == exact) {
    char first = *pattern;
    for (size_t i = 0; i <= slen; ++i)
      if (sp[i] == first && memcmp(sp+i+1, pattern+1, plen-1) == 0)
        return i + startIndex;
  } else {
    int first = tolower((unsigned char) *pattern);
    for (size_t i = 0; i <= slen; ++i)
      if (tolower((unsigned char) sp[i]) == first &&
	  rwMemiEqual(sp+i+1, pattern+1, plen-1))
	return i + startIndex;
  }
  return RW_NPOS;

}

// Prepend characters to self:
RWCString&
RWCString::prepend(char c, size_t rep)
{
  size_t tot = length() + rep;	// Final string length

  // Check for shared representation or insufficient capacity:
  if ( pref_->references() > 1 || capacity() < tot )
  {
    RWCStringRef* temp = RWCStringRef::getRep(adjustCapacity(tot), tot);
    memcpy(temp->data()+rep, data(), length());
    if (pref_->removeReference(MULTITHREAD_LOCK) == 0) delete pref_;
    pref_ = temp;
  }
  else
  {
    memmove(pref_->data()+rep, data(), length());
    (*pref_)[pref_->nchars_ = tot] = '\0';
  }

  char* p = pref_->data();
  while (rep--)
    *p++ = c;

  return *this;
}


// Remove at most n1 characters from self beginning at pos,
// and replace them with the first n2 characters of cs.
RWCString&
RWCString::replace(size_t pos, size_t n1, const char* cs, size_t n2)
{
  n1 = rwmin(n1, length()-pos);
  if (!cs) n2 = 0;

  size_t tot = length()-n1+n2;	// Final string length
  size_t rem = length()-n1-pos;	// Length of remnant at end of string

  // Check for shared representation, insufficient capacity,
  // excess waste, or overlapping copy:
  if ( pref_->references() > 1 
       || capacity() < tot
       || capacity() - tot > getMaxWaste()
       || (cs && (cs >= data() && cs < data()+length())) )
  {
    RWCStringRef* temp = RWCStringRef::getRep(adjustCapacity(tot), tot);
    if (pos) memcpy(temp->data(), data(), pos);
    if (n2 ) memcpy(temp->data()+pos, cs, n2);
    if (rem) memcpy(temp->data()+pos+n2, data()+pos+n1, rem);
    if (pref_->removeReference(MULTITHREAD_LOCK) == 0) delete pref_;
    pref_ = temp;
  }
  else
  {
    if (rem) memmove(pref_->data()+pos+n2, data()+pos+n1, rem);
    if (n2 ) memcpy (pref_->data()+pos   , cs, n2);
    (*pref_)[pref_->nchars_ = tot] = 0;	// Add terminating null
  }

  return *this;
}


// Truncate or add blanks as necessary
void
RWCString::resize(size_t N)
{
  if (N < length())
    remove(N);			// Shrank; truncate the string
  else
    append(' ', N-length());	// Grew or stayed the same

}

// Return a substring of self stripped at beginning and/or end

RWCSubString RWCString::strip(

#ifndef RW_GLOBAL_ENUMS
  RWCString::
#endif
             stripType st,
  char c)
{
  size_t start = 0;		// Index of first character
  size_t end = length();	// One beyond last character
  const char* direct = data();	// Avoid a dereference w dumb compiler

  RWASSERT((int)st != 0);
  if (st & leading)
    while (start < end && direct[start] == c)
      ++start;
  if (st & trailing)
    while (start < end && direct[end-1] == c)
      --end;
  if (end == start) start = end = RW_NPOS;  // make the null substring
  return RWCSubString(*this, start, end-start);
}


// Change self to lower-case
void
RWCString::toLower()
{
  cow();
  register size_t N = length();
  register char* p = pref_->data();
  while ( N-- ) { *p = tolower((unsigned char)*p); p++;}
}

// Change self to upper case
void
RWCString::toUpper()
{
  cow();
  register size_t N = length();
  register char* p = pref_->data();
  while ( N-- ) { *p = toupper((unsigned char)*p); p++;}
}

char&
RWCString::operator[](size_t i)
{
   assertElement(i); 
   cow();
   return (*pref_)[i];
}

// Check to make a string index is in range
void
RWCString::assertElement(size_t i) const
{
  if ( i==RW_NPOS || i>=length() )
    RWTHROW( RWBoundsErr( RWMessage( RWTOOL_INDEX,
				     (unsigned)i,
				     (unsigned)length()-1) ) );
}

/********************** Protected functions ***********************/

// Special constructor to initialize with the concatenation of a1 and a2:
RWCString::RWCString(const char* a1, size_t N1, const char* a2, size_t N2)
{
  if (!a1) N1=0;
  if (!a2) N2=0;
  size_t tot = N1+N2;
  pref_ = RWCStringRef::getRep(adjustCapacity(tot), tot);
  memcpy(pref_->data(),    a1, N1);
  memcpy(pref_->data()+N1, a2, N2);
  RWPOSTCONDITION(length() == N1+N2);
}

// Calculate a nice capacity greater than or equal to nc
/* static */ size_t
RWCString::adjustCapacity(size_t nc)
{
  size_t ic = getInitialCapacity();
  if (nc<=ic) return ic;
  size_t rs = getResizeIncrement();
  return (nc - ic + rs - 1) / rs * rs + ic;
}

// Make self a distinct copy
void
RWCString::clone()
{
  RWCStringRef* temp = RWCStringRef::getRep(length(), length());
  memcpy(temp->data(), data(), length());
  if (pref_->removeReference(MULTITHREAD_LOCK) == 0) delete pref_;
  pref_ = temp;

  RWPOSTCONDITION(pref_->references() == 1);
}

// Make self a distinct copy with capacity nc.
void
RWCString::clone(size_t nc)
{
  RWCStringRef* temp = RWCStringRef::getRep(nc, length());
  memcpy(temp->data(), data(), length());
  if (pref_->removeReference(MULTITHREAD_LOCK) == 0) delete pref_;
  pref_ = temp;

  RWPOSTCONDITION(pref_->references() == 1);
}



/****************** Related global functions ***********************/

RWBoolean rwexport
operator==(const RWCString& s1, const char* s2)
{
  const char* data = s1.data();
  size_t len = s1.length();
  for (size_t i = 0; s2[i]; ++i)
    if (data[i] != s2[i] || i == len) return FALSE;
  return (i == len);
}

// Return a lower-case version of str:
RWCString rwexport
toLower(const RWCString& str)
{
  register size_t N = str.length();
  RWCString temp((char)0, N);
  register const char* uc = str.data();
  register       char* lc = (char*)temp.data();
  // Guard against tolower() being a macro:
  while( N-- ) { *lc++ = tolower((unsigned char)*uc); uc++; }
  return temp;
}

// Return an upper-case version of str:
RWCString rwexport
toUpper(const RWCString& str)
{
  register size_t N = str.length();
  RWCString temp((char)0, N);
  register const char* uc = str.data();
  register       char* lc = (char*)temp.data();
  // Guard against toupper() being a macro:
  while( N-- ) { *lc++ = toupper((unsigned char)*uc); uc++; }
  return temp;
}

RWCString rwexport
operator+(const RWCString& s, const char* cs)
{
  // Use the special concatenation constructor:
  return RWCString(s.data(), s.length(), cs, strlen(cs));
}            

RWCString rwexport
operator+(const char* cs, const RWCString& s)
{
  // Use the special concatenation constructor:
  return RWCString(cs, strlen(cs), s.data(), s.length());
}

RWCString rwexport
operator+(const RWCString& s1, const RWCString& s2)
{
  // Use the special concatenation constructor:
  return RWCString(s1.data(), s1.length(), s2.data(), s2.length());
}

/******************** Static Member Functions **********************/

#if !defined(_RWTOOLSDLL) || !defined(__WIN16__)

/*
 * Code for non 16-bit Windows DLL versions of the libary.  Otherwise,
 * the instance data must be retrieved from the instance manager.
 */

// Static member variable initialization:
size_t		RWCString::initialCapac     = 15;
size_t		RWCString::resizeInc        = 16;
size_t		RWCString::freeboard        = 15;

size_t
RWCString::initialCapacity(size_t ic)
{
  size_t ret = initialCapac;
  initialCapac = ic;
  return ret;
}

size_t
RWCString::resizeIncrement(size_t ri)
{
  size_t ret = resizeInc;
  resizeInc = ri;
  return ret;
}

size_t
RWCString::maxWaste(size_t mw)
{  
  size_t ret = freeboard;
  freeboard = mw;
  return ret;
}

#endif


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWCSubString                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
 * A zero lengthed substring is legal.  It can start
 * at any character.  It is considered to be "pointing"
 * to just before the character.
 *
 * A "null" substring is a zero lengthed substring that
 * starts with the nonsense index RW_NPOS.  It can
 * be detected with the member function isNull().
 */

// Private constructor 
RWCSubString::RWCSubString(const RWCString & str, size_t start, size_t nextent)
: str_((RWCString*)&str),
  begin_(start),
  extent_(nextent)
{
#ifdef RWDEBUG
  size_t N = str.length();

  // Allow zero lengthed and null substrings:
  if ((start==RW_NPOS && nextent!=0) || (start!=RW_NPOS && start+nextent>N))
    subStringError(N, start, nextent);
#endif
}

// Sub-string operator
RWCSubString 
RWCString::operator()(size_t start, size_t len) 
{
  return RWCSubString(*this, start, len);
}

/*
 * Returns a substring matching "pattern", or the null substring 
 * if there is no such match.  It would be nice if this could be yet another 
 * overloaded version of operator(), but this would result in a type
 * conversion ambiguity with operator(size_t, size_t). 
 */
RWCSubString
RWCString::subString(const char* pattern, size_t startIndex)
{
  RWPRECONDITION2(pattern!=rwnil,"RWCString::subString(const char*, size_t): null pointer");
  size_t i = index(pattern, startIndex);
  return RWCSubString(*this, i, i == RW_NPOS ? 0 : strlen(pattern));
}
char&
RWCSubString::operator[](size_t i)
{
   assertElement(i); 
   str_->cow();
   return (*str_->pref_)[begin_+i];
}

char&
RWCSubString::operator()(size_t i)
{ 
#ifdef RWBOUNDS_CHECK    
   assertElement(i);
#endif
   str_->cow();
   return (*str_->pref_)[begin_+i];
}

#ifndef RW_NO_CONST_OVERLOADS
const RWCSubString
RWCString::operator()(size_t start, size_t len) const
{
  return RWCSubString(*this, start, len);
}

const RWCSubString
RWCString::subString(const char* pattern, size_t startIndex) const
{
  size_t i = index(pattern, startIndex);
  return RWCSubString(*this, i, i == RW_NPOS ? 0 : strlen(pattern));
}
#endif

RWCSubString&
RWCSubString::operator=(const RWCString& str) 
{
  if( !isNull() )
    str_->replace(begin_, extent_, str.data(), str.length());

  return *this;
}

RWCSubString&
RWCSubString::operator=(const char* cs)
{
  if (!isNull() )
    str_->replace(begin_, extent_, cs, strlen(cs));

  return *this;
}

RWBoolean rwexport
operator==(const RWCSubString& ss, const char* cs)
{
  RWPRECONDITION2(cs!=rwnil,"::operator==(const RWCSubString& ss, const char* cs): nil pointer");

  if ( ss.isNull() ) return *cs =='\0'; // Two null strings compare equal

  RWASSERT(ss.begin_+ss.extent_<=ss.str_->length());

  const char* data = ss.str_->data() + ss.begin_;
  for (size_t i = 0; cs[i]; ++i)
    if (cs[i] != data[i] || i == ss.extent_) return FALSE;
  return (i == ss.extent_);
}

RWBoolean rwexport
operator==(const RWCSubString& ss, const RWCString& s)
{
  if (ss.isNull()) return s.isNull(); // Two null strings compare equal.
  if (ss.extent_ != s.length()) return FALSE;
  return !memcmp(ss.str_->data() + ss.begin_, s.data(), ss.extent_);
}

RWBoolean rwexport
operator==(const RWCSubString& s1, const RWCSubString& s2)
{
  if (s1.isNull()) return s2.isNull();
  if (s1.extent_ != s2.extent_) return FALSE;
  return !memcmp(s1.str_->data()+s1.begin_, s2.str_->data()+s2.begin_, s1.extent_);
}

// Convert self to lower-case
void
RWCSubString::toLower()
{
  if(!isNull())
  {				// Ignore null substrings
     str_->cow();
     register char* p = (char*)(str_->data() + begin_); // Cast away constness
     size_t N = extent_;
     while( N-- ) { *p = tolower((unsigned char)*p); p++;}
  }
}

// Convert self to upper-case
void
RWCSubString::toUpper()
{
  if(!isNull())
  {				// Ignore null substrings
     str_->cow();
     register char* p = (char*)(str_->data() + begin_); // Cast away constness
     size_t N = extent_;
     while( N-- ) { *p = toupper((unsigned char)*p); p++;}
  }
}

void
RWCSubString::subStringError(size_t sr, size_t start, size_t n) const
{
  RWTHROW(RWBoundsErr(RWMessage(RWTOOL_SUBSTRING,
				(unsigned)start,
				(unsigned)n,
				(unsigned)sr) ) );
}

void
RWCSubString::assertElement(size_t i) const
{
  if (i==RW_NPOS || i>=length())
    RWTHROW(RWBoundsErr(RWMessage(RWTOOL_INDEX,
				  (unsigned)i,
				  (unsigned)length()) ) );
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                           LOCALE RELATED                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

RWBoolean
RWCString::isAscii() const
{
  const char* cp = data();
  for (size_t i = 0; i < length(); ++i)
    if (cp[i] & ~0x7F)
      return FALSE;
  return TRUE;
}

#ifndef RW_NO_LOCALE

// String collation
int
RWCStringRef::collate(const char* cstr) const
{
  // Cast to "char*" necessary for Sun cfront:
  return ::strcoll(data(), (char*)cstr);
}

RWCString rwexport
strXForm(const RWCString& cstr)
{
  // Get the size required to transform the string;
  // cast to "char*" necessary for Sun cfront:
  size_t N = ::strxfrm(NULL, (char*)cstr.data(), 0);

  RWCString temp((char)0, N);

  // Return null string in case of failure:
  if ( ::strxfrm((char*)temp.data(), (char*)cstr.data(), N) >= N )
    return RWCString();

  return temp;
}

size_t
RWCString::mbLength() const 
{
  const char* cp = data();
  size_t i = 0;
  size_t len = 0;
  mblen((const char*)0, MB_CUR_MAX);  // clear static state (bleah!)
  while (i < length() && cp[i]) {
    int l = mblen(cp+i, MB_CUR_MAX);
    if (l <= 0) return RW_NPOS;
    i += l;
    ++len;
  }
  if (i > length()) return RW_NPOS; // incomplete last char
  return len;
}

#endif
