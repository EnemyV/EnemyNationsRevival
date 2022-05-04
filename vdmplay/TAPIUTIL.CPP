//
// TAPIUTIL.CPP
//
// This module implements the CTapiConnection class.
//
#include "stdafx.h"
#include <string.h>
#include "TapiUtil.h"

// All TAPI line functions return 0 for SUCCESS, so define it.
#define SUCCESS 0

// TAPI version that this sample is designed to use.
#define SAMPLE_TAPI_VERSION 0x00010004

// Early TAPI version
#define EARLY_TAPI_VERSION 0x00010003

// Possible return error for resynchronization functions.
#define WAITERR_WAITABORTED  1

// A pointer to my class because TAPI needs a callback
CTapiConnection *MyThis;

#if FALSE
// Structures needed to handle special non-dialable characters.
#define g_sizeofNonDialable (sizeof(g_sNonDialable)/sizeof(g_sNonDialable[0]))

typedef struct {
    LONG lError;
    DWORD dwDevCapFlag;
    LPSTR szToken;
    LPSTR szMsg;
} NONDIALTOKENS;

NONDIALTOKENS g_sNonDialable[] = {
    {LINEERR_DIALBILLING,  LINEDEVCAPFLAGS_DIALBILLING,  "$", 
            "Wait for the credit card bong tone" },
    {LINEERR_DIALDIALTONE, LINEDEVCAPFLAGS_DIALDIALTONE, "W", 
            "Wait for the second dial tone" },
    {LINEERR_DIALDIALTONE, LINEDEVCAPFLAGS_DIALDIALTONE, "w", 
            "Wait for the second dial tone" },
    {LINEERR_DIALQUIET,    LINEDEVCAPFLAGS_DIALQUIET,    "@", 
            "Wait for the remote end to answer" },
    {LINEERR_DIALPROMPT,   0,                            "?", 
            "Press OK when you are ready to continue dialing"},
};
#endif


static int MsgBox(LPCSTR msg)
{
#ifdef __AFX_H__
	return AfxMessageBox(msg);
#else
	return MessageBox(NULL, msg, "TAPI", MB_OK);
#endif
}

static void myDbgOut(LPCSTR msg)
{
	OutputDebugString(msg);
}

static void throwMemException()
{
#ifdef __AFX_H__
	AfxThrowMemoryException();
#else
	throw CTapiError(0);
#endif
}


int (*dlrMessageBox)(LPCSTR msg) = MsgBox;
void (*dlrDbgOutStr_p)(LPCSTR msg) = myDbgOut;
void (*dlrThrowMemoryException)() = throwMemException; 

