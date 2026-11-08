#pragma once

#include <cstdint>
#include <string>

/**
 * CPUInfo class for querying CPU information
 * Supports Windows, macOS, and Linux
 */
class CPUInfo
{
  public:
    CPUInfo( )  = default;
    ~CPUInfo( ) = default;

    /**
     * Get CPU speed in MHz
     * Returns 0.0 if unable to determine
     */
    double get_cpu_mhz( ) const;

    /**
     * Get CPU speed in GHz
     * Returns 0.0 if unable to determine
     */
    double get_cpu_ghz( ) const;

    /**
     * Get CPU speed in Hz
     * Returns 0 if unable to determine
     */
    int64_t get_cpu_hz( ) const;

  private:
#if defined( _WIN32 )
    double get_cpu_mhz_windows( ) const;
#elif defined( __APPLE__ )
    double get_cpu_mhz_macos( ) const;
#elif defined( __linux__ )
    double get_cpu_mhz_linux( ) const;
#endif
};