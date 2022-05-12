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
//บ Source module name: NWPS_ERR.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_ERR_INC
#define NWPS_ERR_INC
/*=============== Print server communication errors =======================*/
#define NWPSE_SUCCESSFUL					  (NWCCODE)0x0000 /*  0 PServer Comm*/
#define NWPSE_NO_AVAILABLE_SPX_CONNECTI  (NWCCODE)0x0040 /* 64 PServer Comm*/
#define NWPSE_SPX_NOT_INITIALIZED		  (NWCCODE)0x0041 /* 65 PServer Comm*/
#define NWPSE_NO_SUCH_PSERVER            (NWCCODE)0x0042 /* 66 PServer Comm*/
#define NWPSE_UNABLE_TO_GET_SERVER_ADDR  (NWCCODE)0x0043 /* 67 PServer Comm*/
#define NWPSE_UNABLE_TO_CONNECT_TO_SERV  (NWCCODE)0x0044 /* 68 PServer Comm*/
#define NWPSE_NO_AVAILABLE_IPX_SOCKETS   (NWCCODE)0x0045 /* 69 PServer Comm*/
#define NWPSE_ALREADY_ATTACH_TO_A_PRINT  (NWCCODE)0x0046 /* 70 PServer Comm*/
#define NWPSE_IPX_NOT_INITIALIZED		  (NWCCODE)0x0047 /* 71 PServer Comm*/
#define NWPSE_CONNECTION_TERMINATED      (NWCCODE)0x00ED /*237 PServer Comm*/

/*=============== Print server error codes ================================*/
#define NWPSE_TOO_MANY_NW_SERVERS		  (NWCCODE)0x0101 /*  257 PServer	*/
#define NWPSE_UNKNOWN_NW_SERVER			  (NWCCODE)0x0102 /*  258 PServer	*/
#define NWPSE_BINDERY_LOCKED				  (NWCCODE)0x0103 /*  259 PServer	*/
#define NWPSE_NW_SERVER_MAXED_OUT		  (NWCCODE)0x0104 /*  260 PServer	*/
#define NWPSE_NO_RESPONSE					  (NWCCODE)0x0105 /*  261 PServer	*/
#define NWPSE_ALREADY_ATTACHED			  (NWCCODE)0x0106 /*  262 PServer	*/
#define NWPSE_CANT_ATTACH					  (NWCCODE)0x0107 /*  263 PServer	*/
#define NWPSE_NO_ACCOUNT_BALANCE         (NWCCODE)0x0108 /*  264 PServer	*/
#define NWPSE_NO_CREDIT_LEFT				  (NWCCODE)0x0109 /*  265 PServer	*/
#define NWPSE_INTRUDER_DETECTION_LOCK	  (NWCCODE)0x010A /*  266 PServer	*/
#define NWPSE_TOO_MANY_CONNECTIONS		  (NWCCODE)0x010B /*  267 PServer	*/
#define NWPSE_ACCOUNT_DISABLED			  (NWCCODE)0x010C /*  268 PServer	*/
#define NWPSE_UNAUTHORIZED_TIME			  (NWCCODE)0x010D /*  269 PServer	*/
#define NWPSE_UNAUTHORIZED_STATION		  (NWCCODE)0x010E /*  270 PServer	*/
#define NWPSE_NO_MORE_GRACE				  (NWCCODE)0x010F /*  271 PServer	*/
#define NWPSE_LOGIN_DISABLED				  (NWCCODE)0x0110 /*  272 PServer	*/
#define NWPSE_ILLEGAL_ACCT_NAME			  (NWCCODE)0x0111 /*  273 PServer	*/
#define NWPSE_PASSWORD_HAS_EXPIRED		  (NWCCODE)0x0112 /*  274 PServer	*/
#define NWPSE_ACCESS_DENIED				  (NWCCODE)0x0113 /*  275 PServer	*/
#define NWPSE_CANT_LOGIN					  (NWCCODE)0x0114 /*  276 PServer	*/
#define NWPSE_PRINTER_ALREADY_INSTALLED  (NWCCODE)0x0115 /*  277 PServer	*/
#define NWPSE_CANT_OPEN_CONFIG_FILE      (NWCCODE)0x0116 /*  278 PServer	*/
#define NWPSE_CANT_READ_CONFIG_FILE      (NWCCODE)0x0117 /*  279 PServer	*/
#define NWPSE_UNKNOWN_PRINTER_TYPE		  (NWCCODE)0x0118 /*  280 PServer	*/
#define NWPSE_MAX_FORMS_EXCEDED			  (NWCCODE)0x0119 /*  281 PServer	*/
#define NWPSE_NO_SUCH_JOB					  (NWCCODE)0x011A /*  282 PServer	*/
#define NWPSE_UNKNOWN_PRINTER_ERROR      (NWCCODE)0x011B /*  283 PServer	*/
#define NWPSE_COMMUNICATIONS_ERROR		  (NWCCODE)0x011C /*  284 PServer	*/
#define NWPSE_MODE_NOT_SUPPORTED         (NWCCODE)0x011D /*  285 PServer	*/

