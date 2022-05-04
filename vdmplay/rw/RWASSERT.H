#ifndef __RWASSERT_H__
#define __RWASSERT_H__

/*
 * rwassert macro definition for testing purposes.
 *
 * $Id: rwassert.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwassert.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:42  vriezen
 * Move all files to 6.1
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/08/18  21:46:37  jims
 * Add macro to allow writing to cout instead of cerr
 *
 * Revision 1.4  1993/08/13  16:09:57  randall
 * moved '\' from back to front of print statement
 *
 * Revision 1.3  1993/02/07  00:27:28  keffer
 * Now includes rwAssertTest() directly in the header file as a static function.
 *
 * Revision 1.2  1993/02/06  21:34:47  keffer
 * Changed macro from ABORT_ON_FAIL to RW_ABORT_ON_FAIL
 *
 * Revision 1.1  1993/01/29  22:49:14  randall
 * Initial revision
 *
 */

#include "rw/rstream.h"

STARTWRAP
#include <assert.h>
ENDWRAP

/* Macro definition of rwassert(), which is used for testing.
 * If RW_ABORT_ON_FAIL is defined on the command line then the
 * system defined assert() is used which terminates the test 
 * program. Otherwise, RWAssertTest is used, which just reports
 * the error and keeps on going.
 *
 * Assertion errors are written to standard error (cerr) by default.
 * If RW_ASSERT_TO_COUT is defined, the errors will be written to
 * standard output (cout) instead.
 */
#ifdef RW_ABORT_ON_FAIL

# define rwassert(a) assert(a)

#else

# define rwassert(a) rwAssertTest(a, __FILE__, __LINE__)
 
  static int
  rwAssertTest(int result, const char* file, int line)
  {
    if (!result)
    {
#ifdef RW_ASSERT_TO_COUT
      cout
#else
      cerr
#endif
           << "\n**** Assert failure in file: " << file
           << "; line: " << line << " ****" << flush;
    }
    return result;
  }

#endif


#endif  /* RWASSERT_H__  */
