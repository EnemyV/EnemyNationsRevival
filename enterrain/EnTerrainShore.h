#pragma once
//---------------------------------------------------------------------------
// enterrain: shoreline + road facing rules, shared by world generation
// (CGameMap::AddCoastlines / AssignCoastFacings / MakeLakes, CHex::ChangeToRoad)
// and the terraintester (a map then only needs land vs. water types).
//
// The game adapts CGameMap/CHex onto ShoreGrid; SetType/SetAlt/SetCoastIndex keep
// all of the game's side effects (CHex::SetType's force-store / slope retype,
// theTerrain.GetSprite's out-of-range fallback).
//---------------------------------------------------------------------------
#include "EnTerrain.h"

namespace enterrain
{

// Coastline sprite index = art group + facing (CHex::land_ul .. island).
static const int CoastGroupOcean = 0;    // OCEAN_COAST_OFF
static const int CoastGroupLake  = 13;   // LAKE_COAST_OFF
static const int CoastGroupRiver = 26;   // RIVER_COAST_OFF
enum CoastFacing { LandUL = 0, LandUR, LandLR, LandLL, LandUp, LandRt, LandDn, LandLf,
                   WaterUL, WaterUR, WaterLR, WaterLL, Island };

// A mutable hex map. x/y passed in are ALREADY wrapped into [0,W)x[0,H).
class ShoreGrid
{
public:
    virtual ~ShoreGrid() {}
    virtual int  Width() const = 0;
    virtual int  Height() const = 0;
    virtual int  Type( int x, int y ) const = 0;
    virtual void SetType( int x, int y, int type ) = 0;
    virtual int  Alt( int x, int y ) const = 0;
    virtual void SetAlt( int x, int y, int alt ) = 0;
    // Scratch per-hex flag (the game uses CHex::lr during worldgen).
    virtual bool Mark( int x, int y ) const = 0;
    virtual void SetMark( int x, int y ) = 0;
    virtual void ClearMark( int x, int y ) = 0;
    // Current coastline sprite index (-1 = not a coastline sprite).
    virtual int  CoastIndex( int x, int y ) const = 0;
    // Assign a coastline sprite index; return true if the hex's sprite changed.
    virtual bool SetCoastIndex( int x, int y, int index ) = 0;
};

inline bool IsWaterType( int t ) { return t == TerrRiver || t == TerrOcean || t == TerrLake; }   // CHex::IsWater

// CGameMap::AddCoastlines: river-bank lowering, water/bank -> coastline conversion,
// corner fill, wall healing, dry-coast cleanup, then AssignCoastFacings (worldgen).
void AddCoastlines( ShoreGrid& g );

// CGameMap::AssignCoastFacings: facing (+ art group) of every coastline hex from its
// neighbours. wasWater[y*W+x] = coastline hex that was water before conversion.
// keepGroup = keep each hex's stored art group (load refit). Returns hexes changed.
int AssignCoastFacings( ShoreGrid& g, const unsigned char* wasWater, bool keepGroup );

// CGameMap::MakeLakes coast relabel for one hex (x,y) that just became lake: every
// coastline in its 3x3 whose index is not in the lake group gets index + 13.
void RelabelLakeCoasts( ShoreGrid& g, int x, int y );

// CHex::ChangeToRoad facing from the 4-neighbour mask: 1 above (x,y-1), 2 left (x-1,y),
// 4 right (x+1,y), 8 below (x,y+1).
int RoadFacing( int nbrMask );

//---------------------------------------------------------------------------
// SHORE DISPLAY (render only; the map's types and coast indices are untouched).
// Each coast facing covers land at some of its hex's 4 corners. Each coastline hex picks
// its facing on its own, so two coast hexes can disagree about the corners they share; a
// thin land tongue then draws as a floating land fragment. For display, a corner shows
// land only if EVERY hex at that corner says land ("water wins"), and the piece is
// re-picked from those corners. A coastline hex left with no land is drawn as SHOAL
// (shallow water over sand: water variant ShoalVariant), which is what it is in play:
// coastline that boats cross slowly.
//---------------------------------------------------------------------------
static const int ShoalVariant = 7;   // <water type>_7_* tiles in terrain_gpu

// Land-corner mask of a hex as drawn by the game: 1 NW, 2 NE, 4 SE, 8 SW.
inline int LandCorners( int type, int index )
{
    static const int k[13] = { 1, 2, 4, 8, 3, 6, 12, 9, 14, 13, 11, 7, 0 };   // LandUL..Island
    if ( type == TerrCoastline )
    {
        int f = ( index % 13 + 13 ) % 13;
        return k[f];
    }
    return IsWaterType( type ) ? 0 : 15;
}

// Display type + index of a COASTLINE hex. at(dx, dy, type, index) returns the stored
// type/index of the hex at offset (dx,dy) from it (wrapped by the caller; false = off map,
// treated as water). Returns false (draw as stored) unless the drawing changes.
template <class At>
inline bool CoastDisplay( int index, At&& at, int& outType, int& outIndex )
{
    int claim[3][3];   // [dy+1][dx+1]
    int ntype[3][3];
    for ( int dy = -1; dy <= 1; ++dy )
        for ( int dx = -1; dx <= 1; ++dx )
        {
            int t = TerrOcean, i = 0;
            if ( !( dx == 0 && dy == 0 ) && !at( dx, dy, t, i ) ) { t = TerrOcean; i = 0; }
            if ( dx == 0 && dy == 0 ) { t = TerrCoastline; i = index; }
            ntype[dy + 1][dx + 1] = t;
            claim[dy + 1][dx + 1] = LandCorners( t, i );
        }
    // corner c of this hex (NW,NE,SE,SW) is vertex (cx,cy); the 4 hexes at a vertex are
    // (cx-1..cx, cy-1..cy); the bit each of them uses for that vertex:
    static const int cx[4] = { 0, 1, 1, 0 }, cy[4] = { 0, 0, 1, 1 };
    const int self = claim[1][1];
    int mask = 0;
    for ( int c = 0; c < 4; ++c )
    {
        bool land = true;
        for ( int hy = cy[c] - 1; hy <= cy[c] && land; ++hy )
            for ( int hx = cx[c] - 1; hx <= cx[c] && land; ++hx )
            {
                int vx = cx[c] - hx, vy = cy[c] - hy;   // vertex position in that hex: 0/1
                int bit = vy == 0 ? ( vx == 0 ? 1 : 2 ) : ( vx == 0 ? 8 : 4 );
                if ( !( claim[hy + 1][hx + 1] & bit ) ) land = false;
            }
        if ( land ) mask |= 1 << c;
    }
    if ( mask == self )
        return false;
    if ( mask == 0 )
    {
        // shoal: in the water body it sits in (most common orthogonal water; else by art group)
        const int wt[3] = { TerrRiver, TerrLake, TerrOcean };
        int cnt[3] = { 0, 0, 0 };
        const int ox[4] = { 0, 1, 0, -1 }, oy[4] = { -1, 0, 1, 0 };
        for ( int e = 0; e < 4; ++e )
            for ( int q = 0; q < 3; ++q )
                if ( ntype[oy[e] + 1][ox[e] + 1] == wt[q] ) ++cnt[q];
        const int group = ( index / 13 ) * 13;
        int best = group == CoastGroupRiver ? 0 : ( group == CoastGroupLake ? 1 : 2 );
        for ( int q = 0; q < 3; ++q )
            if ( cnt[q] > cnt[best] ) best = q;
        outType = wt[best];
        outIndex = ShoalVariant;
        return true;
    }
    for ( int f = 0; f < 12; ++f )
        if ( LandCorners( TerrCoastline, f ) == mask )
        {
            int group = ( index / 13 ) * 13;
            if ( group > CoastGroupRiver ) group = CoastGroupOcean;   // out-of-range index draws as 0..12
            outType = TerrCoastline;
            outIndex = group + f;
            return true;
        }
    return false;   // two opposite corners only: no art for it, keep the stored piece
}

} // namespace enterrain
