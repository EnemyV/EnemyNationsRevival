////////////////////////////////////////////////////////////////////////////
//
//  pathservice.cpp : the bounded movement-A* worker pool.
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "pathservice.h"

#include "stdafx.h"
#include "cpathmgr.h"
#include "Perf.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PathService thePathService;

/////////////////////////////////////////////////////////////////////////////
// the switch

static int s_iWorkerOn = -1;   // -1 = not resolved yet, 0 = off, 1 = on

static int ResolveOnOff( const char* pszEnv )
{
    if ( pszEnv == NULL || pszEnv[0] == '\0' )
        return ( 0 );

    char sz[8];
    int  i = 0;
    for ( ; i < (int)sizeof( sz ) - 1 && pszEnv[i] != '\0'; ++i )
        sz[i] = (char)tolower( (unsigned char)pszEnv[i] );
    sz[i] = '\0';

    if ( strcmp( sz, "1" ) == 0 || strcmp( sz, "true" ) == 0 || strcmp( sz, "on" ) == 0 ||
         strcmp( sz, "yes" ) == 0 )
        return ( 1 );
    return ( 0 );
}

BOOL PathService::Enabled( void )
{
    if ( s_iWorkerOn < 0 )
        s_iWorkerOn = ResolveOnOff( getenv( "EN_PATH_WORKER" ) );
    return ( s_iWorkerOn ? TRUE : FALSE );
}

int PathService::ConfiguredWorkers( void )
{
    const char* psz = getenv( "EN_PATH_WORKERS" );
    if ( psz == NULL || psz[0] == '\0' )
        return ( 1 );

    int n = atoi( psz );
    if ( n < 1 )
        n = 1;
    if ( n > (int)max_workers )
        n = (int)max_workers;
    return ( n );
}

/////////////////////////////////////////////////////////////////////////////
// PathService

PathService::PathService( )
    : m_bRunning( false ), m_iMapEX( 0 ), m_iMapEY( 0 ), m_nBusy( 0 ), m_nAlive( 0 ), m_bStopping( false ),
      m_bPaused( false ), m_uNextId( 0 )
{
}

PathService::~PathService( )
{
    Stop( );
}

void PathService::FreeResult( PathResult& r )
{
    delete[] r.path;
    r.path    = NULL;
    r.pathLen = 0;
}

BOOL PathService::Start( int iMapEX, int iMapEY, int nWorkers )
{
    if ( nWorkers < 1 )
        nWorkers = 1;
    if ( nWorkers > (int)max_workers )
        nWorkers = (int)max_workers;

    {
        std::lock_guard<std::mutex> lk( m_mtx );
        if ( m_bRunning.load( std::memory_order_relaxed ) || !m_aThreads.empty( ) )
            return ( FALSE );

        m_iMapEX    = iMapEX;
        m_iMapEY    = iMapEY;
        m_bStopping = false;
        m_bPaused   = false;
        m_nBusy     = 0;
        m_nAlive    = 0;
        m_qReq.clear( );
        m_qRes.clear( );
        m_bRunning.store( true, std::memory_order_relaxed );
    }

    for ( int i = 0; i < nWorkers; ++i )
    {
        try
        {
            m_aThreads.push_back( std::thread( &PathService::WorkerMain, this ) );
        }
        catch ( ... )
        {
            // A thread that could not be created must not leave the service claiming
            // workers it does not have; keep whatever started and report the failure.
            Perf::CounterInc( "pq.threadfail" );
            break;
        }
    }

    if ( m_aThreads.empty( ) )
    {
        m_bRunning.store( false, std::memory_order_relaxed );
        return ( FALSE );
    }

    // Say which arm ran, once, the way the shadow's "effective=" line does - a tester
    // must never have to infer it from counter presence. Only reached when
    // EN_PATH_WORKER asked for the service, so an untouched environment stays silent.
    // It is also the presence marker: `findstr PathService enations.exe` answers
    // whether step C is in a given binary, which a class name never could.
    fprintf( stderr, "[PathService] started: %d worker(s), map %dx%d\n", (int)m_aThreads.size( ), iMapEX, iMapEY );

    Perf::CounterInc( "pq.start" );
    return ( TRUE );
}

