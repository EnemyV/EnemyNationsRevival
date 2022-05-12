// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"

#ifdef _MAC
#include <macname1.h>
#include <macos\files.h>
#include <macos\folders.h>
#include <macname2.h>
#include <plstring.h>

// from winlm.h.  Included here so we don't have to include codefrag.h
extern "C" BOOL WINAPI GetMacInstanceInformationEx(HINSTANCE hInstance,
	long* pcid, short* prn, char *szFragment, int cchFragment);

STDAPI EnsureTypeLibFolder(void);
HRESULT TypeLibFspFromFullPath(const char *szPathName, FSSpec *pfspTlb);
OSErr FullPathFromFSSpec(char *szFileName, FSSpec *pfsp);
HRESULT _LoadTypeLibGetMacPath(const char *szPathName, char *szMacPath, ITypeLib **ppTypeLib);
#endif //_MAC

#ifdef AFXCTL_FACT_SEG
#pragma code_seg(AFXCTL_FACT_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

#define GUID_CCH    39  // Characters in string form of guid, including '\0'

#define ERROR_BADKEY_WIN16  2   // needed when running on Win32s

inline BOOL _AfxRegDeleteKeySucceeded(LONG error)
{
	return (error == ERROR_SUCCESS) || (error == ERROR_BADKEY) ||
		(error == ERROR_FILE_NOT_FOUND);
}

// Under Win32, a reg key may not be deleted unless it is empty.
// Thus, to delete a tree,  one must recursively enumerate and
// delete all of the sub-keys.

LONG AFXAPI _AfxRecursiveRegDeleteKey(HKEY hParentKey, LPTSTR szKeyName)
{
	DWORD   dwIndex = 0L;
	TCHAR   szSubKeyName[256];
	HKEY    hCurrentKey;
	DWORD   dwResult;

	if ((dwResult = RegOpenKey(hParentKey, szKeyName, &hCurrentKey)) ==
		ERROR_SUCCESS)
	{
		// Remove all subkeys of the key to delete
		while ((dwResult = RegEnumKey(hCurrentKey, 0, szSubKeyName, 255)) ==
			ERROR_SUCCESS)
		{
			if ((dwResult = _AfxRecursiveRegDeleteKey(hCurrentKey,
				szSubKeyName)) != ERROR_SUCCESS)
				break;
		}

		// If all went well, we should now be able to delete the requested key
		if ((dwResult == ERROR_NO_MORE_ITEMS) || (dwResult == ERROR_BADKEY) ||
			(dwResult == ERROR_BADKEY_WIN16))
		{
			dwResult = RegDeleteKey(hParentKey, szKeyName);
		}
	}

	RegCloseKey(hCurrentKey);
	return dwResult;
}

void _AfxUnregisterInterfaces(ITypeLib* pTypeLib)
{
	TCHAR szKey[128];
	_tcscpy(szKey, _T("Interface\\"));
	LPTSTR pszGuid = szKey + _tcslen(szKey);

	int cTypeInfo = pTypeLib->GetTypeInfoCount();

	for (int i = 0; i < cTypeInfo; i++)
	{
		TYPEKIND tk;
		if (SUCCEEDED(pTypeLib->GetTypeInfoType(i, &tk)) &&
			(tk == TKIND_DISPATCH || tk == TKIND_INTERFACE))
		{
			ITypeInfo* pTypeInfo = NULL;
			if (SUCCEEDED(pTypeLib->GetTypeInfo(i, &pTypeInfo)))
			{
				TYPEATTR* pTypeAttr;
				if (SUCCEEDED(pTypeInfo->GetTypeAttr(&pTypeAttr)))
				{
#if defined(_UNICODE) || defined(OLE2ANSI)
					StringFromGUID2(pTypeAttr->guid, pszGuid, GUID_CCH);
#else
					WCHAR wszGuid[39];
					StringFromGUID2(pTypeAttr->guid, wszGuid, GUID_CCH);
					_wcstombsz(pszGuid, wszGuid, GUID_CCH);
#endif
					_AfxRecursiveRegDeleteKey(HKEY_CLASSES_ROOT, szKey);
					pTypeInfo->ReleaseTypeAttr(pTypeAttr);
				}

				pTypeInfo->Release();
			}
		}
	}
}

