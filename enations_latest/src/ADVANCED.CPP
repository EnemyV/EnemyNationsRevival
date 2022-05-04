#ifndef VPSYSTEM
#define VPSYSTEM
#endif


#include "stdafx.h"
#include "base.h"
#include "resource.h"
#include "help.h"
#include "version.h"


#ifndef WIN32
typedef unsigned char UCHAR, FAR* PUCHAR;
typedef DWORD ULONG;
#endif

#include "nb30.h"

#if !defined(WIN32) && !defined(EXPORT)
#define EXPORT _export
#else
#define EXPORT
#endif

#if defined(WIN32)
#include "tapiutil.h"
#ifdef WITH_DP
#include "dplay.h"
extern HRESULT (WINAPI *pDirectPlayCreate)( LPGUID lpGUID, LPDIRECTPLAY FAR *lplpDP, IUnknown FAR *pUnk);
extern HRESULT (WINAPI *pDirectPlayEnumerate)( LPDPENUMDPCALLBACK, LPVOID );

#endif
#endif


extern HINSTANCE vphInst;

static void ParseIpxAddress(LPCSTR addr,  LPSTR netNum, LPSTR nodeNum, UINT& portNum)
{
	char buf[64];

	*netNum = *nodeNum = 0;
	portNum = 0;

	strcpy(buf, addr);

	char* dot = strchr(buf, '.');

	if (dot == NULL)
		return;

	*dot++ = 0;

	strcpy(netNum, buf);

	char* colon = strchr(dot, ':');

	if (colon)
		*colon++ = 0;

	strcpy(nodeNum,dot);
	

	if (!colon)
		return;

	char* comma = strchr(colon, ',');

	if (comma)
		*comma++ = 0;


	if (comma)
		portNum = atoi(comma);


}

static void MakeIpxAddress(LPSTR addrBuf, LPCSTR netNum, LPCSTR nodeNum, UINT portNum)
{
	addrBuf[0] = 0;

	if (netNum != "" && nodeNum != "")
	{
		if (!portNum)
			portNum = DEF_IPX_PORT;

		strcpy(addrBuf,netNum);
		strcat(addrBuf, ".");
		strcat(addrBuf,nodeNum);
		strcat(addrBuf,":,");

		char buf[10];

		itoa(portNum, buf, 10);
		strcat(addrBuf,buf);

	}

}



#if 0
static BOOL GetIPAddr (const char * psAddr, DWORD & dwRtn)
{

	// a NULL string is ok
	if (*psAddr == 0)
		{
		dwRtn = 0;
		return (FALSE);
		}

	// break up the IP address
	BOOL bErr = FALSE;
	int iSub = atoi (psAddr);
	if ((iSub < 0) || (iSub > 255) || ((iSub == 0) && (*psAddr != '0')))
		bErr = TRUE;
	dwRtn = (iSub & 0xFF) << 24;
	while (isdigit (*psAddr))
		psAddr++;
	if (*psAddr == '.')
		psAddr++;
	else
		bErr = TRUE;

	iSub = atoi (psAddr);
	if ((iSub < 0) || (iSub > 255) || ((iSub == 0) && (*psAddr != '0')))
		bErr = TRUE;
	dwRtn |= (iSub & 0xFF) << 16;
	while (isdigit (*psAddr))
		psAddr++;
	if (*psAddr == '.')
		psAddr++;
	else
		bErr = TRUE;

	iSub = atoi (psAddr);
	if ((iSub < 0) || (iSub > 255) || ((iSub == 0) && (*psAddr != '0')))
		bErr = TRUE;
	dwRtn |= (iSub & 0xFF) << 8;
	while (isdigit (*psAddr))
		psAddr++;
	if (*psAddr == '.')
		psAddr++;
	else
		bErr = TRUE;

	iSub = atoi (psAddr);
	if ((iSub < 0) || (iSub > 255) || ((iSub == 0) && (*psAddr != '0')))
		bErr = TRUE;
	dwRtn |= (iSub & 0xFF);
	while (isdigit (*psAddr))
		psAddr++;
	if (*psAddr != 0)
		bErr = TRUE;

	return (bErr);
}
#endif

