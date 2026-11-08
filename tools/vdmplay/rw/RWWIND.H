#ifndef __RWWIND_H__
#define __RWWIND_H__

/*
 * Microsoft Windows related directives.
 *
 * $Id: rwwind.h,v 6.3 1994/07/12 19:58:19 vriezen Exp $
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
 * $Log: rwwind.h,v $
 * Revision 6.3  1994/07/12  19:58:19  vriezen
 * Update Copyright notice
 *
 * Revision 6.2  1994/05/16  18:07:38  jims
 * Port to Win32 DLL
 *
 * Revision 6.1  1994/04/15  19:10:55  vriezen
 * Move all files to 6.1
 *
 * Revision 2.6  1993/09/11  10:32:21  jims
 * Move toward support for DLLs under Win32
 *
 * Revision 2.5  1993/09/03  02:08:13  keffer
 * Macro _CLASSDLL is now named _RWTOOLSDLL
 *
 * Revision 2.4  1993/08/05  11:44:46  jims
 * Port to WIN32
 *
 * Revision 2.3  1993/04/10  23:48:21  jims
 * No longer sets RW_MULTI_THREAD flag for DLL situation
 *
 * Revision 2.2  1993/02/07  17:41:54  keffer
 * Moved the definition for _RWCLASSTYPE from rwwind.h to
 * defs.h; this allows the library to be used with far virtual
 * tables by non-Windows code.
 *
 * Revision 2.1  1993/01/15  22:23:49  keffer
 * Tools.h++ V5.2 alpha
 *
 * Revision 2.0  1992/10/23  03:27:32  keffer
 * RCS Baseline version
 *
 * 
 *    Rev 1.4   22 Jun 1992 17:52:50   KEFFER
 * Now allows a DLL to be built that uses the Tools.h++ DLL.
 * 
 *    Rev 1.3   28 May 1992 11:04:28   KEFFER
 * Can now build static Windows library using Microsoft C/C++ 7.0
 * 
 *    Rev 1.2   17 Mar 1992 12:00:14   KEFFER
 * Fine-tuned.
 * 
 *    Rev 1.1   18 Feb 1992 09:54:42   KEFFER
 * 
 *    Rev 1.0   17 Nov 1991 13:40:14   keffer
 * Initial revision.
 *
 */

/*
 * Special stuff for 16-bit Windows (__WIN16__) 
 * and Windows NT / Win32s (__WIN32__).
 *
 * Under Windows, these header files can be used in two different modes: 
 * as part of a DLL, or as part of a regular (static) library.  
 * For each of these two ways, we can either be COMPILING the library,
 * or USING the library.  This makes a matrix of four possible situations.
 *
 *************************************************************************
 ******************************  DLL *************************************
 *************************************************************************
 *
 *                         Creating a DLL
 *
 * The macro __DLL__ should be define when compiling to create a DLL.
 *
 *     The Borland compilers automatically do this if either the -WD or 
 *     -WDE switch is being used.  In this situation, the macros 
 *     RWExport and rwexport expand to _export.
 *
 *     For other compilers, we must define __DLL__ where appropriate
 *     if the compiler doesn't.  See the example for Microsoft below.
 *
 *     RWExport and rwexport expand to _export by default.  Massage as
 *     necessary for your compiler; again see below for the Microsoft
 *     specific directives.
 *
 *                       Using the resultant DLL
 *
 * In order to use the resultant DLL, you must define the macro _RWTOOLSDLL
 * when compiling.  This will communicate to the Rogue Wave header files
 * your intention to use a DLL version of the Rogue Wave library.
 *
 * If you intend to use a DLL version of the Borland RTL as well, then you
 * should also define the macro _RTLDLL, as per Borland's instructions.
 *
 * You must also specify the -WS switch ("smart callbacks") for the 
 * Borland Compiler.
 *
 *
 *************************************************************************
 ***********************  Static Windows Library **************************
 *************************************************************************
 *
 *
 *                Creating a RW static Windows library
 *
 * If your intention is to create a Rogue Wave library to be used as 
 * as a static Windows library, then one of the macros __WIN16__ or __WIN32__
 * must have been defined in compiler.h (or by the compiler directly).
 *
 * Borland:	__WIN16__ will be defined if both _Windows and __MSDOS__ 
 *              is defined.  _Windows will be defined automatically if 
 *              any -W switch is used.  Borland defines __WIN32__ automatically
 *              in their 32-bit compiler for Windows NT / Win32s.
 *
 * Microsoft:	Microsoft automatically defines _WINDOWS if you use the
 *		/GA (preferred) or /GW switch.  __WIN16__ will be defined 
 *              if both _WINDOWS and _MSDOS are defined.
 *              __WIN32__ will only be defined if either it, or WIN32, 
 *              is explicitly defined on the cl386 command line.
 *
 * 
 *                 Using a RW static Windows library
 *
 * Nothing special needs to be done.  Just link compile with the appropriate
 * compile switches and link in the RW static Windows library.
 *
 */


/*
 * Check for Microsoft C/C++ and massage as necessary.
 */
#if defined(_MSC_VER)
#  if defined(_WINDLL) && !defined(__DLL__)
#    define __DLL__ 1
#  endif
#  if !defined(_export)
#    define _export __export
#  endif
#endif


/* For backwards compatibility: */
#if defined(_RWCLASSDLL) && !defined(_RWTOOLSDLL)
# define _RWTOOLSDLL 1
#endif

#if defined(_RWTOOLSDLL)
#  if !defined(__LARGE__) && !defined(__WIN32__)
#    error   Must use large or flat memory model when compiling or using the Tools.h++ DLL!
#  endif
#  if defined(_RWBUILDDLL)
     // Compiling the Tools.h++ DLL.
#    ifndef _MSC_VER
#      define RWExport _export	                /* Mark classes as exported */
#      define rwexport _export	                /*    & functions as well   */
#    else /* Microsoft: */
#      define RWExport __declspec(dllexport)
#      define rwexport __declspec(dllexport)
#    endif
#  else
     // Using the Tools.h++ DLL.
#    if defined(__WIN16__)
#      define RWExport huge	/* Mark classes as huge  */
#      define rwexport far	/*    & functions as far */
#    elif defined(__WIN32__)
#      ifndef _MSC_VER
#        define RWExport _import     
#        define rwexport
#      else
#        define RWExport  __declspec(dllimport)
#        define rwexport
#      endif  
#    endif
#  endif
#else
   // Neither compiling, nor using the Tools.h++ DLL.
#  define RWExport
#  define rwexport
#endif

#endif /* __RWWIND_H__ */
