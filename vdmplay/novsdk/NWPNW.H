// **************************************************************************
//
// NWPNW.H: Personal NetWare Library Header File
//
// Notes: All of the LENGTH constants assume byte-length, NOT character
//        length (as in a multi-byte environment), so strncpy() calls can
//        safely use these constants as memory limits. -MES
//
// Last Modified: [3-10-94]
//
// **************************************************************************
// (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
// No part of this file may be duplicated, revised, translated, localized or
// modified in any manner or compiled, linked, or uploaded or downloaded to or
// from any computer system without the prior written consent of Novell, Inc.
// **************************************************************************

#ifndef	__nwpnw__
#define	__nwpnw__		/* Nothing */


#include <nwlocale.h>
#include <ntypes.h>


#if defined (_Windows)						// _Windows is set by Borland...

#	if !defined (NWWIN)
#		define NWWIN
#	endif

#	include <windows.h>

#	define NWsprintf NWwsprintf			// The NWLOCALE library for windows 
													// does not have NWwsprintf -mgb
#endif


// **************************************************************************
// From NWPNWERR.H: Personal NetWare Error Codes
//
// **************************************************************************


#include <nwerror.h>		// For NetWare Error codes

#undef SOCKET_NOT_OPEN
#undef SOCKET_ALREADY_OPEN

#define	CC_SUCCESS								0x0000
#define	CC_FAILURE								0xFFFF


// DOS Error Codes

#define DOS_ERR_INVALID_FUNCTION				
#define DOS_ERR_FILE_NOT_FOUND				2
#define DOS_ERR_PATH_NOT_FOUND				3
#define DOS_ERR_TOO_MANY_OPEN_FILES			4
#define DOS_ERR_ACCESS_DENIED					
#define DOS_ERR_INVALID_HANDLE				6
#define DOS_ERR_INSUFFICIENT_MEMORY			8

#define DOS_ERR_INVALID_DRIVE					5
#define DOS_ERR_CANNOT_REMOVE_CUR_DIR		16
#define DOS_ERR_NOT_SAME_DEVICE				17
#define DOS_ERR_NO_MORE_FILES					8
#define DOS_ERR_WRITE_PROTECTED				19
#define DOS_ERR_DRIVE_NOT_READY				21
#define DOS_ERR_UNKNOWN_COMMAND				22

#define DOS_ERR_SEEK_ERROR						5
#define DOS_ERR_PRINTER_OUT_OF_PAPER		28
#define DOS_ERR_WRITE_FAULT					29
#define DOS_ERR_READ_FAULT						0
#define DOS_ERR_GENERAL_FAILURE				31
#define DOS_ERR_SHARING_VIOLATION			32
#define DOS_ERR_LOCK_VIOLATION				33

#define DOS_ERR_SHARING_BUFFER_OVERFLOW	36
#define DOS_ERR_DISK_FULL						9

#define DOS_ERR_BAD_NET_REQUEST				50
#define DOS_ERR_NO_SERVER_RESPONSE			51
#define DOS_ERR_DUPLICATE_NAME				52
#define DOS_ERR_NET_NAME_NOT_FOUND			53
#define DOS_ERR_NET_BUSY						4
#define DOS_ERR_INVALID_DEVICE				55
#define DOS_ERR_GENERAL_NET_ERROR			59

#define DOS_ERR_PRINT_QUEUE_FULL				1
#define DOS_ERR_PRINT_FILE_TOO_BIG			62
#define DOS_ERR_PRINT_FILE_DELETED			63

#define DOS_ERR_DEVICE_TYPE_BAD				66
#define DOS_ERR_SERVER_NOT_FOUND				7
#define DOS_ERR_SERVER_TOO_LONG				68

#define DOS_ERR_REDIRECTION_PAUSED			72
//#define MYDOS_ERR_NET_RESOURCE_OUT		78

#define DOS_ERR_INT24_FAIL						3
#define DOS_ERR_TOO_MANY_REDIRS				84	/* out of redirection table space */
#define DOS_ERR_DUPLICATE_REDIR				85	/* already redirected */
#define DOS_ERR_INVALID_PASSWORD				6
#define DOS_ERR_INVALID_PARAM					7


// **************************************************************************
// PNW Library error codes

#define MK_MP(l,h)	((void NWFAR *)(((uint16)(l)) | ((uint32)((uint16)(h))) << 16))
#define MP_LOW(l)		((uint16)(uint32)(l))
#define MP_HIGH(l)	((uint16)(((uint32)(l) >> 16) & 0xffff))

#define CC_PNW_ERROR								0x7000
#define CC_PNW_LOAD_UNICODE_FAILED			(CC_PNW_ERROR | 0x01)


// client library completion codes
#define CC_CLIENT_NOT_RESIDENT				0x4001
#define CC_IPX_NOT_RESIDENT					0x4002
#define CC_SERVER_NOT_RESIDENT				0x4003
#define CC_VLM_NOT_RESIDENT					0x4004

#define CC_USER_NAME_TOO_LONG					0x4005
#define CC_SERVER_NAME_TOO_LONG				0x4006
#define CC_PASSWORD_TOO_LONG					0x4007

#define CC_TOO_MANY_ENTRIES					0x4008
#define CC_TOO_MANY_USERS						CC_TOO_MANY_ENTRIES
#define CC_TOO_MANY_RESOURCES					CC_TOO_MANY_ENTRIES

#define CC_NO_RIGHTS_TO_RESOURCE				0x4009
#define CC_NO_RIGHTS_TO_NET_DIR				CC_NO_RIGHTS_TO_RESOURCE
#define CC_NO_RIGHTS_TO_NET_PRN				CC_NO_RIGHTS_TO_RESOURCE

#define CC_DUPLICATE_NAME						0x400A
#define CC_DUPLICATE_RESOURCE					CC_DUPLICATE_NAME
#define CC_DUPLICATE_USER						CC_DUPLICATE_NAME

#define CC_NOT_FOUND								0x400B
#define CC_RESOURCE_NOT_FOUND					CC_NOT_FOUND
#define CC_PRINT_JOB_NOT_FOUND				CC_NOT_FOUND
#define CC_SETUP_STRING_NOT_FOUND			CC_NOT_FOUND
#define CC_FORM_NOT_FOUND						CC_NOT_FOUND

#define CC_DUP_WORKGROUP_FOUND				0x400C

#define CC_NO_MORE_ENTRIES						0x0B00
#define CC_NO_FREE_SLOTS						0x0B01

#define CC_CLIENT_NOT_INITIALIZED			0x400C
#define CC_SERVER_NOT_INITIALIZED			0x400D
#define CC_VLM_NOT_INITIALIZED				0x400E

#define CC_NO_SERVER_LIST_TO_REBUILD		0x400F

#define CC_INVALID_SOURCE_PATH				0x4011
#define CC_INVALID_TARGET_PATH				0x4012



#define CC_LOCAL_OUT_OF_DYNAMIC_MEMORY		0x5001
#define CC_NO_SERVER_WITH_FREE_CONN			0x5002
#define CC_LOCAL_NO_MORE_SERVERS				0x5003


#define CC_LOCAL_NO_MORE_USERS				0x5005
#define CC_LOCAL_NO_MORE_RESOURCES			0x5005
#define CC_LOCAL_NO_MORE_WORKGROUPS			0x5005
#define CC_LOCAL_NO_MORE_SERVICES			0x5005

#define CC_LOCAL_REDIR_NOT_FOUND				0x5008



#define PNW_OPEN_FAILED							0x7F00
#define PNW_READ_FAILED							0x7E00
#define PNW_WRITE_FAILED						0x7D00
#define PNW_WRONG_READ_SIZE					0x7C00
#define PNW_GET_REDIR_FAILED					0x7B00
#define PNW_REDIRECT_DEVICE_FAILED			0x7A00
#define PNW_CANCEL_REDIR_FAILED				0x7900
#define PNW_DELETE_FAILED						0x7800
#define PNW_RENAME_FAILED						0x7700

#define PNW_DOS_ERROR							0x7000


#define PNW_MALLOC_FAILED						CC_LOCAL_OUT_OF_DYNAMIC_MEMORY
#define PNW_OUT_OF_MEMORY						CC_LOCAL_OUT_OF_DYNAMIC_MEMORY


// Fixed Disk Errors
#define CC_FDS_ERROR								0x6000
#define CC_CANNOT_CREATE_DIR					0x6001


/****************************************************************************
	Constants
*****************************************************************************/
#define MAX_NETWORK_NODES			50		// This is the key to upping the 
													// number of users, servers, etc.

#define MAX_PNW_ROUTES				20

#define MAX_VIEWABLE_RESOURCES	250	// This is the maximum number of 
													// resources we allow the user to see

