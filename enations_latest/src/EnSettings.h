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

int     EnGetProfileInt    ( const char* section, const char* entry, int default_val );
void    EnWriteProfileInt  ( const char* section, const char* entry, int value );

CString EnGetProfileString ( const char* section, const char* entry, const char* default_val = 0 );
void    EnWriteProfileString( const char* section, const char* entry, const char* value );

// Returns the requested resource string, or an empty CString if the id was
// not found in the .RC string table.
CString EnLoadString( unsigned int id );

// Win32 MessageBoxA wrapper that does not depend on CWinApp/MFC. The third
// "helpId" parameter is accepted for source-compatibility with AfxMessageBox
// call sites and is otherwise ignored on modern Windows. The title used is
// "Second Chance", matching what MFC's AfxMessageBox produced via
// CWinApp::m_pszAppName before this shim.
int EnMessageBox( const char*  text,   unsigned int type = 0, unsigned int helpId = 0 );
int EnMessageBox( unsigned int idText, unsigned int type = 0, unsigned int helpId = 0 );

#endif // __EN_SETTINGS_H__
