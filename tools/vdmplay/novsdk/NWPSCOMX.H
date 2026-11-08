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
//บ Source module name: NWPSCOMX.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifdef __cplusplus
extern "C" {
#endif

/*====================== Print Server Communications ======================*/
/*---------------------------------------------------------------
  The connType and connID fields are used to determine the context
  in which to run this library.	The connType and connID used by
  NWPSComLoginToPrintServer are used only to acquire rights.
  ---------------------------------------------------------------*/
NWCCODE NWAPI NWPSComAttachToPrintServer(
  WORD  connType,                      /* Connection type to use				*/
  DWORD connID,								/* Connection ID to file server		*/
  WORD  timeOut,								/* Number of seconds before timeout */
  char  NWPTR pServerName,             /* Print server name                */
  WORD  NWPTR spxID);						/* SPX Connection number				*/

/*---------------------------------------------------------------
  The connType and connID fields are used only to acquire rights.
  The connType and connID used by NWPSComAttachToPrintServer are
  used to determine the context in which to run this library.
  ---------------------------------------------------------------*/
NWCCODE NWAPI NWPSComLoginToPrintServer(
  WORD  connType,                      /* Connection type to use				*/
  DWORD connID,								/* Connection ID to file server		*/
  WORD  spxID,                         /* SPX Connection number				*/
  BYTE  NWPTR accessLevel);				/* Client's access level				*/


#ifdef __cplusplus
}
#endif
