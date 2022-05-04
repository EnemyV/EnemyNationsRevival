/*
 * Template definitions for RWTPtrSlistDictionary<K,V> and RWTPtrSlistDictionaryIterator<K,V>
 *
 * $Id: tpsldict.cc,v 6.4 1994/07/26 19:11:28 jims Exp $
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
 * $Log: tpsldict.cc,v $
 * Revision 6.4  1994/07/26  19:11:28  jims
 * Fix copy ctor to use passed in argument instead of *this
 *
 * Revision 6.3  1994/07/12  20:04:35  vriezen
 * Updated Copyright.
 *
 * Revision 6.2  1994/07/05  21:56:29  nevis
 * Fixes bug #117 by defining explicitly a copy constructor.
 *
 * Revision 6.1.7.2  1994/07/05  20:57:15  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.2  1994/07/05  20:56:37  nevis
 * Fixes bug #117 by defining copy constructor for SGI compiler..
 *
 * Revision 6.1  1994/04/15  19:47:56  vriezen
 * Move all files to 6.1
 *
 * Revision 1.9  1994/01/12  17:58:25  jims
 * Add const to 'key' parameter of removeKeyAndValue
 *
 * Revision 1.8  1994/01/12  03:09:17  jims
 * Add constness to T* parameters where appropriate
 *
 * Revision 1.7  1993/11/08  13:16:37  jims
 * Port to ObjectStore
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/09/07  19:41:18  griswolf
 *  fixed applyToKeyAndValue to correctly handle last item
 *
 * Revision 1.4  1993/02/18  23:29:41  keffer
 * Corrected syntax error in postcondition.
 *
 * Revision 1.3  1993/02/17  20:25:41  keffer
 * Now uses RWTPtrAssocLink<KP,VP> rather than RWTValAssocLink<KP*,VP*>
 *
 * Revision 1.2  1993/02/12  23:33:45  keffer
 * Ported to g++ v2.3
 *
 * Revision 1.1  1993/01/28  21:11:49  keffer
 * Ported to cfront V3.0
 *
 *
 ***************************************************************************
 */


// Copy constructor (some of these names get pretty bloody long, eh?)
template <class KP, class VP>
RWTPtrSlistDictionary<KP,VP>::RWTPtrSlistDictionary(const RWTPtrSlistDictionary<KP,VP>& d)  {

  // Cast away "constness", which we will honor anyway...
  RWTPtrSlistDictionaryIterator<KP,VP> next((RWTPtrSlistDictionary<KP,VP>&)d);

  while (++next)
    insertKeyAndValue(next.key(), next.value());

  RWPOSTCONDITION(entries()==d.entries());
}

template <class KP, class VP> RWTPtrSlistDictionary<KP,VP>&
RWTPtrSlistDictionary<KP,VP>::operator=(const RWTPtrSlistDictionary<KP,VP>& d)
{
  if (this!=&d)
  {

    clear();

    // Cast away "constness", which we will honor anyway...
    RWTPtrSlistDictionaryIterator<KP,VP> next((RWTPtrSlistDictionary<KP,VP>&)d);

    while (++next)
      insertKeyAndValue(next.key(), next.value());

  }
  RWPOSTCONDITION(entries()==d.entries());
  return *this;
}

template <class KP, class VP> VP*&
RWTPtrSlistDictionary<KP,VP>::operator[](KP* key)
{
  RWTPtrAssocLink<KP,VP>* assoc = findLink(key);

  if (assoc==rwnil)
    RWTIsvSlist<RWTPtrAssocLink<KP,VP> >::insert(assoc = new RWTPtrAssocLink<KP,VP>(key));

  RWPOSTCONDITION(*key == *(assoc->key_));
  return assoc->value_;
}

template <class KP, class VP> void
RWTPtrSlistDictionary<KP,VP>::applyToKeyAndValue(void (*applyFun)(KP*,VP*&,void*), void* a)
{
  RWTPtrAssocLink<KP,VP>* link = firstLink();
  while (link != tailLink())
  {
    applyFun(link->key_, link->value_, a);
    link = link->next();
  }
}

