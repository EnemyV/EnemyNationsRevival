#ifndef __RWTVASSLNK_H__
#define __RWTVASSLNK_H__

/*
 * RWTValAssocLink: Key / Value association link using values
 * RWTPtrAssocLink: Key / Value association link using pointers
 *
 * $Id: tasslnk.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * Class RWTValAssocLink<K,V> defines an association between a key of
 * type K, and a value of type V in a singly-linked link, using value
 * semantics.
 *
 * It has a single value constructor that takes just the key.
 * This means that the value will be constructed using the default
 * constructor for type V.  Usually this works just fine.  However, if the
 * value (type V) is a builtin, then its value will be left undefined.
 * Usually this also works fine.  However, if this is unsatisfactory,
 * then you can supply your own definition that overrides the template-
 * generated definition.  For an explanation of user-specified overrides
 * of template-generated definitions, see Stroustrup II, sec. 8.4.1.
 *
 * Example:
 *
 *   RWTValAssocLink<int,double>::RWTValAssocLink(int i) :
 *     key_(i)
 *    {
 *      value_ = 0.0;	// Explicitly set the value to zero.
 *    }
 *
 * Class RWTPtrAssocLink<K,V> defines an association between a pointer to
 * a key of type K and a pointer to a value of type V in a singly-linked
 * link.
 *
 * It also has a single value constructor which takes just a pointer to
 * a key.  It always sets the value pointer to nil:
 *
 *   RWTPtrAssocLink<int,double>::RWTPtrAssocLink(int* i) :
 *     key_(i),
 *     value_(rwnil)
 *    {
 *    }
 *
 ***************************************************************************
 *
 * $Log: tasslnk.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:11:04  vriezen
 * Move all files to 6.1
 *
 * Revision 2.7  1993/12/31  00:02:53  jims
 * Left justify preprocessor directives
 *
 * Revision 2.6  1993/11/04  14:06:32  jims
 * Port to ObjectStore
 *
 * Revision 2.5  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.4  1993/02/17  20:26:07  keffer
 * Added class RWTPtrAssocLink<KP,VP>
 *
 * Revision 2.3  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.2  1993/01/28  21:11:49  keffer
 * Ported to cfront V3.0
 *
 *    Rev 1.0   02 Mar 1992 16:10:52   KEFFER
 * Initial revision.
 */

#ifndef __RWISLIST_H__
# include "rw/islist.h"
#endif

template <class K, class V> struct RWTExport RWTValAssocLink : public RWIsvSlink
{
  K		key_;
  V		value_;
  RWTValAssocLink(const K& key);
  RWTValAssocLink(const K& key, const V& value) : key_(key), value_(value) { }

  RWTValAssocLink<K,V>* next() const { return (RWTValAssocLink<K,V>*)next_;}

};

template <class K, class V> struct RWTExport RWTPtrAssocLink : public RWIsvSlink
{
  K*		key_;
  V*		value_;
  RWTPtrAssocLink(K* key) : key_(key), value_(rwnil) {;}
  RWTPtrAssocLink(K* key, V* value) : key_(key), value_(value) {;}

  RWTPtrAssocLink<K,V>* next() const { return (RWTPtrAssocLink<K,V>*)next_;}

};

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tasslnk.cc"
#endif

#endif	/* __RWTVASSLNK_H__ */
