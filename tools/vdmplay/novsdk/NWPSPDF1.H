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
//บ Source module name: NWPSPDF1.H														  บ
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
NWCCODE NWAPI NWP1PdfGetVersion(
  DWORD connID,								/* NDS Context handle					*/
  DWORD NWPTR pdfVersion);             /* Returns db version					*/

#define NWPSPdfGetVersion( connType, connID, pdfVersion)\
		  NWP1PdfGetVersion( connID, pdfVersion)

/*==================================================================
  Set the Version number stored in the database.  The database
  file/stream is automaticly opened and closed by this call.
  This call returns 0 on success, or non-zero error code on failure.
  ==================================================================*/
NWCCODE NWAPI NWP1PdfSetVersion(
  DWORD connID,								/* NDS Context handle					*/
  DWORD pdfVersion);                   /* Returns db version					*/

#define NWPSPdfSetVersion( connType, connID, pdfVersion)\
		  NWP1PdfSetVersion( connID, pdfVersion)

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
NWCCODE NWAPI NWP1PdfAddForm(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR formName,                /* form name								*/
  WORD  formNumber,							/* form number                      */
  WORD  formLength,							/* form length                      */
  WORD  formWidth);							/* form width								*/

#define NWPSPdfAddForm( connType, connID, formName, formNumber, formLength, \
								formWidth)\
		  NWP1PdfAddForm( connID, formName, formNumber, formLength, formWidth)

/*===================================================================
  Delete a form from the PrintDef database.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfDeleteForm(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR formName);					/* form name								*/

#define NWPSPdfDeleteForm( connType, connID, formName)\
		  NWP1PdfDeleteForm( connID, formName)

/*===================================================================
  Find a Form in the PrintDef database.
  If the user wants to find all the forms, sequence should be
  set to -1 on the first call, and the sequence value will be
  updated when the call returns.
  If the user wants to find a specific Form, the sequence pointer
  should be NULL and the formName should be set to the desired form.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfScanForm(
  DWORD connID,								/* NDS Context handle					*/
  DWORD NWPTR sequence,                /* -1 for first call                */
  char  NWPTR formName);					/* name or NULL							*/

#define NWPSPdfScanForm( connType, connID, sequence, formName)\
		  NWP1PdfScanForm( connID, sequence, formName)

/*===================================================================
  Get each Form in turn; these calls obsolete the ScanForm call
  ===================================================================*/
NWCCODE NWAPI NWP1PdfGetFirstForm(
  DWORD connID,								/* NetWare Server Connection ID		*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR formName,                /* form name								*/
  WORD  NWPTR formNumber,					/* number or NULL                   */
  WORD  NWPTR formLength,					/* length space or NULL             */
  WORD  NWPTR formWidth);					/* width space or NULL					*/

NWCCODE NWAPI NWP1PdfGetNextForm(
  NWPSListHandle handle,               /* Handle for GetNext and EndNext   */
  char  NWPTR formName,                /* form name								*/
  WORD  NWPTR formNumber,					/* number or NULL                   */
  WORD  NWPTR formLength,					/* length space or NULL             */
  WORD  NWPTR formWidth);					/* width space or NULL					*/

#define NWPSPdfGetFirstForm( connType, connID, handlee, formName, N, L, W)\
		  NWP1PdfGetFirstForm( connID, handle, formName, N, L, W)
#define NWPSPdfGetNextForm NWP1PdfGetNextForm

/*======================================================
  Read the form information from the PrintDef database.
  If the form does not exist, an error code is returned.
  If it does exist, the form's name, number, length and
  width are set and a 0 is returned.
  ======================================================*/
NWCCODE NWAPI NWP1PdfReadForm(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR formName,                /* form name								*/
  WORD  NWPTR formNumber,					/* number or NULL                   */
  WORD  NWPTR formLength,					/* length space or NULL             */
  WORD  NWPTR formWidth);					/* width space or NULL					*/

#define NWPSPdfReadForm( connType, connID, formName, formNumber, \
								 formLength, formWidth)\
		  NWP1PdfReadForm( connID, formName, formNumber, formLength, formWidth)

