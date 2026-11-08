#ifndef	_NWDSFILT_HEADER_
#define	_NWDSFILT_HEADER_

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
#include <nwdstype.h>
#endif

#ifndef	_NWDSDC_HEADER_
#include <nwdsdc.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif


typedef struct _filter_node
{
   struct _filter_node  NWFAR *parent;
   struct _filter_node  NWFAR *left;
   struct _filter_node  NWFAR *right;
   void                 NWFAR *value;
   uint32               syntax;
   uint16               token;
} Filter_Node_T;

#define FTOK_END     0
#define FTOK_OR      1
#define FTOK_AND     2
#define FTOK_NOT     3
#define FTOK_LPAREN  4
#define FTOK_RPAREN  5
#define FTOK_AVAL    6
#define FTOK_EQ      7
#define FTOK_GE      8
#define FTOK_LE      9
#define FTOK_APPROX  10
#define FTOK_ANAME   14
#define FTOK_PRESENT 15
#define FTOK_RDN     16
#define FTOK_BASECLS 17
#define FTOK_MODTIME 18
#define FTOK_VALTIME 19

#define FBIT_END     (1L << FTOK_END)
#define FBIT_OR      (1L << FTOK_OR)
#define FBIT_AND     (1L << FTOK_AND)
#define FBIT_NOT     (1L << FTOK_NOT)
#define FBIT_LPAREN  (1L << FTOK_LPAREN)
#define FBIT_RPAREN  (1L << FTOK_RPAREN)
#define FBIT_AVAL    (1L << FTOK_AVAL)
#define FBIT_EQ      (1L << FTOK_EQ)
#define FBIT_GE      (1L << FTOK_GE)
#define FBIT_LE      (1L << FTOK_LE)
#define FBIT_APPROX  (1L << FTOK_APPROX)
#define FBIT_ANAME   (1L << FTOK_ANAME)
#define FBIT_PRESENT (1L << FTOK_PRESENT)
#define FBIT_RDN     (1L << FTOK_RDN)
#define FBIT_BASECLS (1L << FTOK_BASECLS)
#define FBIT_MODTIME (1L << FTOK_MODTIME)
#define FBIT_VALTIME (1L << FTOK_VALTIME)

#define FBIT_OPERAND (FBIT_LPAREN | FBIT_NOT | FBIT_PRESENT | FBIT_RDN \
         | FBIT_BASECLS | FBIT_ANAME | FBIT_MODTIME | FBIT_VALTIME)
#define FBIT_RELOP   (FBIT_EQ | FBIT_GE | FBIT_LE | FBIT_APPROX)
#define FBIT_BOOLOP  (FBIT_AND | FBIT_OR)

typedef struct
{
   Filter_Node_T NWFAR *fn;
   uint16      level;
   uint32      expect;
} Filter_Cursor_T;

#define FTAG_ITEM    0
#define FTAG_OR      1
#define FTAG_AND     2
#define FTAG_NOT     3


#ifdef __cplusplus
	extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSAddFilterToken
(
	Filter_Cursor_T  NWFAR *cur,
	uint16         tok,
	void           NWFAR *val,
	uint32         syntax
);

NWDSCCODE NWFAR NWPASCAL NWDSAllocFilter
(
	Filter_Cursor_T NWFAR * NWFAR *cur
);

void NWFAR NWPASCAL NWDSFreeFilter
(
	Filter_Cursor_T NWFAR *cur,
	void (NWFAR *freeVal)(uint32 syntax, void NWFAR *val)
);

NWDSCCODE NWFAR NWPASCAL NWDSPutFilter
(
	NWDSContextHandle context,
	Buf_T NWFAR *buf,
	Filter_Cursor_T NWFAR *cur,
	void (NWFAR *freeVal)(uint32 syntax, void NWFAR *val)
);

NWDSCCODE NWFAR NWPASCAL NWDSDelFilterToken
(
	Filter_Cursor_T NWFAR *cur,
	void (NWFAR *freeVal)(uint32 syntax, void NWFAR *val)
);

#ifdef __cplusplus
	}
#endif

#endif									/* #ifndef _NWDSFILT_HEADER_ */

