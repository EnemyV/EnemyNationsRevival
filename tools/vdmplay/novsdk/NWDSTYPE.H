/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef __NWDSTYPE_H
#define __NWDSTYPE_H

#ifdef NWNLM
	#define NWFAR
	#define NWPASCAL
#endif

#ifndef MACINTOSH

typedef unsigned long uint32;
typedef signed long int32;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned char uint8;
typedef signed char int8;

#ifndef NWCALDEF_INC
#include <nwcaldef.h>
#endif

#ifndef NWDSCCODE
#define NWDSCCODE		int
#endif

#ifndef NWUNSIGNED
#define NWUNSIGNED unsigned
#endif

#else		/* for the Macintosh... */
#define NWDSCCODE    long

#ifndef NWFAR
#define NWFAR			far
#endif

#ifndef NWPASCAL
#define NWPASCAL		pascal
#endif

#ifndef NWAPI
#define NWAPI			NWFAR NWPASCAL
#endif

#define NWUNSIGNED unsigned long
#ifndef uchar
# define uchar	unsigned char
#endif
#ifndef ushort
# define ushort	unsigned short
#endif
#ifndef ulong
# define ulong	unsigned long
#endif

#ifndef uint8
# define uint8	unsigned char
#endif
#ifndef uint16
# define uint16	unsigned short
#endif
#ifndef uint32
# define uint32	unsigned long
#endif

#ifndef int8
# define int8	char
#endif
#ifndef int16
# define int16	short
#endif
#ifndef int32
# define int32	long
#endif

#include "NWMacTyp.h"
#endif

#endif