// TCP/IP dialog box
BOOL CALLBACK EXPORT DlgProcIP (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char sIPAddr[128];

	switch (uMsg)
	  {
		case WM_INITDIALOG : {

			int iPort = vpFetchInt ("TCP", "WellKnownPort", DEF_TCP_PORT);
			SetDlgItemInt (hDlg, IDC_IP_SOCKET, iPort, FALSE);

			LPCSTR addrKey = (lParam) ? "RegistrationAddress" : "ServerAddress";
			vpFetchString ("TCP", addrKey, "", sIPAddr, 127);
			if (!sIPAddr[0])
				strcpy(sIPAddr, DEF_IP_REG_SERVER);

			SetDlgItemText (hDlg, IDC_IP_SERVER_ADDR, sIPAddr);

			SetWindowLong (hDlg, DWL_USER, lParam);
			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == EN_CHANGE)
				{
				switch (LOWORD (wParam))
					case IDC_IP_SOCKET : 
					case IDC_IP_SERVER_ADDR : {
						BOOL bOK;
						char sBuf[20];
						GetDlgItemText (hDlg, IDC_IP_SOCKET, sBuf, 2);
						if (sBuf[0] == 0)
							bOK = TRUE;
						else
							GetDlgItemInt (hDlg, IDC_IP_SOCKET, &bOK, FALSE);

						GetDlgItemText (hDlg, IDC_IP_SERVER_ADDR, sBuf, 127);
						if (bOK)
							EnableWindow (GetDlgItem (hDlg, IDOK), TRUE);
						else
							EnableWindow (GetDlgItem (hDlg, IDOK), FALSE);
						break; }
				break;
				}

			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {

					case IDOK : {
						// if it's empty use the default
						char sBuf[3];
						GetDlgItemText (hDlg, IDC_IP_SOCKET, sBuf, 2);
						int iPort;
						if (sBuf[0] == 0)
							iPort = DEF_TCP_PORT;
						else
							iPort = GetDlgItemInt (hDlg, IDC_IP_SOCKET, NULL, FALSE);
						vpStoreInt ("TCP", "WellKnownPort", iPort);

						GetDlgItemText (hDlg, IDC_IP_SERVER_ADDR, sIPAddr, 127);
						sIPAddr[127] = 0;
						LPCSTR addrKey = GetWindowLong (hDlg, DWL_USER) ? "RegistrationAddress" : "ServerAddress";
						vpStoreString ("TCP", addrKey, sIPAddr);

						EndDialog (hDlg, -1);
						return (TRUE); }

					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);
					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
					case IDC_DIAG:
						{
							BOOL serverMode = GetWindowLong(hDlg, DWL_USER);
							WinExec(serverMode ? 
								"vpdiag /TCP /SERVER" :
								"vpdiag /TCP", SW_SHOW);
							return TRUE;
						}

			}
			break;
	  }

	return (FALSE);
}

// IPX dialog box
BOOL CALLBACK EXPORT DlgProcIPX (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			// VADIM - is [20] long enough
			char netAddr[32], nodeAddr[32];
			UINT portNum;


			if (lParam)
			{
				char regAddr[128];

				vpFetchString("IPX", "RegistrationAddress", "", regAddr, sizeof(regAddr));
				ParseIpxAddress(regAddr, netAddr, nodeAddr, portNum);
			}
			else
			{
				vpFetchString ("IPX", "ServerNet", "", netAddr, sizeof(netAddr));

				vpFetchString ("IPX", "ServerNode", "", nodeAddr, sizeof(nodeAddr));

				portNum =  vpFetchInt ("IPX", "WellKnownPort", DEF_IPX_PORT);
			}

			SetDlgItemText (hDlg, IDC_IPX_NET_NUM, netAddr);
			SetDlgItemText (hDlg, IDC_IPX_NODE_NUM, nodeAddr);
			SetDlgItemInt (hDlg, IDC_IPX_SOCKET, portNum, FALSE);


			SetWindowLong (hDlg, DWL_USER, lParam);

			// VADIM - should any be disabled if it's the server?
			return (TRUE); }

		case WM_COMMAND :
