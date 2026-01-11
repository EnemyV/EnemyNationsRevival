#include "cpuspeed.hpp"

#if defined( _WIN32 )
#include <windows.h>

double CPUInfo::get_cpu_mhz_windows() const
{
    HKEY hKey;
    LONG status =
        RegOpenKeyExA( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey );

    if ( status != ERROR_SUCCESS )
        return 0.0;

    DWORD mhz = 0;
    DWORD size = sizeof( mhz );
    status     = RegQueryValueExA( hKey, "~MHz", NULL, NULL, (LPBYTE)&mhz, &size );
    RegCloseKey( hKey );

    return ( status == ERROR_SUCCESS ) ? static_cast<double>( mhz ) : 0.0;
}

#elif defined( __APPLE__ )
#include <sys/sysctl.h>

double CPUInfo::get_cpu_mhz_macos() const
{
    int64_t freq = 0;
    size_t  size = sizeof( freq );

    if ( sysctlbyname( "hw.cpufrequency", &freq, &size, NULL, 0 ) == 0 && freq > 0 )
    {
        return freq / 1e6;  // Convert Hz to MHz
    }

    return 0.0;
}

#elif defined( __linux__ )
#include <cstdio>
#include <cstdlib>
#include <cstring>

double CPUInfo::get_cpu_mhz_linux() const
{
    FILE* fp = fopen( "/proc/cpuinfo", "r" );
    if ( !fp )
        return 0.0;

    char   line[256];
    double mhz = 0.0;

    while ( fgets( line, sizeof( line ), fp ) )
    {
        if ( strncmp( line, "cpu MHz", 7 ) == 0 )
        {
            char* mhz_str = strstr( line, ": " );
            if ( mhz_str )
            {
                mhz = strtod( mhz_str + 2, NULL ); // skip ": "
                break;
            }
        }
    }

    fclose( fp );
    return mhz;
}
#endif

double CPUInfo::get_cpu_mhz() const
{
#if defined( _WIN32 )
    return get_cpu_mhz_windows();
#elif defined( __APPLE__ )
    return get_cpu_mhz_macos();
#elif defined( __linux__ )
    return get_cpu_mhz_linux();
#else
    return 0.0; // Unsupported platform
#endif
}

double CPUInfo::get_cpu_ghz() const
{
    return get_cpu_mhz() / 1000.0;
}

int64_t CPUInfo::get_cpu_hz() const
{
    return static_cast<int64_t>( get_cpu_mhz() * 1e6 );
}