BOOL AFXAPI AfxOleRegisterTypeLib(HINSTANCE hInstance, REFGUID tlid,
	LPCTSTR pszFileName, LPCTSTR pszHelpDir)
{
	USES_CONVERSION;

	BOOL bSuccess = FALSE;
	CString strPathName;
	TCHAR *szPathName = strPathName.GetBuffer(_MAX_PATH);
	::GetModuleFileName(hInstance, szPathName, _MAX_PATH);
#ifndef _MAC
	strPathName.ReleaseBuffer();
#endif

	LPTYPELIB ptlib = NULL;

	// If a filename was specified, replace final component of path with it.
	if (pszFileName != NULL)
	{
#ifdef _MAC
		FSSpec fsp;
		UnwrapFile(szPathName, &fsp);
		lstrcpy((char*)&fsp.name+1, pszFileName);
		*fsp.name = (BYTE)strlen(pszFileName);
		WrapFile(&fsp, szPathName, sizeof(szPathName));
	}

	EnsureTypeLibFolder();
	HRESULT hr = _LoadTypeLibGetMacPath(szPathName, szPathName, &ptlib);
	strPathName.ReleaseBuffer();
	if (SUCCEEDED(hr))
#else
		int iBackslash = strPathName.ReverseFind('\\');
		if (iBackslash != -1)
			strPathName = strPathName.Left(iBackslash+1);
		strPathName += pszFileName;
	}

	if (SUCCEEDED(LoadTypeLib(T2COLE(strPathName), &ptlib)))
#endif
	{
		ASSERT_POINTER(ptlib, ITypeLib);

		LPTLIBATTR pAttr;
		GUID tlidActual = GUID_NULL;

		if (SUCCEEDED(ptlib->GetLibAttr(&pAttr)))
		{
			ASSERT_POINTER(pAttr, TLIBATTR);
			tlidActual = pAttr->guid;
			ptlib->ReleaseTLibAttr(pAttr);
		}

		// Check that the guid of the loaded type library matches
		// the tlid parameter.
		ASSERT(IsEqualGUID(tlid, tlidActual));

		if (IsEqualGUID(tlid, tlidActual))
		{
			// Register the type library.
			if (SUCCEEDED(RegisterTypeLib(ptlib,
					T2OLE((LPTSTR)(LPCTSTR)strPathName), T2OLE((LPTSTR)pszHelpDir))))
				bSuccess = TRUE;
		}

		RELEASE(ptlib);
	}
	else
	{
		TRACE1("Warning: Could not load type library from %s\n", (LPCTSTR)strPathName);
	}


	return bSuccess;
}

#define TYPELIBWIN   _T("win32")
#define TYPELIBWIN_2 _T("win16")

