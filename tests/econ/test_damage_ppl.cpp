// Building damage multipliers and the population floor, on the SHIPPED bodies.
//
// run-damage-ppl-test.py extracts CUnit::DecDamagePoints, its unit.h declaration, the
// UnitDamage / UnitSetDamage net handlers and the CPlayer inline bodies VERBATIM into the
// dmg_*.inc files included below. Everything in this file is scene: fake players, units,
// net messages and the theApp / theGame / theAreaList objects those bodies touch.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>
#include <vector>

using BOOL  = int;
using DWORD = unsigned long;
constexpr BOOL TRUE = 1, FALSE = 0;
#ifndef NULL
#define NULL 0
#endif
#define ASSERT_STRICT_VALID( p ) ( (void)0 )
#define NUM_FRAMES_SHOW_HIT 4
enum { EVENT_BLDG_HURTING = 1, EVENT_BLDG_DYING, EVENT_NOTIFY };
static DWORD timeGetTime( ) { return 1234; }
#ifndef __max
#define __max( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

struct CHexCoord { int x = 0, y = 0; };

// ---------------------------------------------------------------- research + player
struct CRsrchStatus { BOOL m_bDiscovered = FALSE; };
struct CRsrchArray { enum { bldg_armor = 3, bldg_armor_2 = 6, bldg_armor_3 = 7, num_topics = 8 }; };
struct CRsrchVec
{
    std::vector<CRsrchStatus> v = std::vector<CRsrchStatus>( CRsrchArray::num_topics );
    int GetSize( ) const { return (int)v.size( ); }
};

class CPlayer
{
public:
    BOOL      m_bMe = FALSE, m_bAI = FALSE;
    float     m_fEdictBldgDmgMult = 1.0f, m_fSurplusBldgDmgMult = 1.0f;
    CRsrchVec m_aRsrch;
    int       m_iPplBldg = 0, m_iPplNeedBldg = 1;
    float     m_fPplMult = 1.0f;

    BOOL          IsMe( ) const { return m_bMe; }
    BOOL          IsAI( ) const { return m_bAI; }
    CRsrchStatus& GetRsrch( int i ) { return m_aRsrch.v[i]; }
    float GetEdictBldgDmgMult( ) const { return ( m_fEdictBldgDmgMult * m_fSurplusBldgDmgMult ); }
    void  SetArmorTier( int iTier )
    {
        GetRsrch( CRsrchArray::bldg_armor ).m_bDiscovered   = iTier >= 1;
        GetRsrch( CRsrchArray::bldg_armor_2 ).m_bDiscovered = iTier >= 2;
        GetRsrch( CRsrchArray::bldg_armor_3 ).m_bDiscovered = iTier >= 3;
    }

#include "dmg_player.inc"
    void UpdatePplMult( )
    {
#include "dmg_pplmult.inc"
    }
};

// ---------------------------------------------------------------- units
struct CUnitData { int m_iDP = 1000; int GetDamagePoints( ) const { return m_iDP; } };
struct CStructureData : CUnitData { int m_iTB = 100; int GetTimeBuild( ) const { return m_iTB; } };

class CUnit
{
public:
    enum UNIT_TYPE { building, vehicle };
    enum { destroying = 0x10, unit_set_damage = 0x200 };

    UNIT_TYPE  m_iUnitType = vehicle;
    CPlayer*   m_pOwner = nullptr;
    CUnitData* m_pData = nullptr;
    DWORD      m_dwID = 0, m_dwFlags = 0, m_dwHitFlash = 0, m_dwLastShooter = 0;
    int        m_iDamagePoints = 1000, m_iDamagePer = 100, m_iFrameHit = 0, m_iDied = 0;
    float      m_fDamageMult = 1.0f, m_fDamPerfMult = 1.0f;

    virtual ~CUnit( ) { }
    UNIT_TYPE  GetUnitType( ) const { return m_iUnitType; }
    CPlayer*   GetOwner( ) const { return m_pOwner; }
    CUnitData* GetData( ) const { return m_pData; }
    int        GetDamagePoints( ) const { return m_iDamagePoints; }
    BOOL       IsFlag( DWORD f ) const { return ( m_dwFlags & f ) != 0; }
    void       PrepareToDie( DWORD ) { m_iDied++; }
    void       UpdateDamageLevel( ) { }
    void       SetUnitSetDamage( CUnit* pKiller )
    {
        m_dwLastShooter = pKiller ? pKiller->m_dwID : 0;
        m_dwFlags |= unit_set_damage;
    }

#include "dmg_decl.inc"
};

class CBuilding : public CUnit
{
public:
    CStructureData m_sd;
    int            m_iConstDone = -1, m_iFoundTime = 0;
    CBuilding( ) { m_iUnitType = building; m_pData = &m_sd; }
    CStructureData* GetData( ) { return &m_sd; }
    CHexCoord       GetHex( ) const { return CHexCoord( ); }
    BOOL            IsLive( ) const { return TRUE; }
};

