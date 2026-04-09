//---------------------------------------------------------------------------
// EnSettings.cpp — Win32 implementation of the game-side settings/string shim
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "EnSettings.h"

#include <windows.h>
#include <stdio.h>

// CString header — still using MFC's CString during Phase 4. Phase 5 will
// migrate to std::string.
#include <afx.h>

namespace {

const char* const kRegBasePath = "Software\\Second Chance\\Second Chance";

void BuildKeyPath( char* dst, size_t cap, const char* section )
{
    _snprintf_s( dst, cap, _TRUNCATE, "%s\\%s", kRegBasePath, section );
}

} // namespace

int EnGetProfileInt( const char* section, const char* entry, int default_val )
{
    if ( !section || !entry )
        return default_val;

    char keyPath[ 512 ];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( ::RegOpenKeyExA( HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
        return default_val;

    DWORD value = 0;
    DWORD type  = 0;
    DWORD size  = sizeof( value );
    LONG  rc    = ::RegQueryValueExA( hKey, entry, NULL, &type, (LPBYTE)&value, &size );
    ::RegCloseKey( hKey );

    if ( rc != ERROR_SUCCESS || type != REG_DWORD )
        return default_val;

    return (int)value;
}

void EnWriteProfileInt( const char* section, const char* entry, int value )
{
    if ( !section || !entry )
        return;

    char keyPath[ 512 ];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( ::RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) != ERROR_SUCCESS )
        return;

    DWORD val = (DWORD)value;
    ::RegSetValueExA( hKey, entry, 0, REG_DWORD, (const BYTE*)&val, sizeof( val ) );
    ::RegCloseKey( hKey );
}

CString EnGetProfileString( const char* section, const char* entry, const char* default_val )
{
    CString result;
    if ( default_val )
        result = default_val;

    if ( !section || !entry )
        return result;

    char keyPath[ 512 ];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    HKEY hKey = NULL;
    if ( ::RegOpenKeyExA( HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
        return result;

    // Query size first.
    DWORD type = 0;
    DWORD size = 0;
    LONG  rc   = ::RegQueryValueExA( hKey, entry, NULL, &type, NULL, &size );
    if ( rc != ERROR_SUCCESS || ( type != REG_SZ && type != REG_EXPAND_SZ ) || size == 0 )
    {
        ::RegCloseKey( hKey );
        return result;
    }

    char* buf = result.GetBuffer( (int)size + 1 );
    rc = ::RegQueryValueExA( hKey, entry, NULL, &type, (LPBYTE)buf, &size );
    if ( rc == ERROR_SUCCESS )
    {
        // Ensure null-terminated even if registry value wasn't.
        buf[ size ] = '\0';
        result.ReleaseBuffer( -1 );
    }
    else
    {
        result.ReleaseBuffer( 0 );
        if ( default_val )
            result = default_val;
    }

    ::RegCloseKey( hKey );
    return result;
}

void EnWriteProfileString( const char* section, const char* entry, const char* value )
{
    if ( !section || !entry )
        return;

    char keyPath[ 512 ];
    BuildKeyPath( keyPath, sizeof( keyPath ), section );

    if ( !value )
    {
        // Match CWinApp::WriteProfileString(NULL): delete the entry.
        HKEY hKey = NULL;
        if ( ::RegOpenKeyExA( HKEY_CURRENT_USER, keyPath, 0, KEY_WRITE, &hKey ) == ERROR_SUCCESS )
        {
            ::RegDeleteValueA( hKey, entry );
            ::RegCloseKey( hKey );
        }
        return;
    }

    HKEY hKey = NULL;
    if ( ::RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) != ERROR_SUCCESS )
        return;

    ::RegSetValueExA( hKey, entry, 0, REG_SZ,
                      (const BYTE*)value, (DWORD)( strlen( value ) + 1 ) );
    ::RegCloseKey( hKey );
}

CString EnLoadString( unsigned int id )
{
    // Win32 LoadStringA against the game module — no MFC AfxGetResourceHandle
    // dependency. Buffer 1024 chars matches the longest IDS_* in the .RC file.
    char buf[ 1024 ];
    HMODULE hModule = ::GetModuleHandleA( NULL );
    int len = ::LoadStringA( hModule, id, buf, (int)sizeof( buf ) );
    if ( len <= 0 )
        return CString();
    return CString( buf, len );
}