/*================= Print server error codes ==============================*/
#define NWPSE_NO_SUCH_QUEUE				  (NWCCODE)0x0200 /*  512 PServer	*/
#define NWPSE_NOT_AUTHORIZED_FOR_QUEUE   (NWCCODE)0x0201 /*  513 PServer	*/
#define NWPSE_QUEUE_HALTED               (NWCCODE)0x0202 /*  514 PServer	*/
#define NWPSE_UNABLE_TO_ATTACH_TO_QUEUE  (NWCCODE)0x0203 /*  515 PServer	*/
#define NWPSE_TOO_MANY_QUEUE_SERVERS	  (NWCCODE)0x0204 /*  516 PServer	*/

/*================= Print server error codes ==============================*/
#define NWPSE_INVALID_REQUEST            (NWCCODE)0x0300 /*  768 PServer	*/
#define NWPSE_NOT_ENOUGH_MEMORY			  (NWCCODE)0x0301 /*  769 PServer	*/
#define NWPSE_NO_SUCH_PRINTER            (NWCCODE)0x0302 /*  770 PServer	*/
#define NWPSE_INVALID_PARAMETER			  (NWCCODE)0x0303 /*  771 PServer	*/
#define NWPSE_PRINTER_BUSY               (NWCCODE)0x0304 /*  772 PServer	*/
#define NWPSE_CANT_DETACH_PRIMARY_SERVE  (NWCCODE)0x0305 /*  773 PServer	*/
#define NWPSE_GOING_DOWN					  (NWCCODE)0x0306 /*  774 PServer	*/
#define NWPSE_NOT_CONNECTED				  (NWCCODE)0x0307 /*  775 PServer	*/
#define NWPSE_ALREADY_IN_USE				  (NWCCODE)0x0308 /*  776 PServer	*/
#define NWPSE_NO_JOB_ACTIVE				  (NWCCODE)0x0309 /*  777 PServer	*/
#define NWPSE_NOT_ATTACHED_TO_SERVER	  (NWCCODE)0x030A /*  778 PServer	*/
#define NWPSE_ALREADY_IN_LIST            (NWCCODE)0x030B /*  779 PServer	*/
#define NWPSE_DOWN							  (NWCCODE)0x030C /*  780 PServer	*/
#define NWPSE_NOT_IN_LIST					  (NWCCODE)0x030D /*  781 PServer	*/
#define NWPSE_NO_RIGHTS                  (NWCCODE)0x030E /*  782 PServer	*/
#define NWPSE_CMD_NOT_SUPPORTED			  (NWCCODE)0x030F /*  783 PServer	*/
#define NWPSE_PASSWORD_UNAVAILABLE       (NWCCODE)0x0310 /*  784 PServer	*/
#define NWPSE_ILLEGAL_NOTIFY_OBJECT      (NWCCODE)0x0311 /*  785 PServer	*/

/*================= Print server error codes ==============================*/
#define NWPSE_UNABLE_TO_VERIFY_IDENTITY  (NWCCODE)0x0400 /* 1024 PServer	*/
#define NWPSE_NOT_REMOTE_PRINTER         (NWCCODE)0x0401 /* 1025 PServer	*/
#define NWPSE_UNAUTHORIZED_PRINTER		  (NWCCODE)0x0402 /* 1026 PServer	*/

/*================= XNP error codes =======================================*/
#define NWPSE_NOT_READY                  (NWCCODE)0x0500 /* 1280 PServer	*/
#define NWPSE_INVALID_PROCESS            (NWCCODE)0x0501 /* 1281 PServer	*/
#define NWPSE_INVALID_JOB_ID				  (NWCCODE)0x0502 /* 1282 PServer	*/
#define NWPSE_NO_MSG_FILE					  (NWCCODE)0x0503 /* 1283 PServer	*/
#define NWPSE_JOB_REQUEST_ACTIVE         (NWCCODE)0x0504 /* 1284 PServer	*/

