#ifndef __RWXDRSTREA_H__
#define __RWXDRSTREA_H__

/*
 * rwxdr[io]stream ---	Portable I/O streams based on XDR format.
 *
 * $Id: xdrstrea.h,v 6.3 1994/06/18 03:14:42 myersn Exp $
 *
 ****************************************************************************
 *
 *	Copyright (C) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 *
 *	@(#)
 *
 ***************************************************************************
 *
 * $Log: xdrstrea.h,v $
 * Revision 6.3  1994/06/18  03:14:42  myersn
 * add out-of-line virtual dtor.
 *
 * Revision 6.2  1994/06/16  04:29:14  vriezen
 * Added flush() support.
 *
 * Revision 6.1  1994/04/15  19:48:56  vriezen
 * Move all files to 6.1
 *
 * Revision 1.5  1993/11/17  04:12:46  keffer
 * Added function putString(), to allow support of embedded nulls
 *
 * Revision 1.4  1993/09/16  00:48:16  keffer
 * Reordered include files to avoid xlC multiple definition bug.
 *
 * Revision 1.3  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 1.2  1993/04/24  01:46:10  keffer
 * Added SCCS keyword
 *
 * Revision 1.1  1993/04/14  21:12:55  keffer
 * Initial revision
 *
 *
 */

#include "rw/compiler.h"
STARTWRAP
#include <stddef.h>	/* Required for IBM xlC */
#include <stdio.h>	/* Required for IBM xlC */
#include <rpc/types.h>	/* Due to bug in some rpc/xdr.h. */
/* Sun cfront V2.X has bug in prototype for xdr_u_char(); see toolread.doc */
#include <rpc/xdr.h>
ENDWRAP
#include "rw/vstream.h"

class RWExport RWCollectable;

int xdr(XDR*, RWCollectable*&);

/************************************************
 *						*
 *		class RWXDRIStream		*
 *						*
 ************************************************/

class RWExport RWXDRistream : public RWvistream, public RWios
{

public:

  RWXDRistream(XDR*);
  virtual ~RWXDRistream();

  virtual int		eof()           {return RWios::eof();    }
  virtual int		fail()          {return RWios::fail();   }
  virtual int		bad()           {return RWios::bad();    }
  virtual int		good()          {return RWios::good();   }
  virtual int		rdstate()       {return RWios::rdstate();}
  virtual void		clear(int v=0)  {       RWios::clear(v); }

  virtual int		get();
  virtual RWvistream&	get(char&);
#ifndef RW_NO_OVERLOAD_WCHAR
  virtual RWvistream&	get(wchar_t&);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  virtual RWvistream&	get(unsigned char&);
#endif
  virtual RWvistream&	get(char*  , size_t);
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

private:

  XDR*	xdrp;
  int	status;

};

/************************************************
 *						*
 *		class RWXDROStream		*
 *						*
 ************************************************/

class RWExport RWXDRostream : public RWvostream, public RWios
{

public:

  RWXDRostream(XDR*);
  virtual ~RWXDRostream();

  virtual int		eof()           {return RWios::eof();    }
  virtual int		fail()          {return RWios::fail();   }
  virtual int		bad()           {return RWios::bad();    }
  virtual int		good()          {return RWios::good();   }
  virtual int		rdstate()       {return RWios::rdstate();}
  virtual void		clear(int v=0)  {       RWios::clear(v); }

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
  virtual RWvostream&   flush() {return *this;};

  virtual RWvostream&	put(char);
  virtual RWvostream&	put(const char* p, size_t N);
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

private:

  XDR*	xdrp;
  int	status;

};

#endif /* __RWXDRSTREA_H__ */
