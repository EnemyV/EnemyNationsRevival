#ifndef	_NWDSSCH_HEADER_
#define	_NWDSSCH_HEADER_

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

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif

#ifndef	_NWDSATTR_HEADER_
#include <nwdsattr.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSDefineAttr
(
	NWDSContextHandle	context,
	char			NWFAR *attrName,
	Attr_Info_T	NWFAR *attrDef
);

NWDSCCODE NWFAR NWPASCAL NWDSDefineClass
(
	NWDSContextHandle	context,
	char		NWFAR *className,
	Class_Info_T	NWFAR *classInfo,
	Buf_T				NWFAR *classItems
);

NWDSCCODE NWFAR NWPASCAL NWDSListContainableClasses
(
	NWDSContextHandle	context,
	char		NWFAR *parentObject,
	int32		NWFAR *iterationHandle,
	Buf_T		NWFAR *containableClasses
);

NWDSCCODE NWFAR NWPASCAL NWDSModifyClassDef
(
	NWDSContextHandle	context,
	char		NWFAR *className,
	Buf_T		NWFAR *optionalAttrs
);

NWDSCCODE NWFAR NWPASCAL NWDSReadAttrDef
(
	NWDSContextHandle	context,
	uint32	infoType,
	uint8		allAttrs,
	Buf_T		NWFAR *attrNames,
	int32		NWFAR *iterationHandle,
	Buf_T		NWFAR *attrDefs
);

NWDSCCODE NWFAR NWPASCAL NWDSReadClassDef
(
	NWDSContextHandle	context,
	uint32	infoType,
	uint8		allClasses,
	Buf_T		NWFAR *classNames,
	int32		NWFAR *iterationHandle,
	Buf_T		NWFAR *classDefs
);

NWDSCCODE NWFAR NWPASCAL NWDSRemoveAttrDef
(
	NWDSContextHandle	context,
	char		NWFAR *attrName
);

NWDSCCODE NWFAR NWPASCAL NWDSRemoveClassDef
(
	NWDSContextHandle	context,
	char		NWFAR *className
);

#ifdef __cplusplus
}
#endif

#endif
