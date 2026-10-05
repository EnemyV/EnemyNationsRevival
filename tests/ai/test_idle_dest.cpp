// IdleDestUsesMaterial (cairoute.cpp), the production text, against stock-shaped data.
//
// The per-unit build and repair inputs below are the stock ENATIONS.DAT tables
// (lumber, steel, Xil/copper, moly, goods per unit), summed per building exactly as
// sprtinit.cpp rolls up m_aiTotalInput / m_aiTotalRepair. The code-side cost tweaks
// sprtinit applies (infantry/rangers steel, artillery/cruiser steel and a Xil bump only
// where Xil is already non-zero) never move an input between zero and non-zero, so the
// predicate's answers are the shipped ones.

#include <cstdio>
#include <map>
#include <vector>

typedef int BOOL;
#define TRUE  1
#define FALSE 0

static int g_checks = 0, g_failures = 0, g_wrongAccessor = 0;
static void CHECK( bool good, const char* what )
{
    ++g_checks;
    if ( !good )
    {
        ++g_failures;
        std::printf( "FAIL: %s\n", what );
    }
}

struct CMaterialTypes
{
    enum { lumber, steel, copper, moly, goods, food, oil, gas, coal, iron, num_types, num_build_types = food };
};

struct CBuildVehicle
{
    int m_aiTotalInput[CMaterialTypes::num_build_types] = {};
    int GetTotalInput( int i ) const { return ( m_aiTotalInput[i] ); }
};
struct CBuildShipyard : CBuildVehicle
{
};
struct CBuildRepair
{
    int m_aiTotalRepair[CMaterialTypes::num_build_types] = {};
    int GetTotalRepair( int i ) const { return ( m_aiTotalRepair[i] ); }
};

// The union accessors ASSERT_STRICT on the union type in the game; here a mismatch is
// counted, so the predicate is also checked for asking each record the right way.
struct CStructureData
{
    enum BLDG_UNION_TYPE { UTmaterials, UTvehicle, UThousing, UTcommand, UTembassy, UTfort, UTpower, UTresearch,
                           UTrepair, UTwarehouse, UTmine, UTfarm, UTshipyard, num_union_types };
    BLDG_UNION_TYPE m_iUnionType = UTmaterials;
    CBuildVehicle   m_veh;
    CBuildShipyard  m_ship;
    CBuildRepair    m_rpr;

    BLDG_UNION_TYPE GetUnionType( ) const { return ( m_iUnionType ); }
    CBuildVehicle* GetBldVehicle( ) const
    {
        g_wrongAccessor += ( m_iUnionType != UTvehicle );
        return ( const_cast<CBuildVehicle*>( &m_veh ) );
    }
    CBuildShipyard* GetBldShipyard( ) const
    {
        g_wrongAccessor += ( m_iUnionType != UTshipyard );
        return ( const_cast<CBuildShipyard*>( &m_ship ) );
    }
    CBuildRepair* GetBldRepair( ) const
    {
        g_wrongAccessor += ( m_iUnionType != UTrepair );
        return ( const_cast<CBuildRepair*>( &m_rpr ) );
    }
};

struct CAIData
{
    std::map<int, CStructureData> m_map;
    CStructureData const* GetStructureData( int i ) const
    {
        auto it = m_map.find( i );
        return ( it == m_map.end( ) ? nullptr : &it->second );
    }
};
static CAIData  g_data;
static CAIData* pGameData = &g_data;

#include "idle_dest_actual.inc"

// ------------------------------------------------------------------ stock data

enum { light_0, light_1, light_2, heavy, barracks_2, repair, shipyard_1, shipyard_3, barracks_3, smelter, rocket,
       not_loaded };
static const char* const s_szName[] = { "light_0", "light_1",    "light_2",    "heavy",      "barracks_2", "repair",
                                        "shipyard_1", "shipyard_3", "barracks_3", "smelter",    "rocket" };

typedef std::vector<std::vector<int>> Units;   // per unit: lumber, steel, Xil, moly, goods

static void Add( int iType, CStructureData::BLDG_UNION_TYPE ut, Units const& units )
{
    CStructureData& sd = g_data.m_map[iType];
    sd.m_iUnionType    = ut;
    int* pTot = ( ut == CStructureData::UTrepair )     ? sd.m_rpr.m_aiTotalRepair
                : ( ut == CStructureData::UTshipyard ) ? sd.m_ship.m_aiTotalInput
                                                       : sd.m_veh.m_aiTotalInput;
    for ( auto const& u : units )
        for ( int i = 0; i < CMaterialTypes::num_build_types; ++i )
            pTot[i] += u[i];
}

