#ifndef  __RWGDLIST_H__
#define  __RWGDLIST_H__

/*
 * Declarations for General Doubly-linked lists.
 *
 * $Id: gdlist.h,v 6.3 1994/07/14 21:04:32 jims Exp $
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
 * $Log: gdlist.h,v $
 * Revision 6.3  1994/07/14  21:04:32  jims
 * Use RWGExport for generic collection classes
 *
 * Revision 6.2  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:59  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1994/01/13  05:03:17  jims
 * Add member function  const type* at(size_t i) const  as documented in manual
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/08/03  21:02:38  dealys
 * Ported to MPW C++ 3.3 - RW_BROKEN_TOKEN_PASTE
 *
 * Revision 2.4  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.3  1993/03/25  03:50:41  keffer
 * Added prefix RW to class name
 *
 * Revision 2.2  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.3   18 Feb 1992 19:22:58   KEFFER
 * Now includes "rw/generic.h".
 * Class tag is now RWExport instead of huge.
 * 
 *    Rev 1.2   28 Oct 1991 09:08:14   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   09 Oct 1991 18:34:32   keffer
 * Ported to Zortech V3.0
 * 
 *    Rev 1.0   28 Jul 1991 08:11:18   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/dlist.h"
#include "rw/generic.h"

#ifdef RW_BROKEN_TOKEN_PASTE
#  define RWGDlist(type) type##RWGDlist      
#  define RWGDlistIterator(type) type##RWGDlistIterator
#else
#  define RWGDlist(type) name2(type,RWGDlist)
#  define RWGDlistIterator(type) name2(type,RWGDlistIterator)
#endif

#define RWGDlistdeclare(type)							\
class RWGExport RWGDlist(type) : public RWDlist {				\
public:										\
  RWGDlist(type)() { }								\
  RWGDlist(type)(type* a) : RWDlist(a) { }					\
  type* append(type* a)								\
    { return (type*)RWDlist::append(a); }					\
  type*& at(size_t i)								\
    { return (type*&)RWDlist::at(i); }						\
  const type* at(size_t i) const						\
    { return (const type*)RWDlist::at(i); }					\
  RWBoolean contains( RWBoolean (*t)(const type*, const void*), const void* a) const\
    { return RWDlist::contains((RWtestGeneric)t, a); }				\
  RWBoolean containsReference(const type* a) const				\
    { return RWDlist::containsReference(a); }					\
  void  apply(void (*ap)(type*, void*), void* x) 				\
    { RWDlist::apply((RWapplyGeneric)ap, x); }					\
  type* find(RWBoolean (*t)(const type*, const void*), const void* a) const	\
    { return (type*)RWDlist::find((RWtestGeneric)t, a); }			\
  type* findReference(const type* a) const					\
    { return (type*)RWDlist::findReference(a); }				\
  type* first() const								\
    { return (type*)RWDlist::first(); }						\
  type* get()									\
    { return (type*)RWDlist::get(); }						\
  type* insert(type* a)								\
    { return (type*)RWDlist::append(a); }					\
  type* insertAfter(int i, type* a)						\
    { return (type*)RWDlist::insertAfter(i,a); }				\
  type* insertAt(size_t i, type* a)						\
    { return (type*)RWDlist::insertAt(i,a); }					\
  type* last() const								\
    { return (type*)RWDlist::last(); }						\
  size_t occurrencesOf(RWBoolean (*t)(const type*, const void*), const void* a) const	\
    { return RWDlist::occurrencesOf((RWtestGeneric)t, a); }			\
  size_t occurrencesOfReference(const type* a) const				\
    { return RWDlist::occurrencesOfReference(a); }				\
  type*  prepend(type* a)							\
    { return (type*)RWDlist::prepend(a); }					\
  type* remove(RWBoolean (*t)(const type*, const void*), const void* a)		\
    { return (type*)RWDlist::remove((RWtestGeneric)t, a); }			\
  type* removeReference(const type* a)						\
    { return (type*)RWDlist::removeReference(a); }				\
};										\
										\
class RWGExport RWGDlistIterator(type) : public RWDlistIterator {		\
public:										\
  RWGDlistIterator(type)(RWGDlist(type)& s) : RWDlistIterator(s) { }		\
  type* findNext(RWBoolean (*t)(const type*, const void*), const void* a)	\
    { return (type*)RWDlistIterator::findNext((RWtestGeneric)t, a); }		\
  type* findNextReference(const type* a)					\
    { return (type*)RWDlistIterator::findNextReference(a); }			\
  type* insertAfterPoint(type* a)						\
    { return (type*)RWDlistIterator::insertAfterPoint(a); }			\
  type* key() const {return (type*)RWDlistIterator::key();}			\
  type* operator()() {return (type*)(RWDlistIterator::operator()()); }		\
  type* remove() {return (type*)RWDlistIterator::remove();}			\
  type* removeNext(RWBoolean (*t)(const type*, const void*), const void* a)	\
    {return (type*)RWDlistIterator::removeNext((RWtestGeneric)t, a);}		\
  type* removeNextReference(const type* a)					\
    {return (type*)RWDlistIterator::removeNextReference(a);}			\
};

#endif /* __RWGDLIST_H__ */
