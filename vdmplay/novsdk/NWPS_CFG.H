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
//บ Source module name: NWPS_CFG.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_DEF_INC
#include <nwps_def.h>
#endif

#ifndef NWPS_CFG_INC
#define NWPS_CFG_INC
/****************************************************************************/
/*************** CFG - Print Server Configuration Information ***************/
/****************************************************************************/

/*====================== Configuration Attributes ==========================*/
#define NWPS_ATTR_CART						0	 /* Cartridge							 */
#define NWPS_ATTR_CLASS                1	 /* Object Class						 */
#define NWPS_ATTR_CN                   2	 /* CN or Common Name				 */
#define NWPS_ATTR_CONF						3	 /* Printer Configuration			 */
#define NWPS_ATTR_DQUEUE					4	 /* Default Queue                 */
#define NWPS_ATTR_DESC						5	 /* Description						 */
#define NWPS_ATTR_DEVICE					6	 /* Device								 */
#define NWPS_ATTR_HOST_DEV             7	 /* Host Device						 */
#define NWPS_ATTR_HOST_RES             8	 /* Host Resource                 */
#define NWPS_ATTR_HOST_SER             9	 /* Host Server						 */
#define NWPS_ATTR_L							10  /* L or Locality                 */
#define NWPS_ATTR_MEMORY					11  /* Memory								 */
#define NWPS_ATTR_NADD						12  /* Network Address					 */
#define NWPS_ATTR_NADD_REST				13  /* Network Address Restriction	 */
#define NWPS_ATTR_NOTIFY					14  /* Notify								 */
#define NWPS_ATTR_O							15  /* O or Organization				 */
#define NWPS_ATTR_OPER						16  /* Operator							 */
#define NWPS_ATTR_OU                   17  /* OU or Organizational Unit     */
#define NWPS_ATTR_OWNER                18  /* Owner								 */
#define NWPS_ATTR_PAGE						19  /* Page Description Language     */
#define NWPS_ATTR_PJOB						20  /* Print Job Configuration		 */
#define NWPS_ATTR_PCTRL                21  /* Printer Control					 */
#define NWPS_ATTR_PRINT_SER				22  /* Queue Volume Name				 */
#define NWPS_ATTR_PRINTER					23  /* Printer                       */
#define NWPS_ATTR_PRIV_KEY             24  /* Private Key						 */
#define NWPS_ATTR_PUBL_KEY             25  /* Public Key                    */
#define NWPS_ATTR_QUEUE                26  /* Queue								 */
#define NWPS_ATTR_QUE_DIR					27  /* Queue Directory					 */
#define NWPS_ATTR_SAP						28  /* SAP Name							 */
#define NWPS_ATTR_SEE_ALSO             29  /* See Also							 */
#define NWPS_ATTR_SERIAL					30  /* Serial Number                 */
#define NWPS_ATTR_SERVER					31  /* Server								 */
#define NWPS_ATTR_STAT						32  /* Status								 */
#define NWPS_ATTR_TYPE						33  /* Supported Typefaces           */
#define NWPS_ATTR_USER						34  /* User                          */
#define NWPS_ATTR_VERS						35  /* Version                       */
#define NWPS_ATTR_VOLUME					36  /* Queue Volume Name				 */
#define NWPS_ATTR_ACL						37  /* Access Control					 */

/* StartFlag meanings */
#define NWPS_AUTO_START                1	 /* Printer is started by PServer */
#define NWPS_USER_START                0	 /* Printer is started by user	 */

/*==== For operator notification purposes, this means notify job owner =====*/
#define NWPS_JOB_OWNER						-1

/*============================== Banner types ==============================*/
#define NWPS_BANNER_TEXT					0	 /* Text banner is generated		 */
#define NWPS_BANNER_POST					1	 /* PostScript banner generated	 */

/*================ Flags for NWPSCfgGetPrinterDefaults: ====================*/
#define NWPS_DEFAULT                   (WORD) -2/* Default type, or subtype */

