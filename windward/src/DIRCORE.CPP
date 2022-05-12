//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// Written by David Thielen
//   Copyright 1994, All Rights Reserved


#include "stdafx.h"
#include "direct.h"
#include "dos.h"

#include "_windwrd.h"


extern "C"
{
void __splitpath (const char *path, char *drive, char *dir, char *fname, char *ext);
}


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDir

void CDir::ctor (char const *pDir)
{
char sBuf [_MAX_PATH];

	// If NULL then we get the default dir
	if ((pDir == NULL) || (*pDir == 0))
		{
		if (_getcwd (sBuf, _MAX_PATH) == NULL)
			{
			ASSERT (FALSE);
			CFileException::ThrowOsError (errno);
			}
		sPathName = sBuf;
		sPathName.MakeUpper ();
		bIncFile = FALSE;
		return;
		}

	ASSERT (AfxIsValidString (pDir));

	// Ok - make it a fully qualified path
	if (_fullpath (sBuf, pDir, _MAX_PATH) == NULL)
		{
		ASSERT (FALSE);
		CFileException::ThrowOsError (errno);
		}
	sPathName = sBuf;
	sPathName.MakeUpper ();

	// Lets see if it includes a file
	bIncFile = (strchr (sPathName, '.') == NULL) ? FALSE : TRUE;
}

// This assumes we have just a path - no file name
void CDir::AddFile (char const *pFile)
{

	// BUGBUG - what about pFile=NULL -> remove file?
	ASSERT_VALID (this);
	ASSERT (bIncFile == FALSE);
	ASSERT (pFile != NULL);

	// add trailing \ if needed
	if (sPathName.IsEmpty ())
		sPathName = '\\';
	else
		if (sPathName [sPathName.GetLength () - 1] != '\\')
			sPathName += '\\';

	sPathName += pFile;
	sPathName.MakeUpper ();
	bIncFile = TRUE;
}

unsigned CDir::ChDir (BOOL bChDrive)
{
CString sDir = GetDir (TRUE, FALSE);
CString sDrv = GetDrive ();

	ASSERT_VALID (this);
	ASSERT (! bChDrive || ! sDrv.IsEmpty ());
	ASSERT (! sDir.IsEmpty ());

	// Change to the drive
	if (bChDrive)
		if (_chdrive (toupper (sDrv[0]) - 'A' + 1) != 0)
			return (errno);

	// If no dir - thats an error
	if (sDir.IsEmpty ())
		return (-1);

	// Change the directory
	if ((sDir[1] == ':') && (sDir.GetLength() == 2))
		sDir += '\\';
	if (_chdir (sDir) != 0)
		return (errno);

	return (0);
}

BOOL CDir::DoesExist ()
{
unsigned uRtn;

	ASSERT_VALID (this);

	// we want dir\*.* so we strip off the file name
	CString sPath = GetDir (TRUE, TRUE) + "*.*";

	struct _find_t findInfo;
	uRtn = _dos_findfirst (sPath, 0xFF, &findInfo);

	if ((uRtn == 0) || (uRtn == 12))
		return (TRUE);
	return (FALSE);
}

int CDir::FindAll (FILEFINDPROC fnEnum, void *pvData, unsigned uAtr)
{

	ASSERT_VALID (this);
	ASSERT (fnEnum != NULL);
	ASSERT (bIncFile == TRUE);
	ASSERT (sPathName.GetLength () <= _MAX_PATH);

	unsigned uRtn;
	struct _find_t findInfo;
	uRtn = _dos_findfirst (sPathName, uAtr, &findInfo);

	while (uRtn == 0)
	  {
		CFileStatus fs;
		if (CFile::GetStatus (GetDir (TRUE, TRUE) + findInfo.name, fs) == 0)
		  {
#ifdef _DEBUG
findInfo.name[12] = 0;
TRACE ("FindAll(%s) GetStatus failed\n", findInfo.name);
#endif
			uRtn = 0x02;
		  break;
		  }

	  if ((uRtn = (*fnEnum) (fs, pvData)) != 0)
			return (uRtn);
		uRtn = _dos_findnext (&findInfo);
	  }

#ifdef _DEBUG
	if ((uRtn != 3) && (uRtn != 0x12))
		{
		findInfo.name[12] = 0;
		TRACE ("FindAll(%s) = 0x%X\n", findInfo.name, uRtn);
		}
#endif

	return (0);
}

