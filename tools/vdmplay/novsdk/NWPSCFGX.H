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
//บ Source module name: NWPSCFGX.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
/****************************************************************************/
/**  N W P S C f g x x x x x x x x x	C a l l s  ***************************/
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*++++++++++++ calls to make attribute name/number conversions +++++++++++++*/
/*==========================================================
  Convert an attribute name to a print service attribute id.
  If the name can not be mapped, a -1 is returned.
  ==========================================================*/
int NWAPI NWPSCfgAttrNameToNumber(
  char  NWPTR attrName);				/* Attribute name */

/*==========================================================
  Convert a print service attribute id to an attribute name.
  if the attribute id is invalid a NULL is returned.
  ==========================================================*/
char NWPTR NWAPI NWPSCfgAttrNumberToName(
  WORD  attrID);							/* Attribute Number */

/*===================================================================
  Get the default settings for a specified printer type.
  PrinterType should be NWPS_DEFAULT or NWPS_P_xxx.
  SubType is the port number for Parallel and Serial printers.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetPrinterDefaults(
  WORD  printerType,                /* Type of printer defaults */
  WORD  subtype,							/* Device SubType (Port) value  */
  NWPS_PConfig NWPTR pconfig);		/* Buffer to store defaults in  */

/*===============================================================
  Information stored in the bindery configuration file has a
  different format than NWPS_PConfig.	Those applications needing
  the old format can use these calls to do the conversion.
  ===============================================================*/
void NWAPI NWPSApiConfigToFileConfig(
  char  NWPTR name,
  NWPS_PConfig NWPTR apiConfig,
  BYTE  NWPTR fileConfig);

void NWAPI NWPSFileConfigToApiConfig(
  BYTE  NWPTR fileConfig,
  char  NWPTR name,
  DWORD NWPTR length,
  NWPS_PConfig NWPTR apiConfig);

/*========== calls to change the print server list =========================*/
/*
PSERVER ATTRIBUTE    ATTRIB VALUE	  DEFAULT		 MULTI-VALUED
-----------------    ------------	  -------		 ------------
NWPS_ATTR_ACL			Object_ACL_T	  PServer R/W		  YES
NWPS_ATTR_CN			char[]			  PServer name      NO
NWPS_ATTR_DESC       char[]			  " "               NO
NWPS_ATTR_HOST_DEV	char[]			  (none)            NO
NWPS_ATTR_L          char[]			  (none)            YES
NWPS_ATTR_NADD       Net_Address_T	  (none)            YES
NWPS_ATTR_O          char[]			  (none)            YES
NWPS_ATTR_OPER       NWPS_Typed_Name  current user      YES
NWPS_ATTR_OU			char[]			  (none)            YES
NWPS_ATTR_PRINTER    Typed_Name_T	  (none)            YES
NWPS_ATTR_PRIV_KEY	Octet_String_T   (none)            NO
NWPS_ATTR_PUBL_KEY	Octet_String_T   (none)            NO
NWPS_ATTR_SAP			char[]			  (none)            NO
NWPS_ATTR_SEE_ALSO	char[]			  (none)            YES
NWPS_ATTR_STAT       Integer_T		  NWPS_DOWN         NO
NWPS_ATTR_USER       NWPS_Typed_Name  current OU		  YES
NWPS_ATTR_VERS       char[]			  (none)            NO
*/

/*===================================================================
  Create a new Print Server object in the bindery/directory.
  A default Operator and User is created.  In the bindery the
  operator is SUPERVISOR and the user is group EVERYONE.  In the
  directory the operator is the current user and the user is
  the current Organizational Unit.	A password is also created.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/

NWCCODE NWAPI NWPSCfgAddPrintServer(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName);			/* Name of print server to add */

/*===================================================================
  Delete a Print Server. Any configuration information is also
  removed from the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeletePrintServer(
  WORD  connType,                      /* directory or bindery flag */
  DWORD connID,								/* connection identifier */
  char  NWPTR pServerName);				/* Name of print server to delete */ 

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstPrintServer, 
  NWPSCfgGetNextPrintServer, and NWPSCfgEndNextPrintServer trio of 
  calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find a print server.
  To find all the defined print servers, sequence should be set
  to -1 on the first call and the call will update the number.
  To verify a print server name, set sequence to NULL and
  PServerName to the name you want to find.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanPrintServer(
  WORD  connType,                   /* Directory or bindery flag */
  DWORD connID,							/* Connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR pServerName);			/* Name of print server */ 

/*===================================================================
  Find the first print server.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstPrintServer(
  WORD  connType,                   /* Directory or bindery flag */
  DWORD connID,							/* Connection identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR pServerName);			/* Name of print server */ 

/*===================================================================
  Find the next print server.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextPrintServer(
  NWPSListHandle handle,				/* handle for list of print servers */
  char  NWPTR pServerName);			/* Name of print server */ 

