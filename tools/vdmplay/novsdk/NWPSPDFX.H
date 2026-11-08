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
//บ Source module name: NWPSPDFX.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifdef __cplusplus
extern "C" {
#endif

/*++++++++++++++++++++++ General Database calls +++++++++++++++++++++*/
/*==================================================================
  Get the Version number stored in the database.  The database
  file/stream is automaticly opened and closed by this call.
  This call returns 0 on success, or non-zero error code on failure.
  ==================================================================*/
NWCCODE NWAPI NWPSPdfGetVersion(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD NWPTR pdfVersion);             /* Returns db version					*/

/*==================================================================
  Set the Version number stored in the database.  The database
  file/stream is automaticly opened and closed by this call.
  This call returns 0 on success, or non-zero error code on failure.
  ==================================================================*/
NWCCODE NWAPI NWPSPdfSetVersion(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD pdfVersion);                   /* Returns db version					*/

/*=========================================================================
	Set the Context's DCK_NAME_CONTEXT to the Organizational Unit or
	Organization in the Directory where a database already exists, searching
	the Directory toward the [Root] from the given Printer or Print Queue.
  =========================================================================*/
NWCCODE NWAPI NWPSPdfLocateDBAndSetContext(
  DWORD contextID,							/* Directory Context Handle			*/
  char  NWPTR printerOrQueueObjectName ); /* search starting point			*/

/*++++++++++++++++++++++++++++ Form Calls +++++++++++++++++++++++++++++++++*/
/*==================================================================
  Add a form to the PrintDef database. Forms are independent
  of printers and are unique on each file server or context.
  This call returns 0 on success, or non-zero error code on failure.
  ==================================================================*/
NWCCODE NWAPI NWPSPdfAddForm(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR formName,                /* form name								*/
  WORD  formNumber,							/* form number                      */
  WORD  formLength,							/* form length                      */
  WORD  formWidth);							/* form width								*/

/*===================================================================
  Delete a form from the PrintDef database.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfDeleteForm(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR formName);					/* form name								*/

/*===================================================================
  Find a Form in the PrintDef database.
  If the user wants to find all the forms, sequence should be
  set to -1 on the first call, and the sequence value will be
  updated when the call returns.
  If the user wants to find a specific Form, the sequence pointer
  should be NULL and the formName should be set to the desired form.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfScanForm(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD NWPTR sequence,                /* -1 for first call                */
  char  NWPTR formName);					/* name or NULL							*/

/*===================================================================
  Get each Form in turn; these calls obsolete the ScanForm call
  ===================================================================*/
NWCCODE NWAPI NWPSPdfGetFirstForm(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR formName,                /* form name								*/
  WORD  NWPTR formNumber,					/* number or NULL                   */
  WORD  NWPTR formLength,					/* length space or NULL             */
  WORD  NWPTR formWidth);					/* width space or NULL					*/

NWCCODE NWAPI NWPSPdfGetNextForm(
  NWPSListHandle handle,               /* Handle for GetNext and EndNext   */
  char  NWPTR formName,                /* form name								*/
  WORD  NWPTR formNumber,					/* number or NULL                   */
  WORD  NWPTR formLength,					/* length space or NULL             */
  WORD  NWPTR formWidth);					/* width space or NULL					*/

/*======================================================
  Read the form information from the PrintDef database.
  If the form does not exist, an error code is returned.
  If it does exist, the form's name, number, length and
  width are set and a 0 is returned.
  ======================================================*/
NWCCODE NWAPI NWPSPdfReadForm(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR formName,                /* form name								*/
  WORD  NWPTR formNumber,					/* number or NULL                   */
  WORD  NWPTR formLength,					/* length space or NULL             */
  WORD  NWPTR formWidth);					/* width space or NULL					*/

/*===================================================================
  Update the form information. If you don't want to change the
  name, set newFormName to NULL. If you don't want to change a
  parameter set the value to -1.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfUpdateForm(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR oldFormName,             /* old form name							*/
  char  NWPTR newFormName,             /* new form name or NULL				*/
  WORD  formNumber,							/* new form number or -1				*/
  WORD  formLength,							/* new form length or -1				*/
  WORD  formWidth);							/* new form width or -1             */

/*+++++++++++++++++++++++++++ Device Calls ++++++++++++++++++++++++++++++++*/
/*===================================================================
  Add a device to the PrintDef database. The Device is created
  without any Functions or modes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfAddDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName);             /* device name                      */

/*===================================================================
  Delete a device from the PrintDef database.
  When a Device is deleted, all of the device's Functions and Modes
  are also deleted.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfDeleteDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName);             /* device to be removed             */

/*===================================================================
  Find a Device in the PrintDef database.
  To find all of the Devices, set sequence to -1 on the first call
  and it will be reset by the call if a Device if found.
  To find a specific Device, set sequence pointer to NULL and set deviceName
  to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfScanDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD NWPTR sequence,                /* must give seq. or name				*/
  char  NWPTR deviceName);             /* returned name found					*/