int main( )
{
    Add( barracks_2, CStructureData::UTvehicle, { { 20, 90, 0, 0, 0 }, { 25, 135, 0, 0, 0 } } );
    Add( barracks_3, CStructureData::UTvehicle, { { 15, 60, 0, 0, 0 }, { 20, 90, 0, 0, 0 } } );
    Add( heavy, CStructureData::UTvehicle,
         { { 0, 1650, 8, 0, 0 }, { 90, 795, 0, 0, 0 }, { 0, 1215, 12, 0, 0 }, { 110, 1080, 0, 0, 0 }, { 0, 1150, 20, 0, 0 } } );
    Add( light_1, CStructureData::UTvehicle,
         { { 10, 120, 0, 0, 0 }, { 35, 240, 0, 0, 0 }, { 50, 975, 0, 0, 0 }, { 80, 570, 0, 0, 0 }, { 90, 865, 0, 0, 0 } } );
    Add( light_2, CStructureData::UTvehicle,
         { { 30, 205, 0, 0, 0 }, { 0, 1800, 10, 0, 0 }, { 45, 250, 0, 0, 0 }, { 100, 885, 0, 0, 0 },
           { 0, 1350, 15, 0, 0 }, { 120, 1200, 0, 0, 0 } } );
    Add( repair, CStructureData::UTrepair,
         { { 0, 640, 0, 0, 0 }, { 0, 340, 0, 0, 0 }, { 0, 415, 0, 0, 0 }, { 0, 450, 0, 0, 0 }, { 0, 415, 0, 0, 0 },
           { 0, 490, 0, 0, 0 }, { 0, 415, 0, 0, 0 }, { 0, 525, 0, 0, 0 }, { 0, 640, 0, 0, 0 }, { 0, 750, 5, 0, 0 },
           { 0, 565, 0, 0, 0 }, { 0, 715, 0, 0, 0 }, { 0, 865, 3, 0, 0 } } );
    Add( shipyard_1, CStructureData::UTshipyard, { { 45, 540, 0, 0, 0 }, { 90, 600, 0, 0, 0 }, { 45, 300, 0, 0, 0 } } );
    Add( shipyard_3, CStructureData::UTshipyard,
         { { 80, 480, 0, 0, 0 }, { 0, 1950, 20, 0, 0 }, { 0, 3750, 40, 0, 0 }, { 40, 240, 10, 0, 0 } } );
    Add( light_0, CStructureData::UTvehicle, { { 85, 825, 0, 0, 0 }, { 55, 525, 0, 0, 0 }, { 15, 180, 0, 0, 0 } } );
    Add( smelter, CStructureData::UTmaterials, {} );
    Add( rocket, CStructureData::UTwarehouse, {} );

    char sz[160];

    // IdleTruckTask's num_types destination set, and the copper verdict the stock data
    // gives each: only these four build or repair anything that costs Xil.
    const struct { int iType; bool bCopper; } aSet[] = {
        { light_0, false }, { light_1, false }, { light_2, true },     { heavy, true },
        { barracks_2, false }, { repair, true }, { shipyard_1, false }, { shipyard_3, true },
    };
    for ( auto const& e : aSet )
    {
        std::snprintf( sz, sizeof( sz ), "%s %s copper", s_szName[e.iType], e.bCopper ? "uses" : "never uses" );
        CHECK( ( IdleDestUsesMaterial( e.iType, CMaterialTypes::copper ) != FALSE ) == e.bCopper, sz );

        // Steel is in every unit's bill, so every one of them takes steel.
        std::snprintf( sz, sizeof( sz ), "%s uses steel", s_szName[e.iType] );
        CHECK( IdleDestUsesMaterial( e.iType, CMaterialTypes::steel ) == TRUE, sz );

        // Materials no CBuildUnit has a slot for are never a destination's input.
        for ( int iMat = CMaterialTypes::num_build_types; iMat < CMaterialTypes::num_types; ++iMat )
        {
            std::snprintf( sz, sizeof( sz ), "%s: non-build material %d is never an input", s_szName[e.iType], iMat );
            CHECK( IdleDestUsesMaterial( e.iType, iMat ) == FALSE, sz );
        }
        CHECK( IdleDestUsesMaterial( e.iType, -1 ) == FALSE, "a negative material index is refused" );
    }

    CHECK( IdleDestUsesMaterial( repair, CMaterialTypes::lumber ) == FALSE, "repair bays use no lumber (stock data)" );
    CHECK( IdleDestUsesMaterial( barracks_3, CMaterialTypes::copper ) == FALSE, "barracks_3 never uses copper" );
    CHECK( IdleDestUsesMaterial( smelter, CMaterialTypes::steel ) == FALSE, "a materials building is not a destination" );
    CHECK( IdleDestUsesMaterial( rocket, CMaterialTypes::copper ) == FALSE, "a warehouse-type record is not a destination" );
    CHECK( IdleDestUsesMaterial( not_loaded, CMaterialTypes::copper ) == FALSE, "a type with no data is refused" );
    CHECK( g_wrongAccessor == 0, "each record is read through the accessor its union type allows" );

    std::printf( "%d checks, %d failures\n", g_checks, g_failures );
    return ( g_failures ? 1 : 0 );
}