BOOL AFXAPI AfxOleUnregisterTypeLib(REFGUID tlid, WORD wVerMajor,
	WORD wVerMinor, LCID lcid)
{
	USES_CONVERSION;

	// Load type library before unregistering it.
	ITypeLib* pTypeLib = NULL;
	if (wVerMajor != 0)
	{
		if (FAILED(LoadRegTypeLib(tlid, wVerMajor, wVerMinor, lcid, &pTypeLib)))
			pTypeLib = NULL;
	}

	// Format typelib guid as a string
	OLECHAR szTypeLibID[GUID_CCH];
	int cchGuid = ::StringFromGUID2(tlid, szTypeLibID, GUID_CCH);

	ASSERT(cchGuid == GUID_CCH);    // Did StringFromGUID2 work?
	if (cchGuid != GUID_CCH)
		return FALSE;

	TCHAR szKeyTypeLib[_MAX_PATH];
	BOOL bSurgical = FALSE;
	LONG error = ERROR_SUCCESS;

	wsprintf(szKeyTypeLib, _T("TYPELIB\\%s"), OLE2CT(szTypeLibID));

	HKEY hKeyTypeLib;
	if (RegOpenKey(HKEY_CLASSES_ROOT, szKeyTypeLib, &hKeyTypeLib) ==
		ERROR_SUCCESS)
	{
		int iKeyVersion = 0;
		HKEY hKeyVersion;
		TCHAR szVersion[_MAX_PATH];

		// Iterate through all installed versions of the control

		while (RegEnumKey(hKeyTypeLib, iKeyVersion, szVersion, _MAX_PATH) ==
			ERROR_SUCCESS)
		{
			hKeyVersion = NULL;
			BOOL bSurgicalVersion = FALSE;

			if (RegOpenKey(hKeyTypeLib, szVersion, &hKeyVersion) !=
				ERROR_SUCCESS)
			{
				++iKeyVersion;
				continue;
			}

			int iKeyLocale = 0;
			HKEY hKeyLocale;
			TCHAR szLocale[_MAX_PATH];

			// Iterate through all registered locales for this version

			while (RegEnumKey(hKeyVersion, iKeyLocale, szLocale, _MAX_PATH) ==
				ERROR_SUCCESS)
			{
				// Don't remove HELPDIR or FLAGS keys.
				if ((_tcsicmp(szLocale, _T("HELPDIR")) == 0) ||
					(_tcsicmp(szLocale, _T("FLAGS")) == 0))
				{
					++iKeyLocale;
					continue;
				}

				hKeyLocale = NULL;

				if (RegOpenKey(hKeyVersion, szLocale, &hKeyLocale) !=
					ERROR_SUCCESS)
				{
					++iKeyLocale;
					continue;
				}

				// Check if a 16-bit key is found when unregistering 32-bit
				HKEY hkey;
				if (RegOpenKey(hKeyLocale, TYPELIBWIN_2, &hkey) ==
					ERROR_SUCCESS)
				{
					RegCloseKey(hkey);

					// Only remove the keys specific to the 32-bit version
					// of control, leaving things intact for 16-bit version.
					error = _AfxRecursiveRegDeleteKey(hKeyLocale, TYPELIBWIN);
					bSurgicalVersion = TRUE;
					RegCloseKey(hKeyLocale);
				}
				else
				{
					// Delete everything for this locale.
					RegCloseKey(hKeyLocale);
					if (_AfxRecursiveRegDeleteKey(hKeyVersion, szLocale) ==
						ERROR_SUCCESS)
					{
						// Start over again, so we don't skip anything.
						iKeyLocale = 0;
						continue;
					}
				}
				++iKeyLocale;
			}
			RegCloseKey(hKeyVersion);

			if (bSurgicalVersion)
			{
				bSurgical = TRUE;
			}
			else
			{
				if (_AfxRecursiveRegDeleteKey(hKeyTypeLib, szVersion) ==
					ERROR_SUCCESS)
				{
					// Start over again, to make sure we don't skip anything.
					iKeyVersion = 0;
					continue;
				}
			}

			++iKeyVersion;
		}
		RegCloseKey(hKeyTypeLib);
	}

	if (!bSurgical)
		error = _AfxRecursiveRegDeleteKey(HKEY_CLASSES_ROOT, szKeyTypeLib);

	if (_AfxRegDeleteKeySucceeded(error))
	{
		// If type library was unregistered successfully, then also unregister
		// interfaces.
		if (pTypeLib != NULL)
		{
			ITypeLib* pDummy = NULL;
			if (FAILED(LoadRegTypeLib(tlid, wVerMajor, wVerMinor, lcid, &pDummy)))
				_AfxUnregisterInterfaces(pTypeLib);
			else
				pDummy->Release();

			pTypeLib->Release();
		}
	}

	return _AfxRegDeleteKeySucceeded(error);
}

static const LPCTSTR rglpszCtrlProgID[] =
{
	_T("\0") _T("%1"),
	_T("CLSID\0") _T("%2"),
	NULL
};

#ifdef _MAC
#define INPROCSERVER   _T("InprocServer")
#define INPROCSERVER_2 _T("InprocServer")
#define TOOLBOXBITMAP  _T("ToolboxBitmap")
#else
#define INPROCSERVER   _T("InprocServer32")
#define INPROCSERVER_2 _T("InprocServer")
#define TOOLBOXBITMAP  _T("ToolboxBitmap32")
#endif

