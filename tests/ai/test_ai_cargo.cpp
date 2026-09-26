// test_ai_cargo.cpp -- carrier capacity, cargo cost and manifest packing for
// the AI's amphibious wave.
//
//  (1) the shipped enaicargo helper (aicargo.h, #included directly, not a copy);
//  (2) a source lint over caitmgr.cpp / caigmgr.cpp so the AI's capacity gates
//      cannot go back to counting objects against MAX_CARGO.
//
// Engine contract (LoadCarrier in netapi.cpp, CVehicle::GetEffPeopleCarry):
//     cost = IsPeople() ? 1 : MAX_CARGO
//     reject if m_iCargoSize + cost > GetEffPeopleCarry()
//     GetEffPeopleCarry = GetPeopleCarry + LCbonus * MAX_CARGO
//
// Usage: test_ai_cargo.exe [caitmgr.cpp] [caigmgr.cpp]
// Exit: 0 all pass, 1 a check failed, 2 a named source file could not be read.

#define _CRT_SECURE_NO_WARNINGS

#include "microtest.h"

#include "../../enations_latest/src/aicargo.h"

#include <cstdio>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// part 1 -- the shipped arithmetic
// ---------------------------------------------------------------------------

static const int SLOTS = enaicargo::kSlotsPerVehicle;  // 5

static void TestCargoCost()
{
    // the engine's two prices, and nothing in between
    CHECK_EQ( enaicargo::CargoCost( true ), 1 );
    CHECK_EQ( enaicargo::CargoCost( false ), SLOTS );
}

static void TestFreeSlots()
{
    CHECK_EQ( enaicargo::FreeSlots( 0, 5 ), 5 );
    CHECK_EQ( enaicargo::FreeSlots( 3, 5 ), 2 );
    CHECK_EQ( enaicargo::FreeSlots( 5, 5 ), 0 );
    // an over-full carrier reports no room, never a negative budget that would
    // make the next comparison pass
    CHECK_EQ( enaicargo::FreeSlots( 7, 5 ), 0 );
    CHECK_EQ( enaicargo::FreeSlots( -1, 5 ), 5 );
    CHECK_EQ( enaicargo::FreeSlots( 0, 0 ), 0 );
}

static void TestHasRoomFor()
{
    // --- a BASE landing craft (capacity 5) holds exactly one med_tank ---
    CHECK( enaicargo::HasRoomFor( 0, 5, enaicargo::CargoCost( false ) ) );
    CHECK( !enaicargo::HasRoomFor( 5, 5, enaicargo::CargoCost( false ) ) );
    // and it cannot take a single extra ranger once that tank is aboard
    CHECK( !enaicargo::HasRoomFor( 5, 5, enaicargo::CargoCost( true ) ) );

    // --- a teched craft (LC2+LC3 => 5 + 2*5 = 15) holds 15 rangers ---
    CHECK( enaicargo::HasRoomFor( 4, 15, enaicargo::CargoCost( true ) ) );   // 5th ranger
    CHECK( enaicargo::HasRoomFor( 14, 15, enaicargo::CargoCost( true ) ) );  // 15th ranger
    CHECK( !enaicargo::HasRoomFor( 15, 15, enaicargo::CargoCost( true ) ) );
    CHECK( enaicargo::HasRoomFor( 10, 15, enaicargo::CargoCost( false ) ) );  // 3rd tank fits exactly
    CHECK( !enaicargo::HasRoomFor( 11, 15, enaicargo::CargoCost( false ) ) );

    // --- exact capacity ---
    CHECK( enaicargo::HasRoomFor( 5, 10, enaicargo::CargoCost( false ) ) );
    CHECK( !enaicargo::HasRoomFor( 6, 10, enaicargo::CargoCost( false ) ) );

    // --- fragmented capacity: four free slots take rangers but not a tank ---
    CHECK( enaicargo::HasRoomFor( 11, 15, enaicargo::CargoCost( true ) ) );
    CHECK( !enaicargo::HasRoomFor( 11, 15, enaicargo::CargoCost( false ) ) );

    // --- zero capacity: nothing is loadable, including infantry ---
    CHECK( !enaicargo::HasRoomFor( 0, 0, enaicargo::CargoCost( true ) ) );
    CHECK( !enaicargo::HasRoomFor( 0, 0, enaicargo::CargoCost( false ) ) );

    // --- reservations: slots promised to cargo that is not aboard yet ---
    // one tank aboard, one tank's worth promised, on a 15-slot craft: a third
    // tank still fits, a fourth does not
    CHECK( enaicargo::HasRoomFor( 5, 15, SLOTS, SLOTS ) );
    CHECK( !enaicargo::HasRoomFor( 10, 15, SLOTS, SLOTS ) );
    // a reservation must never be counted for cargo already aboard -- that is
    // the caller's contract, but a negative reservation must not buy room back
    CHECK( !enaicargo::HasRoomFor( 5, 5, SLOTS, -100 ) );

    // a zero/negative cost is not "free", it is a caller bug: refuse it
    CHECK( !enaicargo::HasRoomFor( 0, 15, 0 ) );
    CHECK( !enaicargo::HasRoomFor( 0, 15, -3 ) );
}