/*============================ Printer Types ==============================*/
#define NWPS_P_ELSEWHERE					(WORD) -1 /* Printer defined elsewhere*/
#define NWPS_P_OTHER                   0	 /* Other or Unknown Printer		 */
#define NWPS_P_PAR							1	 /* Parallel Printer              */
#define NWPS_P_SER							2	 /* Serial Printer					 */
#define NWPS_P_XNP							3	 /* eXtended Network Printer		 */
#define NWPS_P_APPLE                   4	 /* AppleShare Printer				 */
#define NWPS_P_UNIX							5	 /* Unix Printer						 */
#define NWPS_P_AIO							6	 /* AIO Printer						 */

/*========================= SubType (port numbers) =========================*/
#define NWPS_PORT_1							0
#define NWPS_PORT_2							1
#define NWPS_PORT_3							2
#define NWPS_PORT_4							3
#define NWPS_PORT_5							4
#define NWPS_PORT_6							5
#define NWPS_PORT_7							6
#define NWPS_PORT_8							7
#define NWPS_PORT_9							8
#define NWPS_PORT_10                   9

/*=================== Possible Print Server status codes ===================*/
#define NWPS_RUNNING                   0	 /* Running                        */
#define NWPS_GOING_DOWN                1	 /* Ready to quit when jobs finish */
#define NWPS_DOWN                      2	 /* Ready to quit                  */
#define NWPS_INITIALIZING					3	 /* Initialization in progress	  */

/*=================== Possible Printer status codes ========================*/
#define NWPS_PSTAT_JOB_WAIT				0
#define NWPS_PSTAT_FORM_WAIT				1
#define NWPS_PSTAT_PRINTING				2
#define NWPS_PSTAT_PAUSED					3
#define NWPS_PSTAT_STOPPED             4
#define NWPS_PSTAT_MARK_EJECT          5
#define NWPS_PSTAT_READY_TO_DOWN       6
#define NWPS_PSTAT_NOT_CONNECTED       7
#define NWPS_PSTAT_PRIVATE             8
#define NWPS_PSTAT_CONNECTING          9

/*======================= Queue service modes ==============================*/
#define NWPS_QUEUE_ONLY                0
#define NWPS_QUEUE_BEFORE_FORM			1
#define NWPS_FORM_ONLY						2
#define NWPS_FORM_BEFORE_QUEUE			3

/*******************************************************************/
/* Values for Serial Port control as stored in configuration files */
/*******************************************************************/

/*==================== Serial Port Baud Rates ==============================*/
#define NWPS_BAUD_RATE_0300				0
#define NWPS_BAUD_RATE_0600				1
#define NWPS_BAUD_RATE_1200				2
#define NWPS_BAUD_RATE_2400				3
#define NWPS_BAUD_RATE_4800				4
#define NWPS_BAUD_RATE_9600				5
#define NWPS_BAUD_RATE_19200				6
#define NWPS_BAUD_RATE_38400				7

/*==================== Serial Port Data Bits ===============================*/
#define NWPS_DATA_BITS_5					5
#define NWPS_DATA_BITS_6					6
#define NWPS_DATA_BITS_7					7
#define NWPS_DATA_BITS_8					8

/*==================== Serial Port Stop Bits ===============================*/
#define NWPS_STOP_BITS_1					0
#define NWPS_STOP_BITS_1_5             1
#define NWPS_STOP_BITS_2					2

/*==================== Serial Port Parity Type =============================*/
#define NWPS_PARITY_NONE					0
#define NWPS_PARITY_EVEN					1
#define NWPS_PARITY_ODD                2

/****************************************************************/
/* Values for AIO Port control as stored in configuration files */
/****************************************************************/

/*================= Default hardware, board, or port =======================*/
#define NWPS_AIO_WILDCARD					(-1)	

/*======================= AIO Port Hardware Types ==========================*/
/* reserved                            0			*/
#define NWPS_AIO_COMX_TYPE             1
#define NWPS_AIO_ARTIC_TYPE				2
#define NWPS_AIO_WNIM_TYPE             3
/* reserved                            4 - 99	*/
/* unused										100		*/
#define NWPS_AIO_AVMB1_TYPE				101
#define NWPS_AIO_ANIC_TYPE             102
#define NWPS_AIO_WNIC_TYPE             103
#define NWPS_AIO_HAYES_TYPE				104
#define NWPS_AIO_DIGIXI_TYPE				105
#define NWPS_AIO_DIGICX_TYPE				106
#define NWPS_AIO_DIGIXM_TYPE				107
#define NWPS_AIO_DIGIEP_TYPE				108
#define NWPS_AIO_NPCC_TYPE             109
/* unused										110-		*/

