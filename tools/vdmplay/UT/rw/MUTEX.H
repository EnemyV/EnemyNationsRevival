#ifndef __RWMUTEX_H__
#define __RWMUTEX_H__

/*
 * Declarations for class RWMutex
 *
 * $Id: mutex.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * This class is a portable implementation of a simple mutex lock
 * to be used for synchronizing multiple threads within a single process.
 * It is not suitable for use among threads of different processes.
 * 
 ***************************************************************************
 *
 * $Log: mutex.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/07/11  16:56:42  jims
 * Add missing semicolon
 *
 * Revision 6.1  1994/04/15  19:10:32  vriezen
 * Move all files to 6.1
 *
 * Revision 1.12  1994/03/21  22:02:59  jims
 * Add support for POSIX threads
 *
 * Revision 1.11  1993/12/06  20:21:22  jims
 * Qualify enum type in prototype of RWMutex constructor
 *
 * Revision 1.10  1993/11/22  17:52:54  jims
 * Moved inline constructor outside of class
 *
 * Revision 1.9  1993/11/17  01:57:03  myersn
 * fix support for static locks, inlined implementation.
 *
 * Revision 1.8  1993/11/16  08:56:34  myersn
 * add OS/2 support, and support for static lock objects.
 *
 * Revision 1.7  1993/11/04  04:43:28  jims
 * Include rw/defs.h if necessary
 * Define inline RWGuard functions after RWMutex inline function defs
 *
 * Revision 1.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.5  1993/08/04  21:17:22  jims
 * Change RW_WIN32_API to __WIN32__
 *
 * Revision 1.4  1993/07/29  06:42:17  jims
 * Change RW_WIN32_API to __WIN32__
 *
 * Revision 1.3  1993/07/09  07:55:49  jims
 * Port to Windows NT
 *
 * Revision 1.2  1993/04/15  20:35:42  keffer
 * Changed comments.
 *
 */

#ifndef __RWDEFS_H__
# include "rw/defs.h"
#endif

#ifdef RW_MULTI_THREAD		/* This class only relevant in MT situation */

#include "rw/coreerr.h"
#include "rw/rwerr.h"
#if defined(sun)		/* assuming Solaris 2.1 */
#  include <synch.h>
   typedef mutex_t RWMutexType;

#elif defined(RW_POSIX_THREADS)
#  include <pthread.h>
   typedef pthread_mutex_t RWMutexType;
#  define RW_NEEDS_SEM_INIT

#elif defined(__WIN32__)
#  include <windows.h>
   typedef HANDLE RWMutexType;

#elif defined(__OS2__)
#  define INCL_DOSSEMAPHORES
#  define RW_NEEDS_SEM_INIT
#  include <os2.h>
   typedef HMTX RWMutexType;

// defines for other OSs go here
#else
#  error Class RWMutex is not yet supported in this environment
#endif

class RWMutex {
  RWMutexType mutex;
# if defined(RW_NEEDS_SEM_INIT)
  int initFlag;
# endif
public:
  enum StaticCtor {staticCtor};
  RWMutex(); 
  RWMutex(StaticCtor);   // some statics need special handling
  ~RWMutex();            // destroy the mutex
  void init();
  void acquire(); // block until mutex released, acquire and continue
  void release(); // release the mutex
};

class RWGuard {
  RWMutex& rwmutex;
public:
  RWGuard(RWMutex& m);
  ~RWGuard();
};

/*
 * =========================
 * RWMutex member functions
 * -------------------------
 */

/*
 * For those OSs that require a non-zero mutex, we must treat static 
 * mutexes specially: They may not be initialized when we need them.
 * For efficiency, we do conditional compilation in several methods
 * based on that need.
 */

inline RWMutex::RWMutex(RWMutex::StaticCtor)
{
  // empty, because acquire() may already have been used.
}

inline RWMutex::~RWMutex() 
{ 
# if defined(RW_NEEDS_SEM_INIT)
  if(0 == initFlag) return;
  else initFlag = 0;
# endif
# if defined(sun)
  mutex_destroy(&mutex); 
# elif defined(RW_POSIX_THREADS)
  pthread_mutex_destroy(&mutex); 
# elif defined(__WIN32__)
  CloseHandle(mutex);
# elif defined(__OS2__)
  APIRET rv;
  if(0 != (rv = DosCloseMutexSem(mutex)))
    RWTHROW(RWExternalErr(RWMessage(RWCORE_GENERIC,rv)));
# endif
}

void
inline RWMutex::init()  
{ 
# if defined(sun)
  mutex_init(&mutex, USYNC_THREAD, NULL); 
# elif defined(RW_POSIX_THREADS)
  pthread_mutex_init(&mutex, pthread_mutexattr_default);
# elif defined(__WIN32__)
  mutex = CreateMutex(NULL, FALSE, NULL);
# elif defined(__OS2__)
  APIRET rv;
  if(0 != (rv = DosCreateMutexSem(0,&mutex,DC_SEM_SHARED,FALSE)))// some problem
     RWTHROW(RWExternalErr(RWMessage(RWCORE_GENERIC, rv)));
# endif
# if defined(RW_NEEDS_SEM_INIT)
  initFlag = 1;
# endif
}

inline RWMutex::RWMutex() 
{ 
  init();   // Initialize the mutex
}  

inline void RWMutex::acquire() 
{ 
# if defined(RW_NEEDS_SEM_INIT)
  if(0 == initFlag) init();
# endif
# if defined(sun)
  mutex_lock(&mutex);    
# elif defined(RW_POSIX_THREADS)
  pthread_mutex_lock(&mutex);    
# elif defined(__WIN32__)
  WaitForSingleObject(mutex,INFINITE);
# elif defined(__OS2__)
  APIRET rv;
  if(0 != (rv = DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT)))
    RWTHROW(RWExternalErr(RWMessage(RWCORE_GENERIC,rv)));
# endif
}

inline void RWMutex::release()
{ 
# if defined(sun)
  mutex_unlock(&mutex);  
# elif defined(RW_POSIX_THREADS)
  pthread_mutex_unlock(&mutex);  
# elif defined(__WIN32__)
  ReleaseMutex(mutex);
# elif defined(__OS2__)
  APIRET rv;
  if(0 != (rv = DosReleaseMutexSem(mutex)))
    RWTHROW(RWExternalErr(RWMessage(RWCORE_GENERIC,rv)));
# endif
}

/*
 * ================================
 * RWGuard inline member functions
 * --------------------------------
 */

inline RWGuard::RWGuard(RWMutex& m) : rwmutex(m) { rwmutex.acquire(); }
inline RWGuard::~RWGuard()                       { rwmutex.release(); }

#endif  /* RW_MULTI_THREAD */
#endif  /* __RWMUTEX_H__ */


