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
//บ Source module name: NWPS_PKT.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_COM_INC
#include <nwps_com.h>
#endif
#ifndef NWPS_PKT_INC
#define NWPS_PKT_INC
/**************************************************************************/
/************ Print Server Communication Packet Structures ****************/
/**************************************************************************/
/* These structures are organized in pairs for Request/Reply. They are	  */
/* Also grouped logically by related operations to be performed			  */
/**************************************************************************/
#ifndef CMD_LOGIN_TO_PRINT_SERVER
/* General Commands */
#define CMD_LOGIN_TO_PRINT_SERVER		0x01
#define CMD_GET_PRINT_SERVER_INFO		0x02
#define CMD_DOWN								0x03
#define CMD_CANCEL_DOWN                0x04
#define CMD_GET_PRINTER_STATUS			0x05
#define CMD_STOP_PRINTER					0x06
#define CMD_START_PRINTER					0x07
#define CMD_MOUNT_FORM						0x08
#define CMD_REWIND_PRINT_JOB				0x09
#define CMD_EJECT_PAGE						0x0A
#define CMD_MARK_PAGE						0x0B
#define CMD_CHANGE_SERVICE_MODE			0x0C
#define CMD_GET_JOB_STATUS             0x0D
#define CMD_ABORT_JOB						0x0E
#define CMD_SCAN_QUEUE_LIST				0x0F
#define CMD_CHANGE_QUEUE_PRIORITY		0x10
#define CMD_ADD_QUEUE						0x11
#define CMD_DELETE_QUEUE					0x12
#define CMD_GET_PRINTERS_FOR_QUEUE		0x13
#define CMD_SCAN_NOTIFY_LIST				0x14
#define CMD_CHANGE_NOTIFY					0x15
#define CMD_ADD_NOTIFY						0x16
#define CMD_DELETE_NOTIFY					0x17
#define CMD_ATTACH_TO_FILE_SERVER		0x18
#define CMD_DETACH_FROM_FILE_SERVER    0x19
#define CMD_GET_ATTACHED_SERVERS       0x1A

/* NPrinter commands */
#define CMD_GET_RPRINTER					0x80
#define CMD_CONNECT_RPRINTER				0x81
#define CMD_SET_REMOTE_MODE				0x82
#define CMD_SCAN_RPRINTER					0x84

/* New PServer commands for version 4.0+ */
#define CMD_ADD_FORM                   0x1D
#define CMD_DISMOUNT_FORM					0x1E
#define CMD_GET_EXT_PRINTER_STATUS		0x1F

/* Unicode Directory Service commands (version 4.1) */
#define CMD_UDS_LOGIN_TO_PRINT_SERVER	0xA1
#define CMD_UDS_SCAN_QUEUE_LIST			0xAF
#define CMD_UDS_CHANGE_QUEUE_PRIORITY	0xB0
#define CMD_UDS_ADD_QUEUE					0xB1
#define CMD_UDS_DELETE_QUEUE				0xB2
#define CMD_UDS_GET_PRINTERS_FOR_QUEUE	0xB3
#define CMD_UDS_SCAN_NOTIFY_LIST			0xB4
#define CMD_UDS_CHANGE_NOTIFY				0xB5
#define CMD_UDS_ADD_NOTIFY					0xB6
#define CMD_UDS_DELETE_NOTIFY				0xB7
#define CMD_UDS_GET_JOB_ID					0xB8

/* Directory Service commands (version 4.0) */
#define CMD_DS_LOGIN_TO_PRINT_SERVER	0xD1
#define CMD_DS_SCAN_QUEUE_LIST			0xDF
#define CMD_DS_CHANGE_QUEUE_PRIORITY	0xE0
#define CMD_DS_ADD_QUEUE					0xE1
#define CMD_DS_DELETE_QUEUE				0xE2
#define CMD_DS_GET_PRINTERS_FOR_QUEUE	0xE3
#define CMD_DS_SCAN_NOTIFY_LIST			0xE4
#define CMD_DS_CHANGE_NOTIFY				0xE5
#define CMD_DS_ADD_NOTIFY					0xE6
#define CMD_DS_DELETE_NOTIFY				0xE7
#define CMD_DS_GET_JOB_ID					0xE8
#endif