/*===================================================================
  Update the form information. If you don't want to change the
  name, set newFormName to NULL. If you don't want to change a
  parameter set the value to -1.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfUpdateForm(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR oldFormName,             /* old form name							*/
  char  NWPTR newFormName,             /* new form name or NULL				*/
  WORD  formNumber,							/* new form number or -1				*/
  WORD  formLength,							/* new form length or -1				*/
  WORD  formWidth);							/* new form width or -1             */

#define NWPSPdfUpdateForm( connType, connID, oldFormName, newFormName, \
									formNumber, formLength, formWidth)\
		  NWP1PdfUpdateForm( connID, oldFormName, newFormName, formNumber, \
									formLength, formWidth)

/*+++++++++++++++++++++++++++ Device Calls ++++++++++++++++++++++++++++++++*/
/*===================================================================
  Add a device to the PrintDef database. The Device is created
  without any Functions or modes.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfAddDevice(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName);             /* device name                      */

#define NWPSPdfAddDevice( connType, connID, deviceName)\
		  NWP1PdfAddDevice( connID, deviceName)

/*===================================================================
  Delete a device from the PrintDef database.
  When a Device is deleted, all of the device's Functions and Modes
  are also deleted.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfDeleteDevice(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName);             /* device to be removed             */

#define NWPSPdfDeleteDevice( connType, connID, deviceName)\
		  NWP1PdfDeleteDevice( connID, deviceName)

/*===================================================================
  Find a Device in the PrintDef database.
  To find all of the Devices, set sequence to -1 on the first call
  and it will be reset by the call if a Device if found.
  To find a specific Device, set sequence pointer to NULL and set deviceName
  to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfScanDevice(
  DWORD connID,								/* NDS Context handle					*/
  DWORD NWPTR sequence,                /* must give seq. or name				*/
  char  NWPTR deviceName);             /* returned name found					*/

#define NWPSPdfScanDevice( connType, connID, sequence, deviceName)\
		  NWP1PdfScanDevice( connID, sequence, deviceName)

/*===================================================================
  Get each Device in turn; these calls obsolete the ScanDevice call
  ===================================================================*/
NWCCODE NWAPI NWP1PdfGetFirstDevice(
  DWORD connID,								/* NetWare Directory Serv Context	*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR deviceName);             /* returned name found					*/

NWCCODE NWAPI NWP1PdfGetNextDevice(
  NWPSListHandle handle,               /* Handle for GetNext and EndNext   */
  char  NWPTR deviceName);             /* returned name found					*/

#define NWPSPdfGetFirstDevice( connType, connID, handle, deviceName)\
		  NWP1PdfGetFirstDevice( connID, handle, deviceName)
#define NWPSPdfGetNextDevice NWP1PdfGetNextDevice

/*===================================================================
  Find out how many functions and modes are defined for a device.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfReadDevice(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of device                   */
  WORD  NWPTR modeCount,					/* number of modes defined          */
  WORD  NWPTR funcCount);					/* number of func's defined			*/

#define NWPSPdfReadDevice( connType, connID, deviceName, modeCount, \
									funcCount)\
		  NWP1PdfReadDevice( connID, deviceName, modeCount, funcCount)

/*===================================================================
  Change a Device name in the PrintDef database.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfUpdateDevice(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR oldDeviceName,				/* old device name						*/
  char  NWPTR newDeviceName);          /* new device name						*/

#define NWPSPdfUpdateDevice( connType, connID, oldDeviceName, newDeviceName)\
		  NWP1PdfUpdateDevice( connID, oldDeviceName, newDeviceName)

/*+++++++++++++++++++++++++ Mode Calls ++++++++++++++++++++++++++++++++++++*/
/*===================================================================
  Create a new Mode for a specific Device.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfAddMode(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* new mode name							*/

#define NWPSPdfAddMode( connType, connID, deviceName, modeName)\
		  NWP1PdfAddMode( connID, deviceName, modeName)

/*===================================================================
  Delete a Mode from a Device.
  When the Mode is deleted, the Functions are left intact.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfDeleteMode(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* name of mode to remove				*/

#define NWPSPdfDeleteMode( connType, connID, deviceName, modeName)\
		  NWP1PdfDeleteMode( connID, deviceName, modeName)

