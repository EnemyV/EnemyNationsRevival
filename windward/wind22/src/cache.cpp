//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------



#include "stdafx.h"
#include "_windwrd.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


extern "C"
{
    typedef WORD( WINAPI* YIELDTHREAD_FUNC ) ( );
    extern YIELDTHREAD_FUNC pfnYieldThread;
}


CDiskCache theDiskCache;


CCacheElem::CCacheElem( HANDLE hFil, int iOff, int iLen, void* pBuf, void ( *fnCall ) ( DWORD dwData ), DWORD dwData ) {

    m_hFil = hFil;
    m_iOff = iOff;
    m_iLen = iLen;
    m_pBuf = pBuf;
    m_fnCallBack = fnCall;
    m_dwData = dwData;
}

void CDiskCache::ctor() {

    m_posOn = m_lstRequests.end();
    m_bKillMe = FALSE;
    m_dwThrd = NULL;
    m_hWnd = NULL;
    m_pCceOn = NULL;
    InitializeCriticalSection( &m_cs );
}

void CDiskCache::Open( HWND hWnd ) {

    ctor();
    m_hWnd = hWnd;
}

CDiskCache::~CDiskCache() {

    if ( m_dwThrd == NULL ) {
        DeleteCriticalSection( &m_cs );
        return;
    }

    Close();

    if ( m_dwThrd != NULL )
        TerminateThread( ( (CWinThread*)m_dwThrd )->m_hThread, 1 );

    DeleteCriticalSection( &m_cs );
}

void CDiskCache::KillAllRequests() {

    EnterCriticalSection( &m_cs );

    // remove all elements
    auto it = m_lstRequests.begin();
    while ( it != m_lstRequests.end() ) {
        CCacheElem* pCceTst = *it;
        // if we are not reading it we can kill it
        if ( pCceTst != m_pCceOn ) {
            TRAP();
            delete pCceTst;
            it = m_lstRequests.erase( it );
        } else {
            TRAP();
            ++it;
        }
    }

    int iOff;
    if ( m_pCceOn != NULL )
        iOff = m_pCceOn->m_iOff;
    else
        iOff = -1;

    LeaveCriticalSection( &m_cs );

    if ( iOff != -1 )
        KillRequest( iOff );

    // grab any messages from the queue
    MSG msg;
    while ( ::PeekMessage( &msg, m_hWnd, MSG_CACHE, MSG_CACHE, PM_REMOVE ) )
        TRAP();
}

void CDiskCache::Close() {

    EnterCriticalSection( &m_cs );

    // remove all elements
    auto it = m_lstRequests.begin();
    while ( it != m_lstRequests.end() ) {
        CCacheElem* pCceTst = *it;
        // if we are not reading it we can kill it
        if ( pCceTst != m_pCceOn ) {
            TRAP();
            delete pCceTst;
            it = m_lstRequests.erase( it );
        } else {
            TRAP();
            ++it;
        }
    }

    m_bKillMe = TRUE;

    LeaveCriticalSection( &m_cs );
}

// does a synchronous read
void CDiskCache::SyncRequest( int hFil, int iOff, int iLen, void* pBuf ) {

    // sync read
    ::SetFilePointer( (HANDLE)hFil, iOff, NULL, FILE_BEGIN );
    DWORD dwRead;
    ::ReadFile( (HANDLE)hFil, pBuf, iLen, &dwRead, NULL );
    if ( dwRead != (DWORD)iLen )
        ThrowError( ERR_CACHE_READ );
}

void CDiskCache::AddRequest( int hFil, int iOff, int iLen, void* pBuf, void ( *fnCall ) ( DWORD dwData ), DWORD dwData ) {

    // if we aren't going yet we do a sync read
   //BUGBUG if (m_dwThrd == NULL)
    {
        ::SetFilePointer( (HANDLE)hFil, iOff, NULL, FILE_BEGIN );
        DWORD dwRead;
        ::ReadFile( (HANDLE)hFil, pBuf, iLen, &dwRead, NULL );
        if ( dwRead != (DWORD)iLen )
            ThrowError( ERR_CACHE_READ );

        // got it - tell the requestor
        ( *fnCall ) ( dwData );
        return;
    }

#ifdef BUGBUG
    CCacheElem* pCce = new CCacheElem( (HANDLE)hFil, iOff, iLen, pBuf, fnCall, dwData );

    EnterCriticalSection( &m_cs );

    // add to the list sorted by offset
    auto insertBefore = m_lstRequests.end();
    for ( auto it = m_lstRequests.begin(); it != m_lstRequests.end(); ++it ) {
        if ( (*it)->m_iOff >= iOff ) {
            insertBefore = it;
            break;
        }
    }
    m_lstRequests.insert( insertBefore, pCce );

    LeaveCriticalSection( &m_cs );

    // unblock if there were no requests
    if ( m_lstRequests.size() == 1 )
        myThreadPause( m_dwThrd, FALSE );
#endif
}

