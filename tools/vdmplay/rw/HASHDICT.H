#ifndef  __RWHASHDICT_H__
#define  __RWHASHDICT_H__

/*
 * RWHashDictionary -- dictionary implemented using a hash table
 *
 * $Id: hashdict.h,v 6.3 1994/07/12 18:46:56 vriezen Exp $
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
 * $Log: hashdict.h,v $
 * Revision 6.3  1994/07/12  18:46:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/18  02:22:57  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.1  1994/04/15  19:10:14  vriezen
 * Move all files to 6.1
 *
 * Revision 2.6  1994/03/03  00:16:45  jims
 * Override operator== so that the correct (non-virtual) operator<=
 * gets called
 *
 * Revision 2.5  1993/09/15  01:38:57  keffer
 * Added isEqual().
 *
 * Revision 2.4  1993/08/21  22:09:52  keffer
 * Changed return type of select().
 *
 * Revision 2.3  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.1  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.7   04 Aug 1992 18:59:50   KEFFER
 * Added keyword "virtual" to destructor for documentation purposes.
 * 
 *    Rev 1.6   25 May 1992 15:50:30   KEFFER
 * Optimized, reducing size.
 * 
 *    Rev 1.5   22 May 1992 17:04:12   KEFFER
 * Now uses RWDECLARE_COLLECTABLE() macro
 * 
 *    Rev 1.3   29 Apr 1992 14:51:18   KEFFER
 * Hashing now uses chaining to resolve collisions
 * 
 *    Rev 1.2   21 Feb 1992 12:31:04   KEFFER
 * RWCollection::insert() is now private.
 * 
 *    Rev 1.1   28 Oct 1991 09:08:18   keffer
 * Changed inclusions to <rw/xxx.h>
 * 
 *    Rev 1.0   28 Jul 1991 08:15:12   keffer
 * Tools.h++ V4.0.5 PVCS baseline version
 *
 */

/*
 * For the storage and retrieval of (key, value) pairs.
 */

#include "rw/rwset.h"

class RWExport RWCollectableAssociation;

/****************************************************************
 *								*
 *			RWHashDictionary			*
 *								*
 ****************************************************************/

class RWExport RWHashDictionary : public RWSet
{

  friend class RWExport RWHashDictionaryIterator;
  RWDECLARE_COLLECTABLE(RWHashDictionary)

public:

  RWHashDictionary(size_t N = RWCollection::DEFAULT_CAPACITY);
  RWHashDictionary(const RWHashDictionary&);
  virtual ~RWHashDictionary();

  void			operator=(const RWHashDictionary&);
  RWBoolean		operator<=(const RWHashDictionary&) const;
  RWBoolean		operator==(const RWHashDictionary&) const;

/***********************************************************************/

  void				applyToKeyAndValue(RWapplyKeyAndValue, void*);
  virtual void			clear();
  virtual void			clearAndDestroy();			// Deletes all keys AND values.
  virtual RWCollectable*	find(const RWCollectable*) const;		// Returns key
  RWCollectable*		findKeyAndValue(const RWCollectable* key, RWCollectable*& value) const;
  RWCollectable*		findValue(const RWCollectable*) const;	// Returns value
  RWCollectable*		findValue(const RWCollectable*, RWCollectable*) const; // Replace value.
  RWCollectable*		insertKeyAndValue(RWCollectable* key, RWCollectable* value); // Returns key.
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual RWCollectable*	remove(const RWCollectable* key);		// Returns key.
  virtual void			removeAndDestroy(const RWCollectable*);	// Deletes key AND value
  RWCollectable*		removeKeyAndValue(const RWCollectable* key, RWCollectable*& value);
  virtual RWCollection*		select(RWtestCollectable tfn, void* x) const;

protected:

  void				copyAssociations();
  virtual RWCollectableAssociation*
  				findAssociation(const RWCollectable* key) const;
  virtual RWCollectableAssociation*	
  				newAssociation(RWCollectable*, RWCollectable*) const;
  virtual RWCollectableAssociation*
  				removeAssociation(const RWCollectable* key);

private:

  virtual RWCollectable*	insert(RWCollectable* a){return RWSet::insert(a);}

};    


/****************************************************************
 *								*
 *		RWHashDictionaryIterator			*
 *								*
 ****************************************************************/

class RWExport RWHashDictionaryIterator : private RWSetIterator {

public:

  RWHashDictionaryIterator(RWHashDictionary& hd) : RWSetIterator(hd) { }
  virtual ~RWHashDictionaryIterator();

/*********** Virtual functions inherited from class RWIterator ***********/
  RWCollectable*		findNext(const RWCollectable*);	// Find next matching item; return key
  RWCollectable*		key() const;			// Return current key
  RWCollectable*   		operator()();			// Advance iterator, return key
#ifdef RW_NO_ACCESS_ADJUSTMENT
  void				reset() { RWHashTableIterator::reset(); }
#else
  RWSetIterator::reset;
#endif

/******************* Special iterator functions *****************************/

  RWCollectable*		remove();			// Remove current item, return key
  RWCollectable*		removeNext(const RWCollectable*);	// Remove next matching item, return key
  RWCollectable*		value() const;			// Return current value
  RWCollectable*		value(RWCollectable*) const;	// Replace & return current value
};

#endif /* __RWHASHDICT_H__ */
