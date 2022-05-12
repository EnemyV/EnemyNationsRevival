#ifndef __RWDISKTREE_H__
#define __RWDISKTREE_H__
/*
 * RWBTreeOnDisk -- disk-based B-Tree. Uses cache, filemanager.
 *
 * $Id: disktree.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
 *
 ***************************************************************************
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
 * $Log: disktree.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/02  16:43:44  foote
 * Port to Symantec 7.0 on Macintosh
 *
 * Revision 6.1  1994/04/15  19:09:50  vriezen
 * Move all files to 6.1
 *
 * Revision 2.13  1994/01/11  19:45:52  jims
 * ignoreNulls() now a "const" member function
 *
 * Revision 2.12  1994/01/03  18:54:44  griswolf
 * fix cacheCount() to have two prototypes
 *
 * Revision 2.11  1993/11/29  17:29:20  griswolf
 * added method rootLocation() to match the printed class reference.
 *
 * Revision 2.10  1993/09/14  18:19:50  griswolf
 * fixed problem handling imbedded nulls. Added unsigned long for flags.
 *
 * Revision 2.9  1993/09/11  22:13:11  griswolf
 * Changed constructor to match V6 reference manual
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/05/18  21:47:27  keffer
 * Fixed badly formed RCS identifier
 *
 * Revision 2.6  1993/04/22  20:29:55  griswolf
 * Changed interface to return RWBoolean from some members, to accept
 * RWCString arguments in some of those members. Renamed one findValue
 * to replaceValue.
 *
 * Revision 2.5  1993/04/21  23:51:35  griswolf
 * Moved RWDiskTreeNode from disktree.h to disktree.cpp,
 * changed several inline functions to called functions.
 *
 * Revision 2.4  1993/04/13  07:14:49  randall
 * modified constructor to work on different style BTrees on disk
 *
 * Revision 2.3  1993/02/24  16:19:32  griswolf
 * Replaces the old RWBTreeOnDisk with a new implementation that
 * has considerable flexibility wrt keylength, file usage, order.
 *
 * 
 *    Rev 1.5   09 Jun 1992 12:43:48   KEFFER
 * Made retStatus public to satisfy stricter scoping rules.
 * 
 *    Rev 1.4   25 May 1992 15:48:56   KEFFER
 * Complete rewrite, improving efficiency and maintainability.
 * 
 *    Rev 1.3   18 Feb 1992 09:54:16   KEFFER
 * 
 *    Rev 1.2   28 Oct 1991 09:08:12   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.1   28 Jul 1991 12:36:04   keffer
 * No longer uses macro "Const"
 * 
 *    Rev 1.0   28 Jul 1991 08:14:14   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#include "rw/cacheman.h"
#include "rw/filemgr.h"
#include "rw/mempool.h"
#include "rw/cstring.h"
#ifdef RDEBUG
#  include "rw/rstream.h"
#endif

typedef void	(*RWdiskTreeApply)  (const char*, RWstoredValue, void*);
extern "C" {
  typedef int	(*RWdiskTreeCompare)(const char*, const char*, size_t);
}

const RWstoredValue RWBTreeOnDiskCurrentVersion = 0x200;

class RWExport RWDiskTreeNode;
 
/****************************************************************
 *								*
 *			RWBTreeOnDisk				*
 *								*
 ****************************************************************/

class RWExport RWBTreeOnDisk {
friend class RWExport RWDiskTreeNode;

public:
  /* styleMode means:
   *  V6Style	   -- Use the V 6.x and above version. Default
   *  V5Style	   -- Use the V 5.x and below version.
   * createMode means:
   *  autoCreate   -- Look at start arg. If valid use, else create new
   *  create	   -- Ignore start arg. Build new tree in the FileManger
   */
  enum styleMode	{V6Style, V5Style};
  enum createMode	{autoCreate, create};

  RWBTreeOnDisk(RWFileManager&,	  // No default possible!
    unsigned cacheBlocks=10,	  // how many nodes to cache in RAM
    createMode omode=autoCreate,  // use existing data in filemgr?
    unsigned keylen=16,		  // default is 16 chars
    RWBoolean ignoreNull=FALSE,	  // imbedded nulls ok? default to "no"
    RWoffset start=RWNIL,	  // if RWNIL, use fmanager's start();
    styleMode smode=V6Style,	  // default to current style
    unsigned order=10,		  // default is 10-20 tree
    unsigned minorder=10	  // default == order, may be less
    );	

  ~RWBTreeOnDisk();