static const LPCTSTR rglpszCtrlClassID[] =
{
	_T("\0") _T("%1"),
	_T("ProgID\0") _T("%2"),
	INPROCSERVER _T("\0%3"),
	TOOLBOXBITMAP _T("\0%3, %4"),
	_T("MiscStatus\0") _T("0"),
	_T("MiscStatus\\1\0") _T("%5"),
	_T("Control\0") _T(""),
	_T("TypeLib\0") _T("%6"),
	_T("Version\0") _T("%7"),
	NULL
};

BOOL AFXAPI AfxOleRegisterControlClass(HINSTANCE hInstance,
	REFCLSID clsid, LPCTSTR pszProgID, UINT idTypeName, UINT idBitmap,
	int nRegFlags, DWORD dwMiscStatus, REFGUID tlid, WORD wVerMajor,
	WORD wVerMinor)
{
	USES_CONVERSION;

	BOOL bSuccess = FALSE;

	// Format class ID as a string
	OLECHAR szClassID[GUID_CCH];
	int cchGuid = ::StringFromGUID2(clsid, szClassID, GUID_CCH);
	LPCTSTR lpszClassID = OLE2CT(szClassID);

	ASSERT(cchGuid == GUID_CCH);    // Did StringFromGUID2 work?
	if (cchGuid != GUID_CCH)
		return FALSE;

	// Format typelib guid as a string
	OLECHAR szTypeLibID[GUID_CCH];
	cchGuid = ::StringFromGUID2(tlid, szTypeLibID, GUID_CCH);

	ASSERT(cchGuid == GUID_CCH);    // Did StringFromGUID2 work?
	if (cchGuid != GUID_CCH)
		return FALSE;

	CString strPathName;
	AfxGetModuleShortFileName(hInstance, strPathName);

	CString strTypeName;
	if (!strTypeName.LoadString(idTypeName))
	{
		ASSERT(FALSE);  // Name string not present in resources
		strTypeName = lpszClassID; // Use Class ID instead
	}

	TCHAR szBitmapID[_MAX_PATH];
	_itot(idBitmap, szBitmapID, 10);

	TCHAR szMiscStatus[_MAX_PATH];
	_ltot(dwMiscStatus, szMiscStatus, 10);

	// Format version string as "major.minor"
	TCHAR szVersion[_MAX_PATH];
	wsprintf(szVersion, _T("%d.%d"), wVerMajor, wVerMinor);

	// Attempt to open registry keys.
	HKEY hkeyClassID = NULL;
	HKEY hkeyProgID = NULL;

	TCHAR szScratch[_MAX_PATH];
	wsprintf(szScratch, _T("CLSID\\%s"), lpszClassID);
	if (::RegCreateKey(HKEY_CLASSES_ROOT, szScratch, &hkeyClassID) !=
		ERROR_SUCCESS)
		goto Error;
	if (::RegCreateKey(HKEY_CLASSES_ROOT, pszProgID, &hkeyProgID) !=
		ERROR_SUCCESS)
		goto Error;

	ASSERT(hkeyClassID != NULL);
	ASSERT(hkeyProgID != NULL);

	LPCTSTR rglpszSymbols[7];
	rglpszSymbols[0] = strTypeName;
	rglpszSymbols[1] = lpszClassID;
	bSuccess = AfxOleRegisterHelper(rglpszCtrlProgID, rglpszSymbols, 2,
		TRUE, hkeyProgID);

	if (!bSuccess)
		goto Error;

	rglpszSymbols[1] = pszProgID;
#ifndef _MAC
	rglpszSymbols[2] = strPathName;
#else
	// get the code fragment name, which Mac OLE uses to load dlls
	GetMacInstanceInformationEx(hInstance, NULL, NULL, szScratch, sizeof(szScratch));
	rglpszSymbols[2] = szScratch;
#endif
	rglpszSymbols[3] = szBitmapID;
	rglpszSymbols[4] = szMiscStatus;
	rglpszSymbols[5] = OLE2CT(szTypeLibID);
	rglpszSymbols[6] = szVersion;
	bSuccess = AfxOleRegisterHelper(rglpszCtrlClassID, rglpszSymbols, 7,
		TRUE, hkeyClassID);

	if (!bSuccess)
		goto Error;

	if (nRegFlags & afxRegInsertable)
	{
		bSuccess =
			(::RegSetValue(hkeyProgID, _T("Insertable"), REG_SZ, _T(""), 0) ==
				ERROR_SUCCESS) &&
			(::RegSetValue(hkeyClassID, _T("Insertable"), REG_SZ, _T(""), 0) ==
				ERROR_SUCCESS);
	}

#ifndef _MAC
	if (nRegFlags & afxRegApartmentThreading)
	{
		HKEY hkeyInprocServer32;
		bSuccess = (::RegOpenKey(hkeyClassID, INPROCSERVER,
			&hkeyInprocServer32) == ERROR_SUCCESS);
		if (!bSuccess)
			goto Error;
		ASSERT(hkeyInprocServer32 != NULL);
		static TCHAR szApartment[] = _T("Apartment");
		bSuccess = (::RegSetValueEx(hkeyInprocServer32, _T("ThreadingModel"), 0,
			REG_SZ, (const BYTE*)szApartment, (lstrlen(szApartment)+1) * sizeof(TCHAR)) ==
			ERROR_SUCCESS);
		::RegCloseKey(hkeyInprocServer32);
	}
#endif

Error:
	if (hkeyProgID != NULL)
		::RegCloseKey(hkeyProgID);

	if (hkeyClassID != NULL)
		::RegCloseKey(hkeyClassID);

	return bSuccess;
}