#define MAX_VIEWABLE_SERVICES		250	// This is the maximum number of 
													// services we allow the user to see

#define MAX_LISTEN_ECBS				20		// This is the maximum  number of
													// listen ECBs for 
													// NWGetNetworksByRange to use

#define MIN_HOP_COUNT				1		// global minimum for 
													// NWGetNetworksByRange
#define MAX_HOP_COUNT				15 	// global maximum for 
													// NWGetNetworksByRange

#define REDIR_PRINTER_T	3
#define REDIR_DISK_T		4


// PNW Object Types
#define PNW_USER			2
#define PNW_PRINTER		3
#define PNW_DIRECTORY	4
#define PNW_ROUTE			5
#define PNW_SERVER		6

// PNW Control File Types
#define CF_SERVER			1
#define CF_USER			2
#define CF_RESOURCE		3
#define CF_WORKGROUP		5
#define CF_SECURITY		6
#define CF_ERROR			7
#define CF_AUDIT			8
#define CF_HANDLE			9
// Reserved for SCREEN -- 10
#define CF_PRINTER		11
#define CF_SETUP			12

#define CF_QUEUE_COM		0x10	// COM1 = 0x10, COM2 = 0x11
#define CF_QUEUE_LPT		0x20	// LPT1 = 0x20, LPT2 = 0x21, LPT3 = 0x22
#define CF_QUEUE_QUE		0x30	// QUE1 = 0x30, QUE2 = 0x31 ... QUE9 = 0x38


#define PNW_PATH_LENGTH			128
#define RESOURCE_PATH_LENGTH	128
#define MAX_DRIVE_LETTERS		26

#define MAX_OBJECT_SIZE					256
#define PRINTER_DEVICE_NAME_LENGTH	48

// Name Lengths
#define SERVER_NAME_LENGTH		16
#define RESOURCE_NAME_LENGTH	16
#define USER_NAME_LENGTH		16
#define SETUP_NAME_LENGTH		16
#define FULL_NAME_LENGTH		50
#define WORKGROUP_NAME_LENGTH	16
#define SECURITY_STRING_LENGTH	16
#define PASSWORD_STRING_LENGTH	16
#define DEVICE_STRING_LENGTH	5	// Enough for printer strings too
#define AUDIT_MESSAGE_LENGTH	81	// Artificial maximum, not really needed
#define MAX_NOTE_LENGTH			256
#define EXCEPTION_OBJECT_NAME_LENGTH	64	
	// Maximum length of path or object name for an exception
#define SERVICE_DATA_SIZE		512

#define SERVER_DESC_LENGTH		50

#define SPOOL_DIRECTORY_LENGTH	48
#define FORM_NAME_LENGTH		13


#define VLM_CFG_PATH_LENGTH		80

#define SERVICE_NAME_LENGTH		16
#define SERVICE_INFO_LENGTH		12
#define OEM_NAME_LENGTH			16

#define MAX_PNW_EXCLUSIONS		30
//#define MAX_SERVICES			10
#define NUM_SYNC_STAMPS			5

#define NUM_SCAN_EXCEPTIONS		20

#define NW_SERVER_NAME_LENGTH	48
#define NW_VOLUME_NAME_LENGTH	17
#define NW_USER_NAME_LENGTH		48
#define NW_OBJECT_NAME_LENGTH	48
#define NW_QUEUE_NAME_LENGTH	NW_OBJECT_NAME_LENGTH

#define NW_OBJ_WILD				0xFFFF
#define NW_OBJ_USER				0x0001
#define NW_OBJ_GROUP			0x0002
#define NW_OBJ_QUEUE			0x0003
#define NW_OBJ_SERVER			0x0004
#define NW_OBJ_DS_TREE			0x0278

#define OBJECT_NAME_LENGTH		16
#define	OBJ_SERVER_NAME_LENGTH	48

#define BANNER_NAME_LENGTH		14
#define CAP_DESC_LENGTH			13

#define MAX_RESOURCES			20		// Per server

#define MAX_PNW_SERVERS			MAX_NETWORK_NODES
#define MAX_USERS				MAX_NETWORK_NODES		// Connected USERS
#define MAX_DEFINED_USERS		250

#define MAX_WORKGROUPS			MAX_NETWORK_NODES
#define MAX_DEFINED_WORKGROUPS	MAX_NETWORK_NODES		// FIX ??

#define SETUP_VALUE_LENGTH		200
#define MAX_SETUP_RECORDS		50
#define MAX_PRINTER_FORMS		50
#define MAX_CLIENT_CONNECTIONS	32

#define SCREEN_LINE_LENGTH		80
/* reserve space for '(CTRL-ENTER to clear)' */
//#define MAX_MESSAGE_LENGTH		(SCREEN_LINE_LENGTH-21)
#define MAX_MESSAGE_LENGTH		58
#define MAX_SIDEBAND_DATA		(MAX_MESSAGE_LENGTH+1)
#define MESSAGE_STRING_LENGTH 	30

// For Get/SetReceiveFlags
#define MESSAGES_ON				0x00
#define MESSAGES_OFF			0x02
#define MESSAGES_FOREVER		0

#define ALL_NW_RIGHTS			0xDF
#define READ_NW_RIGHTS			0x45
#define WRITE_NW_RIGHTS			0x0A
#define NO_NW_RIGHTS			0x00

// Printer flags used in the internal printer structure

//#define PRN_BANNER_FLAG			0x01
//#define PRN_EXPAND_TABS_FLAG	0x02
//#define PRN_TIMING_OUT			0x04		/* used by client only */
//#define PRN_CONTINUOUS			0x08		/* print direct */
//#define PRN_FORM_FEED			0x10		/* form feed wanted */
//#define PRN_TIMED_HOLD			0x20		/* hold job on submittal */
//#define PRN_HOLD_FOREVER		0x40		/* hold forever */
//#define PRN_NOTIFY				0x80		/* notify when done */

#define LC_FIVE_DATA_BITS		0x00
#define LC_SIX_DATA_BITS		0x01
#define LC_SEVEN_DATA_BITS		0x02
#define LC_EIGHT_DATA_BITS		0x03

#define LC_ONE_STOP_BITS		0x00
#define LC_TWO_STOP_BITS		0x04

#define LC_NO_PARITY			0x00
#define LC_ODD_PARITY			0x08
#define LC_EVEN_PARITY			0x18

#define LC_STICK_PARITY_DISABLED	0x00	/* should be default */
#define LC_STICK_PARITY_ENABLED		0x20

#define LC_BREAK_DISABLED			0x00	/* should be default */
#define LC_BREAK_ENABLED			0x40

// Baud rate divisors. (Values of LSB register).
#define BD_1200		0x60
#define BD_2400		0x30
#define BD_4800		0x18
#define BD_9600		0x0C
#define BD_19200	0x06

// Default Capture settings
#define DEF_AUTOENDCAP_FLAG	1
#define DEF_BANNER_FLAG		0
#define DEF_COPIES_FLAG		1
#define DEF_DIRECT_FLAG		0
#define DEF_FORM_FEED_FLAG	0
#define DEF_PAPER_TYPE		0
#define DEF_NOTIFY_FLAG		0
#define DEF_TABS_SETTING	0	// Setting is no translation
#define DEF_TAB_SIZE		8	// Size is 8
#define DEF_HOLD_FLAG		0
#define DEF_WAIT_SETTING	10

// MAX and MIN capture/Print job value settings
#define MIN_COPIES			1
#define MAX_COPIES			250
#define MIN_TABS			0
#define MAX_TABS			30
#define MIN_PAPERTYPE		0
#define MAX_PAPERTYPE		10
#define MIN_WAIT			0
#define MAX_WAIT			3600

#define MAX_PRINT_JOB_ENTRIES		52
#define JOB_FILE_NAME_LENGTH		15

#define SERIAL_DEVICE		1		// Printer type in PRN_OBJ
#define PARALLEL_DEVICE		2		// Printer type in PRN_OBJ
#define QUEUE_DEVICE		3		// Printer type in PRN_OBJ

// Print job entry flags.  First NIBBLE are used as values
//#define PJE_ON_HOLD			0x01		// placed on hold
//#define PJE_HOLD_FOREVER	0x02		// hold job forever
//#define PJE_READY			0x04		// ready to print
//#define PJE_PRINTING		0x08		// currently printing
//#define PJE_CONTINUOUS		0x40
//#define PJE_FILLING			0x80

