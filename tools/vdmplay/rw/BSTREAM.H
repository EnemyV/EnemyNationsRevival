#ifndef __RWBSTREAM_H__
#define __RWBSTREAM_H__

/*
 * rwb[io]stream ---	Use Binary reads and writes
 *
 * $Id: bstream.h,v 6.4 1994/07/12 18:45:56 vriezen Exp $
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
 * $Log: bstream.h,v $
 * Revision 6.4  1994/07/12  18:45:56  vriezen
 * Update Copyright notice
 *
 * Revision 6.3  1994/06/18  01:32:46  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.2  1994/06/16  04:28:48  vriezen
 * Added flush() support.
 *
 * Revision 6.1  1994/04/15  19:09:27  vriezen
 * Move all files to 6.1
 *
 * Revision 2.10  1994/02/05  02:02:17  jims
 * Add huge pointer support for character reads and writes
 *
 * Revision 2.9  1993/11/17  04:12:46  keffer
 * Added function putString(), to allow support of embedded nulls
 *
 * Revision 2.8  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.7  1993/04/15  02:45:18  keffer
 * Changed inheritance order to avoid cfront bug
 *
 * Revision 2.6  1993/04/14  21:03:52  keffer
 * Changed inheritance hierarchy to allow XDR streams, which cannot use
 * streambuf model.
 *
 * Revision 2.4  1993/03/13  00:12:23  keffer
 * Removed no-longer-used function checkRead();
 *
 * Revision 2.2  1992/11/26  21:04:33  myersn
 * add members to operate on wchar_t values and strings.
 *
 * Revision 2.1  1992/11/19  05:45:01  keffer
 * Introduced new <rw/compiler.h> macro directives
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 *    Rev 1.5   05 Nov 1991 13:51:40   keffer
 * Can now live in the DLL.
 * 
 *    Rev 1.4   17 Oct 1991 09:12:40   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.2   29 Jul 1991 11:33:32   keffer
 * Macro RW_NO_OVERLOAD_UCHAR checks for overloaded unsigned char
 * 
 *    Rev 1.1   24 Jul 1991 13:06:40   keffer
 * Added pvcs keywords
 *
 */

#include "rw/vstream.h"

/************************************************
 *						*
 *		class RWbistream		*
 *						*
 ************************************************/

class RWExport RWbistream : public virtual ios, public RWvistream
{

public:

  RWbistream(istream& str);
  RWbistream(streambuf* s);
  virtual ~RWbistream();

  virtual int		eof()          {return ios::eof();    }
  virtual int		fail()         {return ios::fail();   }
  virtual int		bad()          {return ios::bad();    }
  virtual int		good()         {return ios::good();   }
  virtual int		rdstate()      {return ios::rdstate();}
  virtual void		clear(int v=0) {       ios::clear(v); }

  virtual int		get();
  virtual RWvistream&	get(char&);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t&);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char&);
#endif
  virtual RWvistream&	get(char*  , size_t);
#ifdef RW_SEGMENTED_ARCHITECTURE
  virtual RWvistream&	get(char rwhuge*    , unsigned long);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char*  , size_t);
#endif
  virtual RWvistream&	get(         double*, size_t);
  virtual RWvistream&	get(         float* , size_t);
  virtual RWvistream&	get(         int*   , size_t);
  virtual RWvistream&	get(unsigned int*   , size_t);
  virtual RWvistream&	get(         long*  , size_t);
  virtual RWvistream&	get(unsigned long*  , size_t);
  virtual RWvistream&	get(         short* , size_t);
  virtual RWvistream&	get(unsigned short* , size_t);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t*, size_t N);
#endif
  virtual RWvistream&	getString(char* s  , size_t maxlen);
  virtual RWvistream&	operator>>(char&  );
  virtual RWvistream&	operator>>(double&);
  virtual RWvistream&	operator>>(float& );
  virtual RWvistream&	operator>>(int&   );
  virtual RWvistream&	operator>>(long&  );
  virtual RWvistream&	operator>>(short& );
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	operator>>(wchar_t&);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	operator>>(unsigned char& );
#endif
  virtual RWvistream&	operator>>(unsigned int&  );
  virtual RWvistream&	operator>>(unsigned long& );
  virtual RWvistream&	operator>>(unsigned short&);

};

/************************************************
 *						*
 *		class RWbostream		*
 *						*
 ************************************************/

class RWExport RWbostream : public virtual ios, public RWvostream
{

public:

  RWbostream(ostream& str);
  RWbostream(streambuf* s);
  virtual ~RWbostream();

  virtual int		eof()          {return ios::eof();    }
  virtual int		fail()         {return ios::fail();   }
  virtual int		bad()          {return ios::bad();    }
  virtual int		good()         {return ios::good();   }
  virtual int		rdstate()      {return ios::rdstate();}
  virtual void		clear(int v=0) {       ios::clear(v); }

  virtual RWvostream&	putString(const char*, size_t);
  virtual RWvostream&	operator<<(const char*);
  virtual RWvostream&	operator<<(char);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvostream&	operator<<(wchar_t);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvostream&	operator<<(unsigned char);
#endif
  virtual RWvostream&	operator<<(         double);
  virtual RWvostream&	operator<<(         float );
  virtual RWvostream&	operator<<(         int   );
  virtual RWvostream&	operator<<(unsigned int   );
  virtual RWvostream&	operator<<(         long  );
  virtual RWvostream&	operator<<(unsigned long  );
  virtual RWvostream&	operator<<(         short );
  virtual RWvostream&	operator<<(unsigned short );
  virtual RWvostream&   flush();

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
  virtual RWvostream&	put(const unsigned char* p, size_t N);
#endif
  virtual RWvostream&	put(const          short* , size_t);
  virtual RWvostream&	put(const unsigned short* , size_t);
  virtual RWvostream&	put(const          int*   , size_t);
  virtual RWvostream&	put(const unsigned int*   , size_t);
  virtual RWvostream&	put(const          long*  , size_t);
  virtual RWvostream&	put(const unsigned long*  , size_t);
  virtual RWvostream&	put(const          float* , size_t);
  virtual RWvostream&	put(const          double*, size_t);

};

#endif /* __RWBSTREAM_H__ */