BOOL AFXAPI AfxOleUnregisterClass(REFCLSID clsid, LPCTSTR pszProgID)
{
	USES_CONVERSION;

	// Format class ID as a string
	OLECHAR szClassID[GUID_CCH];
	int cchGuid = ::StringFromGUID2(clsid, szClassID, GUID_CCH);
	LPCTSTR lpszClassID = OLE2CT(szClassID);

	ASSERT(cchGuid == GUID_CCH);    // Did StringFromGUID2 work?
	if (cchGuid != GUID_CCH)
		return FALSE;

	TCHAR szKey[_MAX_PATH];
	long error;
	BOOL bRetCode = TRUE;

#ifndef _MAC
	// check to see if a 16-bit InprocServer key is found when unregistering
	// 32-bit (or vice versa).
	wsprintf(szKey, _T("CLSID\\%s\\%s"), lpszClassID, INPROCSERVER_2);
	HKEY hkey;
	BOOL bSurgical = RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hkey) ==
		ERROR_SUCCESS;

	if (bSurgical)
	{
		// Only remove the keys specific to this version of the control,
		// leaving things in tact for the other version.
		wsprintf(szKey, _T("CLSID\\%s\\%s"), lpszClassID, INPROCSERVER);
		error = RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
		bRetCode = bRetCode && _AfxRegDeleteKeySucceeded(error);

		wsprintf(szKey, _T("CLSID\\%s\\%s"), lpszClassID, TOOLBOXBITMAP);
		error = RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
		bRetCode = bRetCode && _AfxRegDeleteKeySucceeded(error);
	}
	else
#endif
	{
		// No other versions of this control were detected,
		// so go ahead and remove the control completely.
		wsprintf(szKey, _T("CLSID\\%s"), lpszClassID);
		error = _AfxRecursiveRegDeleteKey(HKEY_CLASSES_ROOT, szKey);
		bRetCode = bRetCode && _AfxRegDeleteKeySucceeded(error);

		if (pszProgID != NULL)
		{
			error = _AfxRecursiveRegDeleteKey(HKEY_CLASSES_ROOT,
				(LPTSTR)pszProgID);
			bRetCode = bRetCode && _AfxRegDeleteKeySucceeded(error);
		}
	}

	return bRetCode;
}

static const LPCTSTR rglpszPropPageClass[] =
{
	_T("\0") _T("%1"),
	INPROCSERVER _T("\0%2"),
	NULL
};

BOOL AFXAPI AfxOleRegisterPropertyPageClass(HINSTANCE hInstance,
	REFCLSID clsid, UINT idTypeName)
{
	return AfxOleRegisterPropertyPageClass(hInstance, clsid, idTypeName, 0);
}

