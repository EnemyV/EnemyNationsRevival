#include "stdafx.h"
#include "vputil.h"
#include "vdmplay.h"

class CVpMemPool;

CVpMemPool* vpMemPool;

BOOL gBreakOnAssert;
BOOL gLocalIni;
BOOL gUseLogfile;


void vpMakeIniFile(LPSTR fName)
{
	*fName = 0;

	if (gLocalIni)
		lstrcpy(fName, ".\\");

	strcat(fName, VDMPLAY_INIFILE);
}

// --- Configuration params I/O -------
void vpFetchString(LPCSTR section, 
				   LPCSTR key, 
				   LPCSTR defVal, 
				   LPSTR pStr, 
				   int iLen)
{
	char fName[256];

	vpMakeIniFile(fName);

	*pStr = 0;
	GetPrivateProfileString(
			section, 
			key, 
			defVal,
			pStr, iLen,
			fName);
}

UINT vpFetchInt(LPCSTR section, LPCSTR key, UINT defVal)
{

	char fName[256];

	vpMakeIniFile(fName);

	return GetPrivateProfileInt(
			section, 
			key, 
			defVal,
		fName);
}

void vpStoreInt(LPCSTR section, LPCSTR key, UINT val)
{
	char fName[256];

	vpMakeIniFile(fName);

	char buf[30];

	itoa (val, buf, 10);

	WritePrivateProfileString(
			section, 
			key, 
			buf,
			fName);
}

void vpStoreString(LPCSTR section, LPCSTR key, LPCSTR val)
{
	char fName[256];

	vpMakeIniFile(fName);

	WritePrivateProfileString(
			section, 
			key, 
			val,
			fName);
}


DWORD vpReentrancyCounter = 0;
DWORD vpWaitAbortFlag = 0;

void vpDoAbortWait()
{
	if (vpReentrancyCounter)
		vpWaitAbortFlag++;
}


BOOL  vpWaitLoop(BOOL (*waitDone)(LPVOID), LPVOID testContext, LPDWORD reentrancyCounter)
{
	if (!reentrancyCounter)
		reentrancyCounter = &vpReentrancyCounter;

	*reentrancyCounter += 1;

	 while(!vpWaitAbortFlag)
	 {
		 if (!waitDone(testContext))
		 {
			MSG msg;

			GetMessage(&msg, NULL, 0, 0);

			if (msg.message == WM_QUIT)
			{
				PostQuitMessage(0);
				*reentrancyCounter -= 1;
				 return FALSE;

			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				continue;
			}
		 }
		*reentrancyCounter -= 1;
		 return TRUE;
	 }

	*reentrancyCounter -= 1;
	vpWaitAbortFlag--;
	return FALSE;
}