/*===================================================================
  End scanning for print servers.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextPrintServer(
  NWPSListHandle handle);				/* handle for list of print servers */

/*===================================================================
  verify the print server.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgVerifyPrintServer(
  WORD  connType,                   /* Directory or bindery flag */
  DWORD connID,							/* Connection identifier */
  char  NWPTR pServerName);			/* Name of print server */ 

/*===================================================================
  Create a print server attribute in the bindery/directory.
  Attributes that do not exist in the bindery will fail (Sorry).
  See above for the list of legal attributes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgAddPrintServerAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Delete a print server attribute from the bindery/directory.
  Attributes that do not exist in the bindery will fail (Sorry).
  See above for the list of legal attributes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeletePrintServerAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstPrintServerAttr, 
  NWPSCfgGetNextPrintServerAttr, and NWPSCfgEndNextPrintServerAttr 
  trio of calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find an attribute value in the bindery or directory.
  On the first call, sequence should be set to -1, attrID is set
  to identify the attribute to read and attrValue is a pointer
  to the buffer to write the attribute value to.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanPrintServerAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Find the first attribute value in the bindery or directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstPrintServerAttr(
  WORD  connType,                   /* directory or bindery flag */ 
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Find the next attribute value in the bindery or directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextPrintServerAttr(
  NWPSListHandle handle,				/* handle for the list of print 
													server attributes */
  char  NWPTR pServerName,          /* name of the print server */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  End scanning for attribute values in the bindery or directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextPrintServerAttr(
  NWPSListHandle handle);				/* handle for the list of print 
													server attributes */


/*===================================================================
  Modify an attribute value in the bindery or directory.
  If the attribute is single valued, the AddPrintServerAttr()
  will perform almost the same function.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgModifyPrintServerAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR oldValue,             /* pointer to the old attribute value */
  void  NWPTR newValue);				/* pointer to the new attribute value */


/* calls to change the file server list */
/*
FSERVER ATTRIBUTE    ATTRIB VALUE	  DEFAULT		 MULTI-VALUED
-----------------    ------------	  -------		 ------------
(none)
*/
/*===================================================================
  Add a new file server for the print server to use.	Since print
  servers are context oriented in directory services, this call
  does not make sense and will fail in directory services mode.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgAddNServer(
  WORD  connType,                   /* Directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR nServerName);			/* Name of NetWare server to add */

/*===================================================================
  Delete a file server from the print server's service list.
  Since file servers are context oriented in directory services,
  this call does not make sense and will fail in directory mode.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeleteNServer(
  WORD  connType,                   /* Directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR nServerName);			/* Name of NetWare server to delete */

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstNServer, 
  NWPSCfgGetNextNServer, and NWPSCfgEndNextNServer trio of 
  calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find a file server from the print server's service list.
  Since file servers are context oriented in directory services,
  this call does not make sense and will fail in directory mode.
  On the first call sequence should be set to -1 and the routine
  will change the number before returning.  To verify a specific
  file server is in the serivce list, set sequence to NULL and
  set fserveName before making the call.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanNServer(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR nServerName);			/* Name of found NetWare server */

/*===================================================================
  Find the first file server from the print server's service list.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstNServer(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR nServerName);			/* Name of found NetWare server */

/*===================================================================
  Find the next file server from the print server's service list.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextNServer(
  NWPSListHandle handle,				/* handle for the list of 
													NetWare servers */
  char  NWPTR nServerName);			/* Name of found NetWare server */

/*===================================================================
  End scanning file servers from the print server's service list.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextNServer(
  NWPSListHandle handle);				/* handle for the list of 
													NetWare servers */

/*===================================================================
  Find the file server from the print server's service list.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgVerifyNServer(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR nServerName);			/* Name of found NetWare server */


