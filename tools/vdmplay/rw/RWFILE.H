#ifndef __RWFILE_H__
#define __RWFILE_H__

/*
 * Class RWFile encapsulates ANSI-C binary file operations.   
 *
 * $Id: rwfile.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwfile.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/06/13  19:50:07  vriezen
 * 429.  Fixed dtor to use delete [] and declared copy ctor and op= as private.
 *
 * Revision 6.1  1994/04/15  19:10:47  vriezen
 * Move all files to 6.1
 *
 * Revision 2.11  1994/02/05  02:02:17  jims
 * Add huge pointer support for character reads and writes
 *
 * Revision 2.10  1993/09/10  02:56:53  keffer
 * Switched RCS idents to avoid spurious diffs
 *
 * Revision 2.9  1993/05/18  00:41:46  keffer
 * Now uses new exception handling facility
 *
 * Revision 2.8  1993/05/01  18:17:09  keffer
 * Added overloaded versions for unsigned short and long.
 *
 * Revision 2.7  1993/04/09  19:35:45  keffer
 * Indexing is now done using size_t
 *
 * Revision 2.6  1993/03/25  01:52:09  keffer
 * Added optional "mode" argument to the constructor;
 * Added member function "isValid()".
 *
 * Revision 2.5  1993/03/20  17:44:17  keffer
 * Array lengths now given by 'unsigned'
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
 * 
 *    Rev 1.5   18 Feb 1992 09:54:38   KEFFER
 * 
 *    Rev 1.4   17 Oct 1991 09:12:56   keffer
 * Changed include path to <rw/xxx.h>
 * 
 *    Rev 1.2   29 Jul 1991 11:03:38   keffer
 * Made Exists(const char*) a static function.
 * 
 *    Rev 1.1   24 Jul 1991 13:06:48   keffer
 * Added pvcs keywords
 */

/*
 * Under Unix, there there is no difference between text and binary 
 * files.  However, under MS-DOS, and some other operating systems, 
 * files opened for *text* output will use a CR/LF convention to end 
 * text lines, but do nothing for binary files.  Because RWFile is 
 * intended to model *binary* files, we must shut off this translation.
 * This requires opening the file in an explicit "binary" mode.  
 *
 * IF YOU INTEND FOR RWFILE TO WORK UNDER DOS AND USE THE OPTIONAL
 * "MODE" PARAMETER IN THE CONSTRUCTOR, YOU MUST BE SURE TO USE THE
 * "b" QUALIFIER!
 *
 *  RWFile myFile("junk.dat", "wb+");	// NB: "wb+"  NOT  "w+" !!
 *
 */

#include "rw/defs.h"
STARTWRAP
#include <stdio.h>
ENDWRAP

class RWExport RWFile
{
  RWFile(const RWFile&);  // Not implemented!
  void operator=(const RWFile&); // Not implemented!
public:
  RWFile(const char* name, const char* mode = rwnil);
  ~RWFile();
       
  const char*		GetName()   const {return filename;    }
  FILE*			GetStream() const {return filep;       }
  RWBoolean		isValid()   const {return filep!=rwnil;}
  RWBoolean 		Exists();
  static RWBoolean 	Exists(const char* name);
       
  RWBoolean 		Read(char& c);	
  RWBoolean 		Read(short& i);
  RWBoolean 		Read(int& i);
  RWBoolean 		Read(long& i);
#ifndef RW_NO_OVERLOAD_WCHAR
  RWBoolean 		Read(wchar_t& w);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  RWBoolean 		Read(unsigned char& c);
#endif
  RWBoolean 		Read(unsigned short&);
  RWBoolean 		Read(unsigned int&  );
  RWBoolean 		Read(unsigned long& );
  RWBoolean 		Read(float& );
  RWBoolean 		Read(double&);
  RWBoolean		Read(char*  , size_t N);
#ifdef RW_SEGMENTED_ARCHITECTURE
  RWBoolean		Read(char rwhuge*  , unsigned long N);
#endif
  RWBoolean 		Read(short* , size_t N);
  RWBoolean 		Read(int*   , size_t N);
  RWBoolean		Read(long*  , size_t N);
#ifndef RW_NO_OVERLOAD_WCHAR
  RWBoolean		Read(wchar_t* w, size_t N);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  RWBoolean		Read(unsigned char* c, size_t N){return Read((char*)c, N);}
#endif
  RWBoolean		Read(unsigned short* i, size_t N) {return Read((short*)i, N);}
  RWBoolean		Read(unsigned int*   i, size_t N) {return Read((int  *)i, N);}
  RWBoolean		Read(unsigned long*  i, size_t N) {return Read((long *)i, N);}
  RWBoolean		Read(float* , size_t N);
  RWBoolean		Read(double*, size_t N);

  // Read to null terminator or EOF; no CR/LF translation will be done. Beware of overflow.
  RWBoolean		Read(char* string);	
  
  RWBoolean		Write(char c);
  RWBoolean		Write(short s);
  RWBoolean		Write(int i);
  RWBoolean		Write(long l);
#ifndef RW_NO_OVERLOAD_WCHAR
  RWBoolean		Write(wchar_t w);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  RWBoolean		Write(unsigned char c);
#endif
  RWBoolean		Write(unsigned short s);
  RWBoolean		Write(unsigned int i);
  RWBoolean		Write(unsigned long l);
  RWBoolean		Write(float f);
  RWBoolean		Write(double d);
  RWBoolean		Write(const char* string);
  RWBoolean		Write(const short* , size_t N);
  RWBoolean		Write(const int*   , size_t N);
  RWBoolean		Write(const long*  , size_t N);
#ifndef RW_NO_OVERLOAD_WCHAR
  RWBoolean		Write(const wchar_t* w, size_t N);
#endif
#ifndef RW_NO_OVERLOAD_UCHAR
  RWBoolean		Write(const unsigned char* c, size_t N){return Write((const char*)c, N);}
#endif
  RWBoolean		Write(const unsigned short* i, size_t N){return Write((const short*)i, N);}
  RWBoolean		Write(const unsigned int*   i, size_t N){return Write((const int  *)i, N);}
  RWBoolean		Write(const unsigned long*  i, size_t N){return Write((const long *)i, N);}
  RWBoolean		Write(const float*  , size_t N);
  RWBoolean		Write(const double* , size_t N);
  RWBoolean		Write(const char* string, size_t N);
#ifdef RW_SEGMENTED_ARCHITECTURE
  RWBoolean		Write(const char rwhuge* string, unsigned long N);
#endif
  
  long			CurOffset(); // Returns current offset of file
  RWBoolean		Eof();	     // TRUE if file at EOF
  RWBoolean		Erase();
  RWBoolean		Error();     // TRUE if the file has had an error.
  RWBoolean		Flush();     // Writes all pending output
  RWBoolean		IsEmpty();   // TRUE if the file is empty
  RWBoolean		SeekTo(long offset); /* offset bytes from beginning of file */
  RWBoolean		SeekToBegin()	{return SeekTo(0);}
  RWBoolean		SeekToEnd();

protected:

  char*			filename;
  FILE*			filep;
};

#endif  /* __RWFILE_H__ */
