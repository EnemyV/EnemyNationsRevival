#ifndef __RWPSTREAM_H__
#define __RWPSTREAM_H__

/*
 * rwp[io]stream ---	Portable I/O streams (use escape sequences for 
 * 			writing special characters.
 *
 * $Id: pstream.h,v 6.7 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: pstream.h,v $
 * Revision 6.7  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.6  1994/06/18  01:30:45  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.5  1994/06/16  04:28:48  vriezen
 * Added flush() support.
 *
 * Revision 6.4  1994/06/16  04:25:14  vriezen
 * Cast away const on precision() for weak iostream.h's
 *
 * Revision 6.3  1994/06/16  00:52:28  vriezen
 * Bug #54.  Enhance precision support for RWpostream
 *
 * Revision 6.2  1994/05/09  19:55:58  jims
 * Disallow copying by adding private declarations for assignment operator
 * and copy constructor
 *
 * Revision 6.1  1994/04/15  19:10:34  vriezen
 * Move all files to 6.1
 *
 * Revision 2.12  1994/02/05  02:02:17  jims
 * Add huge pointer support for character reads and writes
 *
 * Revision 2.11  1993/11/17  21:42:02  keffer
 * [io]stream is now member data instead of multiply inheriting, thus
 * avoiding ambiguous shift operations.
 *
 * Revision 2.10  1993/11/17  04:12:46  keffer
 * Added function putString(), to allow support of embedded nulls
 *
 * Revision 2.9  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.8  1993/05/18  00:41:46  keffer
 * Now uses new exception handling facility
 *
 * Revision 2.7  1993/04/15  02:45:18  keffer
 * Changed inheritance order to avoid cfront bug
 *
 * Revision 2.6  1993/04/14  21:03:52  keffer
 * Changed inheritance hierarchy to allow XDR streams, which cannot use
 * streambuf model.
 *
 * Revision 2.5  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.3  1992/11/26  21:04:33  myersn
 * add members to operate on wchar_t values and strings.
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
 *    Rev 1.8   17 Mar 1992 12:26:48   KEFFER
 * Removed RWTV3_COMPATIBLE macro hooks.
 * 
 *    Rev 1.7   18 Feb 1992 09:54:32   KEFFER
 * 
 *    Rev 1.6   13 Nov 1991 11:09:16   keffer
 * Removed "near" qualifiers.  Both putwrap()'s are now out of line.
 * 
 *    Rev 1.5   05 Nov 1991 13:52:50   keffer
 * Can now live in the DLL
 * 
 *    Rev 1.4   17 Oct 1991 09:12:46   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.2   29 Jul 1991 11:33:32   keffer
 * Macro RW_NO_OVERLOAD_UCHAR checks for overloaded unsigned char
 * 
 */

#include "rw/vstream.h"

/************************************************
 *						*
 *		class RWpistream		*
 *						*
 ************************************************/

class RWExport RWpistream : public RWvistream
{

public:

  RWpistream(istream& str);
  RWpistream(streambuf* s);
  virtual ~RWpistream();

  virtual int		eof()          {return istr_.eof();    }
  virtual int		fail()         {return istr_.fail();   }
  virtual int		bad()          {return istr_.bad();    }
  virtual int		good()         {return istr_.good();   }
  virtual int		rdstate()      {return istr_.rdstate();}
  virtual void		clear(int v=0) {       istr_.clear(v); }
  streambuf*		rdbuf()        {return istr_.rdbuf();  }

  virtual int		get();
  virtual RWvistream&	get(char&);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t&);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char&);
#endif
  virtual RWvistream&	get(char*,   size_t N);
#ifdef RW_SEGMENTED_ARCHITECTURE
  virtual RWvistream&	get(char rwhuge*, unsigned long N);
#endif
  virtual RWvistream&	get(double*, size_t N);
  virtual RWvistream&	get(float*,  size_t N);
  virtual RWvistream&	get(int*,    size_t N);
  virtual RWvistream&	get(long*,   size_t N);
  virtual RWvistream&	get(short*,  size_t N);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t*,size_t N);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char*,  size_t N);
