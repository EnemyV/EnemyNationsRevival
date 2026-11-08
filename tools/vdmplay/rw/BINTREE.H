#ifndef __RWBINTREE_H__
#define __RWBINTREE_H__

/*
 * Binary tree of pointers to RWCollectable objects
 *
 * $Id: bintree.h,v 6.3 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: bintree.h,v $
 * Revision 6.3  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  02:22:57  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:09:23  vriezen
 * Move all files to 6.1
 *
 * Revision 2.8  1993/11/06  05:17:44  jims
 * Port to ObjectStore
 *
 * Revision 2.7  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.6  1993/04/14  18:37:36  keffer
 * Moved declaration for RWTreeNode into the .cpp file.
 * Now does level saveGuts(), removing the need for a balance()
 * on restore.
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.4  1993/04/01  00:23:33  myersn
 * and RW prefix to GQueue and GStack mentions.
 *
 * Revision 2.3  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.2  1993/01/28  01:53:04  myersn
 * derive from RWMemoryPool via the macro RWMemoryPool_OPTION for MT-safety
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.8   04 Aug 1992 18:55:32   KEFFER
 * Added "virtual" keyword to destructor for documentation purposes.
 * 
 *    Rev 1.7   29 May 1992 10:45:16   KEFFER
 * Removed declaration for unused function constructFrom().
 * 
 *    Rev 1.6   22 May 1992 17:04:08   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.5   04 Mar 1992 09:02:38   KEFFER
 * nil changed to rwnil
 * 
 *    Rev 1.2   28 Oct 1991 09:08:06   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 */

#include "rw/colclass.h"
#include "rw/iterator.h"
#include "rw/mempool.h"
#include "rw/gqueue.h"
#include "rw/gstack.h"
STARTWRAP
#include <stddef.h>
ENDWRAP

// Forward declaration:
class RWExport RWTreeNode;

declare(RWGQueue, RWCollectable)
declare(RWGStack, RWTreeNode)


/****************************************************************
 *								*
 *			RWBinaryTree				*
 *								*
 ****************************************************************/

class RWExport RWBinaryTree : public RWCollection {
friend class RWExport RWBinaryTreeIterator;

  RWDECLARE_COLLECTABLE(RWBinaryTree)

public:

  RWBinaryTree();
  RWBinaryTree(const RWBinaryTree&);
  virtual ~RWBinaryTree();

  void				operator=(const RWBinaryTree&);
  RWBoolean			operator<=(const RWBinaryTree& bt) const; // Subset of bt
  RWBoolean			operator==(const RWBinaryTree& bt) const;

  // Special member function to balance a binary tree:
  void				balance();

/********  Standard Member Functions for Collection Classes ********/  
  virtual void			apply(RWapplyCollectable, void*);
  virtual void			clear();
//virtual void			clearAndDestroy();
//virtual RWBoolean		contains(const RWCollectable*) const;
  virtual size_t		entries() const;			// Total entries
  virtual RWCollectable*	find(const RWCollectable*) const;		// First occurrence
  virtual RWCollectable*	insert(RWCollectable*);
  virtual RWBoolean		isEmpty() const {return root==rwnil;}
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual size_t		occurrencesOf(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable*);		// Remove first occurrence
//virtual void	 		removeAndDestroy(const RWCollectable*); 
  virtual void			saveGuts(RWvostream&) const;
  virtual void			saveGuts(RWFile&) const;

protected:

  void				applyChildren(const RWTreeNode*, RWapplyCollectable, void*);
  void				balanceUnique();
  RWTreeNode*	        	balanceChildren(size_t, RWGQueue(RWCollectable)&);
  void				countChildren(const RWTreeNode*, size_t&) const;
  void				deleteChildren(RWTreeNode*);
  RWCollectable*		deleteNode(RWTreeNode* victim, RWTreeNode* parent);
  void				insertChildrenOf(const RWTreeNode*);
  void				levelApply(RWapplyCollectable, void*);

private:

#ifdef RDEBUG
  void	printChildren(const RWTreeNode*, ostream&, size_t&, char) const;
  friend ostream& operator<<(ostream&, const RWBinaryTree& bt);
#endif
  RWTreeNode*			root;      // root = top-level item in tree

};    

/****************************************************************
 *								*
 *		RWBinaryTreeIterator				*
 *								*
 ****************************************************************/

class RWExport RWBinaryTreeIterator : public RWIterator {
protected:
  const RWBinaryTree*		tree;		// Binary tree for this iterator.
  const RWTreeNode*		here;		// Current node.
  RWGStack(RWTreeNode)		stack;
private:
  void				descendLeft();
public:
  RWBinaryTreeIterator(const RWBinaryTree&);
  virtual ~RWBinaryTreeIterator();

/*********** Virtual functions inherited from class RWIterator ***********/
  virtual RWCollectable*	findNext(const RWCollectable*);		// Find next matching item
  virtual RWCollectable*	key() const;				// Return current value
  virtual RWCollectable*	operator()();				// Advance iterator
  virtual void	   		reset();
};


/****************************************************************
 *								*
 *			RWTreeNode				*
 *								*
 ****************************************************************/

// Node in binary tree:
class RWExport RWTreeNode   RWMemoryPool_OPTION {
private:
friend class RWExport RWBinaryTree;
friend class RWExport RWBinaryTreeIterator;
  RWTreeNode*		right;	// Pointer to right node.
  RWTreeNode*		left;   // Pointer to left node.
  RWCollectable*	e;      // Pointer to RWCollectable object.
private:
  // Private constructor:
  RWTreeNode(RWCollectable* a, RWTreeNode* p=rwnil, RWTreeNode* n=rwnil) 
    { e = a; left = p; right = n; }
};

#endif /* __RWBINTREE_H__ */



