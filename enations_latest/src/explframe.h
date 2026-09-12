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
//  MEASURED off the shipped art, so this is what the stock game already did -
//  it is not a new timing, it is the old one written down where a mod cannot
//  move it.
//
//  units.rif's EXPL list names four explosion sprites (entries 2, 3 and 4 are
//  what CExplosion picks at random for a dying unit, entry 5 is the one the
//  projectile constructor uses), and those are exactly the four sprites in
//  effect.rif with CEffect::explosion's id. Their ANIM_FRONT_1 frame counts are
//  10, 11, 10 and 10, so the old `AnimCount( ANIM_FRONT_1 ) / 2` evaluated to 5
//  for EVERY stock explosion - the integer divide flattens the 11.
//
//  tests/data/test_data_expl.cpp re-reads those counts out of effect.rif and
//  fails if this constant stops matching them.
const int EXPL_KILLFRAME = 5;

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
