#ifndef __PATHSERVICE_H__
#define __PATHSERVICE_H__

////////////////////////////////////////////////////////////////////////////
//
//  pathservice.h : the bounded worker pool that runs movement A* searches off
//                  the main thread.
//
//  Ladder step C. A worker searches ONLY through CEnSnapNavView over the
//  immutable PathWorld its request carries, with pVehicle == NULL, so it
//  dereferences no live object and takes no global lock. Its answer is not
//  used by the game in this slice: the synchronous search still answers every
//  caller, and the result is compared and freed on the main thread.
//
//  Off unless EN_PATH_WORKER names a value. With the variable absent no thread
//  is created, Submit() refuses, and nothing on the movement path changes.
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pathworld.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class CPathMgr;

// Plan section 2.3. No CVehicle*, no CAIMgr*, no callbacks, no polymorphism:
// everything a worker needs is a value, and the world is a shared_ptr to an
// immutable snapshot so the worker's read set cannot be freed under it.
struct PathRequest
{
    uint64_t  requestId;        // monotonic, assigned by Submit on the main thread
    uint32_t  gameGeneration;   // EnNavGameGeneration() at submit
    uint32_t  vehicleId;
    uint32_t  orderGeneration;  // see EN_PATH_ORDERGEN below
    CHexCoord from;
    CHexCoord to;
    int       iVehType;         // theTransports index; USE_HEADINGS == 0 so no vehicle is needed
    BOOL      bVehBlock;
    BOOL      bDirectPath;
    uint64_t  tSubmit;          // Perf::NowIfEnabled() stamp, for pq.age.us

    std::shared_ptr<const PathWorld> world;

    PathRequest( )
        : requestId( 0 ), gameGeneration( 0 ), vehicleId( 0 ), orderGeneration( 0 ), from( 0, 0 ), to( 0, 0 ),
          iVehType( 0 ), bVehBlock( FALSE ), bDirectPath( FALSE ), tSubmit( 0 )
    {
    }
};

struct PathResult
{
    uint64_t   requestId;
    uint32_t   gameGeneration;
    uint32_t   vehicleId;
    uint32_t   orderGeneration;
    uint64_t   worldEpoch;
    BOOL       success;
    int        exitClass;   // CPathMgr::PROBE_OUTCOME; 0 with EN_PATH_PROBES compiled out
    BOOL       bCapArena;
    BOOL       bCapIter;
    CHexCoord* path;        // worker-new[]ed, ownership transfers to whoever pops the result
    int        pathLen;

    PathResult( )
        : requestId( 0 ), gameGeneration( 0 ), vehicleId( 0 ), orderGeneration( 0 ), worldEpoch( 0 ),
          success( FALSE ), exitClass( 0 ), bCapArena( FALSE ), bCapIter( FALSE ), path( NULL ), pathLen( 0 )
    {
    }
};

class PathService
{
  public:
    enum
    {
        max_workers = 4,      // EN_PATH_WORKERS is clamped to this
        max_queued  = 4096    // Submit refuses beyond this rather than blocking the main thread
    };

    PathService( );
    ~PathService( );

    // A mutex, a condition variable and owned threads: copying one would alias all three.
    PathService( PathService const& )            = delete;
    PathService& operator=( PathService const& ) = delete;

    // Starts nWorkers threads, each with its own CPathMgr sized iMapEX x iMapEY.
    // Already running -> FALSE and nothing changes.
    BOOL Start( int iMapEX, int iMapEY, int nWorkers );

    // Signal, join EVERY worker, then drop queued requests and results (freeing their
    // routes and releasing their snapshots). No timeout, no zombie-thread machinery:
    // a search is bounded by the arena and the iteration budget, so a worker always
    // reaches the next wait. Safe to call when not running.
    void Stop( void );

    BOOL IsRunning( void ) const { return ( m_bRunning.load( std::memory_order_relaxed ) ? TRUE : FALSE ); }

    // Main thread. Returns the assigned requestId, or 0 if the request was refused
    // (not running, quiesced, or the queue is at max_queued).
    uint64_t Submit( PathRequest const& req );

    // Main thread. TRUE when a result was moved into out; the caller now owns out.path.
    BOOL PopResult( PathResult& out );

    int  QueueDepth( void ) const;

