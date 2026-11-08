#include <winsock.h>
#include <stdio.h>


void test()
{
	WSADATA wsa;

    int err;
	err = WSAStartup(0x0101, &wsa);


	SOCKET s1 = socket(AF_INET, SOCK_DGRAM, 0);

	if (s1 == INVALID_SOCKET)
	{
		psockerr();
		return
	}


	SOCKADDR_IN  sin1;

	sin1.sin_family = AF_INET;
	sin1.sin_port = 0x5050;
	sin1.sin_addr = INADDR_ANY;


    if (bind(s1, (sockaddr*) &sin1, sizeof(sin1)))
			goto handleer;



	SOCKET s2 = socket(AF_INET, SOCK_DGRAM, 0);

	if (s1 == INVALID_SOCKET)
	{
		psockerr();
		return
	}


	SOCKADDR_IN  sin2;

	sin2.sin_family = AF_INET;
	sin2.sin_port = 0;
	sin2.sin_addr = INADDR_ANY;


    if (bind(s2, (sockaddr*) &sin2, sizeof(sin2)))
			goto handleer;


	SOCKADDR_IN  sin3;

	sin3.sin_family = AF_INET;
	sin3.sin_port = 0x5050;
	sin3.sin_addr = htonl(INADDR_LOOPBACK);
	
	int l = sendto(s2, "123", 4, 0, (sockaddr*) &sin3, sizeof(sin3));

	if (l == SOCKET_ERROR)
		goto handleer;

	char buf[256];



	SOCKADDR_IN  sin4;
	int flen = sizeof(sin4);

	l = recvfrom(s1, buf, sizeof(buf), 0, (sockaddr*) &sin4, &flen);
	if (l == SOCKET_ERROR)
		goto handleer;

	
	char* = inet_ntoa(sin4);

	print("got %d bytes from %s: <%s>\n", l, sender, buf);

	return;

handlerr:
	printf("Winsock error: %l\n", WSAGetLastError());
}


int main()
{
	test();
	return 0;
}
