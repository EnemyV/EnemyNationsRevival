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
//บ Source module name: NWPS_PDF.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_DEF_INC
#include <nwps_def.h>
#endif

#ifndef NWPS_PDF_INC
#define NWPS_PDF_INC
/*********************************************************************/
/*********** PrintDef - Printer Definition Information ***************/
/*********************************************************************/

/*========== PrintDef reset string (one for each device) ============*/
#define NWPS_RESET_MODE                "(Re-initialize)"
#define NWPS_MAX_FUNC_SIZE             256

typedef struct NWPSFuncData_T {
   int  funcSize;
   BYTE funcData[NWPS_MAX_FUNC_SIZE];
} NWPSPdfFuncData;

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================
  Turn on the debug printf messages in the PrintDef program.
  These messages are in English only and are normally not displayed.
  ==================================================================*/
void NWAPI NWPSPdfDebug(
  BYTE  flag);                         /* 0-turn off; 1-turn on				*/

/*=======================================
  Set/Get the date code on an Import File
  =======================================*/
NWCCODE NWAPI NWPSPdfSetImportDate(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR fileName,                /* name of .PDF file                */
  WORD  year,									/* Year to set date to					*/
  WORD  month,                         /* Month to set date to             */
  WORD  day,									/* Day to set date to					*/
  WORD  hour,									/* Hour to set date to					*/
  WORD  minute,								/* Minute to set date to				*/
  WORD  second);								/* Second to set date to				*/

NWCCODE NWAPI NWPSPdfGetImportDate(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR fileName,                /* name of .PDF file                */
  WORD  NWPTR year,							/* Year the file was created			*/
  WORD  NWPTR month,                   /* Month the file was created       */
  WORD  NWPTR day,							/* Day the file was created			*/
  WORD  NWPTR hour,							/* Hour the file was created			*/
  WORD  NWPTR minute,						/* Minute the file was created		*/
  WORD  NWPTR second);						/* Second the file was created		*/

/*====================================================
// This typedef is for the Managed Import Device calls
// 
// The importManagerParm1 is intended to give context
// to the caller's Import Manager. The other two
// parameters are for reporting progress, e.g. percent
// complete could be calculated: 
//   percentDone = 100 * currentCallCount / totalCallsToBeMade;
// 
// The return code, if non-zero causes the Import to
// abort and the Device remnants deleted. This could be
// useful in implementing a Cancel feature, checking a
// hotkey or mouse button etc. for interruptions.
// 
// The exception to the return code is that when
// totalCallsToBeMade and currentCallCount are equal
// the return code will be ignored. This is the caller's
// opportunity to do screen cleanup etc.
  ====================================================*/
typedef NWCCODE (NWAPI *NWPSImportManager)(
	void NWFAR *importManagerParm1,		/* user specific data					*/
	int  totalCallsToBeMade,				/* max calls to be made             */
	int  currentCallCount);             /* number of this function call		*/

/*=========================================================
  This entry point concludes any NWPSPdfGetFirstxxxxxxx or
  NWPSPdfGetNextxxxxxx calls.
  =========================================================*/
NWCCODE NWAPI NWPSPdfEndNext(
  NWPSListHandle handle);              /* Handle for GetFirst and GetNext  */

#ifdef __cplusplus
}
#endif

#if defined(NWPS_CONN_TYPE)
#	 if	NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE
#	  include <nwpspdf0.h>
#	 elif NWPS_CONN_TYPE == NWPS_DIRECTORY_SERVICE
#	  include <nwpspdf1.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_PRE_40
#	  include <nwpspdf2.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_ALL
#	  include <nwpspdfx.h>
#	 endif
#else
#	  include <nwpspdfx.h>
#endif

#endif  /* NWPS_PDF_INC */
