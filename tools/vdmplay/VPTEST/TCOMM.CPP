#include "stdafx.h"
#include <iostream.h>
#include "commnet.h"
#include "w32comm.h"


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


LRESULT APIENTRY WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CCommNet*  net = (CCommNet*) GetWindowLong(hWnd, GWL_USERDATA); 
	
	if (!net)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);


    if (uMsg == WM_TIMER)
	{
		net->OnTimer();
		return TRUE;
	}
	
	LRESULT ret = FALSE;
	ret = net->OnMessage(hWnd, uMsg, wParam, lParam);

	if (!ret)
		ret = DefWindowProc( hWnd, uMsg, wParam, lParam);

	return ret;

}					


HWND InitWindowsStuff(CCommNet* net)
{
	static BOOL classReady = FALSE;
	WNDCLASS testClass = { 0, WinProc, 0, sizeof(void*), 
										NULL, NULL, NULL, NULL,
										NULL, "TestClass" };


	
	testClass.hInstance = NULL;
    if (!classReady)
	{
		ATOM a = RegisterClass(&testClass);

		if (!a)
			return FALSE;

		classReady = TRUE;
	}


    HWND window = CreateWindow(
        "TestClass",                   /* See RegisterClass() call.          */
        "Hidden Test Window",        /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        NULL,		                    /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );


	if (!window)
	{
		return FALSE;
	}


    SetWindowLong(window, GWL_USERDATA, (DWORD) net);

	UINT timer = SetTimer(window, 0, 1000L, NULL);

	if (!timer)
	{
		DestroyWindow(window);
		window = NULL;
		return FALSE;
	}	 


	return window;
}



void MsgLoop()
{
	MSG msg;


	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


static CNetLink* gLink;


void doTest(const char* portName, char* mode)
{

	CW32CommPort port(portName);
   	CCommNet net(&port);

   	port.m_wnd = InitWindowsStuff(&net);

   	char buf[256];



	strcpy(port.m_deviceName,portName);

	
	if (mode[0] == 'S' || 's' == mode[0])
	{
		net.Listen(TRUE, FALSE);
	}
	else
	{
		net.Listen(FALSE, FALSE);
		CNetAddress*  addr = net.MakeAddress();
		CNetLink* lnk = net.MakeSafeLink(addr);

	}


	MsgLoop();



#if 0

	cout << "Open ...";
	if (!port.Open())
	   goto err;


   cout << "Ok" << endl;

   cout << "Setup ...";

   if (!port.Setup(19200))
   	   goto err;


   cout << "Ok" << endl;


   cout << "StartEventThread ... ";

   if (!port.StartEventThread())
   		goto err;


   cout << "Ok" << endl;
   	


   while(1)
   {
   	   DWORD len;
   	   char* p;

   	   len = port.Read(buf, sizeof(buf));
   	   if (len == (DWORD) -1)
   	   		goto err;
   	 
   		buf[len] = 0;
   	 	cout << buf;
   	 	
   	 	cin >> buf;
   	 	
		if (!strncmp(buf, "QUIT", 4))
			break;

   	 	p = strchr(buf, '\n');
		if (p != NULL)
			*p = '\r';
		else
		    strcat(buf, "\r");


		port.Write(buf, strlen(buf));



  }
  
  port.Close();
#endif

  return;
 
err:
	cout << " Error " << port.GetError() << endl;
}

  	

		



int main(int argc, char* argv[])
{

	doTest(argv[1], argv[2]);


	return 0;
}
							  
