#ifndef	_NWDSAUD_HEADER_
#define	_NWDSAUD_HEADER_

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

#ifndef NWCONNECT_INC
#include <nwconnec.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSAuditGetObjectID
(
	NWDSContextHandle context,
	char		NWFAR *objectName,
	NWCONN_HANDLE		NWFAR *conn,
	uint32	NWFAR *objectID
);

#ifdef __cplusplus
}
#endif

#endif									/* #ifndef _NWDSAUD_HEADER_ */
