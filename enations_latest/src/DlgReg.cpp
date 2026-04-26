// DlgReg.cpp : GetDefaultApp helper (CDlgReg dialog removed)
//

#include "stdafx.h"
#include "lastplnt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

std::string GetDefaultApp ( char const *pExt, char const *pDef, char const *pCmdLine )
{

	// in case of error
	std::string sRtn = std::string( pDef ) + " " + pCmdLine;

	// get the extension
	char cmd [258];
	HKEY key;
	if (RegOpenKeyEx (HKEY_CLASSES_ROOT, pExt, NULL, KEY_READ, &key) != ERROR_SUCCESS)
		return sRtn;

	// read it in
	unsigned long iLen = 256;
	DWORD dwTyp;
	if (RegQueryValueEx (key, "", NULL, &dwTyp, (BYTE*) cmd, &iLen) != ERROR_SUCCESS)
		return sRtn;

	RegCloseKey (key);
	if ( dwTyp != REG_SZ )
		return sRtn;

	// now find the app for this key value
	std::string sKey = std::string( cmd ) + "\\shell\\open\\command";

	if (RegOpenKeyEx (HKEY_CLASSES_ROOT, sKey.c_str(), NULL, KEY_READ, &key) != ERROR_SUCCESS)
		return sRtn;

	iLen = 256;
	if (RegQueryValueEx (key, "", NULL, &dwTyp, (BYTE*) cmd, &iLen) != ERROR_SUCCESS)
		return sRtn;

	RegCloseKey (key);
	if ( dwTyp != REG_SZ )
		return sRtn;

	// put command line in it (may have %1)
	sRtn = cmd;
	size_t iInd = sRtn.find ( '%' );
	if ( ( iInd == std::string::npos ) || ( sRtn [iInd+1] != '1' ) )
		return sRtn + " " + pCmdLine;

	// replace %1 with pCmdLine
	sRtn.replace( iInd, 2, pCmdLine );
	return sRtn;
}