#ifdef VADIM
	// can we test to see if numbers are legit?
			if (HIWORD (wParam) == EN_CHANGE)
				{
				switch (LOWORD (wParam))
					case IDC_IPX_NET_NUM :
					case IDC_IPX_NODE_NUM :
					case IDC_IPX_SOCKET : {
						BOOL bOK;

						EnableWindow (GetDlgItem (hDlg, IDOK), bOK);
						break; }
				break;
				}
#endif

			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {

					case IDOK : {
						const int ASIZE=48;
						char netAddr[ASIZE+1], nodeAddr[ASIZE+1];
						char portStr[ASIZE+1];
						UINT portNum;

						GetDlgItemText (hDlg, IDC_IPX_NET_NUM, netAddr, ASIZE);
						GetDlgItemText (hDlg, IDC_IPX_NODE_NUM, nodeAddr, ASIZE);
						GetDlgItemText (hDlg, IDC_IPX_SOCKET, portStr, ASIZE);


						netAddr[ASIZE] = nodeAddr[ASIZE] = portStr[ASIZE] = 0;
						if (portStr[0])
						{
							portNum = atoi(portStr);
						}
						else
						{
							portNum = DEF_IPX_PORT;
						}


						if (GetWindowLong(hDlg, DWL_USER))
						{
							char regAddr[128];

							MakeIpxAddress(regAddr, netAddr, nodeAddr, portNum);

							vpStoreString("IPX", "RegistrationAddress", regAddr);
						}
						else
						{

							vpStoreString ("IPX", "ServerNet", netAddr);
							vpStoreString ("IPX", "ServerNode", nodeAddr);
							vpStoreInt ("IPX", "WellKnownPort", portNum);
						}

						EndDialog (hDlg, -1);
						return (TRUE); }

					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);
					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
					case IDC_DIAG:
						{
							BOOL serverMode = GetWindowLong(hDlg, DWL_USER);
							WinExec(serverMode ? 
								"vpdiag /ipx /SERVER" :
								"vpdiag /ipx", SW_SHOW);
							return TRUE;
						}
				  }
			break;
	  }

	return (FALSE);
}

// NETBIOS dialog box
BOOL CALLBACK EXPORT DlgProcNETBIOS (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	void vpGetLanas(LANA_ENUM* );

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			// put all legit LANA numbers in combo box
			NCB ncb;
			char sBuf[50];

			SetWindowLong (hDlg, DWL_USER, lParam);

			HWND hWnd = GetDlgItem (hDlg, IDC_NETBIOS_LANA);
			LANA_ENUM lEnum;

			vpGetLanas(&lEnum);

			for(int i = 0; i < lEnum.length; i++)
			{
				wsprintf(sBuf, "%u", lEnum.lana[i]);
				SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)sBuf);
			}
			
			// select the default one
			int iLana = vpFetchInt("NETBIOS", "LANA", DEF_NETBIOS_LANA);
			itoa (iLana, sBuf, 10);
			int iIndex = (int) SendMessage (hWnd, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) (LPCSTR) sBuf);
			if (iIndex < 0)
				iIndex = 0;
			SendMessage (hWnd, CB_SETCURSEL, iIndex, 0);

			// station name
			// VADIM - need the default derived name here
			vpFetchString ("NETBIOS", "StationName", "", sBuf, 48);
			SetDlgItemText (hDlg, IDC_NETBIOS_STATION, sBuf);

			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {

					case IDOK : {
						char sBuf[50];
						HWND hWnd = GetDlgItem (hDlg, IDC_NETBIOS_LANA);
						int iLana = DEF_NETBIOS_LANA;
						int iIndex = (int) SendMessage (hWnd, CB_GETCURSEL, 0, 0);
						if (iIndex >= 0)
							{
							SendMessage (hWnd, CB_GETLBTEXT, iIndex, (LPARAM) sBuf);
							iLana = atoi (sBuf);
							}
						vpStoreInt ("NETBIOS", "LANA", iLana);

						GetDlgItemText (hDlg, IDC_NETBIOS_STATION, sBuf, 48);
						sBuf[48] = 0;
						vpStoreString ("NETBIOS", "StationName", sBuf);

						EndDialog (hDlg, -1);
						return (TRUE); }

					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);
					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
					case IDC_DIAG:
						{
							BOOL serverMode = GetWindowLong(hDlg, DWL_USER);
							WinExec(serverMode ? 
								"vpdiag /NETBIOS /SERVER" :
								"vpdiag /NETBIOS", SW_SHOW);
							return TRUE;
						}
		
				  }
			break;
	  }

	return (FALSE);
}