#define DECLARE_TAPI_API(pfx, n, a)\
typedef pfx (WINAPI *T_##n) a;\
T_##n p_##n = NULL

DECLARE_TAPI_API(LONG, lineInitialize, (
    LPHLINEAPP lphLineApp,
    HINSTANCE hInstance,
    LINECALLBACK lpfnCallback,
    LPCSTR lpszAppName,
    LPDWORD lpdwNumDevs));

DECLARE_TAPI_API(LONG,  lineOpen, (
    HLINEAPP hLineApp, 
    DWORD dwDeviceID,
    LPHLINE lphLine,
    DWORD dwAPIVersion,
    DWORD dwExtVersion,
    DWORD dwCallbackInstance,
    DWORD dwPrivileges,
    DWORD dwMediaModes,
    LPLINECALLPARAMS const lpCallParams));

DECLARE_TAPI_API(LONG, lineClose, (
    HLINE hLine));

DECLARE_TAPI_API(LONG, lineNegotiateAPIVersion, (
    HLINEAPP hLineApp,
    DWORD dwDeviceID,
    DWORD dwAPILowVersion,
    DWORD dwAPIHighVersion,
    LPDWORD lpdwAPIVersion,
    LPLINEEXTENSIONID lpExtensionID));

DECLARE_TAPI_API(LONG, lineGetDevCaps, (
    HLINEAPP hLineApp, 
    DWORD dwDeviceID, 
    DWORD dwAPIVersion,
    DWORD dwExtVersion, 
    LPLINEDEVCAPS lpLineDevCaps));


DECLARE_TAPI_API(LONG, lineGetDevConfig, (
    DWORD dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCSTR lpszDeviceClass));

DECLARE_TAPI_API(LONG, lineConfigDialogEdit, (
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass,
    LPVOID  const lpDeviceConfigIn,
    DWORD   dwSize,
    LPVARSTRING lpDeviceConfigOut));


DECLARE_TAPI_API(LONG, lineDeallocateCall, (
    HCALL hCall));

DECLARE_TAPI_API(LONG, lineGetAddressCaps, (
    HLINEAPP hLineApp, 
    DWORD dwDeviceID, 
    DWORD dwAddressID, 
    DWORD dwAPIVersion,
    DWORD dwExtVersion, 
    LPLINEADDRESSCAPS lpAddressCaps));

DECLARE_TAPI_API(LONG, lineMakeCall, (
    HLINE hLine, 
    LPHCALL lphCall, 
    LPCSTR lpszDestAddress, 
    DWORD dwCountryCode, 
    LPLINECALLPARAMS const lpCallParams));


DECLARE_TAPI_API(LONG, lineDial, (
    HCALL hCall, 
    LPCSTR lpszDestAddress, 
    DWORD dwCountryCode));

DECLARE_TAPI_API(LONG, lineDrop, (
    HCALL hCall, 
    LPCSTR lpsUserUserInfo, 
    DWORD dwSize));

DECLARE_TAPI_API(LONG, lineGetCallStatus, (
    HCALL hCall, 
    LPLINECALLSTATUS lpCallStatus));

DECLARE_TAPI_API(LONG, lineShutdown, (
    HLINEAPP hLineApp));

DECLARE_TAPI_API(LONG, lineGetID, (
    HLINE hLine,
    DWORD dwAddressID, 
    HCALL hCall,
    DWORD dwSelect,        
    LPVARSTRING lpDeviceID, 
    LPCSTR lpszDeviceClass));

DECLARE_TAPI_API(LONG, lineGetLineDevStatus, (
    HLINE hLine, 
    LPLINEDEVSTATUS lpLineDevStatus));

DECLARE_TAPI_API(LONG, lineAnswer, (
	HCALL hCall,
    LPCSTR lpsUserUserInfo,
    DWORD dwSize));

DECLARE_TAPI_API(LONG, lineSetDevConfig, (
    DWORD dwDeviceID,
    LPVOID const lpDeviceConfig,
    DWORD dwSize,
    LPCSTR lpszDeviceClass));



static HINSTANCE hTapiDll;
static UINT initCount;

#define stringize(xx) #xx

#define INIT_TAPI_API(x) \
	p_##x = (T_##x) GetProcAddress(hTapiDll, stringize(x));\
if (!p_##x)\
	return FALSE;


BOOL LoadTapiDll()
{
	hTapiDll = LoadLibrary("TAPI32.DLL");

	return hTapiDll != NULL;
}

void UnloadTapiDll()
{
	if (hTapiDll)
		FreeLibrary(hTapiDll);
	hTapiDll = NULL;
}

BOOL InitTapiApi()
{
	if (initCount++ > 0)
		return TRUE;

	if (!LoadTapiDll())
		return FALSE;

	INIT_TAPI_API( lineInitialize );
	INIT_TAPI_API( lineOpen );
	INIT_TAPI_API( lineClose );
	INIT_TAPI_API( lineNegotiateAPIVersion );
	INIT_TAPI_API( lineGetDevCaps );
	INIT_TAPI_API( lineGetDevConfig );
	INIT_TAPI_API( lineConfigDialogEdit );
	INIT_TAPI_API( lineDeallocateCall);
	INIT_TAPI_API( lineGetAddressCaps );
	INIT_TAPI_API( lineMakeCall );
	INIT_TAPI_API( lineDial );
	INIT_TAPI_API( lineDrop );
	INIT_TAPI_API( lineGetCallStatus );
	INIT_TAPI_API( lineShutdown );
	INIT_TAPI_API( lineGetID );
	INIT_TAPI_API( lineGetLineDevStatus );
	INIT_TAPI_API( lineAnswer );
	INIT_TAPI_API( lineSetDevConfig );


	return TRUE;
}




void StopTapiApi()
{
	if (--initCount <= 0)
	{
		UnloadTapiDll();
		initCount = 0;
	}
}


static BOOL firstTime = TRUE;
static BOOL supported = FALSE;
extern HINSTANCE vphInst;

BOOL IsTapiSupported()
{
	BOOL ret;

	if (!firstTime)
		return supported;

	firstTime = FALSE;

	if (!InitTapiApi())
		return FALSE;

	{
		CTapiConnection conn;

		supported = conn.Create(vphInst, NULL, TRUE);
	}
	

	StopTapiApi();
	return supported;
}



// 
// Constructor
//
CTapiConnection::CTapiConnection()
{
    m_bShuttingDown = FALSE;
    m_bStoppingCall = FALSE;
    m_bInitializing = FALSE;
    m_bTapiInUse = FALSE;
	m_bConnected = FALSE;
    m_dwNumDevs = 0;
    m_hCall = NULL;
    m_hLine = NULL;
    m_dwDeviceID = 0;
    m_hLineApp = NULL;
	m_hComm = NULL;
	m_hInst = NULL;
	m_answered = FALSE;
	m_dialing = 0;

    MyThis = this;


};

//
// Destructor
//
CTapiConnection::~CTapiConnection()
{
    m_bInitialized = FALSE;
	StopTapiApi();

};

//
//  FUNCTION: BOOL Create()
//
//  PURPOSE: Initializes TAPI
//
BOOL CTapiConnection::Create(HINSTANCE hInst, char *szPhoneNumber, BOOL quiet)
{
    long lReturn;

    // If we're already initialized, then initialization succeeds.
    if (m_hLineApp)
        return TRUE;

    // If we're in the middle of initializing, then fail, we're not done.
    if (m_bInitializing)
        return FALSE;

	m_hInst = hInst;
	if (!m_hInst)
		return FALSE;

    m_bInitializing = TRUE;

	if (!InitTapiApi())
		throw CTapiError((DWORD)-1);

    // Initialize TAPI
    do
    {
        lReturn = ::p_lineInitialize(&m_hLineApp, 
            m_hInst, 
            lineCallbackFunc, 
            "VDMPLAY", 
            &m_dwNumDevs);

        if (m_dwNumDevs == 0)
        {
            if (!quiet)
				dlrMessageBox("There are no telephony devices installed.");
            m_bInitializing = FALSE;
			ShutdownTAPI();
            return FALSE;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            dlrDbgOutStr("lineInitialize unhandled error\n");
            m_bInitializing = FALSE;
            return FALSE;
        }
    }
    while(lReturn != SUCCESS);

    dlrDbgOutStr("Tapi initialized.\n");

    // If the user furnished a phone number copy it over.
    if (szPhoneNumber)
        strcpy(m_szPhoneNumber, szPhoneNumber);

    m_bInitializing = FALSE;
    return TRUE;
}


BOOL CTapiConnection::SelectDevice(UINT devId, LPCSTR devName)
{
	if (devId  < m_dwNumDevs)
	{
		m_dwDeviceID = devId;

		
		
		return TRUE;
	}



	return FALSE;
}

void CTapiConnection::SetConfig(CLineDevConfig& cf)
{
	m_cfgInfo = cf;
}


void CTapiConnection::StoreConfig(LPCSTR iniFile, LPCSTR section)
{
	char tmp[16];

	ultoa(m_dwDeviceID, tmp, 10);
	WritePrivateProfileString(section, "DeviceId", tmp, iniFile);
	m_cfgInfo.Store("DevConfig", section, iniFile);
}

void CTapiConnection::LoadConfig(LPCSTR iniFile, LPCSTR section, BOOL fullLoad)
{
	DWORD devId = GetPrivateProfileInt(section, "DeviceId", -1, iniFile);


	if (devId == (DWORD) -1)
	{
			devId = 0;
			fullLoad = FALSE;
	}

	m_dwDeviceID = devId;

	if (!fullLoad)
	{
	//	m_cfgInfo.Query(m_dwDeviceID);
	}
	else
	{
		m_dwDeviceID = devId;
		m_cfgInfo.Load("DevConfig", section, iniFile);
		m_cfgInfo.Set(m_dwDeviceID);
	}
}


BOOL CTapiConnection::InitLine(CLineDevCaps& devCaps)
{
 
   m_dwAPIVersion = 0;
 

	devCaps.Query(m_hLineApp, m_dwDeviceID, m_dwAPIVersion); 

#if 0
    if (!(devCaps->dwBearerModes & LINEBEARERMODE_VOICE ))
    {
        dlrMessageBox("The selected line doesn't support VOICE capabilities");
        return FALSE;
    }
#endif

    // Does this line have the capability to make calls?
    if (!(devCaps->dwLineFeatures & LINEFEATURE_MAKECALL))
    {
        dlrMessageBox("The selected line doesn't support MAKECALL capabilities");
        return FALSE;
    }

    // Does this line have the capability for interactive voice?
    if (!(devCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM))
    {
        dlrMessageBox("The selected line doesn't support DATAMODEM capabilities");
        return FALSE;
    }

		
	return TRUE;
}


BOOL CTapiConnection::lowLevelOpen(DWORD privelege, DWORD media)
{

	long lReturn;

    do
    {
        lReturn = ::p_lineOpen(m_hLineApp, m_dwDeviceID, &m_hLine,
            m_dwAPIVersion, 0, (DWORD) this,
            privelege, media, 0);

        if((lReturn == LINEERR_ALLOCATED)||(lReturn == LINEERR_RESOURCEUNAVAIL))
        {
            HangupCall();
            dlrMessageBox("Line is already in use by a non-TAPI application "
                "or by another TAPI Service Provider.\n");
            return FALSE;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            dlrMessageBox("Unable to Use Line\n");
            HangupCall();
            return FALSE;
        }
    }
    while(lReturn != SUCCESS);

	return TRUE;
}

BOOL CTapiConnection::OpenLine(BOOL answer)
{

	m_dialing = !answer;
	m_answered = FALSE;

	if (!lowLevelOpen(LINECALLPRIVILEGE_NONE, 0))
		return FALSE;


	if (answer)
	{
		CLineDevStatus devStat(m_hLine);

#if 0
	     if (devStat -> dwOpenMediaModes)
			dlrMessageBox("!!!WARNING!!!  Another application is already waiting for calls.");

		  if (devStat -> dwLineFeatures & LINEFEATURE_MAKECALL)
			dlrMessageBox("!!!WARNING!!!  No call appearances available at this time.");
#endif
		  ::p_lineClose(m_hLine);

	
		  if (!lowLevelOpen(LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM))
			  return FALSE;
	}

	return TRUE;
}


#define USEINITLINE
#define USEOPENLINE

// #define WITHGETDEVLINE 1
//
//  FUNCTION: DialCall()
//
//  PURPOSE: Get a number from the user and dial it.
//
BOOL CTapiConnection::DialCall(LPCSTR szPhoneNumber)
{
    LPLINEDEVCAPS lpLineDevCaps = NULL;

    if (m_bTapiInUse)
    {
        dlrMessageBox("A call is already being handled.");
        return FALSE;
    }

    // Make sure TAPI is initialized properly
    if (!m_hLineApp)
    {

        if (!Create(m_hInst, NULL))
            return FALSE;
    }

    // If there are no line devices installed on the machine, quit.
    if (m_dwNumDevs < 1)
        return FALSE;

    // We now have a call active.  Prevent future calls.
    m_bTapiInUse = TRUE;

    // Get a phone number from the user.
    if (szPhoneNumber == (char *)NULL)
    {
        if (m_szPhoneNumber == (char *)NULL)
        {
            HangupCall();
#ifdef WITHGETDEVLINE
			goto DeleteBuffers;
#else
            return m_bTapiInUse;
#endif
        }
    }
    else 
        strcpy(m_szPhoneNumber, szPhoneNumber);

#ifdef WITHGETDEVLINE

    // Get the line to use
    lpLineDevCaps = GetDeviceLine(&m_dwAPIVersion, lpLineDevCaps);

    // Need to check the DevCaps to make sure this line is usable.
    if (lpLineDevCaps == NULL)
    {
        dlrDbgOutStr("Error on Requested line\n");
        goto DeleteBuffers;
    }

    if (!(lpLineDevCaps->dwBearerModes & LINEBEARERMODE_VOICE ))
    {
        dlrMessageBox("The selected line doesn't support VOICE capabilities");
        goto DeleteBuffers;
    }

    // Does this line have the capability to make calls?
    if (!(lpLineDevCaps->dwLineFeatures & LINEFEATURE_MAKECALL))
    {
        dlrMessageBox("The selected line doesn't support MAKECALL capabilities");
        goto DeleteBuffers;
    }

    // Does this line have the capability for interactive voice?
    if (!(lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE))
    {
        dlrMessageBox("The selected line doesn't support INTERACTIVE VOICE capabilities");
        goto DeleteBuffers;
    }
#else

#ifdef USEINITLINE

   CLineDevCaps  devCaps;

   if (!InitLine(devCaps))
	   return FALSE;

#else
   LINEEXTENSIONID lineExtID;
   long lRet;

   m_dwAPIVersion = 0;
 
   lRet = ::p_lineNegotiateAPIVersion(m_hLineApp, m_dwDeviceID, 
      EARLY_TAPI_VERSION, SAMPLE_TAPI_VERSION,
           &m_dwAPIVersion, &lineExtID);

	if (lRet != SUCCESS || !m_dwAPIVersion)
	{
		dlrMessageBox("Can't negotiate API version of the selected line");
		return FALSE;
	}


	CLineDevCaps  devCaps(m_hLineApp, m_dwDeviceID, m_dwAPIVersion); 



    // Does this line have the capability to make calls?
    if (!(devCaps->dwLineFeatures & LINEFEATURE_MAKECALL))
    {
        dlrMessageBox("The selected line doesn't support MAKECALL capabilities");
        return FALSE;
    }

    // Does this line have the capability for data
    if (!(devCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM))
    {
        dlrMessageBox("The selected line doesn't support DATAMODEM cabilities");
        return FALSE;
    }

#endif		

#endif

#ifdef USEOPENLINE
	if (!OpenLine(FALSE))
		return FALSE;
#else

    // Open the Line for an outgoing call.
    do
    {
        lReturn = ::p_lineOpen(m_hLineApp, m_dwDeviceID, &m_hLine,
            m_dwAPIVersion, 0, 0,
            LINECALLPRIVILEGE_NONE, 0, 0);

        if((lReturn == LINEERR_ALLOCATED)||(lReturn == LINEERR_RESOURCEUNAVAIL))
        {
            HangupCall();
            dlrDbgOutStr("Line is already in use by a non-TAPI application "
                "or by another TAPI Service Provider.\n");
            goto DeleteBuffers;
        }

        if (HandleLineErr(lReturn))
            continue;
        else
        {
            dlrDbgOutStr("Unable to Use Line\n");
            HangupCall();
            goto DeleteBuffers;
        }
    }
    while(lReturn != SUCCESS);
#endif

    // Start dialing the number
#ifdef WITHGETDEVLINE

    if( MakeTheCall(lpLineDevCaps, m_szPhoneNumber))
#else
    if( MakeTheCall(devCaps, m_szPhoneNumber))
#endif
        dlrDbgOutStr("lineMakeCall succeeded.\n");
    else
    {
        dlrDbgOutStr("lineMakeCall failed.\n");
        HangupCall();
    }



    if (lpLineDevCaps)
        LocalFree(lpLineDevCaps);

    return m_bTapiInUse;
}

BOOL  CTapiConnection::WaitForCall()
{

   if (m_bTapiInUse)
    {
        dlrMessageBox("A call is already being handled.");
        return FALSE;
    }

    // Make sure TAPI is initialized properly
    if (!m_hLineApp)
    {

        if (!Create(m_hInst, NULL))
            return FALSE;
    }

    // If there are no line devices installed on the machine, quit.
    if (m_dwNumDevs < 1)
        return FALSE;

    // We now have a call active.  Prevent future calls.
    m_bTapiInUse = TRUE;


    CLineDevCaps  devCaps;

	if (!InitLine(devCaps))
	   return FALSE;



	return OpenLine(TRUE);
}

#if 0
//
//  FUNCTION: void GetDeviceLine()
//
//  PURPOSE: Gets the first available line device.
//
//
LPLINEDEVCAPS CTapiConnection::GetDeviceLine(DWORD *pdwAPIVersion, LPLINEDEVCAPS lpLineDevCaps)
{
    DWORD dwDeviceID;
    char szLineUnavail[] = "Line Unavailable";
    char szLineUnnamed[] = "Line Unnamed";
    char szLineNameEmpty[] = "Line Name is Empty";
    LPSTR lpszLineName;
    long lReturn;
    char buf[MAX_PATH];
    LINEEXTENSIONID lineExtID;
    BOOL bDone = FALSE;


    for (dwDeviceID = 0; (dwDeviceID < m_dwNumDevs) && !bDone; dwDeviceID ++)
    {

        lReturn = ::p_lineNegotiateAPIVersion(m_hLineApp, dwDeviceID, 
            EARLY_TAPI_VERSION, SAMPLE_TAPI_VERSION,
            pdwAPIVersion, &lineExtID);

        if ((HandleLineErr(lReturn))&&(*pdwAPIVersion))
        {
            lpLineDevCaps = MylineGetDevCaps(lpLineDevCaps, dwDeviceID, *pdwAPIVersion);

            if ((lpLineDevCaps -> dwLineNameSize) &&
                (lpLineDevCaps -> dwLineNameOffset) &&
                (lpLineDevCaps -> dwStringFormat == STRINGFORMAT_ASCII))
            {
                // This is the name of the device.
                lpszLineName = ((char *) lpLineDevCaps) + 
                    lpLineDevCaps -> dwLineNameOffset;
                sprintf(buf, "Name of device is: %s\n", lpszLineName);
                dlrDbgOutStr(buf);

            }
            else  // DevCaps doesn't have a valid line name.  Unnamed.
                lpszLineName = szLineUnnamed;
        }
        else  // Couldn't NegotiateAPIVersion.  Line is unavail.
            lpszLineName = szLineUnavail;

        // If this line is usable and we don't have a default initial
        // line yet, make this the initial line.
        if ((lpszLineName != szLineUnavail) && 
            (lReturn == SUCCESS ) &&
            (dwDeviceID > 0)) 	  // we don't want first device
        {          
            m_dwDeviceID = dwDeviceID;
            bDone = TRUE;
        }
        else 
            m_dwDeviceID = MAXDWORD;
    }
    return (lpLineDevCaps);
}

#endif
//
//  FUNCTION: MylineGetDevCaps(LPLINEDEVCAPS, DWORD , DWORD)
//
//  PURPOSE: Gets a LINEDEVCAPS structure for the specified line.
//
//  COMMENTS:
//
//    This function is a wrapper around lineGetDevCaps to make it easy
//    to handle the variable sized structure and any errors received.
//    
//    The returned structure has been allocated with LocalAlloc,
//    so LocalFree has to be called on it when you're finished with it,
//    or there will be a memory leak.
//
//    Similarly, if a lpLineDevCaps structure is passed in, it *must*
//    have been allocated with LocalAlloc and it could potentially be 
//    LocalFree()d.
//
//    If lpLineDevCaps == NULL, then a new structure is allocated.  It is
//    normal to pass in NULL for this parameter unless you want to use a 
//    lpLineDevCaps that has been returned by a previous I_lineGetDevCaps
//    call.
//
//


CLineDevCaps::CLineDevCaps(HLINEAPP hLineApp, DWORD devId, DWORD& apiVersion)
: CDVarStruct<LINEDEVCAPS>(1024u)
{
	if (hLineApp) 
		Query(hLineApp, devId, apiVersion);
}

CLineDevCaps::CLineDevCaps()
: CDVarStruct<LINEDEVCAPS>(1024u)
{
}



void CLineDevCaps::Query(HLINEAPP hLineApp, DWORD devId, DWORD& apiVersion)
{	 

	long lRet = 0;
    LINEEXTENSIONID lineExtID;

	if (apiVersion == 0)
	{
        lRet = ::p_lineNegotiateAPIVersion(hLineApp, devId, 
            EARLY_TAPI_VERSION, SAMPLE_TAPI_VERSION,
            &apiVersion, &lineExtID);

		if (lRet != SUCCESS || apiVersion == 0)
		{
			dlrMessageBox("Can't negotiate API version of the selected line");
			throw CTapiError(lRet);
		}

	}

	while(TRUE)
	{
		lRet = ::p_lineGetDevCaps(hLineApp, 
                    devId, apiVersion, 0, m_ptr); 

	    if (lRet == SUCCESS)
			break;
				

		if (LINEERR_STRUCTURETOOSMALL == lRet)
			ReserveNeededSpace();
		else
			throw CTapiError(lRet);
      
	}

}

CLineDevConfig::CLineDevConfig() : CVarString(256u)
{
	m_ptr->dwStringSize = 0;
}

CLineDevConfig::CLineDevConfig(DWORD devId, LPCSTR devClass) : CVarString(256u)
{
	Query(devId, devClass);
}


void CLineDevConfig::Query(DWORD devId, LPCSTR devClass)
{
	long lRet = 0;

	if (!devClass)
		devClass = "tapi/line";

	while(TRUE)
	{
		lRet = ::p_lineGetDevConfig(devId, m_ptr, devClass); 
		
	    if (lRet == SUCCESS)
			break;
				

		if (LINEERR_STRUCTURETOOSMALL == lRet)
			ReserveNeededSpace();
		else
			throw CTapiError(lRet);

	}
}

void CLineDevConfig::Edit(DWORD devId, HWND hWndOwner, LPCSTR devClass)
{
	long lRet = 0;
	CVarString tmp(256);

	if (!devClass)
		devClass = "tapi/line";

	if (!m_ptr->dwStringSize)
		Query(devId, devClass);

	if (!hWndOwner)
		hWndOwner = GetForegroundWindow();

	while(TRUE)
	{
		lRet = ::p_lineConfigDialogEdit(devId, hWndOwner, devClass, 
			Data(), 
			Length(), tmp); 
		
	    if (lRet == SUCCESS)
			break;


		if (LINEERR_STRUCTURETOOSMALL == lRet)
			tmp.ReserveNeededSpace();
		else
			throw CTapiError(lRet);

		tmp.ReserveNeededSpace();
	}

	adopt(tmp);

}

void CLineDevConfig::Set(DWORD devId, LPCSTR devClass)
{
	if (!devClass)
		devClass = "tapi/line";

	LONG lRet = p_lineSetDevConfig(devId, Data(), Length(), devClass);
	
	if (lRet)
		throw CTapiError(lRet); 
}



CLineCallParams::CLineCallParams(LPCSTR callAddress, 
					DWORD bearerMode, 
					DWORD mediaMode,
					DWORD addressMode,
					DWORD flags) : CSVarStruct<LINECALLPARAMS>(callAddress ? 1+strlen(callAddress) : 64)
{
	m_ptr->dwBearerMode = bearerMode;
	m_ptr->dwMediaMode = mediaMode;
	m_ptr->dwAddressMode = addressMode;
	m_ptr->dwCallParamFlags = flags;
    m_ptr->dwDisplayableAddressOffset = sizeof(LINECALLPARAMS);
    m_ptr->dwDisplayableAddressSize = 1+strlen(callAddress);
    strcpy(Contents(m_ptr->dwDisplayableAddressOffset),  callAddress);
}




CLineAddressCaps::CLineAddressCaps() : CDVarStruct<LINEADDRESSCAPS>(1024) 
{
}

CLineAddressCaps::CLineAddressCaps(HLINEAPP app, DWORD devId, DWORD addrId, DWORD apiV, DWORD extV) :
CDVarStruct<LINEADDRESSCAPS>(1024) 
{
	Query(app, devId, addrId, apiV, extV);
}


void CLineAddressCaps::Query(HLINEAPP app, DWORD devId, DWORD addrId, DWORD apiV, DWORD extV)
{
	ULONG lRet;

    while(TRUE)
    {
		lRet =  ::p_lineGetAddressCaps(app, devId, addrId, apiV, extV, m_ptr);
	    if (lRet == SUCCESS)
			break;

		if (LINEERR_STRUCTURETOOSMALL == lRet)
			ReserveNeededSpace();
		else
			throw CTapiError(lRet);


	};
	
}


CLineDevStatus::CLineDevStatus() : CDVarStruct<LINEDEVSTATUS>(1024) 
{
}

CLineDevStatus::CLineDevStatus(HLINE hLine)	 : CDVarStruct<LINEDEVSTATUS>(1024) 
{
	Query(hLine);
}


void CLineDevStatus::Query(HLINE hLine)
{
	ULONG lRet;


    while(TRUE)
    {
		lRet =  ::p_lineGetLineDevStatus(hLine, m_ptr);
	    if (lRet == SUCCESS)
			break;

		if (LINEERR_STRUCTURETOOSMALL == lRet)
			ReserveNeededSpace();
		else
			throw CTapiError(lRet);


	};
	
}







BOOL CTapiConnection::EnumDevices(DevEnumProc proc, LPVOID ctx)
{
	DWORD devId;
	DWORD apiVersion;
	LONG lRet;
    LINEEXTENSIONID lineExtID;

	if (!Create(m_hInst))
		return FALSE;


	CLineDevCaps caps;

    for (devId = 0; devId < m_dwNumDevs; devId++)
    {

        lRet= ::p_lineNegotiateAPIVersion(m_hLineApp, devId, 
            EARLY_TAPI_VERSION, SAMPLE_TAPI_VERSION,
            &apiVersion, &lineExtID);

        if ((lRet == SUCCESS) && apiVersion)
        {

			caps.Query(m_hLineApp, devId, apiVersion);


            if ((caps -> dwLineNameSize) &&
                (caps -> dwLineNameOffset) &&
                (caps -> dwStringFormat == STRINGFORMAT_ASCII))
            {
                // This is the name of the device.
                LPCSTR lineName = caps.Contents(caps->dwLineNameOffset);
                
                proc(ctx, lineName, devId);
            }
		}
	}

	return TRUE;

}

void CTapiConnection::GetDeviceName(LPSTR devName)
{
	DWORD apiVersion = 0;
	CLineDevCaps caps(m_hLineApp, m_dwDeviceID, apiVersion);

	strcpy(devName, caps.Contents(caps->dwLineNameOffset));
}

			
#if 0

LPLINEDEVCAPS CTapiConnection::MylineGetDevCaps(
    LPLINEDEVCAPS lpLineDevCaps,
    DWORD dwDeviceID, DWORD dwAPIVersion)
{
    // Allocate enough space for the structure plus 1024.
    size_t sizeofLineDevCaps = sizeof(LINEDEVCAPS) + 1024;
    long lReturn;
    
    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineDevCaps = 
            (LPLINEDEVCAPS) CheckAndReAllocBuffer(
                (LPVOID) lpLineDevCaps, // Pointer to existing buffer, if any
                sizeofLineDevCaps);      // Minimum size the buffer should be

        if (lpLineDevCaps == NULL)
            return NULL;

        // Make the call to fill the structure.
        do
        {            
            lReturn = 
                ::p_lineGetDevCaps(m_hLineApp, 
                    dwDeviceID, dwAPIVersion, 0, lpLineDevCaps);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                dlrDbgOutStr("lineGetDevCaps unhandled error/n");
                LocalFree(lpLineDevCaps);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineDevCaps -> dwNeededSize) <= (lpLineDevCaps -> dwTotalSize))
            return lpLineDevCaps;

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineDevCaps = lpLineDevCaps -> dwNeededSize;
    }
}

