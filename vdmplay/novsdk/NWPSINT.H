/*
//ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ Copyright (c) Unpublished Work of Novell, Inc.  All rights reserved.	  บ
//บ																								  บ
//บ THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, PROPRIETARY  บ
//บ AND TRADE SECRET INFORMATION OF NOVELL, INC.									  บ
//บ																								  บ
//บ ACCESS TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A	  บ
//บ NEED TO KNOW HOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR ASSIGNMENTS  บ
//บ AND (II) ENTITIES OTHER THAN NOVELL, INC. WHO HAVE ENTERED INTO			  บ
//บ APPROPRIATE LICENSE AGREEMENTS.                                          บ
//บ																								  บ
//บ NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED, COPIED,			  บ
//บ DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED,         บ
//บ EXPANDED, COLLECTED, COMPILED, LINKED, RECAST, TRANSFORMED OR ADAPTED	  บ
//บ WITHOUT THE PRIOR WRITTEN CONSENT OF NOVELL, INC.                        บ
//บ																								  บ
//บ ANY USE OR EXPLOITATION OF THIS WORK WITHOUT AUTHORIZATION COULD SUBJECT บ
//บ THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.								  บ
//ฬออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน
//บ Source module name: NWPSINT.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/

/*
//   The version of the databases for which the API's work is 4.0.
//   The following definitions are for the Names and Locations of
//   the Bindery files used for the databases.
*/

/*****************************/
/* PrintDef file information */
/*****************************/

#define PDF_LIBRARY_VERSION				40L

/*=============================*/
/* Bindery names and locations */
/*=============================*/
#ifdef NWNLM
#define PDF_FILE_NAME						"%s\\SYS:PUBLIC\\PRINTDEF.DAT"
#define PDF_FILE_NAME_PRE_40				"%s\\SYS:PUBLIC\\NET$PRN.DAT"
#define PDF_SHORT_FILE_NAME				"SYS:PUBLIC\\PRINTDEF.DAT"
#define PDF_SHORT_FILE_NAME_PRE_40		"SYS:PUBLIC\\NET$PRN.DAT"
#else
#define PDF_FILE_NAME						"\\\\%s\\SYS\\PUBLIC\\PRINTDEF.DAT"
#define PDF_FILE_NAME_PRE_40				"\\\\%s\\SYS\\PUBLIC\\NET$PRN.DAT"
#define PDF_SHORT_FILE_NAME				"SYS:PUBLIC\\PRINTDEF.DAT"
#define PDF_SHORT_FILE_NAME_PRE_40		"SYS:PUBLIC\\NET$PRN.DAT"
#endif

/*****************************/
/* PrintCon file information */
/*****************************/
#define JOB_MAJOR_VERSION					(BYTE)4
#define JOB_MINOR_VERSION					(BYTE)1

#define JOB_MAJOR_VERSION_PRE_40       (BYTE)1
#define JOB_MINOR_VERSION_PRE_40       (BYTE)1

/*=============================*/
/* Bindery names and locations */
/*=============================*/
#define JOB_DB_NAME							"PRINTJOB.DAT"
#define JOB_BACKUP_NAME                "PRINTJOB.CPY"
#define JOB_RENAME_NAME                "PRINTJOB.REN"

#define JOB_DB_NAME_PRE_40             "PRINTCON.DAT"
#define JOB_BACKUP_NAME_PRE_40			"PRINTCON.CPY"
#define JOB_RENAME_NAME_PRE_40			"PRINTCON.REN"

/*=============================*/
/* Server/sys:Public/file_name */
/*=============================*/
#ifdef NWNLM
#define JOB_PUBLIC_PATH                "%s\\SYS:PUBLIC\\%s"
#else
#define JOB_PUBLIC_PATH                "\\\\%s\\SYS\\PUBLIC\\%s"
#endif

/*===================================*/
/* Server/sys:Mail/User_id/file_name */
/*===================================*/
#ifdef NWNLM
#define JOB_PRIVATE_PATH					"%s\\SYS:MAIL\\%lX\\%s"
#else
#define JOB_PRIVATE_PATH					"\\\\%s\\SYS\\MAIL\\%lX\\%s"
#endif

/*==============================*/
/* Strings used only internally */
/*==============================*/
#define NWPS_DEVICE_NONE_PRE_40			"(None)"
#define NWPS_MODE_NONE_PRE_40          "(None)"

/*=======================*/
/* File handle pseudonym */
/*=======================*/
#ifdef NWNLM
#define NWPS_FILE_HANDLE int
#else
#define NWPS_FILE_HANDLE int
#endif

