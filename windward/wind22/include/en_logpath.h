#ifndef __EN_LOGPATH_H__
#define __EN_LOGPATH_H__
//---------------------------------------------------------------------------
// en_logpath.h - resolve debug-log filenames to an absolute directory.
//
// WinMain/linux_main chdir to the exe dir at startup (GH #8, so ENations.dat /
// data/ / res/ resolve however we were launched). Every logger used a bare
// relative name, so logs followed the exe into cmakeBuild-x64\...\Debug instead
// of the run dir - an hour was lost to a probe log that WAS being written, just
// not where anyone was reading. Capture the launch cwd BEFORE that chdir and
// resolve every log leaf against it.
//
// Precedence: $EN_LOG_DIR -> launch cwd -> bare leaf (= pre-fix behaviour, so a
// failure degrades to exactly what happens today and never crashes).
// Header-only: inline function-local statics are ODR-merged to one instance
// across TUs (same technique as en_assert.h's per-site dedup table).
//---------------------------------------------------------------------------
#include <string>
#include <cstdlib>
#ifdef _WIN32
  #include <direct.h>
  #define EN_LOGPATH_GETCWD _getcwd
  #define EN_LOGPATH_SEP    '\\'
#else
  #include <unistd.h>
  #define EN_LOGPATH_GETCWD getcwd
  #define EN_LOGPATH_SEP    '/'
#endif

inline std::string& EnLogLaunchDirRef( ) { static std::string s_dir; return s_dir; }

// Call ONCE from WinMain/main, before any chdir.
inline void EnLogCaptureLaunchDir( )
{
    char buf[1024];
    if ( EN_LOGPATH_GETCWD( buf, (int)sizeof( buf ) ) != 0 )
        EnLogLaunchDirRef( ) = buf;
}

// Absolute path for a log leaf name. Returns by value - callers must use .c_str()
// inside the same full expression as the fopen/ofstream call (temporary lifetime).
inline std::string EnLogPath( const char* pszLeaf )
{
    const char* pszEnv = getenv( "EN_LOG_DIR" );
    std::string sDir = ( pszEnv && pszEnv[0] ) ? std::string( pszEnv ) : EnLogLaunchDirRef( );
    if ( sDir.empty( ) )
        return std::string( pszLeaf );            // degrade to today's behaviour
    char cLast = sDir[ sDir.size( ) - 1 ];
    if ( cLast != '\\' && cLast != '/' )
        sDir += EN_LOGPATH_SEP;
    return sDir + pszLeaf;
}

#endif // __EN_LOGPATH_H__
