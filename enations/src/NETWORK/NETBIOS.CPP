#include "stdafx.h"
#include "stdio.h"
#include "stddef.h"
#include "resource.h"
#include "_davenet.h"


extern HINSTANCE hInst;

const int BUFFER_SIZE = 512;
char  *pBufBase = NULL;
UCHAR nbVerMajor = 2;

extern "C" {
void CALLBACK NetbiosPostHdlr (NCB *pNcb);
}


HWND  hWndMsg = NULL;
BYTE  ErrXlat [256] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 5, 1, 2, 1, 1,
												1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
												1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

void CALLBACK NetbiosPostHdlr (NCB *_pNcb)
{

	MYNCB *pNcb = (MYNCB *) _pNcb;

	// set error code
	pNcb->msg.bErr = ErrXlat [pNcb->ncb.ncb_retcode];

	// node num
	pNcb->msg.bNetNum = pNcb->ncb.ncb_num;

	switch (pNcb->ncb.ncb_command)
		{
		case NCBDGRECV | ASYNCH :
			// length & name
			memcpy (&(pNcb->msg.iLen), &(pNcb->ncb.ncb_length), 16);
			break;

		case NCBRECV | ASYNCH :
			// length & name
			memcpy (&(pNcb->msg.iLen), &(pNcb->ncb.ncb_length), 16);
			pNcb->msg.bNetNum = pNcb->ncb.ncb_lsn;
			break;

		case NCBCALL | ASYNCH :
		case NCBLISTEN | ASYNCH :
			pNcb->msg.bNetNum = pNcb->ncb.ncb_lsn;
			break;
		}

	// post the message
	::PostMessage (hWndMsg, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
}

BYTE CNetbios::m_iLanaNum = 0;

BOOL LanaDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	  {
		case WM_INITDIALOG : {
			EnableWindow (GetDlgItem (hDlg, IDOK), FALSE);
			EnableWindow (GetDlgItem (hDlg, IDC_TEMP), FALSE);

			HWND hList = GetDlgItem (hDlg, IDC_NB_LANA);
			for (int iNum=0; iNum<256; iNum++)
				{
				ADAPTER_STATUS as;
				memset (&as, 0, sizeof (as));
				NCB ncb;
				memset (&ncb, 0, sizeof (ncb));
				ncb.ncb_command = NCBASTAT;
				ncb.ncb_buffer = (PUCHAR) &as;
				ncb.ncb_length = sizeof (as);
				ncb.ncb_callname[0] = '*';
				ncb.ncb_lana_num = iNum;

				int iRtn = Netbios (&ncb);
				if (((iRtn == 0) || (iRtn == NRC_INCOMP)) && (as.rev_major >= 1))
					{
					char sBuf[20];
					itoa (iNum, sBuf, 10);
					SendMessage (hList, LB_ADDSTRING, 0, (LPARAM) sBuf);
					}
				}
			return (TRUE); }

		case WM_COMMAND :
			if (HIWORD (wParam) == LBN_SELCHANGE)
				{
				int iInd = SendMessage (GetDlgItem (hDlg, IDC_NB_LANA), LB_GETCURSEL, 0, 0);
				EnableWindow (GetDlgItem (hDlg, IDOK), iInd >= 0);
				EnableWindow (GetDlgItem (hDlg, IDC_TEMP), iInd >= 0);
				}

			if (HIWORD (wParam) == BN_CLICKED)
				{
				HWND hList = GetDlgItem (hDlg, IDC_NB_LANA);
				int iInd = SendMessage (hList, LB_GETCURSEL, 0, 0);
				char sBuf[20];
				if (iInd >= 0)
					{
					SendMessage (hList, LB_GETTEXT, iInd, (LPARAM) sBuf);
					CNetbios::m_iLanaNum = atoi (sBuf);
					}

				switch (LOWORD (wParam))
				  {
					case IDOK :
						WritePrivateProfileString ("NETBIOS", "LanaNum", sBuf, "davenet.ini");
					case IDC_TEMP :
						EndDialog (hDlg, IDOK);
						return (TRUE);
					case IDCANCEL :
						EndDialog (hDlg, IDCANCEL);
						return (TRUE);
				  }
				}
			break;
	  }

	return (FALSE);
}