CString CDir::GetDir (BOOL bIncDrv, BOOL bIncEndSlash)
{
char sDir [_MAX_DIR];
CString sRtn;

	ASSERT_VALID (this);

	if (bIncFile)
		{
		if (bIncDrv)
			{
			char sDrv [_MAX_DIR];
			__splitpath (sPathName, sDrv, sDir, NULL, NULL);
			ASSERT (sDrv[0] != 0);
			sRtn = CString (sDrv) + sDir;
			}
		else
			{
			__splitpath (sPathName, NULL, sDir, NULL, NULL);
			sRtn = sDir;
			}
		}

	// All we have is the drive/dir
	else
		{
		if (bIncDrv)
			sRtn = sPathName;
		else
			{
			char sFile [_MAX_FNAME];
			__splitpath (sPathName, NULL, sDir, sFile, NULL);
			sRtn = sDir + CString ('\\') + sFile;
			}
		}


	// get rid of the trailing '\'
	int iLen = sRtn.GetLength ();
	ASSERT (iLen > 0);
	if (sRtn [iLen - 1] == '\\')
		sRtn = sRtn.Left (iLen - 1);

	// Now add if required
	if (bIncEndSlash)
		sRtn += '\\';

	return (sRtn);
}

CString CDir::GetDrive (BOOL bIncSemi)
{
char sDrv [_MAX_DIR];

	ASSERT_VALID (this);

	__splitpath (sPathName, sDrv, NULL, NULL, NULL);
	ASSERT (sDrv[0] != 0);

	// May be network share
	if (! bIncSemi)
		{
		char *pEnd = &sDrv [strlen (sDrv) - 1];
		if ((*pEnd == ':') || (*pEnd == '\\'))
			*pEnd = 0;
		}

	return (sDrv);
}

CString CDir::GetExt (BOOL bIncPer)
{

	ASSERT_VALID (this);

	int iInd = sPathName.Find ('.');
	if (iInd < 0)
		return (CString ());

	if (! bIncPer)
		iInd++;

	return (sPathName.Mid (iInd));
}

CString CDir::GetFile ()
{
char sBase [_MAX_FNAME];
char sExt [_MAX_EXT];

	ASSERT_VALID (this);

	if (! bIncFile)
		return (CString (""));

	__splitpath (sPathName, NULL, NULL, sBase, sExt);
	ASSERT (sBase[0] != 0);

	return (CString (CString (sBase) + sExt));
}

// returns the path relative to the path passed in.
// if no path passed in, uses the default dir
CString CDir::GetRelPath (char const *pRelDir)
{
char sBuf [_MAX_PATH];

	ASSERT_VALID (this);

	// if no path set to the default dir
	if (pRelDir == NULL)
		{
		if (_getcwd (sBuf, _MAX_PATH) == NULL)
			{
			ASSERT (FALSE);
			CFileException::ThrowOsError (errno);
			}
		}
	else
		strcpy (sBuf, pRelDir);
	_strupr (sBuf);

	char *pPath = sPathName.GetBuffer (0);
	CString sRtn = "";
	char *pPathOn = pPath, *pBufOn = sBuf;

	// if we have nothing - return nothing
	if (sPathName.GetLength () == 0)
		goto Dne;

	// This is easy - compare and return starting where they differ
	// We have to go back at a difference to the nearest \, /, or :
	while ((*pPathOn == *pBufOn) && (*pPathOn != 0))
		{
		pPathOn++;
		pBufOn++;
		}
	// if the match fully return nothing
	if ((*pPathOn == *pBufOn) && (*pPathOn == 0))
		goto Dne;

	// if the drives are different - return full path
	if ((pPathOn == pPath) && (*(pPath+1) == ':'))
		return (sPathName);

	// if net shares are different return full path
	if (sPathName.GetLength () >= 2)
		if ((*pPath == '\\') && (*(pPath+1) == '\\'))
		  {
	  	char *pSlsh = strchr (pPath+2, '\\');
			if (pSlsh == NULL)
				return (sPathName);
	  	pSlsh = strchr (pSlsh+1, '\\');
			if ((pSlsh == NULL) || (pSlsh >= pPathOn))
				return (sPathName);
	  	}

	// If we are at the end of pBufOn then we just return the rest
	if ((*pBufOn == 0) && (*pPathOn == '\\'))
		{
		sRtn = pPathOn + 1;
		goto Dne;
		}

	// Lets find the last dir they matched on
	//  ie c:\abcd\efgh matches c:\abcd\efyz with c:\abcd\, not c:\abcd\ef
	while ((*pBufOn != '\\') && (*pBufOn != '/') && (*pBufOn != ':') && (pBufOn > sBuf))
		{
		pPathOn--;
		pBufOn--;
		}

	// If only the drive is the same we return the dir from the root
	if (*pBufOn == ':')
		{
		sRtn = pPathOn + 1;
		goto Dne;
		}

	// Ok - we now have to figure our how many ..\ to add
	// one for each dir left in pBufOn+1
	if (*pBufOn != 0)
		sRtn += "..\\";
	while ((pBufOn = strchr (pBufOn+1, '\\')) != NULL)
		sRtn += "..\\";
	if (*pPathOn != 0)
		sRtn += pPathOn + 1;

Dne:
	sPathName.ReleaseBuffer (-1);
	return (sRtn);
}