/*===================================================================
  Find a Mode in the PrintDef database.
  To find all of the Modes, set sequence to -1 on the first call
  and it will be reset by the call if a Mode is found.
  To find a specific Mode, set sequence to NULL and set modeName
  to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfScanMode(
  DWORD connID,								/* NDS Context handle					*/
  DWORD NWPTR sequence,                /* -1 on first call						*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* returned mode name					*/

#define NWPSPdfScanMode( connType, connID, sequence, deviceName, modeName)\
		  NWP1PdfScanMode( connID, sequence, deviceName, modeName)

/*===================================================================
  Get each Mode in turn; these calls obsolete the ScanMode call
  ===================================================================*/
NWCCODE NWAPI NWP1PdfGetFirstMode(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  NWPSListHandle NWPTR handle,         /* Address of handle to keep        */
  char  NWPTR modeName);					/* returned mode name					*/

NWCCODE NWAPI NWP1PdfGetNextMode(
  NWPSListHandle handle,               /* Address of handle to keep        */
  char  NWPTR modeName);					/* returned mode name					*/

#define NWPSPdfGetFirstMode( connType, connID, deviceName, handle, modeName)\
		  NWP1PdfGetFirstMode( connID, deviceName, handle, modeName)
#define NWPSPdfGetNextMode NWP1PdfGetNextMode

/*===================================================================
  Find information on a defined Mode.
  If the Mode is defined, and funcCount is not NULL, the number
  of functions in the mode is returned in funcCount.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfReadMode(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR reqModeName,             /* request mode name                */
  WORD  NWPTR funcCount);					/* function count or NULL				*/

#define NWPSPdfReadMode( connType, connID, deviceName, reqModeName, \
								 funcCount)\
		  NWP1PdfReadMode( connID, deviceName, reqModeName, funcCount)

/*===================================================================
  Change the name of a Mode.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfUpdateMode(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR oldModeName,             /* old mode name							*/
  char  NWPTR newModeName);				/* new mode name or NULL				*/

#define NWPSPdfUpdateMode( connType, connID, deviceName, oldModeName, \
									newModeName)\
		  NWP1PdfUpdateMode( connID, deviceName, oldModeName, newModeName)


/*++++++++++++++++++ Mode-Function Grouping Calls +++++++++++++++++++++++++*/
/*====================================================================
  Add a previously defined function to a previously defined mode list.
  A zero is returned on succecss, or a non-zero error code on failure
  ====================================================================*/
NWCCODE NWAPI NWP1PdfAddModeFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  char  NWPTR funcName,                /* func to add to group             */
  WORD  location);							/* where to insert function or -1	*/

#define NWPSPdfAddModeFunction( connType, connID, deviceName, modeName, \
										  funcName, location)\
		  NWP1PdfAddModeFunction( connID, deviceName, modeName, funcName, \
										  location)

/*==================================================================
  Delete a function from a defined mode list.
  Neither the Mode or Function is deleted from the Device lists.
  A zero is returned on success, or a non-zero error code on failure
  ==================================================================*/
NWCCODE NWAPI NWP1PdfDeleteModeFunction(
  DWORD connID,								/* NDS Context handle					*/
  DWORD sequence,                      /* number of the function to remove 
														from group								*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName);					/* name of associated mode          */

#define NWPSPdfDeleteModeFunction( connType, connID, sequence, deviceName, \
											  modeName)\
		  NWP1PdfDeleteModeFunction( connID, sequence, deviceName, modeName)

/*===================================================================
  Find the name of a Function associated with a Mode.
  To find all of the Functions associated with a Mode, set
  sequence to -1 on the first call and it will be reset by
  the call if a Functions is found.
  To find a specific Function associated with a Mode, set sequence
  to NULL and set functName to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfScanModeFunction(
  DWORD connID,								/* NDS Context handle					*/
  DWORD NWPTR sequence,                /* -1 on first call						*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  char  NWPTR funcName);					/* returned function name				*/

#define NWPSPdfScanModeFunction( connType, connID, sequence, deviceName, \
											modeName, funcName)\
		  NWP1PdfScanModeFunction( connID, sequence, deviceName, modeName, \
											funcName)

