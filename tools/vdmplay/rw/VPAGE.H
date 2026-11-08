#ifndef __RWVPAGE_H__
#define __RWVPAGE_H__

/*
 * RWVirtualPageHeap: Abstraction of a page heap, swapped to unknown parts
 *
 * $Id: vpage.h,v 6.4 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: vpage.h,v $
 * Revision 6.4  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/16  03:37:14  vriezen
 * Added RW_DECLARE_HOME
 *
 * Revision 6.2  1994/04/15  21:12:29  vriezen
 * Backed out (until next release) RW_DEFINE_HOME and RW_DECLARE_HOME support.
 *
 * Revision 6.1  1994/04/15  19:48:47  vriezen
 * Move all files to 6.1
 *
 * Revision 2.3  1994/03/31  21:56:53  vriezen
 * Add RW_DECLARE_HOME macro to RWVirtualPageHeap
 *
 * Revision 2.2  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.0   11 Mar 1992 14:10:00   KEFFER
 * Initial revision.
 */

#include "rw/tooldefs.h"

typedef unsigned RWHandle;

class RWExport RWVirtualPageHeap {
 RW_DECLARE_HOME(RWVirtualPageHeap)
protected:
  unsigned		pageSize_;
public:
  RWVirtualPageHeap(unsigned pgsize) : pageSize_(pgsize) { }
  virtual		~RWVirtualPageHeap() { }

  unsigned		pageSize() const {return pageSize_;}

  virtual RWHandle	allocate()           = 0;	// Allocate a page
  virtual void		deallocate(RWHandle) = 0;	// Deallocate it
  virtual void		dirty(RWHandle)      = 0;	// Declare page as dirty
  virtual void*		lock(RWHandle)       = 0;	// Lock a page
  virtual void		unlock(RWHandle)     = 0;	// Unlock a page
};

#endif	/* __RWVPAGE_H__ */
