#ifndef __RWTVRTARRY_H__
#define __RWTVRTARRY_H__

/*
 * RWTValVirtualArray<T>: A swapping virtual array of type T
 *
 * $Id: tvrtarry.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: tvrtarry.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:48:29  vriezen
 * Move all files to 6.1
 *
 * Revision 2.9  1993/12/31  00:56:30  jims
 * ObjectStore version: add get_os_typespec() static member function
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/02/17  18:32:03  keffer
 * Now passes T's by const reference, rather than by value
 *
 * Revision 2.6  1993/02/12  01:34:44  keffer
 * Ported to the IBM xlC compiler
 *
 * Revision 2.5  1993/02/07  23:58:09  keffer
 * Ported back to BCC3.1
 *
 * Revision 2.4  1993/02/07  23:37:31  keffer
 * Changed include line for compile-time instantiation.
 *
 * Revision 2.3  1993/02/06  02:06:43  keffer
 * Rearranged things to port to cfront V3.0.
 *
 *    Rev 1.0   11 Mar 1992 14:10:00   KEFFER
 * Initial revision.
 */

#include "rw/tvref.h"

template <class T> class RWTVirtualElement;
template <class T> class RWTVirtualSlice;

/****************************************************************
 *								*
 *		Declaration of RWTValVirtualArray<T>		*
 *								*
 ****************************************************************/

template <class T> class RWTValVirtualArray
{

public:

  RWTValVirtualArray(long size, RWVirtualPageHeap*);
  RWTValVirtualArray(const RWTValVirtualArray<T> &);
  RWTValVirtualArray(const RWTVirtualSlice<T> &);
  ~RWTValVirtualArray();

  RWTValVirtualArray<T>& operator=(const RWTValVirtualArray<T>&);
  void			operator=(const RWTVirtualSlice<T>&);
  T			operator=(const T&);

  long			length() const			// Length of vector
		  		{return vref_->length();}
  T			val(long i) const		// Readonly access
  				{return vref_->val(i);}
  void			set(long i, const T& v)		// Set a value
  				{cow(); vref_->set(i, v);}
  T			operator[](long i) const	// Readonly access
  				{return vref_->val(i);}
  inline RWTVirtualElement<T>
			operator[](long);	// Element as lvalue
  inline RWTVirtualSlice<T>
			slice(long start, long length);	// Slice as lvalue
  void			reshape(long newLength)
  				{cow(); vref_->reshape(newLength);}
  RWVirtualPageHeap*	heap() const
  				{return vref_->heap();}
private:

  RWTVirtualRef<T>*	vref_;

  void			cow();		// Copy On Write
friend class RWTVirtualElement<T>;
friend class RWTVirtualSlice<T>;
};


/****************************************************************
 *								*
 *		Declaration of RWTVirtualElement<T>		*
 *								*
 ****************************************************************/

template <class T> class RWTVirtualElement
{

public:

  // These workarounds necessary in order to satisfy both Borland
  // and cfront V3.0 ---
#ifdef RW_BROKEN_TEMPLATES
  inline operator	T() const;
  inline T		operator=(const T& v);
#else
  operator	T() const             {return varray_->val(start_);}
  T		operator=(const T& v) {varray_->set(start_, v); return v;}
#endif

protected:

  // Protected constructor:
  RWTVirtualElement
  (
    RWTValVirtualArray<T>* v,
    long s
  ) : varray_(v), start_(s) { }

  RWTValVirtualArray<T>*	varray_;
  long				start_;

private:

friend class RWTValVirtualArray<T>;

};


/****************************************************************
 *								*
 *		Declaration of RWTVirtualSlice<T>		*
 *								*
 ****************************************************************/

template <class T> class RWTVirtualSlice : public RWTVirtualElement<T>
{

public:

  void			operator=(const RWTVirtualSlice<T>&);
  void			operator=(const RWTValVirtualArray<T>&);
  T			operator=(const T& val);

protected:

  // Protected constructor:
  RWTVirtualSlice
  (
    RWTValVirtualArray<T>* v,
    long s,			// Slice start
    long e			// Slice extent
  ) : RWTVirtualElement<T>(v,s),
      extent_(e)
  {;}

  long			extent_;

private:

friend class RWTValVirtualArray<T>;

};

/****************************************
 *					*
 *		INLINES			*
 *					*
 ****************************************/

template <class T>
inline RWTVirtualElement<T> RWTValVirtualArray<T>::operator[](long i)
{
  return RWTVirtualElement<T>(this,i);
}

template <class T>
inline RWTVirtualSlice<T> RWTValVirtualArray<T>::slice(long start, long length)
{
  return RWTVirtualSlice<T>(this,start,length);
}

#ifdef RW_BROKEN_TEMPLATES
template <class T>
inline RWTVirtualElement<T>::operator T() const
{
  return varray_->val(start_);
}

template <class T>
inline T RWTVirtualElement<T>::operator=(const T& v)
{
  varray_->set(start_, v); return v;
}
#endif

#ifdef RW_COMPILE_INSTANTIATE
# include "rw/tvrtarry.cc"
#endif

#endif	/* __RWTVRTARRY_H__ */