void CDiskCache::KillRequest( int iOff ) {

    EnterCriticalSection( &m_cs );

    // find it
    bool bFoundInProcess = false;
    for ( auto it = m_lstRequests.begin(); it != m_lstRequests.end(); ++it ) {
        CCacheElem* pCceTst = *it;
        if ( pCceTst->m_iOff == iOff ) {
            TRAP();
            // if we are not reading it we can kill it
            if ( pCceTst != m_pCceOn ) {
                TRAP();
                delete pCceTst;
                m_lstRequests.erase( it );
            } else {
                TRAP();
                bFoundInProcess = true;
            }
            break;
        }
    }

    LeaveCriticalSection( &m_cs );

    // if it wasn't the one in process we're done
    if ( !bFoundInProcess )
        return;

    // ok - we need to wait till it reads (when we return the buf may be deleted)
    while ( TRUE ) {
        EnterCriticalSection( &m_cs );

        if ( m_pCceOn == NULL ) {
            TRAP();
            LeaveCriticalSection( &m_cs );
            return;
        }
        if ( m_pCceOn->m_iOff != iOff ) {
            TRAP();
            LeaveCriticalSection( &m_cs );
            return;
        }

        LeaveCriticalSection( &m_cs );
        ::Sleep( 100 );
        myYieldThread();
    }
}

UINT CDiskCache::ThreadFunc( void* pData ) {

    ( (CDiskCache*)pData )->_ThreadFunc();

    return ( 0 );
}

void CDiskCache::_ThreadFunc() {

    //BUGBUG if ( ( m_dwThrd = myGetThreadHdl () ) == 0 )
    ThrowError( ERR_NO_THREAD_LIB );

#ifdef BUGBUG
    while ( !m_bKillMe ) {
        EnterCriticalSection( &m_cs );

        // get the next element
        if ( m_posOn == m_lstRequests.end() )
            m_posOn = m_lstRequests.begin();

        // if nothing we block
        if ( m_posOn == m_lstRequests.end() ) {
            LeaveCriticalSection( &m_cs );
            myThreadPause( m_dwThrd, TRUE );
            continue;
        }

        // we've got one
        m_pCceOn = *m_posOn;
        LeaveCriticalSection( &m_cs );

        // read it
        ::SetFilePointer( m_pCceOn->m_hFil, m_pCceOn->m_iOff, NULL, FILE_BEGIN );
        DWORD dwRead;
        ::ReadFile( m_pCceOn->m_hFil, m_pCceOn->m_pBuf, m_pCceOn->m_iLen, &dwRead, NULL );
        if ( dwRead != (DWORD)m_pCceOn->m_iLen )
            ThrowError( ERR_CACHE_READ );

        // got it - tell the requestor
        ::PostMessage( m_hWnd, MSG_CACHE, (DWORD)m_pCceOn, 0 );

        // Win32s
        if ( iWinType == W32s )
            if ( pfnYieldThread() == TM_QUIT )
                break;

        // take it out of the list
        EnterCriticalSection( &m_cs );
        m_posOn = m_lstRequests.erase( m_posOn );
        m_pCceOn = NULL;
        LeaveCriticalSection( &m_cs );
    }

    // all done - clean it out then kill the thread
    EnterCriticalSection( &m_cs );

    // remove all elements
    for ( auto* p : m_lstRequests )
        delete p;
    m_lstRequests.clear();

    LeaveCriticalSection( &m_cs );

    m_dwThrd = NULL;
    myThreadTerminate();
#endif
}

// call back in the context of the main thread
void CDiskCache::ProcessMessage( CCacheElem* pCce ) {

    ( pCce->m_fnCallBack ) ( pCce->m_dwData );
    delete pCce;
}
