#ifndef __RWTSTACK_H__
#define __RWTSTACK_H__

/*
 * Parameterized stack of T's, implemented using class C.
 *
 * $Id: tstack.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * WARNING: This class is very slow when used with singly-linked lists
 *          because they use an inefficient implementation for removeLast().
 *
 ***************************************************************************
 *
 * $Log: tstack.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:08  vriezen
 * Move all files to 6.1
 *
 * Revision 2.6  1994/01/03  23:33:28  jims
 * ObjectStore version: Add #include <ostore/ostore.hh>
 *
 * Revision 2.5  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.4  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.3  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.2  1993/02/06  22:50:20  keffer
 * Ported to cfront V3.0
 *
 *    Rev 1.0   02 Mar 1992 16:10:52   KEFFER
 * Initial revision.
 */

#ifndef __RWDEFS_H__
#  include "rw/defs.h"
#endif

#ifdef RW_BROKEN_TEMPLATES

template <class T, class C> class RWTExport RWTStack
{

public:

  void		clear()		{container_.clear();}
  size_t	entries() const	{return container_.entries();}
  RWBoolean	isEmpty() const	{return container_.isEmpty();}
  void		push(T a)	{container_.append(a);}
  T		pop()		{return container_.removeLast();}
  T		top() const	{return container_.last();}

protected:

  C		container_;

};

#else	/* !RW_BROKEN_TEMPLATES */

template <class T, class C> class RWTExport RWTStack : private C
{

public:

  C::clear;
  C::entries;
  C::isEmpty;
  void		push(T a)	{C::append(a);}
  T		pop()		{return C::removeLast();}
  T		top() const	{return C::last();}

};

#endif	/* RW_BROKEN_TEMPLATES */

#endif	/* __RWTSTACK_H__ */
