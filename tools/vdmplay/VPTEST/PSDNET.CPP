#include "stdafx.h"
#include "psdnet.h"

#ifdef TEST
#include <iostream.h>
#endif /* */

CPseudoNet* CPseudoNet::nets[MAXNET];

CNetAddress* CPseudoNet::MakeAddress(LPCVPNETADDRESS a)
{
	return MakeAddressObject(*(int *)a);
}

CNetAddress* CPseudoNet::MakeBroadcastAddress()
{
	return MakeAddressObject(-1);
}
	
CNetLink* CPseudoNet::MakeUnsafeLink()
{
	PNLink*  l = new PNLink(m_address, this);
	
	RegisterLink(l);
	return l;
}

void CPseudoNet::RegisterLink(PNLink* l)
{
	for(int i = 0; i < MAXLINK; i++)
	{
		if (!m_links[i]) 
		{
			m_links[i] = l;
			break;
		}
	}
	
	VPASSERT(i < MAXLINK);
}

void CPseudoNet::SimulationLoop()
{
	BOOL done = FALSE;
	
	while(!done)
	{
		done = TRUE;
		
		for(int i = 0; i < MAXNET; i++)
		{
			CPseudoNet* n = CheckNet(i);
			
			if (n && n->Poll())
			{
				done = FALSE;
			}
		}
	}
}
	
	
	

void CPseudoNet::UnregisterLink(PNLink* l)
{
	for(int i = 0; i < MAXLINK; i++)
	{
		if (m_links[i] == l) 
		{
			m_links[i] = 0;
			break;
		}
	}
	
	VPASSERT(i < MAXLINK);
}


CNetAddress* CPseudoNet::PNLink::GetRemoteAddress()
{
	VPASSERT(target);
	return net->MakeAddressObject(target->myAddress);
}


CNetLink* CPseudoNet::MakeSafeLink(CNetAddress* a)
{
	PNAddress& pa = *(PNAddress*) a;
	CPseudoNet* targetNet = CheckNet(pa.index);
	
	if (targetNet)
	{
		PNLink*  myLink = (PNLink*) MakeUnsafeLink();
	    PNLink*  hisLink = (PNLink*) targetNet->MakeUnsafeLink();
		
		myLink->target = hisLink;
		hisLink->target = myLink;
		
		if (targetNet->m_connectHook)
			targetNet->m_connectHook(hisLink, targetNet->m_hookData);
		
#ifdef TEST
		cout << "MakeSafelink: myLink( " << m_address << ") " <<
			(LPVOID) myLink << " hisLink(" << pa.index << ") " <<
				(LPVOID) hisLink << endl;
#endif /* */
		return myLink;
	}
	return NULL;
}

BOOL CPseudoNet::PNLink::SendTo(CNetAddress& a, LPVOID data, DWORD dataSize,
  DWORD flags)
{
	PNAddress& pa = (PNAddress&) a;
	
	if ((pa.index == -1) || (flags & VP_BROADCAST))
	{
		for(int i = 0; i < MAXNET; i++)
		{
			CPseudoNet* tNet = net->CheckNet(i);
			if (tNet)
				tNet->AcceptData(data, dataSize, myAddress);
		}
		return TRUE;
	}
	
	CPseudoNet* aNet = net->CheckNet(pa.index);
	if (aNet)
		aNet->AcceptData(data, dataSize, myAddress);
	
	return TRUE;
}


BOOL CPseudoNet::Listen()
{
	m_listenLink = (PNLink*) MakeUnsafeLink();
	
	return m_listenLink != NULL;
}

void  CPseudoNet::BecomeDeef()
{
	m_listenLink->Unref();
	m_listenLink = 0;
}

BOOL CPseudoNet::Poll()
{
#ifdef TEST
	cout << "Poll for " << m_address << endl;
#endif /* */
    BOOL hadData = FALSE;
	for(int i = 0; i < MAXLINK; i++)
	{
		PNLink* l = m_links[i];
		
		while (l && l->HasData())
		{
#ifdef  TEST
			cout << "Got something for link " << i << endl;
#endif /* */
		    hadData = TRUE;
			if (l->target) // Safe link
			{
				m_safeHook(l, m_hookData);
			}
			else
			{
				m_unsafeHook(l, m_hookData);
			}
		}
	}
	return hadData;
}
		


BOOL CPseudoNet::PNLink::Send(LPVOID data, DWORD dataSize,  DWORD flags)
{
#ifdef TEST	
	cout << "Pnlink:SendData this = " << (LPVOID) this;
	cout  << " target = " << (LPVOID) target << endl;
#endif /* */

	if (!target)
	{
		SetError(VPNET_ERR_LINK_LOST);
		return FALSE;
	}
	
	
	target->AcceptData(data, dataSize, myAddress);
	
	return TRUE;
}


