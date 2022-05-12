/*
 * Error routines.
 *
 * $Id: rwerr.cpp,v 3.7 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1989 - 1993.
 * This software is subject to copyright protection under the laws of
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/RWERR.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 3.7  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 3.6  1993/09/07  00:02:34  jims
 * Change "||" to "&&" in preprocessor check for Win16 DLL
 *
 * Revision 3.5  1993/09/06  21:48:09  jims
 * Use RWInstanceManager to make rwErrRoutine unique to each task
 * using Win16 DLL
 *
 * Revision 3.4  1993/08/04  22:02:51  jims
 * Use __WIN16__ || __WIN32__ instead of _Windows
 *
 * Revision 3.3  1993/06/25  17:51:25  north
 * added use of RW_GLOBAL_ENUMS
 *
 * Revision 3.2  1993/05/31  21:46:32  keffer
 * New messaging architecture for localization
 *
 * Revision 3.1  1993/05/18  00:38:12  keffer
 * Rewrote to use new messaging and exception handling facility.
 *
 */

#include "rw/rwerr.h"
#include "rw/message.h"
#include "rw/coreerr.h"
STARTWRAP
#include <string.h>
ENDWRAP

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: rwerr.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#if defined(_RWBUILDDLL) && defined(__WIN16__)
#include "rw/instmgr.h"
#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                RWxmsg                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifdef RW_NO_XMSG


    RWxmsg::RWxmsg(const char* msg)
    : doFree_(TRUE)
    {
      makeCopy(msg);
    }

    RWxmsg::RWxmsg(const RWMessage& msg)
    : doFree_(TRUE)
    {
      makeCopy(msg.str());
    }

    RWxmsg::RWxmsg(const RWxmsg& m)
    : doFree_(m.doFree_)
    {
      if(doFree_)
	makeCopy(m.msg_);
      else
	msg_ = m.msg_;
    }

    RWxmsg::~RWxmsg()
    {
      if (doFree_)
	delete (char*)msg_;
    }

    // Protected constructor:
    RWxmsg::RWxmsg(const char* msg, RWBoolean doCopy)
    : doFree_(doCopy)
    {
      if (doCopy)
	makeCopy(msg);
      else
	msg_ = msg;
    }


    void RWxmsg::makeCopy(const char* str)
    {
      char* tmp = new char[strlen(str)+1];
      strcpy(tmp, str);
      msg_ = tmp;
    }

    void RWxmsg::raise() {RWTHROW(*this);}


#else	/* RW_NO_XMSG */

    RWxmsg::RWxmsg(const char* msg)
    : xmsg(msg)
    {;}

    RWxmsg::RWxmsg(const RWMessage& msg)
    : xmsg(msg.str())
    {;}

#endif	/* RW_NO_XMSG */


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                               RWxalloc                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifdef RW_NO_XMSG

    RWxalloc::RWxalloc(size_t size)
    : RWxmsg(rwMsgLookup(RWCORE_NOMEM), FALSE),
      size_(size)
    {;}

#else

    RWxalloc::RWxalloc(size_t size)
    : xalloc(rwMsgLookup(RWCORE_NOMEM), size)
    {;}

#endif	/* RW_NO_XMSG */


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                            RWExternalErr                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

RWExternalErr::RWExternalErr(const char* msg)
: RWxmsg(msg)
{;}

RWExternalErr::RWExternalErr(const RWMessage& msg)
: RWxmsg(msg)
{;}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              RWFileErr                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef RW_GLOBAL_ENUMS
RWFileErr::RWFileErr(const char* msg, FILE* fd,            errType type)
#else
RWFileErr::RWFileErr(const char* msg, FILE* fd, RWFileErr::errType type)
#endif
: RWExternalErr(msg),
  fd_(fd),
  type_(type)
{;}

#ifdef RW_GLOBAL_ENUMS
RWFileErr::RWFileErr(const RWMessage& msg, FILE* fd,            errType type)
#else
RWFileErr::RWFileErr(const RWMessage& msg, FILE* fd, RWFileErr::errType type)
#endif
: RWExternalErr(msg),
  fd_(fd),
  type_(type)
{;}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWStreamErr                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