BOOL AFXAPI AfxOleRegisterPropertyPageClass(HINSTANCE hInstance,
	REFCLSID clsid, UINT idTypeName, int nRegFlags)
{
	ASSERT(!(nRegFlags & afxRegInsertable));    // can't be insertable

	USES_CONVERSION;

	BOOL bSuccess = FALSE;

	// Format class ID as a string
	OLECHAR szClassID[GUID_CCH];
	int cchGuid = ::StringFromGUID2(clsid, szClassID, GUID_CCH);
	LPCTSTR lpszClassID = OLE2CT(szClassID);

	ASSERT(cchGuid == GUID_CCH);    // Did StringFromGUID2 work?
	if (cchGuid != GUID_CCH)
		return FALSE;

	CString strPathName;
#ifndef _MAC
	AfxGetModuleShortFileName(hInstance, strPathName);
#else
	GetMacInstanceInformationEx(hInstance, NULL, NULL,
		strPathName.GetBuffer(64), 64);
	strPathName.ReleaseBuffer();
#endif

	CString strTypeName;
	if (!strTypeName.LoadString(idTypeName))
	{
		ASSERT(FALSE);  // Name string not present in resources
		strTypeName = lpszClassID; // Use Class ID instead
	}

	HKEY hkeyClassID = NULL;

	TCHAR szKey[_MAX_PATH];
	wsprintf(szKey, _T("CLSID\\%s"), lpszClassID);
	if (::RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkeyClassID) !=
		ERROR_SUCCESS)
		goto Error;

	LPCTSTR rglpszSymbols[2];
	rglpszSymbols[0] = strTypeName;
	rglpszSymbols[1] = strPathName;
	bSuccess = AfxOleRegisterHelper(rglpszPropPageClass, rglpszSymbols,
		2, TRUE, hkeyClassID);

	if (!bSuccess)
		goto Error;

#ifndef _MAC
	if (nRegFlags & afxRegApartmentThreading)
	{
		HKEY hkeyInprocServer32;
		bSuccess = (::RegOpenKey(hkeyClassID, INPROCSERVER,
			&hkeyInprocServer32) == ERROR_SUCCESS);
		if (!bSuccess)
			goto Error;
		ASSERT(hkeyInprocServer32 != NULL);
		static TCHAR szApartment[] = _T("Apartment");
		bSuccess = (::RegSetValueEx(hkeyInprocServer32, _T("ThreadingModel"), 0,
			REG_SZ, (const BYTE*)szApartment, (lstrlen(szApartment)+1) * sizeof(TCHAR)) ==
			ERROR_SUCCESS);
		::RegCloseKey(hkeyInprocServer32);
	}
#endif

Error:
	if (hkeyClassID != NULL)
		::RegCloseKey(hkeyClassID);

	return bSuccess;
}

#ifdef _MAC
//+--------------------------------------------------------------
//  Function:   FullPathFromDirID
//
//  Synopsis:   Prepends the input string with the full path of
//              the given directory.  If the input string isn't
//              empty, it should begin with a mac path separator
//              character, ':'
//
//  Arguments:  LONG idDir directory id of target dir
//              SHORT rnVol volume ref num
//              CHAR *pchPathBuf buffer to receive path
//
//  Returns:    noErr if full path returned, otherwise error code
//
//  History:    24-Jan-95   jamesdu   Created
//---------------------------------------------------------------

static OSErr FullPathFromDirID(long idDir, short rnVol, char *pchPathBuf)
{
	CInfoPBRec cipb;
	OSErr oserr;
	char szBuildPath[_MAX_PATH];

	memset(&cipb, 0, sizeof(cipb));
	cipb.dirInfo.ioFDirIndex = -1; // use the directory ID
	cipb.dirInfo.ioNamePtr = (unsigned char *)szBuildPath;
	cipb.dirInfo.ioVRefNum = rnVol; // use the directory ID
	cipb.dirInfo.ioDrDirID = idDir;

	do
	{
		oserr = PBGetCatInfoSync(&cipb);
		if (noErr != oserr)
		{
			return oserr;
		}
		szBuildPath[*szBuildPath+1] = '\0';
		*szBuildPath = ':';
		strcat(szBuildPath, pchPathBuf);
		cipb.dirInfo.ioDrDirID = cipb.dirInfo.ioDrParID;
		// don't copy the leading colon if we've reached the root
		strcpy(pchPathBuf, szBuildPath + (1 == cipb.dirInfo.ioDrDirID));
	}
	while (cipb.dirInfo.ioDrDirID != 1); // until root reached

	return noErr;
}


