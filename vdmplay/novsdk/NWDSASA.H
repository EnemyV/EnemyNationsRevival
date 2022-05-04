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
#ifndef	_NWDSASA_HEADER_
#define	_NWDSASA_HEADER_

#ifndef	_NWDSDC_HEADER_
#include "nwdsdc.h"		/* for NWDSContextHandle typedef	*/
#endif

#ifndef NWCONNECT_INC
#include "nwconnec.h"
#endif

#define SESSION_KEY_SIZE	16
typedef	uint8	NWDS_Session_Key_T[SESSION_KEY_SIZE];	/* Optional session key */

#define	GENERATE_CERTIFICATION_KP_F			1

#ifdef MACINTOSH
#include "nwcaldef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSAuthenticate
(
	NWCONN_HANDLE		conn,
	uint32				optionsFlag, 
	NWDS_Session_Key_T NWFAR *sessionKey
);

NWDSCCODE NWFAR NWPASCAL NWDSChangeObjectPassword
(
	NWDSContextHandle context, 
	uint32 optionsFlag, 
	char NWFAR *objectName, 
	char NWFAR *oldPassword,
	char NWFAR *newPassword
);

NWDSCCODE NWFAR NWPASCAL NWDSGenerateObjectKeyPair
(
	NWDSContextHandle	contextHandle,
	char		NWFAR *objectName,
	char		NWFAR *objectPassword,
	uint32	optionsFlag
);

NWDSCCODE NWFAR NWPASCAL NWDSLogin
(
	NWDSContextHandle	context, 
	uint32	optionsFlag, 
	char NWFAR *objectName, 
	char NWFAR *password,
	uint32 validityPeriod
);

NWDSCCODE NWFAR NWPASCAL NWDSLogout
(
	NWDSContextHandle	context
);

NWDSCCODE NWFAR NWPASCAL NWDSVerifyObjectPassword
(
	NWDSContextHandle context, 
	uint32 optionsFlag,
	char NWFAR *objectName,
	char NWFAR *password
);

#ifdef __cplusplus
}
#endif

#endif


