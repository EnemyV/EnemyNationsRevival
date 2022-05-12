#ifndef __VPUTIL_H__
#define __VPUTIL_H__

// For multithreading, will be implemented later 
class CMutex
{
#ifndef WIN32
public:
   void Lock() { }
   void Unlock() { }
#else
public:
	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock()  { LeaveCriticalSection(&m_cs); }
	CMutex() { InitializeCriticalSection(&m_cs); }
	~CMutex() { DeleteCriticalSection(&m_cs); }
private:
	CRITICAL_SECTION m_cs;
#endif
};

// Lock/Unlock the mutex
class CMxLock  
{
public:
	CMxLock(CMutex& mx) : m_mutex(mx) { mx.Lock(); }
	~CMxLock() { m_mutex.Unlock(); }

protected:
   CMutex& m_mutex;
	
};

class CRef
{
public:	
    unsigned       Unref() 
	{ 
		unsigned ret = --m_refCnt;
		if (!ret)
			delete this;
		return ret;
	}
	
	virtual ~CRef() { }
	unsigned       Ref() { return ++m_refCnt; }
	unsigned       Count() const { return m_refCnt; }
	CRef() : m_refCnt(1) { }

    unsigned m_refCnt;
	
};


class CActivity
{
public:
    CActivity() { }
	virtual ~CActivity() { }
	virtual void Complete() = 0;
};


class VPTracer
{
  LPCSTR m_file;
  int    m_line;
  LPCSTR m_class;
  LPCSTR m_method;

public:
   VPTracer(LPCSTR file, int line, LPCSTR cls, LPCSTR method) :
	m_file(file), m_line(line), m_class(cls), m_method(method)
   {  
   }

   DoTrace(LPCSTR format, ...);
   

};		   

extern void (*_vpassert)(LPCSTR text, LPCSTR file, int line);

#define VPASSERT(x) if (!(x))  _vpassert(#x, __FILE__, __LINE__)

#if !defined(NDEBUG) && !defined(NOVPTRACE)
#define VPTRACE3(cls, mthd, args) \
  { VPTracer t(__FILE__, __LINE__, #cls, #mthd);  t.DoTrace args; }
#define VPTRACE(args) \
  { VPTracer t(__FILE__, __LINE__, NULL, NULL);  t.DoTrace args; }

#define VPPROCNAME(x) static const char __vpProc[] = #x
#else
# define VPTRACE3(cls, mthd, args)
# define VPTRACE(args)
#define VPPROCNAME(x)
#endif


extern const char FAR vpEnterFmt[];
extern const char FAR vpLeaveFmt[];
extern const char FAR vpLeaveBoolFmt[];
extern const char FAR vpTrue[];
extern const char FAR vpFalse[];

#define VPENTER(x) VPPROCNAME(x); \
	VPTRACE((vpEnterFmt,  __vpProc))

#define VPEXIT()  VPTRACE((vpLeaveFmt, __vpProc))
#define VPEXITBOOL(v) \
 VPTRACE((vpLeaveBoolFmt, __vpProc, (v) ?  vpTrue : vpFalse))

#define VPLEAVEBOOL(x,v) VPEXITBOOL(v)

#define VPLEAVE(x) VPEXIT()



#ifndef ASSERT
#define ASSERT(x)  VPASSERT(x)
#endif

void vpFetchString (LPCSTR section, LPCSTR key, LPCSTR defVal, LPSTR pStr, int iLen);
UINT vpFetchInt(LPCSTR section, LPCSTR key, UINT defVal);
void vpStoreInt(LPCSTR section, LPCSTR key, UINT val);
void vpStoreString(LPCSTR section, LPCSTR key, LPCSTR val);
void vpMakeIniFile(LPSTR fName);

int vpFindFreePort(LPSTR pName);

LPVOID vpAllocMem(size_t s);
void vpFreeMem(LPVOID p, size_t s);
void vpMemPoolInit();
void vpMemPoolCleanup();

extern DWORD vpReentrancyCounter;
extern DWORD vpWaitAbortFlag;

BOOL  vpWaitLoop(BOOL (*waitDone)(LPVOID), LPVOID testContext, LPDWORD reentancyCounter=NULL);
void  vpDoAbortWait();

void vpUnloadIpxDll();
BOOL vpLoadIpxDll();

void vpUnloadWinsockDll();
BOOL vpLoadWinsockDll();

BOOL vpLoadDp();
void vpUnloadDp();
BOOL IsDpSupported();

#endif 