/****************************************************************************/
/*============ Print Server Request/Reply Packet Structures ================*/
/****************************************************************************/
/*===== Generic Request and Reply Structures ===============================
  These are used by many of the commands because all that is needed is the
  Command BYTE or the ReturnCode WORD. Commented copies are placed 
  strategically in order to fully document both the request and reply for
  each of the commands.
  ==========================================================================*/
typedef struct NWPS_ReqstPkt_Other_T {
	BYTE	command;                      
} NWPS_ReqstPacket_Other;			 

typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other;

/*===== CMD_LOGIN_TO_PRINT_SERVER ==========================================*/		
typedef struct NWPS_ReqstPkt_LoginToPSrvr_T {
	BYTE	command;
	BYTE	nServerName[48];					/* User's primary bindery server   */
	WORD	userConnectionNumber;			/* Connection number on the server */
} NWPS_ReqstPacket_LoginToPSrvr;

typedef struct NWPS_ReplyPkt_LoginToPSrvr_T {
	WORD	returnCode;
	BYTE	accessLevel;
} NWPS_ReplyPacket_LoginToPSrvr;

/*===== CMD_GET_PRINT_SERVER_INFO ==========================================*/		
/* typedef struct NWPS_ReqstPkt_Other_T {
	BYTE	command;                      
} NWPS_ReqstPacket_Other; */

typedef struct NWPS_ReplyPkt_GetPSrvrInfo_T {
	WORD	returnCode;
	NWPS_PSInfo info;                   /* defined in NWPS_COM.H				 */
} NWPS_ReplyPacket_GetPSrvrInfo;

/*===== CMD_DOWN ===========================================================*/								
typedef struct NWPS_ReqstPkt_DownPServer_T {
	BYTE	command;
	BYTE	doDownNow;							/* Go down immediately? TRUE/FALSE	 */
	BYTE	jobOutcome;                   /* What to do with current jobs.     */
} NWPS_ReqstPacket_DownPServer;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_CANCEL_DOWN ====================================================*/					
/* typedef struct NWPS_ReqstPkt_Other_T {
	BYTE	command;                      
} NWPS_ReqstPacket_Other; */			

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_GET_PRINTER_STATUS =============================================*/			
typedef struct NWPS_ReqstPkt_GetPrtStatus_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
} NWPS_ReqstPacket_GetPrtStatus;

typedef struct NWPS_ReplyPkt_GetPrtStatus_T {
	WORD	returnCode;
	BYTE	nPrinterStatus;					/* NWPS_PSTAT_xxxxxxx					 */
	BYTE	deviceStatusCode;             /* Online/Offline  Valid only if
														nPrinterStatus field is either
														NWPS_PSTAT_PRINTING or
														NWPS_PSTAT_MARK_EJECT.				 */
	BYTE	activeJob;							/* non-zero => active; zero => idle  */
	BYTE	queueServiceMode;             /* 0 - Change forms as needed   
														1 - Minimize changes within queues
														2 - Never change forms
														3 - Minimize changes across queues*/
	WORD	mountedForm;						/* form number of mounted form		 */
	BYTE	formName[16];						/* name of mounted form              */
	BYTE	printerName[48];					/* name of printer						 */
} NWPS_ReplyPacket_GetPrtStatus; 

/*===== CMD_STOP_PRINTER ===================================================*/					
typedef struct NWPS_ReqstPkt_StopPrinter_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
   BYTE  jobOutcome;
} NWPS_ReqstPacket_StopPrinter;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_START_PRINTER ==================================================*/					
typedef struct NWPS_ReqstPkt_StartPrinter_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
} NWPS_ReqstPacket_StartPrinter;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_MOUNT_FORM =====================================================*/						
typedef struct NWPS_ReqstPkt_MountForm_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	formNumber;
} NWPS_ReqstPacket_MountForm;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_REWIND_PRINT_JOB ===============================================*/				
typedef struct NWPS_ReqstPkt_RewindPJob_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	byPage;								/* 0 => by Byte; 1 => byPage			 */
	BYTE	relative;							/* 0 => absolute; 1 => relative		 */
   WORD  copyNumber;                   /* used if absolute                  */
	long	offset;								/* signed offset based on the
														'byPage' and 'relative' fields	 */
} NWPS_ReqstPacket_RewindPJob;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_EJECT_PAGE =====================================================*/						
typedef struct NWPS_ReqstPkt_EjectPage_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
} NWPS_ReqstPacket_EjectPage;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_MARK_PAGE ======================================================*/						
typedef struct NWPS_ReqstPkt_MarkPage_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	markingCharToUse;             
} NWPS_ReqstPacket_MarkPage;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_CHANGE_SERVICE_MODE ============================================*/			
typedef struct NWPS_ReqstPkt_ChgServiceMode_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	newServiceMode;					/* Form Change Behavior  (0-3)		 */
} NWPS_ReqstPacket_ChgServiceMode;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_GET_JOB_STATUS =================================================*/				
typedef struct NWPS_ReqstPkt_GetPJobStatus_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
} NWPS_ReqstPacket_GetPJobStatus;