/* calls to change the printer configuration */
/*
PRINTER ATTRIBUTE    ATTRIB VALUE	  DEFAULT		 MULTI-VALUED
-----------------    ------------	  -------		 ------------
NWPS_ATTR_ACL			Object_ACL_T	  printer R/W		  YES
NWPS_ATTR_CART       char[]			  (none)            YES
NWPS_ATTR_CN			char[]			  printer name      NO
NWPS_ATTR_CONF       Octet_String_T   LPT1				  NO
NWPS_ATTR_DESC       char[]			  " "               NO
NWPS_ATTR_DQUEUE		char[]			  (none)            NO
NWPS_ATTR_HOST_DEV	char[]			  (none)            NO
NWPS_ATTR_L          char[]			  (none)            YES
NWPS_ATTR_MEMORY		Integer_T		  (none)            NO
NWPS_ATTR_NADD       Net_Address_T	  (none)            YES
NWPS_ATTR_NADD_REST	Net_Address_T	  (none)            YES
NWPS_ATTR_NOTIFY		NWPS_Typed_Name  Job Owner         YES
NWPS_ATTR_O          char[]			  (none)            YES
NWPS_ATTR_OPER       NWPS_Typed_Name  current user      YES
NWPS_ATTR_OU			char[]			  (none)            YES
NWPS_ATTR_OWNER		NWPS_Typed_Name  current user      YES
NWPS_ATTR_PAGE       char[]			  (none)            YES
NWPS_ATTR_PRINT_SER	Typed_Name_T	  pServer/number	  NO
NWPS_ATTR_QUEUE		Typed_Name_T	  (none)            YES
NWPS_ATTR_SEE_ALSO	char[]			  (none)            YES
NWPS_ATTR_SERIAL		char[]			  (none)            YES
NWPS_ATTR_STAT       Integer_T	 NWPS_PSTAT_NOT_CONNECTED	NO
NWPS_ATTR_TYPE(faces)char[]			  (none)            YES
*/

/*===================================================================
  Create a new printer object.
  Printer number is required for bindery identification only.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgAddPrinter(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName,          /* Name of the Printer to add */
  WORD  NWPTR printerNumber);       /* Number of the Printer to add */

/*===================================================================
  Delete a printer from the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeletePrinter(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of printer to delete */ 

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstPrinter, 
  NWPSCfgGetNextPrinter, and NWPSCfgEndNextPrinter trio of 
  calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find a printer in the directory/bindery.  To find all of the
  printers and their configurations, set sequence to -1 before
  the first call and the routine will update the number before
  returning.  To verify the existance of a specific printer,
  set sequence to NULL and set the name in the PConfig strucutre.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanPrinter(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer sought */

/*===================================================================
  Find the first printer in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstPrinter(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer sought */

/*===================================================================
  Find the next printer in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextPrinter(
  NWPSListHandle handle,				/* handle for the list of printers */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer sought */

/*===================================================================
  End scan for printers in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextPrinter(
  NWPSListHandle handle);				/* handle for the list of printers */

/*===================================================================
  Find the printer in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgVerifyPrinter(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer to verify */

/*===================================================================
  Add a printer attribute to the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgAddPrinterAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Delete a printer attribute from the bindery/directory.
  See AddPrinterAttr for a list of legal bindery attributes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeletePrinterAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstPrinterAttr, 
  NWPSCfgGetNextPrinterAttr, and NWPSCfgEndNextPrinterAttr trio of 
  calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find a printer attribute in the bindery/directory.
  See AddPrinterAttr for a list of legal bindery attributes.
  To find the first value of a multi-valued attribute, set sequence
  to -1 before making the call.	Sequence is updated internally 
  in preparation for each call. attrValue should be a buffer large
  enough to hold each attribute value.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanPrinterAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Find the first printer attribute in the bindery/directory.
  See AddPrinterAttr for a list of legal bindery attributes.
  attrValue should be a buffer large enough to hold each attribute value.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstPrinterAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Find the next printer attribute in the bindery/directory.
  attrValue should be a buffer large enough to hold each attribute value.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextPrinterAttr(
  NWPSListHandle handle,				/* handle to the list of printer 
													attributes */
  char  NWPTR printerName,          /* name of the printer */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  End scan for printer attributes in the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextPrinterAttr(
  NWPSListHandle handle);				/* handle to the list of printer 
													attributes */

/*===================================================================
  Change a printer attribute in the bindery/directory.
  This function is similar to a add attribute to a single value
  attribute, however, some attributes must be changed in one call.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgModifyPrinterAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR oldValue,             /* pointer to old attribute buffer */
  void  NWPTR newValue);				/* pointer to new attribute buffer */


