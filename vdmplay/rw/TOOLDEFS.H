#ifndef __RWTOOLDEFS_H__
#define __RWTOOLDEFS_H__

/*
 * General declarations for Tools.h++
 *
 * $Id: tooldefs.h,v 6.4 1994/07/18 15:20:14 jims Exp $
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
 * $Log: tooldefs.h,v $
 * Revision 6.4  1994/07/18  15:20:14  jims
 * Update version to 610
 *
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/30  20:38:00  vriezen
 * Change RWhashAddress(void *) to RWhashAddress(const void *)
 *
 * Revision 6.1  1994/04/15  19:11:15  vriezen
 * Move all files to 6.1
 *
 * Revision 2.18  1994/03/02  01:15:42  jims
 * Update version number to 0x0604
 *
 * Revision 2.17  1994/01/25  22:54:09  jims
 * Update version number
 *
 * Revision 2.16  1993/11/15  02:21:45  keffer
 * Updated version number
 *
 * Revision 2.15  1993/11/07  17:26:48  keffer
 * Updated version number
 *
 * Revision 2.14  1993/09/16  04:14:53  keffer
 * Changed prototype for RWbsearch to satisfy CSet++
 *
 * Revision 2.13  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.12  1993/09/03  00:00:19  keffer
 * Added a declaration for function rwToolsVersion().
 *
 * Revision 2.11  1993/06/06  00:16:17  keffer
 * rewrote RWhashAddress() to work on DEC Alpha and other 64 bit
 * architectures.
 *
 * Revision 2.10  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.9  1993/04/09  02:49:27  keffer
 * Added RWClassID for generics.
 *
 * Revision 2.8  1993/03/24  23:46:40  keffer
 * Changed function prototype for RWbsearch()
 *
 * Revision 2.7  1993/03/17  21:05:21  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.6  1993/03/05  02:49:13  keffer
 * Added explicit casts and parenthesis to quiet fussy Borland compiler.
 *
 * Revision 2.5  1993/02/12  01:24:29  keffer
 * Improved flat-model identity hashing algorithm.
 *
 * Revision 2.4  1993/01/25  23:50:56  keffer
 * Upgraded version number.
 *
 * Revision 2.3  1993/01/25  22:15:42  keffer
 * Added version number RWTOOLS.
 *
 * Revision 2.1  1992/11/16  04:08:15  keffer
 * RWNO_STD_TYPEDEFS became RW_STD_TYPEDEFS
 * Moved include <stddef.h> to <rw/defs.h>.
 * Added header file backward.h
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.9   28 May 1992 15:36:04   KEFFER
 * Introduced RWhashAddress() for identity hashes
 * 
 *    Rev 1.8   28 May 1992 11:05:28   KEFFER
 * Added rwnew macro.
 * 
 *    Rev 1.7   13 May 1992 10:29:04   KEFFER
 * Changed typedef userCreator to RWuserCreator; left backwards compat. stubs.
 * 
 *    Rev 1.6   17 Mar 1992 12:26:50   KEFFER
 * Removed RWTV3_COMPATIBLE macro hooks.
 * 
 *    Rev 1.5   21 Feb 1992 12:42:56   KEFFER
 * Added RWNO_STD_TYPEDEFS directive.
 * 
 *    Rev 1.4   13 Nov 1991 11:45:52   keffer
 * Added Class ID's for V4.1 new objects.
 * 
 *    Rev 1.3   28 Oct 1991 09:08:26   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.2   08 Sep 1991 13:43:42   keffer
 * Added STARTWRAP/ENDWRAP wrapper around stddef.h for Glockenspiel
 * 
 *    Rev 1.1   28 Jul 1991 12:35:34   keffer
 * Now includes <stddef.h>.
 * 
 *    Rev 1.0   28 Jul 1991 08:17:44   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

#define RWTOOLS 0x0610	/* Version number */

#ifndef __RWDEFS_H__
#  include "rw/defs.h"
#endif

#if defined(RWMEMCK) && defined(__cplusplus)
#  define rwnew new(__FILE__, __LINE__)
   void* operator new(size_t size, const char* filename, int line);
#else
#  define rwnew new
#endif

// Function to return the version of Tools.h++:
unsigned rwexport rwToolsVersion();

class RWExport RWCollectable;

#ifdef RW_STD_TYPEDEFS

/*
 * Standard Smalltalk interface, activated by defining
 * RW_STD_TYPEDEFS before including this file.
 */