/*========================= AIO Port Baud Rates ============================*/
#define NWPS_AIO_BAUD_50					0
#define NWPS_AIO_BAUD_75					1
#define NWPS_AIO_BAUD_110					2
#define NWPS_AIO_BAUD_134p5				3
#define NWPS_AIO_BAUD_150					4
#define NWPS_AIO_BAUD_300					5
#define NWPS_AIO_BAUD_600					6
#define NWPS_AIO_BAUD_1200             7
#define NWPS_AIO_BAUD_1800             8
#define NWPS_AIO_BAUD_2000             9
#define NWPS_AIO_BAUD_2400             10
#define NWPS_AIO_BAUD_3600             11
#define NWPS_AIO_BAUD_4800             12
#define NWPS_AIO_BAUD_7200             13
#define NWPS_AIO_BAUD_9600             14
#define NWPS_AIO_BAUD_19200				15
#define NWPS_AIO_BAUD_38400				16
#define NWPS_AIO_BAUD_57600				17
#define NWPS_AIO_BAUD_115200				18

/*====================== AIO Port Data Bits ================================*/
#define NWPS_AIO_DATA_BITS_5				0
#define NWPS_AIO_DATA_BITS_6				1
#define NWPS_AIO_DATA_BITS_7				2
#define NWPS_AIO_DATA_BITS_8				3

/*====================== AIO Port Stop Bits ================================*/
#define NWPS_AIO_STOP_BITS_1				0
#define NWPS_AIO_STOP_BITS_1p5			1
#define NWPS_AIO_STOP_BITS_2				2

/*====================== AIO Port Parity Type ==============================*/
#define NWPS_AIO_PARITY_NONE				0
#define NWPS_AIO_PARITY_ODD				1
#define NWPS_AIO_PARITY_EVEN				2
#define NWPS_AIO_PARITY_MARK				3
#define NWPS_AIO_PARITY_SPACE          4

/*================== Possible types of print servers =======================*/
#define NWPS_TYPE_UNKNOWN					0	 /* Pre 1.1 PServer type */
#define NWPS_TYPE_EXE						1	 /* Dedicate PServer for DOS */
#define NWPS_TYPE_NLM						2	 /* NetWare Loadable Module  */
#define NWPS_TYPE_SERVER_VAP				3	 /* VAP, in server*/
#define NWPS_TYPE_BRIDGE_VAP				4	 /* VAP, in Bridge */
#define NWPS_TYPE_UNIX						5	 /* NetWare For Unix PServer */

/***************************************************************/
/* Printer configuration structures which make up NWPS_PConfig */
/***************************************************************/
/*=================== Serial Printer Config Info ===========================*/
typedef struct NWPS_Serial_t {
  WORD  portNumber;							/* COMn port number		  */
  WORD  startFlag;							/* TRUE - Auto-start Printer	*/
													/* FALSE - User will start printer	*/
  WORD  useInterrupts;						/* TRUE - use irq driver	 */
  WORD  irqNumber;							/* IRQ number for printer	 */
  WORD  useXonXoff;							/* Use X-On/X-Off? (Serial)	*/
  WORD  baudRate;                      /* Baud rate  (Serial)	  */
  WORD  dataBits;                      /* Data bits  (Serial)	  */
  WORD  stopBits;                      /* Stop bits  (Serial)	  */
  WORD  parity;								/* Parity type  (Serial)	 */
} NWPS_Serial;

/*=================== Parallel Printer Config Info =========================*/
typedef struct NWPS_Parallel_t {
  WORD  portNumber;							/* LPTn port number		  */
  WORD  startFlag;							/* TRUE - Auto-start Printer	*/
													/* FALSE - User will start printer	*/
  WORD  useInterrupts;						/* TRUE - use irq driver	 */
  WORD  irqNumber;							/* IRQ number for printer	 */
} NWPS_Parallel;

