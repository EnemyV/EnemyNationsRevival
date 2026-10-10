#ifndef EN_HANGMODAL_H
#define EN_HANGMODAL_H
//---------------------------------------------------------------------------
// en_hangmodal.h - "a native modal box is up" flag for the hang watchdog
// (enations_latest/src/en_hangdump.cpp).
//
// The watchdog only sees heartbeats from the SDL pump loops. A native modal
// box (MessageBoxA, SDL_ShowSimpleMessageBox, GetOpenFileNameA) runs its own
// loop, so a box left open past the threshold looked like a frozen main thread
// and wrote a bogus ~2 GB "hang" dump. Hold an EnHangModalScope around every
// such call; while the depth is non-zero the watchdog resets its timer and
// never dumps (same treatment as IsDebuggerPresent).
//
// Header-only and in wind22/include so the wind22 static lib (datafile.cpp,
// dlgmsg.cpp, ...) can use it without linking against the game: the inline
// function-local static is ODR-merged to ONE counter across every TU. Builds
// on every platform; off Windows nothing reads it (the watchdog never starts).
//---------------------------------------------------------------------------
#include <atomic>

inline std::atomic<int>& EnHangModalDepthRef( )
{
    static std::atomic<int> s_nDepth( 0 );
    return s_nDepth;
}

inline bool EnHangModalActive( )
{
    return EnHangModalDepthRef( ).load( std::memory_order_relaxed ) > 0;
}

class EnHangModalScope
{
public:
    EnHangModalScope( )  { EnHangModalDepthRef( ).fetch_add( 1, std::memory_order_relaxed ); }
    ~EnHangModalScope( ) { EnHangModalDepthRef( ).fetch_sub( 1, std::memory_order_relaxed ); }
private:
    EnHangModalScope( const EnHangModalScope& );              // non-copyable
    EnHangModalScope& operator=( const EnHangModalScope& );
};

#endif // EN_HANGMODAL_H
