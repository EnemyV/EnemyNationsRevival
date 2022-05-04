#include "stdafx.h"
#include <winsock.h>
#include <wsipx.h>

#include "datagram.h"
#include "wsipxnet.h"
#include <iostream.h>


void assertion(LPCSTR text, LPCSTR file, int line)
{
	cout << "Assertion: " << text << " in file " << file << " at line " << line << endl;
	abort();
}

void (*_vpassert)(LPCSTR text, LPCSTR file, int line) = assertion;



class TIpxNet: public CWSIpxNet
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

		BOOL r = Listen(TRUE);

		// m_address.m_stationAddress.s_addr = INADDR_LOOPBACK;

		return r;
	}




	TIpxNet() : CWSIpxNet(0, 0, 0) 
	{

		cout << "Local Address: " << ipx_ntoa(m_address.m_stationAddress) << endl;

//		m_address.m_stationAddress.s_addr = INADDR_LOOPBACK;



	}				

};



int main()
{
   TIpxNet net1;
   TIpxNet net2;

   net1.DoListen();
   net2.DoListen();

   CNetLink* l1 = net1.MakeUnsafeLink();
   CNetLink* l2 = net2.MakeUnsafeLink();
   
   CNetAddress* a1 = net1.MakeAddress(NULL);
   CNetAddress* a2 = net2.MakeAddress(NULL);

   l2->SendTo(*a1, "12345", 6, 0);
   
   char buf[256];

   DWORD s = l1->ReceiveFrom(buf, sizeof(buf), *a2);

   buf[s] = 0;
   
   cout << " got " << s << " bytes: " << buf << endl;
      

	    

   return (0);
}


