//
// Created by Bobbias on 025, 2023-05-25.
//
#ifndef ENATIONS_MEM_POOL_H
#define ENATIONS_MEM_POOL_H
#pragma once

#include "netcmd.h"
#include "vdmplay.h"

#include <algorithm>
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
            CString str;
            str.Format( "Was already initialized!!" );
            OutputDebugStringA( str );
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

        CString str;
        str.Format( "init: allocated %u blocks of size %u bytes each\n", N, S );
        OutputDebugStringA( str );
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
        CString str;
        str.Format( "Expanding pool: adding %u blocks (total will be %u)\n", newBlocks, currentSize + newBlocks );
        OutputDebugStringA( str );
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
        str.Format( "Pool expanded. Total: %u blocks, Free: %u blocks\n", (unsigned int)allocatedBlocks.size( ),
                    (unsigned int)freeList.size( ) );
        OutputDebugStringA( str );
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

        CString str;
        str.Format( "allocate: ptr=%p, free=%u, active=%u\n", block->data, (unsigned int)freeList.size( ),
                    (unsigned int)activeAllocations.size( ) );
        OutputDebugStringA( str );
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
            CString str;
            str.Format( "deallocate: ERROR - ptr=%p not in active allocations\n", ptr );
            OutputDebugStringA( str );
            ASSERT( false );
            return;
        }
        activeAllocations.erase( ptr );
#endif

        // Find the block that contains this pointer
        Block* targetBlock = nullptr;

        for ( Block* block: allocatedBlocks )
        {
            if ( block->data == ptr )
            {
                targetBlock = block;
                break;
            }
        }

        if ( targetBlock == nullptr )
        {
#ifdef _DEBUG
            CString str;
            str.Format( "deallocate: ERROR - ptr=%p not found in pool\n", ptr );
            OutputDebugStringA( str );
#endif
            ASSERT( false );
            return;
        }

        // Check if already freed
        for ( Block* freeBlock: freeList )
        {
            if ( freeBlock == targetBlock )
            {
#ifdef _DEBUG
                CString str;
                str.Format( "deallocate: ERROR - ptr=%p already freed (double free)\n", ptr );
                OutputDebugStringA( str );
#endif
                ASSERT( false );
                return;
            }
        }

        // Return to free list
        freeList.push_back( targetBlock );

#ifdef _DEBUG
        CString str;
        str.Format( "deallocate: ptr=%p, free=%u, active=%u\n", ptr, (unsigned int)freeList.size( ),
                    (unsigned int)activeAllocations.size( ) );
        OutputDebugStringA( str );
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