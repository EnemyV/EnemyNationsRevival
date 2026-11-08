#ifndef __RWINSTMGR_H__
#define __RWINSTMGR_H__

/*
 * Declarations for RWInstanceManager --- manages multi-thread and per-process
 * instance data.
 *
 * $Id: instmgr.h,v 6.2 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: instmgr.h,v $
 * Revision 6.2  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.1  1994/04/15  19:10:22  vriezen
 * Move all files to 6.1
 *
 * Revision 1.16  1994/03/21  22:04:58  jims
 * Add support for POSIX threads
 *
 * Revision 1.15  1993/11/22  18:21:44  jims
 * Move OS/2 section up where it belongs -- before the class definition
 *
 * Revision 1.14  1993/11/18  01:20:44  jims
 * Move support for OS/2 MT from rwtsd to instmgr
 *
 * Revision 1.13  1993/11/16  08:42:38  myersn
 * add OS/2 support
 *
 * Revision 1.12  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.11  1993/08/05  11:40:24  jims
 * Remove exitProc and GlobalRelease... function
 *
 * Revision 1.10  1993/07/29  06:42:17  jims
 * Change RW_WIN32_API to __WIN32__
 *
 * Revision 1.9  1993/07/09  07:58:05  jims
 * Port to Windows NT
 *
 * Revision 1.8  1993/07/08  00:10:55  jims
 * use rwfar from defs.h instead of #defining FAR
 *
 * Revision 1.7  1993/07/01  18:12:36  jims
 * Remove #include directive for rwwind.h
 *
 * Revision 1.6  1993/06/24  01:39:17  keffer
 * Now includes <rw/defs.h> *before* checking for RW_MULTI_THREAD
 *
 * Revision 1.5  1993/04/12  12:03:21  jims
 * Now supports thread-specific data for multiple threads within on process,
 * continues to support per-task data for Windows 3.x DLLs
 *
 * Revision 1.4  1993/03/25  01:17:42  keffer
 * Now includes "rw/defs.h"
 *
 * Revision 1.3  1993/02/06  02:34:31  keffer
 * Added copyright notice.
 *
 */

#include "rw/defs.h"

#if (defined(__DLL__) && defined(__WIN16__)) || defined(RW_MULTI_THREAD)

#if defined(__DLL__) && defined(__WIN16__)
#  include "rw/rwtsd.h"

#elif defined(sun)		// MT & Sun -- assuming Solaris 2.x/Cafe
#  include <thread.h>
   typedef thread_key_t RWTSDKEY;

#elif defined(RW_POSIX_THREADS)
#  include <pthread.h>
   typedef pthread_key_t RWTSDKEY;

#elif defined(__WIN32__)
#  include <windows.h>
   typedef DWORD RWTSDKEY;

#elif defined(__OS2__)
#  if defined(__IBMCPP__)
#    define RWTHREADID *_threadid
#  elif defined(__TURBOC__)
#    define RWTHREADID _threadid
#  endif
#  define INCLDOSPROCESS
#  include <os2.h>
#  define rwfar
#  define rwpascal
   typedef unsigned long RWTSDKEY;

   RWTSDKEY rwfar rwpascal 
   RWGetTaskSpecificKey();

   int rwfar rwpascal 
   RWSetTaskSpecificData(RWTSDKEY hKey, void rwfar*);

   void rwfar* rwfar rwpascal 
   RWGetTaskSpecificData(RWTSDKEY hKey);

   void rwfar* rwfar rwpascal 
   RWReleaseTaskSpecificData(RWTSDKEY hKey);

#else
#  error RW_MULTI_THREAD not supported in this environment
#endif

class RWExport RWInstanceManager
{
public:
  void rwfar*		addValue();       // set instance specific data
  void rwfar*		currentValue();	  // get instance specific data
  void 	                freeValue();      // release key and discard data

  // Values to be provided and discarded by the specializing class:
  virtual void rwfar*	newValue() = 0;
  virtual void          deleteValue(void rwfar*) = 0;

  RWInstanceManager();

private:
  RWTSDKEY              tsd_key;
};

#else	/* neither 16-bit Windodws DLL nor MultiThread */

#error RWInstanceManager class not meant to be used outside DLL or MT situation

#endif

#endif	/* __RWINSTMGR_H__ */
