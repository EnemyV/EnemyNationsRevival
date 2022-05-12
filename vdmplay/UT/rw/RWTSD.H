#ifndef __RWTSD_H__
#define __RWTSD_H__

/*
 * Declarations for Task Specific Data calls to rwtsd.dll
 *
 * $Id: rwtsd.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwtsd.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:54  vriezen
 * Move all files to 6.1
 *
 * Revision 1.9  1993/11/22  17:47:04  jims
 * Restore file to sane state (same as rev 1.6)
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/08/05  11:40:24  jims
 * Remove exitProc and GlobalRelease... function
 *
 * Revision 1.4  1993/02/10  02:45:35  jims
 * Now compiles under STRICT; RWTSDKEY typedef changed to unsigned long
 *
 * Revision 1.3  1993/02/05  07:55:14  jims
 * Cosmetic changes
 *
 * Revision 1.2  1993/02/03  20:15:55  jims
 * Added test to see if header file already included.
 * Added header and log information
 *
 * 
 */
#include <windows.h>

typedef unsigned long RWTSDKEY;

RWTSDKEY FAR PASCAL 
RWGetTaskSpecificKey();

int FAR PASCAL 
RWSetTaskSpecificData(RWTSDKEY hKey, void FAR*);

void FAR* FAR PASCAL 
RWGetTaskSpecificData(RWTSDKEY hKey);

void FAR* FAR PASCAL 
RWReleaseTaskSpecificData(RWTSDKEY hKey);

#endif  /* __RWTSD_H__ */