/*===================================================================
  Get each Device in turn; these calls obsolete the ScanDevice call
  ===================================================================*/
NWCCODE NWAPI NWPSPdfGetFirstDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR deviceName);             /* returned name found					*/

NWCCODE NWAPI NWPSPdfGetNextDevice(
  NWPSListHandle handle,               /* Handle for GetNext and EndNext   */
  char  NWPTR deviceName);             /* returned name found					*/

/*===================================================================
  Find out how many functions and modes are defined for a device.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfReadDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of device                   */
  WORD  NWPTR modeCount,					/* number of modes defined          */
  WORD  NWPTR funcCount);					/* number of func's defined			*/

/*===================================================================
  Change a Device name in the PrintDef database.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfUpdateDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR oldDeviceName,				/* old device name						*/
  char  NWPTR newDeviceName);          /* new device name						*/

/*+++++++++++++++++++++++++ Mode Calls ++++++++++++++++++++++++++++++++++++*/
/*===================================================================
  Create a new Mode for a specific Device.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfAddMode(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* new mode name							*/

/*===================================================================
  Delete a Mode from a Device.
  When the Mode is deleted, the Functions are left intact.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfDeleteMode(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* name of mode to remove				*/

/*===================================================================
  Find a Mode in the PrintDef database.
  To find all of the Modes, set sequence to -1 on the first call
  and it will be reset by the call if a Mode is found.
  To find a specific Mode, set sequence to NULL and set modeName
  to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfScanMode(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD NWPTR sequence,                /* -1 on first call						*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* returned mode name					*/

/*===================================================================
  Get each Mode in turn; these calls obsolete the ScanMode call
  ===================================================================*/
NWCCODE NWAPI NWPSPdfGetFirstMode(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR modeName);					/* returned mode name					*/

NWCCODE NWAPI NWPSPdfGetNextMode(
  NWPSListHandle handle,               /* Handle for GetFirst and EndNext  */
  char  NWPTR modeName);					/* returned mode name					*/

/*===================================================================
  Find information on a defined Mode.
  If the Mode is defined, and funcCount is not NULL, the number
  of functions in the mode is returned in funcCount.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfReadMode(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR reqModeName,             /* request mode name                */
  WORD  NWPTR funcCount);					/* function count or NULL				*/

/*===================================================================
  Change the name of a Mode.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfUpdateMode(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR oldModeName,             /* old mode name							*/
  char  NWPTR newModeName);				/* new mode name or NULL				*/


/*++++++++++++++++++ Mode-Function Grouping Calls +++++++++++++++++++++++++*/
/*====================================================================
  Add a previously defined function to a previously defined mode list.
  A zero is returned on succecss, or a non-zero error code on failure
  ====================================================================*/
NWCCODE NWAPI NWPSPdfAddModeFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  char  NWPTR funcName,                /* func to add to group             */
  WORD  location);							/* where to insert function or -1	*/

/*==================================================================
  Delete a function from a defined mode list.
  Neither the Mode or Function is deleted from the Device lists.
  A zero is returned on success, or a non-zero error code on failure
  ==================================================================*/
NWCCODE NWAPI NWPSPdfDeleteModeFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD sequence,                      /* number of the function to remove 
														from group								*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* name of associated mode          */

/*===================================================================
  Find the name of a Function associated with a Mode.
  To find all of the Functions associated with a Mode, set
  sequence to -1 on the first call and it will be reset by
  the call if a Functions is found.
  To find a specific Function associated with a Mode, set sequence
  to NULL and set functName to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfScanModeFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD NWPTR sequence,                /* -1 on first call						*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  char  NWPTR funcName);					/* returned function name				*/

/*===================================================================
  Get each Function in turn; these calls obsolete the ScanModeFunction call
  ===================================================================*/
NWCCODE NWAPI NWPSPdfGetFirstModeFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR funcName);					/* returned function name				*/

NWCCODE NWAPI NWPSPdfGetNextModeFunction(
  NWPSListHandle handle,               /* Handle for GetFirst and EndNext  */
  char  NWPTR funcName);					/* returned function name				*/

/*==================================================================
  Find the Modes which use this Function.
  To find all of the Modes associated with a Function, set
  sequence to -1 on the first call and it will be reset by
  the call if a Mode is found.
  A zero is returned on success, or a non-zero error code on failure
  ==================================================================*/
NWCCODE NWAPI NWPSPdfScanFunctionMode(
  WORD  connType,
  DWORD connID,
  DWORD NWPTR funcSequence,
  DWORD NWPTR modeSequence,
  char  NWPTR deviName,
  char  NWPTR modeName,
  char  NWPTR funcName);

