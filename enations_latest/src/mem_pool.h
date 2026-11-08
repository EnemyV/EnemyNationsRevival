//
// Created by Bobbias on 025, 2023-05-25.
//
#ifndef ENATIONS_MEM_POOL_H
#define ENATIONS_MEM_POOL_H
#pragma once

#include "netcmd.h"
#include "vdmplay.h"
#include "EnSettings.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_set>
#include <vector>

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
    void  free( void* p ) { strat.deallocate( p ); }
};

// Memory pool strategy using individual heap allocations
// Each block is allocated separately, so no pointer invalidation!
template<unsigned int S, unsigned int N>
class mempool_std_heap
{
  private:
    struct Block
    {
        uint8_t data[S];

        Block( ) { std::memset( data, 0, S ); }
    };

    std::mutex           mtx;
    typedef unsigned int size_type;

  private:
    std::vector<Block*> allocatedBlocks;  // Track all blocks we've allocated
    std::vector<Block*> freeList;         // Available blocks for reuse
    bool                isInit;

#ifdef _DEBUG
    std::unordered_set<void*> activeAllocations;  // Track active allocations
#endif

    mempool_std_heap( const mempool_std_heap& )            = delete;
    mempool_std_heap& operator=( const mempool_std_heap& ) = delete;

  public:
    mempool_std_heap( ): isInit( false )
    {
        allocatedBlocks.reserve( N );
        freeList.reserve( N );
    }

    ~mempool_std_heap( )
    {
        std::lock_guard<std::mutex> lock( mtx );

        // Delete all blocks
        for ( Block* block: allocatedBlocks )
        {
            if ( block  == nullptr )
            {
#if LOGGINGON
                // log that this is an issue!
                std::string str = strPrintf(
                    "mempool_std_heap::~mempool_std_heap: ERROR - null block pointer found during destruction\n" );
                OutputDebugStringA( str.c_str() );
#endif
                continue;
            }
            delete block;
        }

        allocatedBlocks.clear( );
        freeList.clear( );
        isInit = false;
    }

    void init( )
    {
        std::lock_guard<std::mutex> lock( mtx );

        if ( isInit )
        {
            OutputDebugStringA( "Was already initialized!!" );
            return;
        }

        // Pre-allocate initial blocks
        for ( size_type i = 0; i < N; ++i )
        {
            Block* block = new Block( );
            allocatedBlocks.push_back( block );
            freeList.push_back( block );
        }

#ifdef _DEBUG
        activeAllocations.clear( );

        // strPrintf only handles positional %1/%2 (not printf %u/%s) — use snprintf here.
        char buf[128];
        _snprintf( buf, sizeof( buf ), "init: allocated %u blocks of size %u bytes each\n",
                   (unsigned)N, (unsigned)S );
        OutputDebugStringA( buf );
#endif

        isInit = true;
    }

    void expandMemoryPool( )
    {
        if ( !isInit )
        {
            ASSERT( FALSE );
            return;
        }

        size_type currentSize = allocatedBlocks.size( );
        size_type newBlocks   = currentSize;  // Double the pool size

#ifdef _DEBUG
        // strPrintf only handles positional %1/%2 (not printf %u/%s) — use snprintf.
        char buf1[128];
        _snprintf( buf1, sizeof( buf1 ), "Expanding pool: adding %u blocks (total will be %u)\n",
                   (unsigned)newBlocks, (unsigned)( currentSize + newBlocks ) );
        OutputDebugStringA( buf1 );
#endif

        // Allocate new blocks - each on its own heap address
        // Old pointers remain completely valid!
        for ( size_type i = 0; i < newBlocks; ++i )
        {
            Block* block = new Block( );
            allocatedBlocks.push_back( block );
            freeList.push_back( block );
        }

#ifdef _DEBUG
        char buf2[128];
        _snprintf( buf2, sizeof( buf2 ), "Pool expanded. Total: %u blocks, Free: %u blocks\n",
                   (unsigned int)allocatedBlocks.size( ),
                   (unsigned int)freeList.size( ) );
        OutputDebugStringA( buf2 );
#endif
    }

