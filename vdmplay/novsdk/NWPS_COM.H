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
//บ Source module name: NWPS_COM.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_DEF_INC
#include <nwps_def.h>
#endif

#ifndef NWPS_COM_INC
#define NWPS_COM_INC
/*********************************************************************/
/*********** COM - Pserver Transport Interface ***********************/
/*********************************************************************/

/*========= Client privilege levels from the Print Server =================*/
#define NWPS_LIMITED                   0	/* Limited access only				*/
#define NWPS_USER                      1	/* User access                   */
#define NWPS_OPERATOR						2	/* Operator access					*/

/*========================== Job Outcomes =================================*/
#define NWPS_PLACE_ON_HOLD             0	/* Place job on hold             */
#define NWPS_RETURN_TO_QUEUE				1	/* Return job to queue				*/
#define NWPS_THROW_AWAY                2	/* Throw job away                */

/*=================== Network Printer Status codes ========================*/
#define NWPS_PRINTER_RUNNING				0	/* Printer is running				*/
#define NWPS_PRINTER_OFFLINE				1	/* Printer is offline				*/
#define NWPS_PRINTER_PAPER_OUT			2	/* Printer is out of paper       */
#define NWPS_CONNECTION_LOST				4	/* NPrinter connection was lost	*/

/*===== Network Printer and Extended Network Printer Info flags ===========*/
#define NWPS_PRINTER_SHARED				0	/* NPrinter is shared with net	*/
#define NWPS_PRINTER_PRIVATE				1	/* NPrinter is private to ws		*/

/*=== Print Server info structure returned by NWPSComGetPrintServerInfo ===*/
typedef struct
{
  BYTE  status,								/* Print server status					*/
		  numPrinters,                   /* Number of attached printers		*/
		  numModes,                      /* Number of queue service modes    */
		  majorVersion,						/* Print server protocol, major vers*/
		  minorVersion,						/* Print server protocol, minor vers*/
		  revision,                      /* Print server protocol, revision	*/
		  serialNumber[4],					/* Serial number in BCD             */
		  serverType,							/* Print Server Type
														0 - Unknown            
														1 - Dedicate print server for DOS
														2 - NetWare Loadable Module	 
														3 - VAP, in file server  
														4 - VAP, in Bridge	
														5 - Unix print server				*/
        nameServiceMode;               /* NWPS_BINDERY_SERVICE or
                                          NWPS_DIRECTORY_SERVICE           */
  BYTE  futureUse[8];						/* Reserved for future use          */
} NWPS_PSInfo;

/*==============================================
  Network Printer information structure returned
  by NWPSComRequestNetworkPrinter()
  ==============================================*/
typedef struct
{
  WORD  printerType,                   /* Type of Network printer          */
		  useInterrupts,						/* Should we use interrupts?			*/
		  irqNumber,							/* IRQ number for printer				*/
		  numBlocks,							/* Number of blocks in buffer       */
		  useXonXoff,							/* Use Xon/Xoff?		 (Serial only) */
		  baudRate,                      /* Baud rate			 (Serial only) */
		  dataBits,                      /* Number of bits     (Serial only) */
		  stopBits,                      /* Number of bits     (Serial only) */
		  parity,								/* Parity type        (Serial only) */
		  socket;								/* Socket number for Network printer*/
} NWPS_NInfo;

/*===============================================
  Network Printer status structure used to report
  the current status to the print server.
  (Not used in any API calls)
  ===============================================*/
typedef struct
{
  BYTE  printerNumber,						/* Network printer number				*/
		  needBlocks,							/* Number of blocks needed to fill 
														buffers									*/
		  finishedBlocks,                /* Number of blocks printed			*/
		  status,								/* Printer online, offline, or 
														out-of-paper							*/
		  inSideBand;							/* True-NPrinter is in sideband mode*/
} NWPS_NStatus;

/*======================================================
  Data types sent by the PServer to a Network printer.
  The first byte of every print server to printer packet
  will have one of these codes in it.
  (Not used in any API calls)
  ======================================================*/
#define NWPS_DST_DATA						(BYTE) 0
#define NWPS_DST_FLUSH						(BYTE) 1
#define NWPS_DST_PAUSE						(BYTE) 2
#define NWPS_DST_START						(BYTE) 3
#define NWPS_DST_SIDEBAND					(BYTE) 4
#define NWPS_DST_NEW_JOB					(BYTE) 5
#define NWPS_DST_RELEASE					(BYTE) 6
#define NWPS_DST_RECLAIM					(BYTE) 7
#define NWPS_DST_EOJ                   (BYTE) 8