#define PJE_USER_HOLD		0x40		// placed on hold
#define PJE_OPERATOR_HOLD	0x80		// placed on hold
#define PJE_ON_HOLD			0x40		// placed on hold
#define PJE_HOLD_FOREVER	0x40		// hold job forever
#define PJE_READY			0x10		// ready to print
#define PJE_PRINTING		0x08		// currently printing
#define PJE_CONTINUOUS		0x04
#define PJE_FILLING			0x20

// VLMGetCaptureSettings PrFlags Bits
// 0x80 -- Banner on/off
// 0x40 -- Tabs translated by print server
// 0x20 -- Reserved
// 0x10 -- Reserved (Notify Flag!!!)
// 0x08 -- No form feed if set
// 0x04 -- Release print job if connection is interrupted (always set)
// 0x02 -- Reserved
// 0x01 -- Reserved

#define VLMF_CAP_BANNER		0x80	// Set in client record PrintFlags field
#define VLMF_TRANSLATE_TABS	0x40	// Set in client record PrintFlags field
#define VLMF_NOTIFY			0x10	// Set in client record PrintFlags field
#define VLMF_NO_FF			0x08	// Set in client record PrintFlags field
#define VLMF_RELEASE_JOB	0x04	// Autoendcap

#define VLMF_DIRECT			0x04
#define VLMF_USER_HOLD		0x40
#define VLMF_OPERATOR_HOLD	0x80

#define MAX_RESOURCE_RESTRICTIONS		10
#define MAX_SHARED_DEVICE_NAME_LENGTH	48	

// Used for PRN_OBJ.PrinterState field
#define PRINTER_STATE_READY			0x00
#define PRINTER_STATE_PRINTING		0x01
#define PRINTER_STATE_WAITING		0x02
#define PRINTER_STATE_NOT_DEFINED	0x04

#define PRINTER_STATE_ACTIVE		0x03

// Used for PRN_OBJ.PrinterState field (problems)
#define PRINTER_STATE_HALTED			0x10
#define PRINTER_STATE_WAITING_FOR_FORM	0x20
#define PRINTER_STATE_OFF_LINE			0x40
#define PRINTER_STATE_OUT_OF_PAPER		0x80

#define PRINTER_STATE_PROBLEM			0xF0	// Something blocking the printer

// Used for PRN_OBJ.OldJobFlag field
#define PRINTER_OLD_JOB_HOLD			0
#define PRINTER_OLD_JOB_PRINT			1
#define PRINTER_OLD_JOB_DELETE			2

// Used for PRN_OBJ.ConsoleTImeout field
// If zero, no show window, if -1 wait for user key, if # tick to wait
#define PRINTER_CONSOLE_NO_SHOW			0
#define PRINTER_CONSOLE_WAIT			0xFFFF

// Network Printer -- Printer Control constants
#define DEFAULT_CHARS_PER_SEC		16384

#define PRINTER_SERVICE_INTERVAL 	1	// This one and
#define PRINTER_CHARS_PER_SEC_CONST	18	// This one must equal 18 when multiplied
#define DEFAULT_CONSOLE_TIMEOUT		PRINTER_CONSOLE_WAIT	// PRN_OBJ.ConsoleTimeout
#define DEFAULT_OLD_JOB_FLAG		PRINTER_OLD_JOB_HOLD	// PRN_OBJ.OldJobFlag

// Network Printer definition ranges
#define MIN_CPS			1
#define MAX_CPS			65535
#define MIN_ERRORWAIT	10
#define MAX_ERRORWAIT	1200
#define DEFAULT_ERRORWAIT	30
#define MIN_DATABITS	7
#define MAX_DATABITS	8
#define MIN_STOPBITS	1
#define MAX_STOPBITS	2
#define MIN_CONSOLE_WAIT_TIME	0
#define MAX_CONSOLE_WAIT_TIME	300		// Five minutes


// Server Configuration Ranges
#define MIN_CONNECTIONS 		2
#define MAX_CONNECTIONS 		MAX_NETWORK_NODES
#define MIN_CLIENT_TASKS 		4
#define MAX_CLIENT_TASKS 		200
#define MIN_DIR_BUFFERS 		20
#define MAX_DIR_BUFFERS 		1000
#define MIN_NET_DIRS			0
#define MAX_NET_DIRS			20
#define MIN_NET_PRINTERS		0
#define MAX_NET_PRINTERS	 	3
#define MIN_PRINT_B_SIZE 		512
#define MAX_PRINT_B_SIZE		4096
#define MIN_RECEIVE_BUFFERS		3
#define MAX_RECEIVE_BUFFERS 	20
#define MIN_RECEIVE_B_SIZE		1024
#define MAX_RECEIVE_B_SIZE		8192
#define MIN_IO_BUFFERS			0
#define MAX_IO_BUFFERS			25
#define MIN_IO_BUF_MUL			2
#define MAX_IO_BUF_MUL			64
#define MIN_IO_B_SIZE			512
#define MAX_IO_B_SIZE			32768
#define MIN_SERVICE_EXTENSIONS	0
#define DEFAULT_SERVICE_EXTENSIONS	3
#define MAX_SERVICE_EXTENSIONS	10

// Audit Actions -- See Auditing - 6 in NCP manual for more
#define A_LOGIN 				21
#define A_LOGOUT 				23
#define A_SERVER_UP				50	// Used by the server
#define A_SERVER_DOWN			18	// Used by the server
#define A_USER_GIVEN			0xFFFF	// -1
#define A_SUPER_CHANGE_PASS		48
#define A_DELETE_AUDIT_LOG		7	// Huh?

// Operation Identifiers
//#define PASSWORD_OP 		1
//#define LOGIN_DATE_OP		2
//#define SECURITY_OP			3
#define CREATE_OP			4
#define DELETE_OP			5
//#define RENAME_OP			6
#define CHANGE_OP			7
//#define SELECT_OP			8

// Search Identifiers
#define FIND_BY_NAME		1
#define FIND_BY_VALUE		2

// VLM Defines
#define VLM_ID_VLM		0x0001
#define VLM_ID_CONN		0x0010
#define VLM_ID_TRAN		0x0020
#define VLM_ID_IPX		0x0021
#define VLM_ID_TCP		0x0022
#define VLM_ID_NWP		0x0030
#define VLM_ID_BIND		0x0031
#define VLM_ID_NDS		0x0032

#define VLM_ID_PNW		0x0033

#define VLM_ID_REDIR	0x0040
#define VLM_ID_FIO		0x0041
#define VLM_ID_PRINT	0x0042
#define VLM_ID_GENR		0x0043
#define VLM_ID_NETX		0x0050
#define VLM_ID_AUTO		0x0060
#define VLM_ID_NMR		0x0100

#define VLM_GEN			0x01
#define GEN_VER			0x00

#define VLM_STA			0x03

#define CONN_ALO		0x04
#define CONN_FRE		0x06
#define CONN_GET		0x07
#define CONN_SET		0x08
#define CONN_LUP		0x0A
#define CONN_NLUN		0x0D
#define CONN_NLUC		0x0E
#define CONN_GNUM		0x0F

#define TRAN_REQ		0x06

#define REDIR_APIS		0x08

#define CEI_NET_TYPE	0x01
#define CEI_AUTH		0x03
#define CEI_MAJ_MIN_VER	0x08
#define CEI_CONN_NUM	0x0D	// 13d
#define CEI_TRAN_BUFF	0x11	// 17d

#define NWP_CON			0x04
#define NWP_DIS			0x05
#define NWP_LIN			0x08
#define NWP_LOUT		0x09

#define NWP_SPEC		0x0A
#define		GS_MOD			0x02

#define NWP_PNW 		0x0D

#define NWP_MUX_DX		0x0F
#	define GET_PREF_NAME	0x00
#	define SET_PREF_NAME	0x01
#	define GET_PREF_CONN_ID	0x02
#	define GET_SERV_ADDR	0x03

#define PRINT_SGPD		0x04	// Set\Get Printer Data
#define PRINT_GPN		0x07	// Get Number of Printers
#define PRINT_GSR		0x08	// Get\Set print redirection

#define GENR_SPEC		0x06

// Swab a word
#define SwabWord(W)		( ((W) >> 8) | ((W) << 8) )

#define SwabLong(L)		( ((L) >> 24) | (((L) & 0x00FF0000L) >> 8) | (((L) & 0x0000FF00L) << 8) | ((L) << 24) )

#define VLM_LOADED		0x8000
#define PNW_LOADED		0x0001
#define BIND_LOADED		0x0002
#define NDS_LOADED		0x0004

// Resource Secondary Flags -- can be ORed together
#define RES_FLAGS_SECONDARY		0x01
#define RES_FLAGS_HIDDEN		0x02
#define RES_FLAGS_PRIVATE		0x80

#define ACTUAL_PATH_LENGTH	128
#define MOUNT_POINT_LENGTH	48

