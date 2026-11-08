#ifndef __RWFILEMGR_H__
#define __RWFILEMGR_H__

/*
 * RWFileManager --- manages free space in a file
 *
 * $Id: filemgr.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: filemgr.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/16  18:10:10  jims
 * Add optional mode paramter to constructor to allow use of
 * read-only files
 *
 * Revision 6.1  1994/04/15  19:09:57  vriezen
 * Move all files to 6.1
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/07/19  20:45:26  keffer
 * friend classes now use elaborated-type-specifier (ARM Sec. 11.4)
 *
 * Revision 2.4  1993/05/18  00:41:46  keffer
 * Now uses new exception handling facility
 *
 * Revision 2.3  1993/04/09  23:09:56  keffer
 * Extensive rewrite.  Allocations now done in units of RWspace.
 *
 * Revision 2.2  1993/03/01  23:28:38  keffer
 * Renamed variables.
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 *    Rev 1.2   18 Feb 1992 09:54:20   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:14   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:15:00   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/rwfile.h"
#include "rw/tooldefs.h"
#include "rw/rwerr.h"

class RWListManager;

class RWExport RWFileManager : public RWFile
{

public:

  RWFileManager(const char* fname, const char* mode = rwnil);

  ~RWFileManager();

  RWoffset		allocate(RWspace);  	// Allocate storage 
  void			deallocate(RWoffset);	// Deallocate storage.
  RWoffset		endData() const  {return endOfData_;} 
  RWoffset		start()   const	 {return startOfData_;}

#ifdef RDEBUG
  RWoffset              walkFreeList(RWoffset&, int&, RWspace&);
  void			summarize();
#endif

protected:

  RWoffset              allocateAtEnd(RWspace);
  RWBoolean             deallocateFromEnd(RWoffset, RWspace);
  void                  readErr();
  void                  seekErr();
  void                  writeErr();
  RWoffset              rootOffset() const;

private:

  // Cannot have 2 managers for the same file.
  RWFileManager(const RWFileManager&);
  void			operator=(const RWFileManager&);

private:

  RWListManager*        filemgr_;       // Implementation
  RWoffset		startOfData_;	// Offset to first data in file.
  RWoffset		endOfData_;	// Offset to last data in file.

friend class RWListManager;
};

#endif /* __RWFILEMGR_H__ */
