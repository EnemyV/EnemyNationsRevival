#include "stdafx.h"
#include <iostream.h>
#include <nb30.h>
#include "datagram.h"
#include "nbnet.h"

#include <ctype.h>
#include <process.h>

void vpassertion(LPCSTR text, LPCSTR file, int line);
void vptracemsg(LPCSTR text);
int gTraceLocation = TRUE;


void (*_vpassert)(LPCSTR text, LPCSTR file, int line) = vpassertion;
void (*_vptracemsg)(LPCSTR text) = vptracemsg;

void vpassertion(LPCSTR text, LPCSTR file, int line)
{
	char buf[512];

	wsprintf(buf, "Assertion (%s) in file %s at line %d", text, file, line);
	cout << buf << endl;
	abort();
}

void vptracemsg(LPCSTR text)
{
	cout << text << flush;
}


HANDLE hEvent,srReady;

#ifndef _NDEBUG
int VPTracer::DoTrace(LPCSTR fmt, ...)
{
	va_list ap;
	char buf[1024];
	int offset = 0;

	buf[0] = 0;
	
	if (gTraceLocation && m_file)
	{
		wsprintf(buf,"%s:%d - ", m_file, m_line);
	}
	
	va_start(ap, fmt);
	wvsprintf(buf+strlen(buf), fmt, ap);
	strcat(buf, "\n");
	
	if (_vptracemsg)
		_vptracemsg(buf);

	return (0);
	
}

#endif


typedef char NBNAME[17];
//		     0123456789ABCDEF

static BOOL resetDone = 0;

class TNbNet: public CNbNet
{
public:
		TNbNet(LPCSTR gName, LPCSTR sName, int lana) : CNbNet(gName, sName, lana)
		{
		 m_pair = NULL;
		}

		~TNbNet()
		{

		//	if (m_pair)
		//		m_pair->PrepareCleanup();
		}
		void PrepareCleanup()
		{
			m_cleaningUp = TRUE;
			m_pair = NULL;
		}

		virtual BOOL ResetNetbios()
		{
			if (!resetDone)
				resetDone = CNbNet::ResetNetbios();

			return resetDone;
		}

		static void CALLBACK CompletionHook(PNCB pNcb);

		void OnNcbCompletion(CNcb* ncb) 
		{
			CNbNet::OnNcbCompletion(ncb);
		}

		void SetCompletionHook(CNcb* ncb)
		{
			ncb->SetCompletion(CompletionHook);
		}

		void FixGroupName(TNbNet* n)
		{
			m_groupName = n->m_groupName;
			m_groupName.nameNumber = 0;
			n->m_pair = this;
			m_pair = n;
		}


		BOOL RegisterOneName(int cmd, nameinfo& name)
		{
			if (name.nameNumber != 0)
				return TRUE;

			return CNbNet::RegisterOneName(cmd, name);
		}



		TNbNet* m_pair;
};


void CALLBACK TNbNet::CompletionHook(PNCB pNcb)
{
	CNcb* ncb = CONTAINING_RECORD(pNcb, CNcb, m_ncb);
	
	ncb->m_net->OnNcbCompletion(ncb);
}



NBNAME n1 = "SERVER          ";
NBNAME n2 = "VDMPLAY         ";
NBNAME n3 = "CLIENT          ";


void doTest(const char* portName)
{
    TNbNet ns(n2, n1, 0);
	TNbNet nc(n2, n3, 0);
	BOOL ret1,ret2;

	ns.Init();
	nc.Init();

	ret1 = ns.Listen(TRUE, TRUE);

	nc.FixGroupName(&ns);

	ret1 = nc.Listen(TRUE, FALSE);


	CNetAddress* addr1 = ns.MakeAddress(NULL);
	CNetAddress* addr2 = ns.MakeAddress(NULL);
	CNetLink*    sl = ns.MakeUnsafeLink();
	CNetLink*    cl = nc.MakeUnsafeLink();

	cl->SendTo(*addr1, "Test", 5, VP_BROADCAST);

	char ch = 0;
	while(ch != '!' && !sl->HasData())
	{
	  cout << "\nWaiting: ";
	  cin >> ch;
	}

	char buf[256];
	DWORD size;

	size = sl->ReceiveFrom(buf, sizeof(buf), *addr1);

	buf[size] = 0;


	cout << "Got::  " << buf;

	SetEvent(hEvent);
	
	
}


void doServerTest(const char* portName)
{
    TNbNet ns(n2, n1, 0);
	BOOL ret1;

	ns.Init();

	ret1 = ns.Listen(TRUE, TRUE);

	SetEvent(srReady);


	CNetAddress* addr = ns.MakeAddress(NULL);
	CNetLink*    sl = ns.MakeUnsafeLink();

	char ch = 0;
	while(ch != '!' && !sl->HasData())
	{
	  cout << "\nWaiting: ";
	  cin >> ch;
	}

	char buf[256];
	DWORD size;

	size = sl->ReceiveFrom(buf, sizeof(buf), *addr);
	buf[size] = 0;


	cout << "Got::  " << buf;
		
	
	
}
	 

void doClientTest(const char* portName)
{
   	TNbNet nc(n2, n3, 0);
	BOOL ret1,ret2;

	nc.Init();


	ret1 = nc.Listen(TRUE, FALSE);


	CNetAddress* addr = nc.MakeAddress(NULL);
	CNetLink*    cl = nc.MakeUnsafeLink();

	cl->SendTo(*addr, "Test", 5, VP_BROADCAST);

	
	

}



void clThread(LPVOID )
{
	doClientTest(NULL);
	
	cout << "End client" << endl;
}


void srThread(LPVOID )
{
	doServerTest(NULL);
	cout << "End Server" << endl;

	SetEvent(hEvent);
}




int main(int argc, char* argv[])
{

 	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	switch(toupper(argv[1][0]))
	{
		case 'S':
			srThread(NULL);
			break;
		case 'C':
			clThread(NULL);
			break;
		case 'B':
			_beginthread(srThread, 0, NULL);
			WaitForSingleObject(srReady, INFINITE);
			_beginthread(clThread, 0, NULL);
			break;			
	 }

	WaitForSingleObject(hEvent, INFINITE);


	return 0;
}
							  
