#ifndef DATA_TEST_EN_LOGPATH_H
#define DATA_TEST_EN_LOGPATH_H
#include <string>
// tests/data/shim: the real EnLogPath writes beside the launch dir. The fixture
// must not litter, so every log line goes to the null device.
inline std::string EnLogPath( const char* ) { return std::string( "NUL" ); }
#endif
