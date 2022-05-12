#ifndef __RWBTREE_H__
#define __RWBTREE_H__

/*
 * RWBTree -- in memory B-Tree.
 *
 * $Id: btree.h,v 6.2 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: btree.h,v $
 * Revision 6.2  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:09:30  vriezen
 * Move all files to 6.1
 *
 * Revision 2.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/03/20  22:02:16  keffer
 * Eliminated int to unsigned type conversions.
 *
 * Revision 2.3  1993/03/13  02:14:22  keffer
 * keyP removed in favor of RWCollectableP
 *
 * Revision 2.2  1993/01/28  01:56:00  myersn
 * derive from RWMemoryPool via the macro RWMemoryPool_OPTION for MT-safety
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.5   04 Aug 1992 18:55:48   KEFFER
 * Added "virtual" keyword to destructor for documentation purposes.
 * 
 *    Rev 1.4   22 May 1992 17:04:08   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   04 Mar 1992 09:07:24   KEFFER
 * Changed nil to rwnil
 * 
 *    Rev 1.2   18 Feb 1992 09:54:10   KEFFER
 * 
 *    Rev 1.1   28 Oct 1991 09:08:08   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:12:46   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/colclass.h"
#include "rw/mempool.h"

// Set the order of the B-Tree:
#ifdef RDEBUG
  const unsigned rworder  = 2;
#else
  const unsigned rworder  = 50;
#endif
  const unsigned rworder2 = 2*rworder;

/****************************************************************
 *								*
 *			RWBTreeNode				*
 *								*
 ****************************************************************/

class RWExport RWBTreeNode   RWMemoryPool_OPTION {
friend class RWExport RWBTree;

  RWBTreeNode();			// Private constructors.
  RWBTreeNode(RWCollectable*);
  unsigned	counter;		// How many of the [rworder2] fields are used.
  RWCollectableP key[rworder2];		// Array of pointers keys.
  RWBTreeNode*	next[rworder2+1];	// Array of pointers to children nodes.
  unsigned	binarySearch(const RWCollectable*) const; 	// Binary search for insertion.
  void		initialize();
  void		siz(size_t&) const;	// Count items in self & children.
  RWBoolean	subSetOf(const RWBTree& bt) const;
};

/****************************************************************
 *								*
 *			RWBTree					*
 *								*
 ****************************************************************/

class RWExport RWBTree : public RWCollection {

  RWDECLARE_COLLECTABLE(RWBTree)

public:

  RWBTree();
  virtual ~RWBTree();
  RWBTree(const RWBTree&);

  void				operator=(const RWBTree&);
  RWBoolean			operator<=(const RWBTree& bt) const; // Subset of bt
  RWBoolean			operator==(const RWBTree& bt) const;

// Special member function to return the height of the B-tree.
  unsigned			height() const;

/************ Standard Collection classes functions **************/
  virtual void			apply(RWapplyCollectable, void*);
  virtual void			clear();
//virtual void			clearAndDestroy();
//virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const;		// Total entries
  virtual RWCollectable*	find(const RWCollectable*) const;
  virtual RWCollectable*	insert(RWCollectable*);
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual RWBoolean		isEmpty() const {return root==rwnil;}
  virtual size_t		occurrencesOf(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable*);
//virtual void			removeAndDestroy(const RWCollectable*);

private:

  RWBTreeNode*		root;			// root = first node in tree.
  RWCollectable* 	tempKey;		// Place holder to handle node over- and underflow
  RWBTreeNode*		tempNode;		// Place holder to handle node over- and underflow

protected:

  void			apl(RWBTreeNode*, RWapplyCollectable, void*);	// Apply to all children
  void			del(RWBTreeNode*);				// Delete all children.
  int			ins(RWCollectable* a, RWBTreeNode*);		// Insert a in tree.
  int			rem(const RWCollectable* a, RWBTreeNode*, RWCollectable*&); // Remove a
};    
			 
#endif /* __RWBTREE_H__ */

