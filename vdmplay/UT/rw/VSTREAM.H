#ifndef __RWVSTREAM_H__
#define __RWVSTREAM_H__

/*
 * rw[io]stream --- abstract base class for I/O
 *
 * $Id: vstream.h,v 6.7 1994/07/14 15:12:38 jims Exp $
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
 * $Log: vstream.h,v $
 * Revision 6.7  1994/07/14  15:12:38  jims
 * Add #ifdef for RW_TRAILING_RWEXPORT for declaration of flush
 *
 * Revision 6.6  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.5  1994/06/16  04:28:48  vriezen
 * Added flush() support.
 *
 * Revision 6.4  1994/06/16  03:57:59  vriezen
 * Added outlined virtual dtors.
 *
 * Revision 6.3  1994/04/15  21:48:21  vriezen
 * Added back in RWvios::dummy function
 *
 * Revision 6.2  1994/04/15  21:12:29  vriezen
 * Backed out (until next release) RW_DEFINE_HOME and RW_DECLARE_HOME support.
 *
 * Revision 6.1  1994/04/15  19:48:50  vriezen
 * Move all files to 6.1
 *
 * Revision 2.17  1994/03/23  20:42:45  vriezen
 * ADD RW_DECLARE_HOME macro to RWios, RWvios, RWvistream and RWvostream
 *
 * Revision 2.16  1994/02/05  02:02:17  jims
 * Add huge pointer support for character reads and writes
 *
 * Revision 2.15  1993/11/18  22:12:19  jims
 * add version number to vstreams for backward compatibility
 *
 * Revision 2.14  1993/11/17  04:12:46  keffer
 * Added function putString(), to allow support of embedded nulls
 *
 * Revision 2.13  1993/11/14  21:35:26  keffer
 * Added virtual destructor for RWvios
 *
 * Revision 2.12  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.11  1993/08/19  01:22:53  keffer
 * Added default constructor for RWios.
 *
 * Revision 2.10  1993/06/05  17:55:20  keffer
 * Put RWvios pure virtuals in RWv[io]stream to avoid cfront V2.0 bug.
 *
 * Revision 2.9  1993/04/15  00:49:45  keffer
 * Cfront "sorry, not implemented" when attempting merge of
 * conversion functions.
 *
 * Revision 2.8  1993/04/14  21:03:52  keffer
 * Changed inheritance hierarchy to allow XDR streams, which cannot use
 * streambuf model.
 *
 * Revision 2.7  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.6  1993/02/23  14:54:09  alv
 * made constructors non-inline; this avoids vtable duplication by
 * giving an object file in which to put the vtable.
 *
 * Revision 2.5  1993/01/22  17:38:12  keffer
 * V1.2 style streams no longer supported.
 *
 * Revision 2.3  1992/11/26  21:04:33  myersn
 * add members to operate on wchar_t values and strings.
 *
 * Revision 2.2  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.1  1992/11/15  22:12:09  keffer
 * Removed use of macro NL
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 *    Rev 1.5   05 Nov 1991 13:52:52   keffer
 * Can now live in the DLL
 * 
 *    Rev 1.4   17 Oct 1991 09:12:58   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.2   29 Jul 1991 11:33:30   keffer
 * Macro RW_NO_OVERLOAD_UCHAR checks for overloaded unsigned char
 * 
 *    Rev 1.1   24 Jul 1991 13:06:50   keffer
 * Added pvcs keywords
 *
 */

#include "rw/rstream.h"

/*
 * vstreams maintain a version number to help with backward compatibility 
 * issues.  See RWCString::saveOn for an example of how the version number
 * can be used internally.  Users of the library can set the version
 * number of a stream to force it to behave as in a previous incarnation.
 *
 *     Version 0: Baseline (virtually introduced with Tool600)
 *     Version 1: RWCStrings now save/restore themselves to/from virtual
 *                streams as sequence of characters, not numbers.  (Introduced
 *                with Tool602).
 */
static const unsigned current_version = 1;

/************************************************
 *						*
 *		class RWvios			*
 *						*
 ************************************************/

class RWExport RWvios
{
public:

  virtual ~RWvios();

  // Parallel the ios interface:
  virtual int		eof()          = 0;
  virtual int		fail()         = 0;
  virtual int		bad()          = 0;
  virtual int		good()         = 0;
  virtual int		rdstate()      = 0;
  virtual void		clear(int v=0) = 0;

};


/************************************************
 *						*
 *		class RWios			*
 *						*
 ************************************************/

// Convenience class for virtual streams that want ios type functionality,
// but cannot use streambufs in their implementation:

class RWExport RWios : public virtual RWvios
{
public:

  virtual ~RWios();
  RWios() : status(0) {;}
  virtual int		eof()           {return status & ios::eofbit;}
  virtual int		fail()          {return status & ios::failbit;}
  virtual int		bad()           {return status & ios::badbit;}
  virtual int		good()          {return status==0;}
  virtual int		rdstate()       {return status;}
  virtual void		clear(int v=0)  {status = v;}

protected:

  int status;

};


/************************************************
 *						*
 *		class RWvistream		*
 *						*
 ************************************************/

class RWExport RWvistream : public virtual RWvios
{
public:

  virtual ~RWvistream();
  virtual int		eof()          = 0;
  virtual int		fail()         = 0;
  virtual int		bad()          = 0;
  virtual int		good()         = 0;
  virtual int		rdstate()      = 0;
  virtual void		clear(int v=0) = 0;

