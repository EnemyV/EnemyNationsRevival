#ifndef __SAVECOMPAT_H__
#define __SAVECOMPAT_H__

//---------------------------------------------------------------------------
// SaveCompat.h — std::string ↔ CArchive bridge for Phase 5c migration.
//
// Adds operator<<(CArchive&, const std::string&) and operator>>(CArchive&,
// std::string&) overloads that write the SAME binary format CString uses,
// by routing through a temporary CString. This guarantees save-file
// binary compatibility while individual CString members are migrated to
// std::string one at a time.
//
// Cost: one extra string copy per serialize boundary. Acceptable —
// serialization happens at game-save / game-load, not in a hot path.
//
// Pulled in by player.cpp, racedata.cpp, ipcplay.cpp, minerals.cpp, and
// other files whose Serialize methods touch migrated std::string fields.
//
// Phase 5c will eventually replace CArchive entirely; this header is the
// scaffolding that lets the migration proceed incrementally.
//---------------------------------------------------------------------------

// Phase 1g step 2: under the stub gates CArchive/CString come from
// mfc_compat.h via stdafx.h; afx.h is gone. The operator bodies don't
// care which CArchive/CString implementation is providing them.
#if defined(ENATIONS_USE_STUB_WND) && defined(ENATIONS_USE_STUB_APP)
// mfc_compat.h types are already in scope via the precompiled stdafx.h
#else
#include <afx.h>     // CArchive, CString
#endif
#include <string>

inline CArchive& operator<<( CArchive& ar, const std::string& s )
{
    CString cs( s.c_str() );
    ar << cs;
    return ar;
}

inline CArchive& operator>>( CArchive& ar, std::string& s )
{
    CString cs;
    ar >> cs;
    s.assign( (LPCSTR)cs );
    return ar;
}

#endif // __SAVECOMPAT_H__
