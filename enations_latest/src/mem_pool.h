//
// Created by Bobbias on 025, 2023-05-25.
//
// from here methinks
// http://www.mario-konrad.ch/blog/programming/cpp-memory_pool.html
#ifndef ENATIONS_MEM_POOL_H
#define ENATIONS_MEM_POOL_H
#pragma once

#include "netcmd.h"
#include "vdmplay.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <mutex>

// Forward declaration of memory pool wrapper
template<class Strategy>
class memory_pool
{
  private:
    Strategy strat;

  public:
    memory_pool( ) { strat.init( ); }

    memory_pool( const memory_pool& )            = delete;
    memory_pool& operator=( const memory_pool& ) = delete;

    void* alloc( ) { return strat.allocate( ); }

    void  init( ) { strat.init( ); }

    void free( void* p ) { strat.deallocate( p ); }
};

// Memory pool strategy using free list
template<unsigned int S, unsigned int N>
class mempool_std_heap
{
  private:
    // Free list node structure
    struct EntryList
    {
        uint8_t*   p;     // pointer to memory chunk
        EntryList* next;  // next free chunk
    };

    std::mutex           mtx;  // add a mutex
    typedef unsigned int size_type;

  private:
    uint8_t*   buf;         // pointer to memory buffer (dynamically allocated)
    EntryList  entries[N];  // bookkeeping array
    EntryList* freeList;    // head of free list
    size_type  available;   // number of available chunks
    bool isInit = false;

#ifdef _DEBUG
    // tracking for testing/debugging
    std::unordered_set<void*> allocated;
#endif

    mempool_std_heap( const mempool_std_heap& )            = delete;
    mempool_std_heap& operator=( const mempool_std_heap& ) = delete;


  public:
    mempool_std_heap( ): buf( nullptr ), freeList( nullptr ), available( 0 ) {}

    ~mempool_std_heap( )
    {
        if ( buf != nullptr )
        {
            delete[] buf;
            buf = nullptr;
        }
        freeList = nullptr;
        isInit = false;
    }

    void init( )
    {
        std::lock_guard<std::mutex> lock( mtx );

        // Allocate the buffer if not already allocated
        if ( !isInit )
        {
            buf = new uint8_t[S * N];
        }
        else
        {
            CString str;
            str.Format( "Was already initialized!!" );
            OutputDebugStringA( str );        
        }

        // Zero out the entire buffer
        std::memset( buf, 0, S * N );

        // Initialize available count
        available = N;

        // Initialize the free list - link all entries together
        for ( size_type i = 0; i < N; ++i )
        {
            entries[i].p    = &buf[S * i];
            entries[i].next = ( i < N - 1 ) ? &entries[i + 1] : nullptr;
        }

        // Set free list head to first entry
        freeList = &entries[0];

#ifdef _DEBUG
        allocated.clear( );
#endif
#ifdef _DEBUG
        CString str;
        str.Format( "init: buf=%p, size=%u", buf, S * N );
        OutputDebugStringA( str );
        OutputDebugStringA( "\n" );

        for ( size_type i = 0; i < N; ++i )
        {
            str.Format( "init: entry[%u].p = %p", i, entries[i].p );
            OutputDebugStringA( str );
            OutputDebugStringA( "\n" );
        }
#endif


        isInit = true;
    }

    // Allocate a block from the pool
    void* allocate( )
    {
        std::lock_guard<std::mutex> lock( mtx );

        if (!isInit)
        {
            ASSERT( false );  // Must call init() before allocate()
            init( );
        }

        // Check if we have available blocks
        if ( freeList == nullptr || available == 0 )
        {
            return nullptr;  // Pool exhausted
        }

        // Get the first free block
        EntryList* node = freeList;

        // Defensive check - ensure node pointer is valid
        if ( node == nullptr || node->p == nullptr )
        {
            return nullptr;
        }

        void* ptr = node->p;
#ifdef _DEBUG
        CString str;
        str.Format( "allocate: ptr=%p, buf=%p, buf + size=%p", ptr, buf, buf + ( S * N ) );
        OutputDebugStringA( str );
        OutputDebugStringA( "\n" );
#endif



        // Verify pointer is within our buffer bounds
        uint8_t* p = static_cast<uint8_t*>( ptr );
        if ( p < buf || p >= buf + ( S * N ) )
        {
            return nullptr;  // Invalid pointer
        }

        // Move free list head to next free block
        freeList = node->next;
        --available;

        // Zero out the memory - use size_t to avoid any truncation
        std::memset( ptr, 0, static_cast<size_t>( S ) );

#ifdef _DEBUG
        // Track this pointer
        allocated.insert( ptr );
#endif

        return ptr;
    }

    // Deallocate a block back to the pool
    void deallocate( void* ptr )
    {
        if ( ptr == nullptr )
        {
            return;
        }

        std::lock_guard<std::mutex> lock( mtx );
#ifdef _DEBUG
        // Verify pointer was allocated from this pool
        if ( allocated.find( ptr ) == allocated.end( ) )
        {
            // Error: trying to free unallocated pointer
            return;
        }
        allocated.erase( ptr );
#endif

        // Find which entry this pointer corresponds to
        uint8_t*  p   = static_cast<uint8_t*>( ptr );
        size_type idx = ( p - buf ) / S;

        // Validate the pointer is within our buffer and properly aligned
        if ( idx >= N || p != &buf[S * idx] )
        {
            // Invalid pointer - not from this pool
            return;
        }

        // Return the block to the free list
        entries[idx].next = freeList;
        freeList          = &entries[idx];
        ++available;
    }

    // Query available blocks
    size_type get_available( ) const { return available; }

    // Check if pool is empty
    bool is_empty( ) const { return available == N; }

    // Check if pool is full
    bool is_full( ) const { return available == 0; }
};

// Pool size constants
const int SMALL_POOL_SIZE = ( 48 + 3 ) & ~3;  // Align to 4-byte boundary

// Type definitions for specific pool sizes
typedef memory_pool<mempool_std_heap<VP_MAXSENDDATA, 10>>   mempool_large;
typedef memory_pool<mempool_std_heap<SMALL_POOL_SIZE, 100>> mempool_small;

#endif  // ENATIONS_MEM_POOL_H