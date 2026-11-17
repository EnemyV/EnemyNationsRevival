//---------------------------------------------------------------------------
// enterrain: shoreline + road facing rules. Moved verbatim (logic-wise) from
// wrldinit.cpp (CGameMap::AddCoastlines / AssignCoastFacings / MakeLakes relabel)
// and terrain.cpp (CHex::ChangeToRoad facing switch). Comments kept from the game.
//---------------------------------------------------------------------------
#include "EnTerrainShore.h"

#include <cstring>
#include <vector>

namespace enterrain
{

namespace
{
// theMap.GetHex( x, y ): wraps both coordinates.
struct G
{
    ShoreGrid& g;
    int W, H;
    explicit G( ShoreGrid& gr ) : g( gr ), W( gr.Width() ), H( gr.Height() ) {}
    int  wx( int x ) const { return ( ( x % W ) + W ) % W; }
    int  wy( int y ) const { return ( ( y % H ) + H ) % H; }
    int  Type( int x, int y ) const { return g.Type( wx( x ), wy( y ) ); }
    void SetType( int x, int y, int t ) { g.SetType( wx( x ), wy( y ), t ); }
    int  Alt( int x, int y ) const { return g.Alt( wx( x ), wy( y ) ); }
    void SetAlt( int x, int y, int a ) { g.SetAlt( wx( x ), wy( y ), a ); }
    bool IsWater( int x, int y ) const { return IsWaterType( Type( x, y ) ); }
    // Shore-SUPPRESSION test: IsWater() excludes swamp, but swamp is water-like, so the
    // bank loops treat it as water for the don't-shore decision only.
    bool IsShoreWater( int x, int y ) const { return IsWater( x, y ) || Type( x, y ) == TerrSwamp; }
    bool Mark( int x, int y ) const { return g.Mark( wx( x ), wy( y ) ); }
    void SetMark( int x, int y ) { g.SetMark( wx( x ), wy( y ) ); }
    int  Off( int x, int y ) const { return wy( y ) * W + wx( x ); }
};
} // namespace

// For oceans (& lakes) we use the water tiles so it stays flat.
// For rivers we surround the existing tiles and lower the river altitude
void AddCoastlines( ShoreGrid& grid )
{
    G g( grid );
    const int W = g.W, H = g.H;
    const int lTotal = W * H;

    // first we drop all river hexes (all 4 corners) by 1, but not below sea_level
    for ( int y = 0; y < H; ++y )
        for ( int x = 0; x < W; ++x )
        {
            if ( g.Type( x, y ) == TerrRiver )
            {
                if ( !g.Mark( x, y ) )
                    if ( g.Alt( x, y ) > SeaLevel )
                        g.SetAlt( x, y, g.Alt( x, y ) - 1 );

                // above, upper right, right (the guards test the BANK hex)
                static const int dx[3] = { 0, 1, 1 }, dy[3] = { -1, -1, 0 };
                for ( int k = 0; k < 3; ++k )
                {
                    int tx = x + dx[k], ty = y + dy[k];
                    if ( ( !g.IsWater( tx, ty ) ) && ( !g.Mark( tx, ty ) ) )
                        if ( g.Alt( tx, ty ) > SeaLevel + 1 )
                        {
                            g.SetAlt( tx, ty, g.Alt( tx, ty ) - 1 );
                            g.SetMark( tx, ty );
                        }
                }
            }
        }

    // turn off lr
    for ( int y = 0; y < H; ++y )
        for ( int x = 0; x < W; ++x )
            grid.ClearMark( x, y );

    // ORIGIN TRACKING for the facing pass: in a tight corner (1-wide channel, narrow
    // mouth) EVERY water hex of the passage becomes coastline; remember which coastline
    // hexes started as WATER so the facing pass can treat them as wet.
    std::vector<unsigned char> wasWater( lTotal, 0 );

    // set coastlines
    for ( int y = 0; y < H; ++y )
        for ( int x = 0; x < W; ++x )
        {
            bool isCoast = false;
            // oceans (and lakes) we convert the edge water
            if ( g.Type( x, y ) == TerrOcean )
            {
                // River/lake MOUTH: an ocean hex that touches another water body must stay
                // open water (else the whole narrow gap becomes a shore WALL between the
                // river and the sea). The river's own banks carry the land-side shore art.
                bool bMouth = false;
                for ( int ix = -1; ix <= 1 && !bMouth; ix++ )
                    for ( int iy = -1; iy <= 1 && !bMouth; iy++ )
                    {
                        int iNType = g.Type( x + ix, y + iy );
                        if ( iNType == TerrRiver || iNType == TerrLake )
                            bMouth = true;
                    }
                if ( bMouth )
                {
                    // the gap stays open water, but its LAND banks still need a shore band
                    for ( int ix = -1; ix <= 1; ix++ )
                        for ( int iy = -1; iy <= 1; iy++ )
                            if ( ( !g.IsShoreWater( x + ix, y + iy ) ) && ( g.Type( x + ix, y + iy ) != TerrCoastline ) )
                                g.SetType( x + ix, y + iy, TerrCoastline );
                }
                else
                    for ( int ix = -1; ix <= 1 && !isCoast; ix++ )
                        for ( int iy = -1; iy <= 1 && !isCoast; iy++ )
                            if ( ( !g.IsShoreWater( x + ix, y + iy ) ) && ( g.Type( x + ix, y + iy ) != TerrCoastline ) )
                            {
                                g.SetType( x, y, TerrCoastline );
                                wasWater[y * W + x] = 1;
                                isCoast = true;   // (the game's goto IsCoast)
                            }
            }
            if ( isCoast )
                continue;

            // rivers we put riverbanks around the outside of water (lakes too: river-made
            // lakes exist BEFORE this pass, unlike MakeLakes' ocean->lake relabel)
            if ( ( g.Type( x, y ) == TerrRiver ) || ( g.Type( x, y ) == TerrLake ) )
                for ( int ix = -1; ix <= 1; ix++ )
                    for ( int iy = -1; iy <= 1; iy++ )
                        if ( ( !g.IsShoreWater( x + ix, y + iy ) ) && ( g.Type( x + ix, y + iy ) != TerrCoastline ) )
                            g.SetType( x + ix, y + iy, TerrCoastline );
        }

    // we now have a bunch of possibilities where we don't have appropiate art
    // so we walk through again and create additional coastline as needed
    // WE DO THIS TWICE BECAUSE PLACING COASTLINE MAY MAKE ANOTHER PREVIOUS HEX CHANGED
    for ( int iTest = 0; iTest < 2; iTest++ )
        for ( int y = 0; y < H; ++y )
            for ( int x = 0; x < W; ++x )
            {
                if ( g.Type( x, y ) != TerrCoastline )
                    continue;
                // for any side we have water on, we need coastline or water on the adjoining
                // sides (set to coastline if it's land). We just blast coastline.
                // above & below
                if ( g.IsWater( x, y - 1 ) || g.IsWater( x, y + 1 ) )
                {
                    if ( !g.IsShoreWater( x - 1, y ) ) g.SetType( x - 1, y, TerrCoastline );
                    if ( !g.IsShoreWater( x + 1, y ) ) g.SetType( x + 1, y, TerrCoastline );
                }
                // left & right
                if ( g.IsWater( x - 1, y ) || g.IsWater( x + 1, y ) )
                {
                    if ( !g.IsShoreWater( x, y - 1 ) ) g.SetType( x, y - 1, TerrCoastline );
                    if ( !g.IsShoreWater( x, y + 1 ) ) g.SetType( x, y + 1, TerrCoastline );
                }
            }

    // [shore v3] DETECT + heal coastline WALLS through water: a coastline hex that splits
    // two WATER areas instead of separating water from land. ORTHOGONAL neighbours only;
    // MARK-then-APPLY in one sweep. A coastline hex is a wall if:
    //   (a) its orthogonal neighbours hold >=2 DIFFERENT open-water types, OR
    //   (b) LAKE on an OPPOSITE pair (a same-body lake neck).
    // Heal: convert to water (lake if any lake nbr, else river) at sea level.
    {
        std::vector<unsigned char> wall( lTotal, 0 );
        for ( int y = 0; y < H; ++y )
            for ( int x = 0; x < W; ++x )
            {
                if ( g.Type( x, y ) != TerrCoastline )
                    continue;
                const int ox[4] = { x, x, x - 1, x + 1 }, oy[4] = { y - 1, y + 1, y, y };   // above, below, left, right
                int  iWaterType = -1;
                bool bTwoTypes  = false;
                for ( int n = 0; n < 4; n++ )
                {
                    if ( !g.IsWater( ox[n], oy[n] ) )
                        continue;
                    int iType = g.Type( ox[n], oy[n] );
                    if ( iWaterType < 0 )
                        iWaterType = iType;
                    else if ( iType != iWaterType )
                        bTwoTypes = true;
                }
                bool bLakeNeck =
                    ( g.Type( x, y - 1 ) == TerrLake && g.Type( x, y + 1 ) == TerrLake ) ||
                    ( g.Type( x - 1, y ) == TerrLake && g.Type( x + 1, y ) == TerrLake );
                if ( bTwoTypes || bLakeNeck )
                    wall[y * W + x] = 1;
            }
        // apply (separate sweep -> decisions used ORIGINAL types, no cascade)
        for ( int y = 0; y < H; ++y )
            for ( int x = 0; x < W; ++x )
            {
                if ( !wall[y * W + x] )
                    continue;
                bool bLake = false;
                for ( int dy = -1; dy <= 1; dy++ )
                    for ( int dx = -1; dx <= 1; dx++ )
                        if ( g.Type( x + dx, y + dy ) == TerrLake )
                            bLake = true;
                g.SetType( x, y, bLake ? TerrLake : TerrRiver );
                if ( g.Alt( x, y ) > SeaLevel )
                    g.SetAlt( x, y, SeaLevel );   // draw flat as water
            }
    }

    // one last time through changing. We can have coastline tiles that don't touch
    // water even on a diaganol. This we turn back to plains
    for ( int y = 0; y < H; ++y )
        for ( int x = 0; x < W; ++x )
        {
            if ( g.Type( x, y ) != TerrCoastline )
                continue;
            bool haveWater = false;
            for ( int ix = -1; ix <= 1 && !haveWater; ix++ )
                for ( int iy = -1; iy <= 1 && !haveWater; iy++ )
                    if ( g.IsWater( x + ix, y + iy ) )
                        haveWater = true;
            if ( haveWater )
                continue;
            // ok, we're clean
            if ( g.Alt( x, y ) <= SeaLevel )
                g.SetAlt( x, y, SeaLevel + 1 );
            g.SetType( x, y, TerrPlain );
        }

    // now assign sprites
    AssignCoastFacings( grid, wasWater.data(), false );
}

// Assign coastline sprite facings from the 4-neighbor configuration. Three tiers:
//  1. REAL open-water neighbors decide (original 1996 behavior), plus shapes for the
//     masks 1996 declared "impossible" (bank tongue, narrow mouths).
//  2. No real water but this hex was water and has water-origin coastline neighbors:
//     a tight passage whose ENTIRE channel converted to coastline. Face along it.
//  3. Original coastline-neighbor fallback (land-origin inside corners).
// keepGroup keeps each hex's ocean/lake/river art group from its stored facing.
int AssignCoastFacings( ShoreGrid& grid, const unsigned char* pbWasWater, bool keepGroup )
{
    G g( grid );
    const int W = g.W, H = g.H;
    int nChanged = 0;

    // "wet" = real water OR a coastline hex that was water before conversion.
    auto wet = [&]( int x, int y ) -> bool {
        return g.IsWater( x, y ) || ( g.Type( x, y ) == TerrCoastline && pbWasWater[g.Off( x, y )] );
    };

    for ( int y = 0; y < H; ++y )
        for ( int x = 0; x < W; ++x )
        {
            if ( g.Type( x, y ) != TerrCoastline )
                continue;
            const int lOn = y * W + x;
            // 4-bit masks (0 - 15) for water & coastline neighbors
            int  iWater = 0, iWet = 0, iCoast = 0, iTyp = CoastGroupOcean;
            bool bAnyRiver = false, bAnyOpen = false;

            const int nx[4] = { x, x + 1, x, x - 1 };   // above (1), right (2), below (4), left (8)
            const int ny[4] = { y - 1, y, y + 1, y };
            for ( int iN = 0; iN < 4; iN++ )
            {
                if ( g.IsWater( nx[iN], ny[iN] ) )
                {
                    if ( g.Type( nx[iN], ny[iN] ) == TerrRiver )
                        bAnyRiver = true;
                    else                                   // ocean or lake = "open" water
                        bAnyOpen = true;
                    iWater |= 1 << iN;
                    iWet   |= 1 << iN;
                }
                else if ( g.Type( nx[iN], ny[iN] ) == TerrCoastline )
                {
                    iCoast |= 1 << iN;
                    if ( pbWasWater[g.Off( nx[iN], ny[iN] )] )
                        iWet |= 1 << iN;
                }
            }

            // [shore-arttype] Pick the coast ART GROUP from the OPEN water it borders: river-
            // bank art only when river is the ONLY water type bordering.
            if ( bAnyRiver && !bAnyOpen )
                iTyp = CoastGroupRiver;

            // TIER 1: if we have REAL water on any border then water decides
            int iIndex = Island;
            if ( iWater != 0 )
            {
                switch ( iWater )
                {
                case 1:  iIndex = LandDn; break;   // water above
                case 2:  iIndex = LandLf; break;   // water right
                case 3:  iIndex = LandLL; break;   // water above & right
                case 4:  iIndex = LandUp; break;   // water below
                case 6:  iIndex = LandUL; break;   // water right & below
                case 8:  iIndex = LandRt; break;   // water left
                case 9:  iIndex = LandLR; break;   // water above & left
                case 12: iIndex = LandUR; break;   // water below & left
                // 1996 "impossible" masks: show the bank transition toward the land side(s)
                case 5:  iIndex = LandRt; break;   // water above & below (strait)
                case 7:  iIndex = LandLf; break;   // land LEFT only
                case 10: iIndex = LandUp; break;   // water left & right (strait)
                case 11: iIndex = LandDn; break;   // land BELOW only
                case 13: iIndex = LandRt; break;   // land RIGHT only
                case 14: iIndex = LandUp; break;   // land ABOVE only
                case 15:  // wet on all 4 sides
                    // A land-origin hex here is a true 1-hex island. A WATER-origin hex only
                    // borders land diagonally: kiss that corner with quarter-land art instead.
                    iIndex = Island;
                    if ( pbWasWater[lOn] )
                    {
                        if ( !wet( x - 1, y - 1 ) )      iIndex = LandUL;
                        else if ( !wet( x + 1, y - 1 ) ) iIndex = LandUR;
                        else if ( !wet( x + 1, y + 1 ) ) iIndex = LandLR;
                        else if ( !wet( x - 1, y + 1 ) ) iIndex = LandLL;
                    }
                    break;
                }
            }
            else if ( pbWasWater[lOn] && iWet != 0 )
            {
                // TIER 2: a tight passage whose entire channel converted to coastline.
                // the river art group still applies if the passage hugs a river
                for ( int ix = -1; ix <= 1 && iTyp == CoastGroupOcean; ix++ )
                    for ( int iy = -1; iy <= 1; iy++ )
                        if ( g.Type( x + ix, y + iy ) == TerrRiver )
                        {
                            iTyp = CoastGroupRiver;
                            break;
                        }

                switch ( iWet )
                {
                case 1:  iIndex = LandDn; break;   // dead-end stub openings
                case 2:  iIndex = LandLf; break;
                case 4:  iIndex = LandUp; break;
                case 8:  iIndex = LandRt; break;
                case 5:  iIndex = LandRt; break;   // strait (1-wide channel)
                case 10: iIndex = LandUp; break;
                case 7:  iIndex = LandLf; break;   // T-junction: land 1 side
                case 11: iIndex = LandDn; break;
                case 13: iIndex = LandRt; break;
                case 14: iIndex = LandUp; break;
                // adjacent pair = an elbow; the inside diagonal disambiguates
                case 3:  iIndex = wet( x + 1, y - 1 ) ? WaterUR : LandLL; break;
                case 6:  iIndex = wet( x + 1, y + 1 ) ? WaterLR : LandUL; break;
                case 9:  iIndex = wet( x - 1, y - 1 ) ? WaterUL : LandLR; break;
                case 12: iIndex = wet( x - 1, y + 1 ) ? WaterLL : LandUR; break;
                case 15:  // wet all around: land only on a diagonal - kiss it
                    iIndex = Island;
                    if ( !wet( x - 1, y - 1 ) )      iIndex = LandUL;
                    else if ( !wet( x + 1, y - 1 ) ) iIndex = LandUR;
                    else if ( !wet( x + 1, y + 1 ) ) iIndex = LandLR;
                    else if ( !wet( x - 1, y + 1 ) ) iIndex = LandLL;
                    break;
                }
            }
            else
            {
                // TIER 3: if no water touching it's an inside corner (original 1996 fallback)
                // see if we are a river coast
                for ( int ix = -1; ix <= 1; ix++ )
                    for ( int iy = -1; iy <= 1; iy++ )
                        if ( g.Type( x + ix, y + iy ) == TerrRiver )
                        {
                            iTyp = CoastGroupRiver;
                            break;
                        }

                switch ( iCoast )
                {
                case 3:  iIndex = WaterUR; break;
                case 6:  iIndex = WaterLR; break;
                case 7:  iIndex = LandLf;  break;   // above, right, & below
                case 11: iIndex = LandDn;  break;   // above, right, & left
                case 12: iIndex = WaterLL; break;
                case 13: iIndex = LandRt;  break;   // above, below, & left
                case 14: iIndex = LandUp;  break;   // right, below, and left
                case 9:  iIndex = WaterUL; break;
                default: iIndex = Island;  break;   // error
                }
            }

            // on refit keep the stored art group (ocean/lake/river)
            bool bAssign = true;
            if ( keepGroup )
            {
                int F = grid.CoastIndex( x, y );
                if ( F >= 0 && F <= 38 )
                    iTyp = ( F / 13 ) * 13;
                else
                    bAssign = false;    // overlay/unknown sprite: leave it alone
            }

            // assign the sprite
            if ( bAssign )
            {
                if ( grid.SetCoastIndex( x, y, iTyp + iIndex ) )
                    nChanged++;
                if ( grid.Alt( x, y ) < SeaLevel )  // if cause of riverbanks
                    grid.SetAlt( x, y, SeaLevel );
            }
        }

    return nChanged;
}

void RelabelLakeCoasts( ShoreGrid& grid, int x, int y )
{
    G g( grid );
    for ( int ix = -1; ix <= 1; ix++ )
        for ( int iy = -1; iy <= 1; iy++ )
        {
            int tx = g.wx( x + ix ), ty = g.wy( y + iy );
            if ( grid.Type( tx, ty ) == TerrCoastline )
            {
                int iOff = grid.CoastIndex( tx, ty );
                if ( ( iOff < CoastGroupLake ) || ( iOff >= CoastGroupRiver ) )
                    grid.SetCoastIndex( tx, ty, iOff + CoastGroupLake );
            }
        }
}

int RoadFacing( int iType )
{
    switch ( iType )
    {
    // case 0 - default
    case 0x1:  // above
    case 0x8:  // below
    case 0x9:  // above & below
        return 1;    // r_vert
    case 0x2:  // left
    case 0x4:  // right
    case 0x6:  // left & right
        return 0;    // r_horz
    case 0x3: return 7;    // r_l_lr
    case 0x5: return 8;    // r_l_ll
    case 0xA: return 6;    // r_l_ur
    case 0xC: return 9;    // r_l_ul
    case 0x7: return 4;    // r_t_dn
    case 0xB: return 3;    // r_t_rt
    case 0xD: return 5;    // r_t_lf
    case 0xE: return 2;    // r_t_up
    default:  return RoadX;   // no neighbors (0), all neighbors (0xF), bugs
    }
}

} // namespace enterrain
