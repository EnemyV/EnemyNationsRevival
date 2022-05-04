#ifndef	_NWDSATTR_HEADER_
#define	_NWDSATTR_HEADER_

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


#define	TIME_BITMAP_LEN					42
#define  NUM_POSTAL_ADDRESS_ELEMENTS	6

typedef	char 	NWFAR *CE_String_T;
typedef	char 	NWFAR *Class_Name_T;
typedef	char 	NWFAR *CI_String_T;
typedef	char 	NWFAR *CN_String_T;
typedef 	char	NWFAR *DN_T;
typedef	int32	Integer_T;
typedef	uint8	Boolean_T;
typedef	char 	NWFAR *NU_String_T;
typedef	char 	NWFAR *Postal_Address_T[NUM_POSTAL_ADDRESS_ELEMENTS];
typedef	char 	NWFAR *PR_String_T;
typedef	char 	NWFAR *Secure_Name_T;
typedef	char 	NWFAR *TN_String_T;
typedef	uint32 Counter_T;

typedef struct
{
	uint32	remoteID;
	char		NWFAR *objectName;
} Back_Link_T;

typedef struct	
{
	uint32		numOfBits;
	uint8			NWFAR *data;
} Bit_String_T;

typedef	struct _ci_list
{
	struct _ci_list 	NWFAR *next;
	char					NWFAR *s;
} CI_List_T;

typedef	struct 
{
	char 				NWFAR *telephoneNumber;
	Bit_String_T	parameters;
}Fax_Number_T;

typedef struct
{
	char		NWFAR *objectName;
	uint32	level;
	uint32	interval;
} Typed_Name_T;

typedef struct	
{
	uint32		addressType;
	uint32		addressLength;
	uint8			NWFAR *address;
} Net_Address_T;

typedef	struct 
{
 	char			NWFAR *protectedAttrName;
	char			NWFAR *subjectName;
	uint32		privileges;
} Object_ACL_T;

typedef	struct 
{
	uint32		length;
	uint8			NWFAR *data;
} Octet_String_T;

typedef Octet_String_T	Stream_T;

typedef	struct _octet_list
{
	struct _octet_list	NWFAR *next;
	uint32		  			length;
	uint8			  			NWFAR *data;
} Octet_List_T;

typedef struct
{
	char		NWFAR *objectName;
	uint32	amount;
} Hold_T;

typedef struct	
{
	char		NWFAR	*serverName;
	int32				replicaType;
	int32				replicaNumber;
	uint32			count;
	Net_Address_T	replicaAddressHint[1];
} Replica_Pointer_T;
						 
typedef struct
{
	uint32	type;
	char		NWFAR *address;
} EMail_Address_T;

typedef struct
{
	uint32	nameSpaceType;
	char		NWFAR *volumeName;
	char		NWFAR *path;
} Path_T;

typedef struct
{
     uint32	wholeSeconds;
     uint32	eventID;
} NWDS_TimeStamp_T;

typedef struct
{
   uint32   wholeSeconds;
   uint16   replicaNum;
   uint16   eventID;
} TimeStamp_T;

typedef struct
{
	char		NWFAR *attrName;
	uint32	syntaxID;
	uint32	valueLen;
	void		NWFAR *value;	 
} Unknown_Attr_T;

#endif									/* #ifndef _NWDSATTR_HEADER_ */