#define ENCRYPTED_PASSWORD_LENGTH	16

#define LOGIN_REST_TIME_LENGTH	42		// LoginRestrictionsTimes (USER OBJ)

// User Account flags (See USER_OBJ.AccoutnFlags)
#define USER_FLAG_SUPERVISOR		0x0001	// Supervisor privileges if set
#define USER_FLAG_ACCOUNT_DISABLED	0x0002	// Account is disabled if set
#define USER_FLAG_CHANGE_PASSWORD	0x0004	// User allowed to change pass if set
#define USER_FLAG_UNIQUE_PASSWORD	0x0008	// Password must be unique if set

#define USER_DEFAULT_ACCOUNT_FLAGS	(USER_FLAG_SUPERVISOR | USER_FLAG_CHANGE_PASSWORD)

// User account definition ranges
#define MIN_MINIMUM_PASS_LENGTH	1
#define MAX_MINIMUM_PASS_LENGTH	15
#define MIN_DAYS_BETW_CHANGES	1
#define MAX_DAYS_BETW_CHANGES	100

#define USER_DEFAULT_GRACE_LOGINS	5
#define DEFAULT_GRACE_LOGINS		USER_DEFAULT_GRACE_LOGINS

// Route flags (See ROUTE_OBJ.RouteFlag)
#define ROUTE_FLAG_HOME_NETWORK		0x0001

#define MAX_SERVER_MANAGERS	10

#define SERVER_MODULE_SECURITY		0x0001	// load security module
#define SERVER_MODULE_AUTH_FULL		0x0002	// load full authentication
#define SERVER_MODULE_AUTH_LOCAL	0x0004	// load local (mobile) authentication
#define SERVER_MODULE_SCREEN		0x0008	// load screen popup module
#define SERVER_MODULE_REPORT		0x0010	// load error/audit module
#define SERVER_MODULE_DIAGS			0x0020	// load diagnostics module
#define SERVER_MODULE_EXT			0x0040	// load extension services module
#define SERVER_MODULE_DOD			0x0080	// load DOD module
#define SERVER_MODULE_CONSOLE		0x0100	// load Console module -- Get Conn Open Files

#define SERVER_MODULE_DEFAULT		0x00FB		// all but AUTH_LOCAL
#define SERVER_MODULE_SALVAGE		0x0200		// load the salvage module

// Used for NWCopyControlFile	
#define COPY_NCP_TO_LOCAL	1
#define COPY_NCP_TO_NCP		0

// Used by Fixed Disk Security calls
#define FDS_IS_ENABLED			1
#define FDS_IS_DISABLED			2
#define FDS_PASSWORD_CHANGED	3

#define FDS_INVALID_PASSWORD	0xF9	// -7 -- Bad old password

// Used as one of ServerFlags
#define SERVER_FLAG_LOCAL_SECURITY	0x01	// SECURITY.SYS is loaded and enabled


// Local security rights masks
#define	LSB_NORIGHTS			0x0000	// 0
#define	LSB_COMPUTER_ENABLED	0x0001	// 1
#define	LSB_LPT1_ENABLED		0x0002	// 2
#define	LSB_LPT2_ENABLED		0x0004	// 4
#define	LSB_LPT3_ENABLED		0x0008	// 8
#define	LSB_FLOPPY_ENABLED		0x0010	// 16
#define	LSB_ALLRIGHTS			0x0FFF	// 0x1F

// NWCreateWorkgroup change flags masks
#define NWMOVE_RESOURCES      0x00000001
#define NWCOPY_ROUTES         0x00000002
#define NWRESET_USERS         0x00000004

// NWSetNetCfgInfo flags masks
#define NETCFG_WORKGROUPNAME		0x0001
//#define NETCFG_WORKGROUPIDHIGH		(NETCFG_WORKGROUPNAME << 1)
//#define NETCFG_WORKGROUPIDLOW		(NETCFG_WORKGROUPNAME << 2)
//#define NETCFG_WORKGROUPNAMEID		(NETCFG_WORKGROUPNAME | NETCFG_WORKGROUPIDLOW | NETCFG_WORKGROUPIDHIGH)
#define NETCFG_WORKGROUPNET			(NETCFG_WORKGROUPNAME << 1)
#define NETCFG_BROADCAST_RETRIES	(NETCFG_WORKGROUPNAME << 2)
#define NETCFG_BROADCAST_TIMEOUT 	(NETCFG_WORKGROUPNAME << 3)
#define NETCFG_BROADCAST_DELAY		(NETCFG_WORKGROUPNAME << 4)
#define NETCFG_ENDMASK				0x001f

// For System File Identification, index to sync stamp array in Server_t
#define USER_FILE_ID		0
#define WORKGROUP_FILE_ID	1
#define RESOURCE_FILE_ID	2


#define PNW_SEMAPHORE_LOCK		1				// To pass to NWLockLogicalRecord
#define PNW_SEMAPHORE_TIMEOUT	(30 * 18)	// To pass to NWLockLogicalRecord

// For SERVER_STATE.StateFlags
#define SERVER_STATE_FLAG_UP				0x00000001
#define SERVER_STATE_FLAG_LOCAL				0x00000002
#define SERVER_STATE_FLAG_ORPHAN			0x00000004
#define SERVER_STATE_FLAG_IN_CURRENT_WG		0x00000008


/****************************************************************************
	Structures
*****************************************************************************/

typedef struct IPXAddressStruct {
	nuint8	Network[4];		/* high-low */
	nuint8	Node[6];		/* high-low */
	nuint8	Socket[2];		/* high-low */
} IPX_ADDRESS;

// NWBuildServerList structs

typedef struct {
	nstr			ServerName[NW_SERVER_NAME_LENGTH];
	nuint16			ServerType;
	IPX_ADDRESS		ServerAddress;
	nuint8			ServerFlags;				// See SERVER_FLAG_*
	nuint32			ServerID;
	nuint16			ServerFreeConnCount;
	nuint32			SyncStamp[NUM_SYNC_STAMPS];	// See *_FILE_ID
} Server_t;

typedef struct {
	nstr		UserName[USER_NAME_LENGTH];
	nuint32		UserID;
	IPX_ADDRESS	UserAddress;
} User_t;

typedef struct {
	nstr		WorkgroupName[WORKGROUP_NAME_LENGTH];
	nuint32		WorkgroupID;
} Workgroup_t;

typedef struct {
	nstr	ServerName[SERVER_NAME_LENGTH];
	nstr	LogicalName[RESOURCE_NAME_LENGTH];
 	nuint8	ResourceType;
	nuint8	ResourceFlags;
	nuint16	ServerType;
} Resource_t;

typedef struct SetupRecordStruct {
	nstr 	Name[SETUP_NAME_LENGTH];
	nuint8 	Len;
	nuint8 	Value[SETUP_VALUE_LENGTH];
} SETUP_RECORD;

typedef struct {
	nstr	Name[FORM_NAME_LENGTH];
	nuint8	Number;
} FORM_RECORD;

typedef struct {
	nstr	Name[FORM_NAME_LENGTH];
	nuint8	Number;
	nuint8	Length;
	nuint8	Width;
} NW_FORM_RECORD;

// Audit structure
typedef struct {
	nstr	Date[6];		// year, month, day, hour, min, sec
	nuint32	ServiceCategory;
	nuint16	ClientType;
	nuint16	EntryType;
	nstr	Name[USER_NAME_LENGTH];
	// If user_enter type then length of given message
	nuint16	MessageLength;
} AUDIT_ENTRY;

typedef struct {
	nuint8	Banner;
	nuint8	Copies;
	nuint8	Direct;
	nuint8	Notify;
	nuint8	FormFeed;
	nuint8	Hold;
	nuint16	PaperType;	// FormNumber
	nstr	FormName[FORM_NAME_LENGTH];
	nuint8	Tabs;
	nuint8	AutoEndcap;
	nuint16	Wait;
	nstr	SetupStringName[SETUP_NAME_LENGTH];
	nuint32	JobFileSize;
} CAPTURE_SETTINGS;

typedef struct {
	nuint8	PFStatus;
	nuint8	PrFlags;
	nuint8	PTabSize;
	nuint8	PCopiesH;
	nuint8	PCopies;
	nuint8	PFormType;
	nuint8	Reserved;
	nstr	PBanner[BANNER_NAME_LENGTH];
	nuint8	PLPTNum;
	nuint16	PTimeOut;	// 0 - 65535, 0 means no timeout
	nuint8	PConcat;	// Autoendcap
	nuint16	PLines;		// Max lines per page (66)
	nuint16	PChars;		// Max chars per line (132)
	nstr	PFormName[13];
	nuint8	PLSTActive;
	nuint8	PFileFlag;
	nuint8	PDoingTimeOut;
	nuint32	PHeader;
	nuint32	PTail;
	nuint8	PServer;
	nuint8	PStarted;
	nuint8	PQSet;
	nuint8	PJobValid;
	nuint32	PQueueID;
	nuint16	PJobNum;
	nuint16	PBufferCount;
	nuint16	PJobNumHi;
} VLM_CAPTURE_SETTINGS;