  virtual int		get()          = 0;		// Treat as number; EOF on end-of-file
  virtual RWvistream&	get(char&)     = 0;		// Treat as number
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t&)  = 0;	        // Treat as character
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char&) = 0;	// Treat as number
#endif
  virtual RWvistream&	get(char*   , size_t N) = 0;
#ifdef RW_SEGMENTED_ARCHITECTURE
  virtual RWvistream&   get(char rwhuge *, unsigned long N) = 0;
#endif
  virtual RWvistream&	get(double* , size_t N) = 0;
  virtual RWvistream&	get(float*  , size_t N) = 0;
  virtual RWvistream&	get(int*    , size_t N) = 0;
  virtual RWvistream&	get(long*   , size_t N) = 0;
  virtual RWvistream&	get(short*  , size_t N) = 0;
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t*, size_t N) = 0;
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char* , size_t N) = 0;
#endif
  virtual RWvistream&	get(unsigned short*, size_t N) = 0;
  virtual RWvistream&	get(unsigned int*  , size_t N) = 0;
  virtual RWvistream&	get(unsigned long* , size_t N) = 0;
  virtual RWvistream&	getString(char* s  , size_t N) = 0;	// Treat as character string
  virtual RWvistream&	operator>>(char&)    = 0;		// Treat as character
  virtual RWvistream&	operator>>(double&)  = 0;
  virtual RWvistream&	operator>>(float&)   = 0;
  virtual RWvistream&	operator>>(int&)     = 0;
  virtual RWvistream&	operator>>(long&)    = 0;
  virtual RWvistream&	operator>>(short&)   = 0;
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	operator>>(wchar_t&) = 0;
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	operator>>(unsigned char&) = 0;
#endif
  virtual RWvistream&	operator>>(unsigned short&) = 0;
  virtual RWvistream&	operator>>(unsigned int&)   = 0;
  virtual RWvistream&	operator>>(unsigned long&)  = 0;

// For backwards compatibility
private:
  unsigned version_;
public:
  void version(unsigned v) { version_ = v; }
  unsigned version() { return version_; }
// constructor
  RWvistream() : version_(current_version) { }
};


/************************************************
 *						*
 *		class RWvostream		*
 *						*
 ************************************************/

class RWExport RWvostream : public virtual RWvios
{
public:
  virtual ~RWvostream();
  virtual int		eof()          = 0;
  virtual int		fail()         = 0;
  virtual int		bad()          = 0;
  virtual int		good()         = 0;
  virtual int		rdstate()      = 0;
  virtual void		clear(int v=0) = 0;

  virtual RWvostream&	putString(const char*, size_t) = 0;	// Treat as character string
  virtual RWvostream&	operator<<(const char*) = 0;		// Treat as character string
  virtual RWvostream&	operator<<(char) = 0;			// Treat as character
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvostream&	operator<<(wchar_t) = 0;
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvostream&	operator<<(unsigned char)  = 0;
#endif
  virtual RWvostream&	operator<<(double)         = 0;
  virtual RWvostream&	operator<<(float)          = 0;
  virtual RWvostream&	operator<<(int)            = 0;
  virtual RWvostream&	operator<<(unsigned int)   = 0;
  virtual RWvostream&	operator<<(long)           = 0;
  virtual RWvostream&	operator<<(unsigned long)  = 0;
  virtual RWvostream&	operator<<(short)          = 0;
  virtual RWvostream&	operator<<(unsigned short) = 0;
  virtual RWvostream&   flush()                    = 0;

  virtual RWvostream&	put(char) = 0;			   // Treat as number
  virtual RWvostream&	put(const char* p, size_t N) = 0;  // Treat as numbers
#ifdef RW_SEGMENTED_ARCHITECTURE
  virtual RWvostream&	put(const char rwhuge * p, unsigned long N) = 0;
#endif
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvostream&	put(wchar_t) = 0;
  virtual RWvostream&	put(const wchar_t* p, size_t N) = 0;
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvostream&	put(unsigned char) = 0;
  virtual RWvostream&	put(const unsigned char*  , size_t N) = 0;
#endif
  virtual RWvostream&	put(const short*          , size_t N) = 0;
  virtual RWvostream&	put(const unsigned short* , size_t N) = 0;
  virtual RWvostream&	put(const int*            , size_t N) = 0;
  virtual RWvostream&	put(const unsigned int*   , size_t N) = 0;
  virtual RWvostream&	put(const long*           , size_t N) = 0;
  virtual RWvostream&	put(const unsigned long*  , size_t N) = 0;
  virtual RWvostream&	put(const float*          , size_t N) = 0;
  virtual RWvostream&	put(const double*         , size_t N) = 0;

// For backwards compatibility
private:
  unsigned version_;
public:
  void version(unsigned v) { version_ = v; }
  unsigned version() { return version_; }
// constructor
  RWvostream() : version_(current_version) { }
};

// If the following function were a member function of RWvostream, rather
// than a global function, classes derived from RWvostream would not 
// be able to redefine other oper<< functions, without hiding this one, or 
// redefining this one also.
inline RWvostream&  operator<<(RWvostream& v, RWvostream& (*f)(RWvostream&)) {
  return (*f)(v);
}

#ifdef RW_TRAILING_RWEXPORT
RWvostream&  rwexport flush(RWvostream&);
#else
rwexport RWvostream&  flush(RWvostream&);
#endif

#endif /* __RWVSTREAM_H__ */