BOOL CDir::IsLocal ()
{

	CString sDrv = GetDrive (TRUE);
	return (sDrv[1] == ':');
}

unsigned CDir::MkDir ()
{
CString sDir = GetDir (TRUE, FALSE);

	ASSERT_VALID (this);
	ASSERT (! sDir.IsEmpty ());

	// If no dir - thats an error
	if (sDir.IsEmpty ())
		return (-1);

	// starting with the first dir, make them all
	char *pPath = sDir.GetBuffer (0);
	char *pPathOn = strchr (pPath, '\\');			// skip root
	if (pPathOn != NULL)
		while ((pPathOn = strchr (pPathOn+1, '\\')) != NULL)
		  {
	  	*pPathOn = 0;
			if (_mkdir (pPath) != 0)
				if (errno != 0x0D)
					return (errno);
			*pPathOn = '\\';
		  }

	if (_mkdir (sDir) != 0)
		if (errno != 0x0D)
			return (errno);

	return (0);
}

#ifdef _DEBUG
#pragma check_stack (on)
#endif
int CDir::NextSub (const char *pDir, WALKDIRPROC fnEnum, void *pvData)
{
struct _find_t fi;
unsigned uRtn;

	ASSERT (strlen (pDir) <= _MAX_DIR);
	CString sDir (pDir);
	uRtn = _dos_findfirst (sDir + "*.*", _A_SUBDIR, &fi);

	while (uRtn == 0)
		{
		// Get the next dir (other than . & ..)
		while ((uRtn == 0) && ((strcmp (fi.name, ".") == 0) ||
					(strcmp (fi.name, "..") == 0) || (! (fi.attrib & _A_SUBDIR))))
			uRtn = _dos_findnext (&fi);

		// are we done here?
		if (uRtn != 0)
			return (0);

		// We found one
		CString sName = sDir + fi.name;
		if (bIncFile)
			sName += "\\" + GetFile ();
		CDir dirTmp (sName);
		if ((uRtn = (*fnEnum) (dirTmp, pvData)) != 0)
			return (uRtn);

		// walk that sub
		if ((uRtn = NextSub (dirTmp.GetDir (TRUE, TRUE), fnEnum, pvData)) != 0)
			return (uRtn);

		// next file (insure dir at top of while)
		uRtn = _dos_findnext (&fi);
		}

	return (0);
}

// Walk all subdirectories & the one we are in
int CDir::WalkDirs (WALKDIRPROC fnEnum, void *pvData)
{
unsigned uRtn;

	// dir we are in
	CDir dirTmp (*this);
	if ((uRtn = (*fnEnum) (dirTmp, pvData)) != 0)
		return (uRtn);

	// Get the first sub-dir & return
	return (NextSub (GetDir (TRUE, TRUE), fnEnum, pvData));
}
#ifdef _DEBUG
#pragma check_stack ()
#endif

CString CDir::SetName (char const *pDrv, char const *pDir, char const *pFile)
{
CString sRtn;

	// if drive, put in drive and :
	if ((pDrv == NULL) || (*pDrv == 0))
		sRtn = "";
	else
		sRtn = pDrv + ':';

	// add path
	sRtn += pDir;

	// add trailing \ if needed
	if (sRtn.IsEmpty ())
		sRtn = '\\';
	else
		if (sRtn [sRtn.GetLength () - 1] != '\\')
			sRtn += '\\';

	// Add the file name - make sure it has a .
	sRtn += pFile;
	if (strchr (pFile, '.') == NULL)
		sRtn += ".";
	sRtn.MakeUpper ();

	return (sRtn);
}


#ifdef _DEBUG

void CDir::AssertValid () const
{

	// assert base object
	CObject::AssertValid ();

	// assert members
//BUGBUG	ASSERT_VALID_CLASS (&sPathName);

	// assert path
	if (sPathName[0] != '\\')
		{
		ASSERT (sPathName.GetLength () >= 3);
		ASSERT (sPathName[1] == ':');
		ASSERT (sPathName[2] == '\\');
		}
	else
		ASSERT (sPathName[1] == '\\');
}

void CDir::Dump (CDumpContext& dc) const
{

	ASSERT_VALID (this);

	CObject::Dump(dc);
	AFX_DUMP1 (dc, " with path ", (char const *) sPathName);
}

#endif