typedef struct {
	nuint32	EPNameNDSID;
	nuint16	EPConnHandle;
	nstr	EPQueueName[65];
	nuint32	EPTargetServer;
	nuint8	EPTargetTime[6];
	nuint16	EPControl;		// Auto-start, restart, Entry open, User and operator hold
	nstr	EPBName[13];
	nstr	EPDescription[CAP_DESC_LENGTH];
	nuint32	EPReserved;
} VLM_EXT_CAPTURE_SETTINGS;

typedef struct {
	nuint16	BufferLen;
	nuint16	ValueLen;
	nuint8	Value[1];
} VLM_SETUP_RECORD;


// VLM Structs

//typedef struct {
//	nuint32 TotalJobs;
//	nuint32 JobsRetrieved;
//	nuint32 JobList[125];
//} QUEUE_LIST;
//
//typedef struct {
//	nuint16	RecordInUse;
//	nuint32	PreviousRecord;
//	nuint32	NextRecord;
//	nuint32	ClientStation;
//	nuint32	ClientTaskNumber;
//	nuint32	ClientIDNumber;
//	nuint32	TargetServerIDNumber;
//	nuint8	TargetExecutionTime[6];
//	nuint8	JobEntryTime[6];
//	nuint32	JobNumber;
//	nuint16	JobType;
//	nuint16	JobPosition;
//	nuint16	JobControlFlags;
//	nstr	JobFileName[14];
//	nuint32	JobFileHandle;
//	nuint32	ServerStation;
//	nuint32	ServerTaskNumber;
//	nuint32	ServerIDNumber;
//	nstr	TextJobDescription[50];
//	nuint8	ClientRecordArea[152];
//} PRINT_JOB_RECORD;
//
//typedef struct {
//	nuint8	ClientStation;
//	nuint8	ClientTaskNumber;
//	nuint32	ClientIDNumber;
//	nuint32	TargetServerIDNumber;
//	nuint8	TargetExecutionTime[6];
//	nuint8	JobEntryTime[6];
//	nuint16	JobNumber;
//	nuint16	JobType;
//	nuint8	JobPosition;
//	nuint8	JobControlFlags;
//	nstr	JobFileName[14];
//	nuint8	JobFileHandle[6];
//	nuint8	ServerStation;
//	nuint8	ServerTaskNumber;
//	nuint32	ServerIDNumber;
//	nstr	TextJobDescription[50];
//	nuint8	ClientRecordArea[152];
//} PRINT_JOB_RECORD_2;

typedef struct {
	nuint8	Version;
	nuint8	TabSize;
	nuint16	Copies;
	nuint16	PrintFlags;		// Byte swapped.
	// See VLMF_CAP_BANNER, VLMF_TRANSLATE_TABS, VLMF_NOTIFY, VLMF_NO_FF
	nuint16	PrinterLines;
	nuint16	PrinterColumns;
	nstr	FormName[13];
	nuint8	Reserved[9];
	nstr	BannerName[13];
	nstr	FileName[13];
	nstr	HeaderFile[14];
	nstr	FilePath[80];
} CLIENT_RECORD_AREA;


// PNW Objects

typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];
} PNW_OBJ;

typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];

	nstr	FullName[FULL_NAME_LENGTH];
	nuint16	AccountFlags;		// See USER_FLAG_*, Use USER_DEFAULT_ACCOUNT_FLAGS
	nuint8	MinPasswordLength;
	nuint8	MaxDaysBetweenPasswordChange;
	nuint8	LastPasswordChangeDay;
	nuint8	LastPasswordChangeMonth;
	nuint16	LastPasswordChangeYear;
	nuint8	GraceLoginsRemaining;		// Use USER_DEFAULT_GRACE_LOGINS
	nuint8	LoginRestrictionTimes[LOGIN_REST_TIME_LENGTH];	// FF's means allow
	nuint32	EncryptionSeed;
	nuint8	NewPasswordLength;
	nuint8	LastEncryptedPassword[ENCRYPTED_PASSWORD_LENGTH];
	nuint8	EncryptedPassword[ENCRYPTED_PASSWORD_LENGTH];
} USER_OBJ;


// Common fields between directories and printers

typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];

	nuint32	ServerID;
	nuint16	ResourceFlag;		// See RES_FLAGS_*
	nuint32	PrimaryServerID;
	nuint32	PrimaryResourceID;
} RES_OBJ;


typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];

	nuint32	ServerID;
	nuint16	ResourceFlag;		// See RES_FLAGS_*
	nuint32	PrimaryServerID;
	nuint32	PrimaryResourceID;
	nstr	ActualPath[ACTUAL_PATH_LENGTH];
	nstr	SuggestedMountPoint[MOUNT_POINT_LENGTH];
} DIR_OBJ;


typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved1;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];

	nuint32	ServerID;
	nuint16	ResourceFlag;		// See RES_FLAGS_*
	nuint32	PrimaryServerID;
	nuint32	PrimaryResourceID;
	nstr	DeviceName[PRINTER_DEVICE_NAME_LENGTH];

	// Printer file
	nuint16	Reserved2;			// Version stamp
	nuint8	ServiceInterval;
	nuint16	CharsPerSecond;
	nuint16	Reserved3;			// Printer flags??  What for?
	nuint16	ErrorWaitTime;		// Seconds
	nuint16	ConsoleTimeout;		// Pop-up box time out.  See PRINTER_CONSOLE_*
	nuint8	OldJobFlag;			// What to do with jobs in queue at boot (See PRINTER_OLD_JOB_*)
	nstr	SpoolDirectory[SPOOL_DIRECTORY_LENGTH];	// NULL if use default location
	nuint8	PrinterType;		// Read-Only -- 1 = COM, 2 = LPT, 3 = QUE
	nuint16	PrinterNumber;		// Read-Only -- 0 = LPT1/COM1 ...
	nuint8	LineControlValues;
	nuint8	BaudRateDivisor;
	nuint8	XonXoffFlag;
	nstr	StartFormName[FORM_NAME_LENGTH];	// For displaying.
	nuint8	StartFormNumber;	// For use by server.
	nuint8	Reserved6[12];		// Rprinter address
} PRN_OBJ;


typedef struct {
	nuint8	PrinterState;		// See PRINTER_STATE_*
	nstr	CurrentFormName[FORM_NAME_LENGTH];
	nuint8	CurrentFormNumber;
	nuint8	CurrentJobNumber;
	nuint16	CurrentCopies;
	nuint16	CharsLeftInBuffer;
	nuint32	CurrentSeekPosition;
	nuint32	CurrentJobSize;
} QUEUE_STATUS;

typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];	// Not used by routes

	nuint8	NetworkAddress[4];
	nuint8	NodeAddress[6];
	nuint16	RouteFlag;					// See ROUTE_FLAG_*
	nuint8	ReferenceCount;
} ROUTE_OBJ;


typedef struct {
	nuint32	UserID;
	nuint16	Rights;
} USER_RIGHTS;


typedef struct {
	nuint32	ObjectID;
	nuint8	ObjectType;
	nuint8	Reserved;
	nuint16	ReservedMore;
	nuint8	LastModifyDateTime[6];
	nuint32	LastModifyStamp;
	nstr	Name[OBJECT_NAME_LENGTH];

	nuint8	LastKnownAddress[10];
	nuint32	OwnerID;
	nuint16	WorkgroupManagerRights;
	nuint8	TotalManagers;
	USER_RIGHTS	ManagerArray[MAX_SERVER_MANAGERS];
	nuint8	HostType;			// 1=DOS

	nuint16	Reserved2;	// Version stamp
	nstr	ServerName[NW_SERVER_NAME_LENGTH];
	nstr	ServerDesc[SERVER_DESC_LENGTH];
	nuint16	MaxConnections;
	nuint16	MaxOpenFiles;
	nuint16	NumClientTasks;
	nuint16	NumDirBuffers;					// Default: 20
	nuint16	MaxServiceExtensions;			// Default: 3
	nuint16	MaxDirs;
	nuint16	MaxPrns;
	nuint16	SizePrintBuffers;
	nuint16	NumReceiveBuffers;
	nuint16	SizeReceiveBuffers;
	nuint16	NumIOBuffers;
	nuint16	SizeIOBuffers;
	nuint8	ServerLoadType;		// 0 = No load, 1 = Load Conventional
	nuint32	ModuleFlag;			// See SERVER_MODULE_*
	nuint32	ScheduleControl;
} SERVER_OBJ;

