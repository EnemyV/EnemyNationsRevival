#ifndef __RWMODEL_H__
#define __RWMODEL_H__

/*
 * RWModel --- maintains a list of dependent clients
 *
 * $Id: model.h,v 6.4 1994/07/18 19:04:25 jims Exp $
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
 *
 ***************************************************************************
 *
 * $Log: model.h,v $
 * Revision 6.4  1994/07/18  19:04:25  jims
 * Add copyright
 *
 * Revision 6.3  1994/06/16  04:20:24  vriezen
 * Added outlined virtual dtors.
 *
 * Revision 6.2  1994/04/15  21:12:29  vriezen
 * Backed out (until next release) RW_DEFINE_HOME and RW_DECLARE_HOME support.
 *
 * Revision 6.1  1994/04/15  19:10:31  vriezen
 * Move all files to 6.1
 *
 * Revision 2.4  1994/03/31  23:16:08  vriezen
 * Add RW_DECLARE_HOME and RW_DEFINE_HOME for class RWModelClient
 *
 * Revision 2.3  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.1  1992/11/16  04:24:16  keffer
 * Broke out RWModel functionality into RWModel and RWModelClient
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.1   18 Feb 1992 09:55:34   KEFFER
 * 
 *    Rev 1.0   12 Nov 1991 13:13:18   keffer
 * Initial revision.
 */

#include "rw/ordcltn.h"

class RWExport RWModelClient;

class RWExport RWModel
{
public:
  virtual ~RWModel();
  RWModel();
  void			addDependent(RWModelClient*);
  void			removeDependent(RWModelClient*);
  // Allows peek at dependent list:
  const RWOrdered*	dependents() {return &dependList;}
  virtual void		changed(void* pData = 0);	// Notify all dependents
private:
  RWOrdered		dependList;
};


class RWExport RWModelClient : public RWCollectable
{
public:
  virtual ~RWModelClient();
  virtual void		updateFrom(RWModel* model, void* pData) = 0;
};

#endif	/* __RWMODEL_H__ */
