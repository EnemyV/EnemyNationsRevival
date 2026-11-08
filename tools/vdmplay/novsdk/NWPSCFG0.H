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
//บ Source module name: NWPSCFG0.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
/****************************************************************************/
/**  N W P 0 C f g x x x x x x x x x	C a l l s  ***************************/
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

NWCCODE NWAPI NWP0CfgAddPrintServer(
DWORD connID,							/* connection identifier */
char	NWPTR pServerName);			/* Name of print server to add */

#define NWPSCfgAddPrintServer( connType, connID, pServerName ) \
		  NWP0CfgAddPrintServer( connID, pServerName)

/*===================================================================
  Delete a Print Server. Any configuration information is also
  removed from the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeletePrintServer(
  DWORD connID,								/* connection identifier */
  char  NWPTR pServerName);				/* Name of print server to delete */ 

#define NWPSCfgDeletePrintServer( connType, connID, pServerName)\
		  NWP0CfgDeletePrintServer( connID, pServerName)

/*===================================================================
  Find the first print server.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetFirstPrintServer(
  DWORD connID,							/* Connection identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR pServerName);			/* Name of print server */ 

#define NWPSCfgGetFirstPrintServer( connType, connID, handle, pServerName)\
		  NWP0CfgGetFirstPrintServer( connID, handle, pServerName)

/*===================================================================
  Find the next print server.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetNextPrintServer(
  NWPSListHandle handle,				/* handle for list of print servers */
  char  NWPTR pServerName);			/* Name of print server */ 

#define NWPSCfgGetNextPrintServer( handle, pServerName)\
		  NWP0CfgGetNextPrintServer( handle, pServerName)

/*===================================================================
  End scanning for print servers.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgEndNextPrintServer(
  NWPSListHandle handle);				/* handle for list of print servers */

#define NWPSCfgEndNextPrintServer( handle)\
		  NWP0CfgEndNextPrintServer( handle)

/*===================================================================
  verify the print server.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgVerifyPrintServer(
  DWORD connID,							/* Connection identifier */
  char  NWPTR pServerName);			/* Name of print server */ 

#define NWPSCfgVerifyPrintServer( connType, connID, pServerName)\
		  NWP0CfgVerifyPrintServer( connID, pServerName)

/*===================================================================
  Create a print server attribute in the bindery/directory.
  Attributes that do not exist in the bindery will fail (Sorry).
  See above for the list of legal attributes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgAddPrintServerAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgAddPrintServerAttr( connType, connID, pServerName, attrID,\
											  attrValue)\
		  NWP0CfgAddPrintServerAttr( connID, pServerName, attrID, attrValue)

/*===================================================================
  Delete a print server attribute from the bindery/directory.
  Attributes that do not exist in the bindery will fail (Sorry).
  See above for the list of legal attributes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeletePrintServerAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgDeletePrintServerAttr( connType, connID, pServerName, \
												  attrID, attrValue)\
		  NWP0CfgDeletePrintServerAttr( connID, pServerName, attrID, attrValue)

/*===================================================================
  Find the first attribute value in the bindery or directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetFirstPrintServerAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgGetFirstPrintServerAttr( connType, connID, pServerName, \
													 attrID, handle, attrValue)\
		  NWP0CfgGetFirstPrintServerAttr( connID, pServerName, attrID, \
													 handle, attrValue)

/*===================================================================
  Find the next attribute value in the bindery or directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetNextPrintServerAttr(
  NWPSListHandle handle,				/* handle for the list of print 
													server attributes */
  char  NWPTR pServerName,          /* name of the print server */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgGetNextPrintServerAttr( handle, pServerName, attrValue)\
		  NWP0CfgGetNextPrintServerAttr( handle, pServerName, attrValue)

/*===================================================================
  End scanning for attribute values in the bindery or directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgEndNextPrintServerAttr(
  NWPSListHandle handle);				/* handle for the list of print
													server attributes */

#define NWPSCfgEndNextPrintServerAttr( handle )\
		  NWP0CfgEndNextPrintServerAttr( handle )

/*===================================================================
  Modify an attribute value in the bindery or directory.
  If the attribute is single valued, the AddPrintServerAttr()
  will perform almost the same function.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgModifyPrintServerAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR oldValue,             /* pointer to the old attribute value */
  void  NWPTR newValue);				/* pointer to the new attribute value */

#define NWPSCfgModifyPrintServerAttr( connType, connID, pServerName, \
												  attrID, oldValue, newValue)\
		  NWP0CfgModifyPrintServerAttr( connID, pServerName, attrID, \
												  oldValue, newValue)


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
NWCCODE NWAPI NWP0CfgAddNServer(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR nServerName);			/* Name of NetWare server to add */

