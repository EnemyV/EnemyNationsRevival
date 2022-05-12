#ifndef __RWCOLCLASS_H__
#define __RWCOLCLASS_H__

/*
 * Base class for the Smalltalk(TM)-like collection classes.
 *
 * $Id: colclass.h,v 6.4 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: colclass.h,v $
 * Revision 6.4  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/18  03:28:01  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.2  1994/06/13  20:07:00  vriezen
 * Bug #481.  Fixed oper>> when rhs operand is RWCollection*
 *
 * Revision 6.1  1994/04/15  19:09:34  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1993/09/09  02:38:31  keffer
 * Added member function copyContentsTo().
 *
 * Revision 2.6  1993/04/14  18:38:37  keffer
 * Added declaration for static m.f. saveObjToStream() and
 * saveObjToRWFile()
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/17  21:07:06  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.2  1993/02/06  03:22:47  keffer
 * Added RWExport tags to RWBag, RWOrdered, etc. declarations.
 *
 *    Rev 1.5   12 Jun 1992 14:35:16   KEFFER
 * Ported to Computer Innovations C++; inserted declarations for RWBag, etc.
 * 
 *    Rev 1.4   25 May 1992 15:46:38   KEFFER
 * CAPACITY and RESIZE are now non-const and can be changed.
 * 
 *    Rev 1.2   28 Oct 1991 09:08:08   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   29 Jul 1991 13:53:02   keffer
 * Changed return type of asBag(), asOrderedCollection(), asSet() and
 * asSortedCollection() to their RW names.
 * 
 *    Rev 1.0   28 Jul 1991 08:13:12   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/collect.h"

class RWExport RWBag;
class RWExport RWOrdered;
class RWExport RWSet;
class RWExport RWBinaryTree;

/****************************************************************
 *								*
 *			RWCollection				*
 *								*
 ****************************************************************/

class RWExport RWCollection : public RWCollectable {
  RWDECLARE_ABSTRACT_COLLECTABLE(RWCollection)
public:

  static size_t		DEFAULT_CAPACITY;
  static size_t		DEFAULT_RESIZE;

public:
  virtual ~RWCollection();

  /************* Virtual function inherited from RWCollectable *****************/
  virtual RWspace		binaryStoreSize() const;
  virtual RWClassID		isA() const {return __RWCOLLECTION;}
  virtual void			restoreGuts(RWvistream&);
  virtual void			restoreGuts(RWFile&);
  virtual void			saveGuts(RWvostream&) const;
  virtual void			saveGuts(RWFile&) const;

  /****************  Added virtual functions for RWCollections ****************/
  virtual void			apply(RWapplyCollectable, void*) = 0;
  virtual void			clear() = 0;
  virtual void			clearAndDestroy();
  virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const = 0;	// Total entries
  virtual RWCollectable*	find(const RWCollectable*) const = 0;		// First occurrence
  virtual RWCollectable*	insert(RWCollectable*) = 0;
  virtual RWBoolean		isEmpty() const = 0;
  virtual size_t		occurrencesOf(const RWCollectable*) const = 0;
  virtual RWCollectable*	remove(const RWCollectable*) = 0;	// Remove first occurrence
  virtual void			removeAndDestroy(const RWCollectable*); 

  /************************ Special functions *******************************/
  void				operator+=(const RWCollection& c); // Add items of c to self
  void				operator-=(const RWCollection& c); // Remove items of c from self
  void				addContentsTo(RWCollection*) const;
  void				copyContentsTo(RWCollection*) const;
  RWBag				asBag() const;
  RWOrdered			asOrderedCollection() const;
  RWSet				asSet() const;
  RWBinaryTree			asSortedCollection() const;
  RWCollection*			select(RWtestCollectable, void*) const;

protected:

  static void			saveObjToStream(RWCollectable*, void*);
  static void			saveObjToRWFile(RWCollectable*, void*);

};	  

typedef void (*RWapplyKeyAndValue)(RWCollectable*, RWCollectable*, void*);

#endif /* __RWCOLCLASS_H__ */