#endif
  virtual RWvistream&	get(unsigned short*, size_t N);
  virtual RWvistream&	get(unsigned int*,   size_t N);
  virtual RWvistream&	get(unsigned long*,  size_t N);
  virtual RWvistream&	getString(char* s,   size_t maxlen);
  virtual RWvistream&	operator>>(char&);
  virtual RWvistream&	operator>>(double&);
  virtual RWvistream&	operator>>(float&);
  virtual RWvistream&	operator>>(int&);
  virtual RWvistream&	operator>>(long&);
  virtual RWvistream&	operator>>(short&);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	operator>>(wchar_t&);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	operator>>(unsigned char&);
#endif
  virtual RWvistream&	operator>>(unsigned int&);
  virtual RWvistream&	operator>>(unsigned long&);
  virtual RWvistream&	operator>>(unsigned short&);
protected:
  void			syntaxErr(const char* expect, char was);
  char			getCChar();
  istream		istr_;
private:
  // Disallow copying:
  RWpistream(const RWpistream&);
  RWpistream& operator=(const RWpistream&);
};

/************************************************
 *						*
 *		class RWpostream		*
 *						*
 ************************************************/

class RWExport RWpostream : public RWvostream
{

public:

  RWpostream(ostream& str);
  RWpostream(streambuf* s);
  virtual ~RWpostream();

  virtual int		eof()          {return ostr_.eof();    }
  virtual int		fail()         {return ostr_.fail();   }
  virtual int		bad()          {return ostr_.bad();    }
  virtual int		good()         {return ostr_.good();   }
  virtual int		rdstate()      {return ostr_.rdstate();}
  virtual void		clear(int v=0) {       ostr_.clear(v); }
  streambuf*		rdbuf()        {return ostr_.rdbuf();  }
  
  int		        precision(int p)  { return ostr_.precision(p); }
// Some platforms do not have ostream::precision() as const fn.
  int		        precision() const { return ((ostream&)ostr_).precision(); }
  

  virtual RWvostream&	putString(const char*, size_t);
  virtual RWvostream&	operator<<(const char*);
  virtual RWvostream&	operator<<(char);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvostream&	operator<<(wchar_t);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvostream&	operator<<(unsigned char);
#endif
  virtual RWvostream&	operator<<(double);
  virtual RWvostream&	operator<<(float);
  virtual RWvostream&	operator<<(int);
  virtual RWvostream&	operator<<(unsigned int);
  virtual RWvostream&	operator<<(long);
  virtual RWvostream&	operator<<(unsigned long);
  virtual RWvostream&	operator<<(short);
  virtual RWvostream&	operator<<(unsigned short);
  virtual RWvostream&	flush();


  virtual RWvostream&	put(char);
  virtual RWvostream&	put(const char* p, size_t N);
#ifdef RW_SEGMENTED_ARCHITECTURE
  virtual RWvostream&	put(const char rwhuge* p, unsigned long N);
#endif
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvostream&	put(wchar_t);
  virtual RWvostream&	put(const wchar_t* p, size_t N);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvostream&	put(unsigned char);
  virtual RWvostream&	put(const unsigned char* p,  size_t N);
#endif
  virtual RWvostream&	put(const short* p,          size_t N);
  virtual RWvostream&	put(const unsigned short* p, size_t N);
  virtual RWvostream&	put(const int* p,            size_t N);
  virtual RWvostream&	put(const unsigned int* p,   size_t N);
  virtual RWvostream&	put(const long* p,           size_t N);
  virtual RWvostream&	put(const unsigned long* p,  size_t N);
  virtual RWvostream&	put(const float* p,          size_t N);
  virtual RWvostream&	put(const double* p,         size_t N);
protected:
  static const unsigned	MAXCOL;
  void			putwrap(char);
#ifndef RW_NO_OVERLOAD_WCHAR
  void			putwrap(wchar_t c);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  void			putwrap(unsigned char c);
#endif
  void			putwrap(const char* s, size_t len);
  void			putCChars(const char* s, size_t len);
  ostream		ostr_;
  unsigned		column_;
private:
  // Disallow copying:
  RWpostream(const RWpostream&);
  RWpostream& operator=(const RWpostream&);
};

#endif /* __RWPSTREAM_H__ */
