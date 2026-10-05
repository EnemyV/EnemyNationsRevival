// The game clock across LetsGo, on the SHIPPED text.
//
// run-load-clock-test.py extracts VERBATIM: the CCreateBase type enum (new_game.h),
// Get/SetElapsedSeconds (player.h) and LetsGo's clock statement(s) (newworld.cpp) into
// lc_*.inc. Everything else here is scene: a game holding the clock and an app holding
// m_pCreateGame, which LetsGo still owns at that point (DestroyExceptMain deletes it later).
//
// The clock is set first as CGame::Serialize would leave it after reading a save made at
// ~1016 s (7950 s was MacOpus's measurement; any non-zero value with a sub-second remainder).
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>

using DWORD = unsigned long;

class CGame
{
  public:
#include "lc_accessors.inc"
    DWORD m_dwElapsedTime = 0;
};
static CGame theGame;

class CCreateBase
{
  public:
#include "lc_enum.inc"
    explicit CCreateBase( int iTyp ) : m_iTyp( iTyp ) {}
    int m_iTyp;
};

class CConquerApp
{
  public:
    CCreateBase* m_pCreateGame = nullptr;
    void         ClockAtLetsGo( );
};

void CConquerApp::ClockAtLetsGo( )
{
#include "lc_tail.inc"
}

static const DWORD SAVED = 390151;   // 1016 s and 7/384 of a second, as read from the save

static DWORD RunLetsGo( CCreateBase* pCreate )
{
    CConquerApp app;
    app.m_pCreateGame       = pCreate;
    theGame.m_dwElapsedTime = SAVED;
    app.ClockAtLetsGo( );
    return theGame.m_dwElapsedTime;
}

int main( )
{
    CHECK_EQ( ( SAVED >> 4 ) / 24, 1016 );

    // a loaded game keeps the clock Serialize restored, bit for bit
    const int aLoad[] = { CCreateBase::load_single, CCreateBase::load_multi, CCreateBase::load_join };
    for ( int iTyp : aLoad )
    {
        CCreateBase create( iTyp );
        const DWORD dw = RunLetsGo( &create );
        std::printf( "typ %d (load) -> %lu s\n", iTyp, theGame.GetElapsedSeconds( ) );
        CHECK_EQ( dw, SAVED );
    }

    // a new or joined game starts at 0 whatever the clock held before
    const int aNew[] = { CCreateBase::scenario, CCreateBase::single, CCreateBase::create_net,
                         CCreateBase::join_net };
    for ( int iTyp : aNew )
    {
        CCreateBase create( iTyp );
        const DWORD dw = RunLetsGo( &create );
        std::printf( "typ %d (new/join) -> %lu s\n", iTyp, theGame.GetElapsedSeconds( ) );
        CHECK_EQ( dw, 0 );
    }

    // no create object: the 1996 behaviour (zero)
    CHECK_EQ( RunLetsGo( nullptr ), 0 );

    return microtest::Summary( );
}