#define NWPSCfgAddNServer( connType, connID, pServerName, nServerName)\
		  NWP0CfgAddNServer( connID, pServerName, nServerName)

/*===================================================================
  Delete a file server from the print server's service list.
  Since file servers are context oriented in directory services,
  this call does not make sense and will fail in directory mode.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeleteNServer(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR nServerName);			/* Name of NetWare server to delete */

#define NWPSCfgDeleteNServer( connType, connID, pServerName, nServerName)\
		  NWP0CfgDeleteNServer( connID, pServerName, nServerName)

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
NWCCODE NWAPI NWP0CfgAddPrinter(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName,          /* Name of the Printer to add */
  WORD  NWPTR printerNumber);       /* Number of the Printer to add */

#define NWPSCfgAddPrinter( connType, connID, pServerName, printerName, \
									printerNumber)\
		  NWP0CfgAddPrinter( connID, pServerName, printerName, printerNumber)

/*===================================================================
  Delete a printer from the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeletePrinter(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of printer to delete */ 

#define NWPSCfgDeletePrinter( connType, connID, pServerName, printerName)\
		  NWP0CfgDeletePrinter( connID, pServerName, printerName)

/*===================================================================
  Find the first printer in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetFirstPrinter(
  DWORD connID,							/* connection identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer sought */

#define NWPSCfgGetFirstPrinter( connType, connID, handle, pServerName, \
										  printerName)\
		  NWP0CfgGetFirstPrinter( connID, handle, pServerName, printerName)

/*===================================================================
  Find the next printer in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetNextPrinter(
  NWPSListHandle handle,				/* handle for the list of printers */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer sought */

#define NWPSCfgGetNextPrinter( handle, pServerName, printerName)\
		  NWP0CfgGetNextPrinter( handle, pServerName, printerName)

/*===================================================================
  End scan for printers in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgEndNextPrinter(
  NWPSListHandle handle);				/* handle for the list of printers */

#define NWPSCfgEndNextPrinter( handle)\
		  NWP0CfgEndNextPrinter( handle)

/*===================================================================
  Find the printer in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgVerifyPrinter(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* Name of print server */ 
  char  NWPTR printerName);			/* Name of the Printer to verify */

#define NWPSCfgVerifyPrinter( connType, connID, pServerName, printerName)\
		  NWP0CfgVerifyPrinter( connID, pServerName, printerName)

/*===================================================================
  Add a printer attribute to the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgAddPrinterAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgAddPrinterAttr( connType, connID, pServerName, printerName, \
										 attrID, attrValue)\
		  NWP0CfgAddPrinterAttr( connID, pServerName, printerName, attrID, \
										 attrValue)

/*===================================================================
  Delete a printer attribute from the bindery/directory.
  See AddPrinterAttr for a list of legal bindery attributes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeletePrinterAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgDeletePrinterAttr( connType, connID, pServerName, \
											 printerName, attrID, attrValue)\
		  NWP0CfgDeletePrinterAttr( connID, pServerName, printerName, \
											 attrID, attrValue)

/*===================================================================
  Find the first printer attribute in the bindery/directory.
  See AddPrinterAttr for a list of legal bindery attributes.
  attrValue should be a buffer large enough to hold each attribute value.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetFirstPrinterAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgGetFirstPrinterAttr( connType, connID, pServerName, \
												printerName, attrID, handle, attrValue)\
		  NWP0CfgGetFirstPrinterAttr( connID, pServerName, printerName, \
												attrID, handle, attrValue)

/*===================================================================
  Find the next printer attribute in the bindery/directory.
  attrValue should be a buffer large enough to hold each attribute value.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetNextPrinterAttr(
  NWPSListHandle handle,				/* handle to the list of printer
													attributes */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgGetNextPrinterAttr( handle, printerName, attrValue)\
		  NWP0CfgGetNextPrinterAttr( handle, attrValue)

/*===================================================================
  End scan for printer attributes in the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgEndNextPrinterAttr(
  NWPSListHandle handle);				/* handle to the list of printer 
													attributes */

#define NWPSCfgEndNextPrinterAttr( handle)\
		  NWP0CfgEndNextPrinterAttr( handle)

/*===================================================================
  Change a printer attribute in the bindery/directory.
  This function is similar to a add attribute to a single value
  attribute, however, some attributes must be changed in one call.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgModifyPrinterAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR pServerName,          /* name of the print server */
  char  NWPTR printerName,          /* name of the printer */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR oldValue,             /* pointer to old attribute buffer */
  void  NWPTR newValue);				/* pointer to new attribute buffer */