    // Save seam (plan section 2.6). Quiesce refuses new submissions and waits until the
    // queue is empty and every worker is idle, so nothing is in flight across
    // serialization. Resume re-opens submissions.
    void Quiesce( void );
    void Resume( void );

    static void FreeResult( PathResult& r );

    // EN_PATH_WORKER must carry a VALUE: 1 / true / on / yes (any case) enable the
    // service. Absent, empty, 0, false, off, no - or anything unrecognised - leave it
    // off, so the obvious way to switch it off really does. Same rule as EN_PATH_SHADOW,
    // where treating any non-empty string as ON made "=0" turn the feature on.
    static BOOL Enabled( void );
    static int  ConfiguredWorkers( void );

    // Step D. EN_PATH_ASYNC, same value rule, AND the pool must be on: with either
    // switch absent every mover searches synchronously, as it always has.
    static BOOL AsyncEnabled( void );
    // EN_PATH_ASYNC_VERIFY: at install, also run the synchronous live search and
    // compare. Needs EN_PATH_PROBES compiled in; a no-op without it.
    static BOOL AsyncVerifyEnabled( void );

  private:
    void WorkerMain( void );

    mutable std::mutex       m_mtx;
    std::condition_variable  m_cvWork;   // a request arrived, or we are stopping
    std::condition_variable  m_cvIdle;   // the queue drained and every worker went idle
    std::deque<PathRequest>  m_qReq;
    std::deque<PathResult>   m_qRes;
    std::vector<std::thread> m_aThreads;

    std::atomic<bool> m_bRunning;

    int      m_iMapEX;
    int      m_iMapEY;
    int      m_nBusy;    // workers inside a search
    int      m_nAlive;   // workers past Init and not yet exited
    bool     m_bStopping;
    bool     m_bPaused;  // quiesced: Submit refuses, workers keep draining
    uint64_t m_uNextId;
};

extern PathService thePathService;

// The service under the names the game's lifecycle sites use. Each is a no-op when
// EN_PATH_WORKER is off, and Stop is idempotent.
// Step D, all main thread (unit.cpp). The drain installs answers into vehicles; the
// tick counter is bumped by the drain and is what pa.wait.ticks is measured in;
// CancelAll clears every outstanding id so nothing pends across a save or a teardown.
void     EnPathAsyncDrain    ( void );
uint64_t EnPathAsyncTick     ( void );
void     EnPathAsyncCancelAll( char const* pszReason );

// The vehicle currently inside CVehicle::Move, main thread only. A search started
// from in there is answered synchronously (unit.cpp PathAsyncEligible): Move checks,
// before it returns, that a moving vehicle holds a real next step, and a vehicle left
// pending holds none.
class CVehicle;
void            EnPathAsyncSetInMove( CVehicle const* pVeh );
CVehicle const* EnPathAsyncInMove   ( void );

class EnPathAsyncMoveScope
{
  public:
    explicit EnPathAsyncMoveScope( CVehicle const* pVeh ) : m_pPrev( EnPathAsyncInMove( ) )
    {
        EnPathAsyncSetInMove( pVeh );
    }
    ~EnPathAsyncMoveScope( ) { EnPathAsyncSetInMove( m_pPrev ); }
    EnPathAsyncMoveScope( EnPathAsyncMoveScope const& )            = delete;
    EnPathAsyncMoveScope& operator=( EnPathAsyncMoveScope const& ) = delete;

  private:
    CVehicle const* m_pPrev;
};

void EnPathWorkerStart  ( int iMapEX, int iMapEY );
void EnPathWorkerStop   ( void );
void EnPathWorkerQuiesce( void );
void EnPathWorkerResume ( void );

// Quiesced for the lifetime of the scope. A guard rather than a pair of calls because
// CGame::SaveGame returns from a dozen places and throws from the serialization itself.
class EnPathWorkerQuiesceScope
{
  public:
    EnPathWorkerQuiesceScope( ) { EnPathWorkerQuiesce( ); }
    ~EnPathWorkerQuiesceScope( ) { EnPathWorkerResume( ); }
    EnPathWorkerQuiesceScope( EnPathWorkerQuiesceScope const& )            = delete;
    EnPathWorkerQuiesceScope& operator=( EnPathWorkerQuiesceScope const& ) = delete;
};

#endif  // __PATHSERVICE_H__
