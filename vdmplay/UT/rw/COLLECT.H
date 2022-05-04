#ifndef __RWCOLLECT_H__
#define __RWCOLLECT_H__

/*
 * Declarations for RWCollectables: an abstract base class.
 *
 * $Id: collect.h,v 6.6 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: collect.h,v $
 * Revision 6.6  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.5  1994/07/11  19:14:18  vriezen
 * Using RW_INLINE_FRIEND. Bug# 670
 *
 * Revision 6.4  1994/06/13  20:11:51  vriezen
 * Bug #481.  Fixed oper>> when rhs operand is RWCollection*
 *
 * Revision 6.3  1994/06/13  20:07:28  jims
 * Now always defines RWnilCollectable as a function call that returns
 * the nil collectable object.  Set on first reference to avoid static init
 *
 * Revision 6.2  1994/05/16  18:23:45  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:09:39  vriezen
 * Move all files to 6.1
 *
 * Revision 2.14  1993/09/03  02:08:13  keffer
 * Macro _CLASSDLL is now named _RWTOOLSDLL
 *
 * Revision 2.13  1993/08/05  11:49:12  jims
 * Distinguish between using a WIN16 DLL from a WIN32 DLL by
 * checking for __WIN16__
 *
 * Revision 2.12  1993/08/03  18:40:35  dealys
 * Ported to MPW C++ 3.3 -- RW_BROKEN_TOKEN_PASTE
 *
 * Revision 2.11  1993/04/22  20:38:48  keffer
 * Persistence operators now store and restore nil pointers.
 *
 * Revision 2.10  1993/04/12  12:28:28  jims
 * Use global nil object for MULTI_THREAD, but not for Windows 3.x DLL
 *
 * Revision 2.9  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.8  1993/04/09  02:50:00  keffer
 * Added support for operator<< and >> for virtual streams and RWFile.
 *
 * Revision 2.7  1993/04/08  16:44:42  keffer
 * Cleanup and removed V4.X support.
 *
 * Revision 2.6  1993/03/17  21:07:06  keffer
 * Return type of binaryStoreSize() is now RWspace
 *
 * Revision 2.3  1993/03/13  00:17:34  keffer
 * UNDEFINED_REFERENCE_BUG becomes RW_UNDEFINED_REFERENCE_BUG
 *
 * Revision 2.2  1993/02/03  00:01:56  jims
 * Removed #include directive for procinit.h
 *
 *    Rev 1.12   09 Sep 1992 12:47:10   KEFFER
 * Changed macro "Init" to "RWInit" to avoid conflict with JPI header.
 * 
 *    Rev 1.6   21 Feb 1992 12:29:26   KEFFER
 * nil collectable now defined when using the DLL.
 * 
 *    Rev 1.4   13 Nov 1991 11:07:42   keffer
 * More robust DLL code; No longer declares GVector(RWCollectable); store and
 * read tables now maintained by a separate manager.
 * 
 */

#ifndef __RWTOOLDEFS_H__
#  include "rw/tooldefs.h"
#endif
#ifndef __RWMEMPOOL_H__
#  include "rw/mempool.h"
#endif
#ifndef __RWGENERIC_H__
#  include "rw/generic.h"		/* Looking for name2() */
#endif
#ifdef RW_UNDEFINED_REFERENCE_BUG
#  include "rw/rstream.h"
#endif

/************************************************************************
 *									*
 * The macro RWDECLARE_ABSTRACT_COLLECTABLE should be included in the   *
 * declaration of any abstract class that derives from RWCollectable.	*
 *									*
 ************************************************************************/

#define RWDECLARE_ABSTRACT_COLLECTABLE(className)			\
  public:								\
    RW_INLINE_FRIEND RWvistream& operator>>(RWvistream& s, className*& pCl)\
      { pCl = (className*)RWCollectable::recursiveRestoreFrom(s); return s; }\
    RW_INLINE_FRIEND RWFile&     operator>>(RWFile& f,     className*& pCl)\
      { pCl = (className*)RWCollectable::recursiveRestoreFrom(f); return f; }

/********

****NOTE****: , friend code above is duplicated in next macro.  This
allows future additions to RWDECLARE_ABSTRACT_COLLECTABLE to be
independant of RWDECLARE_COLLECTABLE.  (The alternative of having
RWDECLARE_COLLECTABLE invoke RWDECLARE_ABSTRACT_COLLECTABLE did not
seem right.  Having both macros invoke a third (i.e DECLARE_COMMON) 
seems like too much.)

*******/

/************************************************************************
 *									*
 * The macro RWDECLARE_COLLECTABLE should be included in the declaration*
 * of any class that derives from RWCollectable.			*
 *									*
 ************************************************************************/