CNetbios::CNetbios (HINSTANCE hInstApp, HWND hWnd)
{

	m_hInst = hInstApp;
	m_hWnd = hWnd;
	m_ID = NET_PROTO_NETBIOS;
	m_hMem = NULL;
	m_pNcb = NULL;

	// find the first legit lana as default
	ADAPTER_STATUS as;
	NCB ncb;
	for (int iNum=0; iNum<256; iNum++)
		{
		memset (&as, 0, sizeof (as));
		memset (&ncb, 0, sizeof (ncb));
		ncb.ncb_command = NCBASTAT;
		ncb.ncb_buffer = (PUCHAR) &as;
		ncb.ncb_length = sizeof (as);
		ncb.ncb_callname[0] = '*';
		ncb.ncb_lana_num = iNum;

		int iRtn = Netbios (&ncb);
		if (((iRtn == 0) || (iRtn == NRC_INCOMP)) && (as.rev_major >= 1))
			{
			m_iLanaNum = iNum;
			break;
			}
		}

	m_iLanaNum = (BYTE) GetPrivateProfileInt ("NETBIOS", "LanaNum", m_iLanaNum, "davenet.ini");

	// get the version of NETBIOS
	memset (&as, 0, sizeof (as));
	memset (&ncb, 0, sizeof (ncb));
	ncb.ncb_command = NCBASTAT;
	ncb.ncb_buffer = (PUCHAR) &as;
	ncb.ncb_length = sizeof (as);
	ncb.ncb_callname[0] = '*';
	ncb.ncb_lana_num = m_iLanaNum;
	int iRtn = Netbios (&ncb);

	// if we get error 0x23 then we have an invalid lana number
	if (((iRtn != 0) && (iRtn != NRC_INCOMP)) || (as.rev_major < 1))
		{
		if (DialogBox (hInst, MAKEINTRESOURCE (IDD_NETBIOS_LANA), hWnd, (DLGPROC) LanaDlgProc) != IDOK)
			return;

		// get the version of NETBIOS (again)
		memset (&as, 0, sizeof (as));
		memset (&ncb, 0, sizeof (ncb));
		ncb.ncb_command = NCBASTAT;
		ncb.ncb_buffer = (PUCHAR) &as;
		ncb.ncb_length = sizeof (as);
		ncb.ncb_callname[0] = '*';
		ncb.ncb_lana_num = m_iLanaNum;
		iRtn = Netbios (&ncb);
		}

	if (((iRtn == 0) || (iRtn == NRC_INCOMP)) && (as.rev_major >= 1))
		nbVerMajor = as.rev_major;
	else
		{
		char sBuf[172], sFmt[92];;
		LoadString (hInst, IDS_NETBIOS_BAD_VER_ERROR, sFmt, 92);
		sprintf (sBuf, sFmt, m_iLanaNum, as.rev_major, as.rev_minor, iRtn);
		MsgBox (sBuf);
		nbVerMajor = 0;
		return;
		}

	// reset the adapter
	if (nbVerMajor > 2)
		{
		NCB ncb;
		memset (&ncb, 0, sizeof (ncb));
		ncb.ncb_command = NCBRESET;
		ncb.ncb_lsn = 255;
		ncb.ncb_num = 255;
		ncb.ncb_lana_num = m_iLanaNum;
		Netbios (&ncb);
		}

	// save the hWnd to post to
	hWndMsg = hWnd;

	// NCB's locked down
	m_hMem = GlobalAlloc (GMEM_FIXED, (sizeof (MYNCB) + BUFFER_SIZE) * NUM_NCBS);
	if (m_hMem == NULL)
		return;
	m_pNcb = (MYNCB  *) GlobalLock (m_hMem);
	if (m_pNcb != NULL)
		{
		memset (m_pNcb, 0, sizeof (MYNCB) * NUM_NCBS);
		pBufBase = (char  *) (m_pNcb + NUM_NCBS);
		return;
		}

	// we had an error
	MsgBox (IDS_NETBIOS_INIT_ERROR);
}

void CNetbios::NetbiosMsgBox (BYTE cErr)
{

	char * pBuf = (char *) malloc (534);
	if (pBuf == NULL)
		return;
		
	if ((cErr == 0x0E) || (cErr == 0x11))
		LoadString (hInst, IDS_NETBIOS_ERR_TABLE_FULL, pBuf, 256);
	else
		{
		LoadString (hInst, IDS_NETBIOS_ERROR, pBuf + 276, 256);
		sprintf (pBuf, pBuf+276, cErr);
		}
	MsgBox (pBuf);

	free (pBuf);
}

BOOL CNetbios::InitOk ()
{

	if (! m_pNcb)
		return (FALSE);
	return (TRUE);
}

CNetbios::~CNetbios ()
{

	if (m_hMem == NULL)
		return;

	Close ();
	GlobalUnlock (m_hMem);
	GlobalFree (m_hMem);
	m_pNcb = NULL;
	m_hMem = NULL;
}

