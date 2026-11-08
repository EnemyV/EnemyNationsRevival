#ifndef __RWSEQCLTN_H__
#define __RWSEQCLTN_H__

/*
 * Abstract base class for sequenceable collections.
 *
 * $Id: seqcltn.h,v 6.7 1994/07/12 19:58:19 vriezen Exp $
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
 * This is an abstract base class, inherited by collections whose
 * elements are ordered and accessible by an index.
 *
 * $Log: seqcltn.h,v $
 * Revision 6.7  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.6  1994/06/22  23:40:38  vriezen
 * Remove RW_DEFINE_HOME and RW_DECLARE_HOME, no longer needed.
 *
 * Revision 6.5  1994/06/18  00:49:24  myersn
 * move RWSequenceable dtor out-of-line.
 *
 * Revision 6.4  1994/06/16  03:34:55  vriezen
 * Added RW_DECLARE_HOME
 *
 * Revision 6.3  1994/06/13  20:07:00  vriezen
 * Bug #481.  Fixed oper>> when rhs operand is RWCollection*
 *
 * Revision 6.2  1994/04/15  21:12:29  vriezen
 * Backed out (until next release) RW_DEFINE_HOME and RW_DECLARE_HOME support.
 *
 * Revision 6.1  1994/04/15  19:10:56  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1994/03/23  21:16:01  vriezen
 * Added RW_DECLARE_HOME macro to RWSequenceable
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.3  1993/01/25  18:12:13  keffer
 * RW_NO_CONST_OVERLOADS->RW_NO_CONST_OVERLOAD
 *
 * Revision 2.1  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.2   18 Feb 1992 09:54:44   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:22   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:16:54   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/colclass.h"

class RWExport RWSequenceable : public RWCollection {
  RWDECLARE_ABSTRACT_COLLECTABLE(RWSequenceable)
public:
  virtual                       ~RWSequenceable();

  /************ Virtual functions inherited from RWCollection *************/
  virtual void			apply(RWapplyCollectable, void*) = 0;
  virtual void			clear() = 0;
//virtual void			clearAndDestroy();
//virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const = 0;	// Total entries
  virtual RWCollectable*	find(const RWCollectable*) const = 0;		// First occurrence
  virtual RWCollectable*	insert(RWCollectable*) = 0;
  virtual RWBoolean		isEmpty() const = 0;
  virtual size_t		occurrencesOf(const RWCollectable*) const = 0;
  virtual RWCollectable*	remove(const RWCollectable*) = 0;	// Remove first occurrence
//virtual void			removeAndDestroy(const RWCollectable*); 

public:

  /****************  Added virtual functions for RWSequenceables ****************/
  virtual RWCollectable*	append(RWCollectable*) = 0;
  virtual RWCollectable*&	at(size_t) = 0; // Some collections can use as lvalue
#ifndef RW_NO_CONST_OVERLOAD
  virtual const RWCollectable*	at(size_t) const = 0; // Cannot use as lvalue
#endif
  virtual RWCollectable*	first() const = 0;
  virtual size_t		index(const RWCollectable*) const = 0;
  virtual RWCollectable*	insertAt(size_t, RWCollectable*) = 0;
  virtual RWCollectable*	last() const = 0;
  virtual RWCollectable*	prepend(RWCollectable*) = 0;

  // For backwards compatibility:
  virtual RWCollectable*	insertAfter(int, RWCollectable*) = 0;
};

#endif /* __RWSEQCLTN_H__ */