    // Allocate a block from the pool
    void* allocate( )
    {
        std::lock_guard<std::mutex> lock( mtx );

        if ( !isInit )
        {
            ASSERT( false );
            init( );
        }

        // Expand if we're out of blocks
        if ( freeList.empty( ) )
        {
            expandMemoryPool( );
        }

        // Get a free block
        Block* block = freeList.back( );
        freeList.pop_back( );

        if ( block == nullptr )
        {
            ASSERT( false );
            return nullptr;
        }

        // Zero out the block
        std::memset( block->data, 0, S );

#ifdef _DEBUG
        activeAllocations.insert( block->data );
        // CString str;
        // str.Format( "allocate: ptr=%p, free=%u, active=%u\n", block->data, (unsigned int)freeList.size( ),
        //            (unsigned int)activeAllocations.size( ) );
        // OutputDebugStringA( str );
#endif

        return block->data;
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
        // Verify this was allocated from our pool
        if ( activeAllocations.find( ptr ) == activeAllocations.end( ) )
        {
            std::string str = strPrintf( "deallocate: ERROR - ptr=%p not in active allocations\n", ptr );
            OutputDebugStringA( str.c_str() );
            ASSERT( false );
            return;
        }
        activeAllocations.erase( ptr );
#endif

        // Recover the owning Block in O(1): `data` is the sole member of Block
        // (offset 0, standard-layout), so allocate() returns block->data ==
        // (Block*)block. The old code linear-scanned allocatedBlocks here — O(n)
        // per free, and n grows without bound as the pool doubles (this pool
        // backs every projectile, so it hit ~51k blocks in a single battle,
        // making combat teardown O(n^2)). Pointer identity makes the scan
        // unnecessary.
        static_assert( offsetof( Block, data ) == 0,
                       "allocate() returns block->data; O(1) free needs data at offset 0" );
        Block* targetBlock = reinterpret_cast<Block*>( ptr );

#ifdef _DEBUG
        // Debug-only integrity checks (kept O(n) — Debug already pays for the
        // activeAllocations set above, and these catch corruption/double-free
        // during development). Release trusts the caller (a correct program
        // never frees a foreign or already-freed pointer), keeping free O(1).
        {
            bool inPool = false;
            for ( Block* block : allocatedBlocks )
                if ( block == targetBlock ) { inPool = true; break; }
            if ( !inPool ) { ASSERT( false ); return; }
            for ( Block* freeBlock : freeList )
                if ( freeBlock == targetBlock ) { ASSERT( false ); return; }  // double free
        }
#endif

        // Return to free list
        freeList.push_back( targetBlock );

#ifdef _DEBUG
       // CString str;
       // str.Format( "deallocate: ptr=%p, free=%u, active=%u\n", ptr, (unsigned int)freeList.size( ),
       //             (unsigned int)activeAllocations.size( ) );
       // OutputDebugStringA( str );
#endif
    }

    // Query available blocks
    size_type get_available( ) const
    {
        std::lock_guard<std::mutex> lock( const_cast<std::mutex&>( mtx ) );
        return freeList.size( );
    }

    // Check if pool is empty
    bool is_empty( ) const
    {
        std::lock_guard<std::mutex> lock( const_cast<std::mutex&>( mtx ) );
        return freeList.size( ) == allocatedBlocks.size( );
    }

    // Check if pool is full
    bool is_full( ) const
    {
        std::lock_guard<std::mutex> lock( const_cast<std::mutex&>( mtx ) );
        return freeList.empty( );
    }
};

// Pool size constants
const int SMALL_POOL_SIZE = ( 48 + 3 ) & ~3;  // Align to 4-byte boundary

// Type definitions for specific pool sizes
typedef memory_pool<mempool_std_heap<VP_MAXSENDDATA, 10>>   mempool_large;
typedef memory_pool<mempool_std_heap<SMALL_POOL_SIZE, 100>> mempool_small;

#endif  // ENATIONS_MEM_POOL_H