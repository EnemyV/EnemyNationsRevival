#ifndef	_NWDSBUFT_HEADER_			
#define	_NWDSBUFT_HEADER_			

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

#include <time.h>

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef	_NWDSDC_HEADER_
#include <nwdsdc.h>
#endif

#ifndef	__NWDSDEFS_H
#include <nwdsdefs.h>
#endif

#ifndef	_NWDSATTR_HEADER_
#include <nwdsattr.h>
#endif

                                  
#define	INPUT_BUFFER	0x00000001

typedef struct
{
	uint32	operation;
	uint32	flags;
	uint32	maxLen;
	uint32	curLen;
	uint8		NWFAR *lastCount;
	uint8		NWFAR *curPos;
	uint8		NWFAR *data;
} Buf_T;

typedef struct
{
	uint32	objectFlags;
	uint32	subordinateCount;
	time_t	modificationTime;
	char		baseClass[MAX_SCHEMA_NAME_BYTES + 2];
} Object_Info_T;

typedef struct
{
	uint32			length;
	uint8				data[MAX_ASN1_NAME];
} Asn1ID_T;

typedef struct
{
	uint32			attrFlags;
	uint32			attrSyntaxID;
	uint32			attrLower;
	uint32			attrUpper;
	Asn1ID_T			asn1ID;
} Attr_Info_T;

typedef struct	
{
	uint32			classFlags;
	Asn1ID_T			asn1ID;		/* an object identifier created allocated   */
} Class_Info_T;					/* according to the rules specified in the  */
										/* ASN.1 standard; if no object identifier  */
										/* has been registered for the class, a     */
										/* zero-length octet string is specified    */

typedef struct
{
	uint32	ID;
	char		defStr[MAX_SCHEMA_NAME_BYTES + 2];
	int16 	flags;
} Syntax_Info_T;

#define NWDSPutClassName(c, b, n) NWDSPutClassItem(c, b, n) /* 
																					c -- context
						                                             b -- buf
                                                               n -- itemName
																				*/

#define NWDSPutSyntaxName(c, b, n) NWDSPutClassItem(c, b, n) /* 
																					c -- context
						                                             b -- buf
                                                               n -- itemName
																				*/
#ifdef __cplusplus
	extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSAllocBuf
(
	size_t	size,
	Buf_T		NWFAR * NWFAR *buf
);

NWDSCCODE NWFAR NWPASCAL NWDSComputeAttrValSize
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	syntaxID,
	uint32	NWFAR *attrValSize
);

NWDSCCODE NWFAR NWPASCAL NWDSFreeBuf
(
	Buf_T		NWFAR *buf
);

NWDSCCODE NWFAR NWPASCAL NWDSGetAttrCount
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	NWFAR *attrCount
);

NWDSCCODE NWFAR NWPASCAL NWDSGetAttrDef
(
	NWDSContextHandle			context,
	Buf_T			NWFAR *buf,
	char			NWFAR *attrName,
	Attr_Info_T	NWFAR *attrInfo
);

NWDSCCODE NWFAR NWPASCAL NWDSGetAttrName
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	char		NWFAR *attrName,
	uint32	NWFAR *attrValCount,
	uint32	NWFAR *syntaxID

);

NWDSCCODE NWFAR NWPASCAL NWDSGetAttrVal
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	syntaxID,
	void		NWFAR *attrVal
);

NWDSCCODE NWFAR NWPASCAL NWDSGetClassDef
(
 	NWDSContextHandle				context,	
	Buf_T				NWFAR *buf,
	char				NWFAR *className,
	Class_Info_T	NWFAR *classInfo
);

NWDSCCODE NWFAR NWPASCAL NWDSGetClassDefCount
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	NWFAR *classDefCount
);

NWDSCCODE NWFAR NWPASCAL NWDSGetClassItem
(
	NWDSContextHandle		context,	
	Buf_T		NWFAR *buf,
	char		NWFAR *itemName
);

NWDSCCODE NWFAR NWPASCAL NWDSGetClassItemCount
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	NWFAR *itemCount
);

NWDSCCODE NWFAR NWPASCAL NWDSGetObjectCount
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	NWFAR *objectCount
);

NWDSCCODE NWFAR NWPASCAL NWDSGetObjectName
(
	NWDSContextHandle				context,
	Buf_T				NWFAR *buf,
	char				NWFAR *objectName,
	uint32			NWFAR *attrCount,
	Object_Info_T	NWFAR *objectInfo
);

NWDSCCODE NWFAR NWPASCAL NWDSGetPartitionInfo
(
	NWDSContextHandle				context,
	Buf_T				NWFAR *buf,
	char				NWFAR *partitionName,
	uint32			NWFAR *replicaType
);

NWDSCCODE NWFAR NWPASCAL NWDSGetServerName
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	char		NWFAR *serverName,
	uint32	NWFAR *partitionCount
);

NWDSCCODE NWFAR NWPASCAL NWDSGetSyntaxCount
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	NWFAR *syntaxCount
);

NWDSCCODE NWFAR NWPASCAL NWDSGetSyntaxDef
(
	NWDSContextHandle		context, 
	Buf_T		NWFAR *buf,
	char		NWFAR *syntaxName,
	Syntax_Info_T	NWFAR *syntaxDef
);

NWDSCCODE NWFAR NWPASCAL NWDSInitBuf
(
	NWDSContextHandle		context, 
	uint32	operation,
	Buf_T		NWFAR *buf		
);

NWDSCCODE NWFAR NWPASCAL NWDSPutAttrName
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	char		NWFAR *attrName
);

NWDSCCODE NWFAR NWPASCAL NWDSPutAttrVal
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	uint32	syntaxID,
	void		NWFAR *attrVal
);

NWDSCCODE NWFAR NWPASCAL NWDSPutChange
(
	NWDSContextHandle 		context,	
	Buf_T		NWFAR *buf,	
	uint32	changeType,	
	char  	NWFAR *attrName
);

NWDSCCODE NWFAR NWPASCAL NWDSPutClassItem
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf,
	char		NWFAR *itemName
);

NWDSCCODE NWFAR NWPASCAL NWDSBeginClassItem
(
	NWDSContextHandle		context,
	Buf_T		NWFAR *buf
);

#ifdef __cplusplus
	}
#endif

#endif									/* #ifndef _NWDSBUFT_HEADER_ */