class RWExport RWBag;
class RWExport RWBagIterator;
class RWExport RWBinaryTree;
class RWExport RWBinaryTreeIterator;
class RWExport RWCollectableDate;
class RWExport RWCollectableInt;
class RWExport RWCollectableString;
class RWExport RWCollectableTime;
class RWExport RWCollection;
class RWExport RWDlistCollectables;
class RWExport RWDlistCollectablesIterator;
class RWExport RWHashDictionary;
class RWExport RWHashDictionaryIterator;
class RWExport RWIdentityDictionary;
class RWExport RWIdentitySet;
class RWExport RWOrdered;
class RWExport RWOrderedIterator;
class RWExport RWSequenceable;
class RWExport RWSet;
class RWExport RWSetIterator;
class RWExport RWSlistCollectables;
class RWExport RWSlistCollectablesIterator;
class RWExport RWSlistCollectablesQueue;
class RWExport RWSlistCollectablesStack;

typedef RWBag				Bag;
typedef RWBagIterator			BagIterator;
typedef RWBinaryTree			SortedCollection;
typedef RWBinaryTreeIterator		SortedCollectionIterator;
typedef RWCollectable			Object;			// Careful: All-to-common type
typedef RWCollectableDate		Date;
typedef RWCollectableInt		Integer;
typedef RWCollectableString		String;
typedef RWCollectableTime		Time;
typedef RWCollection			Collection;
typedef RWHashDictionary		Dictionary;
typedef RWHashDictionaryIterator	DictionaryIterator;
typedef RWIdentityDictionary		IdentityDictionary;
typedef RWIdentitySet			IdentitySet;
typedef RWOrdered			OrderedCollection;
typedef RWOrderedIterator		OrderedCollectionIterator;
typedef RWSequenceable			SequenceableCollection;
typedef RWSet 				Set;
typedef RWSetIterator			SetIterator;
typedef RWSlistCollectables		LinkedList;
typedef RWSlistCollectablesIterator	LinkedListIterator;
typedef RWSlistCollectablesQueue	Queue;
typedef RWSlistCollectablesStack	Stack;

#endif	/* RW_STD_TYPEDEFS */

/*
 * Class ID definitions:
 */

#define __RWUNKNOWN                     0x8000
#define __RWGDLIST                      0x8014
#define __RWGDLISTITERATOR              0x8015
#define __RWGQUEUE                      0x8016
#define __RWGSTACK                      0x8017
#define __RWGVECTOR                     0x8018
#define __RWBAG                         0x801b
#define __RWBAGITERATOR                 0x801c
#define __RWBINARYTREE                  0x8001
#define __RWBINARYTREEITERATOR          0x8002
#define __RWBTREE                       0x8003
#define __RWBTREEDICTIONARY             0x8004
#define __RWBTREENODE                   0x8005
#define __RWBTREEONDISK                 0x8006
#define __RWCOLLECTABLE                 0x8007
#define __RWCOLLECTABLEASSOCIATION      0x8008
#define __RWCOLLECTABLEDATE             0x8009
#define __RWCOLLECTABLEINT              0x800a
#define __RWCOLLECTABLESTRING           0x800b
#define __RWCOLLECTABLETIME             0x800c
#define __RWCOLLECTION                  0x801d
#define __RWDATE                        0x801e
#define __RWDISKTREENODE                0x800d
#define __RWDLINK                       0x800e
#define __RWDLIST                       0x800f
#define __RWDLISTCOLLECTABLES           0x8010
#define __RWDLISTCOLLECTABLESITERATOR   0x8011
#define __RWDLISTITERATOR               0x8012
#define __RWFACTORY                     0x801f
#define __RWFILEMANAGER                 0x8013
#define __RWFREELISTDATA                0x8021
#define __RWHASHDICTIONARY              0x8022
#define __RWHASHDICTIONARYITERATOR      0x8023
#define __RWIDENTITYDICTIONARY          0x8024
#define __RWIDENTITYSET                 0x8025
#define __RWINTEGER                     0x8026
#define __RWITERATOR                    0x8027
#define __RWORDERED                     0x8028
#define __RWORDEREDITERATOR             0x8029
#define __RWSEQUENCEABLE                0x802a
#define __RWSET                         0x802b
#define __RWSETITERATOR                 0x802c
#define __RWSLINK                       0x8031
#define __RWSLIST                       0x8032
#define __RWSLISTCOLLECTABLES           0x8033
#define __RWSLISTCOLLECTABLESITERATOR   0x8034
#define __RWSLISTCOLLECTABLESQUEUE      0x8035
#define __RWSLISTCOLLECTABLESSTACK      0x8036
#define __RWSLISTITERATOR               0x8037
//#define __RWSTRING                      0x802d
#define __RWSTRINGREF                   0x802e
//#define __RWSUBSTRING                   0x802f
#define __RWTIME                        0x8030
#define __RWTREENODE                    0x8038