//
//  FUNCTION: LPVOID CheckAndReAllocBuffer(LPVOID, size_t, LPCSTR)
//
//  PURPOSE: Checks and ReAllocates a buffer if necessary.
//
LPVOID CTapiConnection::CheckAndReAllocBuffer(LPVOID lpBuffer, size_t sizeBufferMinimum)
{
    size_t sizeBuffer;

    if (lpBuffer == NULL)  // Allocate the buffer if necessary. 
    {
        sizeBuffer = sizeBufferMinimum;
        lpBuffer = (LPVOID) LocalAlloc (LPTR, sizeBuffer);
            
        if (lpBuffer == NULL)
        {
            dlrDbgOutStr("LocalAlloc failed in CheckAndReAllocBuffer./n");
            return NULL;
        }
    }
    else // If the structure already exists, make sure its good.
    {
        sizeBuffer = LocalSize((HLOCAL) lpBuffer);

        if (sizeBuffer == 0) // Bad pointer?
        {
            dlrDbgOutStr("LocalSize returned 0 in CheckAndReAllocBuffer/n");
            return NULL;
        }

        // Was the buffer big enough for the structure?
        if (sizeBuffer < sizeBufferMinimum)
        {
            dlrDbgOutStr("Reallocating structure\n");
            LocalFree(lpBuffer);
            return CheckAndReAllocBuffer(NULL, sizeBufferMinimum);
        }
    }
                
    ((LPVARSTRING) lpBuffer ) -> dwTotalSize = (DWORD) sizeBuffer;
    return lpBuffer;
}


