#ifndef __PATHWORLD_H__
#define __PATHWORLD_H__

////////////////////////////////////////////////////////////////////////////
//
//  pathworld.h : an immutable copy of everything the movement A* reads.
//
//  PathWorld holds LOGICAL VALUES only - no CHex*, CVehicle*, CBuilding* or
//  CBridgeUnit* is ever dereferenced out of one. Buildings and bridges are
//  identified by the same opaque token CEnNavView hands the search today (the
//  object address as a size_t), which is compared for identity and nothing
//  else.
//
//  It is built on the MAIN thread by Build(), from the same reads the live
//  view makes, and published by swapping a shared_ptr. A published PathWorld
//  is never written again, so a reader needs no lock beyond acquiring its own
//  shared_ptr.
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "terrain.h"
#include <memory>
#include <vector>

// Hex key -> record index. Built once, read-only afterwards: open addressing,
// linear probe, power-of-two capacity, no allocation on a lookup.
class CPwIndex
{
  public:
    enum { none = 0xFFFFFFFFu };

    void     Reserve( size_t nEntries );
    void     Insert( DWORD dwKey, DWORD dwVal );
    DWORD    Find( DWORD dwKey ) const;
    size_t   Bytes( ) const { return ( m_aKey.size( ) * sizeof( DWORD ) * 2 ); }

  private:
    std::vector<DWORD> m_aKey;   // none == empty slot
    std::vector<DWORD> m_aVal;
    DWORD              m_dwMask = 0;
};

class PathWorld
{
  public:
    // What every vehicle in a hex is doing, derived exactly as
    // CEnNavView::IsHexMovingVehicle derives it from the four sub-hexes.
    enum { occ_none = 0, occ_moving = 1, occ_blocked = 2 };

    struct Hex
    {
        BYTE bUnits;  // CHex::GetUnits()
        BYTE bType;   // CHex::GetType()
        BYTE bAlt;    // CHex::GetAlt()
        BYTE bOcc;    // occ_*
    };

    struct Bldg
    {
        size_t    uKey;       // opaque CBuilding identity, never dereferenced
        CHexCoord hexExit;
        CHexCoord hexShip;
        int       iExitDir;
        int       iShipDir;
        BOOL      bShipExit;  // GetData()->HasShipExit()
    };

    // One record per REGISTERED BRIDGE HEX, not per span: theBridgeHex is already
    // keyed by hex and a CBridgeUnit is itself a per-hex object, so a per-hex
    // record is both the natural shape and the smaller one - a span table would
    // need the same per-hex index plus a second indirection to save nothing.
    struct Bridge
    {
        size_t uParent;       // opaque CBridge identity, never dereferenced
        int    iExit;
        BOOL   bParentBuilt;
    };

    // Main thread. Reads the live globals through the same calls the live view
    // makes. Returns NULL if there is no world.
    static std::shared_ptr<const PathWorld> Build( void );

    // The currently published snapshot, or NULL.
    static std::shared_ptr<const PathWorld> Current( void );
    static void                             Publish( std::shared_ptr<const PathWorld> const& ptr );
    static void                             Clear( void );

    // EN_PATH_SNAP names a value: 1 / true / on / yes enable the per-tick build.
    static BOOL Enabled( void );
    // Called once per main-loop tick from the publication point.
    static void PublishTick( void );

    // MIRRORS CGameMap::GetHex( int, int ) (terrain.inl), which is _GetHex( Wrap(x),
    // Wrap(y) ) - the accessor CEnLiveNavView::GetHex reads every hex through. The
    // row pitch is the map's own ( y << m_iSideShift ) + x, and the mask is applied
    // to BOTH axes exactly as CHexCoord::Wrap applies it (the map is square and a
    // power of two - see the BUGS #65 note in terrain.inl). Without the mask an
    // unwrapped y walks into another row and an unwrapped x walks into the next one,
    // which is a live-vs-snapshot divergence at the wrap seam and nowhere else.
    Hex const& At( int x, int y ) const
    {
        return ( m_aHex[( (size_t)( y & m_iHexMask ) << m_iSideShift ) + (size_t)( x & m_iHexMask )] );
    }

    // theBuildingHex::_GetBuilding does NOT wrap its key (building.inl _ToArg) and
    // theBridgeHex::GetBridge DOES (bridge.h ToArg). The live view calls exactly those
    // two, so these two mirror exactly those two - a miss on an unwrapped building key
    // is the live behaviour, not a defect.
    Bldg const* FindBldg( int x, int y ) const
    {
        DWORD dw = m_idxBldg.Find( Key( x, y ) );
        return ( dw == CPwIndex::none ? NULL : &m_aBldg[dw] );
    }
    Bridge const* FindBridge( int x, int y ) const
    {
        DWORD dw = m_idxBridge.Find( Key( x & m_iHexMask, y & m_iHexMask ) );
        return ( dw == CPwIndex::none ? NULL : &m_aBridge[dw] );
    }

    int WheelMult( int iTerrainType, int iWheel ) const { return ( m_aiWheelMult[iTerrainType][iWheel] ); }

    int Wrap( int iVal ) const { return ( iVal & m_iHexMask ); }
    int Diff( int iVal ) const { return ( ( ( iVal + m_iWidthHalf ) & m_iHexMask ) - m_iWidthHalf ); }

    int TrafficRules( ) const { return ( m_iTrafficRules ); }

    uint64_t Epoch( ) const { return ( m_uEpoch ); }
    uint32_t Generation( ) const { return ( m_uGeneration ); }
    size_t   Bytes( ) const;

    int Width( ) const { return ( m_iWidth ); }
    int Height( ) const { return ( m_iHeight ); }

  private:
    static DWORD Key( int x, int y ) { return ( ( (DWORD)x << 16 ) | (DWORD)y ); }

    std::vector<Hex>    m_aHex;
    std::vector<Bldg>   m_aBldg;
    std::vector<Bridge> m_aBridge;
    CPwIndex            m_idxBldg;
    CPwIndex            m_idxBridge;

    int m_aiWheelMult[CHex::num_types][NUM_WHEEL_TYPES];

    int m_iWidth      = 0;
    int m_iHeight     = 0;
    int m_iHexMask    = 0;
    int m_iWidthHalf  = 0;
    int m_iSideShift  = 0;
    int m_iTrafficRules = 0;

    uint64_t m_uEpoch      = 0;
    uint32_t m_uGeneration = 0;
};

// New world / load / close. Resets the epoch to 0 and bumps the generation, so a
// snapshot taken in a previous world can never read as current in this one.
void EnNavNewGame( void );
uint32_t EnNavGameGeneration( void );

#endif  // __PATHWORLD_H__
