
#ifndef __RWFACTORY_H__
#define __RWFACTORY_H__

/*
 * RWFactory --- can create an instance of a registered class
 *
 * $Id: factory.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: factory.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/16  18:01:21  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:09:55  vriezen
 * Move all files to 6.1
 *
 * Revision 2.6  1993/09/03  02:08:13  keffer
 * Macro _CLASSDLL is now named _RWTOOLSDLL
 *
 * Revision 2.5  1993/08/05  11:49:12  jims
 * Distinguish between using a WIN16 DLL from a WIN32 DLL by
 * checking for __WIN16__
 *
 * Revision 2.4  1993/04/12  12:32:43  jims
 * The Factory now shared among multiple threads within a single process
 *
 * Revision 2.3  1993/03/13  02:21:20  keffer
 * ClassID->RWClassId
 *
 * Revision 2.2  1993/02/03  00:19:55  jims
 * Removed #include directive for procinit.h
 *
 * Revision 2.1  1993/01/15  22:23:49  keffer
 * Tools.h++ V5.2 alpha
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.5   13 May 1992 10:25:46   KEFFER
 * Changed typedef for userCreator to RWuserCreator.
 * 
 *    Rev 1.4   18 Mar 1992 11:28:32   KEFFER
 * Initializes instance manager.
 * 
 *    Rev 1.3   18 Feb 1992 09:54:20   KEFFER
 * 
 *    Rev 1.2   12 Nov 1991 13:15:00   keffer
 * Factory can now live in the DLL.
 * 
 *    Rev 1.1   28 Oct 1991 09:08:14   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 */

#include "rw/rwset.h"

class RWExport RWCollectable;

class RWExport RWFactory : public RWSet {
public:
  RWFactory();
  ~RWFactory();	

  void 			addFunction(RWuserCreator, RWClassID);
  // Create object with given class id:
  RWCollectable*	create(RWClassID) const; 
  RWuserCreator		getFunction(RWClassID) const;
  void			removeFunction(RWClassID);
};

#ifndef RW_TRAILING_RWEXPORT
rwexport RWFactory*	getRWFactory();
#else
RWFactory* rwexport	getRWFactory();
#endif

#if defined(_RWTOOLSDLL) && defined(__WIN16__)
  /* DLL situation; get the proper factory from the instance manager. */
# define theFactory (getRWFactory())
#else
  extern RWFactory* theFactory;	// The one-of-a-kind global factory.
#endif

#endif /* __RWFACTORY_H__ */
