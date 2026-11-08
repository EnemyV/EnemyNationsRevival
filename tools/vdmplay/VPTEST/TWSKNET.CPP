#include "stdafx.h"
#include <winsock.h>
#include "tcpnet.h"
#include "smap.h"
#include <iostream.h>



struct NTLink: public CVPLink
{
     CNotification*  m_data;
	 NTLink(CNotification* n) : m_data(n) { }
	 
};

declare(VPList, NTLink);
typedef VPList(NTLink) NTList;
declare2(VPListIterator, NTList, NTLink);


class CNotifyQueue
{
public:
		void Add(CNotification* n)  { m_list.Append(new NTLink(n)); }
		unsigned Count() const { return m_list.Count(); }
		CNotification* Get() 
		{
			if (!Count()) return NULL;
			NTLink* d = m_list.First();
			m_list.Remove(d);
			CNotification* n = d->m_data;
			delete d;
			return n;
		}
		
		~CNotifyQueue()
		{
			while(NULL != Get())
			{
			}
		}
		
	
	   NTList  m_list;
	   
};

class TLocalSession: public CLocalSession
{
public:

    void OnUnexpectedMsg(genericMsg* msg, CNetLink* link, BOOL isSafe)
	{
		cout << "LocalSession:: Unexpected message " 
			<< (char) msg->hdr.msgKind << endl;
	}
	

    void ShouldNotBeHere(LPCSTR aClass, LPCSTR aMethod, LPCSTR txt)
	{
		cout << "Should not be here: " << aClass << " "
			 << aMethod << " " << txt << endl;
	}
	
	virtual BOOL GoodBroadcastOptions(DWORD flags)
	{
		return TRUE;
    }
	
	void PostNotification(CNotification* n)
	{
		m_notifications.Add(n);
	}

	TLocalSession(CNetInterface* net) :
		CLocalSession(net,
				      new CSimplePlayerMap,
					  new CSimpleWSMap) 
	{
	}

	
	CNotifyQueue  m_notifications;
};

class TRemoteSession: public CRemoteSession
{
public:
    void OnUnexpectedMsg(genericMsg* msg, CNetLink* link, BOOL isSafe)
	{
		cout << "RemoteSession:: Unexpected message " 
			<< (char) msg->hdr.msgKind << endl;
	}
	

    void ShouldNotBeHere(LPCSTR aClass, LPCSTR aMethod, LPCSTR txt)
	{
		cout << "Should not be here: " << aClass << " "
			 << aMethod << " " << txt << endl;
	}

    virtual BOOL GoodBroadcastOptions(DWORD flags)
	{
		return TRUE;
    }
	
	void PostNotification(CNotification* n)
	{
		m_notifications.Add(n);
	}
	
	TRemoteSession(CNetInterface* net) :
		CRemoteSession(net,
				      new CSimplePlayerMap,
					  new CSimpleWSMap) 
	{
	}
	
	CNotifyQueue  m_notifications;

};


const char* PrintableAddress(LPCVPNETADDRESS a, CNetInterface* net)
{
	static char buf[256];
	
	CNetAddress* na = net->MakeAddress(a);
	if (a)
	{
		na->GetPrintForm(buf, sizeof(buf));
		na->Unref();
		return buf;
	}
	return "Unknown";
}
 

void PrintPlayer(ostream& os, LPCVPPLAYERINFO info, CNetInterface* net)
{
	const VPPLAYERINFO&  pInfo = *info;
	
	cout << "Player: " << pInfo.playerName << 
		" Id: " << pInfo.playerId << 
			" at address: " << 
	PrintableAddress(&pInfo.playerAddress, net) << endl;
}


const char* msgText(WORD msgCode)
{
		switch(msgCode)
		{
			case VP_JOIN:
				return "VP_JOIN";
			case VP_LEAVE:
				return "VP_LEAVE";
			case VP_READDATA:
				return "VP_READDATA";
			case VP_SENDDATA:
				return "VP_SENDDATA";
			case VP_SESSIONCLOSE:
				return "VP_SESSIONCLOSE";
			case VP_PLAYERENUM:
				return "VP_PLAYERENUM";
			case VP_SESSIONENUM:
				return "VP_SESSIONENUM";
			default:
				return "UNKNOWN MESSSAGE CODE";
		
		}
}
		


void ProcessNotifications(const char* label, CNotifyQueue* q, CNetInterface* net)
{
	CNotification* n;
	
	while(NULL != (n = q->Get()))
	{
		VPMESSAGE& msg = n->m_msg;
		WORD   msgCode = VPGETNOTIFICATION(msg.notificationCode);
		
		cout << label << " Got notification: " << msgText(msgCode) << endl;
		cout << "From:  " << msg.senderId << " for: " << msg.toId << endl;
		
		switch(msgCode)
		{
			case VP_JOIN:
				PrintPlayer(cout, msg.u.playerInfo, net);
				break;
	
			case VP_LEAVE:
				PrintPlayer(cout,  msg.u.playerInfo, net);
				break;				
			case VP_READDATA:
				break;
			case VP_SENDDATA:
				break;
			case VP_SESSIONCLOSE:
				break;
			case VP_PLAYERENUM:
				break;
			case VP_SESSIONENUM:
				break;
				
		}
		
	n->Complete();
	delete n;
	}
}



class TTcpNet: public CTcpNet
{


public:

	
	void ConfigureSocket(SOCKET s, u_long flags)
	{
		u_long on = 1;
		int bufSize = 1024;
		int bvLen = sizeof(int);

		if (!(flags & FD_CONNECT))
		{
			ioctlsocket(s, FIONBIO, &on);
		}

		setsockopt(s, SOL_SOCKET, SO_RCVBUF, (LPCSTR) &bufSize, bvLen);
	}


