#ifndef __W22_SETTINGS_H__
#define __W22_SETTINGS_H__

//---------------------------------------------------------------------------
// w22_settings.h — Thin settings abstraction for wind22
//
// Replaces direct ptheApp->GetProfileInt/WriteProfileInt calls so wind22
// doesn't need CWinApp. Implementation uses Win32 registry directly.
//
// Path: HKCU\Software\Second Chance\Second Chance\<section>\<entry>
//---------------------------------------------------------------------------

#include <windows.h>

namespace w22 {

// Read an integer setting from the registry. Returns default_val if not found.
int GetProfileInt( const char* section, const char* entry, int default_val );

// Write an integer setting to the registry.
void WriteProfileInt( const char* section, const char* entry, int value );

// Read a string setting. Returns pointer to internal buffer (valid until next call).
// If not found, returns default_val.
const char* GetProfileString( const char* section, const char* entry, const char* default_val );

// Write a string setting.
void WriteProfileString( const char* section, const char* entry, const char* value );

// Get the main game window HWND. Set by the game during init.
HWND GetMainHWND();
void SetMainHWND( HWND hWnd );

}  // namespace w22

#endif // __W22_SETTINGS_H__
