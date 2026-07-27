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

    // Intel Macs expose the nominal CPU frequency here.
    if ( sysctlbyname( "hw.cpufrequency", &freq, &size, NULL, 0 ) == 0 && freq > 0 )
        return freq / 1e6;  // Hz -> MHz

    // Apple Silicon does NOT expose hw.cpufrequency (it returns ENOENT), so the
    // query above fails and we'd report 0 MHz. The engine treats <=200 MHz as a
    // 1996-era machine and forces 8-bit/low-detail assets (e.g. the menu falls
    // back to the 96x96 WL tile and loads MN08 instead of MN24). Return a
    // realistic modern figure so the full 24-bit/high-detail path is selected.
    // Try the max-frequency key first; otherwise assume a fast Apple-Silicon core.
    freq = 0; size = sizeof( freq );
    if ( sysctlbyname( "hw.cpufrequency_max", &freq, &size, NULL, 0 ) == 0 && freq > 0 )
        return freq / 1e6;

    return 3200.0;  // Apple Silicon performance core ~3.2 GHz
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