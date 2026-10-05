// Civil Defence replication (CNetBldgDmgMult) on the SHIPPED code.
//
// run-bldg-dmg-mult-test.py extracts the message, its ctor, CNetCmd::FitsBuffer, the server's
// receive case, the Civil Defence helpers and the three CPlayer methods VERBATIM into the
// bdm_*.inc files below. Everything else here is scene: a fake CPlayer / theGame, a clock, and
// a "wire" that carries the posted bytes from the owner client to the server.
//
// Built with EN_BASELINE (pre-fix ref): no message exists, the old inline getter is used, and
// only the server-side multiplier checks run -- they fail, which is the bug.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using BOOL  = int;
using BYTE  = unsigned char;
using WORD  = unsigned short;
using DWORD = unsigned long;
constexpr BOOL TRUE = 1, FALSE = 0;
#ifndef NULL
#define NULL 0
#endif

enum { EDICT_MEAT_SHIELD = 3, EDICT_CIVIL_DEFENCE = 9 };

static DWORD g_dwNow = 5000;
static DWORD timeGetTime( ) { return g_dwNow; }

#ifndef EN_BASELINE
using COLORREF = DWORD;
#define ASSERT_STRICT( x ) ( (void)0 )
#define VP_TIMESTAMP 0
typedef WORD VPPLAYERID;
#pragma pack( push, 1 )
#include "bdm_hdr.inc"
class CPlayer;
#include "bdm_cmd.inc"
#include "bdm_msg.inc"
#include "bdm_stubs.inc"
#pragma pack( pop )
#include "bdm_civdef.inc"
#endif

// ---------------------------------------------------------------- scene: player + game
class CPlayer
{
  public:
    BOOL  m_bLocal = FALSE;
    int   m_iPlyrNum = 0;
    WORD  m_iNetNum = 0;
    DWORD m_dwEdicts = 0;
    float m_fEdictBldgDmgMult = 1.0f, m_fSurplusBldgDmgMult = 1.0f;
    int   m_iRemoteBldgDmgPermille = 1000, m_iSentBldgDmgPermille = -1;
    DWORD m_dwSentBldgDmgTime = 0;

    BOOL IsLocal( ) const { return m_bLocal; }
    int  GetPlyrNum( ) const { return m_iPlyrNum; }
    WORD GetNetNum( ) const { return m_iNetNum; }
    bool IsEdictActive( int id ) const { return ( m_dwEdicts & ( 1u << id ) ) != 0; }

#ifdef EN_BASELINE
#include "bdm_player.inc"
#else
    float GetEdictBldgDmgMult( ) const;
    void  ReportBldgDmgMult( );
    void  SetRemoteBldgDmgPermille( int iPermille );
#endif
};

struct CGame
{
    BOOL                           m_bNet = FALSE, m_bServer = TRUE;
    std::vector<CPlayer*>          m_aPlyrs;
    std::vector<std::vector<BYTE>> m_aWire;   // what PostToServer put on the wire

    BOOL     IsNetGame( ) const { return m_bNet; }
    BOOL     AmServer( ) const { return m_bServer; }
    CPlayer* _GetPlayerByPlyr( int iPlyrNum ) const
    {
        for ( CPlayer* p : m_aPlyrs )
            if ( p->m_iPlyrNum == iPlyrNum )
                return p;
        return NULL;
    }
    void PostToServer( const void* pMsg, int iLen )
    {
        const BYTE* pb = (const BYTE*)pMsg;
        m_aWire.push_back( std::vector<BYTE>( pb, pb + iLen ) );
    }
} theGame;

#ifndef EN_BASELINE
#include "bdm_netcmd.inc"
#include "bdm_player.inc"

static void ProcessMessage( CNetCmd* pCmd )
{
    switch ( pCmd->GetType( ) )
    {
#include "bdm_rx.inc"
    default:
        break;
    }
}

// Deliver one wire datagram to the "server" as the net layer would: stamped with the sender's
// connection id, and only if FitsBuffer accepts its length.
static void Deliver( const std::vector<BYTE>& v, WORD idFrom )
{
    std::vector<BYTE> buf( v );
    CNetCmd* pCmd = (CNetCmd*)buf.data( );
    pCmd->msgFrom = idFrom;
    if ( pCmd->FitsBuffer( (int)buf.size( ) ) )
        ProcessMessage( pCmd );
}
#endif

static bool Near( float a, float b ) { return std::fabs( a - b ) < 1e-5f; }

