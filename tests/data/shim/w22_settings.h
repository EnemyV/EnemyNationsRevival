#ifndef DATA_TEST_W22_SETTINGS_H
#define DATA_TEST_W22_SETTINGS_H
#include "stdafx.h"
// tests/data/shim: the registry/profile store, backed by a map the checks can
// seed and read back (the pin-retirement check needs to see whether a write
// happened at all).
namespace w22 {
struct TestProfile {
    std::unordered_map<std::string, std::string> vals;
    int                                          writes = 0;
};
TestProfile& Profile();

inline const char* GetProfileString( const char* pSec, const char* pKey, const char* pDef ) {
    std::string k = std::string( pSec ) + "\\" + pKey;
    std::unordered_map<std::string, std::string>::iterator it = Profile().vals.find( k );
    if ( it == Profile().vals.end() ) return pDef ? pDef : "";
    return it->second.c_str();
}
inline void WriteProfileString( const char* pSec, const char* pKey, const char* pVal ) {
    Profile().vals[std::string( pSec ) + "\\" + pKey] = pVal ? pVal : "";
    Profile().writes++;
}
}
#endif