/*===================================================================
  Get each Function in turn; these calls obsolete the ScanModeFunction call
  ===================================================================*/
NWCCODE NWAPI NWP1PdfGetFirstModeFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR funcName);					/* returned function name				*/

NWCCODE NWAPI NWP1PdfGetNextModeFunction(
  NWPSListHandle handle,               /* Handle for GetFirst and EndNext  */
  char  NWPTR funcName);					/* returned function name				*/

#define NWPSPdfGetFirstModeFunction( connType, connID, deviceName, modeName,\
                             handle, funcName)\
		  NWP1PdfGetFirstModeFunction( connID, deviceName, modeName, handle, \
                             funcName)
#define NWPSPfdGetNextModeFunction NWP1PfdGetNextModeFunction

/*==================================================================
  Find the Modes which use this Function.
  To find all of the Modes associated with a Function, set
  sequence to -1 on the first call and it will be reset by
  the call if a Mode is found.
  A zero is returned on success, or a non-zero error code on failure
  ==================================================================*/
NWCCODE NWAPI NWP1PdfScanFunctionMode(
  DWORD connID,
  DWORD NWPTR funcSequence,
  DWORD NWPTR modeSequence,
  char  NWPTR deviName,
  char  NWPTR modeName,
  char  NWPTR funcName);

#define NWPSPdfScanFunctionMode( connType, connID, funcSequence, \
											modeSequence, deviName, modeName, funcName)\
		  NWP1PdfScanFunctionMode( connID, funcSequence, modeSequence, \
											deviName, modeName, funcName)

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

NWCCODE NWAPI NWP1PdfReadModeFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR modeName,                /* name of associated mode          */
  /*-------- no function name because this is a mode group ----------------*/
  WORD  funcOffset,							/* number of bytes to skip          */
  WORD  NWPTR funcSize,                /* req:buff size; ret:# read			*/
  BYTE  NWPTR funcString);             /* buffer for read block				*/


#define NWPSPdfReadModeFunction( connType, connID, deviceName, modeName, \
											funcOffset, funcSize, funcString)\
		  NWP1PdfReadModeFunction( connID, deviceName, modeName, funcOffset, \
											funcSize, funcString)

/*+++++++++++++++++++++++++++ Function Calls ++++++++++++++++++++++++++++++*/
/*===================================================================
  Add a Function to a Device.
  funcSize should be set to the number of bytes in funcString.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfAddFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName,                /* name of func. to add             */
  WORD  funcSize,                      /* count of bytes in function       */
  BYTE  NWPTR funcString);             /* list of bytes in function			*/

#define NWPSPdfAddFunction( connType, connID, deviceName, funcName, \
									 funcSize, funcString)\
		  NWP1PdfAddFunction( connID, deviceName, funcName, funcSize, \
									 funcString)

/*===================================================================
  Delete a function from the Device in the PrintDef database.
  If the Function is refered to in any Mode, the Function
  will be automaticly deleted from the Mode as well.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfDeleteFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName);					/* name of func. to delete          */

#define NWPSPdfDeleteFunction( connType, connID, deviceName, funcName)\
		  NWP1PdfDeleteFunction( connID, deviceName, funcName)

/*===================================================================
  Find a Function defined for a specific Device.
  To find all of the Functions associated with a Device, set
  sequence to -1 on the first call and it will be reset by
  the call if a Functions is found.
  To find a specific Function associated with a Device, set sequence
  to NULL and set functName to the desired search name.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfScanFunction(
  DWORD connID,								/* NDS Context handle					*/
  DWORD NWPTR sequence,                /* -1 on first call						*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName);					/* name of next function				*/

#define NWPSPdfScanFunction( connType, connID, sequence, deviceName, \
									  funcName)\
		  NWP1PdfScanFunction( connID, sequence, deviceName, funcName)

/*===================================================================
  Get each Function in turn; these calls obsolete the ScanFunction call
  ===================================================================*/
NWCCODE NWAPI NWP1PdfGetFirstFunction(
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR deviceName,					/* name of associated device			*/
  NWPSListHandle NWPTR handle,         /* Handle for GetNext and EndNext   */
  char  NWPTR funcName, 					/* returned function name				*/
  NWPSPdfFuncData NWPTR funcData);     /* returned function data struct    */