/***********************************************/
/* Print Server Configuration file information */
/***********************************************/
#ifdef NWNLM
#define CFG_PSERVER_DIRECTORY          "%s\\SYS:SYSTEM\\%08lX"
#define CFG_FILESERV_FILE					"%s\\SYS:SYSTEM\\%08lX\\FILESERV"
#define CFG_DESCRIPT_FILE					"%s\\SYS:SYSTEM\\%08lX\\DESCRIPT"
#define CFG_PRINT_FILE						"%s\\SYS:SYSTEM\\%08lX\\PRINT.%03d"
#define CFG_PRINT_INDEX						"%s\\SYS:SYSTEM\\%08lX\\PRINT.IDX"
#define CFG_QUEUE_FILE						"%s\\SYS:SYSTEM\\%08lX\\QUEUE.%03d"
#define CFG_NOTIFY_FILE                "%s\\SYS:SYSTEM\\%08lX\\NOTIFY.%03d"
#else
#define CFG_PSERVER_DIRECTORY          "\\\\%s\\SYS\\SYSTEM\\%08lX"
#define CFG_FILESERV_FILE					"\\\\%s\\SYS\\SYSTEM\\%08lX\\FILESERV"
#define CFG_DESCRIPT_FILE					"\\\\%s\\SYS\\SYSTEM\\%08lX\\DESCRIPT"
#define CFG_PRINT_FILE						"\\\\%s\\SYS\\SYSTEM\\%08lX\\PRINT.%03d"
#define CFG_PRINT_INDEX						"\\\\%s\\SYS\\SYSTEM\\%08lX\\PRINT.IDX"
#define CFG_QUEUE_FILE						"\\\\%s\\SYS\\SYSTEM\\%08lX\\QUEUE.%03d"
#define CFG_NOTIFY_FILE                "\\\\%s\\SYS\\SYSTEM\\%08lX\\NOTIFY.%03d"
#endif


/*
//  these types are for 3.11 backward compatibilty only 
*/
#define NWPS_P_PAR_1                   0	 /* Parallel port 1			*/
#define NWPS_P_PAR_2                   1	 /* Parallel port 2			*/
#define NWPS_P_PAR_3                   2	 /* Parallel port 3			*/
#define NWPS_P_SER_1                   3	 /* Serial port 1        */
#define NWPS_P_SER_2                   4	 /* Serial port 2        */
#define NWPS_P_SER_3                   5	 /* Serial port 3        */
#define NWPS_P_SER_4                   6	 /* Serial port 4        */
#define NWPS_P_REM_PAR_1					7	 /* NPrinter - parallel port 1 */
#define NWPS_P_REM_PAR_2					8	 /* NPrinter - parallel port 2 */
#define NWPS_P_REM_PAR_3					9	 /* NPrinter - parallel port 3 */
#define NWPS_P_REM_SER_1					10  /* NPrinter - serial port 1	 */
#define NWPS_P_REM_SER_2					11  /* NPrinter - serial port 2	 */
#define NWPS_P_REM_SER_3					12  /* NPrinter - serial port 3	 */
#define NWPS_P_REM_SER_4					13  /* NPrinter - serial port 4	 */
#define NWPS_P_REM_OTHER_1             14  /* Other type of network printer  */
#define NWPS_P_ELSEWHERE_1             15  /* Defined Elsewhere		*/
#define NWPS_P_XNP_1                   16  /* eXtended Network Printer	 */
#define NWPS_P_LOC_AIO						17  /* AIO Auto-Start Printer   */
#define NWPS_P_REM_AIO						18  /* AIO User-Start Printer   */

#define NWPS_P_APPLE_1						100 /* Apple Talk printer		*/
#define NWPS_P_UNIX_1						200 /* UNIX User-Start Printer		*/

/*********************************************************************/

/*
//  printer's Queue Configuration File structure 
*/
typedef struct {
  char  name[48];
  BYTE  priority;
} QUEUE;

/*
//  printer's Notify Configuration File structure 
*/
typedef struct {
  char  name[48];
  WORD  type;
  WORD  first;
  WORD  next;
} NOTIFY;