static void TestVehiclesPerCarrier()
{
    CHECK_EQ( enaicargo::VehiclesPerCarrier( 0 ), 0 );
    CHECK_EQ( enaicargo::VehiclesPerCarrier( 4 ), 0 );  // cannot take a vehicle at all
    CHECK_EQ( enaicargo::VehiclesPerCarrier( 5 ), 1 );
    CHECK_EQ( enaicargo::VehiclesPerCarrier( 9 ), 1 );
    CHECK_EQ( enaicargo::VehiclesPerCarrier( 10 ), 2 );
    CHECK_EQ( enaicargo::VehiclesPerCarrier( 15 ), 3 );
}

// Independent check of CarriersNeeded: pack the manifest into craft greedily
// and count them (shares no code with the helper).
static int PackAndCount( int nVehicles, int nPeople, int iCapacity )
{
    if ( iCapacity <= 0 )
        return ( ( nVehicles || nPeople ) ? -1 : 0 );
    if ( nVehicles && iCapacity < SLOTS )
        return ( -1 );

    std::vector<int> craft;  // free slots in each open craft
    for ( int i = 0; i < nVehicles; ++i )
    {
        size_t iFit = craft.size( );
        for ( size_t j = 0; j < craft.size( ); ++j )
            if ( craft[j] >= SLOTS ) { iFit = j; break; }
        if ( iFit == craft.size( ) )
            craft.push_back( iCapacity );
        craft[iFit] -= SLOTS;
    }
    for ( int i = 0; i < nPeople; ++i )
    {
        size_t iFit = craft.size( );
        for ( size_t j = 0; j < craft.size( ); ++j )
            if ( craft[j] >= 1 ) { iFit = j; break; }
        if ( iFit == craft.size( ) )
            craft.push_back( iCapacity );
        craft[iFit] -= 1;
    }
    return ( (int)craft.size( ) );
}

