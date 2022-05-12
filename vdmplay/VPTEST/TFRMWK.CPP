#include <assert.h>
#define ASSERT assert
#include "stdafx.h"
#include "psdnet.h"
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

union paddress
{
	int           index;
	VPNETADDRESS a;
};

struct MYSesInfo
{
	VPSESSIONINFO sesInfo;
	char moreData[10];
};

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

void PrintPlayer(ostream& os, LPVPPLAYERINFO info)
{
	VPPLAYERINFO&  pInfo = *info;
	
	cout << "Player: " << pInfo.playerName << 
		" Id: " << pInfo.playerId << 
			" at address: " << 
	PrintableAddress(&pInfo.playerAddress) << endl;
}

void ProcessNotificatons(const char* label, CNotifyQueue* q)
{
	CNotfication* n;
	
	while(NULL != (n = q->Get())
	{
		VPMESSAGE& msg = q.m_msg;
		WORD   msgCode = VPGETNOTIFICATION(msg.notificationCode));
		
		cout << label << " Got notification: " << msgText[msgCode] << endl;
		cout << "From:  " << msg.fromId << " for: " << msg.toId << endl;
		
		switch(msgCode)
		{
			case VP_JOIN:
				PrintPlayer(cout, (LPVPPLAYERINFO) msg.data);
				break;
	
			case VP_LEAVE:
				PrintPlayer(cout, (LPVPPLAYERINFO) msg.data);
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

void test1()
{
	CPseudoNet  net1(1);
	CPseudoNet  net2(2);
	TLocalSession lses(&net1);
	TRemoteSession rses(&net2);
	
	lses.InitNetwork();
	rses.InitNetwork();

	strcpy(myInfo.sesInfo.sessionName, "TESTSESSION");
	myInfo.sesInfo.dataSize = 1 + strlen(myInfo.sesInfo.sessionName);
	
	lses.InitSessionInfo(&myInfo.sesInfo);
	
	paddress pa;
	
	pa.index = 1;
	rses.ConnectToServer(&pa.a);

	net1.SimulationLoop();
	
	lses.AddLocalPlayer("VADIM\0               ", 0, 0);
	rses.AddLocalPlayer("NIKITA\0              ", 0, 0);
	
	net1.SimulationLoop();
	
	
}

int main(int argc, char* argv[])
{
	test1();
	
	return (0);
}

