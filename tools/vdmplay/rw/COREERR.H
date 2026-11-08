#ifndef __RWCOREERR_H__
#define __RWCOREERR_H__

/*
 * Error messages for Core.h++
 *
 * $Id: coreerr.h,v 6.4 1994/07/12 18:46:56 vriezen Exp $
 *
 ****************************************************************************
 *
 *  Rogue Wave Software, Inc.
 *  P.O. Box 2328
 *  Corvallis, OR 97339
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
 * $Log: coreerr.h,v $
 * Revision 6.4  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/14  16:43:35  jims
 * Now #defines RWCORE_XXX to be a function for all builds
 *
 * Revision 6.2  1994/05/16  18:02:59  jims
 * Use functions for DLL since can't export variables
 *
 * Revision 6.1  1994/04/15  19:09:45  vriezen
 * Move all files to 6.1
 *
 * Revision 2.3  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.2  1993/05/18  21:48:25  keffer
 * Added RCS keywords
 *
 */

#include "rw/message.h"

/*
 * RWMsgId is a typedef for RWCatMsg*.
 * For each RWMsgId of the form RWCORE_XXX their corresponds an RWCOREMSG_XXX
 *   of type RWCatMsg.
 * Instead of simply defining a variable: RWMsgId RWCORE_XXX = &RWCOREMSG_XXX, 
 *   RWCORE_XXX is #defined to be a function that returns &RWCOREMSG_XXX.
 * This is done to side-step the problem of compilers (e.g. DEC)
 *   that generate pre-main code for the initialization of address constants.
 * It also serves us in the case of DLLs where variables cannot be exported.
 */

#define RWCORE_EOF        RWCORE_EOF()
#define RWCORE_GENERIC    RWCORE_GENERIC()
#define RWCORE_INVADDR    RWCORE_INVADDR()
#define RWCORE_LOCK       RWCORE_LOCK()
#define RWCORE_NOINIT     RWCORE_NOINIT()
#define RWCORE_NOMEM      RWCORE_NOMEM()
#define RWCORE_OPERR      RWCORE_OPERR()
#define RWCORE_OUTALLOC   RWCORE_OUTALLOC()
#define RWCORE_OVFLOW     RWCORE_OVFLOW()
#define RWCORE_STREAM     RWCORE_STREAM()
#define RWCORE_SYNSTREAM  RWCORE_SYNSTREAM()

extern const rwexport RWMsgId RWCORE_EOF;
extern const rwexport RWMsgId RWCORE_GENERIC;
extern const rwexport RWMsgId RWCORE_INVADDR;
extern const rwexport RWMsgId RWCORE_LOCK;
extern const rwexport RWMsgId RWCORE_NOINIT;
extern const rwexport RWMsgId RWCORE_NOMEM;
extern const rwexport RWMsgId RWCORE_OPERR;
extern const rwexport RWMsgId RWCORE_OUTALLOC;
extern const rwexport RWMsgId RWCORE_OVFLOW;
extern const rwexport RWMsgId RWCORE_STREAM;
extern const rwexport RWMsgId RWCORE_SYNSTREAM;

#endif /* __RWCOREERR_H__ */