/*
// The following conditional includes pick up the varying definitions
// of NWPSComAttachToPrintServer and NWPSComLoginToPrintServer.  Since
// the majority of the NWPSComxxxxx calls are unaffected by the
// distinction between connections and their types the prototype were
// kept in this file.  This is an exception compared to the other
// major groups of supported calls.
*/
#if defined(NWPS_CONN_TYPE)
#	 if	NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE
#	  include <nwpscom0.h>
#	 elif NWPS_CONN_TYPE == NWPS_DIRECTORY_SERVICE
#	  include <nwpscom1.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_PRE_40
#	  include <nwpscom0.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_ALL
#	  include <nwpscom0.h>
#	 endif
#else
#	  include <nwpscomx.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*====================== Print Server Communications ======================*/
NWCCODE NWAPI NWPSComAbortPrintJob(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  BYTE  jobOutcome);                   /* Job outcome                      */

NWCCODE NWAPI NWPSComAddNotifyObject(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR objectName,					/* Object name                      */
  WORD  objectType,							/* Object type                      */
  WORD  notifyDelay,                   /* First notify delay					*/
  WORD  notifyInterval);					/* Notify interval						*/

NWCCODE NWAPI NWPSComAddQueueToPrinter(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR queueName,					/* Queue name								*/
  WORD  priority);							/* Priority                         */

NWCCODE NWAPI NWPSComAttachPServerToNServer(
  WORD  spxID,                         /* SPX Connection number				*/
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR password);					/* Password                         */

NWCCODE NWAPI NWPSComCancelDownRequest(
  WORD  spxID);								/* SPX Connection number				*/

NWCCODE NWAPI NWPSComChangeNotifyInterval(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR objectName,					/* Object name                      */
  WORD  objectType,							/* Object type                      */
  WORD  notifyDelay,                   /* First notify delay					*/
  WORD  notifyInterval);					/* Notify interval						*/

NWCCODE NWAPI NWPSComChangeQueuePriority(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR queueName,					/* Queue name								*/
  WORD  priority);							/* New priority							*/

NWCCODE NWAPI NWPSComChangeServiceMode(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerName,                   /* Printer number                   */
  BYTE  serviceMode);						/* New service mode						*/

NWCCODE NWAPI NWPSComDeleteNotifyObject(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR objectName,					/* Object name                      */
  WORD  objectType);                   /* Object type                      */

NWCCODE NWAPI NWPSComDeleteQueueFromPrinter(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR queueName,					/* Queue name								*/
  BYTE  detach,								/* Detach immediately?					*/
  BYTE  jobOutcome);                   /* Outcome of current job				*/

NWCCODE NWAPI NWPSComDetachFromPrintServer(
  WORD  spxID);								/* SPX Connection number				*/

NWCCODE NWAPI NWPSComDetachPServerFromNServer(
  WORD  spxID,                         /* SPX Connection number				*/
  char  NWPTR nServerName,             /* NetWare server name					*/
  BYTE  detach,								/* Detach immediately?					*/
  BYTE  jobOutcome);                   /* Outcome of current jobs          */

NWCCODE NWAPI NWPSComDownPrintServer(
  WORD  spxID,                         /* SPX Connection number				*/
  BYTE  immediate,							/* Go down immediately?             */
  BYTE  jobOutcome);                   /* Outcome of current jobs          */

NWCCODE NWAPI NWPSComEjectForm(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID);							/* Printer number                   */

NWCCODE NWAPI NWPSComGetAttachedNServers(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  NWPTR sequence,                /* Sequence number. 0 first time    */
  char  NWPTR nServerName);				/* NetWare server name					*/

NWCCODE NWAPI NWPSComGetNotifyObject(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  WORD  NWPTR sequence,                /* Sequence number.	0 first time	*/
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR objectName,					/* Object name                      */
  WORD  NWPTR objectType,					/* Object type                      */
  WORD  NWPTR notifyDelay,             /* First notify delay					*/
  WORD  NWPTR notifyInterval);			/* Notify interval						*/

NWCCODE NWAPI NWPSComGetPrintersServicingQ(
  WORD  spxID,                         /* SPX Connection number				*/
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR queueName,					/* Queue name								*/
  WORD  maxPrinters,                   /* Maximum # of returned printers	*/
  WORD  NWPTR actualPrinters,          /* Actual # of returned printers    */
  WORD  NWPTR printerArray);				/* Array for returned printer #s    */
  