typedef struct NWPS_ReplyPkt_GetPJobStatus_T {
	WORD	returnCode;
	BYTE	nServerName[48];
	BYTE	queueName[48];
	WORD	queueJobNumber;
	BYTE	jobDescription[50];
	WORD	copiesRequested;
	DWORD copySize;
	WORD	copiesCompleted;
	DWORD bytesIntoCurrentCopy;
	WORD	formNumber;
	BYTE	expandTabs;                   /* non-zero => expand tabs;
														zero => byte stream					 */
} NWPS_ReplyPacket_GetPJobStatus; 

/*===== CMD_ABORT_JOB ======================================================*/						
typedef struct NWPS_ReqstPkt_AbortPrintJob_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	jobOutcome;                   /* What to do with current jobs.     */
} NWPS_ReqstPacket_AbortPrintJob;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_SCAN_QUEUE_LIST ================================================*/				
typedef struct NWPS_ReqstPkt_GetQServiced_T {
	BYTE	command;
	BYTE	printer;                      /* 0 to maxPrinters-1					 */
	WORD	sequence;							/* initially zero                    */
} NWPS_ReqstPacket_GetQServiced;

typedef struct NWPS_ReplyPkt_GetQServiced_T {
	WORD	returnCode;
	WORD	newSequence;
	BYTE	nServerName[48];
	BYTE	queueName[48];
	BYTE	priority;
} NWPS_ReplyPacket_GetQServiced;

/*===== CMD_CHANGE_QUEUE_PRIORITY ==========================================*/		
typedef struct NWPS_ReqstPkt_ChgQPriority_T {
	BYTE	command;
	BYTE	printer;
	BYTE	nServerName[48];
	BYTE	queueName[48];
	BYTE	priority;
} NWPS_ReqstPacket_ChgQPriority;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_ADD_QUEUE ======================================================*/						
typedef struct NWPS_ReqstPkt_AddQToPrtr_T {
	BYTE	command;
	BYTE	printer;
	BYTE	nServerName[48];
	BYTE	queueName[48];
	BYTE	priority;
} NWPS_ReqstPacket_AddQToPrtr;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DELETE_QUEUE ===================================================*/					
typedef struct NWPS_ReqstPkt_DelQFromPrtr_T {
	BYTE	command;
	BYTE	printer;
	BYTE	now;									/* "immediately" (TRUE) or
														"after current job (FALSE)       */
	BYTE	jobOutcome;                   /* discard job or return to queue	*/
	BYTE	nServerName[48];
	BYTE	queueName[48];
} NWPS_ReqstPacket_DelQFromPrtr;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_GET_PRINTERS_FOR_QUEUE =========================================*/		
typedef struct NWPS_ReqstPkt_GetPtrsForQ_T {
	BYTE	command;
	BYTE	nServerName[48];
	BYTE	queueName[48];
	BYTE	maxPrinters;
} NWPS_ReqstPacket_GetPtrsForQ;

typedef struct NWPS_ReplyPkt_GetPtrsForQ_T {
	WORD	returnCode;
	BYTE	numPrinters;						/* number of printers servicing 
														the print queue						*/
	BYTE	printers [255];					/* Buffer for returned printer 
														numbers 0-254 valid					*/
} NWPS_ReplyPacket_GetPtrsForQ;

/*===== CMD_SCAN_NOTIFY_LIST ===============================================*/				
typedef struct NWPS_ReqstPkt_GetNotifyObj_T {
	BYTE	command;
	BYTE	printer;
	WORD	sequenceNumber;
} NWPS_ReqstPacket_GetNotifyObj;

typedef struct NWPS_ReplyPkt_GetNotifyObj_T {
	WORD	returnCode;
	WORD	sequence;							/* new sequence number					*/
	BYTE	nServerName[48];					/* Bindery Server name					*/
	BYTE	objectName[48];					/* name of object to be notified    */
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	WORD	initial;                      /* Time to first notice             */
	WORD	repeat;								/* Time between subsequent notices	*/
} NWPS_ReplyPacket_GetNotifyObj;