void PathService::Stop( void )
{
    {
        std::lock_guard<std::mutex> lk( m_mtx );
        if ( !m_bRunning.load( std::memory_order_relaxed ) && m_aThreads.empty( ) )
            return;
        m_bStopping = true;
        m_bRunning.store( false, std::memory_order_relaxed );
    }
    m_cvWork.notify_all( );
    m_cvIdle.notify_all( );

    // JOIN, every one of them, with no timeout. A worker is either waiting on m_cvWork
    // (woken above) or inside a search bounded by the CCell arena and the iHang budget,
    // so this returns in well under the tick the caller is standing in. Each worker has
    // already destroyed its own CPathMgr on its own thread by the time join returns -
    // that is what keeps the thread_local CMap node pool from being orphaned.
    for ( size_t i = 0; i < m_aThreads.size( ); ++i )
        if ( m_aThreads[i].joinable( ) )
            m_aThreads[i].join( );
    m_aThreads.clear( );

    std::lock_guard<std::mutex> lk( m_mtx );
    for ( size_t i = 0; i < m_qReq.size( ); ++i )
        Perf::CounterInc( "pq.dropped" );
    m_qReq.clear( );   // releases the snapshots these requests pinned
    while ( !m_qRes.empty( ) )
    {
        FreeResult( m_qRes.front( ) );
        m_qRes.pop_front( );
        Perf::CounterInc( "pq.dropped" );
    }
    m_bStopping = false;
    m_bPaused   = false;
    m_nBusy     = 0;
    m_nAlive    = 0;
}

uint64_t PathService::Submit( PathRequest const& req )
{
    if ( !m_bRunning.load( std::memory_order_relaxed ) )
        return ( 0 );

    uint64_t uId = 0;
    {
        std::lock_guard<std::mutex> lk( m_mtx );
        if ( m_bStopping || m_bPaused || m_nAlive == 0 )
            return ( 0 );
        if ( m_qReq.size( ) >= (size_t)max_queued )
        {
            Perf::CounterInc( "pq.qfull" );
            return ( 0 );
        }

        uId = ++m_uNextId;
        m_qReq.push_back( req );
        m_qReq.back( ).requestId = uId;
    }
    m_cvWork.notify_one( );
    Perf::CounterInc( "pq.submit" );
    return ( uId );
}

BOOL PathService::PopResult( PathResult& out )
{
    std::lock_guard<std::mutex> lk( m_mtx );
    if ( m_qRes.empty( ) )
        return ( FALSE );
    out = m_qRes.front( );
    m_qRes.pop_front( );
    return ( TRUE );
}

int PathService::QueueDepth( void ) const
{
    std::lock_guard<std::mutex> lk( m_mtx );
    return ( (int)m_qReq.size( ) + m_nBusy );
}

void PathService::Quiesce( void )
{
    std::unique_lock<std::mutex> lk( m_mtx );
    m_bPaused = true;
    // Submissions are closed, but queued work still runs: "quiesced" means nothing is
    // in flight, not that pending orders were thrown away. m_nAlive == 0 is the
    // no-worker escape - without it a queue left behind by a failed Init would hang
    // the save.
    m_cvIdle.wait( lk, [this] { return ( m_bStopping || m_nAlive == 0 || ( m_qReq.empty( ) && m_nBusy == 0 ) ); } );

    if ( !m_qReq.empty( ) )
    {
        for ( size_t i = 0; i < m_qReq.size( ); ++i )
            Perf::CounterInc( "pq.dropped" );
        m_qReq.clear( );
    }
}

void PathService::Resume( void )
{
    {
        std::lock_guard<std::mutex> lk( m_mtx );
        m_bPaused = false;
    }
    m_cvWork.notify_all( );
}

