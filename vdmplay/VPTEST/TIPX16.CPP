#include "../stdafx.h"


#define NWWIN
#include <nwipxspx.h>
#include "datagram.h"
#include "ipx16net.h"



#include <iostream.h>
#include <stdarg.h>


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



class TIpxNet: public CIpx16Net
{


public:


	BOOL DoListen()
	{
		// m_address.m_stationAddress.s_addr = INADDR_ANY;

		BOOL r = Listen(TRUE);

		// m_address.m_stationAddress.s_addr = INADDR_LOOPBACK;

		return r;
	}


	BOOL Poll();

	TIpxNet() : CIpx16Net(0, 0) 
	{

		cout << "Local Address: " << ipx16_ntoa(m_address.m_stationAddress) << endl;

//		m_address.m_stationAddress.s_addr = INADDR_LOOPBACK;



	}
	
	~TIpxNet()
	{
		Cleanup();
		Poll();
	}
					

};


BOOL TIpxNet::Poll()
{  
	BOOL rescan = TRUE;
	BOOL ret = FALSE;
    
    cout << "Polling ";
    char buf[32];
    cin >> buf;
    
	while(rescan)
	{
		rescan = FALSE;
		CEcbQIter iter(m_activeEcbs);
		CEcb*    ecb;   
	
		while(NULL != (ecb = iter()))
		{
			if (ecb->Completed())
			{
		 		OnEcbCompletion(ecb);
		 		rescan = TRUE;
		 		ret = TRUE;
		 		break; 
			}
		}
		
	}
	
	return ret;

}



int main()
{
   TIpxNet net1;
//   TIpxNet net2;

   net1.DoListen();
//  net2.DoListen();

   CNetLink* l1 = net1.MakeUnsafeLink();
//   CNetLink* l2 = net2.MakeUnsafeLink();
   
   CNetAddress* a1 = net1.MakeBroadcastAddress();
   CNetAddress* a2 = net1.MakeAddress(NULL);

   l1->SendTo(*a1, "12345", 6, VP_BROADCAST);
   
 //  net2.Poll();
   net1.Poll();
   
   char buf[256];

   DWORD s = l1->ReceiveFrom(buf, sizeof(buf), *a2);

   buf[s] = 0;
   
   cout << " got " << s << " bytes: " << buf << endl;
      

	    

   return (0);
}