/*===== CMD_CHANGE_NOTIFY ==================================================*/					
typedef struct NWPS_ReqstPkt_ChgNotifyDel_T {
	BYTE	command;
	BYTE	printer;
	BYTE	nServerName[48];					/* Bindery Server name					*/
	BYTE	objectName[48];					/* name of object to be notified    */
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	WORD	initial;                      /* Time to first notice             */
	WORD	repeat;								/* Time between subsequent notices	*/
} NWPS_ReqstPacket_ChgNotifyDel;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_ADD_NOTIFY =====================================================*/						
typedef struct NWPS_ReqstPkt_AddNotifyObj_T {
	BYTE	command;
	BYTE	printer;
	BYTE	nServerName[48];					/* Bindery Server name					*/
	BYTE	objectName[48];					/* name of object to be notified    */
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	WORD	initial;                      /* Time to first notice             */
	WORD	repeat;								/* Time between subsequent notices	*/
} NWPS_ReqstPacket_AddNotifyObj;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DELETE_NOTIFY ==================================================*/					
typedef struct NWPS_ReqstPkt_DelNotifyObj_T {
	BYTE	command;
	BYTE	printer;
	BYTE	nServerName[48];					/* Bindery Server name					*/
	BYTE	objectName[48];					/* name of object to be notified    */
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
} NWPS_ReqstPacket_DelNotifyObj;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_ATTACH_TO_FILE_SERVER ==========================================*/		
typedef struct NWPS_ReqstPkt_AttachPSrvNSrv_T {
	BYTE	command;
	BYTE	nServerName[48];					/* Bindery Server name					*/
	BYTE	password[128];                /* password for PServer object		*/
} NWPS_ReqstPacket_AttachPSrvNSrv;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DETACH_FROM_FILE_SERVER ========================================*/	
typedef struct NWPS_ReqstPkt_DetachPServer_T {
	BYTE	command;
	BYTE	immediately;						/* detach immediately? TRUE/FALSE	*/
	BYTE	jobOutcome;                   /* What to do with current jobs.     */
	BYTE	nServerName[48];					/* Bindery Server name					*/
} NWPS_ReqstPacket_DetachPServer;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_GET_ATTACHED_SERVERS ===========================================*/		
typedef struct NWPS_ReqstPkt_GetAttachdSrvr_T {
	BYTE	command;
	BYTE	sequenceNumber;					/* initial value: zero		 */
} NWPS_ReqstPacket_GetAttachdSrvr;

typedef struct NWPS_ReplyPkt_GetAttachdSrvr_T {
	WORD	returnCode;
	BYTE	sequenceNumber;					/* updated sequence number */
	BYTE	nServerName[48];					/* Bindery Object Name 2-47 chars */
} NWPS_ReplyPacket_GetAttachdSrvr;

/*===== CMD_GET_RPRINTER ===================================================*/					
typedef struct NWPS_ReqstPkt_GetNPrinter_T {
	BYTE	command;
	BYTE	lastPrinterNumber;				/* initially 0xFF                    */
} NWPS_ReqstPacket_GetNPrinter;

typedef struct NWPS_ReplyPkt_GetNPrinter_T {
	WORD	returnCode;
	BYTE	printerNumber;                /* 0 - 254									 */
	WORD	printerType;						/* NWPS_P_REM_xxxxx						 */
	BYTE	printerName[48];					/* printer name							 */
} NWPS_ReplyPacket_GetNPrinter; 

/*===== CMD_CONNECT_RPRINTER ===============================================*/				
typedef struct NWPS_ReqstPkt_ReqstNPrinter_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 - 254									 */
} NWPS_ReqstPacket_ReqstNPrinter;

typedef struct NWPS_ReplyPkt_ReqstNPrinter_T {
	WORD	returnCode;
	NWPS_NInfo info;							/* defined in NWPS_COM.H				 */
} NWPS_ReplyPacket_ReqstNPrinter; 

/*===== CMD_SET_REMOTE_MODE ================================================*/				
typedef struct NWPS_ReqstPkt_SetNPrintrMode_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 - 254									 */
	BYTE	mode;                         /* NWPS_PRINTER_SHARED or
														NWPS_PRINTER_PRIVATE              */
} NWPS_ReqstPacket_SetNPrintrMode;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*******************************************************
 ** The packets below were added with the 4.0 PServer **
 *******************************************************/
