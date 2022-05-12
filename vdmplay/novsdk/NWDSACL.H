#ifndef _NWDSACL_HEADER_
#define _NWDSACL_HEADER_

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif

#ifndef	_NWDSDC_HEADER_
#include <nwdsdc.h>
#endif


#ifdef __cplusplus
	extern "C" {
#endif

NWDSCCODE NWFAR NWPASCAL NWDSGetEffectiveRights
(
	NWDSContextHandle context,
	char	NWFAR *subjectName,
	char	NWFAR *objectName,
	char	NWFAR *attrName,
	uint32	NWFAR *privileges
);

NWDSCCODE NWFAR NWPASCAL NWDSListAttrsEffectiveRights
(
	NWDSContextHandle		context,
	char		NWFAR *objectName,
	char		NWFAR *subjectName,
	uint8		allAttrs,
	Buf_T		NWFAR *attrNames,
	int32		NWFAR *iterationHandle,
	Buf_T		NWFAR *privilegeInfo
);

#ifdef __cplusplus
	}
#endif

#endif

