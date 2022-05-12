#ifndef __RWRSTREAM_H__
#define __RWRSTREAM_H__

/*
 * Includes either stream.h or iostream.h, depending
 * on the compiler.
 *
 * $Id: rstream.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rstream.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/02  18:22:13  foote
 * Port to Symantec 7.0 on Macintosh
 *
 * Revision 6.1  1994/04/15  19:10:41  vriezen
 * Move all files to 6.1
 *
 * Revision 2.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.4  1993/03/13  01:53:16  keffer
 * Moved OPEN[IO]STREAM to backward.h
 *
 * Revision 2.2  1992/11/15  22:12:09  keffer
 * Removed use of macro NL
 *
 * Revision 2.1  1992/11/04  23:03:15  myersn
 * *** empty log message ***
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.8   04 Mar 1992 10:20:22   KEFFER
 * Includes "rw/defs.h" instead of "rw/compiler.h"
 * 
 *    Rev 1.7   18 Feb 1992 09:54:36   KEFFER
 * 
 *    Rev 1.6   27 Oct 1991 17:39:06   keffer
 * Corrected Glock inclusion of iostream.hxx
 * 
 *    Rev 1.5   17 Oct 1991 09:12:56   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.3   24 Sep 1991 11:08:10   keffer
 * Zortech now includes iostream.hpp; Glock iostream.hxx.
 * 
 *    Rev 1.2   08 Sep 1991 19:10:08   keffer
 * Changed name of eatwhite() to rwEatwhite()
 * 
 *    Rev 1.1   24 Jul 1991 13:06:46   keffer
 * Added pvcs keywords
 *
 */

#include "rw/defs.h"

#if defined(__ZTC__) && !defined(THINK_CPLUS)
#  include <iostream.hpp>
#else
#  ifdef __GLOCK__
#    include <iostream.hxx>
#  else
#    include <iostream.h>
#  endif
#endif

inline istream& rwEatwhite(istream& s) {return s >> ws;}

#endif /* __RWRSTREAM_H__ */
