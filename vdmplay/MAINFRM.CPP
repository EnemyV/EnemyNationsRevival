// mainfrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "vptst16.h"

#include "mainfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define WITHVP(x) x

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_COMMAND(ID_TRANSPORTS_IPX, OnTransportsIpx)
	ON_COMMAND(ID_TRANSPORTS_NETBIOS, OnTransportsNetbios)
	ON_COMMAND(ID_TRANSPORTS_TCP, OnTransportsTcp)
	ON_COMMAND(ID_VDMPLAY_CLEANUP, OnVdmplayCleanup)
	ON_COMMAND(ID_VDMPLAY_CREATESESSION, OnVdmplayCreatesession)
	ON_COMMAND(ID_VDMPLAY_ENUMSESSIONS, OnVdmplayEnumsessions)
	ON_COMMAND(ID_VDMPLAY_GETVERSION, OnVdmplayGetversion)
	ON_COMMAND(ID_VDMPLAY_JOINSESIONN, OnVdmplayJoinsesionn)
	ON_COMMAND(ID_VDMPLAY_STARTUP, OnVdmplayStartup)
	ON_COMMAND(ID_VDMPLAY_STOPENUMSESSIONS, OnVdmplayStopenumsessions)
	ON_COMMAND(ID_VDMPLAY_SUPPORTEDPROTOCOLS, OnVdmplaySupportedprotocols)
	ON_MESSAGE(WM_VPNOTIFY, OnVpNotify)
	ON_COMMAND(ID_VDMPLAY_CLOSESESSION, OnVdmplayClosesession)
	ON_COMMAND(ID_TRANSPORTS_COMM, OnTransportsComm)
	ON_COMMAND(ID_TRANSPORTS_MODEM, OnTransportsModem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


int gTransport;
UINT gTransId;
VPHANDLE gVp;
VPSESSIONHANDLE gSes;

VPGUID myGuid = { "TESTGAME" };


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{                                               
	
	
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

void CMainFrame::SelectTransport(UINT id, int vpTransport)
{
	CMenu* mnu = GetMenu();
	
	if (gTransId != 0)
	{
		mnu->CheckMenuItem(gTransId, MF_UNCHECKED);
	}	


	mnu->CheckMenuItem(id, MF_CHECKED);
	gTransId = id;
	gTransport = vpTransport;
}



/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnTransportsIpx()
{
	// TODO: Add your command handler code here
	SelectTransport(ID_TRANSPORTS_IPX, VPT_IPX);
	
}

void CMainFrame::OnTransportsNetbios()
{
	// TODO: Add your command handler code here
	SelectTransport(ID_TRANSPORTS_NETBIOS, VPT_NETBIOS);
	
}

void CMainFrame::OnTransportsTcp()
{
	// TODO: Add your command handler code here
	SelectTransport(ID_TRANSPORTS_TCP, VPT_TCP);
	
}

void CMainFrame::OnVdmplayCleanup()
{
	// TODO: Add your command handler code here
	ASSERT(gVp);
	
	WITHVP(vpCleanup(gVp));
	
	
	
	
}

void CMainFrame::OnVdmplayCreatesession()
{
	char sessionName[64] = "vptest16";



	WITHVP(gSes = vpCreateSession(gVp, m_hWnd, sessionName, 0, NULL));



	if (!gSes)
	{
		AfxMessageBox("vpCreateSession Failed");
	 	return;
	}

	// TODO: Add your command handler code here
	
}

void CMainFrame::OnVdmplayEnumsessions()
{

	WITHVP(vpEnumSessions(gVp, m_hWnd, TRUE, NULL));

	// TODO: Add your command handler code here
	
}

void CMainFrame::OnVdmplayGetversion()
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnVdmplayJoinsesionn()
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::EnableVpItems(BOOL flag)
{
	CMenu* mnu = GetMenu();
	UINT mark = MF_BYCOMMAND | (flag ? MF_ENABLED : MF_GRAYED);
	UINT markOff = MF_BYCOMMAND | ((!flag) ? MF_ENABLED : MF_GRAYED);
	
	mnu->EnableMenuItem(ID_VDMPLAY_CLEANUP, mark);
	mnu->EnableMenuItem(ID_VDMPLAY_ENUMSESSIONS, mark);
	mnu->EnableMenuItem(ID_VDMPLAY_STOPENUMSESSIONS, mark);
	mnu->EnableMenuItem(ID_VDMPLAY_JOINSESIONN, mark);
	mnu->EnableMenuItem(ID_VDMPLAY_CREATESESSION, mark);
	mnu->EnableMenuItem(ID_VDMPLAY_STARTUP, markOff);
}
	

void CMainFrame::OnVdmplayStartup()
{
	// TODO: Add your command handler code here
	if (!gVp)
	{
		WITHVP(gVp = vpStartup(1, &myGuid, 32, 32, gTransport, NULL));

		if (!gVp)
		{
			AfxMessageBox("vpStartup FAILED");
			return;
		}
		else
			AfxMessageBox("vpStartup OK");
		
		
		EnableVpItems(TRUE);
	}
	
	
}

void CMainFrame::OnVdmplayStopenumsessions()
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnVdmplaySupportedprotocols()
{
	// TODO: Add your command handler code here
	
}

LONG CMainFrame::OnVpNotify(WPARAM w, LPARAM l)
{
	LPCVPMESSAGE  msg = (LPCVPMESSAGE) l;
		
#if 0	
	switch(VPGETNOTIFICATION(w))
	{
		case VP_SESSIONENUM:
			OnSessionEnumNotify((LPCVPMESSAGE) l);
			break;

		case VP_PLAYERENUM:
			OnPlayerEnumNotify((LPCVPMESSAGE) l);
			break;

		case VP_JOIN:
			OnJoinNotify((LPCVPMESSAGE) l);
			break;

		case VP_LEAVE:
			OnLeaveNotify((LPCVPMESSAGE) l);
			break;

		case VP_READDATA:
			OnReadDataNotify((LPCVPMESSAGE) l);
			break;
	
		case VP_SENDDATA:
			OnSendDataNotify((LPCVPMESSAGE) l);
			break;

		case VP_SESSIONCLOSE:
			OnCloseSessionNotify((LPCVPMESSAGE) l);
			break;

	}
#endif

	WITHVP(vpAcknowledge(gVp,  msg));
	// ! Don't touch l after acknwwledge
	 return TRUE;
}


void CMainFrame::OnVdmplayClosesession()
{

	// TODO: Add your command handler code here
	BOOL ret;
	WITHVP(ret = vpCloseSession(gSes, NULL));

	if (!ret)
	{
			AfxMessageBox("vpCloseSession FAILED");
	}
	else
		AfxMessageBox("vpCloseSession OK");

}

void CMainFrame::OnTransportsComm()
{
	// TODO: Add your command handler code here
	WITHVP(SelectTransport(ID_TRANSPORTS_COMM, VPT_COMM));
	
}

void CMainFrame::OnTransportsModem()
{
	// TODO: Add your command handler code here
	WITHVP(SelectTransport(ID_TRANSPORTS_MODEM, VPT_MODEM));
	
}