NWCCODE NWAPI NWP1PdfGetNextFunction(
  NWPSListHandle handle,               /* Handle for GetFirst and EndNext  */
  char  NWPTR funcName, 					/* returned function name				*/
  NWPSPdfFuncData NWPTR funcData);     /* returned function data struct    */

#define NWPSPdfGetFirstFunction( connType, connID, deviceName, handle,\
									  funcName, funcData)\
		  NWP1PdfGetFirstFunction( connID, deviceName, handle, \
                             funcName, funcData)
#define NWPSPfdGetNextFunction NWP1PfdGetNextFunction

/*===================================================================
  Read the byte string associated with a function.
  funcSize should be set equal to the size of the buffer funcString.
  funcSize will be set to the actual size of the byte string if the
  call is successful.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/

NWCCODE NWAPI NWP1PdfReadFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR funcName,                /* name of the function             */
  WORD  funcOffset,							/* bytes to skip past					*/
  WORD  NWPTR funcSize,                /* req: buff size; ret:# read       */
  BYTE  NWPTR funcString);             /* byte list or NULL                */


#define NWPSPdfReadFunction( connType, connID, deviceName, funcName, \
									  funcOffset, funcSize, funcString)\
		  NWP1PdfReadFunction( connID, deviceName, funcName, funcOffset, \
									  funcSize, funcString)

/*===================================================================
  Change the function string assigned to a function name.
  To change the function's name, set newFuncName to a new name.
  To leave the Function name the same, set newFuncName to NULL.
  To change the function string, set funcSize to the number of
  bytes in the funcString and set funcString to point to the
  new string. To leave the string the same, set funcSize to -1.
  A zero is returned on succecss, or a non-zero error code on failure
  ===================================================================*/
NWCCODE NWAPI NWP1PdfUpdateFunction(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR deviceName,					/* name of associated device			*/
  char  NWPTR oldFuncName,             /* current function name				*/
  char  NWPTR newFuncName,             /* new name or NULL						*/
  WORD  funcSize,                      /* number of bytes or -1				*/
  BYTE  NWPTR funcString);             /* byte list or NULL                */

#define NWPSPdfUpdateFunction( connType, connID, deviceName, oldFuncName, \
										 newFuncName, funcSize, funcString)\
		  NWP1PdfUpdateFunction( connID, deviceName, oldFuncName, \
										 newFuncName, funcSize, funcString)

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
NWCCODE NWAPI NWP1PdfManagedImportDevice(
  DWORD connID,								/* NetWare Server Connection ID		*/
  char  NWPTR fileName,                /* name of .PDF file                */
  char  NWPTR deviceName,					/* new device name or NULL          */
  NWPSImportManager importManagerFunc, /* pointer to the Manager Function	*/
  void  NWPTR importManagerParm1);		/* caller's Manager context variable*/

#define NWPSPdfImportDevice( connType, connID, fileName, deviceName)\
		  NWP1PdfManagedImportDevice( connID, fileName, deviceName, NULL, NULL)

#define NWPSPdfManagedImportDevice( connType, connID, fileName, deviceName,\
												importManagerFunc, importManagerParm1)\
		  NWP1PdfManagedImportDevice( connID, fileName, deviceName, \
												importManagerFunc, importManagerParm1)

/*===============================================
  Export a device to a Pdf file from the database
  ===============================================*/
NWCCODE NWAPI NWP1PdfExportDevice(
  DWORD connID,								/* NDS Context handle					*/
  char  NWPTR fileName,                /* name of .PDF file                */
  char  NWPTR deviceName);             /* name of device to export			*/

#define NWPSPdfExportDevice( connType, connID, fileName, deviceName)\
		  NWP1PdfExportDevice( connID, fileName, deviceName)

/*=========================================================
  Delete the Directory Services "Printer Control" Attribute
  at the given context.
  =========================================================*/
NWCCODE NWAPI NWP1PdfDeleteDatabase(
  DWORD connID);								/* NDS Context handle					*/

#define NWPSPdfDeleteDatabase( connType, connID)\
		  NWP1PdfDeleteDatabase( connID)

#ifdef __cplusplus
}
#endif