RWStreamErr::RWStreamErr(const char* msg, ios& stream)
: RWExternalErr(msg),
  stream_(stream)
{;}

RWStreamErr::RWStreamErr(const RWMessage& msg, ios& stream)
: RWExternalErr(msg),
  stream_(stream)
{;}



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                            RWInternalErr                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

RWInternalErr::RWInternalErr(const char* msg)
: RWxmsg(msg)
{;}


RWInternalErr::RWInternalErr(const RWMessage& msg)
: RWxmsg(msg)
{;}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWBoundsErr                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

RWBoundsErr::RWBoundsErr(const char* msg)
: RWInternalErr(msg)
{;}


RWBoundsErr::RWBoundsErr(const RWMessage& msg)
: RWInternalErr(msg)
{;}


#ifdef RW_NO_EXCEPTIONS

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//          Code for compilers that do not support exceptions           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <stdlib.h>	/* Looking for exit() */

#if defined(__WIN16__) || defined(__WIN32__)

/************************************************
 *						*
 *	The default error handler; Windows	*
 *						*
 ************************************************/

#include <windows.h>

static void rwfar
rwDefaultErrHandler(const RWxmsg& err)
{
  MessageBox(NULL, err.why(), "Fatal Error", MB_TASKMODAL | MB_ICONSTOP | MB_OK);
  exit(1);
}

#else /* Not Windows */  

/************************************************
 *						*
 *	The default error handler; non-Windows	*
 *						*
 ************************************************/

static void rwfar
rwDefaultErrHandler(const RWxmsg& err)
{
  fputs("** "                          , stderr);
  fputs(err.why()                      , stderr);
  fputs("\n** Processing terminated.\n", stderr);
  exit(1);
}

#endif /* __WIN16__ || __WIN32__ */

#if defined(_RWBUILDDLL) && defined(__WIN16__)

/*
 * Maintain a unique Error Handler for each task using the DLL:
 */
class ErrHandlerManager : public RWInstanceManager
{
    // Redefined from RWInstanceManager; Supplies initial value:	
    virtual void rwfar*	newValue()
      { return (void rwfar*) new rwErrHandler(rwDefaultErrHandler); }

    // Redefined from RWInstanceManager; deletes value:
    virtual void deleteValue(void rwfar* p) { delete (rwErrHandler*) p; }
public:
    /*
     * Get current value.  If no currentValue, then this is a new task, so
     * initialize with default value and return that.
     */	
    rwErrHandler& getCurrentErrHandler()
      { rwErrHandler* eh = (rwErrHandler*) currentValue();
        if (eh)
          return *eh;
	else
          return *(rwErrHandler*) addValue();
      }
};

static ErrHandlerManager ehManager;

#define rwErrRoutine (ehManager.getCurrentErrHandler())

#else  /* Not building a Win16 DLL */

static rwErrHandler rwErrRoutine = rwDefaultErrHandler;

#endif /* defined(_RWBUILDDLL) && defined(__WIN16__) */


rwErrHandler rwexport
rwSetErrHandler(rwErrHandler routine)
{
  rwErrHandler temp = rwErrRoutine;
  rwErrRoutine = routine;
  return temp;
}

void rwexport RWThrow( const RWxmsg&        err) { (*rwErrRoutine)(err); }
void rwexport RWThrow( const RWxalloc&      err) { (*rwErrRoutine)(err); }
void rwexport RWThrow( const RWExternalErr& err) { (*rwErrRoutine)(err); }
void rwexport RWThrow( const RWFileErr&     err) { (*rwErrRoutine)(err); }
void rwexport RWThrow( const RWStreamErr&   err) { (*rwErrRoutine)(err); }
void rwexport RWThrow( const RWInternalErr& err) { (*rwErrRoutine)(err); }
void rwexport RWThrow( const RWBoundsErr&   err) { (*rwErrRoutine)(err); }

#endif	/* RW_NO_EXCEPTIONS */