/*
//  printer's Configuration File structure 
*/
typedef struct {
  char  name[48];                      /* Name of printer			 */
  WORD  printerSubtype;                /* Subtype of printer		 */
  WORD  useInterrupts;						/* Use interrupts or polling?   */
  WORD  irqNumber;							/* IRQ number for printer interrupt */
  WORD  serviceMode;                   /* Queue service mode		 */
  WORD  bufferSize;							/* Buffer size in K			 */
  WORD  baudRate;                      /* Baud rate  (Serial only)	*/
  WORD  dataBits;                      /* Data bits  (Serial only)	*/
  WORD  stopBits;                      /* Stop bits  (Serial only)	*/
  WORD  parity;								/* Parity type  (Serial only)   */
  WORD  useXonXoff;							/* Use XOn/XOff protocol? Serial only*/
  WORD  currentForm;                   /* Currently mounted form		*/
  /* Version 1.00 stops here */
  WORD  bannerType;							/* text or postscript banner	  */
  WORD  pollTime;                      /* Delay before checking queue	 */
  BYTE  station[ 10 ];						/* station restriction			*/
  BYTE  driverName[ 9 ];					/* name of NPRINTER.NLM to load */
  BYTE  yetToBeDesigned[ 31 ];			/* These bytes will contain info  */
  /*			.										such as network printer station */
  /*			.										restrictions, etc.  The exact   */
  /*			.										format hasn't been decided yet. */
  DWORD bufferLen;							/* Size of the buffer		 */
  BYTE  buffer[1024];						/* This is where XNP printers will */
  /*													store their individual info	*/
} PCONFIG;

#define PCONFIG_V100_SIZE					70
#define PCONFIG_V200_SIZE					sizeof(PCONFIG)


/*
//   NWPS_Job_Old_Db_Hdr is the first record in the 4.0 PrnConDB database.
//   It contains the following information about the database:
//     The version number,
//     the number of NWPS_Job_Rec records in PrnConDB,
//     the name of the default print job configuration and
//     the name of the job record owner.
*/
typedef struct {
  char  text[ 76 ];							/* Printcon database. Version 4.0 */
  char  defaultPJName[ 32 ];				/* Name of default NWPS_Job_Rec */
  char  defaultPJOwner[ 256 ];			/* owner of the job record */
  WORD  numOfRecords;						/* # of NWPS_Job_Rec's in PrnConDB	*/
  WORD  numOfBlocks;                   /* # of 50-(NWPS_Job_Name_Rec) blocks*/
  BYTE  majorVersion;						/* 4					  */
  BYTE  minorVersion;						/* 0                */
} NWPS_Job_Old_Db_Hdr;

/*
//   NWPS_Job_41_Db_Hdr is the first record in the 4.1 PrnConDB database.
//   It contains the following information about the database:
//     The version number,
//     the number of NWPS_Job_Rec records in PrnConDB,
//     the name of the default print job configuration and
//     the name of the job record owner IN UNICODE.
*/
typedef struct {
  char  text[ 76 ];							/* Printcon database. Version 4.1   */
  char  defaultPJName[ 32 ];				/* Name of default NWPS_Job_Rec     */
  char  unused[ 256 ];			         /* no longer used.                  */
  WORD  numOfRecords;						/* # of NWPS_Job_Rec's in PrnConDB	*/
  WORD  numOfBlocks;                   /* # of 50-(NWPS_Job_Name_Rec) blocks*/
  BYTE  majorVersion;						/* 4					  */
  BYTE  minorVersion;						/* 1 unicode defaultPJOwner etc. */
  WORD  pJDBOwner[ 256 ];			      /* owner of the default job record */
} NWPS_Job_41_Db_Hdr;

/*
//   NWPS_Job_Name_Rec is the type of record found in the
//   second section of the PrnConDB database.  Each one of
//   these records contains the name of each NWPS_Job_Rec
//   and a pointer to their location in the third section of
//   the database.  There is space set aside in this second
//   section for fifty NWPS_Job_Name_Rec records; if this
//   limit is exceeded then another fifty-record block following
//   the first one is allocated after the third section of the
//   database is moved down to make room for the expansion.
*/
typedef struct {
  char  printJobName[ NWPS_JOB_NAME_SIZE ];	/* 1 - 31 chars long + 0	*/
  long  offset;										/* Offset of the record
															// (from the beginning 
															// of the 3rd section for 4.0
															// databases and from the start
															// of the file for pre-4.0)		
															*/
} NWPS_Job_Name_Rec;

/*
// The following structures define the Pre-4.0 PrintCon Database
*/

#define NWPS_JOB_IDX_ELEMENTS          37

