#ifndef __NET_H__
#define __NET_H__

BOOL Net_Init(BOOL& tcpOk, BOOL& ipxOk, BOOL& netbiosOk);
CString Net_GetTcpHostName();
CString Net_GetTcpAddress();

class CNetSocket
{
public:
	virtual BOOL Ping(LPCSTR addr, UINT port, BOOL broadcast)=0;
	virtual BOOL WaitData(UINT& port, BOOL broadcast)=0;
	virtual BOOL ReadData(CString& data, CString& sender, UINT& port, LPARAM lParam) = 0;
	virtual BOOL Reply(LPCSTR addr, UINT port, LPCSTR data, UINT size)=0;

	CNetSocket() { }
	virtual ~CNetSocket();
};

CNetSocket* Net_MakeTcpSocket(HWND hWnd, UINT msgCode);
CNetSocket* Net_MakeIpxSocket(HWND hWnd, UINT msgCode);

CString Net_GetIpxAddress();

void Net_SetWindow(HWND hWnd);
void Net_EnumLanas(void (*callback)(UINT, LPVOID), LPVOID context);
BOOL Net_InitServerLana(UINT lana);
void Net_LookForServerOnLana(UINT  lana, void (*FoundLana)(UINT, UINT, LPVOID), LPVOID ctx);
void Net_ResetLanas();

#define WM_WINSOCK (WM_USER+1)
#define WM_NBNOTIFY (WM_USER+2)

#define DEFAULT_TCP_PORT 1707
#define DEFAULT_IPX_PORT 1707

#endif