///////////////////////////////////////
//

OSErr FullPathFromFSSpec(char *szFileName, FSSpec *pfsp)
{
	*szFileName = ':';
	strncpy(szFileName+1, (char *)pfsp->name+1, *pfsp->name);
	*(szFileName + *pfsp->name+1) = '\0';
	return FullPathFromDirID(pfsp->parID, pfsp->vRefNum, szFileName);
}

// EnsureTypeLibFolder should go into the WLM OLE dll.

// fills in the fsspec's parent ID and vRefNum fields to point to
// the extensions:Type Libraries folder.  Leaves the name field unchanged.
// Entire fsspec is unchanged if an error occurs

STDAPI EnsureTypeLibFolder(void)
{
	FSSpec fsp;
	OSErr oserr;
	char szPath[_MAX_PATH];


	oserr = FindFolder((unsigned short)kOnSystemDisk, kExtensionFolderType, kDontCreateFolder,
		&fsp.vRefNum, &fsp.parID);
	if (noErr == oserr)
	{
		strcpy((char *)&fsp.name, (char *)"\pType Libraries");
		oserr = FullPathFromFSSpec(szPath, &fsp);
		if (noErr == oserr)
			RegisterTypeLibFolder(szPath);
	}

	return oserr == noErr ? S_OK : E_INVALIDARG;
}

HRESULT TypeLibFspFromFullPath(const char *szPathName, FSSpec *pfspTlb)
{
	char *pch;
// truncate to 27 chars if necessary and append ".tlb" to filename
	if (!UnwrapFile(szPathName, pfspTlb))
		return ERROR_INVALID_NAME;

	pfspTlb->name[1+*pfspTlb->name] = '\0';
	pfspTlb->name[28] = '\0';

	// remove any "dot extension"
	pch = PLstrrchr(pfspTlb->name, '.');
	if (NULL != pch)
		*pch = '\0';
	strcat((char *)pfspTlb->name+1, ".tlb");
	*pfspTlb->name = (BYTE)lstrlen((char *)pfspTlb->name+1);
	return NOERROR;
}

#undef LoadTypeLib
#ifdef _AFXDLL
#define LoadTypeLib _afxOLE.pfnLoadTypeLib
#endif

HRESULT _LoadTypeLibGetMacPath(const char *szPathName, char *szMacPath, ITypeLib **ppTypeLib)
{
	HRESULT hr;
	FSSpec fspTlb;

	TypeLibFspFromFullPath(szPathName, &fspTlb);
	hr = LoadTypeLibFSp(&fspTlb, ppTypeLib);
	if (FAILED(hr))
	{
		// try in type lib folder
		BSTR bstr;
		hr = QueryTypeLibFolder(&bstr);
		if (SUCCEEDED(hr))
		{
			int i = SysStringLen(bstr);
			ASSERT(i < 256 - 31);
			lstrcpy(szMacPath, bstr);
			SysFreeString(bstr);
			lstrcat(szMacPath, ":");
			lstrcat(szMacPath, (char*)fspTlb.name+1);
			hr = LoadTypeLib(szMacPath, ppTypeLib);
		}
	}
	else
	{
		FullPathFromFSSpec(szMacPath, &fspTlb);
	}

	return hr;
}

STDAPI _LoadTypeLib(const char *szPathName, ITypeLib **ppTypeLib)
{
	char szMacPath[256];
	return _LoadTypeLibGetMacPath(szPathName, szMacPath, ppTypeLib);
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Force any extra compiler-generated code into AFX_INIT_SEG

#ifdef AFX_INIT_SEG
#pragma code_seg(AFX_INIT_SEG)
#endif