typedef struct {
	char		header[55];
	char		filler[2];		/* \r\n */
	char		copyright[55];
	char		filler1[3];    /* \r\n^Z */
	BYTE		fileTag;       /* ^C which won't be in old files */
	BYTE		majorVersion;	/* 1 */
	BYTE		minorVersion;	/* 1 */
	char		defaultPJName[ NWPS_JOB_NAME_SIZE ];
} NWPS_Job_Pre40_Db_Hdr; /* this is the first structure in the file. */

/*
// The offset field for NWPS_Job_Name_Rec is absolute from the beginning
// of the file for this database.
*/
typedef struct {
	NWPS_Job_Name_Rec node[ NWPS_JOB_IDX_ELEMENTS ];
} NWPS_Job_Pre40_Idx; /* this is the second structure in the file. */

typedef struct {
	char		targetServer[48];
	char		targetQueue[48];
	BYTE		tabSize;
	WORD		numberOfCopies;	 
	char		formName[13];
	BYTE		reserve[27];
	BYTE		notifyUserFlag;
	DWORD    printServerID;
	char		bannerNameField[13];
	char		bannerFileField[13];
	char		deviceName[33]; /*  used to acces more info in printdef */
	char		modeName[33];	 /*  used to acces more info in printdef */
	BYTE		bannerFlag;       /*  printControlFlag  */
	BYTE		bytestreamFlag;  /*	printControlFlag	*/
	BYTE		formfeedFlag;	  /*	printControlFlag	*/
	WORD		timeOut;
	BYTE		localPrinter;
	BYTE		autoEndspool;
} NWPS_Job_Pre40_Rec; /* the balance of the file is composed of these structures
					that are pointed to by the index nodes. */

/*
// This routine hashes strings for use in searching the databases for 3.11
// stuff.
*/
int NWFAR NWPASCAL _NWPSHashFunction(
	char NWFAR *string,
	int hashTableSize);

/*
// This is the table used by the 3.11 Hash Function for translation
*/
extern char _NWPShashTranslateTable[256];

/*
// NLMs need to know what this struct looks like here, but everyone 
// in the PRINTDEF code needs this.
*/
typedef struct Pdf_Hash_Record {
	DWORD offset;
	BYTE	length;
} NWPS_Pdf_Hash_Record;

typedef struct EngineInfo {
	char		 recName[33/*NWPS_DEVI_NAME_SIZE + 1*/];	 /*used for device and mode*/
	char		 wantUpperCaseRecName[33/*NWPS_DEVI_NAME_SIZE + 1*/];  /*used for device and mode*/
	char		 upperCaseRecName[33/*NWPS_DEVI_NAME_SIZE + 1*/];	 /*used for device and mode*/
	WORD		 recNameLen;
	WORD		 devNameLen;
	WORD		 tempWord;

	DWORD     entryOffset;
	DWORD     deviOffset;
	DWORD     modeOffset;
	DWORD     funcOffset;
	DWORD     tempOffset;

	DWORD     maxEntries;

	DWORD     pdfScanIndex;   /* table index returned to the caller */
	WORD		 modeNumber;		/* mode number */
	WORD		 funcNumber;		/* function number */
	WORD		 index, index2;	/* internal temporary indexes */
	NWPS_Pdf_Hash_Record *hashTable;

	/*
	// bit array for used form numbers 
	*/
	BYTE		 usedForms[8 /*(NWPS_MAX_FORMS + 7) / 8*/];

	WORD		 formNumber;
	WORD		 formWidth;
	WORD		 formLength;

	WORD		 funcSize;
	WORD		 bytesSkipped;
	WORD		 bufferedBytes;
	WORD		 readSize;

	/*
	// version variable 
	*/
	DWORD      version;

	/*
	// add option variables 
	*/
	DWORD      recSize;
	DWORD      recOffset;
	WORD		 *indexList;
	DWORD		 *pre40IndexList;

	/*
	// delete option variables 
	*/
	DWORD      *offsetList;

	/*
	// Modify option variables 
	*/
	DWORD      newRecSize;
	WORD		 newNameLen;
} EngineInfo_T;