#define RWDECLARE_COLLECTABLE(className)				\
  public:								\
    virtual RWCollectable*	newSpecies() const;			\
    virtual RWCollectable*	copy() const;				\
    virtual RWClassID		isA() const;				\
    RW_INLINE_FRIEND RWvistream& operator>>(RWvistream& s, className*& pCl)\
      { pCl = (className*)RWCollectable::recursiveRestoreFrom(s); return s; }\
    RW_INLINE_FRIEND RWFile&     operator>>(RWFile& f,     className*& pCl)\
      { pCl = (className*)RWCollectable::recursiveRestoreFrom(f); return f; }


/* See NOTE above !!! */


/************************************************************************
 *									*
 * The macro RWDEFINE_COLLECTABLE should be included in one ".cpp"	*
 * file to implement various functions for classing deriving from	*
 * RWCollectable.  It presently serves four purposes:			*
 * 1) To provide a definition for newSpecies().				*
 * 2) To provide a definition for copy().				*
 * 3) To provide a definition for isA().				*
 * 4) To define a "creator function" to be inserted into the 		*
 *    one-of-a-kind global RWFactory pointed to by theFactory.		*
 *									*
 ************************************************************************/

#ifdef RW_BROKEN_TOKEN_PASTE
#  define RWInit(className)       RWInitCtor##className
#  define rwCreateFN(className)   rwCreate##className
#else
#  define RWInit(className)	name2(RWInitCtor,className)
#  define rwCreateFN(className)	name2(rwCreate,className)
#endif

#define RWDEFINE_COLLECTABLE(className, id)			\
  RWCollectable* className::newSpecies() const			\
    { return new className; }					\
  RWCollectable* className::copy() const			\
    { return new className(*this); }				\
  RWClassID      className::isA() const				\
    { return id; }						\
								\
  /* Global function to create an instance of the class:*/	\
  RWCollectable* rwCreateFN(className)()			\
    { return new className; }					\
								\
  /* Static constructor to insert above into factory: */	\
  struct RWInit(className) {					\
    RWInit(className)();					\
    ~RWInit(className)();					\
  };								\
  RWInit(className)::RWInit(className)()			\
    { rwAddToFactory(rwCreateFN(className), id); }		\
  RWInit(className)::~RWInit(className)()			\
    { rwRemoveFromFactory(id); }				\
  RWInit(className) name2(rwDummy,className);



/////////////////////////////////////////////////////////////////
//                                                             //
//                       RWCollectable                         //
//                                                             //
/////////////////////////////////////////////////////////////////


class RWExport RWCollectable
{

  RWDECLARE_COLLECTABLE(RWCollectable)

public:

  RWCollectable();
  virtual			~RWCollectable();

  virtual RWspace		binaryStoreSize() const;
  virtual int			compareTo(const RWCollectable*) const;
  virtual unsigned		hash() const;
  virtual RWBoolean		isEqual(const RWCollectable*) const;
  virtual void			restoreGuts(RWFile&);
  virtual void			restoreGuts(RWvistream&);
  virtual void			saveGuts(RWFile&) const;
  virtual void			saveGuts(RWvostream&) const;
  RWspace			recursiveStoreSize() const;
  static RWCollectable*		recursiveRestoreFrom(RWFile&       file,
                                                     RWCollectable*p = 0);
  static RWCollectable*		recursiveRestoreFrom(RWvistream&   vstream,
                                                     RWCollectable*p = 0);
  void				recursiveSaveOn(RWFile&) const;
  void				recursiveSaveOn(RWvostream&) const;

};

#ifndef RW_TRAILING_RWEXPORT
rwexport RWvostream&  operator<<(RWvostream& vstream, const RWCollectable* p);
rwexport RWFile&      operator<<(RWFile& file,        const RWCollectable* p);
#else
RWvostream& rwexport  operator<<(RWvostream& vstream, const RWCollectable* p);
RWFile&     rwexport  operator<<(RWFile& file,        const RWCollectable* p);
#endif

inline RWvostream& operator<<(RWvostream& vstream, const RWCollectable& rc)
	{ rc.recursiveSaveOn(vstream); return vstream; }

inline RWFile&     operator<<(RWFile& file, const RWCollectable& rc)
	{ rc.recursiveSaveOn(file);    return file;    }

inline RWvistream& operator>>(RWvistream& vstream, RWCollectable& r)
	{ RWCollectable::recursiveRestoreFrom(vstream, &r); return vstream; }

inline RWFile&     operator>>(RWFile& file,        RWCollectable& r)
	{ RWCollectable::recursiveRestoreFrom(file, &r);    return file; }

extern rwexport void           rwDestroy(RWCollectable*, void*);
extern rwexport RWBoolean      rwIsEqualFun(const void* a, const void* b);
extern rwexport void           rwAddToFactory(RWuserCreator fn, RWClassID id);
extern rwexport RWCollectableP rwCreateFromFactory(RWClassID);
extern rwexport void           rwRemoveFromFactory(RWClassID);

extern rwexport RWCollectableP getRWNilCollectable();
#define RWnilCollectable (getRWNilCollectable())

#endif /* __RWCOLLECT_H__ */