// COMM dialog box
BOOL CALLBACK EXPORT DlgProcCOMM (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			// put all accessable COM numbers in combo box
			HWND hWnd = GetDlgItem (hDlg, IDC_DIRECT_PORT);
#ifdef WIN32
			for (int iPort=0; iPort<255; iPort++)
				{
				char sBuf[10];
				strcpy (sBuf, "COM");
				itoa (iPort, &sBuf[3], 10);
				HANDLE dev = CreateFile (sBuf, GENERIC_READ | GENERIC_WRITE, 0, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
				if (dev != INVALID_HANDLE_VALUE)
					{
					CloseHandle (dev);
					SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) sBuf);
					}
				}
#else
			for (int iPort=0; iPort<255; iPort++)
				{
				char sBuf[10];
				strcpy (sBuf, "COM");
				itoa (iPort, &sBuf[3], 10);
				int iHdl = OpenComm (sBuf, 1024, 1024);
				if (iHdl >= 0)
					{
					CloseComm (iHdl);
					SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) sBuf);
					}
				}
#endif

			// select the default one
			char sBuf[12];
			vpFetchString ("COMM", "Port", DEF_COMM_PORT, sBuf, 10);
			int iIndex = (int) SendMessage (hWnd, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) sBuf);
			if (iIndex < 0)
				iIndex = 0;
			SendMessage (hWnd, CB_SETCURSEL, iIndex, 0);

			SetWindowLong (hDlg, DWL_USER, lParam);
			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {

					case IDOK : {
						char sBuf[50];
						HWND hWnd = GetDlgItem (hDlg, IDC_DIRECT_PORT);
						int iIndex = (int) SendMessage (hWnd, CB_GETCURSEL, 0, 0);
						if (iIndex >= 0)
							SendMessage (hWnd, CB_GETLBTEXT, iIndex, (LPARAM) sBuf);
						else
							strcpy (sBuf, DEF_COMM_PORT);
						vpStoreString ("COMM", "Port", sBuf);

						EndDialog (hDlg, -1);
						return (TRUE); }

					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);
					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
				  }
			break;
	  }

	return (FALSE);
}