//
//  FUNCTION: MylineGetAddressCaps(LPLINEADDRESSCAPS, ..)
//
//  PURPOSE: Return a LINEADDRESSCAPS structure for the specified line.
//
LPLINEADDRESSCAPS CTapiConnection::MylineGetAddressCaps (
    LPLINEADDRESSCAPS lpLineAddressCaps,
    DWORD dwDeviceID, DWORD dwAddressID,
    DWORD dwAPIVersion, DWORD dwExtVersion)
{
    size_t sizeofLineAddressCaps = sizeof(LINEADDRESSCAPS) + 1024;
    long lReturn;
    
    // Continue this loop until the structure is big enough.
    while(TRUE)
    {
        // Make sure the buffer exists, is valid and big enough.
        lpLineAddressCaps = 
            (LPLINEADDRESSCAPS) CheckAndReAllocBuffer(
                (LPVOID) lpLineAddressCaps,
                sizeofLineAddressCaps);

        if (lpLineAddressCaps == NULL)
            return NULL;
            
        // Make the call to fill the structure.
        do
        {
            lReturn = 
                ::p_lineGetAddressCaps(m_hLineApp,
                    dwDeviceID, dwAddressID, dwAPIVersion, dwExtVersion,
                    lpLineAddressCaps);

            if (HandleLineErr(lReturn))
                continue;
            else
            {
                dlrDbgOutStr("lineGetAddressCaps unhandled error\n");
                LocalFree(lpLineAddressCaps);
                return NULL;
            }
        }
        while (lReturn != SUCCESS);

        // If the buffer was big enough, then succeed.
        if ((lpLineAddressCaps -> dwNeededSize) <= 
            (lpLineAddressCaps -> dwTotalSize))
        {
            return lpLineAddressCaps;
        }

        // Buffer wasn't big enough.  Make it bigger and try again.
        sizeofLineAddressCaps = lpLineAddressCaps -> dwNeededSize;
    }
}