static void TestCarriersNeeded()
{
    // nothing to carry is not the same answer as cannot carry
    CHECK_EQ( enaicargo::CarriersNeeded( 0, 0, 5 ), 0 );
    CHECK_EQ( enaicargo::CarriersNeeded( 0, 0, 0 ), 0 );
    CHECK_EQ( enaicargo::CarriersNeeded( 1, 0, 0 ), -1 );
    CHECK_EQ( enaicargo::CarriersNeeded( 0, 1, 0 ), -1 );
    // a craft too small for a vehicle can never lift one, however many there are
    CHECK_EQ( enaicargo::CarriersNeeded( 1, 0, 4 ), -1 );
    CHECK_EQ( enaicargo::CarriersNeeded( 0, 4, 4 ), 1 );

    // --- the shipped wave shape ---
    // base craft (capacity 5): 3 tanks + 3 rangers needs 4 craft; each tank
    // fills a craft whole and the rangers need a 4th.
    CHECK_EQ( enaicargo::CarriersNeeded( 3, 3, 5 ), 4 );
    // teched craft (capacity 15): 3 tanks fill one craft exactly, so the
    // rangers need a second
    CHECK_EQ( enaicargo::CarriersNeeded( 3, 3, 15 ), 2 );

    // --- packing, not aggregate capacity ---
    // 2 tanks on capacity-5 craft: 2 craft, and their aggregate free space (0)
    // is honest; but 2 tanks + 0 rangers on capacity-9 craft is still 2 craft
    // even though 2*9 = 18 >= 2*5, because one craft holds only one tank
    CHECK_EQ( enaicargo::CarriersNeeded( 2, 0, 9 ), 2 );
    // and the slack in those two craft (18 - 10 = 8) lifts 8 rangers free
    CHECK_EQ( enaicargo::CarriersNeeded( 2, 8, 9 ), 2 );
    CHECK_EQ( enaicargo::CarriersNeeded( 2, 9, 9 ), 3 );

    // --- capacity upgrade during assembly shrinks the requirement ---
    CHECK( enaicargo::CarriersNeeded( 4, 4, 5 ) > enaicargo::CarriersNeeded( 4, 4, 10 ) );
    CHECK( enaicargo::CarriersNeeded( 4, 4, 10 ) >= enaicargo::CarriersNeeded( 4, 4, 15 ) );

    // --- agreement with an independent greedy pack, over the whole space ---
    const int aCaps[] = { 1, 4, 5, 6, 9, 10, 15, 20 };
    for ( int c = 0; c < (int)( sizeof( aCaps ) / sizeof( aCaps[0] ) ); ++c )
        for ( int v = 0; v <= 8; ++v )
            for ( int p = 0; p <= 12; ++p )
                CHECK_EQ( enaicargo::CarriersNeeded( v, p, aCaps[c] ), PackAndCount( v, p, aCaps[c] ) );

    // --- a computed answer must actually fit the manifest ---
    for ( int c = 0; c < (int)( sizeof( aCaps ) / sizeof( aCaps[0] ) ); ++c )
        for ( int v = 0; v <= 6; ++v )
            for ( int p = 0; p <= 10; ++p )
            {
                int n = enaicargo::CarriersNeeded( v, p, aCaps[c] );
                if ( n <= 0 )
                    continue;
                // total weight fits, AND every vehicle has a whole craft-share
                CHECK( v * SLOTS + p <= n * aCaps[c] );
                CHECK( v <= n * enaicargo::VehiclesPerCarrier( aCaps[c] ) );
            }

    // negatives are clamped, not propagated into a bogus craft count
    CHECK_EQ( enaicargo::CarriersNeeded( -3, -3, 5 ), 0 );
    CHECK_EQ( enaicargo::CarriersNeeded( -3, 5, 5 ), 1 );
}

// ---------------------------------------------------------------------------
// part 2 -- source lint: the AI's gates must not count objects again
// ---------------------------------------------------------------------------

