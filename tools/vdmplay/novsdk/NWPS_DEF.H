/*
//ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ (c) 1993 Novell, Inc.	All rights reserved.                              บ
//บ																								  บ
//บ THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND		  บ
//บ TREATIES.																					  บ
//บ																								  บ
//บ NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED, COPIED,			  บ
//บ DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED,         บ
//บ EXPANDED, COLLECTED, COMPILED, LINKED, RECAST, TRANSFORMED OR ADAPTED	  บ
//บ WITHOUT THE PRIOR WRITTEN CONSENT OF NOVELL, INC.                        บ
//บ																								  บ
//บ ANY USE OR EXPLOITATION OF THIS WORK WITHOUT AUTHORIZATION COULD SUBJECT บ
//บ THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.								  บ
//ฬออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน
//บ Source module name: NWPS_DEF.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_DEF_INC
#define NWPS_DEF_INC

/*
//  My definitions in case someone forgot theirs 
*/
#ifndef DWORD
#define DWORD				unsigned long
#endif

#ifndef WORD
#define WORD				unsigned short
#endif

#ifndef BYTE
#define BYTE				unsigned char
#endif

#ifndef NWCCODE
#define NWCCODE			unsigned int
#endif

#ifdef MAX_DN_BYTES
#define NWPS_MAX_NAME_SIZE             MAX_DN_BYTES
#else
#define NWPS_MAX_NAME_SIZE             514
#endif

#ifdef OBJECT_NAME_SIZE
#define NWPS_BIND_NAME_SIZE				OBJECT_NAME_SIZE
#else
#define NWPS_BIND_NAME_SIZE				48
#endif

#ifndef NWPASCAL
#ifdef NWNLM
#define NWPASCAL
#else
#define NWPASCAL			pascal
#endif /* NWNLM */
#endif /* NWPASCAL */

#ifndef NWFAR
#ifdef NWNLM
#define NWFAR
#else
#define NWFAR				far
#endif /* NWNLM */
#endif /* NWFAR */


#ifndef NWAPI
#define NWAPI NWFAR NWPASCAL
#endif

#ifndef NWPTR
#define NWPTR NWFAR *
#endif

/*=====================================
	ConnectionType values used troughout
  =====================================*/
#define NWPS_BINDERY_SERVICE				0		/* ID is a connection id		*/
#define NWPS_DIRECTORY_SERVICE			1		/* ID is a context id			*/
#define NWPS_BINDERY_SERVICE_PRE_40    2		/* ID is a connection id		*/
/* This value is compile time only */
#define NWPS_BINDERY_SERVICE_ALL      -2		/* ID is a connection id		*/

/*====================
	 Maximum name sizes
  ====================*/
/*===================== PrintCon name sizes ===============================*/
#define NWPS_JOB_NAME_SIZE             32    /* 31 bytes and a '\0'			*/ 
#define NWPS_BANNER_NAME_SIZE          12    /* 12 bytes and a '\0'			*/ 
#define NWPS_BANNER_FILE_SIZE          12    /* 12 bytes and a '\0'			*/ 
#define NWPS_HEADER_FILE_SIZE          12    /* 12 bytes and a '\0'			*/ 

/*===================== PrintDef name sizes ===============================*/
#define NWPS_FORM_NAME_SIZE				12    /* 12 bytes and a '\0'			*/ 
#define NWPS_DEVI_NAME_SIZE				32    /* 32 bytes and a '\0'			*/ 
#define NWPS_MODE_NAME_SIZE				32    /* 32 bytes and a '\0'			*/ 
#define NWPS_FUNC_NAME_SIZE				32    /* 32 bytes and a '\0'			*/ 

/*================== Print Server Configuration Sizes =====================*/
#define NWPS_DESCRIPT_SIZE             128	/* matches Bind. prop. value	*/
#define NWPS_APPLE_NAME_SIZE				32    /* 32 bytes and a '\0'			*/ 
#define NWPS_APPLE_TYPE_SIZE				32    /* 32 bytes and a '\0'			*/ 
#define NWPS_APPLE_ZONE_SIZE				32    /* 32 bytes and a '\0'			*/ 
#define NWPS_UNIX_HOST_SIZE				255	/* 255 bytes and a '\0'       */ 
#define NWPS_UNIX_PRNT_SIZE				255	/* 255 bytes and a '\0'       */ 
#define NWPS_OTHER_SIZE                1024	/* bytes for NWPS_P_OTHER		*/

/*===========================
	 Maximum number of objects
  ===========================*/
/*================================================
	If -1 is used for a Printer or Form number,
	the first available number will be substituted.
  ================================================*/
#define NWPS_MAX_PRINTERS					255	/* numbered 0 - 254				*/
#define NWPS_MAX_FORMS						255	/* numbered 0 - 254				*/

/*==== GetFirst, GetNext, and EndNext require the following handle =========*/
typedef void NWPTR NWPSListHandle;

#ifdef __cplusplus
extern "C" {
#endif

   #if defined(NWWIN) || defined(NWOS2) || defined(NWNLM)

void NWAPI NWPSGetLibraryVersion(
  BYTE NWFAR *majorVersion,
  BYTE NWFAR *minorVersion,
  BYTE NWFAR *revisionLevel,
  BYTE NWFAR *betaReleaseLevel);
   #endif
   #ifdef __cplusplus
}
   #endif
#endif /* NWPS_DEF_INC */
