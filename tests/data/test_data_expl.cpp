// test_data_expl.cpp -- the explosion cleanup frame (015 area 3, phase 3, spec C.2).
//
// Compiles the SHIPPED enations_latest/src/explframe.h -- the predicates
// CExplosion::Operate actually calls -- and runs them through a tick loop that
// stands in for Operate's frame stepping. projbase.cpp itself cannot be
// compiled standalone (it pulls the whole game), so what is NOT proven here is
// that Operate calls these predicates with the right arguments; that is a
// static read, and the source lint at the bottom pins the call sites.
//
// The five gates from the spec:
//   4-frame, 40-frame, 400-frame and MISSING art all release on the same frame,
//   and a finished animation does not release early.
//
// Also linted here: CStructure::InitSprites no longer gates FLhaveArt on a
// sprite existing (spec C.1), so an art mod that drops a sprite cannot change
// what can be built.

#include "../ai/microtest.h"
#include "../../enations_latest/src/explframe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

namespace {

// Comparing two compile-time constants trips /W4's C4127 and the point is the
// RELATION, not a constant expression. One opaque helper keeps it quiet.
bool Le( int a, int b ) { return a <= b; }
bool Lt( int a, int b ) { return a < b; }

// One explosion's life, stepped the way CExplosion::Operate steps it.
// nAnimFrames == 0 means "no sprite at all", which Operate reads as finished.
// Returns the frame the corpse was released on, or -1 if it never was.
int RunExplosion( int nAnimFrames, int iKillFrame, int* piDeleteFrame )
{
    int iReleased = -1;
    int iDeleted  = -1;

    int iFrames = 0;
    for ( int iTick = 0; iTick < 1000; iTick++ )
    {
        iFrames++;  // Operate's m_iFrames++

        const bool bFinished = ( nAnimFrames <= 0 ) || ( iFrames >= nAnimFrames );

        if ( ( iReleased < 0 ) && enexpl::ShouldRelease( iFrames, iKillFrame ) ) iReleased = iFrames;

        if ( enexpl::MayDelete( bFinished, iFrames, iKillFrame ) )
        {
            iDeleted = iFrames;
            break;
        }
    }

    if ( piDeleteFrame ) *piDeleteFrame = iDeleted;
    return iReleased;
}

void TestEveryArtLengthReleasesOnTheSameFrame( )
{
    const int kK = enexpl::EXPL_KILLFRAME;

    int iDel4 = 0, iDel40 = 0, iDel400 = 0, iDelNone = 0;
    const int r4    = RunExplosion( 4, kK, &iDel4 );
    const int r40   = RunExplosion( 40, kK, &iDel40 );
    const int r400  = RunExplosion( 400, kK, &iDel400 );
    const int rNone = RunExplosion( 0, kK, &iDelNone );

    std::printf( "[data_expl] release frame: 4-frame=%d 40-frame=%d 400-frame=%d missing=%d (EXPL_KILLFRAME=%d)\n", r4,
                 r40, r400, rNone, kK );

    CHECK_EQ( r4, kK );
    CHECK_EQ( r40, kK );
    CHECK_EQ( r400, kK );
    CHECK_EQ( rNone, kK );

    // The old rule was AnimCount/2, which is what this replaces: it would have
    // given 2, 20, 200 and "never" for these four.
    CHECK( r4 == r40 );
    CHECK( r40 == r400 );
    CHECK( r400 == rNone );
}

void TestFinishedAnimationDoesNotReleaseEarly( )
{
    const int kK = enexpl::EXPL_KILLFRAME;

    // 4-frame art finishes at frame 4, well before the release frame. The
    // explosion must still be alive at the release frame, and must not have
    // released before it.
    int       iDel = 0;
    const int r    = RunExplosion( 4, kK, &iDel );
    CHECK( r >= kK );
    CHECK( iDel >= kK );

    // Missing art is the extreme case: "finished" on the very first frame.
    int       iDelNone = 0;
    const int rNone    = RunExplosion( 0, kK, &iDelNone );
    CHECK( rNone == kK );
    CHECK( iDelNone == kK );
}

void TestLongArtStillOutlivesTheRelease( )
{
    // 400-frame art: released at EXPL_KILLFRAME, deleted only when the
    // animation ends. The corpse hold is the constant; the visuals are free.
    int       iDel = 0;
    const int r    = RunExplosion( 400, enexpl::EXPL_KILLFRAME, &iDel );
    CHECK_EQ( r, enexpl::EXPL_KILLFRAME );
    CHECK_EQ( iDel, 400 );
}

void TestExplosionWithNoCorpseIsNotHeld( )
{
    // Most explosions are just an impact puff: no corpse, so nothing to release
    // and no reason to keep the object past its animation.
    int       iDel = 0;
    const int r    = RunExplosion( 4, enexpl::EXPL_NO_KILLFRAME, &iDel );
    CHECK_EQ( r, -1 );
    CHECK_EQ( iDel, 4 );

    // ...and with no art at all it goes on the first frame, as it used to.
    int       iDelNone = 0;
    const int rNone    = RunExplosion( 0, enexpl::EXPL_NO_KILLFRAME, &iDelNone );
    CHECK_EQ( rNone, -1 );
    CHECK_EQ( iDelNone, 1 );
}

void TestConstantIsInsideTheBandTheOldRuleCouldProduce( )
{
    // sprtinit.cpp:3113 asserts an animation is at most 26 frames, so the old
    // AnimCount/2 rule could only ever produce 0..13. That is the cheap bound;
    // TestConstantMatchesTheShippedArt below is the real check.
    CHECK( Lt( 0, enexpl::EXPL_KILLFRAME ) );
    CHECK( Le( enexpl::EXPL_KILLFRAME, 13 ) );
    CHECK( Lt( enexpl::EXPL_KILLFRAME, enexpl::EXPL_NO_KILLFRAME ) );
}

//---------------------------------------------------------------------------
//  The constant, pinned to the shipped art.
//
//  EXPL_KILLFRAME is the old `AnimCount( ANIM_FRONT_1 ) / 2` written down as a
//  simulation constant, so it has to keep equalling that for the sprites the
//  game actually uses. This reads effect.rif with the on-disk sprite layout the
//  loader uses (sprite.h: CSpriteHdr, CSpriteView, CSpriteDIB, 32-bit, pack(4))
//  and re-derives the number. Path from EN_EFFECT_RIF; SKIP when the extracted
//  data set is not on this machine, the way tests/ai/test_ai_data.cpp skips a
//  missing stdgta.dat.
//---------------------------------------------------------------------------

// on-disk layout, from sprite.h
const int kSpriteHdrFixed    = 4 + 4 + 4 + 4 * 8;         // compression, nViews, nSupers, CBlockInfo[4]
const int kSuperviewInfo     = 4 * ( 8 + 4 * 4 );         // CLayoutInfo[NUM_ZOOM_LEVELS]
const int kSpriteViewAnim0   = 128 + 32 + 4 + 4 + 4 + 4;  // reserved, anchor, superviewIdx, nHotSpots, nBase, nOverlay
const int kEffectExplosionId = 2;                         // CEffect::explosion

typedef std::vector<unsigned char> Blob;

unsigned long LE32( const Blob& b, size_t at )
{
    if ( at + 4 > b.size( ) ) return 0;
    return (unsigned long)b[at] | ( (unsigned long)b[at + 1] << 8 ) | ( (unsigned long)b[at + 2] << 16 ) |
           ( (unsigned long)b[at + 3] << 24 );
}

bool ReadWhole( const char* pPath, Blob& out )
{
    FILE* fp = NULL;
    fopen_s( &fp, pPath, "rb" );
    if ( fp == NULL ) return false;
    unsigned char buf[8192];
    size_t        n;
    out.clear( );
    while ( ( n = fread( buf, 1, sizeof( buf ), fp ) ) > 0 ) out.insert( out.end( ), buf, buf + n );
    fclose( fp );
    return !out.empty( );
}

bool Is4( const Blob& b, size_t at, const char* p4 )
{
    return ( at + 4 <= b.size( ) ) && ( memcmp( &b[at], p4, 4 ) == 0 );
}

// Payload bounds of the named top-level LIST inside a RIFF of form pForm.
bool FindList( const Blob& b, const char* pForm, const char* pList, size_t& rStart, size_t& rEnd )
{
    if ( b.size( ) < 12 || !Is4( b, 0, "RIFF" ) || !Is4( b, 8, pForm ) ) return false;
    size_t end = (size_t)LE32( b, 4 ) + 8;
    if ( end > b.size( ) ) end = b.size( );

    size_t p = 12;
    while ( p + 8 <= end )
    {
        const unsigned long cb = LE32( b, p + 4 );
        if ( cb > end - ( p + 8 ) ) return false;
        if ( Is4( b, p, "LIST" ) && cb >= 4 && Is4( b, p + 8, pList ) )
        {
            rStart = p + 12;
            rEnd   = p + 8 + (size_t)cb;
            return true;
        }
        p = p + 8 + (size_t)cb + ( cb & 1 );
    }
    return false;
}

// ANIM_FRONT_1 counts of every sprite in effect.rif carrying the explosion id,
// in file order. Empty means the parse failed.
std::vector<int> ExplosionFront1Counts( const Blob& b )
{
    std::vector<int> out;
    size_t           s = 0, e = 0;
    if ( !FindList( b, "EFFX", "SP24", s, e ) ) return out;

    size_t p = s;
    while ( p + 8 <= e )
    {
        const unsigned long cb   = LE32( b, p + 4 );
        const size_t        body = p + 8;
        if ( cb > e - body ) return std::vector<int>( );

        if ( Is4( b, p, "DATA" ) )
        {
            size_t    q   = body;
            const int iID = (short)( b[q] | ( b[q + 1] << 8 ) );
            q += 2;
            const long lLenTotal = (long)LE32( b, q );
            q += 4;
            q += 4;  // m_iType: overridden at load, not needed here

            if ( ( lLenTotal != -1 ) && ( iID == kEffectExplosionId ) )
            {
                const size_t lHdrLen = (size_t)LE32( b, q );
                q += 4;
                const size_t hdr = q;
                if ( hdr + lHdrLen > b.size( ) ) return std::vector<int>( );

                const int nViews  = (int)LE32( b, hdr + 4 );
                const int nSupers = (int)LE32( b, hdr + 8 );
                if ( nViews <= 0 || nViews > 256 || nSupers < 0 || nSupers > 256 ) return std::vector<int>( );

                const size_t offsAt = hdr + kSpriteHdrFixed + (size_t)nSupers * kSuperviewInfo;
                for ( int v = 0; v < nViews; v++ )
                {
                    const size_t voff = (size_t)LE32( b, offsAt + 4 * v );
                    const size_t at   = hdr + voff + kSpriteViewAnim0;
                    if ( at + 4 > b.size( ) ) return std::vector<int>( );
                    out.push_back( (int)LE32( b, at ) );
                }
            }
        }
        p = body + (size_t)cb + ( cb & 1 );
    }
    return out;
}

void TestConstantMatchesTheShippedArt( )
{
    const char* pPath = getenv( "EN_EFFECT_RIF" );
    Blob        b;
    if ( pPath == NULL || *pPath == 0 || !ReadWhole( pPath, b ) )
    {
        std::printf( "[data_expl] SKIP art measurement (set EN_EFFECT_RIF to the extracted effect.rif)\n" );
        return;
    }

    const std::vector<int> counts = ExplosionFront1Counts( b );
    CHECK( !counts.empty( ) );
    if ( counts.empty( ) )
    {
        std::printf( "[data_expl] effect.rif did not parse - the sprite layout constants are wrong\n" );
        return;
    }

    std::printf( "[data_expl] stock explosion ANIM_FRONT_1 counts:" );
    for ( size_t i = 0; i < counts.size( ); i++ ) std::printf( " %d", counts[i] );
    std::printf( "  (EXPL_KILLFRAME=%d)\n", enexpl::EXPL_KILLFRAME );

    // units.rif's EXPL list names four explosion sprites and effect.rif holds
    // exactly four with that id, so this covers every explosion a dying unit
    // can draw.
    CHECK_EQ( (int)counts.size( ), 4 );

    for ( size_t i = 0; i < counts.size( ); i++ )
    {
        // A sane frame count in the first place - sprtinit.cpp:3113's bound. If
        // this trips, the layout arithmetic above is reading the wrong field and
        // the equality below would be meaningless.
        CHECK( Lt( 0, counts[i] ) );
        CHECK( Le( counts[i], 26 ) );

        // ...and the actual claim: the constant IS the old art-derived value.
        CHECK_EQ( counts[i] / 2, enexpl::EXPL_KILLFRAME );
    }
}

//---------------------------------------------------------------------------
//  Source lints: what a header-only fixture cannot reach.
//---------------------------------------------------------------------------

// The repo root comes from the environment (the runner sets it); the lints
// SKIP rather than fail when it is not set, so the arithmetic gates above still
// run from any working directory.
bool ReadSource( const char* pRelPath, std::string& out )
{
    const char* pRoot = getenv( "EN_REPO_ROOT" );
    if ( pRoot == NULL || *pRoot == 0 ) return false;
    const std::string sPath = std::string( pRoot ) + "/" + pRelPath;

    FILE* fp = NULL;
    fopen_s( &fp, sPath.c_str( ), "rb" );
    if ( fp == NULL ) return false;
    char buf[8192];
    size_t n;
    out.clear( );
    while ( ( n = fread( buf, 1, sizeof( buf ), fp ) ) > 0 ) out.append( buf, n );
    fclose( fp );
    return true;
}

void TestProjbaseCallSites( )
{
    std::string s;
    if ( !ReadSource( "enations_latest/src/projbase.cpp", s ) )
    {
        std::printf( "[data_expl] SKIP projbase.cpp lint (not found from this cwd)\n" );
        return;
    }

    // The art-derived kill frame is gone...
    CHECK( s.find( "AnimCount (CSpriteView::ANIM_FRONT_1) / 2" ) == std::string::npos );
    // ...and the shipped predicates are the ones Operate asks.
    CHECK( s.find( "enexpl::ShouldRelease( m_iFrames, m_iKillFrame )" ) != std::string::npos );
    CHECK( s.find( "enexpl::MayDelete( bFinished != FALSE, m_iFrames, m_iKillFrame )" ) != std::string::npos );
    // The frame counter is stepped exactly once per Operate.
    CHECK( s.find( "m_iFrames++;" ) != std::string::npos );
    // Both explosion constructors install the constant.
    size_t at = s.find( "m_iKillFrame = enexpl::EXPL_KILLFRAME;" );
    CHECK( at != std::string::npos );
    CHECK( s.find( "m_iKillFrame = enexpl::EXPL_KILLFRAME;", at + 1 ) != std::string::npos );
    // A missing sprite no longer deletes the explosion before the cleanup.
    CHECK( s.find( "No sprite attached - remove and destroy" ) == std::string::npos );
}

void TestFLhaveArtIsNotGatedOnASprite( )
{
    std::string s;
    if ( !ReadSource( "enations_latest/src/sprtinit.cpp", s ) )
    {
        std::printf( "[data_expl] SKIP sprtinit.cpp lint (not found from this cwd)\n" );
        return;
    }

    CHECK( s.find( "if (GetSprite(i, 0, TRUE) != NULL)\n            pSd->m_udFlags" ) == std::string::npos );
    CHECK( s.find( "if (GetSprite(i, 0, TRUE) != NULL)\r\n            pSd->m_udFlags" ) == std::string::npos );
    CHECK( s.find( "pSd->m_udFlags = (CUnitData::UNIT_DATA_FLAGS) (pSd->m_udFlags | CUnitData::FLhaveArt);" ) !=
           std::string::npos );
}

}  // namespace

int main( )
{
    TestEveryArtLengthReleasesOnTheSameFrame( );
    TestFinishedAnimationDoesNotReleaseEarly( );
    TestLongArtStillOutlivesTheRelease( );
    TestExplosionWithNoCorpseIsNotHeld( );
    TestConstantIsInsideTheBandTheOldRuleCouldProduce( );
    TestConstantMatchesTheShippedArt( );
    TestProjbaseCallSites( );
    TestFLhaveArtIsNotGatedOnASprite( );
    return microtest::Summary( );
}