void PathService::WorkerMain( void )
{
#ifdef _WIN32
    // Matches the AI actors. The POSIX shim takes a HANDLE this thread does not have;
    // the bounded worker count is the protection there.
    ::SetThreadPriority( ::GetCurrentThread( ), THREAD_PRIORITY_BELOW_NORMAL );
#endif

    int iMapEX = 0;
    int iMapEY = 0;
    {
        std::lock_guard<std::mutex> lk( m_mtx );
        iMapEX = m_iMapEX;
        iMapEY = m_iMapEY;
    }

    // Constructed, Init'ed, used and destroyed on THIS thread, all of it inside this
    // function so the destructor cannot run anywhere else. The CMap node pool
    // EnPoolAllocator recycles through is thread_local and freed at thread exit, so an
    // arena torn down on another thread would orphan its pooled nodes for the process
    // lifetime - the leak the per-AI CPathMap precedent shipped with.
    CPathMgr mgr;

    // Every mpath.* counter inside a search is renamed to mpath.shadow.in.* on a marked
    // instance. A worker must not move the production exit mix, which is a main-thread
    // measurement; the worker's own accounting is pq.*.
    mgr.MarkShadow( );

    BOOL bReady = FALSE;
    try
    {
        bReady = mgr.Init( iMapEX, iMapEY );
    }
    catch ( ... )
    {
        bReady = FALSE;
    }

    if ( !bReady )
    {
        Perf::CounterInc( "pq.initfail" );
        m_cvIdle.notify_all( );
        return;
    }

    {
        std::lock_guard<std::mutex> lk( m_mtx );
        ++m_nAlive;
    }

    for ( ;; )
    {
        PathRequest req;
        {
            std::unique_lock<std::mutex> lk( m_mtx );
            m_cvWork.wait( lk, [this] { return ( m_bStopping || !m_qReq.empty( ) ); } );
            if ( m_bStopping )
                break;
            req = m_qReq.front( );
            m_qReq.pop_front( );
            ++m_nBusy;
        }

        CPathMgr::SNAPSEARCH out;
        const uint64_t       tWork = Perf::NowIfEnabled( );
        if ( req.world )
            mgr.SearchSnapshot( *req.world, req.from, req.to, req.iVehType, req.bVehBlock, req.bDirectPath, out );
        Perf::CounterAddElapsedUs( "pq.worker.us", tWork );
        Perf::CounterInc( "pq.worker.calls" );

        PathResult res;
        res.requestId       = req.requestId;
        res.gameGeneration  = req.gameGeneration;
        res.vehicleId       = req.vehicleId;
        res.orderGeneration = req.orderGeneration;
        res.worldEpoch      = req.world ? req.world->Epoch( ) : 0;
        res.success         = ( out.phexPath != NULL ) ? TRUE : FALSE;
        res.exitClass       = out.iClass;
        res.bCapArena       = out.bCapArena;
        res.bCapIter        = out.bCapIter;
        res.path            = out.phexPath;
        res.pathLen         = out.iPathLen;

        // The generation is NOT checked here: EnNavGameGeneration() is a live global and
        // this thread reads none. The main thread rejects a straggler at the drain.
        bool bDrop = false;
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            --m_nBusy;
            if ( m_bStopping )
                bDrop = true;
            else
                m_qRes.push_back( res );

            if ( m_qReq.empty( ) && m_nBusy == 0 )
                m_cvIdle.notify_all( );
        }

        if ( bDrop )
        {
            FreeResult( res );
            Perf::CounterInc( "pq.dropped" );
            break;
        }

        Perf::CounterAddElapsedUs( "pq.age.us", req.tSubmit );
        Perf::CounterInc( "pq.done" );
    }

    {
        std::lock_guard<std::mutex> lk( m_mtx );
        --m_nAlive;
    }
    m_cvIdle.notify_all( );
}

/////////////////////////////////////////////////////////////////////////////
// the lifecycle seams the game calls

void EnPathWorkerStart( int iMapEX, int iMapEY )
{
    if ( !PathService::Enabled( ) )
        return;
    thePathService.Start( iMapEX, iMapEY, PathService::ConfiguredWorkers( ) );
}

void EnPathWorkerStop( void )
{
    thePathService.Stop( );
}

void EnPathWorkerQuiesce( void )
{
    if ( !thePathService.IsRunning( ) )
        return;
    thePathService.Quiesce( );
}

void EnPathWorkerResume( void )
{
    if ( !thePathService.IsRunning( ) )
        return;
    thePathService.Resume( );
}

// end of pathservice.cpp
