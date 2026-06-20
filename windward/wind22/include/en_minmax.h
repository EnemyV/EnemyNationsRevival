#ifndef EN_MINMAX_H
#define EN_MINMAX_H
//---------------------------------------------------------------------------
// en_minmax.h — the project's global min/max function templates, in ONE place.
//
// Legacy code calls unqualified min(a,b)/max(a,b) (originally <windows.h>'s
// textual macros) and the cross-platform port calls ::min(...)/::max(...).
// We can't use macros: libstdc++ headers (<vector>, <map>, ...) call unqualified
// max()/min() internally and the macros mangle them (MSVC's STL tolerates it;
// libstdc++ does not). So provide GLOBAL function templates: unqualified
// min/max bind here (mixed int/unsigned handled via common_type), while
// std::min/std::max call sites — and the STL's own internal unqualified usage —
// bind to std:: and are untouched.
//
// Single source of truth, included by BOTH:
//   - windward/wind22/include/win32_compat.h  (POSIX: Linux / macOS)
//   - enations_latest/src/stdafx.h            (MSVC, under #ifdef _WIN32)
//
// On MSVC this MUST be included AFTER NOMINMAX + <windows.h>, otherwise
// <windows.h> defines max/min as textual macros that mangle these definitions.
//---------------------------------------------------------------------------
#include <algorithm>
#include <type_traits>

template <class A, class B>
inline typename std::common_type<A, B>::type min(A a, B b) {
    typedef typename std::common_type<A, B>::type R;
    return ((R)a < (R)b) ? (R)a : (R)b;
}
template <class A, class B>
inline typename std::common_type<A, B>::type max(A a, B b) {
    typedef typename std::common_type<A, B>::type R;
    return ((R)a > (R)b) ? (R)a : (R)b;
}

#endif // EN_MINMAX_H