int vpFindFreePort(LPSTR pName)
{
	int iPort;
#ifdef WIN32
	for (iPort=1; iPort<255; iPort++)
	{
		char sBuf[10];
		strcpy (sBuf, "COM");
		itoa (iPort, &sBuf[3], 10);
		HANDLE dev = CreateFile (sBuf, GENERIC_READ | GENERIC_WRITE, 0, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
		if (dev != INVALID_HANDLE_VALUE)
		{
				CloseHandle (dev);
				strcpy(pName, sBuf);
				return iPort;
		}
	}
#else
	 for (iPort=1; iPort<255; iPort++)
	 {
			char sBuf[10];
			strcpy (sBuf, "COM");
			itoa (iPort, &sBuf[3], 10);
			int iHdl = OpenComm (sBuf, 1024, 1024);
			if (iHdl >= 0)
			{
				CloseComm (iHdl);
				strcpy(pName, sBuf);
				return iPort;
			}
	 }
	
	
#endif
	  return 0;

}


// --- debugging  -----

const char FAR vpEnterFmt[] = "Enter %s";
const char FAR vpLeaveFmt[] = "Leaving %s";
const char FAR vpLeaveBoolFmt[] = "Leaving %s code %s";
const char FAR vpTrue[] = "TRUE";
const char FAR vpFalse[] = "FALSE";

// --- memory manager -----------
#define MARKER1 0xFEFEFEF1l
#define MARKER2 0xFEFEFEF2l
#define MARKER3 0xFEFEFEF3l

void vpMemError(LPCSTR fmt, LPVOID p=NULL, size_t s=0)
{

	VPTRACE((fmt, p, s));
}

class CVpMemPool
{

public:

	virtual LPVOID		Alloc(size_t s);
	virtual void		Free(LPVOID p, size_t s);

	CVpMemPool(size_t maxChunkSize, size_t granularity);
	virtual ~CVpMemPool();
	virtual void Reset();
	BOOL Ok() const 
	{ 
		return !(!m_map || !m_counts || !m_hits || !m_requests);
	}
protected:
	 struct mhdr
	 {
		DWORD	marker1;
		DWORD	size;
		struct mhdr* next;
		DWORD	marker2;
	 };
	 struct mtrailer
	 {
		 DWORD marker3;
	 };

	 void InitChunk(mhdr* p, size_t s);
	 BOOL FreeChunk(mhdr* p, size_t s);
	 BOOL CheckChunk(mhdr* p, size_t s=0);
	 BOOL CheckPool();

	 size_t  m_maxChunkSize;
	 size_t  m_mapSize;
	 size_t  m_granularity;

	 LPSTR* m_map;
	 size_t* m_counts;
	 size_t* m_hits;
	 size_t* m_requests;
	 size_t  m_allocsCount;
	 size_t  m_freesCount;
	 mhdr*	 m_usedMem;
	 CMutex m_mx;
};



CVpMemPool::CVpMemPool(size_t maxChunkSize, size_t granularity) :
m_maxChunkSize(maxChunkSize), m_granularity(granularity),
m_allocsCount(0), m_freesCount(0), m_usedMem(NULL)
{
	CMxLock lk(m_mx);

	m_mapSize = maxChunkSize/granularity;
	ASSERT(m_mapSize*granularity == maxChunkSize);


	m_map = new LPSTR[m_mapSize];
	m_counts = new size_t[m_mapSize];
	m_hits = new size_t[m_mapSize];
	m_requests = new size_t[m_mapSize];

	if (!Ok())
	{
		delete [] m_requests;
		delete [] m_hits;
		delete [] m_counts;
		delete [] m_map;
		return;
	}

	memset(m_map, 0, sizeof(m_map[0])*m_mapSize);
	memset(m_counts, 0, sizeof(m_counts[0])*m_mapSize);
	memset(m_hits, 0, sizeof(m_hits[0])*m_mapSize);
	memset(m_requests, 0, sizeof(m_requests[0])*m_mapSize);

	if (!m_counts)
	{
		return;
	}

}

BOOL CVpMemPool::CheckChunk(mhdr* p, size_t s)
{
#ifndef NDEBUG
	if (p->marker2 != MARKER2)
		return FALSE;
	if (p->marker1 != MARKER1)
		return FALSE;

	if (s && p->size != s)
		return FALSE;

	mtrailer* t = (mtrailer*) (p->size + (LPSTR)(p+1));
	if (t->marker3 != MARKER3)
		return FALSE;
#endif
	return TRUE;
}

void CVpMemPool::InitChunk(mhdr* p, size_t s)
{
#ifndef NDEBUG
	p->marker1 = MARKER1;
	p->marker2 = MARKER2;
	p->size = s;
	p->next = m_usedMem;
	m_usedMem = p;
	
	mtrailer* t = (mtrailer*) (s + (LPSTR)(p+1));
	t->marker3 = MARKER3;
#endif
}

BOOL CVpMemPool::FreeChunk(mhdr* p, size_t s)
{
#ifndef NDEBUG
	if (!CheckChunk(p, s))
	{
		vpMemError("Freeing damaged chunk %p size %u", p, s);
		return FALSE;
	}

	if (CheckPool())
	{
		if (m_usedMem == p)
			m_usedMem = p->next;
		else
		{
			mhdr* pp;

			for(pp = m_usedMem; pp; pp = pp->next)
			{
				if (pp->next == p)
				{
					break;
				}
			}

			if (!pp)
			{
				vpMemError("Freeing unknown chunk %p size %u", p, s);
				return FALSE;
			}
			pp->next = p->next;
		}
	}
	else
	{
		vpMemError("Memory pool is damaged");
		return FALSE;
	}
#endif
	return TRUE;
}


BOOL CVpMemPool::CheckPool()
{
#ifndef NDEBUG
	mhdr* p;

	for(p = m_usedMem; p != NULL; p = p->next)
	{
		if (!CheckChunk(p))
		{
			vpMemError("Invalid memory chunk at %p", p);
			return FALSE;
		}
	}
#endif
	return TRUE;
}

void CVpMemPool::Reset()
{
	CMxLock lk(m_mx);
	size_t i;

	if (!m_map)
		return;

	for(i = 0; i < m_mapSize; i++)
	{
		LPSTR p = m_map[i];
		m_map[i] = NULL;
		while(p)
		{
			LPSTR  next = *(LPSTR*) p;
#ifndef NDEBUG
			delete [] (p-sizeof(mhdr));
#else
			delete [] p;
#endif
			p = next;
		}
	}
}

CVpMemPool::~CVpMemPool()
{
	Reset();
	CMxLock lk(m_mx);

	delete [] m_requests;
	delete [] m_hits;
	delete [] m_counts;
	delete [] m_map;

}


LPVOID CVpMemPool::Alloc(size_t s)
{
	ASSERT(m_map);
	CMxLock lk(m_mx);

#ifndef NDEBUG
	CheckPool();
#endif

	if (s < sizeof(LPSTR))
		s = sizeof(LPSTR);

	m_allocsCount++;
	if (s > m_maxChunkSize)
	{
#ifndef NDEBUG
		LPSTR p = new char[s + sizeof(mhdr)	+ 
			sizeof(mtrailer)];
		if (!p)
			return NULL;

		InitChunk((mhdr*)p, s);

		return p + sizeof(mhdr);
#else
		return new char[s];
#endif
	}
	size_t ix = (s + m_granularity - 1)/m_granularity - 1;

	m_requests[ix]++;

	LPVOID p = m_map[ix];
	if (!p)
	{
#ifndef NDEBUG
		size_t ss = (ix+1)*m_granularity;
		
		p = new char[ss + sizeof(mhdr)	+ 
			sizeof(mtrailer)];
		if (!p)
			return NULL;

		InitChunk((mhdr*)p, s);

		return (LPSTR)p + sizeof(mhdr);
#else
		return new char [(ix+1)*m_granularity];
#endif
	}
	else
	{
		InitChunk((mhdr*)p - 1, s);
	}

	m_hits[ix]++;
	m_counts[ix]--;
	m_map[ix] = *(LPSTR*) p;
	return p;
}

void CVpMemPool::Free(LPVOID p, size_t s)
{
	ASSERT(m_map);
	CMxLock lk(m_mx);

	m_freesCount++;

	if (s < sizeof(LPSTR))
		s = sizeof(LPSTR);

	if (!p)
		return;

#ifndef NDEBUG
	if (!FreeChunk((mhdr*)p - 1, s))
	{
		return;
	}



#endif

	if (s > m_maxChunkSize)
	{
#ifndef NDEBUG
		delete [] ((LPSTR)p - sizeof(mhdr));
#else
		delete [] (LPSTR) p;
#endif
		return;
	}

	size_t ix = (s + m_granularity - 1)/m_granularity - 1;

	m_counts[ix]++;

	LPSTR mp = m_map[ix];

	
	m_map[ix] = (LPSTR) p;
	*(LPSTR*)p = mp;
}


LPVOID vpAllocMem(size_t s)
{
	if (vpMemPool) 
		return vpMemPool->Alloc(s);

	return new char[s];
}

void vpFreeMem(LPVOID p, size_t s)
{
	if (vpMemPool)
		vpMemPool->Free(p, s);
	else
		delete [] (LPSTR) p;
}

void vpMemPoolInit()
{
#ifdef USE_VPMEM
	ASSERT(!vpMemPool);
	vpMemPool = new CVpMemPool(1024, 8);
	if (vpMemPool && !vpMemPool->Ok())
	{
		delete vpMemPool;
		vpMemPool = NULL;
	}


#endif
}

void vpMemPoolCleanup()
{
#ifdef USE_VPMEM
	delete vpMemPool;
	vpMemPool = NULL;
#endif
}
