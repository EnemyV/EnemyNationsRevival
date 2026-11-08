#ifndef __RWTOOLERR_H__
#define __RWTOOLERR_H__ 1

/*
 * Error messages for Tools.h++
 *
 * $Id: toolerr.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
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
 * $Log: toolerr.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/14  16:42:18  jims
 * Now #defines RWTOOL_XXX to be a function for all builds
 *
 * Revision 6.1  1994/04/15  19:11:16  vriezen
 * Move all files to 6.1
 *
 * Revision 2.4  1994/03/02  19:19:33  jims
 * New message INDEXERR shows invalid index and number of
 * items in collection
 *
 * Revision 2.3  1993/10/27  19:10:54  jims
 * For DLLs, RWTOOL_XXX are exported functions, not objects
 *
 * Revision 2.2  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.1  1993/05/18  00:40:48  keffer
 * Rewrote to use new messaging and exception handling facility.
 *
 */

#include "rw/message.h"

/*
 * RWMsgId is a typedef for RWCatMsg*.
 * For each RWMsgId of the form RWTOOL_XXX their corresponds an RWTOOLMSG_XXX
 *   of type RWCatMsg.
 * Instead of simply defining a variable: RWMsgId RWTOOL_XXX = &RWTOOLMSG_XXX, 
 *   RWTOOL_XXX is #defined to be a function that returns &RWTOOLMSG_XXX.
 * This is done to side-step the problem of compilers (e.g. DEC)
 *   that generate pre-main code for the initialization of address constants.
 * It also serves us in the case of DLLs where variables cannot be exported.
 */

#define RWTOOL_ALLOCOUT     RWTOOL_ALLOCOUT()
#define RWTOOL_BADRE        RWTOOL_BADRE()
#define RWTOOL_CRABS        RWTOOL_CRABS()
#define RWTOOL_FLIST        RWTOOL_FLIST()
#define RWTOOL_ID           RWTOOL_ID()
#define RWTOOL_INDEX        RWTOOL_INDEX()
#define RWTOOL_LOCK         RWTOOL_LOCK()
#define RWTOOL_LONGINDEX    RWTOOL_LONGINDEX()
#define RWTOOL_MAGIC        RWTOOL_MAGIC()
#define RWTOOL_NEVECL       RWTOOL_NEVECL()
#define RWTOOL_NOCREATE     RWTOOL_NOCREATE()
#define RWTOOL_NOTALLOW     RWTOOL_NOTALLOW()
#define RWTOOL_READERR      RWTOOL_READERR()
#define RWTOOL_REF          RWTOOL_REF()
#define RWTOOL_SEEKERR      RWTOOL_SEEKERR()
#define RWTOOL_STREAM       RWTOOL_STREAM()
#define RWTOOL_SUBSTRING    RWTOOL_SUBSTRING()
#define RWTOOL_UNLOCK       RWTOOL_UNLOCK()
#define RWTOOL_WRITEERR     RWTOOL_WRITEERR()
#define RWTOOL_INDEXERR     RWTOOL_INDEXERR()

extern const RWMsgId rwexport RWTOOL_ALLOCOUT;
extern const RWMsgId rwexport RWTOOL_BADRE;
extern const RWMsgId rwexport RWTOOL_CRABS;
extern const RWMsgId rwexport RWTOOL_FLIST;
extern const RWMsgId rwexport RWTOOL_ID;
extern const RWMsgId rwexport RWTOOL_INDEX;
extern const RWMsgId rwexport RWTOOL_LOCK;
extern const RWMsgId rwexport RWTOOL_LONGINDEX;
extern const RWMsgId rwexport RWTOOL_MAGIC;
extern const RWMsgId rwexport RWTOOL_NEVECL;
extern const RWMsgId rwexport RWTOOL_NOCREATE;
extern const RWMsgId rwexport RWTOOL_NOTALLOW;
extern const RWMsgId rwexport RWTOOL_READERR;
extern const RWMsgId rwexport RWTOOL_REF;
extern const RWMsgId rwexport RWTOOL_SEEKERR;
extern const RWMsgId rwexport RWTOOL_STREAM;
extern const RWMsgId rwexport RWTOOL_SUBSTRING;
extern const RWMsgId rwexport RWTOOL_UNLOCK;
extern const RWMsgId rwexport RWTOOL_WRITEERR;
extern const RWMsgId rwexport RWTOOL_INDEXERR;

#endif  /*  __RWTOOLERR_H__ */