#endif

//
//  FUNCTION: MakeTheCall(LPLINEDEVCAPS, LPCSTR)
//
//  PURPOSE: Dials the call
//

BOOL CTapiConnection::MakeTheCall(LPLINEDEVCAPS lpLineDevCaps, LPCTSTR lpszAddress)
{
#if 0
	LPLINECALLPARAMS  lpCallParams = NULL;
    LPLINEADDRESSCAPS lpAddressCaps = NULL;
#endif

    long lReturn;
    BOOL bFirstDial = TRUE;

#if 0                               
    // Get the capabilities for the line device we're going to use.
    lpAddressCaps = MylineGetAddressCaps(lpAddressCaps,
        m_dwDeviceID, 0, m_dwAPIVersion, 0);
    if (lpAddressCaps == NULL)
        return FALSE;

    // Setup our CallParams.

    lpCallParams = CreateCallParams (lpCallParams, lpszAddress);
    if (lpCallParams == NULL)
        return FALSE;

#else
	CLineAddressCaps addrCaps(m_hLineApp, m_dwDeviceID, 0, m_dwAPIVersion);
  	CLineCallParams callParams(lpszAddress);


#endif

    do
    {                   
        if (bFirstDial)
            //lReturn = ::p_lineMakeCall(m_hLine, &m_hCall, lpszAddress, 0, lpCallParams);
            lReturn = WaitForReply( ::p_lineMakeCall(m_hLine, &m_hCall, lpszAddress, 
#if 1
						0, callParams));
#else
                        0, lpCallParams) );
#endif
        else
            lReturn = WaitForReply(::p_lineDial(m_hCall, lpszAddress, 0));

        if (lReturn == WAITERR_WAITABORTED)
        {
             dlrDbgOutStr("While Dialing, WaitForReply aborted.\n");
             goto errExit;
        }
            
        if (HandleLineErr(lReturn))
            continue;
        else
        {
            if (bFirstDial)
                dlrDbgOutStr("lineMakeCall unhandled error\n");
            else
                dlrDbgOutStr("lineDial unhandled error\n");

            goto errExit;
        }
    }
    while (lReturn != SUCCESS);
        
    bFirstDial = FALSE;

#if 0
    if (lpCallParams)
        LocalFree(lpCallParams);
    if (lpAddressCaps)
        LocalFree(lpAddressCaps);
#endif
    
    return TRUE;
    
  errExit:
#if 0
    if (lpCallParams)
        LocalFree(lpCallParams);
    if (lpAddressCaps)
        LocalFree(lpAddressCaps);
#endif

    dlrMessageBox("Dial failed.");

    return FALSE;
}   

