#ifndef __EN_SETTINGS_H__
#define __EN_SETTINGS_H__

//---------------------------------------------------------------------------
// EnSettings.h — Game-side settings & string-table shim
//
// Replaces theApp.GetProfileInt / WriteProfileInt / GetProfileString /
// WriteProfileString and CString::LoadString call sites with thin Win32-only
// helpers, so the game no longer depends on CWinApp for settings or
// resource-string lookup.
//
// Registry path: HKCU\Software\Second Chance\Second Chance\<section>\<entry>
// (matches the existing CWinApp::SetRegistryKey("Second Chance") layout, so
// previously-persisted settings continue to load correctly.)
//---------------------------------------------------------------------------

// CString is still in use throughout the game (Phase 5 will replace it).
// Including <afx.h> here gives us the right typedef without depending on
// the precompiled header for callers.
#include <afx.h>
#include <string>

int     EnGetProfileInt    ( const char* section, const char* entry, int default_val );
void    EnWriteProfileInt  ( const char* section, const char* entry, int value );

CString EnGetProfileString ( const char* section, const char* entry, const char* default_val = 0 );
void    EnWriteProfileString( const char* section, const char* entry, const char* value );

// Returns the requested resource string, or an empty CString if the id was
// not found in the .RC string table.
CString EnLoadString( unsigned int id );

// --- std::string variants (Phase 5a prep) ---
// These allow callers to migrate away from CString incrementally.
std::string EnGetProfileStdString( const char* section, const char* entry, const char* default_val = "" );
std::string EnLoadStdString( unsigned int id );

// std::string formatting helper — replaces wind22's csPrintf(CString*, ...).
// Performs positional substitution: %1, %2, etc. are replaced by the
// corresponding variadic const char* arguments (same semantics as csPrintf).
std::string strPrintf( const char* fmt, ... );

// Win32 MessageBoxA wrapper that does not depend on CWinApp/MFC. The third
// "helpId" parameter is accepted for source-compatibility with AfxMessageBox
// call sites and is otherwise ignored on modern Windows. The title used is
// "Second Chance", matching what MFC's AfxMessageBox produced via
// CWinApp::m_pszAppName before this shim.
int EnMessageBox( const char*  text,   unsigned int type = 0, unsigned int helpId = 0 );
int EnMessageBox( unsigned int idText, unsigned int type = 0, unsigned int helpId = 0 );

// "Show once" message box — checks registry key <section>\<entry> first;
// if the user already dismissed this warning the default is returned silently.
// After the user responds, the entry is set so the warning won't appear again.
// Replaces wind22's CDlgMsg::MsgBox pattern.
int EnMessageBoxOnce( const char*  text,   unsigned int type, const char* section, const char* entry, int iDefault = IDYES );
int EnMessageBoxOnce( unsigned int idText, unsigned int type, const char* section, const char* entry, int iDefault = IDYES );

#endif // __EN_SETTINGS_H__
