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
//บ Source module name: NWPS_JOB.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_DEF_INC
#include <nwps_def.h>
#endif

#ifndef NWPS_JOB_INC
#define NWPS_JOB_INC
/*********************************************************************/
/*********** PrintCon - Print Job Configuration Information **********/
/*********************************************************************/

/*====================== PrintCon search flags =============================*/
#define NWPS_EXTENDED_SEARCH				0
#define NWPS_SINGLE_SEARCH             1
#define NWPS_LIMITED_SEARCH				2

/*========================================================================
	Either or both of the following bits may be returned with a given Job
	recordName/recordOwner pair. The first indicates the Job which would
	be returned by NWPSJobGetDefault. The second indicates the Job recorded
	in the database header as a default which is in that database.
  ========================================================================*/
#define NWPS_DOMINANT_DEFAULT_JOB		0x01 /* returned once per search	  */
#define NWPS_CURRENT_DB_DEFAULT_JOB    0x02 /* returned once per database */ 
															
#define NWPS_DBOWNER_PUBLIC				"(PUBLIC)"
#define NWPS_DEVICE_NONE					"(NONE)"
#define NWPS_MODE_NONE						"(NONE)"

/*====================================
	Flags used for printControlFlags in
	the NWPS_ClientRecord structure
  ====================================*/
#define NWPS_SUPPRESS_FF					0x0800
#define NWPS_NOTIFY_USER					0x1000
#define NWPS_TEXT_MODE						0x4000
#define NWPS_PRINT_BANNER					0x8000

/*========================================================
  This structure is overlayed on the QMS
  NWQueueJobStruct.clientRecordArea to define a print job.
  It is not used in any of the print services APIs.
  (Formerly called NWPS_PJob.)
  ========================================================*/
typedef struct {
  BYTE						versionNumber;
  BYTE						tabSize;
  WORD						numberOfCopies;
  WORD						printControlFlags;
  WORD						maxLinesPerPage;
  WORD						maxCharsPerLine;
  BYTE						formName[13];
  BYTE						reserve[9];
  BYTE						bannerNameField[13];
  BYTE						bannerFileField[13];
  BYTE						headerFileName[14];
  BYTE						directoryPath[80];
} NWPS_ClientRecord;

/*=======================================================
  NWPS_Job_Rec is the type of record in the third and
  last section of the PrnConDB database.	Each one of
  these records contains all the fields that make up a
  print job configuration as described in the NetWare 386
  Print Server documentation.
  =======================================================*/
/*======== Flags for the NWPS_Job_Rec structure printJobFlag field =========*/
#define NWPS_JOB_EXPAND_TABS				0x00000001	/*File type:0=Stream 1=Tab*/
#define NWPS_JOB_NO_FORMFEED				0x00000002	/*Formfeed tail:0=Yes 1=No*/
#define NWPS_JOB_NOTIFY                0x00000004	/*Notify:0=No 1=Yes		*/
#define NWPS_JOB_PRINT_BANNER          0x00000008	/*Banner:0=No 1=Yes		*/
#define NWPS_JOB_AUTO_END					0x00000010	/*Auto endcap:0=No 1=Yes*/
#define NWPS_JOB_TIMEOUT					0x00000020	/*Enable T.O.:0=No 1=Yes*/

#define NWPS_JOB_ENV_DS                0x00000040	/*Use D.S. Environment	*/
#define NWPS_JOB_ENV_MASK					0x000001C0	/*Bindery vs. D.S. Mask */

#define NWPS_JOB_DS_PRINTER				0x00000200	/*D.S. Printer not Queue*/
#define NWPS_JOB_PRINTER_MASK          0x00000E00	/*D.S. Printer vs. Queue*/

/*=========================== Default Flags ================================*/
#define NWPS_JOB_DEFAULT			  (NWPS_JOB_PRINT_BANNER | NWPS_JOB_AUTO_END)
#define NWPS_JOB_DEFAULT_COPIES			1				/*Default Number of Copies*/
#define NWPS_JOB_DEFAULT_TAB				8				/*Default Tab Expansion */

typedef struct {
  DWORD printJobFlag;						/* Bits 31 30 29 ... 2 1 0 contain:  */
													/* 0: File type: 0=Byte stream 1=Text*/
													/* 1: Suppress formfeed:  0=No 1=Yes */
													/* 2: Notify when done:   0=No 1=Yes */
													/* 3: Print banner:		  0=No 1=Yes */
													/* 4: Auto endcap:		  0=No 1=Yes */
													/* 5: Enable timeout:	  0=No 1=Yes */
													/* 8-6: Environment:                 */
													/*     000=Bindary						 */
													/*     001=Directory Services        */
													/* 11-9: Destination Type:           */
													/*     000=Queue_Name					 */
													/*     001=Printer_Name              */
													/* 31-12: Unused							 */
  WORD  copies;								/* 1 - 65,000								 */
  WORD  timeOutCount;						/* 1 - 1,000								 */
  BYTE  tabSize;								/* 1 - 18									 */
  BYTE  localPrinter;						/* 0=Lpt1, 1=Lpt2, 2=Lpt3 etc.		 */
  char  formName[ NWPS_FORM_NAME_SIZE + 2 ];     /* 1-12 chars              */
  char  name[ NWPS_BANNER_NAME_SIZE + 2 ];		 /* 1-12 chars              */
  char  bannerName[ NWPS_BANNER_FILE_SIZE + 2 ]; /* 1-12 chars              */
  char  device[ NWPS_DEVI_NAME_SIZE + 2 ];		 /* 1-32 chars              */
  char  mode[ NWPS_MODE_NAME_SIZE + 2 ];			 /* 1-32 chars              */
  union {
	 struct {
		/* pad structures on even boundries */
		char	nServer[ NWPS_BIND_NAME_SIZE + 2 ];     /* 2-48 chars           */
		char	printQueue[ NWPS_BIND_NAME_SIZE + 2 ];  /* 1-48 chars           */
		char	printServer[ NWPS_BIND_NAME_SIZE + 2 ]; /* 1-48 chars           */
	 } nonDS;
	 char    DSObjectName[ NWPS_MAX_NAME_SIZE ];    
  } u;
  BYTE  reserved[390];						/* Adds up to 1024 total (was 1026)	 */
} NWPS_Job_Rec;


/*===========================================================
	DBOwner values and their meanings-
	---------------------------------
	NULL		 : Use the current user. No return possible.
	Empty     : Use the current user. Return real object name.
	UserName  : Use the specified user.
	DS Object : Use the specified DS object.
  ===========================================================*/

#if defined(NWPS_CONN_TYPE)
#	 if	NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE
#	  include <nwpsjob0.h>
#	 elif NWPS_CONN_TYPE == NWPS_DIRECTORY_SERVICE
#	  include <nwpsjob1.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_PRE_40
#	  include <nwpsjob2.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_ALL
#	  include <nwpsjobx.h>
#	 endif
#else
#	  include <nwpsjobx.h>
#endif

#endif  /* NWPS_JOB_INC */