	BOOL DoListen()
	{
		// m_address.m_stationAddress.s_addr = INADDR_ANY;

		BOOL r = Listen();

		// m_address.m_stationAddress.s_addr = INADDR_LOOPBACK;

		return r;
	}




	TTcpNet() : CTcpNet(0, 0, 0) 
	{

		cout << "Local Address: " << inet_ntoa(m_address.m_stationAddress) << endl;

//		m_address.m_stationAddress.s_addr = INADDR_LOOPBACK;



	}				


	BOOL Poll();




};





BOOL TTcpNet::Poll()
{
	CTCPLink::LinkIter iter(CTCPLink::linkMap);
	CTCPLink* l;
	BOOL gotSomething = FALSE;

	while(NULL != (l = iter()))
	{
		if ((l->m_net == this) && l->HasData())
		{
			gotSomething = TRUE;
			if (l->m_state == CONNECTED)
			{
				if (m_safeHook)
					m_safeHook(l, m_hookData);
			} 
			else
			{
				if (m_unsafeHook)
					m_unsafeHook(l, m_hookData);
			}
		}

	}

	if (m_listenLink)
	{
		CTCPLink* newLink = AcceptLink();

		if (newLink)
		{
			gotSomething = TRUE;
			if (m_connectHook)
				m_connectHook(newLink, m_hookData);

			newLink->Unref();
		}
	 }

	 return gotSomething;

}


void SimulationLoop(TTcpNet* n1, TTcpNet* n2)
{
	
	while(n1->Poll() || n2->Poll())
	  ;
}


struct MYSesInfo
{
	VPSESSIONINFO sesInfo;
	char moreData[10];
};

VPGUID myGuid = { "TESTGAME" };

MYSesInfo myInfo =

{ 
 {
  { "TESTGAME" },   //  gameid
  {  "" },            //  sessionId
  0x1,              //  version,
  0,                // playerCount,
  0,                // sessionFlags
  16,               // playerDataSize
 }
};



void test2()
{
#if 0
	TTcpNet  net1;
	TTcpNet  net2;
	TLocalSession lses(&net1);
	TRemoteSession rses(&net2);
	
	lses.InitNetwork(TRUE);
	rses.InitNetwork(FALSE);

	strcpy(myInfo.sesInfo.sessionName, "TESTSESSION");
	myInfo.sesInfo.dataSize = 1 + strlen(myInfo.sesInfo.sessionName);
	
	lses.InitSessionInfo(&myInfo.sesInfo);

		
	VPNETADDRESS pa;
	
	net1.GetAddress(&pa);

	rses.ConnectToServer(&pa, NULL);

	SimulationLoop(&net1, &net2);
	ProcessNotifications("Server ", &lses.m_notifications, &net1);
	ProcessNotifications("Station ", &rses.m_notifications, &net2);
	
	lses.AddLocalPlayer("VADIM\0               ", 0, 0);
	SimulationLoop(&net1, &net2);
	ProcessNotifications("Server ", &lses.m_notifications, &net1);
	ProcessNotifications("Station ", &rses.m_notifications, &net2);

	rses.AddLocalPlayer("NIKITA\0              ", 0, 0);
	
	SimulationLoop(&net1, &net2);
	ProcessNotifications("Server ", &lses.m_notifications, &net1);
	ProcessNotifications("Station ", &rses.m_notifications, &net2);
	
#endif

	
}

char sessionName[64] = "Test Session";

void test3()
{
	VPHANDLE  vpL = vpStartup(1, &myGuid, 32, 32, VPT_TCP);

	VPHANDLE  vpR = vpStartup(1, &myGuid, 32, 32, VPT_TCP);


	VPSESSIONHANDLE vpsL = vpCreateSession(vpL, NULL, sessionName, 0, NULL);

	
	vp


	
	vpCloseSession(vpsL, NULL);


	vpCleanup(vpL);
	vpCleanup(vpR);

}



void test1()
{
	char buf[256];
	DWORD len;

	TTcpNet  net1;
	TTcpNet  net2;

	net1.DoListen();

	CNetAddress* srvA = net1.MakeAddress(NULL);

#if 0
	CNetLink* lnk = net2.MakeUnsafeLink();

	lnk->SendTo(*srvA, "1234567", 8, 0);

	CNetAddress* clntA = net2.MakeAddress(NULL);


	len = net1.m_dgLink->ReceiveFrom(buf, sizeof(buf), *clntA);


	cout << "Got " << len << "Bytes: " << buf << endl;

	clntA->Unref();
	lnk->Unref();
#else
	CNetLink* 
#endif
	lnk = net2.MakeSafeLink(srvA);
	CNetLink* lnk2 = net1.AcceptLink();
	lnk->Send("1234567", 8, 0);

	if (lnk2)
	{

		len = lnk2->Receive(buf, sizeof(buf));
		cout << "Got Safe from lnk2: " << len << " Bytes: " << buf << endl;
		lnk2->Send("Answer:\n", 8, 0);
		lnk2->Send(buf, len, 0);		 
		lnk2->Unref();
	}
	
	len = lnk->Receive(buf, sizeof(buf));
	buf[len]= 0;
	cout << "Got Safe from lnk: " << len << " Bytes: " << buf << endl;

					
	lnk->Unref();
	srvA->Unref();


}
	
int main()
{
	test3();
	return (0);
}