  void			applyToKeyAndValue(RWdiskTreeApply ap, void* x)
	{ apl(baseInfo.rootLoc, ap, x); }
  RWoffset		baseLocation() const
	{ return baseLoc; }
  void			clear();
  unsigned		cacheCount() const { return cacheBlocks; }
  unsigned		cacheCount(unsigned blocks);
  RWBoolean		contains(const char* key) const
	{RWCString rK; RWstoredValue rV; return (findKeyAndValue(key, rK, rV) ? TRUE : FALSE); }
  unsigned long		entries() const;		// Total entries
  RWoffset		extraLocation(RWoffset off)
	{ RWoffset r = baseInfo.extraLoc; baseInfo.extraLoc=off; return r; }
  RWBoolean		find(const char* key) const
	{ return contains(key); }
  RWBoolean		findKey(const char* key, RWCString& retKey) const
	{ RWstoredValue d; return findKeyAndValue(key,retKey, d); }
  RWBoolean		findKeyAndValue(const char* key, RWCString&, RWstoredValue&) const;
  RWstoredValue		findValue(const char* key) const
	{ RWCString retK; RWstoredValue ret; return(findKeyAndValue(key,retK,ret)) ? ret : RWNIL ;}
  unsigned		height() const;
  RWBoolean		insertKeyAndValue(const char*key, RWstoredValue val);
  RWBoolean		isEmpty() const
	{ return baseInfo.rootLoc == RWNIL; }
  unsigned		keyLength() const
	{ return baseInfo.keylen; }
  unsigned		minOrder() const
	{ return baseInfo.minorder; }
  unsigned		nodeSize();
  unsigned		occurrencesOf(const char* key) const
	{ return contains(key) ? 1 : 0; }
  unsigned		order() const
	{ return baseInfo.order; }
  void			remove(const char* key)
	{ RWstoredValue d; RWCString k; removeKeyAndValue(key,k,d); }
  RWBoolean		removeKeyAndValue(const char* key, RWCString& retKey, RWstoredValue& retVal);
  RWBoolean		removeKeyAndValue(const char* key, RWstoredValue& retVal)
	{ RWCString retK; return removeKeyAndValue(key, retK, retVal); }
  RWBoolean		replaceValue(const RWCString& key, const RWstoredValue newval, RWstoredValue& oldVal);
  RWoffset		rootLocation() const
	{ return baseLoc; }
  RWdiskTreeCompare	setComparison(RWdiskTreeCompare cf);
  RWstoredValue		version()
	{ return (RWNIL == baseInfo.version) ? 0 : baseInfo.version; }
#ifdef RDEBUG
  void treePrint()    //for debugging purposes
    { cout <<endl<<"\t\tRWBTreeOnDisk::treePrint()"<<endl; tp(baseInfo.rootLoc);}
private:
  void tp(RWoffset);   //recursive function for treePrint
#endif

public:

  /******* used as signal during recursion unwrap ******/
  enum retStatus {more, success, ignored};

private:
  RWDiskTreeNode*	root;			// root = first node in tree.
  RWDiskTreeNode*	workNode;		// node currently in memory
  RWoffset		workOffset;		// that node's disk location
  RWdiskTreeCompare	compareKeys;		// Compare function for keys
  char*			moreKey;		// Hold over/under-flow key
  RWstoredValue		moreItem;		// Hold over/under-flow item
  RWoffset		moreOffset;		// Hold o/u-flow disk offset
  RWoffset		baseLoc;		// offset to baseInfo in fmgr

  // information about the nodeRef pseudo-struct:
  unsigned		nodeRefSize;

private:
  // recursive functions to: apply, delete, insert or remove
  void			apl(RWoffset, RWdiskTreeApply, void*);
  void			del(RWoffset);
  retStatus		ins(const char* key, RWstoredValue val, RWoffset start);
  retStatus		rem(const char* key, RWoffset start, RWCString&, RWstoredValue&);
  // utility functions
  void			combineNodes(int,RWoffset);// opposite of splitNode
  void			moveItLeft(int,RWoffset);  // lengthen left sib node
  void			moveItRight(int,RWoffset); // lengthen right sib node
  void			readcache(RWoffset, RWDiskTreeNode*) const;
  void			writecache(RWoffset, RWDiskTreeNode*);
  void			readRoot();		// get root node from file
  retStatus		restoreNode(int);	// if node got too small
  void			splitNode(int loc);	// split at location loc
  void			swapWithSuccessor(int); // uses moreXXX
/*
 * The next two functions are private to prevent copies: only one
 * RWBTreeOnDisk can look at the data file at a time. Implementing locks so
 * that multiple accessors to the same data could co-exist might be 
 * interesting.
 */
  void			operator=(const RWBTreeOnDisk&);
  RWBTreeOnDisk(const RWBTreeOnDisk&);
      
protected:
  RWFileManager*	fmgr;			// knows where data is 
  RWCacheManager*	cmgr;			// optimizes reads/writes
  unsigned		cacheBlocks;		// how many nodes cmgr keeps
  struct baseInfoStruct {
    RWoffset		rootLoc;	// offset to root node in file
    RWoffset		extraLoc;	// offset to "extra" info in file
    union {
      RWstoredValue	version;	// version of this B-Tree
      RWstoredValue	cacheFlag;	// RWNIL if entries() not cached
    };
    unsigned long	entries;	// number of items in this B-Tree
    unsigned		keylen;		// length of keys
    unsigned		order;		// 1/2 max  entries per node
    unsigned		minorder;	// fewest allowed entries per node
    RWoffset		reserved1;	// In case we ever want it
    RWstoredValue	reserved2;	// ditto
    unsigned long	flags;		// holds ignoreNulls for now
  } baseInfo;
protected:
  /* ignoreNulls() prototypes getting individual flags from flags */
  RWBoolean		ignoreNulls() const
  { return baseInfo.flags & 1 ? TRUE : FALSE; }
  void			infoInit(RWstoredValue version,
				  unsigned keylen,
				  unsigned order,
				  unsigned minorder,
				  RWBoolean ignoreNull
				 );
  void			infoReInit();
  void			readInfo();
  void			startup();
  void			writeInfo() const;	// has version knowledge

};    

#endif /*  __RWDISKTREE_H__ */