// MODEM settings dialog box
BOOL CALLBACK EXPORT DlgProcMODEMSettings (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			char sBuf[82];
			vpFetchString ("MODEM", "CALL_INIT", "ATZ", sBuf, 80);
			SetDlgItemText (hDlg, IDC_CLIENT_INIT, sBuf);

			vpFetchString ("MODEM", "LISTEN_INIT", "ATS0=1", sBuf, 80);
			SetDlgItemText (hDlg, IDC_SERVER_INIT, sBuf);

			vpFetchString ("MODEM", "DIAL_PREFIX", "ATDT", sBuf, 80);
			SetDlgItemText (hDlg, IDC_DIAL_PREFIX, sBuf);

			vpFetchString ("MODEM", "DIAL_SUFFIX", "", sBuf, 80);
			SetDlgItemText (hDlg, IDC_DIAL_SUFFIX, sBuf);

#if 0
			vpFetchString ("MODEM", "Answer", DEF_MODEM_ANSWER, sBuf, 80);
			SetDlgItemText (hDlg, IDC_MODEM_ANSWER, sBuf);

			vpFetchString ("MODEM", "Hangup", DEF_MODEM_HANGUP, sBuf, 80);
			SetDlgItemText (hDlg, IDC_MODEM_HANGUP, sBuf);

			vpFetchString ("MODEM", "Connect", DEF_MODEM_CONNECT, sBuf, 80);
			SetDlgItemText (hDlg, IDC_MODEM_CONNECT, sBuf);

			vpFetchString ("MODEM", "Failure", DEF_MODEM_FAILURE, sBuf, 80);
			SetDlgItemText (hDlg, IDC_MODEM_FAILURE, sBuf);
 #endif

			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {

					case IDOK : {
						char sBuf[82];
						sBuf[81] = 0;
						GetDlgItemText (hDlg, IDC_CLIENT_INIT, sBuf, 80);
						vpStoreString ("MODEM", "CALL_INIT", sBuf);

 						GetDlgItemText (hDlg, IDC_SERVER_INIT, sBuf, 80);
						vpStoreString ("MODEM", "LISTEN_INIT", sBuf);

						GetDlgItemText (hDlg, IDC_DIAL_PREFIX, sBuf, 80);
						vpStoreString ("MODEM", "DIAL_PREFIX", sBuf);

						GetDlgItemText (hDlg, IDC_DIAL_SUFFIX, sBuf, 80);
						vpStoreString ("MODEM", "DIAL_SUFFIX", sBuf);
#if 0
						GetDlgItemText (hDlg, IDC_MODEM_ANSWER, sBuf, 80);
						vpStoreString ("MODEM", "Answer", sBuf);

						GetDlgItemText (hDlg, IDC_MODEM_HANGUP, sBuf, 80);
						vpStoreString ("MODEM", "Hangup", sBuf);

						GetDlgItemText (hDlg, IDC_MODEM_CONNECT, sBuf, 80);
						vpStoreString ("MODEM", "Connect", sBuf);

						GetDlgItemText (hDlg, IDC_MODEM_FAILURE, sBuf, 80);
						vpStoreString ("MODEM", "Failure", sBuf);
#endif
						EndDialog (hDlg, -1);
						return (TRUE); }

					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);
					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
				  }
			break;
	  }

	return (FALSE);
}

// MODEM dialog box
BOOL CALLBACK EXPORT DlgProcMODEM (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			// put all accessable COM numbers in combo box
			HWND hWnd = GetDlgItem (hDlg, IDC_MODEM_PORT);
#ifdef WIN32
			for (int iPort=0; iPort<255; iPort++)
				{
				char sBuf[10];
				strcpy (sBuf, "COM");
				itoa (iPort, &sBuf[3], 10);
				HANDLE dev = CreateFile (sBuf, GENERIC_READ | GENERIC_WRITE, 0, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
				if (dev != INVALID_HANDLE_VALUE)
					{
					CloseHandle (dev);
					SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) sBuf);
					}
				}
#else
			for (int iPort=0; iPort<255; iPort++)
				{
				char sBuf[10];
				strcpy (sBuf, "COM");
				itoa (iPort, &sBuf[3], 10);
				int iHdl = OpenComm (sBuf, 1024, 1024);
				if (iHdl >= 0)
					{
					CloseComm (iHdl);
					SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) sBuf);
					}
				}
#endif

			// select the default one
			char sBuf[82];
			vpFetchString ("MODEM", "Port", DEF_COMM_PORT, sBuf, 10);
			int iIndex = (int) SendMessage (hWnd, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) sBuf);
			if (iIndex < 0)
				iIndex = 0;
			SendMessage (hWnd, CB_SETCURSEL, iIndex, 0);

#if 0
			vpFetchString ("MODEM", "Prefix", "", sBuf, 80);
			SetDlgItemText (hDlg, IDC_MODEM_PREFIX, sBuf);
#endif

			vpFetchString ("MODEM", "DIAL_NUMBER", "", sBuf, 80);
			SetDlgItemText (hDlg, IDC_MODEM_NUMBER, sBuf);

			SetWindowLong (hDlg, DWL_USER, lParam);
			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == BN_CLICKED)
				switch (LOWORD (wParam))
				  {

					case IDOK : {
						char sBuf[82];
						HWND hWnd = GetDlgItem (hDlg, IDC_MODEM_PORT);
						int iIndex = (int) SendMessage (hWnd, CB_GETCURSEL, 0, 0);
						if (iIndex >= 0)
							SendMessage (hWnd, CB_GETLBTEXT, iIndex, (LPARAM) sBuf);
						else
							strcpy (sBuf, DEF_COMM_PORT);
						vpStoreString ("MODEM", "Port", sBuf);

#if 0
						GetDlgItemText (hDlg, IDC_MODEM_PREFIX, sBuf, 80);
						sBuf[80] = 0;
						vpStoreString ("MODEM", "Prefix", sBuf);
#endif

						GetDlgItemText (hDlg, IDC_MODEM_NUMBER, sBuf, 80);
						sBuf[80] = 0;
						vpStoreString ("MODEM", "DIAL_NUMBER", sBuf);

						EndDialog (hDlg, -1);
						return (TRUE); }

					case IDC_MODEM_ADV :
						DialogBox (vphInst, MAKEINTRESOURCE (IDD_ADV_MODEM_SETTINGS), hDlg, (DLGPROC) DlgProcMODEMSettings);
						return (TRUE);
						
					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);
					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
				  }
			break;
	  }

	return (FALSE);
}


