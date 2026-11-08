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

#ifndef	_NWDSDC_HEADER_
#define	_NWDSDC_HEADER_

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef _UNICODE_HEADER_
#include <unicode.h>
#endif

 
/*	Directory Context Key names */

#define	DCK_FLAGS				1
#define	DCK_CONFIDENCE			2
#define	DCK_NAME_CONTEXT		3
#define	DCK_TRANSPORT_TYPE	4
#define	DCK_REFERRAL_SCOPE	5
#define  DCK_LAST_CONNECTION	8

/* bit definitions for DCK_FLAGS key */

#define	DCV_DEREF_ALIASES			0x00000001L
#define	DCV_XLATE_STRINGS			0x00000002L
#define	DCV_TYPELESS_NAMES		0x00000004L
#define	DCV_ASYNC_MODE				0x00000008L
#define	DCV_CANONICALIZE_NAMES	0x00000010L
#define	DCV_DEREF_BASE_CLASS    0x00000040L

/* values for DCK_CONFIDENCE key */

#define	DCV_LOW_CONF			0
#define	DCV_MED_CONF			1
#define	DCV_HIGH_CONF			2

#define	MAX_MESSAGE_LEN				(63*1024)
#define	DEFAULT_MESSAGE_LEN			(4*1024)

/* values for DCK_REFERRAL_SCOPE key */

#define	DCV_ANY_SCOPE					0
#define	DCV_COUNTRY_SCOPE				1
#define	DCV_ORGANIZATION_SCOPE		2
#define	DCV_LOCAL_SCOPE				3

typedef	uint32	NWDSContextHandle;

#ifdef __cplusplus
	extern "C" {
#endif

NWDSContextHandle NWFAR NWPASCAL NWDSCreateContext
(
	void
);

NWDSContextHandle NWFAR NWPASCAL NWDSDuplicateContext
(
	NWDSContextHandle oldContext
);

NWDSCCODE NWFAR NWPASCAL NWDSFreeContext
(
	NWDSContextHandle	context
);

NWDSCCODE NWFAR NWPASCAL NWDSGetContext
(
	NWDSContextHandle	context,
	int	            key,
	void 	NWFAR       *value
);

NWDSCCODE NWFAR NWPASCAL NWDSSetContext
(
	NWDSContextHandle	context,
	int	            key,
	void 	NWFAR       *value
);

#ifdef __cplusplus
	}
#endif

#endif									