// ---------------------------------------------------------------- the server's view
// The remote owner's machine computes Civil Defence = 0.85 (Meat Shield 0.9 also on). On the
// server, that player's own ApplySurplusEdicts sees zero power and computes 1.0.
static void TestServerUsesRemoteValue( )
{
    std::printf( "[server] a remote owner's Civil Defence reaches the server's damage multiplier\n" );

    CPlayer owner;   // the owner's own client
    owner.m_bLocal = TRUE; owner.m_iPlyrNum = 2;
    owner.m_dwEdicts = ( 1u << EDICT_CIVIL_DEFENCE ) | ( 1u << EDICT_MEAT_SHIELD );
    owner.m_fEdictBldgDmgMult = 0.9f; owner.m_fSurplusBldgDmgMult = 0.85f;

    CPlayer remote;  // the same player as the server holds it
    remote.m_bLocal = FALSE; remote.m_iPlyrNum = 2; remote.m_iNetNum = 5;
    remote.m_dwEdicts = owner.m_dwEdicts;
    remote.m_fEdictBldgDmgMult = 0.9f; remote.m_fSurplusBldgDmgMult = 1.0f;

    const float fWant = 0.9f * 0.85f;

#ifdef EN_BASELINE
    theGame.m_bNet = TRUE; theGame.m_bServer = TRUE;
    CHECK( Near( remote.GetEdictBldgDmgMult( ), fWant ) );
#else
    // owner client posts its value
    theGame.m_bNet = TRUE; theGame.m_bServer = FALSE;
    theGame.m_aWire.clear( );
    owner.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 1 );

    // before delivery the server only has Meat Shield
    theGame.m_bServer = TRUE;
    theGame.m_aPlyrs = { &remote };
    CHECK( Near( remote.GetEdictBldgDmgMult( ), 0.9f ) );

    Deliver( theGame.m_aWire[0], 5 );
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 850 );
    CHECK( Near( remote.GetEdictBldgDmgMult( ), fWant ) );

    // Meat Shield is not in the message: switching it off on the server leaves Civil Defence alone
    remote.m_fEdictBldgDmgMult = 1.0f;
    CHECK( Near( remote.GetEdictBldgDmgMult( ), 0.85f ) );
    remote.m_fEdictBldgDmgMult = 0.9f;

    // the replicated edict bit gates it: Civil Defence off on the server -> no reduction
    remote.m_dwEdicts &= ~( 1u << EDICT_CIVIL_DEFENCE );
    CHECK( Near( remote.GetEdictBldgDmgMult( ), 0.9f ) );
    remote.m_dwEdicts |= ( 1u << EDICT_CIVIL_DEFENCE );
#endif
}

#ifndef EN_BASELINE
static std::vector<BYTE> Wire( int iPlyr, int iPermille )
{
    CPlayer p;
    p.m_iPlyrNum = iPlyr;
    CNetBldgDmgMult msg( &p, iPermille );
    const BYTE* pb = (const BYTE*)&msg;
    return std::vector<BYTE>( pb, pb + sizeof( msg ) );
}

static void TestWire( )
{
    std::printf( "[wire] layout, FitsBuffer\n" );
    static_assert( sizeof( CNetCmd ) == 12, "CNetCmd" );
    static_assert( sizeof( CNetBldgDmgMult ) == 20, "CNetBldgDmgMult" );
    CHECK_EQ( offsetof( CNetBldgDmgMult, m_iPlyrNum ), 12 );
    CHECK_EQ( offsetof( CNetBldgDmgMult, m_iPermille ), 16 );

    std::vector<BYTE> v = Wire( 3, 875 );
    int iPlyr, iPermille;
    std::memcpy( &iPlyr, &v[12], 4 );
    std::memcpy( &iPermille, &v[16], 4 );
    CHECK_EQ( v[offsetof( CNetCmd, m_bMsg )], CNetCmd::bldg_dmg_mult );
    CHECK_EQ( iPlyr, 3 );
    CHECK_EQ( iPermille, 875 );
    static_assert( CNetCmd::bldg_dmg_mult == CNetCmd::hex_retype + 1, "appended after hex_retype" );
    static_assert( CNetCmd::bldg_dmg_mult + 1 == CNetCmd::last_message, "last before last_message" );

    const CNetCmd* pCmd = (const CNetCmd*)v.data( );
    CHECK( pCmd->FitsBuffer( 20 ) );
    CHECK( pCmd->FitsBuffer( 64 ) );
    CHECK( !pCmd->FitsBuffer( 19 ) );
    CHECK( !pCmd->FitsBuffer( 16 ) );
    CHECK( !pCmd->FitsBuffer( 12 ) );
}