/*========= Cfg Scan and Cfg GetFirst/GetNext error codes =================*/
#define NWPSE_END_OF_LIST					  (NWCCODE)0x7760 /* 30560 Library	*/
#define NWPSE_NO_SUCH_LIST_ENTRY         (NWCCODE)0x7761 /* 30561 Library	*/
#define NWPSE_END_OF_ATTR_LIST			  (NWCCODE)0x7762 /* 30562 Library	*/
/*
// D.S. only; you get this when NWPSCfgVerifyxxxx finds the Common Name, 
// but the Class does not match what you were looking for. 
*/
#define NWPSE_WRONG_CLASS_LIST_ENTRY	  (NWCCODE)0x7763 /* 30563 Library	*/
#define NWPSE_NO_SUCH_PRINT_SERVER    	  (NWCCODE)0x7764 /* 30564 Library	*/

/*================= Job Configuration error codes =========================*/
#define NWPSE_BAD_VERSION					  (NWCCODE)0x7770 /* 30576 Library	*/
#define NWPSE_END_SCAN						  (NWCCODE)0x7771 /* 30577 Library	*/
#define NWPSE_ERROR_EXPANDING_DB         (NWCCODE)0x7772 /* 30578 Library	*/
#define NWPSE_ERROR_GETTING_DEFAULT      (NWCCODE)0x7773 /* 30579 Library	*/
#define NWPSE_ERROR_OPENING_DB			  (NWCCODE)0x7774 /* 30580 Library	*/
#define NWPSE_ERROR_READING_DB			  (NWCCODE)0x7775 /* 30581 Library	*/
#define NWPSE_PRINTCON_DB_FULL			  (NWCCODE)0x7776 /* 30582 Library	*/
#define NWPSE_ERROR_WRITING_DB			  (NWCCODE)0x7777 /* 30583 Library	*/

#define NWPSE_INTERNAL_ERROR				  (NWCCODE)0x7779 /* 30585 Library	*/
#define NWPSE_JOB_NOT_FOUND				  (NWCCODE)0x777A /* 30586 Library	*/
#define NWPSE_NO_DEFAULT_SPECIFIED		  (NWCCODE)0x777B /* 30587 Library	*/
#define NWPSE_OUT_OF_MEMORY				  (NWCCODE)0x777C /* 30588 Library	*/
#define NWPSE_ERROR_SEEKING_DB			  (NWCCODE)0x777D /* 30589 Library	*/
#define NWPSE_NO_ACCESS_RIGHTS_DB		  (NWCCODE)0x777E /* 30590 Library	*/
#define NWPSE_UNICODE_TBL_NOT_LOADED     (NWCCODE)0x777F /* 30590 Library	*/

/*========== Print Def error codes for import/export files ================*/
#define NWPSE_ERROR_OPENING_IMP			  (NWCCODE)0x7790 /* 30608 Library	*/
#define NWPSE_ERROR_READING_IMP			  (NWCCODE)0x7791 /* 30609 Library	*/
#define NWPSE_ERROR_WRITING_IMP			  (NWCCODE)0x7792 /* 30610 Library	*/
#define NWPSE_NO_ACCESS_RIGHTS_IMP		  (NWCCODE)0x7793 /* 30611 Library	*/

/*==================== Print Def error codes ==============================*/
#define NWPSE_CONTEXT_CANNOT_BE_ROOT	  (NWCCODE)0x77A0 /* 30624 Library	*/
#define NWPSE_CONTEXT_CONTAINS_NO_ORGS   (NWCCODE)0x77A1 /* 30625 Library	*/
#define NWPSE_HASHED_STRING_NOT_FOUND	  (NWCCODE)0x77A2 /* 30626 Library	*/
/*
// The "(Re-initialize)" mode, NWPS_RESET_MODE, is required to remain.  
// The error below is returned if the delete of the mode or change of 
// the name is attempted.
*/
#define NWPSE_CANT_CHANGE_MODE           (NWCCODE)0x77A3 /* 30627 Library	*/

									  
/*==================== NLM error codes ====================================*/
/* This is returned by NWPSRegisterLibraryClient									*/
#define NWPSE_UNABLE_TO_GET_SOCKET_RTAG  (NWCCODE)0x77B0 /* 30640 Lib NLM	*/
#define NWPSE_DUPLICATE_THREAD_REG		  (NWCCODE)0x77B1 /* 30641 Lib NLM	*/
/* This is returned by NWPSDeRegisterLibraryClient                         */
#define NWPSE_UNREGISTERED_THREAD		  (NWCCODE)0x77B2 /* 30642 Lib NLM	*/
#define NWPSE_UNREGISTERED_NLM			  (NWCCODE)0x77B3 /* 30643 Lib NLM	*/
#endif /* NWPS_ERR_INC */

