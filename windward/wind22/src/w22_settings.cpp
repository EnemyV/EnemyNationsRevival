//---------------------------------------------------------------------------
// w22_settings.cpp — Win32 registry implementation of wind22 settings
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "_windwrd.h"
#include "w22_settings.h"
#include "winappstub.h"

namespace w22 {

static const char* REG_BASE_PATH = "Software\\Second Chance\\Second Chance";
static HWND s_mainHWnd = NULL;
static char s_stringBuf[1024];

static void BuildKeyPath( char* dst, size_t cap, const char* section )
{
    _snprintf_s( dst, cap, _TRUNCATE, "%s\\%s", REG_BASE_PATH, section );
}

int GetProfileInt( const char* section, const char* entry, int default_val )
{
    char keyPath[512];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( RegOpenKeyExA( HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
        return default_val;

    DWORD value = 0;
    DWORD type = 0;
    DWORD size = sizeof( value );
    LONG result = RegQueryValueExA( hKey, entry, NULL, &type, (LPBYTE)&value, &size );
    RegCloseKey( hKey );

    if ( result != ERROR_SUCCESS || type != REG_DWORD )
        return default_val;

    return (int)value;
}

void WriteProfileInt( const char* section, const char* entry, int value )
{
    char keyPath[512];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) != ERROR_SUCCESS )
        return;

    DWORD val = (DWORD)value;
    RegSetValueExA( hKey, entry, 0, REG_DWORD, (const BYTE*)&val, sizeof( val ) );
    RegCloseKey( hKey );
}

const char* GetProfileString( const char* section, const char* entry, const char* default_val )
{
    char keyPath[512];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( RegOpenKeyExA( HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
        return default_val;

    DWORD type = 0;
    DWORD size = sizeof( s_stringBuf );
    LONG result = RegQueryValueExA( hKey, entry, NULL, &type, (LPBYTE)s_stringBuf, &size );
    RegCloseKey( hKey );

    if ( result != ERROR_SUCCESS || type != REG_SZ )
        return default_val;

    s_stringBuf[sizeof( s_stringBuf ) - 1] = '\0';
    return s_stringBuf;
}

void WriteProfileString( const char* section, const char* entry, const char* value )
{
    char keyPath[512];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) != ERROR_SUCCESS )
        return;

    RegSetValueExA( hKey, entry, 0, REG_SZ, (const BYTE*)value, (DWORD)( strlen( value ) + 1 ) );
    RegCloseKey( hKey );
}

HWND GetMainHWND()
{
    if ( s_mainHWnd )
        return s_mainHWnd;
    // Fallback to ptheApp's main window if game hasn't called SetMainHWND yet
    if ( ptheApp && ptheApp->m_pMainWnd )
        return ptheApp->m_pMainWnd->m_hWnd;
    return NULL;
}

void SetMainHWND( HWND hWnd )
{
    s_mainHWnd = hWnd;
}

}  // namespace w22