#if defined(WIN32)
static void TapiEnumProc(LPVOID ctx, LPCSTR devName, UINT devId)
{
	HWND hWnd = GetDlgItem ((HWND) ctx, IDC_TAPI_DEVICE);

	SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) devName);
}


BOOL CALLBACK EXPORT DlgProcTAPI (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char sBuf[82];
	char fName[256];


	switch (uMsg)
	  {
		case WM_INITDIALOG : 
		{
			vpMakeIniFile(fName);

			CTapiConnection* tapiObj = new CTapiConnection;

			SetWindowLong (hDlg, DWL_USER, (DWORD) tapiObj);

			tapiObj->Create(vphInst);

			tapiObj->EnumDevices(TapiEnumProc, hDlg);
			
			tapiObj->LoadConfig(fName, "TAPI");

			HWND hWnd = GetDlgItem (hDlg, IDC_TAPI_DEVICE);

			SendMessage (hWnd, CB_SETCURSEL, tapiObj->DeviceId(), 0);
			vpFetchString ("TAPI", "DIAL_NUMBER", "", sBuf, 80);

			SetDlgItemText(hDlg, IDC_TAPI_NUMBER, sBuf);

			return TRUE;
		}

		case WM_COMMAND :
 			if (HIWORD (wParam) == BN_CLICKED)
			{
				CTapiConnection* tapiObj = 	(CTapiConnection*) GetWindowLong (hDlg, DWL_USER);

				switch (LOWORD (wParam))
				{

					case IDOK : 
						{
						HWND hWnd = GetDlgItem (hDlg, IDC_TAPI_DEVICE);
						int iIndex = (int) SendMessage (hWnd, CB_GETCURSEL, 0, 0);
						vpMakeIniFile(fName);

						tapiObj->SelectDevice(iIndex);
						tapiObj->StoreConfig(fName, "TAPI");
						GetDlgItemText (hDlg, IDC_TAPI_NUMBER, sBuf, 80);
						sBuf[80] = 0;
						vpStoreString ("TAPI", "DIAL_NUMBER", sBuf);

						delete tapiObj;
						EndDialog (hDlg, -1);
						return (TRUE); 
						}

					case IDC_TAPI_SETTINGS :
						{
						HWND hWnd = GetDlgItem (hDlg, IDC_TAPI_DEVICE);
						int iIndex = (int) SendMessage (hWnd, CB_GETCURSEL, 0, 0);
						tapiObj->SelectDevice(iIndex);

						tapiObj->m_cfgInfo.Edit(tapiObj->DeviceId(), hDlg);

						return (TRUE);
						}
					case IDCANCEL :
						delete tapiObj;
						EndDialog (hDlg, 0);
						return (TRUE);

					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
				}
			}
			break;
	}

	return FALSE;
}

#ifdef WITH_DP
struct dpEnumCtx
{
	HWND hDlg;
	GUID guid;
	int  idx;
};

BOOL FAR PASCAL EnumDP(LPGUID lpGuid, LPSTR lpDesc, DWORD dwMajorVer, DWORD dwMinorVer, LPVOID lpv)
{
	dpEnumCtx*  ctx = (dpEnumCtx*) lpv;
	HWND hWnd = GetDlgItem (ctx->hDlg, IDC_DP_PROVIDER);

	int idx = SendMessage (hWnd, LB_ADDSTRING, 0, (LPARAM) lpDesc);

    SendMessage(hWnd, LB_SETITEMDATA, idx, (LPARAM) lpGuid);

	// if the current guid matches one loaded from the ini file	remeber it's
	// listbox index
	if (!memcmp(lpGuid, &ctx->guid, sizeof(ctx->guid)))
	{
		ctx->idx = idx;
	}

    return(TRUE);
}