// --- V4.0 new objects ---
#define __GBITVEC			0x8040
#define __GSLIST			0x8041
#define __GSLISTITERATOR		0x8042
#define __GSORTEDVECTOR			0x8043
#define __RWBITREF			0x8045
#define __RWBITVEC			0x8046
#define __RWCACHEMANAGER		0x8048
#define __RWREGEXP			0x8049
#define __RWSORTEDVECTOR		0x804a
#define __RWTOKENIZER			0x804b

// --- V4.1 new objects ---
#define __RWDLLDICTIONARY		0x804c
#define __RWDLLSET			0x804d
#define __RWFACTORYMANAGER		0x804e
#define __RWINSTANCEDATA		0x804f
#define __RWINSTANCEDATAMANAGER		0x8050
#define __RWINSTANCEMANAGER		0x8051
#define __RWMODEL			0x8052
#define __RWPROCESS			0x8053
#define __RWPROCESSMANAGER		0x8054
#define __RWREADMANAGER			0x8055
#define __RWREADTABLE			0x8056
#define __RWSTOREMANAGER		0x8057
#define __RWSTORETABLE			0x8058
#define __RWTHREAD			0x8059

// --- V5.0 new objects ---
#define __RWCSTRING			0x802d	/* Recycled from RWString */
#define __RWCSUBSTRING			0x802f	/* Recycled from RWSubString */
#define __RWBUFFER			0x805a
#define __RWTEMPLATE			0x805b
#define __RWVIRTUALREF			0x805c
#define __RWDISKPAGEHEAP		0x805d
#define __RWHASHTABLE			0x805e
#define __RWCOLLECTABLEIDASSOCIATION	0x805f

// --- V6.0 new objects ---
#define __RWCREGEXP			0x8049	/* Recycled from RWRegexp */
#define __RWWSTRING			0x8060
#define __RWWSUBSTRING			0x8061

/*
 * Misc. typedefs:
 */
typedef RWCollectable*	RWCollectableP;
typedef RWCollectable*	(*RWuserCreator)();
typedef long		RWstoredValue;	// Type of value in disk dictionaries.
typedef long		RWoffset;	// Used for file offsets.
typedef void*		RWvoid;		// Useful for arrays of void*.
typedef unsigned long	RWspace;	// Size of file allocations

/*
 * Pointer to functions to test and compare objects.
 */
typedef RWBoolean  (*RWtestGeneric)     (const void*, const void*);
typedef RWBoolean  (*RWtestCollectable) (const RWCollectable*, const void*);
typedef int        (*RWcompare)         (const void*, const void*);

/*
 * Pointer to functions to apply operation to objects: 
 */
typedef void (*RWapplyGeneric)( void*, void*);
typedef void (*RWapplyCollectable)(RWCollectable*, void*);

/*
 * Prototype for the binary search routine, used for insertions and finds:
 */
extern "C" RWBoolean rwexport
RWbsearch(const void* key, const void* base, size_t nelem, size_t width, 
          RWcompare cmf, size_t&);

/*
 * Function used to hash a pointer value.  Choose an appropriate 
 * algorithm based on whether a pointer will fit into a word:
 */

#if RW_BYTES_PER_PTR != RW_BYTES_PER_WORD

# define RW_IDHASH_SHIFT ((RW_BYTES_PER_PTR - RW_BYTES_PER_WORD) << 3)
inline unsigned RWhashAddress(const void* a)
{
  // Algorithm for where word size is not equal to pointer size.
  // This works for compact and large models under MS-DOS as well.
  return (unsigned)((unsigned long)a                 )
       ^ (unsigned)((unsigned long)a >> RW_IDHASH_SHIFT);
# undef RW_IDHASH_SHIFT
}

#else

inline unsigned RWhashAddress(const void* a)
{
  // For other addresses, mix in some higher order bits
  // into the lower order bits:
  return (unsigned)a ^ ((unsigned)a>>3);
}

#endif

// Declarations for backwards compatibility:
#if defined(RWTOOLS4_COMPATIBLE) || defined(RWTOOLS5_COMPATIBLE)
#  include "rw/backward.h"
#endif

#endif /* __RWTOOLDEFS_H__ */