BOOL CALLBACK nbDlgProc (HWND, UINT, WPARAM, LPARAM)
{

	return (0);
}

// cancel all outstanding receives
void CNetbios::Close ()
{

	if (m_pNcb == NULL)
		return;

	// put up a dialog while we wait
	HWND hDlg = CreateDialog (hInst, MAKEINTRESOURCE (IDD_NETBIOS_CLOSE),
																					m_hWnd, (DLGPROC) nbDlgProc);
	UpdateWindow (hDlg);

	NCB ncb;
	memset (&ncb, 0, sizeof (ncb));
	ncb.ncb_command = NCBCANCEL;
	ncb.ncb_lana_num = m_iLanaNum;

	MYNCB *pNcb = m_pNcb;
	for (int iOn=0; iOn<NUM_NCBS; iOn++, pNcb++)
		if ((pNcb->msg.bInUse) || (pNcb->ncb.ncb_cmd_cplt == 0xFF))
			switch (pNcb->ncb.ncb_command)
				{
				case NCBLISTEN | ASYNCH :
				case NCBCALL | ASYNCH :
				case NCBHANGUP | ASYNCH :
				case NCBRECV | ASYNCH :
				case NCBSEND | ASYNCH :
				case NCBCHAINSEND | ASYNCH :
				case NCBDGRECV | ASYNCH :
				case NCBDGRECVBC | ASYNCH :
						ncb.ncb_buffer = (PUCHAR) pNcb;
						Netbios (&ncb);
					break;
				}

	// wait for the messages to complete
	while (TRUE)
		{
		// are we done?
		pNcb = m_pNcb;
		for (int iOn=0; iOn<NUM_NCBS; iOn++, pNcb++)
			if ((pNcb->msg.bInUse) && (pNcb->ncb.ncb_cmd_cplt == 0xFF))
				goto NotDone;
		break;

NotDone:
		MSG msg;
		while (PeekMessage (&msg, NULL, NULL, NULL, PM_REMOVE))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		}

	// kill the window
	DestroyWindow (hDlg);

	// there may be messages in the queue with pointers to a pMsg so we
	// need to flush the queue
	MSG msg;
	while (PeekMessage (&msg, NULL, NULL, NULL, PM_REMOVE))
		{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		}

	// reset the adapter
	if (nbVerMajor > 2)
		{
		NCB ncb;
		memset (&ncb, 0, sizeof (ncb));
		ncb.ncb_command = NCBRESET;
		ncb.ncb_lsn = 255;
		ncb.ncb_num = 255;
		ncb.ncb_lana_num = m_iLanaNum;
		Netbios (&ncb);
		}
}

BOOL CNetbios::Have ()
{

	NCB ncb;
	memset (&ncb, 0, sizeof (ncb));
	ncb.ncb_command = 0x7F;
	Netbios (&ncb);

	if (((0x40 <= ncb.ncb_retcode) && (ncb.ncb_retcode <= 0x4F)) ||
			((0x50 <= ncb.ncb_retcode) && (ncb.ncb_retcode <= 0xFE)) ||
			(ncb.ncb_retcode == 0x03) || (ncb.ncb_retcode == 0x23))
		return (TRUE);

	return (FALSE);
}

MYNCB *	CNetbios::GetNcb ()
{

	if (m_pNcb == NULL)
		return (NULL);

	MYNCB *pNcb = m_pNcb;
	for (int iOn=0; iOn<NUM_NCBS; iOn++, pNcb++)
		if (! pNcb->msg.bInUse)
			{
			// set the callback & lana
			memset (pNcb, 0, sizeof (MYNCB));
			pNcb->ncb.ncb_lana_num = m_iLanaNum;
	    pNcb->ncb.ncb_post = NetbiosPostHdlr;
			pNcb->msg.bInUse = TRUE;
			pNcb->msg.pData = pNcb->ncb.ncb_buffer = (PUCHAR) pBufBase + BUFFER_SIZE * iOn;
			pNcb->ncb.ncb_length = (WORD) (pNcb->msg.iLen = BUFFER_SIZE);
			return (pNcb);
			}

	MsgBox (IDS_NETBIOS_NO_NCBS);
	return (NULL);
}

void CNetbios::SetName (PUCHAR pDest, LPCSTR pSrc)
{

	int iLeft = NAME_MAX;
	while ((*pSrc) && (iLeft > 0))
		{
		*pDest++ = *pSrc++;
		iLeft--;
		}
	iLeft += NCBNAMSZ - NAME_MAX;
	while (iLeft > 0)
		{
		*pDest++ = ' ';
		iLeft--;
		}
}

