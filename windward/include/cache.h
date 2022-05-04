#ifndef __CACHE_H__
#define __CACHE_H__


const int		MSG_CACHE = WM_USER + 583;


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

// this code is designed to handle all async reads from the data file. We order
// the reads by file offset (assuming it is all in 1 .dat file) and go from lowest
// to highest, then back to lowest again. There is no jumping around, prioritization
// of requests, etc. We do lowest to highest only (and not elevator back) because a
// CD doesn't like to go backwards.

typedef void (* CACHECALLBACK) (DWORD dwData);

// linked list element of cache requests
class CCacheElem
{
public:
		CCacheElem ();
		CCacheElem (HANDLE hFil, int iOff, int iLen, void * pBuf, void (* fnCall) (DWORD dwData), DWORD dwData);

		HANDLE	m_hFil;									// file handle
		int			m_iOff;									// file offset
		int			m_iLen;									// num bytes to read
		void *	m_pBuf;									// memory to read data in to
		CACHECALLBACK		m_fnCallBack;		// call when read
		DWORD		m_dwData;								// param to pass back in the callback
};

class CDiskCache
{
public:
		CDiskCache () { ctor (); }
		~CDiskCache ();

		void Open (HWND hWnd);
		void Close ();
		void	KillAllRequests ();

		void	ProcessMessage ( CCacheElem * pCce );

		void AddRequest (int hFil, int iOff, int iLen, void * pBuf, void (* fnCall) (DWORD dwData), DWORD dwData);
		void SyncRequest (int hFil, int iOff, int iLen, void * pBuf);
		void KillRequest (int iOff);

		static UINT ThreadFunc (void * pData);

		// public for thread.cpp only
		DWORD							m_dwThrd;						// ID of this thread

private:
		void ctor ();
		void _ThreadFunc ();

		POSITION					m_posOn;						// element presently on
		CCriticalSection	m_cs;								// for access to the linked list
		BOOL							m_bKillMe;					// kill the thread
		CCacheElem *			m_pCceOn;						// element we are reading right now
		CList <CCacheElem *, CCacheElem *> m_lstRequests;		// stuff to read

		HWND							m_hWnd;							// window that handles messages
};



extern CDiskCache theDiskCache;


#endif