/*=================================================================
  Read the actual function values associated with a mode.
  On the first call, funcOffset should be set to 0, and funcSize
  should be set to the size of the buffer pointed to by funcBuffer.
  On return funcSize will contain the actual number of bytes copied
  to the funcBuffer and the return value will be 0.
  A zero is returned if there is any data after the specified
  funcOffset (start point).  A failure code is retuned if
  there is an error or if funcOffset is greater than the
  available number of bytes.
  =================================================================*/
NWCCODE NWAPI NWPSPdfReadModeFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  /*-------- no function name because this is a mode group ----------------*/
  WORD  funcOffset,							/* number of bytes to skip          */
  WORD  NWPTR funcSize,                /* req:buff size; ret:# read			*/
  BYTE  NWPTR funcString);             /* buffer for read block				*/

/*+++++++++++++++++++++++++++ Function Calls ++++++++++++++++++++++++++++++*/
/*===================================================================
  Add a Function to a Device.
  funcSize should be set to the number of bytes in funcString.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfAddFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName,                /* name of func. to add             */
  WORD  funcSize,                      /* count of bytes in function       */
  BYTE  NWPTR funcString);             /* list of bytes in function			*/

/*===================================================================
  Delete a function from the Device in the PrintDef database.
  If the Function is refered to in any Mode, the Function
  will be automaticly deleted from the Mode as well.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfDeleteFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName);					/* name of func. to delete          */

/*===================================================================
  Find a Function defined for a specific Device.
  To find all of the Functions associated with a Device, set
  sequence to -1 on the first call and it will be reset by
  the call if a Functions is found.
  To find a specific Function associated with a Device, set sequence
  to NULL and set functName to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfScanFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  DWORD NWPTR sequence,                /* -1 on first call						*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName);					/* name of next function				*/

/*===================================================================
  Get each Function in turn; these calls obsolete the ScanFunction call
  ===================================================================*/
NWCCODE NWAPI NWPSPdfGetFirstFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR funcName, 					/* returned function name				*/
  NWPSPdfFuncData NWPTR funcData);     /* returned function data struct    */

NWCCODE NWAPI NWPSPdfGetNextFunction(
  NWPSListHandle handle,               /* Handle for GetFirst and EndNext  */
  char  NWPTR funcName, 					/* returned function name				*/
  NWPSPdfFuncData NWPTR funcData);     /* returned function data struct    */

/*===================================================================
  Read the byte string associated with a function.
  funcSize should be set equal to the size of the buffer funcString.
  funcSize will be set to the actual size of the byte string if the
  call is successful.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfReadFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName,                /* name of the function             */
  WORD  funcOffset,							/* bytes to skip past					*/
  WORD  NWPTR funcSize,                /* req: buff size; ret:# read       */
  BYTE  NWPTR funcString);             /* byte list or NULL                */

/*===================================================================
  Change the function string assigned to a function name.
  To change the function's name, set newFuncName to a new name.
  To leave the Function name the same, set newFuncName to NULL.
  To change the function string, set funcSize to the number of
  bytes in the funcString and set funcString to point to the
  new string. To leave the string the same, set funcSize to -1.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWPSPdfUpdateFunction(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR oldFuncName,             /* current function name				*/
  char  NWPTR newFuncName,             /* new name or NULL						*/
  WORD  funcSize,                      /* number of bytes or -1				*/
  BYTE  NWPTR funcString);             /* byte list or NULL                */

/*++++++++++++++++++++ Import and Export Functions ++++++++++++++++++++++++*/
/*-----------------------------------------------------
  The path name for a PDF file should have
  one of the following formats:
	 "\\<file server>\<volume>\<path>\<file name>[.PDF]"
	 "<volume>:<path>\<file name>[.PDF]"
	 NULL to create the file ".\<device name>[.PDF]"
  Every PDF import/export file also has a date code
  associated with it.  The date is always set when the
  file is created.
  -----------------------------------------------------*/
/*===========================================
  Import a device from a file to the database
  ===========================================*/
NWCCODE NWAPI NWPSPdfImportDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR fileName,                /* name of .PDF file                */
  char  NWPTR deviceName);             /* new device name or NULL          */

NWCCODE NWAPI NWPSPdfManagedImportDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR fileName,                /* name of .PDF file                */
  char  NWPTR deviceName,					/* new device name or NULL          */
  NWPSImportManager importManagerFunc, /* pointer to the Manager Function	*/
  void  NWPTR importManagerParm1);		/* caller's Manager context variable*/


/*===============================================
  Export a device to a Pdf file from the database
  ===============================================*/
NWCCODE NWAPI NWPSPdfExportDevice(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR fileName,                /* name of .PDF file                */
  char  NWPTR deviceName);             /* name of device to export			*/

/*=========================================================
  Delete the Directory Services "Printer Control" Attribute
  at the given context.
  =========================================================*/
NWCCODE NWAPI NWPSPdfDeleteDatabase(
  WORD  connType,                      /* Type of server/network				*/
  DWORD connID);								/* NetWare Server Connection ID		*/

#ifdef __cplusplus
}
#endif