class CVehicle : public CUnit
{
public:
    CUnitData m_ud;
    CVehicle( ) { m_iUnitType = vehicle; m_pData = &m_ud; }
};

// ---------------------------------------------------------------- the world the bodies touch
struct { struct { void SetBldgHit( ) { } void SetVehHit( ) { } } m_wndWorld; } theApp;
struct { void SetLastAttack( CHexCoord ) { } } theAreaList;
struct
{
    int  m_iAiPosts = 0;
    void Event( int, int, CUnit* ) { }
    void PostToClient( CPlayer*, const void*, int ) { m_iAiPosts++; }
} theGame;

struct CMsgUnitDamage
{
    DWORD m_dwIDTarget = 0; int m_iPlyrTarget = 0; DWORD m_dwIDDamage = 0; int m_iPlyrDamage = 0;
    DWORD m_dwIDShoot = 0;  int m_iPlyrShoot = 0;  int m_iDamageShot = 0;
};
struct CMsgUnitSetDamage { DWORD m_dwIDDamage = 0, m_dwIDShoot = 0; int m_iDamageLevel = 0; };

// One "machine": the unit table GetUnit( ) resolves against.
static std::vector<CUnit*>* g_pWorld = nullptr;
static CUnit* GetUnit( DWORD dwID )
{
    for ( CUnit* p : *g_pWorld )
        if ( p->m_dwID == dwID )
            return p;
    return NULL;
}

#include "dmg_unit.inc"
#include "dmg_net.inc"

// ---------------------------------------------------------------- helpers
// Server-side hit, exactly as the server takes it: UnitDamage -> DecDamagePoints.
static int ServerHit( CUnit& u, int iShot )
{
    std::vector<CUnit*> world { &u };
    g_pWorld = &world;
    CMsgUnitDamage msg;
    msg.m_dwIDDamage  = u.m_dwID;
    msg.m_iDamageShot = iShot;
    int iBefore = u.m_iDamagePoints;
    UnitDamage( &msg );
    return iBefore - u.m_iDamagePoints;
}

// ---------------------------------------------------------------- fix 1: Blast Shielding
static void TestBldgArmor( )
{
    std::printf( "[armor] building armor research reduces building damage taken\n" );
    static const int aiExpect[4] = { 100, 80, 78, 76 };
    for ( int iTier = 0; iTier <= 3; iTier++ )
    {
        CPlayer plyr;
        plyr.SetArmorTier( iTier );
        CBuilding b;
        b.m_dwID = 1; b.m_pOwner = &plyr;
        CHECK_EQ( ServerHit( b, 100 ), aiExpect[iTier] );
    }

    // stacks after Meat Shield (0.9): 100 -> 90 -> 72
    {
        CPlayer plyr;
        plyr.SetArmorTier( 1 );
        plyr.m_fEdictBldgDmgMult = 0.9f;
        CBuilding b;
        b.m_dwID = 1; b.m_pOwner = &plyr;
        CHECK_EQ( ServerHit( b, 100 ), 72 );
    }

    // vehicles are untouched
    {
        CPlayer plyr;
        plyr.SetArmorTier( 3 );
        CVehicle v;
        v.m_dwID = 2; v.m_pOwner = &plyr;
        CHECK_EQ( ServerHit( v, 100 ), 100 );
    }

    // repairs (negative damage) are untouched
    {
        CPlayer plyr;
        plyr.SetArmorTier( 3 );
        CBuilding b;
        b.m_dwID = 1; b.m_pOwner = &plyr; b.m_iDamagePoints = 500;
        b.DecDamagePoints( -100 );
        CHECK_EQ( b.m_iDamagePoints, 600 );
    }

    // a lethal hit still kills
    {
        CPlayer plyr;
        plyr.SetArmorTier( 3 );
        CBuilding b;
        b.m_dwID = 1; b.m_pOwner = &plyr; b.m_iDamagePoints = 50;
        ServerHit( b, 100 );
        CHECK_EQ( b.m_iDamagePoints, 0 );
        CHECK_EQ( b.m_iDied, 1 );
    }
}

// ---------------------------------------------------------------- fix 2: set-damage is absolute
// The server takes the hit (UnitDamage) and broadcasts the building's absolute level; a remote
// client (the owner, here) applies it through UnitSetDamage. The client copy must land on
// exactly the server's level, not re-apply construction scaling or the owner's multipliers.
static void ClientSet( CUnit& u, int iLevel )
{
    std::vector<CUnit*> world { &u };
    g_pWorld = &world;
    CMsgUnitSetDamage msg;
    msg.m_dwIDDamage   = u.m_dwID;
    msg.m_iDamageLevel = iLevel;
    UnitSetDamage( &msg );
}