#define NWPSCfgModifyPrinterAttr( connType, connID, pServerName, \
											 printerName, attrID, oldValue, newValue)\
		  NWP0CfgModifyPrinterAttr( connID, pServerName, printerName, \
											 attrID, oldValue, newValue)


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
NWCCODE NWAPI NWP0CfgAddPrintQueue(
  DWORD connID,								/* connection identifier */
  char  NWPTR queueName);					/* Name of the print queue */ 

#define NWPSCfgAddPrintQueue( connType, connID, queueName, volumeName)\
		  NWP0CfgAddPrintQueue( connID, queueName)

/*===================================================================
  Delete a print queue from the bindery/directory.  Any reference
  to the queue is also removed from Printers and Print Servers
  that are within the same context as the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeletePrintQueue(
  DWORD connID,								/* connection identifier */
  char  NWPTR queueName);					/* Name of the queue to delete */

#define NWPSCfgDeletePrintQueue( connType, connID, queueName)\
		  NWP0CfgDeletePrintQueue( connID, queueName)

/*===================================================================
  Find the first print queue in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetFirstPrintQueue(
  DWORD connID,							/* connection identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  char  NWPTR queueName);				/* Name of the print queue */ 

#define NWPSCfgGetFirstPrintQueue( connType, connID, handle, queueName)\
		  NWP0CfgGetFirstPrintQueue( connID, handle, queueName)

/*===================================================================
  Find the next print queue in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetNextPrintQueue(
  NWPSListHandle handle,				/* handle to the list of Print Queues */
  char  NWPTR queueName);				/* Name of the print queue */ 

#define NWPSCfgGetNextPrintQueue( handle, queueName)\
		  NWP0CfgGetNextPrintQueue( handle, queueName)

/*===================================================================
  End scan for print queues in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgEndNextPrintQueue(
  NWPSListHandle handle);				/* handle to the list of Print Queues */

#define NWPSCfgEndNextPrintQueue( handle)\
		  NWP0CfgEndNextPrintQueue( handle)

/*===================================================================
  Find the print queue in the directory/bindery.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgVerifyPrintQueue(
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName);				/* Name of the print queue */ 

#define NWPSCfgVerifyPrintQueue( connType, connID, queueName)\
		  NWP0CfgVerifyPrintQueue( connID, queueName)

/*===================================================================
  Create or Add a Print Queue attribute to the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgAddPrintQueueAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgAddPrintQueueAttr( connType, connID, queueName, attrID, \
											 attrValue)\
		  NWP0CfgAddPrintQueueAttr( connID, queueName, attrID, attrValue)

/*===================================================================
  Delete a Print Queue attribute from the bindery/directory.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgDeletePrintQueueAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgDeletePrintQueueAttr( connType, connID, queueName, attrID, \
												 attrValue)\
		  NWP0CfgDeletePrintQueueAttr( connID, queueName, attrID, attrValue)

/*===================================================================
  Find the first attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetFirstPrintQueueAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  NWPSListHandle NWPTR handle,		/* returned handle */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgGetFirstPrintQueueAttr( connType, connID, queueName, \
													attrID, handle, attrValue)\
		  NWP0CfgGetFirstPrintQueueAttr( connID, queueName, attrID, handle, \
													attrValue)

/*===================================================================
  Find the next attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgGetNextPrintQueueAttr(
  NWPSListHandle handle,				/* handle to the list of Print 
													Queue attributes */
  char  NWPTR queueName,				/* name of the print queue */
  void  NWPTR attrValue);				/* pointer to attribute buffer */

#define NWPSCfgGetNextPrintQueueAttr( handle, queueName, attrValue)\
		  NWP0CfgGetNextPrintQueueAttr( handle, queueName, attrValue)

/*===================================================================
  End scan for attributes for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgEndNextPrintQueueAttr(
  NWPSListHandle handle);				/* handle to the list of Print 
													Queue attributes */

#define NWPSCfgEndNextPrintQueueAttr( handle)\
		  NWP0CfgEndNextPrintQueueAttr( handle)

/*===================================================================
  Modify an attribute for the Print Queue.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP0CfgModifyPrintQueueAttr(
  DWORD connID,							/* connection identifier */
  char  NWPTR queueName,				/* name of the print queue */
  WORD  attrID,							/* attribute name identifier */
  void  NWPTR oldValue,             /* pointer to old attribute buffer */
  void  NWPTR newValue);				/* pointer to new attribute buffer */

#define NWPSCfgModifyPrintQueueAttr( connType, connID, queueName, attrID, \
												 oldValue, newValue)\
		  NWP0CfgModifyPrintQueueAttr( connID, queueName, attrID, oldValue, \
												 newValue)

#ifdef __cplusplus
}
#endif