static bool ReadFile( const char* szPath, std::string& out )
{
    std::FILE* f = std::fopen( szPath, "rb" );
    if ( f == NULL )
        return ( false );
    char   buf[8192];
    size_t n;
    while ( ( n = std::fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        out.append( buf, n );
    std::fclose( f );
    return ( true );
}

static int CountOccurrences( const std::string& hay, const char* szNeedle )
{
    int         n   = 0;
    std::string ndl = szNeedle;
    for ( size_t at = hay.find( ndl ); at != std::string::npos; at = hay.find( ndl, at + ndl.size( ) ) )
        ++n;
    return ( n );
}

static void LintTaskMgr( const std::string& src )
{
    // the three loading gates are now weight-based
    CHECK( CountOccurrences( src, "enaicargo::HasRoomFor" ) >= 3 );
    CHECK( CountOccurrences( src, "#include \"aicargo.h\"" ) == 1 );

    // ...and none of them counts objects against MAX_CARGO any more
    CHECK_EQ( CountOccurrences( src, "GetCargoCount( ) < MAX_CARGO" ), 0 );
    CHECK_EQ( CountOccurrences( src, "GetCargoCount( ) >= MAX_CARGO" ), 0 );

    // the mirrored constant is pinned to base.h on the game side
    CHECK( CountOccurrences( src, "enaicargo::kSlotsPerVehicle == MAX_CARGO" ) == 1 );

}

static void LintGoalMgr( const std::string& src )
{
    CHECK( CountOccurrences( src, "#include \"aicargo.h\"" ) == 1 );
    // landing craft and IFV demand are packed, not divided
    CHECK( CountOccurrences( src, "enaicargo::CarriersNeeded" ) >= 2 );
    CHECK_EQ( CountOccurrences( src, "i /= MAX_CARGO;" ), 0 );
    CHECK_EQ( CountOccurrences( src, "GetTaskParam( CAI_TF_INFANTRY ) / MAX_CARGO" ), 0 );

    // the SEAINVADE armor bucket asks only for what LoadCargo will load
    size_t at = src.find( "else if ( pTask->GetGoalID( ) == IDG_SEAINVADE )" );
    CHECK( at != std::string::npos );
    if ( at != std::string::npos )
    {
        //  bound the window by the end of the function, so added comments
        //  cannot push the checked lines out of it
        size_t      end   = src.find( "int CAIGoalMgr::StageGoalIdx", at );
        CHECK( end != std::string::npos );
        std::string block = src.substr( at, ( end == std::string::npos ? 1400 : end - at ) );
        CHECK( block.find( "awTypes[CTransportData::med_tank] = 1;" ) != std::string::npos );
        CHECK( block.find( "awTypes[CTransportData::light_tank]" ) == std::string::npos );
        CHECK( block.find( "awTypes[CTransportData::light_art]" ) == std::string::npos );

        //  the escort bucket (CAI_TF_SHIPS, "cruiser,destroyer,gun_boat") must
        //  stage all three hulls: awTypes is the gate GetProductionTask checks,
        //  and the AI must never end up building landing craft only
        CHECK( block.find( "awTypes[CTransportData::gun_boat] = 1;" ) != std::string::npos );
        CHECK( block.find( "awTypes[CTransportData::destroyer] = 1;" ) != std::string::npos );
        CHECK( block.find( "awTypes[CTransportData::cruiser] = 1;" ) != std::string::npos );
    }

    //  both consumers of m_pwaVehGoals (GetProductionTask and the
    //  production-priority setter) accept UTvehicle and UTshipyard
    CHECK( CountOccurrences( src, "m_pwaUnits[iVeh] > m_pwaVehGoals[iVeh]" ) >= 2 );
    CHECK( CountOccurrences( src, "CStructureData::UTshipyard" ) >= 2 );
}

int main( int argc, char** argv )
{
    TestCargoCost( );
    TestFreeSlots( );
    TestHasRoomFor( );
    TestVehiclesPerCarrier( );
    TestCarriersNeeded( );

    for ( int i = 1; i < argc; ++i )
    {
        std::string src;
        if ( !ReadFile( argv[i], src ) )
        {
            std::printf( "[ai_cargo] cannot open %s\n", argv[i] );
            return ( 2 );
        }
        std::string path = argv[i];
        if ( path.find( "caitmgr" ) != std::string::npos )
            LintTaskMgr( src );
        else if ( path.find( "caigmgr" ) != std::string::npos )
            LintGoalMgr( src );
        else
        {
            std::printf( "[ai_cargo] unexpected source argument %s\n", argv[i] );
            return ( 2 );
        }
    }

    return ( microtest::Summary( ) );
}
