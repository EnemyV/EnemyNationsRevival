#ifndef __RWTPSLDICT_H__
#define __RWTPSLDICT_H__

/*
 * RWTPtrSlistDictionary<KP,VP>: A dictionary of pointers to keys of type KP,
 *   and pointers to values of type VP, implemented as a singly-linked list.
 *
 * $Id: tpsldict.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * Assumes that KP has:
 *
 *   - well-defined equality semantics (KP::operator==(const KP&)).
 *
 ***************************************************************************
 *
 * $Log: tpsldict.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/07/05  21:56:36  nevis
 * Fixes bug #117 by defining explicitly a copy constructor.
 *
 * Revision 6.1.7.2  1994/07/05  20:57:20  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.2  1994/07/05  20:57:17  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.1  1994/04/15  19:47:59  vriezen
 * Move all files to 6.1
 *
 * Revision 2.13  1994/01/12  17:58:25  jims
 * Add const to 'key' parameter of removeKeyAndValue
 *
 * Revision 2.12  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 2.11  1994/01/03  23:33:28  jims
 * ObjectStore version: Add #include <ostore/ostore.hh>
 *
 * Revision 2.10  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.9  1993/11/08  13:16:37  jims
 * Port to ObjectStore
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.6  1993/03/24  01:29:51  keffer
 * Indexing operations now used unsigned
 *
 * Revision 2.5  1993/02/17  20:25:41  keffer
 * Now uses RWTPtrAssocLink<KP,VP> rather than RWTValAssocLink<KP*,VP*>
 *
 * Revision 2.4  1993/02/12  23:33:45  keffer
 * Ported to g++ v2.3
 *
 * Revision 2.3  1993/02/12  00:18:50  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.2  1993/01/28  21:11:49  keffer
 * Ported to cfront V3.0
 *
 *    Rev 1.0   25 May 1992 15:59:24   KEFFER
 * Initial revision.
 * 
 */
#include "rw/tislist.h"
#include "rw/tasslnk.h"

template <class KP, class VP> class RWTExport RWTPtrSlistDictionaryIterator;

/****************************************************************
 *								*
 *	Declarations for RWTPtrSlistDictionary<KP,VP>		*
 *								*
 ****************************************************************/

template <class KP, class VP>
class RWTExport RWTPtrSlistDictionary 
                         : private RWTIsvSlist< RWTPtrAssocLink<KP, VP> >
{

public:

  RWTPtrSlistDictionary() {;}
  RWTPtrSlistDictionary(const RWTPtrSlistDictionary<KP,VP>&);
  ~RWTPtrSlistDictionary()
	{RWTIsvSlist<RWTPtrAssocLink<KP,VP> >::clearAndDestroy();}

  // Operators:
  RWTPtrSlistDictionary<KP,VP>&
			operator=(const RWTPtrSlistDictionary<KP,VP>&);
  VP*&			operator[](KP* key); // Look up key, return value

  // Member functions:
  void			applyToKeyAndValue(void (*applyFun)(KP*,VP*&,void*), void*);

  void			clear()
	{RWTIsvSlist<RWTPtrAssocLink<KP,VP> >::clearAndDestroy();}

  void			clearAndDestroy();

  RWBoolean		contains(const KP*) const;	// Contain key?

  size_t		entries() const
	{return RWTIsvSlist<RWTPtrAssocLink<KP,VP> >::entries();}

  RWBoolean		isEmpty() const
	{return RWTIsvSlist<RWTPtrAssocLink<KP,VP> >::isEmpty();}

  KP*			find(const KP* key) const;
  KP*		        findKeyAndValue(const KP* key, VP*& retVal) const;
  VP*			findValue(const KP* key) const;

  void			insertKeyAndValue(KP* key, VP* value)
  	{ (*this)[key] = value; }

  KP*			remove(const KP* key);
  KP*			removeKeyAndValue(const KP* key, VP*& retVal);

protected:

  RWTPtrAssocLink<KP,VP > *	findLink(const KP*) const;
  RWTPtrAssocLink<KP,VP > *	removeLink(const KP*);

private:

  friend class RWTExport RWTPtrSlistDictionaryIterator<KP,VP>;

};


/****************************************************************
 *								*
 *     Declarations for RWTPtrSlistDictionaryIterator<KP,VP>	*
 *								*
 ****************************************************************/

template <class KP, class VP>
class RWTExport RWTPtrSlistDictionaryIterator :
	private RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >
{

public:

  // Constructor:
  RWTPtrSlistDictionaryIterator(RWTPtrSlistDictionary<KP,VP>& d)
	: RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >(d) {;}

  RWTPtrSlistDictionaryIterator(const RWTPtrSlistDictionaryIterator<KP,VP>&);
    // Added to address BUG#117

  // Operators:
  RWBoolean	operator++()
	{advance(); return cursor()!=container()->tailLink();}
  RWBoolean	operator+=(size_t n)
	{return RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >::operator+=(n)!=rwnil;}
  KP*		operator()()
	{advance(); return cursor()==container()->tailLink() ? rwnil : cursor()->key_;}

  RWTPtrSlistDictionary<KP,VP>*	container() const
	{return (RWTPtrSlistDictionary<KP,VP>*)RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >::container();}

  KP*		key() const
	{return cursor()->key_;}

  void		reset()
	{RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >::reset();}
  void		reset(RWTPtrSlistDictionary<KP,VP>& s)
	{RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >::reset(s);}

  VP*		value() const
	{return cursor()->value_;}


private:

  // Disallow postfix increment.  Unless we hide it, some compilers will
  // substitute the prefix increment operator in its place.
  RWBoolean		operator++(int);
};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tpsldict.cc"
#endif

#endif	/* __RWTPSLDICT_H__ */