#if 0
//
//  FUNCTION: CreateCallParams(LPLINECALLPARAMS, LPCSTR)
//
//  PURPOSE: Allocates and fills a LINECALLPARAMS structure
//
//
LPLINECALLPARAMS CTapiConnection::CreateCallParams (
    LPLINECALLPARAMS lpCallParams, LPCSTR lpszDisplayableAddress)
{
    size_t sizeDisplayableAddress;

    if (lpszDisplayableAddress == NULL)
        lpszDisplayableAddress = "";
        
    sizeDisplayableAddress = strlen(lpszDisplayableAddress) + 1;
                          
    lpCallParams = (LPLINECALLPARAMS) CheckAndReAllocBuffer(
        (LPVOID) lpCallParams, 
        sizeof(LINECALLPARAMS) + sizeDisplayableAddress);

    if (lpCallParams == NULL)
        return NULL;

    // set everything to 0
    memset(lpCallParams, 0, sizeof(*lpCallParams));

	lpCallParams-> dwTotalSize = sizeof(*lpCallParams) + sizeDisplayableAddress;    
    // This is where we configure the line.
    lpCallParams -> dwBearerMode = LINEBEARERMODE_VOICE;
  //  lpCallParams -> dwMediaMode  = LINEMEDIAMODE_INTERACTIVEVOICE;
    lpCallParams -> dwMediaMode  = LINEMEDIAMODE_DATAMODEM;

    // This specifies that we want to use only IDLE calls and
    // don't want to cut into a call that might not be IDLE (ie, in use).
    lpCallParams -> dwCallParamFlags = LINECALLPARAMFLAGS_IDLE;
                                    
    // if there are multiple addresses on line, use first anyway.
    // It will take a more complex application than a simple tty app
    // to use multiple addresses on a line anyway.
    lpCallParams -> dwAddressMode = LINEADDRESSMODE_ADDRESSID;

    // Address we are dialing.
    lpCallParams -> dwDisplayableAddressOffset = sizeof(LINECALLPARAMS);
    lpCallParams -> dwDisplayableAddressSize = sizeDisplayableAddress;
    strcpy((LPSTR)lpCallParams + sizeof(LINECALLPARAMS),
           lpszDisplayableAddress);

    return lpCallParams;
}
#endif

//
//  FUNCTION: long WaitForReply(long)
//
//  PURPOSE: Resynchronize by waiting for a LINE_REPLY 
//
//  PARAMETERS:
//    lRequestID - The asynchronous request ID that we're
//                 on a LINE_REPLY for.
//
//  RETURN VALUE:
//    - 0 if LINE_REPLY responded with a success.
//    - LINEERR constant if LINE_REPLY responded with a LINEERR
//    - 1 if the line was shut down before LINE_REPLY is received.
//
//  COMMENTS:
//
//    This function allows us to resynchronize an asynchronous
//    TAPI line call by waiting for the LINE_REPLY message.  It
//    waits until a LINE_REPLY is received or the line is shut down.
//
//    Note that this could cause re-entrancy problems as
//    well as mess with any message preprocessing that might
//    occur on this thread (such as TranslateAccelerator).
//
//    This function should to be called from the thread that did
//    lineInitialize, or the PeekMessage is on the wrong thread
//    and the synchronization is not guaranteed to work.  Also note
//    that if another PeekMessage loop is entered while waiting,
//    this could also cause synchronization problems.
//
//    One more note.  This function can potentially be re-entered
//    if the call is dropped for any reason while waiting.  If this
//    happens, just drop out and assume the wait has been canceled.  
//    This is signaled by setting bReentered to FALSE when the function 
//    is entered and TRUE when it is left.  If bReentered is ever TRUE 
//    during the function, then the function was re-entered.
//
//    This function times out and returns WAITERR_WAITTIMEDOUT
//    after WAITTIMEOUT milliseconds have elapsed.
//
//


long CTapiConnection::WaitForReply (long lRequestID)
{
    static BOOL bReentered;
    bReentered = FALSE;

    if (lRequestID > SUCCESS)
    {
        MSG msg; 
        DWORD dwTimeStarted;

        m_bReplyReceived = FALSE;
        m_dwRequestedID = (DWORD) lRequestID;

        // Initializing this just in case there is a bug
        // that sets m_bReplyRecieved without setting the reply value.
        m_lAsyncReply = LINEERR_OPERATIONFAILED;

        dwTimeStarted = GetTickCount();

        while(!m_bReplyReceived)
        {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // This should only occur if the line is shut down while waiting.
            if ((m_hCall != NULL) &&(!m_bTapiInUse || bReentered))
            {
                bReentered = TRUE;
                return WAITERR_WAITABORTED;
            }

            // Its a really bad idea to timeout a wait for a LINE_REPLY.
            // If we are execting a LINE_REPLY, we should wait till we get
            // it; it might take a long time to dial (for example).

            // If 5 seconds go by without a reply, it might be a good idea
            // to display a dialog box to tell the user that a
            // wait is in progress and to give the user the capability to
            // abort the wait.
        }

        bReentered = TRUE;
        return m_lAsyncReply;
    }

    bReentered  = TRUE;
    return lRequestID;
}

