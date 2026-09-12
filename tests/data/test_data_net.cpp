// test_data_net.cpp -- the CNetJoin / CNetPublish data-hash fields (015 phase 3).
//
// netapi.h cannot be compiled standalone (it pulls the vdmplay headers and the
// game's player/create stack), so the two records are MIRRORED here field for
// field. The mirror is not trusted on its own: netapi.cpp carries
//
//     static_assert( sizeof( CNetPublish ) == 44, ... );
//     static_assert( sizeof( CNetJoin )    == 20, ... );
//
// with the numbers this fixture prints, so a layout change breaks the GAME
// build, not just this file. What is checked here is the part a static_assert
// cannot state: where the new field sits, that m_iLen counts it, that the
// header still fits the fixed vdmplay copy with room for the variable tail, and
// that the server's minimum-length rule accepts a current record and rejects a
// pre-015 one.

#include "../ai/microtest.h"
#include "../../enations_latest/src/datahashguard.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace {

typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef int            BOOL;

// Comparisons of two compile-time constants trip /W4's C4127, and the point
// here is the RELATION, not a constant expression. One opaque helper keeps the
// intent readable and the build quiet.
bool Lt( size_t a, size_t b ) { return a < b; }

// vdmplay copies the SESSION record as a fixed 512-byte block and the PLAYER
// record as a fixed 256-byte one (015 plan section 3c: the netapi.cpp comment
// has the two crossed).
const int kSessionBlock = 512;
const int kPlayerBlock  = 256;

//---------------------------------------------------------------------------
//  Pre-015 layouts, for the "what did it cost" numbers.
//---------------------------------------------------------------------------

struct OldNetPublish
{
    int  m_iLen;
    int  m_iNumOpponents;
    int  m_iAIlevel;
    int  m_iWorldSize;
    int  m_iPos;
    int  m_iNumPlayers;
    int  m_iGameID;
    WORD m_cVerMajor;
    WORD m_cVerMinor;
    WORD m_cVerRelease;
    char m_cFlags;
    char m_sPlyrName[1];
};

struct OldNetJoin
{
    int  m_iLen;
    int  m_iPlyrNum;
    BOOL m_bServer;
    char m_sName[1];
};

//---------------------------------------------------------------------------
//  Current layouts (netapi.h).
//---------------------------------------------------------------------------

struct NewNetPublish
{
    int   m_iLen;
    int   m_iNumOpponents;
    int   m_iAIlevel;
    int   m_iWorldSize;
    int   m_iPos;
    int   m_iNumPlayers;
    int   m_iGameID;
    WORD  m_cVerMajor;
    WORD  m_cVerMinor;
    WORD  m_cVerRelease;
    char  m_cFlags;
    DWORD m_dwDataHash;
    char  m_sPlyrName[1];
};

struct NewNetJoin
{
    int   m_iLen;
    int   m_iPlyrNum;
    BOOL  m_bServer;
    DWORD m_dwDataHash;
    char  m_sName[1];

    static int MinLen( ) { return (int)sizeof( NewNetJoin ); }
};

void TestSizesAreWhatTheGameAsserts( )
{
    std::printf( "[data_net] sizeof CNetPublish: %d -> %d (vdmplay copies %d)\n", (int)sizeof( OldNetPublish ),
                 (int)sizeof( NewNetPublish ), kSessionBlock );
    std::printf( "[data_net] sizeof CNetJoin:    %d -> %d (vdmplay copies %d)\n", (int)sizeof( OldNetJoin ),
                 (int)sizeof( NewNetJoin ), kPlayerBlock );

    CHECK_EQ( sizeof( OldNetPublish ), 36 );
    CHECK_EQ( sizeof( NewNetPublish ), 44 );
    CHECK_EQ( sizeof( OldNetJoin ), 16 );
    CHECK_EQ( sizeof( NewNetJoin ), 20 );
}

void TestHeadersStillFitTheFixedBlocks( )
{
    // Alloc pads the buffer to max(516, iLen) and vdmplay copies the first 512
    // (session) or 256 (player) bytes. The header has to leave room for the
    // variable tail: player name + password + game name + description for the
    // session record, one player name for the join record.
    CHECK( Lt( sizeof( NewNetPublish ), kSessionBlock ) );
    CHECK( Lt( sizeof( NewNetJoin ), kPlayerBlock ) );

    // Room for four 64-character strings and their NULs in the session record...
    CHECK( Lt( sizeof( NewNetPublish ) + 4 * 65 - 1, kSessionBlock ) );
    // ...and a 64-character player name in the join record, with plenty over.
    CHECK( Lt( sizeof( NewNetJoin ) + 65 - 1, kPlayerBlock ) );
}