void CNetbios::ErrMsgBox (NETMSG * pMsg)
{

	// first we have to convert this to a NCB
	MYNCB *pNcb = (MYNCB *) (((char *) pMsg) - offsetof (MYNCB, msg));
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);
}

BOOL CNetbios::AddGroupName (LPCSTR pName, LPCVOID pData)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBADDGRNAME | ASYNCH;
	SetName (pNcb->ncb.ncb_name, pName);

	pNcb->msg.bCmd = NET_MSG_ADD_GROUP_NAME;
	pNcb->msg.pUser = (LPVOID) pData;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	if (pNcb->ncb.ncb_retcode == NRC_DUPNAME)
		{
		pNcb->msg.bErr = NET_ERR_DUP_LOCAL_NAME;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

#ifdef BUGBUG
		{
		CString sErr;
		sErr.LoadString (IDS_NETBIOS_ADD_GROUP_ERROR);
		csPrintf (&sErr, pName);
		MsgBox (sErr);
		}
#endif

	_asm int 3
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::AddName (LPCSTR pName, LPCVOID pData)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBADDNAME | ASYNCH;
	SetName (pNcb->ncb.ncb_name, pName);

	pNcb->msg.bCmd = NET_MSG_ADD_NAME;
	pNcb->msg.pUser = (LPVOID) pData;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	if ((pNcb->ncb.ncb_retcode == NRC_DUPNAME) || (pNcb->ncb.ncb_retcode == NRC_INUSE))
		{
		pNcb->msg.bErr = (BYTE) ((pNcb->ncb.ncb_retcode == NRC_DUPNAME) ? NET_ERR_DUP_LOCAL_NAME : NET_ERR_DUP_REMOTE_NAME);
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

#ifdef BUGBUG
		{
		CString sErr;
		sErr.LoadString (IDS_NETBIOS_ADD_DUP);
		csPrintf (&sErr, pName);
		MsgBox (sErr);
		}
#endif

	_asm int 3
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::Call (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBCALL | ASYNCH;
	SetName (pNcb->ncb.ncb_name, pLocal);
	SetName (pNcb->ncb.ncb_callname, pRemote);
	pNcb->msg.bCmd = NET_MSG_CALL;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		pNcb->msg.bNetNum = pNcb->ncb.ncb_lsn;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	_asm int 3
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

// cancel all receives for a LSN
void CNetbios::CancelReceive (int iLSN)
{

	if (m_pNcb == NULL)
		return;
	NCB ncb;
	memset (&ncb, 0, sizeof (ncb));
	ncb.ncb_command = NCBCANCEL;
	ncb.ncb_lana_num = m_iLanaNum;

	MYNCB *pNcb = m_pNcb;
	for (int iOn=0; iOn<NUM_NCBS; iOn++, pNcb++)
		if ((pNcb->msg.bInUse) || (pNcb->ncb.ncb_cmd_cplt == 0xFF))
			if (pNcb->ncb.ncb_command == (NCBRECV | ASYNCH))
				if (pNcb->ncb.ncb_lsn == iLSN)
					{
					ncb.ncb_buffer = (PUCHAR) pNcb;
					Netbios (&ncb);
					}
}

// cancel all receives for a LSN
void CNetbios::CancelReceiveDatagram (int iNum)
{

	if (m_pNcb == NULL)
		return;
	NCB ncb;
	memset (&ncb, 0, sizeof (ncb));
	ncb.ncb_command = NCBCANCEL;
	ncb.ncb_lana_num = m_iLanaNum;

	MYNCB *pNcb = m_pNcb;
	for (int iOn=0; iOn<NUM_NCBS; iOn++, pNcb++)
		if ((pNcb->msg.bInUse) || (pNcb->ncb.ncb_cmd_cplt == 0xFF))
			if (pNcb->ncb.ncb_command == (NCBDGRECV | ASYNCH))
				if (pNcb->ncb.ncb_num == iNum)
					{
					ncb.ncb_buffer = (PUCHAR) pNcb;
					Netbios (&ncb);
					}
}

BOOL CNetbios::DeleteName (LPCSTR pName, LPCVOID pData)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBDELNAME | ASYNCH;
	SetName (pNcb->ncb.ncb_name, pName);

	pNcb->msg.bCmd = NET_MSG_DELETE_NAME;
	pNcb->msg.pData = pNcb->ncb.ncb_name;
	pNcb->msg.pUser = (LPVOID) pData;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::HangUp (int iNum, LPCVOID pUser)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBHANGUP | ASYNCH;
	pNcb->ncb.ncb_lsn = pNcb->msg.bNetNum = (BYTE) iNum;
	pNcb->msg.bCmd = NET_MSG_HANG_UP;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if ((pNcb->ncb.ncb_retcode == NRC_GOODRET) || (pNcb->ncb.ncb_retcode == NRC_SNUMOUT))
		{
		pNcb->msg.bErr = (BYTE) ((pNcb->ncb.ncb_retcode == NRC_GOODRET) ? NET_ERR_NONE : NET_ERR_SESSION_CLOSED);
		pNcb->msg.iLen = pNcb->ncb.ncb_length;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::Listen (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBLISTEN | ASYNCH;
	SetName (pNcb->ncb.ncb_name, pLocal);
	SetName (pNcb->ncb.ncb_callname, pRemote);
	pNcb->msg.bCmd = NET_MSG_LISTEN;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		pNcb->msg.bNetNum = pNcb->ncb.ncb_lsn;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::Receive (int iNum, LPCVOID pUser)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBRECV | ASYNCH;
	pNcb->ncb.ncb_lsn = pNcb->msg.bNetNum = (BYTE) iNum;
	pNcb->msg.bCmd = NET_MSG_RECEIVE;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		pNcb->msg.iLen = pNcb->ncb.ncb_length;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::ReceiveDatagram (int iNum, LPCVOID pUser)
{

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);

	// set up and call
	pNcb->ncb.ncb_command = NCBDGRECV | ASYNCH;
	pNcb->ncb.ncb_num = pNcb->msg.bNetNum = (BYTE) iNum;
	pNcb->msg.bCmd = NET_MSG_RECEIVE_DATAGRAM;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		pNcb->msg.iLen = pNcb->ncb.ncb_length;
		memcpy (pNcb->msg.sName, pNcb->ncb.ncb_callname, NAME_MAX);
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	_asm int 3
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::Send (int iNum, LPCVOID pData, int iLen, LPCVOID pUser)
{

//BUGBUG	ASSERT (AfxIsValidString (pName));
//	ASSERT (AfxIsValidAddress (pData, iLen));
//	ASSERT (iLen <= BUFFER_SIZE);

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);
	// BUGBUG - what to do here?
	if (iLen > BUFFER_SIZE)
		iLen = BUFFER_SIZE;

	// set up and call
	pNcb->ncb.ncb_command = NCBSEND | ASYNCH;
	pNcb->ncb.ncb_lsn = pNcb->msg.bNetNum = (BYTE) iNum;
	memcpy (pNcb->ncb.ncb_buffer, pData, iLen);
	pNcb->ncb.ncb_length = (WORD) (pNcb->msg.iLen = (short int) iLen);
	pNcb->msg.bCmd = NET_MSG_SEND;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

BOOL CNetbios::SendDatagram (int iNum, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser)
{

//BUGBUG	ASSERT (AfxIsValidString (pName));
//	ASSERT (AfxIsValidAddress (pData, iLen));
//	ASSERT (iLen <= BUFFER_SIZE);

	MYNCB *pNcb = GetNcb ();
	if (pNcb == NULL)
		return (FALSE);
	// BUGBUG - what to do here?
	if (iLen > 512)
		iLen = 512;

	// set up and call
	pNcb->ncb.ncb_command = NCBDGSEND | ASYNCH;
	pNcb->ncb.ncb_num = pNcb->msg.bNetNum = (BYTE) iNum;
	memcpy (pNcb->ncb.ncb_buffer, pData, iLen);
	pNcb->ncb.ncb_length = (WORD) (pNcb->msg.iLen = (short int) iLen);
	SetName (pNcb->ncb.ncb_callname, pName);

	pNcb->msg.bCmd = NET_MSG_SEND_DATAGRAM;
	pNcb->msg.pUser = (LPVOID) pUser;

	int iRtn = Netbios (&(pNcb->ncb));
	if ((iRtn == 0) || (iRtn == 255))
		return (TRUE);

	// it worked
	if (pNcb->ncb.ncb_retcode == NRC_GOODRET)
		{
		pNcb->msg.bErr = NET_ERR_NONE;
		PostMessage (m_hWnd, WM_NET_COMPLETE, 0, (LPARAM) &(pNcb->msg));
		return (TRUE);
		}

	// it failed
	_asm int 3
	pNcb->msg.bInUse = FALSE;
	NetbiosMsgBox (pNcb->ncb.ncb_retcode);

	return (FALSE);
}

