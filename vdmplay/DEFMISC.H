#ifndef __RWDEFMISC_H__
#define __RWDEFMISC_H__

/*
 * Internal declarations used by Tools.h++
 *
 * $Header: /vdmplay/DEFMISC.H 1     4/02/96 13:46 Vadim $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 * Voice: (503) 754-3010	FAX: (503) 757-6650
 *
 * Copyright (C) 1989, 1990, 1991, 1992, 1993.
 * This software is subject to copyright protection under the laws of 
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/DEFMISC.H $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 1.15  1993/07/29  06:07:39  myersn
 * tolerate nulls & 8-bit chars better, also escape sequences.
 *
 * Revision 1.14  1993/05/06  02:40:20  myersn
 * change match() and makepat to rwmatchs(), rwmakepat().
 *
 * Revision 1.13  1993/05/01  18:22:47  keffer
 * Wide string stuff moved to rwwchar.h
 *
 * Revision 1.12  1993/03/17  20:38:33  keffer
 * Now include <rw/tooldefs.h>.
 * Declares RWPatternType depending on Tools.h++ version number.
 *
 * Revision 1.10  1993/02/18  23:32:11  keffer
 * Now includes <rw/defs.h>
 *
 * Revision 1.9  1993/02/18  20:52:30  myersn
 * clean up
 *
 * Revision 1.7  1993/02/06  03:51:17  keffer
 * Changed name from _defs.h to defmisc.h
 *
 * Revision 1.6  1993/01/29  20:07:31  myersn
 * change regexp pattern string type, to allow 8-bit clean regexp
 *
 * Revision 1.5  1993/01/28  01:37:33  keffer
 * Wide character support only for V6.
 *
 * Revision 1.4  1993/01/25  18:22:22  keffer
 * Changed macro RW_NO_WCHAR_T to RW_NO_WSTR.
 *
 * Revision 1.3  1992/11/19  05:07:39  keffer
 * Introduced new <rw/compiler.h> macro directives.
 *
 * Revision 1.2  1992/11/16  04:11:45  keffer
 * Added declarations for regular expression routines.
 *
 * Revision 1.1  1992/11/05  23:32:55  keffer
 * Initial revision
 *
 * 
 */

#include "rw/tooldefs.h"

// Zortech chose to call their case-insensitive search "strcmpl"
#ifdef __ZTC__
#  define stricmp(a,b) strcmpl(a,b)
#endif

#ifdef RW_NO_MEMMOVE
  // Overlapping copy:
  extern "C" void* rwmemmove(void* dest, const void* src, size_t n);
# define memmove(dest, src, n) rwmemmove(dest, src, n)
#endif

#ifdef RW_NO_STRICMP
  // Case-insensitive comparison:
  extern "C" int rwstricmp(const char* s1, const char* s2);
# define stricmp(s1, s2) rwstricmp(s1, s2)
#else
#  ifdef __GNUC__
     extern "C" int stricmp(const char* s1, const char* s2);
#  endif
#endif

#ifdef RW_NO_STRNICMP
  // Case-insensitive limited comparison:
  extern "C" int rwstrnicmp(const char* s1, const char* s2, size_t N);
# define strnicmp(s1, s2, N) rwstrnicmp(s1, s2, N)
#else
#  ifdef __GNUC__
     extern "C" int strnicmp(const char* s1, const char* s2, size_t N);
#  endif
#endif

#if defined(ultrix)	/* Ultrix does not define these in <ctype.h> */
  extern "C" { int toupper(int); int tolower(int); }
#endif


/*
 * This union is an array of masks used to pick off N chars of an unsigned 
 * int.  For example, suppose that sizeof(unsigned) is 4 (a typical Unix 
 * machine).  Then, if you wanted say, the first 3 bytes of the unsigned 
 * int, it could be retrieved using:
 *
 *	unsigned i;
 *	RWCharMask mask;
 *	i & mask.in[3];
 *
 * This allows many byte-wise operations (such as hashing, or comparing byte 
 * vectors) to be done using the machine's natural int size.  If the number 
 * of chars doesn't divide evenly into an int then this mask can be used to 
 * isolate the remaining bytes.  
 */

union RWCharMask {
  unsigned	in[sizeof(unsigned)];
  char		ch[sizeof(unsigned)*sizeof(unsigned)];
  RWCharMask();
};

// For regular expressions:

#if defined(RWTOOLS) && (RWTOOLS >= 0x0600)
  typedef unsigned short RWPatternType;
#else
  typedef unsigned char  RWPatternType;
#endif

int         rwmakepat(const char*, RWPatternType*, int);
const char* rwmatchs(const char*, size_t len, const RWPatternType*,
		     const char**);

#endif
