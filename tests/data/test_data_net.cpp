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

}  // namespace

int main( )
{
    TestSizesAreWhatTheGameAsserts( );
    TestHeadersStillFitTheFixedBlocks( );
    TestHashSitsBeforeTheVariableTail( );
    TestMinLenRule( );
    TestPre015RecordIsRefusedByTheHashCompare( );
    return microtest::Summary( );
}