#ifdef NWNLM
/*
// NWPSGetThreadDataAreaPtr returns the pointer to data needed by
// the NLM versions of the calls.
*/
	typedef struct NWPS_Malloc_Struct
	{
		struct NWPS_Malloc_Struct *next;
		long int            size;	 /* size of the struct + area which follows */
	} NWPS_Malloc_Struct;

	typedef struct NWPSLibraryThreadDataStruct
	{
		struct NWPSLibraryThreadDataStruct *next;         /* = NULL */
		/*
		// Owner Thread Identification
		*/
		int threadID;				 

		/*
		// psnlmmalloc needs the following three fields 
		*/
		struct NWPS_Malloc_Struct *clientMemory;			  /* = NULL */
		long int            maxMemAllocation;				  /* = 0;*/
		long int            curMemAllocation;				  /* = 0;*/

		/*
		// pdf calls need the next three fields		 
		*/
		int					  pdfDatabaseFileDontClose;	  /* =  0;*/
		NWPS_FILE_HANDLE	  pdfDatabaseFileHandle;		  /* = -1;*/
		EngineInfo_T		  pdfDatabaseEngineInfo; /* initialized in _PdfGetCallInfo */

		/*
		// cominit needs the following field
		*/
		int IPXSocketRTag;										  /* =  0;*/
		void *comListHead;										  /* = NULL */
		int comSemaphoreHdl;                              /* = -1;*/

	} NWPSLibraryThreadDataStruct;

NWCCODE NWPSGetThreadDataAreaPtr(
	NWPSLibraryThreadDataStruct **threadDataAreaPtr);
#endif

/*********************************************************************/

/*
//   PrintDef File Structure
//   Header:
//     DWORD	Version Number
//     DWORD	File Size
//     DWORD	Form DataBase Offset
//     DWORD	Device DataBase Offset
//     DWORD	(future offsets)
//     .
//     .
//     .
// 
//   Form DataBase:
//     DWORD	Record Count
//     DWORD	Record Offsets[ count ]
//   Form Records:
//     WORD    Form Number
//     WORD    Form Width
//     WORD    Form Length
//     WORD    Name Size (including '\0')
//     char    Form Name[Name Size]
//     * padded to the nearest DWORD *
// 
//   Device DataBase:
//     DWORD	Record Count
//     DWORD	Record Offsets[ count ]
//   Device Records:
//     WORD    Mode Count
//     WORD    Function Count
//     WORD    Name Size (including '\0')
//     DWORD	Mode Offsets[Mode Count]
//     DWORD	Function Offsets[Function Count]
//     char    Device Name[Name Size]
//     * padded to the nearest DWORD *
//     Mode Records:
//       WORD	  Function Count
//       WORD	  Name Size (including '\0')
//       WORD	  Function Index[Function Count]
//       char	  Mode Name[Name Size]
//       * padded to the nearest DWORD *
//     Function Records:
//       WORD	  Function Size
//       WORD	  Name Size (including '\0')
//       BYTE	  Function String[Function Size]
//       char	  Function Name[Name Size]
//       * padded to the nearest DWORD *
*/

/*********************************************************************/
/*
//   Print server request codes
*/

/*
//  general commands 
*/
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
#define CMD_REWIND_JOB_WITH_ID			0x1B
#define CMD_ABORT_JOB_WITH_ID          0x1C
#define CMD_ADD_FORM                   0x1D
#define CMD_DISMOUNT_FORM					0x1E
#define CMD_GET_EXT_PRINTER_STATUS		0x1F
#define CMD_GET_EXT_JOB_STATUS			0x20

/*
//  NPrinter commands 
*/
#define CMD_GET_RPRINTER					0x80
#define CMD_CONNECT_RPRINTER				0x81
#define CMD_SET_REMOTE_MODE				0x82
#define CMD_SCAN_RPRINTER					0x84

/*
//  Directory Service commands 
*/
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

/*
//  XNP Printer commands 
*/
#define XNP_ACCEPT_JOB						0x00
#define XNP_CLOSE_FILE						0x01
#define XNP_CREATE_BANNER					0x02
#define XNP_DECLINE_JOB                0x03
#define XNP_DEREGISTER						0x04
#define XNP_FINISH_JOB						0x05
#define XNP_GET_CONFIG						0x06
#define XNP_OPEN_FILE						0x07
#define XNP_READ_FILE						0x08
#define XNP_REGISTER                   0x09
#define XNP_SEEK_FILE						0x0A
#define XNP_SEND_STATUS                0x0B
#define XNP_WRITE_FILE						0x0C

/*
//  XNP PServer commands 
*/
#define XNP_ABORT_JOB						0x80
#define XNP_GOING_DOWN						0x81
#define XNP_JOB_STATUS						0x82
#define XNP_NEW_JOB							0x83
#define XNP_PAUSE_JOB						0x84
#define XNP_PAUSE_PRINTER					0x85
#define XNP_RECONFIGURE                0x86
#define XNP_REWIND_JOB						0x87
#define XNP_START_JOB						0x88
#define XNP_START_PRINTER					0x89

/*********************************************************************/
/*********************************************************************/