//	The following structures are part of GetServerStatus.  These are
//	returned from the server.

typedef struct {
	nuint16	StructVersion;
	nstr	WorkgroupName[WORKGROUP_NAME_LENGTH];
	nuint32	WorkgroupID;
	nstr	ServerName[SERVER_NAME_LENGTH];
	nuint32	ServerID;								// NEW
	nstr	OEMString[OEM_NAME_LENGTH];				// NEW
	nuint16	OEMID;									// NEW
	nuint8	MajorVersion;
	nuint8	MinorVersion;
	nuint8	BetaVersion;
	nuint8	Reserved[3];
//	nuint8	StartMinutes;
//	nuint8	StartHours;
//	nuint8	StartSeconds;
	nuint32	ServerUpSeconds;		// Seconds the server has been up
//	nuint16	StartYear;				// Relative to 1980
//	nuint8	StartDay;
//	nuint8	StartMonth;
	nuint8	ServerAddressType;		// 0=IPX		// NEW
	nuint8	ServerNetAddr[4];
	nuint8	ServerNodeAddr[6];
	nuint8	ServerSocket[2];
	nstr	ServerDesc[SERVER_DESC_LENGTH];
	nuint16	CfgConnections;
	nuint16	CfgOpenFiles;
	nuint16	CfgClientTasks;
	nuint16	CfgDirBuffers;
	nuint16	CfgServiceExtensions;					// NEW
	nuint16	CurServiceExtensions;					// NEW
	nuint16	CfgResources;
	nuint16	CurResources;
	nuint16	CfgDirs;				// Renamed
	nuint16	CurDirs;				// Renamed
	nuint16	CfgPrns;				// Renamed
	nuint8	CurPrns;				// Renamed
	nuint16	SizePrintBuffers;
	nuint16	NumReceiveBuffers;
	nuint16	RealSizeReceiveBuffers;
	nuint16	SizeReceiveBuffers;
	nuint16	NumIOBuffers;	
	nuint16	SizeIOBuffers;
	nuint16	ServerTSRSize;
	nuint8	AuditingFlag;			// Zero/non-zero		// Renamed
	nuint8	ServerFlags;			// See SERVER_FLAG_*
	nuint8	OperatingSystem;		// 1=DOS
	nuint8	DosMajorVersion;
	nuint8	DosMinorVersion;
	nuint8	IsShareLoaded;			// Zero/non-zero
	nuint8	ServerLoadType;			// 0 = No load, 1 = Load Conventional
	nuint32	ModuleFlag;				// See SERVER_MODULE_*		// NEW
	nuint32	ScheduleControl;
	nuint32	ServerDiskVector;		// Bit 0 = drive1 which is A
	nuint32	RemovableDiskVector;
} SERVER_STATUS;

//typedef	struct StatsStructure {
typedef	struct {
	nuint32	TotalPacketsReceived;
	nuint16	BadPackets;
	nuint16	PacketsNotProcessedImmediately;
	nuint16	LostResponses;
	nuint16	PeakConnectionsUsed;
	nuint16	PeakOpenFiles;
	nuint16	PeakClientTasks;
	nuint16	WatchdogPacketsSent;
	nuint16	ClientsWatchdogged;
	nuint16	ECBRepostsWithNoBuffers;
	nuint16	SendPacketWithECBActive;
	nuint16	TotalSlistRequests;
	nuint16	ServerBusyPackets;
	nuint16	ServerBusyPacketsNoBuffers;
	nuint16	UnknownRequests;
	nuint16	WriteBehindMisses;
	nuint32	IOCacheHits;				// Renamed
	nuint32	IOCacheMisses;				// Renamed
	nuint16	ReadsTooLarge;
	nuint16	CriticalErrors;
	nuint32	SavedLargeDOSArea;
	nuint32	SavedSmallDOSArea;
	nuint16	StarvationCounter;
	nuint16	WriteBehindHits;
} SERVER_STATS1;

typedef struct {
	nuint16	CacheBlocksInUse;
	nuint32	SavedNoDosArea;				// Renamed
	nuint32	PacketQueueRuns;
	nuint32	IdleLoopWaitHits;
	nuint32	Int21NotOurs;
	nuint32	TotalInt21Calls;
	nuint16	PasswordFailures;
} SERVER_STATS2;

//typedef struct CurrentStateStruct {
typedef	struct {
	nuint16	CurConnections;
	nuint16	CurOpenFiles;
	nuint16	CurClientTasks;
	nuint16	CurNumFreeBuffers;
	nuint16	CurNumSemaphores;
} CURRENT_STATE;

#define SERVER_STATUS_SIZE (sizeof (SERVER_STATUS) + sizeof (SERVER_STATS1) + sizeof (SERVER_STATS2) + sizeof (CURRENT_STATE))

typedef struct {
   nuint8	Flag;
   nuint8	Address[10];
} TDS_ROUTE;


typedef struct {
	nstr		WorkgroupName[WORKGROUP_NAME_LENGTH];
	nuint32		WorkgroupID;
	IPX_ADDRESS WorkgroupNet;
	nuint8			BroadcastRetries;
	nuint8			BroadcastTimeout;
	nuint8			SendDelay;
} CFG_INFO;

typedef struct {
	nstr		ServerName[SERVER_NAME_LENGTH];
	nstr		ServiceName[SERVICE_NAME_LENGTH];
	nuint8		ServiceInfo[SERVICE_INFO_LENGTH];
 	nuint16		ServiceVersion;
	nuint16		ServiceToken;
} Service_t;

typedef struct {
	nuint16		ConnHand;
	nuint8		NewAttach;
	Service_t	ServiceInfo;
} OpenService_t;

// FIX -- TEMP until conversion is complete
typedef struct {
	nuint8	address[6];
} NodeAddress;

//typedef struct {
//	nuint8 Network[4];
//	nuint8 Ticks;
//} ENTERPRISE_NETWORK;

typedef struct {
	nuint8	Network[4];
	nuint16	Hops;
	nuint16	Ticks;
} ENTERPRISE_NETWORK;

typedef struct {
	nstr	WorkgroupName[WORKGROUP_NAME_LENGTH];
	nuint32	WorkgroupID;
	nuint8	Network[4];
	nuint8	Ticks;
} ENTERPRISE_WORKGROUP;

// Note: This struct must parallel the TRUSTEE_INFO struct in NWDIRECT.H
typedef struct {
  nuint32	UserID;
  nuint16	AccessMask;
} EXCEPTION_INFO;

typedef struct {
	nstr	ServerDBName[SERVER_NAME_LENGTH];
	nuint32	ServerID;
	nstr	WorkgroupName[WORKGROUP_NAME_LENGTH];
	nuint32	WorkgroupID;
	nuint32	StateFlags;			// See SERVER_STATE_FLAG_*
} SERVER_STATE;


/****************************************************************************
	Global variables
*****************************************************************************/

// Reserved string defines
//	Use these if you are in windows or whatever.

#define CONTROL_DIR_NAME_DEF		"NWCNTL"
#define BROWSE_RESOURCE_NAME_DEF	"NWCNTL"

#define USER_FILE_NAME_DEF			"\\USER"
#define RESOURCE_FILE_NAME_DEF	"\\RESOURCE"
#define WORKGROUP_FILE_NAME_DEF	"\\WORKGRP"

#define SERVER_FILE_NAME_DEF		"\\SERVER"
#define SECURITY_FILE_NAME_DEF	"\\SECURITY"
#define AUDIT_FILE_NAME_DEF		"\\AUDIT"
#define ERROR_FILE_NAME_DEF		"\\ERROR"

#define PRINTER_FILE_NAME_DEF		"\\PRINTER"
#define SPOOLER_FILE_NAME_DEF		"\\SPOOLER"
#define SETUP_FILE_NAME_DEF		"\\SETUP"

#define HANDLE_FILE_NAME_DEF		"\\HANDLE"

#define DEFAULT_SETUP_NAME_DEF	"DEFAULT"
#define DEFAULT_FORM_NAME_DEF		"DEFAULT"
#define LPT_NAME_DEF					"LPT"
#define COM_NAME_DEF					"COM"
#define LOCAL_SECURITY_NAME_DEF	"LOCAL_MACHINE_HARDWARE"

#define EMAIL_DIR_NAME_DEF			"EMAIL"
#define SUPERVISOR_USER_NAME_DEF	"SUPERVISOR"
#define TEMPLATE_USER_NAME_DEF	"TEMPLATE"	// Where default account info is stored

