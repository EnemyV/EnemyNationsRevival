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
#include <string>

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
    // AnimCount/2 rule could only ever produce 0..13. Keeping the constant in
    // that band is what "matches today's stock timing" means here.
    CHECK( Lt( 0, enexpl::EXPL_KILLFRAME ) );
    CHECK( Le( enexpl::EXPL_KILLFRAME, 13 ) );
    CHECK( Lt( enexpl::EXPL_KILLFRAME, enexpl::EXPL_NO_KILLFRAME ) );
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
    TestProjbaseCallSites( );
    TestFLhaveArtIsNotGatedOnASprite( );
    return microtest::Summary( );
}