NWCCODE NWAPI NWPSComGetPrinterStatus(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  BYTE  NWPTR status,						/* Printer status                   */  
  BYTE  NWPTR troubleCode,             /* On line/Off line/Out of paper    */  
  BYTE  NWPTR active,						/* Printer has an active job			*/  
  BYTE  NWPTR serviceMode,             /* Queue service mode					*/
  WORD  NWPTR formNumber,					/* Mounted form number					*/
  char  NWPTR formName,                /* Mounted form name                */
  char  NWPTR printerName);				/* Printer name							*/

NWCCODE NWAPI NWPSComGetExtPrinterStatus(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR primaryStatus,				/* Primary status (60max)				*/
  BYTE  NWPTR primaryLevel,				/* Primary error level					*/
  char  NWPTR secondaryStatus,			/* Secondary status (60max)			*/
  BYTE  NWPTR secondaryLevel,          /* Secondary error level				*/
  BYTE  NWPTR activeJobCount,          /* Number of active jobs				*/
  BYTE  NWPTR serviceMode,             /* Queue service mode					*/
  WORD  NWPTR formsMounted,				/* Number of forms mounted          */
  WORD  NWPTR formList);					/* Array of mounted forms (20max)	*/


NWCCODE NWAPI NWPSComGetPrintJobStatus(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  NWPTR nServerName,             /* NetWare server name					*/  
  char  NWPTR queueName,					/* Queue name								*/  
  DWORD NWPTR jobID,                   /* Job number								*/
  char  NWPTR jobDescription,          /* Description of job					*/
  WORD  NWPTR copies,						/* Number of copies to be printed	*/
  DWORD NWPTR printJobSize,				/* Size of print job                */
  WORD  NWPTR copiesDone,					/* Copies finished						*/
  DWORD NWPTR bytesDone,					/* Bytes into current copy          */
  WORD  NWPTR formNumber,					/* Form number for job					*/
  BYTE  NWPTR textFlag);					/* Is job text?							*/

NWCCODE NWAPI NWPSComGetPrintServerInfo(
  WORD  spxID,                         /* SPX Connection number				*/
  NWPS_PSInfo NWPTR psInfo,				/* Server info structure				*/
  WORD  size);                         /* Size of information requested    */

NWCCODE NWAPI NWPSComGetQueuesServiced(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  WORD  NWPTR sequence,                /* Sequence number.	0 first time	*/
  char  NWPTR nServerName,             /* NetWare server name					*/
  char  NWPTR queueName,					/* Queue name								*/
  WORD  NWPTR priority);					/* Priority                         */

NWCCODE NWAPI NWPSComMarkTopOfForm(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  char  mark);                         /* Character to mark form with		*/

NWCCODE NWAPI NWPSComPrintServerRequest(
  WORD  spxID,                         /* SPX Connection number				*/
  void  NWPTR reqBuffer,					/* address of request packet buffer */
  int   reqSize,								/* length of request packet			*/
  void  NWPTR repBuffer,					/* address of reply packet buffer	*/
  int   repSize);                      /* length of expected reply			*/

NWCCODE NWAPI NWPSComRewindPrintJob(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  BYTE  byPage,								/* Rewind by page?						*/
  BYTE  relative,                      /* Rewind relative to cur. position?*/
  WORD  copy,									/* Copy to rewind to (if absolute)	*/
  DWORD offset);								/* Offset									*/

NWCCODE NWAPI NWPSComSetMountedForm(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  WORD  formNumber);                   /* Form number                      */

NWCCODE NWAPI NWPSComStartPrinter(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID);							/* Printer number                   */

NWCCODE NWAPI NWPSComStopPrinter(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  BYTE  jobOutcome);                   /* Job outcome                      */

/*=================== Network Printer Calls ===============================*/
NWCCODE NWAPI NWPSComGetNextRemotePrinter(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  NWPTR printerID,					/* Printer number                   */
  WORD  NWPTR printerType,             /* Printer type							*/
  char  NWPTR printerName);				/* Name of printer						*/

NWCCODE NWAPI NWPSComRequestRemotePrinter(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  NWPS_NInfo NWPTR info);					/* Network printer info structure	*/

NWCCODE NWAPI NWPSComSetRemoteMode(
  WORD  spxID,                         /* SPX Connection number				*/
  WORD  printerID,							/* Printer number                   */
  BYTE  newMode);                      /* New mode                         */

#ifdef __cplusplus
}
#endif

#endif  /* NWPS_COM_INC */