#define MOVE_RESOURCE_NAME_DEF	"\\MOVERES"


// Use these if you are in DOS.  These will not be linked in windows because
// they are in the dll. -mgb

N_EXTERN_VAR nstr LOCAL_CONTROL_PATH[];
N_EXTERN_VAR nstr CONTROL_DIR_NAME[];
N_EXTERN_VAR nstr BROWSE_RESOURCE_NAME[];

N_EXTERN_VAR nstr USER_FILE_NAME[];
N_EXTERN_VAR nstr RESOURCE_FILE_NAME[];
N_EXTERN_VAR nstr WORKGROUP_FILE_NAME[];

N_EXTERN_VAR nstr SERVER_FILE_NAME[];
N_EXTERN_VAR nstr SECURITY_FILE_NAME[];
N_EXTERN_VAR nstr AUDIT_FILE_NAME[];
N_EXTERN_VAR nstr ERROR_FILE_NAME[];
N_EXTERN_VAR nstr HANDLE_FILE_NAME[];

N_EXTERN_VAR nstr PRINTER_FILE_NAME[];
N_EXTERN_VAR nstr SPOOLER_FILE_NAME[];
N_EXTERN_VAR nstr SETUP_FILE_NAME[];

N_EXTERN_VAR nstr DEFAULT_SETUP_NAME[];
N_EXTERN_VAR nstr DEFAULT_FORM_NAME[];
N_EXTERN_VAR nstr LPT_NAME[];
N_EXTERN_VAR nstr COM_NAME[];
N_EXTERN_VAR nstr LOCAL_SECURITY_NAME[];
N_EXTERN_VAR nstr EMAIL_DIR_NAME[];
N_EXTERN_VAR nstr SUPERVISOR_USER_NAME[];
N_EXTERN_VAR nstr TEMPLATE_USER_NAME[];
N_EXTERN_VAR nstr MOVE_RESOURCE_NAME[];

N_EXTERN_VAR nstr ErrorServerName[];


/****************************************************************************
	Prototypes
*****************************************************************************/


_N_EXTERN void NWAPI NWGetNWPNWVersion (					// PNWVER.CX
   pnuint8 majorVersion,
   pnuint8 minorVersion,
   pnuint8 revisionLevel,
   pnuint8 betaReleaseLevel);


// Auditing stuff -----------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGatherWorkgroupLog (			// PNWAUDIT.C
	pnstr filePath,
	nuint8 logID );

_N_EXTERN nuint16 NWAPI NWDeleteWorkgroupLog (			// PNWAUDIT.C
	nuint8 logID );


// Client stuff -------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetReceiveFlags (				// PNWCLNT.C
	nuint8 N_FAR *status,
	nuint16 N_FAR *duration );

_N_EXTERN nuint16 NWAPI NWSetReceiveFlags (				// PNWCLNT.C
	nuint8 status,
	nuint16 duration );

_N_EXTERN nuint16 NWAPI NWSendMessage (					// PNWCLNT.C
	IPX_ADDRESS N_FAR *address, 
	pnstr message );


// Config file stuff --------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWSetNetCfgInfo (					// CONFIG.C
	pnstr cfgFile,
	CFG_INFO N_FAR *cfgInfo, 
	nuint16 mask);


// Connection stuff ---------------------------------------------

_N_EXTERN nuint16 NWAPI NWAttachIfNeeded (				// ATIFNEED.C
	pnstr serverName, 
	nuint16 N_FAR *connHand, 
	nuint8 N_FAR *detachFlag, 
	nuint16 okToSpecialAttach );

_N_EXTERN nuint16 NWAPI NWDetachConnection (				// DETCONN.C
	nuint16 connHand );

_N_EXTERN nuint16 NWAPI NWLogoutPNW (						// LGOUTPNW.C
	void );

_N_EXTERN nuint16 NWAPI NWGetLoginServer (				// GTLOGSVR.C
	nuint8 otherWorkgroup,
	pnstr serverName );

_N_EXTERN nuint16 NWAPI NWLoginConnection (				// LOGICONN.C
	nuint16 connHand,
	pnstr userName,
	pnstr password );

_N_EXTERN nuint16 NWAPI NWGetPrefConnInfo (				// PNWPREF.C
	nuint8 N_FAR *infoValid,
	nuint8 N_FAR *driveLetter,
	pnstr serverName,
	pnstr resourceName );

_N_EXTERN nuint16 NWAPI NWIsPrefConnInfoValid (			// PNWPREF.C
	void );

_N_EXTERN nuint16 NWAPI NWMapPrefDrive (					// PNWPREF.C
	void );

_N_EXTERN nuint16 NWAPI NWVerifyUserPassword (			// VERIPASS.C
	nuint16 connHand,
	pnstr password );


// Copy stuff ---------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWCopyControlFile (				// PNWCOPY.C
	pnstr sourceServerName,
	pnstr targetServerName,
	nuint8 controlFileID,
	nuint16 copyDirection );

_N_EXTERN nuint16 NWAPI NWBackupSystemFiles (			// BACKREST.C
	nuint16 connHand, 
	pnstr serverName, 
	pnstr targetPath);

_N_EXTERN nuint16 NWAPI NWRestoreSystemFiles (			// BACKREST.C
	nuint16 connHand, 
	pnstr serverName,
	pnstr sourcePath);


// Dos stuff ----------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWCancelPNWRedirection (		// CANPNRED.C
	pnstr deviceStr );


// Enterprise workgroup stuff -----------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetEnterpriseWorkgroups (	// GETENTWG.C
	pnstr workgroupName,
	ENTERPRISE_NETWORK N_FAR *networkBuffer,
	nuint16 numNetworks,
	nuint16 N_FAR *index,
	nuint16 N_FAR *numWorkgroups,
	ENTERPRISE_WORKGROUP N_FAR *workgroupBuffer );

_N_EXTERN nuint16 NWAPI NWSetEnterpriseNetwork (		// STENTNET.C
	ENTERPRISE_WORKGROUP N_FAR *workgroupInfo,
	nuint8 setNetCfg );


// Form stuff -----------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWCreatePrinterForm ( 			// PNWFORM.C
	nuint16 connHand,
	nuint32 queueID,
	FORM_RECORD N_FAR *formRecord );

_N_EXTERN nuint16 NWAPI NWDeletePrinterForm (			// DELFORM.C
	nuint16 connHand,
	nuint32 queueID,
	nuint8 formNumber );

_N_EXTERN nuint16 NWAPI NWScanFormInfo (					// SCANFORM.C
	nuint16 connHand,
	nuint32 queueID,
	nuint16 N_FAR *index,
	FORM_RECORD N_FAR *formRecord );

_N_EXTERN nuint16 NWAPI NWScanNWFormInfo (				// NWNWFORM.C
	nuint16 connHand,
	nuint16 N_FAR *index,
	NW_FORM_RECORD N_FAR *formRecord );


// Object stuff -------------------------------------------------------------
_N_EXTERN nuint16 NWAPI NWCreatePNWObject (				// PNWOBJ.C
	nuint32 serverID, 
	pnstr objectName,
	pnstr buffer,
	nuint8 objectType);

_N_EXTERN nuint16 NWAPI NWDeletePNWObject (				// PNWOBJ.C
	nuint32 objectID,
	nuint8 objectType);

_N_EXTERN nuint16 NWAPI NWModifyPNWObject (				// PNWOBJ.C
	nuint8 N_FAR *object,
	nuint8 objectType);

_N_EXTERN nuint16 NWAPI NWRenamePNWObject (				// PNWOBJ.C
	nuint32 objectID,
	nuint8 objectType,
	pnstr newObjectName);

_N_EXTERN nuint16 NWAPI NWScanPNWObject (					// PNWOBJ.C
	nuint16 connHand,
	nuint8 objectType,
	nuint32 N_FAR *index,
	nuint8 N_FAR *object );


// Queue stuff --------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetQueueStatus (				// GETQSTAT.C
	nuint16 connHand, 
	nuint32 queueID, 
	QUEUE_STATUS N_FAR *queueStatus );

_N_EXTERN nuint16 NWAPI NWSetQueueStatus (				// SETQSTAT.C
	nuint16 connHand, 
	nuint32 queueID, 
	QUEUE_STATUS N_FAR *queueStatus );

_N_EXTERN void NWAPI NWXlateQMSToNCP (						// COPYQMS.C
	nuint32 QMSFileHand,
	nuint8 N_FAR *ncpFileHand );

_N_EXTERN void NWAPI NWXlateNCPToQMS (						// COPYQMS.C
	nuint8 N_FAR *ncpFileHand,
	nuint32 N_FAR *QMSFileHand );