DWORD CPseudoNet::PNLink::Receive(LPVOID data, DWORD dataSize)
{
	if (!target)
	{
		SetError(VPNET_ERR_LINK_LOST);
		return 0;
	}

	
    Datagram* d = dataQ.Get();
	
	if (!d) 
		return 0;
	
	DWORD ret = d->m_size;
	
	_fmemcpy(data, d->m_data, min((WORD)dataSize, (WORD)ret));
	
	delete d;
	return ret;

}

DWORD CPseudoNet::PNLink::ReceiveFrom(LPVOID data, DWORD dataSize, 
  CNetAddress& a)
{
	VPASSERT(!target);
	
    Datagram* d = dataQ.Get();
	
	if (!d) 
		return 0;
	
	DWORD ret = d->m_size;
	
	_fmemcpy(data, d->m_data, min((WORD)dataSize, (WORD)ret));
	
	PNAddress& pa = (PNAddress&) a;
	
	pa.index = d->m_from;
	
	delete d;
	return ret;

}




void CPseudoNet::PNLink::AcceptData(LPVOID data, DWORD dataSize,  int from)
{
#ifdef TEST	
	cout << "Pnlink:AcceptData at " << myAddress 
		 << " Accepting " << dataSize <<
		"bytes from " << from << endl;
#endif

	Datagram* d = new Datagram(data, (WORD) dataSize, from);
	
	
	if (!d || !d->m_data)
	{
#ifdef TEST
		cout << "Pnlink:AcceptData -- can't allocate" << endl;
#endif
		if (d) delete d;
		return;
	}
#ifdef TEST	
	cout << "Pnlink:AcceptData adding datagram to queue" << endl;
#endif /* */
	dataQ.Add(d);
}


#ifdef TEST
#include <iostream.h>

union paddress
{
	int           index;
	VPNETADDRESS a;
};

void SafeDelivery(CNetLink* l, LPVOID )
{
	static char buf[512];
	paddress addr;
	
	CNetAddress* a = l->GetRemoteAddress();
	
	a->GetNormalForm(&addr.a);
	
	a->Unref();
	
	DWORD len ;
	if (0 != (len = l->Receive(buf, sizeof(buf))))
	{
#ifdef TEST
		cout << "Got " << len << " bytes of safe Data from: " <<
			addr.index;
		cout << " <" << buf << ">" << endl;	
#endif /* */
	}
			
}

void UnsafeDelivery(CNetLink* l, LPVOID data)
{
	static char buf[512];
	paddress addr;
	CPseudoNet* net = (CPseudoNet* ) data;
	addr.index = 0;
	
	CNetAddress* a = net->MakeAddressObject(0);
	
	
	DWORD len ;
	if (0 != (len = l->ReceiveFrom(buf, sizeof(buf), *a)))
	{
		a->GetNormalForm(&addr.a);
#ifdef TEST
		cout << "Got " << len << " bytes of unsafe Data from: " <<
			addr.index;
		cout << " <" << buf << ">" << endl;
#endif /* */
	}

	a->Unref();
}
	

void PSNetTest()
{
	CPseudoNet  net1(1), net2(2);
	paddress  pa;
	
	pa.index = 1;

	net1.SetDeliveryHooks(SafeDelivery, UnsafeDelivery, NULL, NULL, &net1);
	net2.SetDeliveryHooks(SafeDelivery, UnsafeDelivery, NULL, NULL, &net2);

	cout << "Doing listen" << endl;
	net1.Listen();
	net2.Listen();
	
	CNetAddress*  a = net2.MakeAddress(&pa.a);

	cout << "Doing Connecting" << endl;

	CNetLink*  safeLnk = net2.MakeSafeLink(a);

	if (!safeLnk)
	{
		cout << "Failed to connect" << endl;
		return;
	}
	
	cout << "Sending data" << endl;
	
	safeLnk->Send("1234567", 8, 0);
	
	cout << "Polling net1" << endl;
	
	net1.Poll();
	
	cout << "Polling net2" << endl;
	net2.Poll();
	
	
	CNetLink* unsafeLnk = net1.MakeUnsafeLink();

	cout << "Doing broadcast" << endl;
	unsafeLnk->SendTo(*a, "BroadCast", 10, VP_BROADCAST);
	
	net1.Poll();
	net2.Poll();
	
	
	cout << "Doing releasing adddress" << endl;
	
	
	
	a->Unref();
	
	
}

int main(int argc, char* argv[])
{
	PSNetTest();
	return (0);
}
#endif /* */
