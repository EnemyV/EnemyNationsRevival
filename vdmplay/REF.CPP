/*
 * Definitions for RWReference
 *
 * $Id: ref.cpp,v 1.7 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1993.
 * This software is subject to copyright protection under the laws of
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * Implementation of reference-counting garbage-collection primitives for
 * multi-threaded environment.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/REF.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 1.7  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 1.6  1993/09/01  03:40:34  myersn
 * change implementation semantics to allow easy static initialization.
 *
 * Revision 1.5  1993/05/29  18:48:24  keffer
 * Renamed or added a dummy variable in case nothing gets compiled
 *
 * Revision 1.4  1993/05/25  18:41:14  keffer
 * Added bogus definition rwUtilityDummy2 to quiet fussy librarians.
 *
 * Revision 1.3  1993/04/12  12:11:22  jims
 * Now uses RWMutex class for locking
 *
 * Revision 1.2  1993/02/03  19:58:44  myersn
 * cache value of refcount while locked in removeReference, as return value.
 *
 * Revision 1.1  1993/01/29  20:17:32  myersn
 * Initial revision
 */

#include "rw/ref.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: ref.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#if defined(RW_MULTI_THREAD)
#include "rw/mutex.h"

void
RWReference::addReference(RWMutex& mutex)
{
  mutex.acquire();
  ++refs_;
  mutex.release();
}

unsigned
RWReference::removeReference(RWMutex& mutex)
{
  mutex.acquire();
  unsigned int result = refs_--;
  mutex.release();
  return result;
}

#else

// This definition is compiled in case nothing else is,
// in order to quiet down some fussy librarians:
int rwDummy_ref_cpp;

#endif