// Resource List stuff ------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWBuildResourceList (			// PNWRLIST.C
	nuint16 resourceType,
	nuint8 includeBigNWResources );			// This parameter is going away

_N_EXTERN nuint16 NWAPI NWGetResource (					// PNWRLIST.C
	nuint16 N_FAR *index, 
	Resource_t N_FAR * N_FAR *resourceInfo );

_N_EXTERN void NWAPI NWFreeResourceList (					// PNWRLIST.C
	void );

_N_EXTERN nuint16 NWAPI NWPingResourceInfo (				// PNWRLIST.C
	pnstr resourceName, 
	pnstr serverName, 
	nuint8 resourceType,
	nuint8 N_FAR *detectMultiple,
	Resource_t N_FAR *resourceInfo );


// Rights stuff -------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetServerManagerRights (		// GTSMRTS.C
	pnstr serverName,
	nuint32 userID,
	nuint16 N_FAR *rights );


// Route stuff --------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetNetworksByRange	(			// NETRANGE.C
	ENTERPRISE_NETWORK N_FAR * routesFound,
	nuint16 N_FAR * routesFoundCount,
	nuint8 N_FAR *destAddr,
	nuint8 minHops,
	nuint8 maxHops );


// Server stuff -------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWCreateWorkgroup (				// CREATWG.C
	nuint8 N_FAR *workgroupName,
	nuint32 changeFlags );

_N_EXTERN nuint16 NWAPI NWChangeClientWorkgroup (		// CHANGWG.C
	nuint8 N_FAR *workgroupName,
	nuint32 workgroupID );

_N_EXTERN nuint16 NWAPI NWJoinClientWorkgroup (			// CHANGWG.C
	nuint8 N_FAR *routeAdded );

_N_EXTERN nuint16 NWAPI NWChangeServerWorkgroup (		// CHANGWG.C
	pnstr workgroupName,
	nuint32 workgroupID );

_N_EXTERN nuint16 NWAPI NWJoinServerWorkgroup (			// CHANGWG.C
	nuint32	changeFlags );


_N_EXTERN nuint16 NWAPI NWGetLocalServerName (			// GTLOCSN.C
	pnstr serverName,
	nuint32 N_FAR *serverID );

_N_EXTERN nuint16 NWAPI NWGetLocalServerInfo (			// GTLCSVRI.C
	SERVER_STATUS N_FAR *serverStatus,
	nuint8 N_FAR *serverIsUp );

_N_EXTERN nuint16 NWAPI NWGetServerStatus (				// GTSVSTAT.C
	nuint16 connHand,
	nuint8 N_FAR *serverStatus );	// Buffer of size SERVER_STATUS_SIZE

_N_EXTERN nuint16 NWAPI NWGetLocalServerConfig (		// GTSVCNFG.C
	SERVER_OBJ N_FAR *serverConfig );

_N_EXTERN nuint16 NWAPI NWSetLocalServerConfig (		// STSVCNFG.C
	SERVER_OBJ N_FAR *serverConfig );

_N_EXTERN nuint16 NWAPI NWSetWatchDog (					// SETWDOG.C
	nuint16 connHand,
	nuint32 minutes );

_N_EXTERN nuint16 NWAPI NWSetBrowseResource (			// STBRORES.C
	nuint16 connHand,
	pnstr drive );

_N_EXTERN nuint16 NWAPI NWGetServerState (				// GTSTATE.C
	pnstr serverName,
	SERVER_STATE N_FAR *serverState );


// Server List stuff --------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWBuildServerList (				// PNWSLIST.C
	nuint8 includeBigNWServers );

_N_EXTERN void NWAPI NWFreeServerList (					// PNWSLIST.C
	void );

_N_EXTERN nuint16 NWAPI NWGetServer (						// PNWSLIST.C
	nuint16 N_FAR *index, 
	Server_t N_FAR * N_FAR *serverInfo );


// Service Extension stuff --------------------------------------------------

_N_EXTERN nuint16 NWAPI NWServiceRequest (				// PNWSLIST.C
	nuint16 connHand, 
	nuint16 serviceToken,
	nuint16 serviceFunction,
	nuint8 N_FAR *reqBuffer,
	nuint16 reqLength,
	nuint8 N_FAR *respBuffer, 
	nuint16 respLength );


// Service Extension List stuff ---------------------------------------------

_N_EXTERN nuint16 NWAPI NWBuildServiceList (				// PNWXLIST.C
	void );

_N_EXTERN void NWAPI NWFreeServiceList (					// PNWXLIST.C
	void );

_N_EXTERN nuint16 NWAPI NWGetServiceListEntry (			// PNWXLIST.C
	nuint16 N_FAR *index, 
	Service_t N_FAR * N_FAR *serviceInfo );


// Setup Record stuff -------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWCreateSetupString ( 			// PNWSETUP.C
	nuint16 connHand,
	nuint32 queueID,
	SETUP_RECORD N_FAR *setupRecord );

_N_EXTERN nuint16 NWAPI NWDeleteSetupString (			// DELSETUP.C
	nuint16 connHand,
	nuint32 queueID,
	pnstr setupName );

_N_EXTERN nuint16 NWAPI NWRenameSetupString (			// RENSETUP.C
	nuint16 connHand,
	nuint32 queueID,
	pnstr setupName, 
	pnstr newSetupName );

_N_EXTERN nuint16 NWAPI NWModifySetupString (			// MODSETUP.C
	nuint16 connHand,
	nuint32 queueID,
	SETUP_RECORD N_FAR *newSetupRecord );

_N_EXTERN nuint16 NWAPI NWScanSetupInfo (					// SCANSETU.C
	nuint16 connHand,
	nuint32 queueID,
	nuint16 N_FAR *index,
	SETUP_RECORD N_FAR *setupRecord );

_N_EXTERN nuint16 NWAPI NWGetSetupStringByName (		// GTSSBNAM.C
	nuint16 connHand,
	nuint32 queueID,
	SETUP_RECORD N_FAR *setupRecord );

_N_EXTERN nuint16 NWAPI NWGetSetupStringByValue (		// GTSSBVAL.C
	nuint16 connHand,
	nuint32 queueID,
	SETUP_RECORD N_FAR *setupRecord );


// Synchronization stuff ----------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetMajorityServer (			// PNWSYNC.C
	nuint16 systemFileID,
	pnstr majorityServerName, 
	nuint8 N_FAR *needsSync );

_N_EXTERN nuint16 NWAPI NWSyncIfNeeded (					// PNWSYNC.C
	nuint16 systemFileID, 
	pnstr majorityServerName,
	nuint8 N_FAR *fileSynced );


// TDS stuff ----------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWGetLocalUserInfo (				// GTLOCUSR.C
	pnstr UserName, 
	nuint32 N_FAR *UserID );

_N_EXTERN nuint16 NWAPI NWGetLocalWorkgroupInfo (		// GTLOCWGI.C
	pnstr workgroupName,
	nuint32 N_FAR *workgroupID );

_N_EXTERN nuint16 NWAPI NWGetBroadcastSettings (		// GTBCSET.C
	nuint8 N_FAR *retries,
	nuint8 N_FAR *timeout, 
	nuint8 N_FAR *sendDelay );

_N_EXTERN nuint16 NWAPI NWSetBroadcastSettings (		// STBCSET.C
	nuint8 retries,
	nuint8 timeout, 
	nuint8 sendDelay );

_N_EXTERN nuint16 NWAPI NWGetRoutes (						// PNWTDS.C
	TDS_ROUTE N_FAR *route );

_N_EXTERN nuint16 NWAPI NWInvalidateUserInfo (			// INVUSER.C
	void );


// User DB stuff ------------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWChangePassword (				// CHGPASS.C
	nuint32 userID,
	pnstr oldPassword ,
	pnstr newPassword );


// User List stuff ----------------------------------------------------------

_N_EXTERN nuint16 NWAPI NWBuildUserList (					// PNWULIST.C
	void );

_N_EXTERN void NWAPI NWFreeUserList (						// PNWULIST.C
	void );

_N_EXTERN nuint16 NWAPI NWGetUser (							// PNWULIST.C
	nuint16 N_FAR *index, 
	User_t N_FAR * N_FAR *userInfo );


// Workgroup List stuff -----------------------------------------------------

_N_EXTERN nuint16 NWAPI NWBuildWorkgroupList (			// WGLIST.C
	void );

_N_EXTERN nuint16 NWAPI NWGetWorkgroupListEntry (		// WGLIST.C
	nuint16 N_FAR *index, 
	Workgroup_t N_FAR * N_FAR *workgroupInfo );

_N_EXTERN void NWAPI NWFreeWorkgroupList (				// WGLIST.C
	void );


#endif