void TestHashSitsBeforeTheVariableTail( )
{
    // The whole point of putting m_dwDataHash BEFORE the name: the name is
    // variable-length and everything after it is found by walking NULs, so a
    // fixed field has to sit in the fixed part of the record.
    CHECK( Lt( offsetof( NewNetPublish, m_dwDataHash ), offsetof( NewNetPublish, m_sPlyrName ) ) );
    CHECK( Lt( offsetof( NewNetJoin, m_dwDataHash ), offsetof( NewNetJoin, m_sName ) ) );

    // And it must not have displaced anything the browser filter reads.
    CHECK_EQ( offsetof( NewNetPublish, m_iGameID ), offsetof( OldNetPublish, m_iGameID ) );
    CHECK_EQ( offsetof( NewNetPublish, m_cVerMajor ), offsetof( OldNetPublish, m_cVerMajor ) );
    CHECK_EQ( offsetof( NewNetPublish, m_cFlags ), offsetof( OldNetPublish, m_cFlags ) );
    CHECK_EQ( offsetof( NewNetJoin, m_iPlyrNum ), offsetof( OldNetJoin, m_iPlyrNum ) );
    CHECK_EQ( offsetof( NewNetJoin, m_bServer ), offsetof( OldNetJoin, m_bServer ) );
}

// Mirrors CNetJoin::Alloc's length arithmetic.
int AllocLen( const char* pName ) { return (int)sizeof( NewNetJoin ) + 2 + (int)strlen( pName ); }
int OldAllocLen( const char* pName ) { return (int)sizeof( OldNetJoin ) + 2 + (int)strlen( pName ); }

void TestMinLenRule( )
{
    // A record built by this build always clears the minimum, even for an empty
    // player name.
    CHECK( AllocLen( "" ) >= NewNetJoin::MinLen( ) );
    CHECK( AllocLen( "a-long-enough-player-name" ) >= NewNetJoin::MinLen( ) );

    // A pre-015 record is 4 bytes shorter for the same name, so the length rule
    // catches it only when the name is 0 or 1 characters...
    CHECK( OldAllocLen( "" ) < NewNetJoin::MinLen( ) );
    CHECK( OldAllocLen( "a" ) < NewNetJoin::MinLen( ) );

    // ...and NOT beyond that, which is exactly why the minimum length is
    // documented as a sanity bound and not as a format discriminator: a pre-015
    // record with a 2+ character name passes the length test and is then caught
    // by the hash compare, because its name bytes sit where the hash belongs.
    CHECK( OldAllocLen( "ab" ) >= NewNetJoin::MinLen( ) );
    CHECK( OldAllocLen( "abcd" ) >= NewNetJoin::MinLen( ) );
}

void TestPre015RecordIsRefusedByTheHashCompare( )
{
    // Build the bytes a pre-015 client would send and read them through the
    // CURRENT layout, the way the server does.
    char buf[kPlayerBlock];
    memset( buf, 0, sizeof( buf ) );

    OldNetJoin* pOld  = (OldNetJoin*)buf;
    pOld->m_iLen      = OldAllocLen( "Joiner" );
    pOld->m_iPlyrNum  = 2;
    pOld->m_bServer   = 0;
    memcpy( pOld->m_sName, "Joiner", 7 );

    const NewNetJoin* pNew = (const NewNetJoin*)buf;
    CHECK( pNew->m_iLen >= NewNetJoin::MinLen( ) );  // passes the length rule

    // "Join" read as a little-endian DWORD. Whatever the host's hash is, it is
    // computed from the gameplay data and cannot be this; the compare refuses.
    const DWORD dwAsRead = pNew->m_dwDataHash;
    CHECK_EQ( dwAsRead, 0x6e696f4aUL );

    const DWORD dwHostHash = 0x1234abcdUL;
    CHECK( dwAsRead != dwHostHash );
}

//---------------------------------------------------------------------------
//  The two guard decisions, compiled from the SHIPPED
//  enations_latest/src/datahashguard.h. Neither side of the handshake is
//  mirrored here: these are the functions SDL2Dialogs.cpp and netapi.cpp call.
//---------------------------------------------------------------------------