template <class KP, class VP> void
RWTPtrSlistDictionary<KP,VP>::clearAndDestroy()
{
  RWTPtrAssocLink<KP,VP>* link = firstLink();
  while (link != tailLink())
  {
    RWTPtrAssocLink<KP,VP>* next = link->next();
    delete link->key_;
    delete link->value_;
    delete link;
    link = next;
  }
  init();
}

template <class KP, class VP> RWBoolean
RWTPtrSlistDictionary<KP,VP>::contains(const KP* key) const
{
  return findLink(key)!=rwnil;
}

template <class KP, class VP> KP*
RWTPtrSlistDictionary<KP,VP>::find(const KP* key) const
{
  RWTPtrAssocLink<KP,VP>* assoc = findLink(key);
  return assoc ? assoc->key_ : rwnil;
}

template <class KP, class VP> KP*
RWTPtrSlistDictionary<KP,VP>::findKeyAndValue(const KP* key, VP*& retVal) const
{
  RWTPtrAssocLink<KP,VP>* assoc = findLink(key);
  if (assoc)
  {
    retVal = assoc->value_;
    RWPOSTCONDITION( *key == *assoc->key_ );
    return assoc->key_;
  }
  return rwnil;
}

template <class KP, class VP> VP*
RWTPtrSlistDictionary<KP,VP>::findValue(const KP* key) const
{
  RWTPtrAssocLink<KP,VP>* assoc = findLink(key);
  return assoc ? assoc->value_ : rwnil;
}

template <class KP, class VP> KP*
RWTPtrSlistDictionary<KP,VP>::remove(const KP* key)
{
  RWTPtrAssocLink<KP,VP>* assoc = removeLink(key);
  if (assoc)
  {
    KP* retKey = assoc->key_;
    delete assoc;
    RWPOSTCONDITION(*retKey == *key);
    return retKey;
  }
  return rwnil;
}

template <class KP, class VP> KP*
RWTPtrSlistDictionary<KP,VP>::removeKeyAndValue(const KP* key, VP*& retVal)
{
  RWTPtrAssocLink<KP,VP>* assoc = removeLink(key);
  if (assoc)
  {
    KP* retKey = assoc->key_;
    retVal     = assoc->value_;
    delete assoc;
    RWPOSTCONDITION(*retKey == *key);
    return retKey;
  }
  return rwnil;
}


// Protected functions ---

template <class KP, class VP> RWTPtrAssocLink<KP,VP>*
RWTPtrSlistDictionary<KP,VP>::findLink(const KP* key) const
{
  RWTPtrAssocLink<KP,VP>* link = firstLink();
  while (link != tailLink())
  {
    if (*link->key_ == *key)
      return link;
    link = link->next();
  }
  return rwnil;
}


template <class KP, class VP> RWTPtrAssocLink<KP,VP > *
RWTPtrSlistDictionary<KP,VP>::removeLink(const KP* key)
{
  RWPRECONDITION(lastLink()->next()==tailLink());

  // Set up a sentinel (casting away const: DO NOT MODIFY *newTail.key_):
  RWTPtrAssocLink<KP,VP> newTail((KP*) key);
  lastLink()->next_ = &newTail;

  // Now search for it:
  RWTPtrAssocLink<KP,VP>* link = headLink();
  while (!(*link->next()->key_ == *key))
    link = link->next();

  // Reset the pointer in the last link:
  lastLink()->next_ = tailLink();

  if (link != lastLink())
  {
    RWPOSTCONDITION(*link->next()->key_ == *key);
    return removeRight(link);
  }
  return rwnil;
}

// BUG 117
template <class KP, class VP>
RWTPtrSlistDictionaryIterator<KP,VP>::RWTPtrSlistDictionaryIterator
    (const RWTPtrSlistDictionaryIterator<KP,VP>& sdi)
  : RWTIsvSlistIterator<RWTPtrAssocLink<KP,VP> >(sdi)
{ ; }