/*===== CMD_SCAN_RPRINTER ===================================================*/					
typedef struct NWPS_ReqstPkt_ScanNPrinter_T {
	BYTE	command;
	BYTE	lastPrinterNumber;				/* initially 0xFF                    */
} NWPS_ReqstPacket_ScanNPrinter;

typedef struct NWPS_ReplyPkt_ScanNPrinter_T {
	WORD	returnCode;
	BYTE	printerNumber;                /* 0 - 254									 */
	WORD	printerType;						/* NWPS_P_REM_xxxxx						 */
	BYTE	printerName[48];					/* printer name							 */
	BYTE	extendedConfigLength;			/* length of 3rd party config data	 */
	BYTE	extendedConfig[100];          /* 3rd party config data				 */
} NWPS_ReplyPacket_ScanNPrinter; 

/*===== CMD_ADD_FORM =======================================================*/						
typedef struct NWPS_ReqstPkt_AddForm_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	formNumber;                   /* mount this additional form        */
} NWPS_ReqstPacket_AddForm;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DISMOUNT_FORM ==================================================*/					
typedef struct NWPS_ReqstPkt_DismountForm_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
	BYTE	formNumber;                   /* dismount this form					 */
} NWPS_ReqstPacket_DisMountForm;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;                   
} NWPS_ReplyPacket_Other; */

/*===== CMD_GET_EXT_PRINTER_STATUS =========================================*/		
typedef struct NWPS_ReqstPkt_GetExPrtStatus_T {
	BYTE	command;
	BYTE	printerNumber;                /* 0 to maxPrinters-1					 */
} NWPS_ReqstPacket_GetExPrtStatus;

typedef struct NWPS_ReplyPkt_GetExPrtStatus_T {
	WORD	returnCode;
	char	primaryPrinterStatus[60];		/* null terminated ASCII string		 */
	BYTE	errorStatus1;						/* Is primaryPrinterStatus an error? 
														TRUE/FALSE								 */
	char	secondaryPrinterStatus[60];	/* null terminated ASCII string		 */
	BYTE	errorStatus2;						/* Is secondaryPrinterStatus an error? 
														TRUE/FALSE								 */
	BYTE	activeJobCount;					/* how many active jobs there are	 */
	BYTE	queueServiceMode;             /* 0 - Change forms as needed   
														1 - Minimize changes within queues
														2 - Never change forms
														3 - Minimize changes across queues*/
	WORD	mountedFormCount;             /* number of mounted forms (20 max.) */
	WORD	mountedFormNumber[20];			/* array of mounted form numbers     */
} NWPS_ReplyPacket_GetExPrtStatus; 

/*===== CMD_DS_LOGIN_TO_PRINT_SERVER =======================================*/	
typedef struct NWPS_ReqstPkt_DSLoginToPSrvr_T {
	BYTE	command;
	BYTE	userName[512];                /* Fully qualified CN= name			 */
} NWPS_ReqstPacket_DSLoginToPSrvr;

typedef struct NWPS_ReplyPkt_DSLoginToPSrvr_T {
	WORD	returnCode;
	BYTE	accessLevel;						/* NWPS_LIMITED
														NWPS_USER
														NWPS_OPERATOR							 */
} NWPS_ReplyPacket_DSLoginToPSrvr;

/*===== CMD_DS_SCAN_QUEUE_LIST =============================================*/
typedef struct NWPS_ReqstPkt_DSGetQServiced_T {
	BYTE	command;
	BYTE	printer;
	WORD	sequence;
} NWPS_ReqstPacket_DSGetQServiced;

typedef struct NWPS_ReplyPkt_DSGetQServiced_T {
	WORD	returnCode;
	WORD	newSequence;
	BYTE	priority;
	BYTE	queue[512];
} NWPS_ReplyPacket_DSGetQServiced;