/* calls to change a printer's queue list */
/*
QUEUE ATTRIBUTE		ATTRIB VALUE	  DEFAULT		 MULTI-VALUED
-----------------    ------------	  -------		 ------------
NWPS_ATTR_ACL			Object_ACL_T	  queue R/W         YES
NWPS_ATTR_CN			char[]			  printer name      NO
NWPS_ATTR_DESC       char[]			  " "               NO
NWPS_ATTR_DEVICE		char[]			  (none)            YES
NWPS_ATTR_HOST_RES	char[]			  (none)            NO
NWPS_ATTR_HOST_SER	char[]			  NetWare Server	  NO
NWPS_ATTR_L          char[]			  (none)            YES
NWPS_ATTR_NADD       Net_Address_T	  (none)            YES
NWPS_ATTR_O          char[]			  (none)            YES
NWPS_ATTR_OPER       NWPS_Typed_Name  current user      YES
NWPS_ATTR_OU			char[]			  (none)            YES
NWPS_ATTR_QUE_DIR    char[]			  <Vol>:\QUEUE\<ID>.QDR  YES
NWPS_ATTR_SEE_ALSO	char[]			  (none)            YES
NWPS_ATTR_SERVER		char[]			  (none)            YES
NWPS_ATTR_USER       NWPS_Typed_Name  current OU		  YES
NWPS_ATTR_VOLUME		char[]			  Volume            NO
*/

/*===================================================================
  Add a print queue to the bindery/directory.  In bindery mode,
  the volumeName is automatically assigned.	In both modes,
  default values are added for the following fields;
		Description, Queue Directory, Operator, User and Volume.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgAddPrintQueue(
  WORD  connType,                      /* directory or bindery flag */
  DWORD connID,								/* connection identifier */
  char  NWPTR queueName,					/* Name of the print queue */ 
  char  NWPTR volumeName);             /* Name of the print queue's volume */ 

/*===================================================================
  Delete a print queue from the bindery/directory.  Any reference
  to the queue is also removed from Printers and Print Servers
  that are within the same context as the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeletePrintQueue(
  WORD  connType,                      /* directory or bindery flag */
  DWORD connID,								/* connection identifier */
  char  NWPTR queueName);					/* Name of the queue to delete */

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstPrintQueue, 
  NWPSCfgGetNextPrintQueue, and NWPSCfgEndNextPrintQueue trio of 
  calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find a print queue in the directory/bindery.
  On the first call, sequence should be set to -1 and the function
  will change it for subsequent calls.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanPrintQueue(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR queueName);				/* Name of the print queue */ 

/*===================================================================
  Find the first print queue in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstPrintQueue(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR queueName);				/* Name of the print queue */ 

/*===================================================================
  Find the next print queue in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextPrintQueue(
  NWPSListHandle handle,				/* handle to the list of Print Queues */
  char  NWPTR queueName);				/* Name of the print queue */ 

/*===================================================================
  End scan for print queues in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextPrintQueue(
  NWPSListHandle handle);				/* handle to the list of Print Queues */

/*===================================================================
  Find the print queue in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgVerifyPrintQueue(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName);				/* Name of the print queue */ 

/*===================================================================
  Create or Add a Print Queue attribute to the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgAddPrintQueueAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Delete a Print Queue attribute from the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgDeletePrintQueueAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*########################### OBSOLETE ##############################
  ##### USE CORRESPONDING GETFIRST/GETNEXT/ENDNEXT TRIO INSTEAD #####
  ###################################################################
  NOTE: This call is inefficient and has been obsoleted
  by the creation of the NWPSCfgGetFirstPrintQueueAttr, 
  NWPSCfgGetNextPrintQueueAttr, and NWPSCfgEndNextPrintQueueAttr trio 
  of calls defined below. This call remains only for backward 
  compatibility's sake. 

  Find an attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ###################################################################*/
NWCCODE NWAPI NWPSCfgScanPrintQueueAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  DWORD NWPTR sequence,             /* Sequence number; start at -1 */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Find the first attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetFirstPrintQueueAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  Find the next attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgGetNextPrintQueueAttr(
  NWPSListHandle handle,				/* handle to the list of Print 
													Queue attributes */
  char  NWPTR queueName,				/* name of the print queue */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

/*===================================================================
  End scan for attributes for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgEndNextPrintQueueAttr(
  NWPSListHandle handle);				/* handle to the list of Print 
													Queue attributes */

/*===================================================================
  Modify an attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSCfgModifyPrintQueueAttr(
  WORD  connType,                   /* directory or bindery flag */
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR oldValue,             /* pointer to old attribute buffer */
  void  NWPTR newValue);				/* pointer to new attribute buffer */

#ifdef __cplusplus
}
#endif