BOOL CALLBACK EXPORT DlgProcDP (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{


	switch (uMsg)
	  {
		case WM_INITDIALOG : 
		{
			dpEnumCtx ctx;
			char buf[128];
			GUID& GUIDFromString(GUID& guid, LPCSTR lpszA);

			memset(&ctx, 0, sizeof(ctx));
			ctx.idx = -1;
			ctx.hDlg = hDlg;

			
			buf[0] = 0;
			vpFetchString("DP", "Provider", "", buf, 128);
			if (strlen(buf))
				GUIDFromString(ctx.guid, buf);


			pDirectPlayEnumerate(EnumDP, &ctx);

			if (ctx.idx != -1)
			{
				HWND hWnd = GetDlgItem (hDlg, IDC_DP_PROVIDER);
				SendMessage (hWnd, LB_SETCURSEL, ctx.idx, 0);
			}

			return TRUE;
		}

		case WM_COMMAND :
 			if (HIWORD (wParam) == BN_CLICKED)
			{

				switch (LOWORD (wParam))
				{

					case IDOK : 
						{
						HWND hWnd = GetDlgItem (hDlg, IDC_DP_PROVIDER);
						int idx = (int) SendMessage (hWnd, LB_GETCURSEL, 0, 0);
						LPGUID pGuid = (LPGUID) SendMessage (hWnd, LB_GETITEMDATA, idx, 0);
						char buf[64];
						BOOL StringFromGUID(LPSTR buf, GUID& guid);

						StringFromGUID(buf, *pGuid);
						
						vpStoreString("DP", "Provider", buf);

						EndDialog (hDlg, -1);
						return (TRUE); 
						}

					case IDCANCEL :
						EndDialog (hDlg, 0);
						return (TRUE);

					case IDX_HELP :
						WinHelp (hDlg, "vdmplay.hlp", HELP_CONTEXT, HLP_TCPIP);
						return (TRUE);
				}
			}
			break;
	}

	return FALSE;
}

#endif

#endif



void doAdvDialog (HWND hWnd, int iProtocol, BOOL bServer)
{

	HINSTANCE hInst = vphInst;

	if (! (vpSupportedTransports () & (1 << iProtocol)))
		return;
		
	switch (iProtocol)
	{
		case VPT_TCP :
			DialogBoxParam (hInst, MAKEINTRESOURCE (IDD_ADV_TCPIP), hWnd, (DLGPROC) DlgProcIP, bServer);
			break;
		case VPT_IPX :
			DialogBoxParam (hInst, MAKEINTRESOURCE (IDD_ADV_IPX), hWnd, (DLGPROC) DlgProcIPX, bServer);
			break;
		case VPT_NETBIOS :
			DialogBoxParam (hInst, MAKEINTRESOURCE (IDD_ADV_NETBIOS), hWnd, (DLGPROC) DlgProcNETBIOS, bServer);
			break;
		case VPT_COMM :
			DialogBoxParam (hInst, MAKEINTRESOURCE (IDD_ADV_COMM), hWnd, (DLGPROC) DlgProcCOMM, bServer);
			break;
		case VPT_MODEM :
			DialogBoxParam (hInst, MAKEINTRESOURCE (IDD_ADV_MODEM), hWnd, (DLGPROC) DlgProcMODEM, bServer);
			break;
#if defined(WIN32)
		case VPT_TAPI:
			DialogBoxParam(hInst, MAKEINTRESOURCE (IDD_ADV_TAPI), hWnd, (DLGPROC) DlgProcTAPI, bServer);
			break;
#ifdef WITH_DP
		case VPT_DP:
			DialogBoxParam(hInst, MAKEINTRESOURCE (IDD_ADV_DP), hWnd, (DLGPROC) DlgProcDP, bServer);
			break;
#endif
#endif
	}
}