//
//  FUNCTION: lineCallbackFunc(..)
//
//  PURPOSE: Receive asynchronous TAPI events
//
void CALLBACK CTapiConnection::lineCallbackFunc(
    DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
    DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{

	CTapiConnection* MyThis = (CTapiConnection*) dwCallbackInstance;

    // Handle the line messages.
    switch(dwMsg)
    {
        case LINE_CALLSTATE:
            MyThis->HandleLineCallState(dwDevice, dwMsg, dwCallbackInstance, dwParam1, dwParam2,
                dwParam3);
            break;

        case LINE_CLOSE:
            // Line has been shut down.  
            ASSERT(MyThis);
            MyThis->m_hLine = NULL;
            MyThis->m_hCall = NULL;
            MyThis->HangupCall(); // all handles invalidated by this time
            break;

        case LINE_REPLY:
			ASSERT(MyThis);
			MyThis->m_bReplyReceived = (MyThis->m_dwRequestedID == dwParam1);
			MyThis->m_lAsyncReply = (long) dwParam2;

            if ((long) dwParam2 != SUCCESS)

                dlrDbgOutStr("LINE_REPLY error\n");
            else
			{
                dlrDbgOutStr("LINE_REPLY: successfully replied.\n");
			}
            break;

        case LINE_CREATE:
            ASSERT(MyThis);
            if (MyThis->m_dwNumDevs <= dwParam1)
                MyThis->m_dwNumDevs = dwParam1+1;
            break;

        default:
            dlrDbgOutStr("lineCallbackFunc message ignored\n");
            break;
    }
    return;
}

//
//  FUNCTION: HandleLineCallState(..)
//
//  PURPOSE: Handle LINE_CALLSTATE asynchronous messages.
//

void CTapiConnection::HandleLineCallState(
    DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance,
    DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{

         // Is this a new call?
	if (dwParam3 == LINECALLPRIVILEGE_OWNER)
    {
		if (m_dialing || (m_hCall && (m_hCall != (HCALL) dwDevice)))
        {
           if (dwMsg == LINECALLSTATE_IDLE)
               ::p_lineDeallocateCall((HCALL) dwDevice);
           else
               ::p_lineDrop((HCALL) dwDevice, NULL, 0);

            return;
        }

        if (m_hCall)
		{
            dlrDbgOutStr("Given OWNER privs to a call already owned?\n"
                      " - Should only happen if handed a call already owned.\n");
		}
        else
        {
            m_hCall = (HCALL) dwDevice;
        }
     }


     if (m_hCall != (HCALL) dwDevice)
      {
        if (dwMsg == LINECALLSTATE_IDLE)
			::p_lineDeallocateCall((HCALL) dwDevice);
        else
        {
			::p_lineDrop((HCALL) dwDevice, NULL, 0);
            dlrDbgOutStr("LINE_CALLSTATE 0x%lx for non-main hCall");
		}
        return;
	 }
    
	// Error if this CALLSTATE doesn't apply to our call in progress.
   


    // dwParam1 is the specific CALLSTATE change that is occurring.
    switch (dwParam1)
    {
        case LINECALLSTATE_DIALTONE:
            dlrDbgOutStr("Dial Tone\n");
            break;

        case LINECALLSTATE_DIALING:
            dlrDbgOutStr("Dialing\n");
            break;

        case LINECALLSTATE_PROCEEDING:
            dlrDbgOutStr("Proceeding\n");
            break;

        case LINECALLSTATE_RINGBACK:
            dlrDbgOutStr("RingBack\n");
            break;

        case LINECALLSTATE_BUSY:
            dlrDbgOutStr("Line busy, shutting down\n");
            HangupCall();
            break;

        case LINECALLSTATE_IDLE:
            dlrDbgOutStr("Line idle\n");
            HangupCall();
			OnDialFail();
            break;

        case LINECALLSTATE_SPECIALINFO:
            dlrDbgOutStr("Special Info, probably couldn't dial number\n");
            HangupCall();
			OnDialFail();
            break;

		case LINECALLSTATE_ACCEPTED:
		case LINECALLSTATE_OFFERING:
 			dlrDbgOutStr( dwParam1 == LINECALLSTATE_OFFERING ? "Call OFFERING\n" : "Call ACCEPTED\n");
			if (!m_answered)
			{
               m_answered = TRUE;
               ::p_lineAnswer(m_hCall, NULL, 0);
			}
			break;


        case LINECALLSTATE_DISCONNECTED:
        {
            LPSTR pszReasonDisconnected;

            switch (dwParam2)
            {
                case LINEDISCONNECTMODE_NORMAL:
                    pszReasonDisconnected = "Remote Party Disconnected";
                    break;

                case LINEDISCONNECTMODE_UNKNOWN:
                    pszReasonDisconnected = "Disconnected: Unknown reason";
                    break;

                case LINEDISCONNECTMODE_REJECT:
                    pszReasonDisconnected = "Remote Party rejected call";
                    break;

                case LINEDISCONNECTMODE_PICKUP:
                    pszReasonDisconnected = 
                        "Disconnected: Local phone picked up";
                    break;

                case LINEDISCONNECTMODE_FORWARDED:
                    pszReasonDisconnected = "Disconnected: Forwarded";
                    break;

                case LINEDISCONNECTMODE_BUSY:
                    pszReasonDisconnected = "Disconnected: Busy";
                    break;

                case LINEDISCONNECTMODE_NOANSWER:
                    pszReasonDisconnected = "Disconnected: No Answer";
                    break;

                case LINEDISCONNECTMODE_BADADDRESS:
                    pszReasonDisconnected = "Disconnected: Bad Address";
                    break;

                case LINEDISCONNECTMODE_UNREACHABLE:
                    pszReasonDisconnected = "Disconnected: Unreachable";
                    break;

                case LINEDISCONNECTMODE_CONGESTION:
                    pszReasonDisconnected = "Disconnected: Congestion";
                    break;

                case LINEDISCONNECTMODE_INCOMPATIBLE:
                    pszReasonDisconnected = "Disconnected: Incompatible";
                    break;

                case LINEDISCONNECTMODE_UNAVAIL:
                    pszReasonDisconnected = "Disconnected: Unavail";
                    break;

                case LINEDISCONNECTMODE_NODIALTONE:
                    pszReasonDisconnected = "Disconnected: No Dial Tone";
                    break;

                default:
                    pszReasonDisconnected = 
                        "Disconnected: LINECALLSTATE; Bad Reason";
                    break;

            }

            dlrDbgOutStr(pszReasonDisconnected);
            dlrDbgOutStr("\n");
            HangupCall();
			OnDisconnect();
            break;
        }

        case LINECALLSTATE_CONNECTED:  // CONNECTED!!!
		{
#if 0
            LPVARSTRING lpVarString = NULL;
            DWORD dwSizeofVarString = sizeof(VARSTRING) + 1024;
            HANDLE hCommFile = NULL;
            long lReturn;

            // Very first, make sure this isn't a duplicated message.
            // A CALLSTATE message can be sent whenever there is a
            // change to the capabilities of a line, meaning that it is
            // possible to receive multiple CONNECTED messages per call.
            // The CONNECTED CALLSTATE message is the only one in TapiComm
            // where it would cause problems if it where sent more
            // than once.

            if (m_bConnected)
                break;

            m_bConnected = TRUE;

            // Get the handle to the comm port from the driver so we can start
            // communicating.  This is returned in a LPVARSTRING structure.
            do
            {
                // Allocate the VARSTRING structure
                lpVarString = (LPVARSTRING) CheckAndReAllocBuffer((LPVOID) lpVarString,
                    dwSizeofVarString);

                if (lpVarString == NULL)
                    goto ErrorConnecting;

                // Fill the VARSTRING structure
                lReturn = lineGetID(0, 0, m_hCall, LINECALLSELECT_CALL,
                    lpVarString, "comm/datamodem");

                if (HandleLineErr(lReturn))
                    ; // Still need to check if structure was big enough.
                else
                {
                    dlrDbgOutStr("lineGetID unhandled error\n");
                    goto ErrorConnecting;
                }

                // If the VARSTRING wasn't big enough, loop again.
                if ((lpVarString -> dwNeededSize) > (lpVarString -> dwTotalSize))
                {
                    dwSizeofVarString = lpVarString -> dwNeededSize;
                    lReturn = -1; // Lets loop again.
                }
            }
            while(lReturn != SUCCESS);

            dlrDbgOutStr("Connected!  Starting communications!\n");

            // Again, the handle to the comm port is contained in a
            // LPVARSTRING structure.  Thus, the handle is the very first
            // thing after the end of the structure.  Note that the name of
            // the comm port is right after the handle, but I don't want it.
            m_hComm = 
                *((LPHANDLE)((LPBYTE)lpVarString +
                    lpVarString -> dwStringOffset));

            if (lpVarString)
                LocalFree(lpVarString);

			OnConnect();

		    break;
            // Couldn't start communications.  Clean up instead.
          ErrorConnecting:


	      	dlrDbgOutStr("Error connecting!\n");
            HangupCall();
             break;
#else
			try 
			{
				m_bConnected = TRUE;
				
				CCallDeviceId devId(m_hCall);

				m_hComm = devId.CommHandle();
				OnConnect();

				break;

			}	
			catch(...)
			{
				 dlrDbgOutStr("Error connecting\n!");
				 HangupCall();
				 m_bConnected = FALSE;
				 break;
			}
	
			break;

#endif
        }

        default:
            dlrDbgOutStr("Unhandled LINECALLSTATE message\n");
            break;
    }
}

//
//  FUNCTION: HandleLineErr(long)
//
//  PURPOSE: Handle several of the standard LINEERR errors
//
BOOL CTapiConnection::HandleLineErr(long lLineErr)
{
    BOOL bRet = FALSE;

    // lLineErr is really an async request ID, not an error.
    if (lLineErr > SUCCESS)
        return bRet;

    // All we do is dispatch the correct error handler.
    switch(lLineErr)
    {
        case SUCCESS:
            bRet = TRUE;
            break;

        case LINEERR_INVALCARD:
        case LINEERR_INVALLOCATION:
        case LINEERR_INIFILECORRUPT:
            dlrDbgOutStr("The values in the INI file are invalid.\n");
            break;

        case LINEERR_NODRIVER:
            dlrDbgOutStr("There is a problem with your Telephony device driver.\n");
            break;

        case LINEERR_REINIT:
            ShutdownTAPI();
            break;

        case LINEERR_NOMULTIPLEINSTANCE:
            dlrDbgOutStr("Remove one of your copies of your Telephony driver.\n");
            break;

        case LINEERR_NOMEM:
            dlrDbgOutStr("Out of memory. Cancelling action.\n");
            break;

        case LINEERR_OPERATIONFAILED:
            dlrDbgOutStr("The TAPI operation failed.\n");
            break;

        case LINEERR_RESOURCEUNAVAIL:
            dlrDbgOutStr("A TAPI resource is unavailable at this time.\n");
            break;

        // Unhandled errors fail.
        default:
            break;
    }
    return bRet;
}


//
//  FUNCTION: BOOL HangupCall()
//
//  PURPOSE: Hangup the call in progress if it exists.
//
BOOL CTapiConnection::HangupCall()
{         
    LPLINECALLSTATUS pLineCallStatus = NULL;
    long lReturn;

    // Prevent HangupCall re-entrancy problems.
    if (m_bStoppingCall)
        return TRUE;

    // If Tapi is not being used right now, then the call is hung up.
    if (!m_bTapiInUse)
        return TRUE;

    m_bStoppingCall = TRUE;

	if (m_hComm)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

    dlrDbgOutStr("Stopping Call in progress\n");

    // If there is a call in progress, drop and deallocate it.
    if (m_hCall)
    {
        pLineCallStatus = (LPLINECALLSTATUS)malloc(sizeof(LINECALLSTATUS));

        if (!pLineCallStatus)
        {
            ShutdownTAPI();
            m_bStoppingCall = FALSE;
            return FALSE;
        }

        lReturn = ::p_lineGetCallStatus(m_hCall, pLineCallStatus);

        // Only drop the call when the line is not IDLE.
        if (!((pLineCallStatus -> dwCallState) & LINECALLSTATE_IDLE))
        {
            WaitForReply(::p_lineDrop(m_hCall, NULL, 0));
            dlrDbgOutStr("Call Dropped.\n");
        }

        // The call is now idle.  Deallocate it!
        do
        {
            lReturn = ::p_lineDeallocateCall(m_hCall);
            if (HandleLineErr(lReturn))
                continue;
            else
            {
                dlrDbgOutStr("lineDeallocateCall unhandled error\n");
                break;
            }
        }
        while(lReturn != SUCCESS);

        dlrDbgOutStr("Call Deallocated.\n");

    }

    // if we have a line open, close it.
    if (m_hLine)
    {
       lReturn = ::p_lineClose(m_hLine);
       if (!HandleLineErr(lReturn))
           dlrDbgOutStr("lineClose unhandled error\n");

        dlrDbgOutStr("Line Closed.\n");
    }

    // Clean up.
    m_hCall = NULL;
    m_hLine = NULL;
    m_bTapiInUse = FALSE;
	m_bConnected = FALSE;
    m_bStoppingCall = FALSE; // allow HangupCall to be called again.

    // Need to free buffer returned from lineGetCallStatus
    if (pLineCallStatus)
        free(pLineCallStatus);  
        
    dlrDbgOutStr("Call stopped\n");
    return TRUE;
}

//
//  FUNCTION: BOOL ShutdownTAPI()
//
//  PURPOSE: Shuts down all use of TAPI
//
BOOL CTapiConnection::ShutdownTAPI()
{
    long lReturn;

    // If we aren't initialized, then Shutdown is unnecessary.
    if (m_hLineApp == NULL)
        return TRUE;

    // Prevent ShutdownTAPI re-entrancy problems.
    if (m_bShuttingDown)
        return TRUE;

    m_bShuttingDown = TRUE;

    HangupCall();
    
    do
    {
        lReturn = ::p_lineShutdown(m_hLineApp);
        if (HandleLineErr(lReturn))
            continue;
        else
        {
            dlrDbgOutStr("lineShutdown unhandled error\n");
            break;
        }
    }
    while(lReturn != SUCCESS);

    m_bTapiInUse = FALSE;
    m_hLineApp = NULL;
    m_hCall = NULL;
    m_hLine = NULL;
    m_bShuttingDown = FALSE;

    dlrDbgOutStr("TAPI uninitialized.\n");


    return TRUE;
}


void CVarString::Store(LPCSTR iniKey, LPCSTR section, LPCSTR inifile)
{
	char* tmp = new char[strlen(iniKey) + 8];
	char buf[16];
	char* dataBuf = new char[CHUNKSIZE*2+1];


	strcat(strcpy(tmp, iniKey),  "_Size");

	ultoa(Length(), buf, 10);
	WritePrivateProfileString(section, tmp, buf, inifile); 
	
	size_t o = 0;
	int i = 1;
	for( o = 0; o < Length(); o += CHUNKSIZE, i++)
	{
		StoreChunk(iniKey, section, inifile, i, tmp, dataBuf);
	}

	delete [] dataBuf;
	delete [] tmp;
}
		

void CVarString::Load(LPCSTR iniKey, LPCSTR section, LPCSTR inifile)
{
	char* tmp = new char[strlen(iniKey) + 8];
	char* dataBuf = new char[CHUNKSIZE*2+1];


	strcat(strcpy(tmp, iniKey),  "_Size");

	size_t len = GetPrivateProfileInt(section, tmp, 0, inifile); 
	if (!len)
		return;

	CVarString tStr(len);

	adopt(tStr);
	
	m_ptr->dwStringSize = len;

	size_t o = 0;
	int i = 1;
	for( o = 0; o < Length(); o += CHUNKSIZE, i++)
	{
		LoadChunk(iniKey, section, inifile, i, tmp, dataBuf);
	}

	delete [] dataBuf;
	delete [] tmp;
}



// encode the i't chunk of the string as a sequence of hexadecimal chars and store it in the ini file
void CVarString::StoreChunk(LPCSTR iniKey, LPCSTR section, LPCSTR inifile, int index, LPSTR keyBuf, LPSTR dataBuf)
{
	static char hexdigit[] = "0123456789ABCDEF";

	size_t offset = (index-1)*CHUNKSIZE;
	size_t hasMore = Length() - offset;
	size_t len = hasMore < CHUNKSIZE ? hasMore : CHUNKSIZE;

	LPCSTR  srcPtr = Str() + offset;
	LPSTR   dstPtr = dataBuf;

	while(len--)
	{
		*dstPtr++ = hexdigit[(*srcPtr & 0xF0) >> 4];
		*dstPtr++ = hexdigit[*srcPtr & 0x0F];
		srcPtr++;
	}

	*dstPtr = 0;

	wsprintf(keyBuf, "%s%d", iniKey, index);
	WritePrivateProfileString(section, keyBuf, dataBuf, inifile);
}

inline hexnibble(char c)
{
	return ('0' <= c && c <= '9') ? c-'0' : (10 + c - 'A');
}

inline char hexbyte(char h, char l)
{
	return (hexnibble(h) << 4) | hexnibble(l);
}

void CVarString::LoadChunk(LPCSTR iniKey, LPCSTR section, LPCSTR inifile, int index, LPSTR keyBuf, LPSTR dataBuf)
{

	size_t offset = (index-1)*CHUNKSIZE;
	size_t hasMore = Length() - offset;
	size_t len = hasMore < CHUNKSIZE ? hasMore : CHUNKSIZE;

	wsprintf(keyBuf, "%s%d", iniKey, index);

	GetPrivateProfileString(section, keyBuf, "", dataBuf, len*2+1, inifile);

	LPCSTR  srcPtr = dataBuf;
	LPSTR   dstPtr = (LPSTR) (Str() + offset);

	while(len--)
	{
		*dstPtr++ = hexbyte(srcPtr[0], srcPtr[1]);
		srcPtr += 2;
	}

	

}


void CLineDeviceId::Query(HLINE hLine, DWORD addrId, HCALL hCall, DWORD select, LPCSTR devClass)
{

	long lRet = 0;


	while(TRUE)
	{
		lRet = ::p_lineGetID(hLine, addrId, hCall, select, m_ptr, devClass);                   

	    if (SUCCESS == lRet)
		{
			if (HasEnoughSpace())
				break;

			ReserveNeededSpace();

		} else 	if (LINEERR_STRUCTURETOOSMALL == lRet)
			ReserveNeededSpace();
		else
			throw CTapiError(lRet);
      
	}

}


