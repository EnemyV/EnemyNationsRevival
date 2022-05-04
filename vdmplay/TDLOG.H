#ifndef __TDLOG_H__
#define __TDLOG_H__


class CTDLogger
{
	
public:
	CTDLogger()  { m_time = GetCurrentTime(); }
	virtual ~CTDLogger() { }

	virtual void Write(LPCSTR) = 0;

	virtual void Log(LPCSTR msg); 

	virtual void SetError(DWORD e1, DWORD e2=0, DWORD e3=0);

	virtual void SetFatalError(DWORD e1, DWORD e2, DWORD e3 = 0);

protected:

	char  m_buf[256];
	DWORD m_time;
	

};



#endif
