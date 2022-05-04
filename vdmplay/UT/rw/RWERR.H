#ifndef __RWERR_H__
#define __RWERR_H__

/*
 * Error definitions
 *
 * $Id: rwerr.h,v 6.5 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwerr.h,v $
 * Revision 6.5  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.4  1994/06/18  00:40:22  myersn
 * move virtual dtors out-of-line and explicit.
 *
 * Revision 6.3  1994/06/10  18:33:44  vriezen
 * Remove non-const RWThrow functions.
 *
 * Revision 6.2  1994/06/09  19:43:12  nevis
 * Now prototypes functions to deal with shared library throw bug.
 * These same changes allow catching of nonconst exception types.
 * RWTHROW macro now always calls RWThrow() function.  Programs can
 * now re-link to different versions of the library to get consistent
 * exception behavior (important for platforms with the shared library
 * throw bug).
 *
 * Revision 6.1  1994/04/15  19:10:46  vriezen
 * Move all files to 6.1
 *
 * Revision 3.7  1994/04/12  00:29:16  vriezen
 * Add STARTWRAP/ENDWRAP (Bug 312)
 *
 * Revision 3.6  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 3.5  1993/08/19  20:36:00  randall
 * added public member functions to access private data for RWFileErr and RWStreamErr
 *
 * Revision 3.4  1993/08/16  17:17:21  randall
 * for Sun, must include exception.h for xmsg and xalloc declarations
 *
 * Revision 3.3  1993/07/30  03:23:42  jims
 * Port to MS C7
 *
 * Revision 3.2  1993/05/31  21:45:26  keffer
 * New messaging architecture for localization
 *
 * Revision 3.1  1993/05/18  00:38:35  keffer
 * Rewrote to use new messaging and exception handling facility.
 *
 */

#include "rw/defs.h"
STARTWRAP
#include <stdio.h>	/* Looking for FILE */
ENDWRAP
#ifndef RW_NO_XMSG
#  ifdef sun
#    include <exception.h>
#  else
#    include <except.h>
#  endif
#endif

class RWExport RWMessage;

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                RWxmsg                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef RW_NO_XMSG

// No host-supplied xmsg; we must supply it:
class RWExport RWxmsg
{
public:

  RWxmsg(const char*);
  RWxmsg(const RWMessage&);
  RWxmsg(const RWxmsg&);
  virtual ~RWxmsg();

  const char*	 why() const	{return msg_; }

  void 		raise();

protected:

  RWxmsg(const char*, RWBoolean doCopy);

private:

  void makeCopy(const char* str);
  const char* msg_;		// Message
  RWBoolean   doFree_;

};


#else


/*
 * Compiler-supplied xmsg available.
 * In this case, RWxmsg is merely an interface.
 */

class RWExport RWxmsg : public xmsg
{

public:

  RWxmsg(const char*);
  RWxmsg(const RWMessage&);
  virtual ~RWxmsg();

};

#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                               RWxalloc                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef RW_NO_XMSG

// No host-supplied xalloc; we must supply it:
class RWExport RWxalloc : public RWxmsg
{
public:

  RWxalloc(size_t size);
  virtual ~RWxalloc();

  size_t requested() const { return size_; }

private:

  size_t size_;

};

#else


/*
 * Compiler-supplied xalloc available.
 * In this case, RWxalloc is merely an interface.
 */

class RWExport RWxalloc : public xalloc
{
public:

  RWxalloc(size_t size);
  virtual ~RWxalloc();

};

#endif


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                            RWExternalErr                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// For errors external to the library;
// these are errors the prudent programmer should expect,
// such as out-of-disk-space, file errors, and the like:

class RWExport RWExternalErr : public RWxmsg
{

public:

  RWExternalErr(const char*);
  RWExternalErr(const RWMessage&);
  virtual ~RWExternalErr();

};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                              RWFileErr                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// For errors involving a file descriptor:
class RWExport RWFileErr : public RWExternalErr
{

public:

  enum errType {unknown, seekErr, readErr, writeErr, openErr, closeErr};
  RWFileErr(const char*     , FILE*, errType type);
  RWFileErr(const RWMessage&, FILE*, errType type);
  virtual ~RWFileErr();

  FILE*   descriptor()          { return fd_;   }
  errType errorType()           { return type_; }

private:

  FILE* fd_;
  errType type_;
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWStreamErr                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// For errors involving streams:
class RWExport RWStreamErr : public RWExternalErr
{

public:

  RWStreamErr(const char*     , ios&);
  RWStreamErr(const RWMessage&, ios&);
  virtual ~RWStreamErr();

  ios&    errStream()          { return stream_; }

private:

  ios&    stream_;

};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                            RWInternalErr                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Logic errors are violated invariants, bounds errors,
// and the like.  

class RWExport RWInternalErr : public RWxmsg
{

public:

  RWInternalErr(const char*);
  RWInternalErr(const RWMessage&);
  ~RWInternalErr();

};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                             RWBoundsErr                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

class RWExport RWBoundsErr : public RWInternalErr
{

public:

  RWBoundsErr(const char*);
  RWBoundsErr(const RWMessage&);
  virtual ~RWBoundsErr();

};


  // Compiler does not support exception handling:
  // Or cannot throw them from a shared library.
  extern void rwexport RWThrow( const RWxmsg&        );
  extern void rwexport RWThrow( const RWxalloc&      );
  extern void rwexport RWThrow( const RWExternalErr& );
  extern void rwexport RWThrow( const RWFileErr&     );
  extern void rwexport RWThrow( const RWStreamErr&   );
  extern void rwexport RWThrow( const RWInternalErr& );
  extern void rwexport RWThrow( const RWBoundsErr&   );
  typedef void (rwfar *rwErrHandler)(const RWxmsg&);
  rwErrHandler rwexport rwSetErrHandler(rwErrHandler);


// All exceptions are thrown by the library code itself so that
// if exceptions are changed from that of the library, both application
// code and library code throw exceptions identically.

// Use extra parens to skirt MS C7 bug as documented in MS Problem ID: C9301003
# define RWTHROW(a) (RWThrow(a))


#endif /* __RWERR_H__ */