/*===== CMD_DS_CHANGE_QUEUE_PRIORITY =======================================*/	
typedef struct NWPS_ReqstPkt_DSChgQPriority_T {
	BYTE	command;
	BYTE	printer;
	BYTE	priority;
	BYTE	queue [512];
} NWPS_ReqstPacket_DSChgQPriority;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DS_ADD_QUEUE ===================================================*/					
typedef struct NWPS_ReqstPkt_DSAddQToPrtr_T {
	BYTE	command;
	BYTE	printer;
	BYTE	priority;
	BYTE	queue [512];
} NWPS_ReqstPacket_DSAddQToPrtr;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DS_DELETE_QUEUE ================================================*/				
typedef struct NWPS_ReqstPkt_DSDelQFromPrtr_T {
	BYTE	command;
	BYTE	printer;
	BYTE	now;									/* "immediately" (TRUE) or
														"after current job (FALSE)       */
	BYTE	jobOutcome;                   /* discard job or return to queue	*/
	BYTE	queue [512];
} NWPS_ReqstPacket_DSDelQFromPrtr;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DS_GET_PRINTERS_FOR_QUEUE ======================================*/	
typedef struct NWPS_ReqstPkt_DSGetPtrsForQ_T {
	BYTE	command;
	BYTE	maxPrinters;
	BYTE	queue [512];
} NWPS_ReqstPacket_DSGetPtrsForQ;

typedef struct NWPS_ReplyPkt_DSGetPtrsForQ_T {
	WORD	returnCode;
	BYTE	numPrinters;						/* number of printers servicing 
														the print queue						*/
	BYTE	printers [255];					/* Buffer for returned printer 
														numbers 0-254 valid					*/
} NWPS_ReplyPacket_DSGetPtrsForQ;

/*===== CMD_DS_SCAN_NOTIFY_LIST ============================================*/			
typedef struct NWPS_ReqstPkt_DSGetNotifyObj_T {
	BYTE	command;
	BYTE	printer;
	WORD	sequenceNumber;
} NWPS_ReqstPacket_DSGetNotifyObj;

typedef struct NWPS_ReplyPkt_DSGetNotifyObj_T {
	WORD	returnCode;
	WORD	sequence;							/* new sequence number					*/
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	WORD	initial;                      /* Time to first notice             */
	WORD	repeat;								/* Time between subsequent notices	*/
	BYTE	name[512];							/* name of object to be notified    */
} NWPS_ReplyPacket_DSGetNotifyObj;

/*===== CMD_DS_CHANGE_NOTIFY ===============================================*/				
typedef struct NWPS_ReqstPkt_DSChgNotifyDel_T {
	BYTE	command;
	BYTE	printer;
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	WORD	initial;                      /* Time to first notice             */
	WORD	repeat;								/* Time between subsequent notices	*/
	BYTE	name[512];							/* name of object to be notified    */
} NWPS_ReqstPacket_DSChgNotifyDel;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DS_ADD_NOTIFY ==================================================*/					
typedef struct NWPS_ReqstPkt_DSAddNotifyObj_T {
	BYTE	command;
	BYTE	printer;
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	WORD	initial;                      /* Time to first notice             */
	WORD	repeat;								/* Time between subsequent notices	*/
	BYTE	name[512];							/* name of object to be notified    */
} NWPS_ReqstPacket_DSAddNotifyObj;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DS_DELETE_NOTIFY ===============================================*/				
typedef struct NWPS_ReqstPkt_DSDelNotifyObj_T {
	BYTE	command;
	BYTE	printer;
	WORD	type;                         /* type of the object i.e. OT_USER, 
														OT_USER_GROUP, etc.					*/
	BYTE	name[512];							/* name of object to be notified    */
} NWPS_ReqstPacket_DSDelNotifyObj;

/* typedef struct NWPS_ReplyPkt_Other_T {
	WORD	returnCode;
} NWPS_ReplyPacket_Other; */

/*===== CMD_DS_GET_JOB_ID ==================================================*/					
typedef struct NWPS_ReqstPkt_DSGetJobID_T {
	BYTE	command;
	BYTE	printer;
} NWPS_ReqstPacket_DSGetJobID;

typedef struct NWPS_ReplyPkt_DSGetJobID_T {
	WORD	returnCode;
	WORD	queueNameFormat;							/* Bindery == 0;
																Directory Services == 1    */
	DWORD jobID;
	union queueName {
		struct binderyFormat {
			BYTE	NWServerName [48];				/* Server whose bindery 
																contains the Queue named
																below                      */
			BYTE	name [48];							/* name of Queue bindery 
																object							*/
			} bindery;
		BYTE		DSName[512];						/* fully qualified name of 
																Directory Services Queue 
																object							*/
		} queue;
} NWPS_ReplyPacket_DSGetJobID;

#endif /* NWPS_PKT_INC */
