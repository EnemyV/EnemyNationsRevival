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

int EnMessageBox( const char* text, unsigned int type, unsigned int /*helpId*/ )
{
    return ::MessageBoxA( NULL, text ? text : "", "Second Chance", type );
}

int EnMessageBox( unsigned int idText, unsigned int type, unsigned int /*helpId*/ )
{
    std::string s = EnLoadStdString( idText );
    return ::MessageBoxA( NULL, s.c_str(), "Second Chance", type );
}

int EnMessageBoxOnce( const char* text, unsigned int type, const char* section, const char* entry, int iDefault )
{
    // Already dismissed? Return the default silently.
    if ( EnGetProfileInt( section, entry, 0 ) != 0 )
        return iDefault;

    UINT uType = MB_TASKMODAL;

    // Map button style
    if ( type & MB_YESNOCANCEL )
        uType |= MB_YESNOCANCEL;
    else if ( type & MB_YESNO )
        uType |= MB_YESNO;
    else
        uType |= MB_OK;

    // Map icon style
    if ( type & MB_ICONSTOP )
        uType |= MB_ICONSTOP;
    else if ( type & MB_ICONEXCLAMATION )
        uType |= MB_ICONEXCLAMATION;
    else if ( type & MB_ICONINFORMATION )
        uType |= MB_ICONINFORMATION;

    int iRtn = ::MessageBoxA( NULL, text ? text : "", "Second Chance", uType );

    // Map OK -> YES for callers that expect IDYES (matches original CDlgMsg behavior)
    if ( iRtn == IDOK )
        iRtn = IDYES;

    // Suppress future prompts
    EnWriteProfileInt( section, entry, 1 );

    return iRtn;
}

int EnMessageBoxOnce( unsigned int idText, unsigned int type, const char* section, const char* entry, int iDefault )
{
    // Already dismissed? Return the default silently (skip the LoadString too).
    if ( EnGetProfileInt( section, entry, 0 ) != 0 )
        return iDefault;

    std::string s = EnLoadStdString( idText );
    return EnMessageBoxOnce( s.c_str(), type, section, entry, iDefault );
}

// --- std::string variants (Phase 5a prep) ---

std::string EnGetProfileStdString( const char* section, const char* entry, const char* default_val )
{
    if ( !section || !entry )
        return default_val ? default_val : "";

    char keyPath[ 512 ];
    _snprintf_s( keyPath, sizeof( keyPath ), _TRUNCATE, "%s\\%s", "Software\\Second Chance\\Second Chance", section );

    HKEY hKey = NULL;
    if ( ::RegOpenKeyExA( HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey ) != ERROR_SUCCESS )
        return default_val ? default_val : "";

    // Query size first
    DWORD type = 0;
    DWORD size = 0;
    LONG  rc   = ::RegQueryValueExA( hKey, entry, NULL, &type, NULL, &size );
    if ( rc != ERROR_SUCCESS || ( type != REG_SZ && type != REG_EXPAND_SZ ) || size == 0 )
    {
        ::RegCloseKey( hKey );
        return default_val ? default_val : "";
    }

    std::string result( size, '\0' );
    rc = ::RegQueryValueExA( hKey, entry, NULL, &type, (LPBYTE)&result[0], &size );
    ::RegCloseKey( hKey );

    if ( rc != ERROR_SUCCESS )
        return default_val ? default_val : "";

    // Remove null terminator if present
    if ( !result.empty() && result.back() == '\0' )
        result.pop_back();

    return result;
}

std::string EnLoadStdString( unsigned int id )
{
    char buf[ 1024 ];
    HMODULE hModule = ::GetModuleHandleA( NULL );
    int len = ::LoadStringA( hModule, id, buf, (int)sizeof( buf ) );
    if ( len <= 0 )
        return std::string();
    return std::string( buf, len );
}

#include <cstdarg>

static std::string InsertCommas( std::string s )
{
    // Insert thousand-separators into the decimal digits
    size_t start = ( !s.empty() && ( s[0] == '-' || s[0] == '+' ) ) ? 1 : 0;
    size_t n = s.length();
    if ( n <= start + 3 )
        return s;
    size_t pos = n;
    while ( pos > start + 3 )
    {
        pos -= 3;
        s.insert( s.begin() + pos, ',' );
    }
    return s;
}

std::string IntToStr( int iNum, int iRadix, bool bComma )
{
    char buf[34];
    _itoa_s( iNum, buf, iRadix );
    std::string s( buf );
    if ( bComma && iRadix == 10 )
        s = InsertCommas( s );
    return s;
}

std::string LongToStr( long lNum, int iRadix, bool bComma )
{
    char buf[34];
    _ltoa_s( lNum, buf, iRadix );
    std::string s( buf );
    if ( bComma && iRadix == 10 )
        s = InsertCommas( s );
    return s;
}

std::string strPrintf( const char* fmt, ... )
{
    if ( !fmt )
        return std::string();

    std::string result( fmt );

    va_list va;
    va_start( va, fmt );

    // Replace %1, %2, ... with corresponding variadic args (same as csPrintf)
    int iStrOn = 1;
    bool bFound;
    do
    {
        bFound = false;
        size_t pos = 0;
        while ( ( pos = result.find( '%', pos ) ) != std::string::npos )
        {
            // Check if the digit(s) after % match iStrOn
            size_t dStart = pos + 1;
            if ( dStart < result.size() && isdigit( (unsigned char)result[dStart] ) )
            {
                int num = atoi( &result[dStart] );
                if ( num == iStrOn )
                {
                    bFound = true;
                    const char* pStr = va_arg( va, const char* );
                    if ( !pStr ) pStr = "";

                    // Count digits in the number
                    size_t dLen = 1;
                    while ( dStart + dLen < result.size() && isdigit( (unsigned char)result[dStart + dLen] ) )
                        dLen++;

                    result.replace( pos, 1 + dLen, pStr );
                    iStrOn++;
                    pos = 0;  // restart scan
                    break;
                }
            }
            pos++;
        }
    } while ( bFound );

    va_end( va );
    return result;
}