static void SetupPair( CPlayer& srv, CPlayer& cli, CBuilding& bSrv, CBuilding& bCli,
                       float fEdict, int iArmor, int iConstDone )
{
    srv.m_fEdictBldgDmgMult = cli.m_fEdictBldgDmgMult = fEdict;
    srv.SetArmorTier( iArmor );
    cli.SetArmorTier( iArmor );
    cli.m_bMe = TRUE;
    bSrv.m_dwID = bCli.m_dwID = 7;
    bSrv.m_pOwner = &srv;
    bCli.m_pOwner = &cli;
    bSrv.m_iConstDone = bCli.m_iConstDone = iConstDone;
}

static void TestSetDamageAbsolute( )
{
    std::printf( "[setdmg] owner client lands on the server's absolute level\n" );
    struct Case { float fEdict; int iArmor; int iConstDone; int iShot; } aCases[] = {
        { 0.9f, 0, -1, 100 },   // Meat Shield only (pre-existing double multiply)
        { 1.0f, 1, -1, 100 },   // Blast Shielding only
        { 0.9f, 3, -1, 137 },   // both
        { 1.0f, 0, 50, 100 },   // under construction: server scales x3, client must not again
        { 0.9f, 2, 50, 40 },    // construction and both multipliers
        { 1.0f, 0, -1, 0 },     // no damage: nothing moves
    };
    for ( const Case& c : aCases )
    {
        CPlayer   srv, cli;
        CBuilding bSrv, bCli;
        SetupPair( srv, cli, bSrv, bCli, c.fEdict, c.iArmor, c.iConstDone );
        ServerHit( bSrv, c.iShot );
        CHECK( bSrv.IsFlag( CUnit::unit_set_damage ) || ( c.iShot == 0 ) );
        ClientSet( bCli, bSrv.m_iDamagePoints );
        CHECK_EQ( bCli.m_iDamagePoints, bSrv.m_iDamagePoints );
    }

    // level ABOVE the client's (server repaired it): land on it, do not scale the gain
    {
        CPlayer   srv, cli;
        CBuilding bSrv, bCli;
        SetupPair( srv, cli, bSrv, bCli, 0.9f, 3, -1 );
        bCli.m_iDamagePoints = 600;
        ClientSet( bCli, 800 );
        CHECK_EQ( bCli.m_iDamagePoints, 800 );
    }

    // repeated hits do not drift
    {
        CPlayer   srv, cli;
        CBuilding bSrv, bCli;
        SetupPair( srv, cli, bSrv, bCli, 0.9f, 1, -1 );
        for ( int i = 0; i < 10; i++ )
        {
            ServerHit( bSrv, 50 );
            ClientSet( bCli, bSrv.m_iDamagePoints );
        }
        CHECK_EQ( bCli.m_iDamagePoints, bSrv.m_iDamagePoints );
        CHECK_EQ( bCli.m_iDied, bSrv.m_iDied );
    }

    // a level of 0 still kills the client copy
    {
        CPlayer   srv, cli;
        CBuilding bSrv, bCli;
        SetupPair( srv, cli, bSrv, bCli, 0.9f, 1, -1 );
        ClientSet( bCli, 0 );
        CHECK_EQ( bCli.m_iDamagePoints, 0 );
        CHECK_EQ( bCli.m_iDied, 1 );
    }
}

// ---------------------------------------------------------------- fix 3: population floor
// CBuilding's destructor / RemoveUnit release a lost building's staff with
// AddPplBldg( -( GetPplMult( ) * GetPeople( ) ) ), foundations included, so a wiped colony
// could subtract more people than it had.
static void LoseBuilding( CPlayer& p, int iPeople )
{
    p.AddPplBldg( -(int)( p.m_fPplMult * iPeople ) );
}

static void TestPplFloor( )
{
    std::printf( "[ppl] building population never drops below 1\n" );

    // ordinary adds/subtracts that stay >= 1 are untouched
    {
        CPlayer p;
        p.m_iPplBldg = 100;
        p.AddPplBldg( -30 );
        CHECK_EQ( p.m_iPplBldg, 70 );
        p.AddPplBldg( 5 );
        CHECK_EQ( p.m_iPplBldg, 75 );
        p.AddPplBldg( -74 );
        CHECK_EQ( p.m_iPplBldg, 1 );
    }

    // colony wiped: 100 people, 90 needed by running buildings, plus 50-people foundations
    {
        CPlayer p;
        p.m_iPplBldg     = 100;
        p.m_iPplNeedBldg = 90;
        p.UpdatePplMult( );
        CHECK( p.m_fPplMult == 1.0f );
        LoseBuilding( p, 60 );   // running
        LoseBuilding( p, 30 );   // running
        LoseBuilding( p, 25 );   // foundation, nobody employed
        LoseBuilding( p, 25 );   // foundation, nobody employed
        CHECK_EQ( p.m_iPplBldg, 1 );

        // rebuilding: the workforce ratio must not go negative (negative production)
        p.m_iPplNeedBldg = 30;
        p.UpdatePplMult( );
        CHECK( p.m_fPplMult > 0.0f );
    }
}

int main( )
{
    TestBldgArmor( );
    TestSetDamageAbsolute( );
    TestPplFloor( );
    return microtest::Summary( );
}
