#ifndef __RWBAG_H__
#define __RWBAG_H__

/*
 * Declarations for RWBag --- an unordered collection.
 *
 * $Id: rwbag.h,v 6.4 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwbag.h,v $
 * Revision 6.4  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/18  02:22:57  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.2  1994/06/16  04:34:44  myersn
 * add buckets() member for use with resize().
 *
 * Revision 6.1  1994/04/15  19:10:43  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1994/01/13  07:55:07  jims
 * Override removeAndDestroy
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/07/19  20:45:26  keffer
 * friend classes now use elaborated-type-specifier (ARM Sec. 11.4)
 *
 * Revision 2.4  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.3  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.2  1993/02/07  18:45:31  keffer
 * Added default value of 0 to resize().
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 * 
 *    Rev 1.4   04 Aug 1992 19:00:26   KEFFER
 * Added "virtual" keyword to destructor for documentation purposes.
 * 
 *    Rev 1.3   22 May 1992 17:04:14   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.2   18 Feb 1992 09:54:36   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:20   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:16:18   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/hashdict.h"

class RWExport RWBagIterator;

/****************************************************************
 *								*
 *			RWBag					*
 *								*
 ****************************************************************/

class RWExport RWBag : public RWCollection
{

  friend class RWExport RWBagIterator;
  RWDECLARE_COLLECTABLE(RWBag)

public:

  RWBag(size_t n = RWCollection::DEFAULT_CAPACITY);
  RWBag(const RWBag&);
  virtual ~RWBag();

  /******************** Member operators ****************************/
  void			operator=(const RWBag&);
  RWBoolean		operator==(const RWBag&)const;

  /****************** Virtual member functions *******************/
  virtual void			apply(RWapplyCollectable, void*);
//virtual RWspace		binaryStoreSize() const;
  virtual void			clear();
  virtual void			clearAndDestroy();
//virtual int			compareTo(const RWCollectable*) const;
//virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const			{return totalEntries;}
  virtual RWCollectable*	find(const RWCollectable*) const;
//virtual size_t		hash() const;
  virtual RWCollectable*	insert(RWCollectable*);
  virtual RWBoolean		isEmpty() const {return totalEntries==0;}
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual size_t		occurrencesOf(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable*);	// Remove first occurrence
  virtual void			removeAndDestroy(const RWCollectable*); 
//virtual void			restoreGuts(RWvistream&);
//virtual void			restoreGuts(RWFile&);
//virtual void			saveGuts(RWvostream&) const;
//virtual void			saveGuts(RWFile&) const;

/********************* Special functions **********************************/

  RWCollectable*		insertWithOccurrences(RWCollectable*, size_t);
  void				resize(size_t n = 0);
  size_t			buckets() const { return contents.buckets(); }

private:

  size_t		totalEntries;
  RWHashDictionary	contents;	// Dictionary of number of occurrences.

  void			deepenTally();
};

class RWExport RWBagIterator : public RWIterator
{

public:

  RWBagIterator(const RWBag&);
  virtual ~RWBagIterator();

  /******************* Virtual function inherited from RWIterator ******************/
  virtual RWCollectable*	findNext(const RWCollectable*);
  virtual RWCollectable*	key() const;		// Return current item
  virtual RWCollectable*	operator()();
  virtual void			reset();

private:

  RWHashDictionaryIterator	contentsIterator;
  int				count;
  RWCollectable*		currentItem;

};

#endif /* __RWBAG_H__ */