/*=================== AIO Printer Config Info ==============================*/
typedef struct NWPS_Aio_t {
  WORD  reserved1;							/* Reserved for future use */
  WORD  startFlag;							/* TRUE - Auto-start Printer	*/
													/* FALSE - User will start printer	*/
  WORD  hardwareType;						/* */
  BYTE  boardNumber;                   /* */
  BYTE  portNumber;							/* */
  BYTE  useXonXoff;							/* Use X-On/X-Off?  */
  BYTE  aioMgr;								/* */
  WORD  baudRate;                      /* Baud rate  */
  WORD  dataBits;                      /* Data bits  */
  WORD  stopBits;                      /* Stop bits  */
  WORD  parity;								/* Parity type  */
} NWPS_Aio;

/*=================== AppleTalk Printer Config Info ========================*/
typedef struct NWPS_AppleTalk_t {
  char  netPrinterName[NWPS_APPLE_NAME_SIZE + 2];
													/* AppleTalk Network Printer Name */
  char  netPrinterType[NWPS_APPLE_TYPE_SIZE + 2];
													/* AppleTalk Network Printer Type */
  char  netPrinterZone[NWPS_APPLE_ZONE_SIZE + 2];
													/* AppleTalk Network Printer Zone */
  WORD  hideFlag;                      /* TRUE - hide printer */
  WORD  errorFlag;							/* TRUE - print error banner */
} NWPS_AppleTalk;

/*=================== Unix Printer Config Info =============================*/
typedef struct NWPS_Unix_t {
  char  hostName[NWPS_UNIX_HOST_SIZE + 1];
													/* Name of the unix host  */
  char  hostPrinter[NWPS_UNIX_PRNT_SIZE + 1];
													/* Unix printer name  */
} NWPS_Unix;

typedef struct {
  WORD  printerType;                   /* Type of printer		  */
  WORD  currentForm;                   /* Currently mounted form	 */
  WORD  bufferSize;							/* Buffer size in K		  */
  WORD  serviceMode;                   /* Queue service mode	  */
  WORD  pollTime;                      /* Queue poll time		  */
  WORD  bannerType;							/* FALSE - text banner page	*/
													/* TRUE - postscript banner page*/
  DWORD length;								/* The length of union-ed data  */
  char  driverName[9];						/* up to 8 chars for aaaaaaaa.NLM 
														plus a zero */
  char  reserved_for_future[ 23 ];		/* pad to an even number of DWORDs */
  union {
	 NWPS_Serial	  ser;
	 NWPS_Parallel   par;
	 NWPS_Aio		  aio;
	 NWPS_AppleTalk  apl;
	 NWPS_Unix		  unx;
	 BYTE            oth[NWPS_OTHER_SIZE];
  } type;
} NWPS_PConfig;

/*==========================================================================
  Operator, User, Owner, and Notify attributes use
  this structure to get the object types back.
  For Notify attributes the tName points to a Typed_Name_T
  structure, while the others point to a char array
  ==========================================================================*/
typedef struct {
  WORD  objectType;
  void  NWPTR tName;
} NWPS_Typed_Name;


/*==========================================================================
  The following types are used for the listed attribute:
  - NWPS_Typed_Name (name field is a char*) Operator, Owner, User
  - NWPS_Typed_Name (name field is a Typed_Name_T*) Notify
  - Typed_Name_T used for Queue, Printer and Print Server attr. values
  - Octet_String_T used for Printer Configuration attr. value
  - Net_Address_T used for Network Address and Restriction attr. values
  These attributes are defined in the directory services header:
	 nwdsattr.h
  ==========================================================================*/

/*============ internal table of known attribute names =====================*/
extern char NWPTR _attrName[];

#if defined(NWPS_CONN_TYPE)
#	 if	NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE
#	  include <nwpscfg0.h>
#	 elif NWPS_CONN_TYPE == NWPS_DIRECTORY_SERVICE
#	  include <nwpscfg1.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_PRE_40
#	  include <nwpscfg0.h>
#	 elif NWPS_CONN_TYPE == NWPS_BINDERY_SERVICE_ALL
#	  include <nwpscfg0.h>
#	 endif
#else
#	  include <nwpscfgx.h>
#endif

#endif

