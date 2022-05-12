#ifndef	_NWDSNAME_HEADER_
#define	_NWDSNAME_HEADER_

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


#ifdef __cplusplus
	extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSAbbreviateName
(
	NWDSContextHandle	context,
	char	NWFAR *inName,
	char	NWFAR *abbreviatedName
);

NWDSCCODE NWFAR NWPASCAL NWDSCanonicalizeName
(
	NWDSContextHandle	context,
	char	NWFAR *objectName,
	char	NWFAR *canonName
);

NWDSCCODE NWFAR NWPASCAL NWDSRemoveAllTypes
(
	NWDSContextHandle	context,
	char	NWFAR *name,
	char	NWFAR *typelessName
);

NWDSCCODE NWFAR NWPASCAL NWDSResolveName
(
	NWDSContextHandle context,
	char		NWFAR *objectName,
	NWCONN_HANDLE		NWFAR *conn,
	uint32	NWFAR *objectID
);

NWDSCCODE NWAPI NWDSCIStringsMatch
(
   NWDSContextHandle context,
   char  NWFAR       *string1,
   char  NWFAR       *string2,
   int   NWFAR       *matches
);

#ifdef __cplusplus
	}
#endif

#endif									/* #ifndef _NWDSNAME_HEADER_ */

