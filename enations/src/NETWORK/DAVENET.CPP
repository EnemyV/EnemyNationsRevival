#include "stdafx.h"
#include "resource.h"
#include "version.h"
#include "_davenet.h"

// HINSTANCE NEAR afxCurrentInstanceHandle = 0;
// HINSTANCE NEAR afxCurrentResourceHandle = 0;

// BUGBUG - don't make CODE DATA fixed for all protocols

HINSTANCE hInst = NULL;

extern "C" int APIENTRY
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID)
{

	switch (dwReason)
	  {
		case DLL_PROCESS_ATTACH :
			hInst = hinstDLL;
			break;
	  }

	return (TRUE);
}

DAVENETAPI long naVersion ()
{

	return (MAKELONG (VER_RELEASE, MAKEWORD (VER_MINOR, VER_MAJOR)));
}

DAVENETAPI BOOL naHave (int ID)
{

	switch (ID)
		{
		case NET_PROTO_NETBIOS :
			return (CNetbios::Have ());
		}

	return (FALSE);
}

CProtocol::~CProtocol ()
{
}

void CProtocol::MsgBox (int iRes)
{

	char *pBuf = (char *) malloc (516);
	if (pBuf == NULL)
		return;
		

	LoadString (hInst, iRes, pBuf, 256);
	LoadString (hInst, IDS_TITLE, pBuf+258, 256);
	MessageBox (m_hWnd, pBuf, pBuf+258, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

	free (pBuf);
}

void CProtocol::MsgBox (char const *psErr)
{

	char sTitle [42];
	LoadString (hInst, IDS_TITLE, sTitle, 40);
	MessageBox (m_hWnd, psErr, sTitle, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
}

DAVENETAPI void * naInit (int ID, HINSTANCE hInst, HWND hWnd)
{

	CProtocol *pRtn;
	switch (ID)
		{
		case NET_PROTO_NETBIOS :
			pRtn = new CNetbios (hInst, hWnd);
			break;
		default:
			pRtn = NULL;
			break;
		}

	if (pRtn == NULL)
		return (NULL);
	if (pRtn->InitOk ())
		return (pRtn);

	// something is wrong
	delete pRtn;
	return (NULL);
}

DAVENETAPI void naErrMsgBox (void  *pHdl, NETMSG * pMsg)
{

	((CProtocol*)pHdl)->ErrMsgBox (pMsg);
}

DAVENETAPI void naClose (void  * pHdl)
{

	((CProtocol*)pHdl)->Close ();
	delete (CProtocol *) pHdl;
}

DAVENETAPI BOOL naAddName (void  * pHdl, LPCSTR pName, LPCVOID pData)
{

	return (((CProtocol*)pHdl)->AddName (pName, pData));
}

DAVENETAPI BOOL naAddGroupName (void  * pHdl, LPCSTR pName, LPCVOID pData)
{

	return (((CProtocol*)pHdl)->AddGroupName (pName, pData));
}

DAVENETAPI BOOL naCall (void  * pHdl, LPCSTR pLocalName, LPCSTR pRemoteName, LPCVOID pData)
{

	return (((CProtocol*)pHdl)->Call (pLocalName, pRemoteName, pData));
}

DAVENETAPI void naCancelReceive (void  * pHdl, int iNum)
{

	((CProtocol*)pHdl)->CancelReceive (iNum);
}

DAVENETAPI void naCancelReceiveDatagram (void  * pHdl, int iNum)
{

	((CProtocol*)pHdl)->CancelReceiveDatagram (iNum);
}

DAVENETAPI BOOL naDeleteName (void  * pHdl, LPCSTR pName, LPCVOID pData)
{

	return (((CProtocol*)pHdl)->DeleteName (pName, pData));
}

DAVENETAPI BOOL naHangUp (void  * pHdl, int iNum, LPCVOID pUser)
{

	return (((CProtocol*)pHdl)->HangUp (iNum, pUser));
}

DAVENETAPI BOOL naListen (void  * pHdl, LPCSTR pLocalName, LPCSTR pRemoteName, LPCVOID pData)
{

	return (((CProtocol*)pHdl)->Listen (pLocalName, pRemoteName, pData));
}

DAVENETAPI BOOL naReceive (void  * pHdl, int iNum, LPCVOID pUser)
{

	return (((CProtocol*)pHdl)->Receive (iNum, pUser));
}

DAVENETAPI BOOL naReceiveDatagram (void  * pHdl, int iNum, LPCVOID pUser)
{

	return (((CProtocol*)pHdl)->ReceiveDatagram (iNum, pUser));
}

DAVENETAPI BOOL naSend (void  * pHdl, int iNum, LPCVOID pData, int iLen, LPCVOID pUser)
{

	return (((CProtocol*)pHdl)->Send (iNum, pData, iLen, pUser));
}

DAVENETAPI BOOL naSendDatagram (void  * pHdl, int iNum, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser)
{

	return (((CProtocol*)pHdl)->SendDatagram (iNum, pName, pData, iLen, pUser));
}

