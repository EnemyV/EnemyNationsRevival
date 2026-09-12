#ifndef EXPLFRAME_H
#define EXPLFRAME_H

//---------------------------------------------------------------------------
//
//  When a dying unit's corpse is released (015 area 3, phase 3).
//
//  The old rule was `m_iKillFrame = AnimCount( ANIM_FRONT_1 ) / 2` - half the
//  EXPLOSION SPRITE'S frame count - and the cleanup at projbase.cpp's
//  CExplosion::Operate then released a dying vehicle's hex ownership, or
//  rewrote a dying building's terrain, when the animation reached it. That made
//  a simulation event a function of ART: 4-frame art released a dying vehicle
//  at frame 2 while 40-frame art did not, and a missing sprite deleted the
//  explosion before the cleanup ran at all. Art has to stay outside the
//  gameplay hash (players may replace sprites and sound), so the release point
//  cannot depend on it.
//
//  So the release point is a SIMULATION CONSTANT, counted in frames of the
//  explosion's OWN life, and it is installed whether or not a sprite exists.
//  4-frame, 40-frame, 400-frame and missing art all release on the same frame,
//  and an animation that finishes early does not release early - it just stops
//  moving while the explosion lives out its count.
//
//  Header-only and dependency-free so tests/data/test_data_expl.cpp compiles
//  the shipped predicates rather than a copy of them.
//
//---------------------------------------------------------------------------

namespace enexpl {

//  Frames a dying unit's corpse is held before the cleanup runs.
//
//  The old art-derived value was AnimCount/2, and sprtinit.cpp:3113 asserts an
//  animation is at most 26 frames, so the rule could only ever produce 0..13.
//  This sits in the middle of that band: within a few frames of what every
//  shipped explosion did, and now the SAME number on every client whatever art
//  it is running.
const int EXPL_KILLFRAME = 6;

//  "This explosion owns no corpse." The old code spelled this 10000 and then
//  asked `m_iKillFrame < 256` to mean the opposite; one name for it now.
const int EXPL_NO_KILLFRAME = 10000;

inline bool OwnsACorpse( int iKillFrame ) { return iKillFrame < EXPL_NO_KILLFRAME; }

//  Has the explosion lived long enough for the cleanup to run? Frames of its
//  own life, never of its sprite, so the answer does not depend on the art.
inline bool ShouldRelease( int iFrames, int iKillFrame )
{
    return OwnsACorpse( iKillFrame ) && ( iFrames >= iKillFrame );
}

//  May the explosion object be destroyed this frame? Only once its animation is
//  done AND its cleanup frame has passed - otherwise art shorter than
//  EXPL_KILLFRAME (or missing entirely, which reads as "finished") would delete
//  the explosion before the release, which is the bug this replaces. An
//  explosion that owns no corpse still goes as soon as its animation ends.
inline bool MayDelete( bool bAnimFinished, int iFrames, int iKillFrame )
{
    if ( !bAnimFinished ) return false;
    return !OwnsACorpse( iKillFrame ) || ( iFrames >= iKillFrame );
}

}  // namespace enexpl

#endif  // EXPLFRAME_H