// Stands in for SDL2_RunJoinNetworkFlow's browser-pick step: returns true when
// the joiner would proceed to theNet.Join, counting the wire calls it makes.
struct JoinAttempt
{
    bool bDialed;
    bool bTold;
};

JoinAttempt TryJoin( DWORD dwPublished, DWORD dwMine )
{
    JoinAttempt a;
    a.bDialed = false;
    a.bTold   = false;

    if ( !endataguard::JoinAllowed( dwPublished, dwMine ) )
    {
        a.bTold = true;   // IDS_DATA_MISMATCH, with both numbers
        return a;         // ...and NOT a single wire call
    }
    a.bDialed = true;
    return a;
}

void TestJoinerRefusesBeforeAnyWireCall( )
{
    const DWORD dwMine = 0x1234abcdUL;

    // Equal hashes: the join proceeds and the player is told nothing.
    JoinAttempt ok = TryJoin( dwMine, dwMine );
    CHECK( ok.bDialed );
    CHECK( !ok.bTold );

    // Different hashes: the player is told, and nothing is dialed. This is the
    // point of doing it here - the host's refusal reaches the joiner as a
    // silent timeout, so the reason has to be given on this side.
    JoinAttempt bad = TryJoin( 0xdeadbeefUL, dwMine );
    CHECK( !bad.bDialed );
    CHECK( bad.bTold );

    // A one-bit difference is still a difference.
    JoinAttempt nearly = TryJoin( dwMine ^ 1UL, dwMine );
    CHECK( !nearly.bDialed );
    CHECK( nearly.bTold );

    // A host that never set the field (pre-015, zeroed buffer) is refused with
    // a reason rather than silently vanishing from the browser.
    JoinAttempt old = TryJoin( 0UL, dwMine );
    CHECK( !old.bDialed );
    CHECK( old.bTold );

    // ...unless our own hash really is 0, which only happens when this install
    // could not read its own gameplay set; then the guard cannot discriminate
    // and the host's copy is the one that decides.
    JoinAttempt bothBroken = TryJoin( 0UL, 0UL );
    CHECK( bothBroken.bDialed );
}

void TestHostPredicate( )
{
    const DWORD dwMine  = 0x1234abcdUL;
    const int   kMinLen = (int)sizeof( NewNetJoin );

    // A record from this build with a matching set is admitted.
    CHECK( endataguard::AcceptJoin( AllocLen( "Joiner" ), kMinLen, dwMine, dwMine ) );

    // A differing set is refused however long the record is.
    CHECK( !endataguard::AcceptJoin( AllocLen( "Joiner" ), kMinLen, 0xdeadbeefUL, dwMine ) );

    // Length is checked FIRST, so a short record is refused without its
    // m_dwDataHash ever being believed - even one whose bytes happen to match.
    CHECK( !endataguard::AcceptJoin( OldAllocLen( "" ), kMinLen, dwMine, dwMine ) );
    CHECK( !endataguard::AcceptJoin( 0, kMinLen, dwMine, dwMine ) );
    CHECK( !endataguard::AcceptJoin( -1, kMinLen, dwMine, dwMine ) );

    // Exactly the minimum is acceptable.
    CHECK( endataguard::AcceptJoin( kMinLen, kMinLen, dwMine, dwMine ) );

    // The host and the joiner agree on what "same" means: whenever the joiner
    // would dial, a well-formed record from it is admitted, and whenever it
    // would not, the host refuses. The joiner's check is a courtesy; the host's
    // is the authority, and they must never disagree in the permissive
    // direction.
    const DWORD adw[] = { 0UL, 1UL, dwMine, 0xdeadbeefUL, 0xffffffffUL };
    for ( int i = 0; i < (int)( sizeof( adw ) / sizeof( adw[0] ) ); i++ )
    {
        const bool bJoinerWouldDial = TryJoin( adw[i], dwMine ).bDialed;
        const bool bHostAdmits      = endataguard::AcceptJoin( AllocLen( "P" ), kMinLen, adw[i], dwMine );
        CHECK( bJoinerWouldDial == bHostAdmits );
    }
}

}  // namespace

int main( )
{
    TestSizesAreWhatTheGameAsserts( );
    TestHeadersStillFitTheFixedBlocks( );
    TestHashSitsBeforeTheVariableTail( );
    TestMinLenRule( );
    TestPre015RecordIsRefusedByTheHashCompare( );
    TestJoinerRefusesBeforeAnyWireCall( );
    TestHostPredicate( );
    return microtest::Summary( );
}
