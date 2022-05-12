#ifndef __W16COMM_H__
#define __W16COMM_H__ 


class CW16CommPort:public CCommPort
{
public:
	virtual BOOL Open();
	virtual DWORD Read(LPVOID buf, DWORD bufSize);
	virtual DWORD Write(LPCVOID buf, DWORD bufSize);
	virtual BOOL Close();
	virtual BOOL Setup(DWORD baudRate);
	virtual BOOL HasCarrier();
	virtual BOOL HasDsr();
	virtual void Flush();
	virtual ~CW16CommPort();

	virtual UINT ClearEvent();

	CW16CommPort(LPCSTR portName=NULL, HWND wnd=NULL) :
	CCommPort(portName, wnd),  m_msr(NULL)
		{ 
		}



protected:
	LPBYTE    m_msr;

};



#endif 