static void TestReceiveGuards( )
{
    std::printf( "[rx] sender, locality, server-only and range guards\n" );
    CPlayer remote;
    remote.m_iPlyrNum = 2; remote.m_iNetNum = 5;
    remote.m_dwEdicts = 1u << EDICT_CIVIL_DEFENCE;
    CPlayer host;
    host.m_bLocal = TRUE; host.m_iPlyrNum = 1; host.m_iNetNum = 2;
    host.m_dwEdicts = 1u << EDICT_CIVIL_DEFENCE; host.m_fSurplusBldgDmgMult = 0.95f;
    theGame.m_bNet = TRUE; theGame.m_bServer = TRUE;
    theGame.m_aPlyrs = { &host, &remote };

    Deliver( Wire( 2, 900 ), 6 );    // spoofed: another connection names player 2
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 1000 );
    Deliver( Wire( 2, 900 ), 5 );    // the right connection
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 900 );
    Deliver( Wire( 1, 800 ), 2 );    // the host's own (local) player: never overridden
    CHECK_EQ( host.m_iRemoteBldgDmgPermille, 1000 );
    CHECK( Near( host.GetEdictBldgDmgMult( ), 0.95f ) );
    Deliver( Wire( 9, 800 ), 5 );    // unknown player: dropped
    Deliver( Wire( 2, 0 ), 5 );      // more than the edict can give: clamped
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 800 );
    Deliver( Wire( 2, 5000 ), 5 );   // above "none": clamped
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 1000 );

    std::vector<BYTE> v = Wire( 2, 850 );
    v.resize( 16 );                   // truncated datagram: FitsBuffer refuses it
    Deliver( v, 5 );
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 1000 );

    theGame.m_bServer = FALSE;        // a non-server client ignores it
    Deliver( Wire( 2, 850 ), 5 );
    CHECK_EQ( remote.m_iRemoteBldgDmgPermille, 1000 );
}

static void TestSenderCadence( )
{
    std::printf( "[tx] owner sends on change, rate-limited; server and single player send nothing\n" );
    CPlayer owner;
    owner.m_bLocal = TRUE; owner.m_iPlyrNum = 2;
    owner.m_fSurplusBldgDmgMult = 1.0f;
    theGame.m_bNet = TRUE; theGame.m_bServer = FALSE;
    theGame.m_aWire.clear( );

    owner.ReportBldgDmgMult( );          // first pump of the game: always reports (1000)
    CHECK_EQ( theGame.m_aWire.size( ), 1 );
    owner.ReportBldgDmgMult( );          // unchanged: nothing
    CHECK_EQ( theGame.m_aWire.size( ), 1 );
    owner.m_fSurplusBldgDmgMult = 0.9f;  // changed within the interval: held
    g_dwNow += 200;
    owner.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 1 );
    g_dwNow += 1000;                     // interval passed: the pending value goes
    owner.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 2 );
    int iPermille;
    std::memcpy( &iPermille, &theGame.m_aWire[1][16], 4 );
    CHECK_EQ( iPermille, 900 );
    for ( int i = 0; i < 100; i++ )      // steady value over many pumps: nothing
        owner.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 2 );

    CPlayer other;                       // a non-local player on this client: never reported
    other.m_iPlyrNum = 3; other.m_fSurplusBldgDmgMult = 0.8f;
    other.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 2 );

    CPlayer hostMe;                      // the host's players: no message
    hostMe.m_bLocal = TRUE; hostMe.m_fSurplusBldgDmgMult = 0.8f;
    theGame.m_bServer = TRUE;
    hostMe.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 2 );

    // single player: no message, and every player's multiplier is the local formula
    theGame.m_bNet = FALSE; theGame.m_bServer = TRUE;
    CPlayer sp, ai;
    sp.m_bLocal = TRUE;
    sp.m_dwEdicts = ai.m_dwEdicts = ( 1u << EDICT_CIVIL_DEFENCE ) | ( 1u << EDICT_MEAT_SHIELD );
    sp.m_fEdictBldgDmgMult = ai.m_fEdictBldgDmgMult = 0.9f;
    sp.m_fSurplusBldgDmgMult = ai.m_fSurplusBldgDmgMult = 0.83f;
    ai.m_iRemoteBldgDmgPermille = 950;   // even a stray stored value is ignored
    sp.ReportBldgDmgMult( );
    ai.ReportBldgDmgMult( );
    CHECK_EQ( theGame.m_aWire.size( ), 2 );
    CHECK( sp.GetEdictBldgDmgMult( ) == 0.9f * 0.83f );
    CHECK( ai.GetEdictBldgDmgMult( ) == 0.9f * 0.83f );
}
#endif

int main( )
{
    TestServerUsesRemoteValue( );
#ifndef EN_BASELINE
    TestWire( );
    TestReceiveGuards( );
    TestSenderCadence( );
#endif
    return microtest::Summary( );
}
