//---------------------------------------------------------------------------
// win32_compat.h — Win32-on-POSIX shim for the Linux build of Enemy Nations.
//
// This header provides the subset of <windows.h> (types, constants, macros,
// and function declarations) that the SDL2-migrated codebase still references,
// implemented for Linux/gcc. It is included by stdafx.h / mfc_compat.h in place
// of <windows.h> when _WIN32 is NOT defined. The Windows/MSVC build never sees
// this file — there, the real <windows.h> is used unchanged.
//
// Design rules:
//   * LP64 correctness. On Win64 `long` is 32-bit; on Linux x86-64 it is 64-bit.
//     Every Win32 width-named type therefore maps to a FIXED-WIDTH integer
//     (DWORD->uint32_t, LONG->int32_t, ...) — never to `long` — so on-disk
//     struct layouts and #pragma pack records match the Windows build.
//   * Handle/pointer-width types (HANDLE/HWND/WPARAM/...) map to pointer width,
//     which is 64-bit on both Win64 and Linux x86-64 — so those agree already.
//   * Calling-convention / decoration keywords compile away: x86-64 has a single
//     calling convention, so __stdcall/WINAPI/etc. become empty.
//
// The implementations live in windward/wind22/src/win32_compat.cpp.
//---------------------------------------------------------------------------

#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#ifdef _WIN32
#error "win32_compat.h is the Linux shim and must not be compiled on Windows"
#endif

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>      // tolower / toupper
#include <strings.h>   // strcasecmp / strncasecmp
#include <unistd.h>    // getpid / getcwd
#include <climits>     // PATH_MAX

//===========================================================================
// 1. Calling-convention / decoration keywords  → compile away on x86-64 Linux.
//===========================================================================
#define __stdcall
#define __cdecl
#define __fastcall
#define __thiscall
#define WINAPI
#define WINAPIV
#define APIENTRY
#define CALLBACK
#define PASCAL
#define CDECL
#define FAR
#define NEAR
#define far
#define near
#define IN
#define OUT
#define OPTIONAL
#ifndef CONST
#define CONST const
#endif

// __declspec(...) collapses to nothing, EXCEPT the two forms with real meaning.
// (If the tree turns out to use __declspec(thread)/__declspec(align(n)), switch
// those specific sites to thread_local / alignas(n) rather than relying on this.)
#define __declspec(x)
#define __forceinline inline __attribute__((always_inline))
#define _inline inline
#define _cdecl
#ifndef __pragma
#define __pragma(x)
#endif
#define __based(x)
#ifndef _CRTIMP
#define _CRTIMP
#endif
#ifndef WINBASEAPI
#define WINBASEAPI
#endif
#ifndef WINUSERAPI
#define WINUSERAPI
#endif
#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif

//===========================================================================
// 2. Fundamental types (LP64-correct: fixed widths, never `long`).
//===========================================================================
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef uint32_t            DWORD;      // NOT unsigned long (LP64!)
typedef uint32_t            ULONG;
typedef int32_t             LONG;       // NOT long (LP64!)
typedef uint16_t            USHORT;
typedef int16_t             SHORT;
typedef unsigned int        UINT;
typedef int                 INT;
typedef unsigned char       UCHAR;
typedef char                CHAR;
typedef float               FLOAT;
typedef int64_t             LONGLONG;
typedef uint64_t            ULONGLONG;
typedef uint64_t            DWORDLONG;
// MSVC spells these as built-in keywords; clang under -fms-extensions ALSO
// treats __int64/__int32/__int16/__int8 as builtin type keywords, so typedef'ing
// them collides ("cannot combine with previous type-name"). gcc does not, so it
// still needs the typedefs. Guard the clang case out.
#ifndef __clang__
typedef int64_t             __int64;        // some sources still spell it this way
typedef int32_t             __int32;
typedef int16_t             __int16;
typedef int8_t              __int8;
#endif
typedef uint32_t            DWORD32;
typedef uint64_t            DWORD64;
typedef uint8_t             BOOLEAN;

// Pointer-width integer types (64-bit on Win64 and Linux x86-64 alike).
typedef intptr_t            INT_PTR;
typedef uintptr_t           UINT_PTR;
typedef intptr_t            LONG_PTR;
typedef uintptr_t           ULONG_PTR;
typedef uintptr_t           DWORD_PTR;
typedef intptr_t            SSIZE_T;
typedef size_t              SIZE_T;

typedef int32_t             HRESULT;
typedef DWORD               COLORREF;
typedef WORD                ATOM;
typedef DWORD               LCID;
typedef DWORD               LANGID;

// Pointers to primitives.
typedef void*               PVOID;
typedef void*               LPVOID;
typedef const void*         LPCVOID;
typedef BOOL*               LPBOOL;
typedef BYTE*               LPBYTE;
typedef BYTE*               PBYTE;
typedef int*                LPINT;
typedef WORD*               LPWORD;
typedef DWORD*              LPDWORD;
typedef DWORD*              PDWORD;
typedef LONG*               LPLONG;
typedef UINT*               PUINT;
typedef char*               LPSTR;
typedef const char*         LPCSTR;
typedef char*               PSTR;
typedef const char*         PCSTR;
typedef char*               LPTSTR;     // ANSI build: TCHAR == char
typedef const char*         LPCTSTR;
typedef char                TCHAR;
typedef unsigned short      WCHAR;
typedef WCHAR*              LPWSTR;
typedef const WCHAR*        LPCWSTR;

// Message / window parameter types (pointer width).
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;

//===========================================================================
// 3. Handles. All opaque pointer-width values.
//===========================================================================
#define DECLARE_HANDLE(name) typedef void* name
typedef void*               HANDLE;
typedef void**              LPHANDLE;
typedef void*               HWND;
typedef void*               HDC;
typedef void*               HINSTANCE;
typedef void*               HMODULE;
typedef void*               HBITMAP;
typedef void*               HICON;
typedef void*               HCURSOR;
typedef void*               HBRUSH;
typedef void*               HPEN;
typedef void*               HFONT;
typedef void*               HPALETTE;
typedef void*               HMENU;
typedef void*               HGDIOBJ;
typedef void*               HGLOBAL;
typedef void*               HLOCAL;
typedef void*               HRGN;
typedef void*               HKEY;
typedef HKEY*               PHKEY;
typedef void*               HFILE;
typedef void*               HACCEL;
typedef void*               HMONITOR;
typedef void*               HKL;
typedef void*               SC_HANDLE;
typedef void*               HWAVEOUT;
typedef void*               HWAVEIN;

//===========================================================================
// 4. Common constants.
//===========================================================================
#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef VOID
#define VOID void
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif
// Standard <windows.h> "already included" markers some headers gate on.
#define _WINDOWS_
#define _INC_WINDOWS

#define INFINITE            0xFFFFFFFFu
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define WAIT_OBJECT_0       0x00000000u
#define WAIT_ABANDONED      0x00000080u
#define WAIT_TIMEOUT        0x00000102u
#define WAIT_FAILED         0xFFFFFFFFu

// Thread priorities (informational; mapped/ignored by the shim).
#define THREAD_PRIORITY_IDLE          (-15)
#define THREAD_PRIORITY_LOWEST        (-2)
#define THREAD_PRIORITY_BELOW_NORMAL  (-1)
#define THREAD_PRIORITY_NORMAL        0
#define THREAD_PRIORITY_ABOVE_NORMAL  1
#define THREAD_PRIORITY_HIGHEST       2
#define THREAD_PRIORITY_TIME_CRITICAL 15
#define CREATE_SUSPENDED              0x00000004u

// CreateFile access / share / disposition.
#define GENERIC_READ        0x80000000u
#define GENERIC_WRITE       0x40000000u
#define FILE_SHARE_READ     0x00000001u
#define FILE_SHARE_WRITE    0x00000002u
#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5
#define FILE_ATTRIBUTE_NORMAL      0x00000080u
#define FILE_ATTRIBUTE_DIRECTORY   0x00000010u
#define FILE_ATTRIBUTE_READONLY    0x00000001u
#define INVALID_FILE_ATTRIBUTES    0xFFFFFFFFu
#define FILE_BEGIN          0
#define FILE_CURRENT        1
#define FILE_END            2
#define INVALID_SET_FILE_POINTER 0xFFFFFFFFu

// MessageBox style/return.
#define MB_OK               0x00000000u
#define MB_OKCANCEL         0x00000001u
#define MB_YESNO            0x00000004u
#define MB_ICONERROR        0x00000010u
#define MB_ICONHAND         0x00000010u
#define MB_ICONQUESTION     0x00000020u
#define MB_ICONEXCLAMATION  0x00000030u
#define MB_ICONINFORMATION  0x00000040u
#define MB_ICONSTOP         0x00000010u
#define MB_SYSTEMMODAL      0x00001000u
#define MB_TASKMODAL        0x00002000u
#define MB_TOPMOST          0x00040000u
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7
#define MB_ABORTRETRYIGNORE 0x00000002u
#define MB_YESNOCANCEL      0x00000003u
#define MB_RETRYCANCEL      0x00000005u
#define MB_DEFBUTTON1       0x00000000u
#define MB_DEFBUTTON2       0x00000100u
#define MB_DEFBUTTON3       0x00000200u
#define MB_DEFMASK          0x00000F00u
#define S_OK                0
#define S_FALSE             1
// Static-control styles.
#define SS_LEFT             0x00000000u
#define SS_CENTER           0x00000001u
#define SS_RIGHT            0x00000002u
#define SS_NOPREFIX         0x00000080u
#define DT_BOTTOM           0x0008u
#define DT_TOP              0x0000u
#define GCL_STYLE           (-26)
#define CS_OWNDC            0x0020u
#define CS_HREDRAW          0x0002u
#define CS_VREDRAW          0x0001u
// Drive types (GetDriveType).
#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6
// OPENFILENAME flags.
#define OFN_READONLY         0x00000001u
#define OFN_OVERWRITEPROMPT  0x00000002u
#define OFN_HIDEREADONLY     0x00000004u
#define OFN_PATHMUSTEXIST    0x00000800u
#define OFN_FILEMUSTEXIST    0x00001000u
#define OFN_CREATEPROMPT     0x00002000u
#define _MAX_DRIVE 3
#define _MAX_DIR   256
#define _MAX_FNAME 256
#define _MAX_EXT   256

// ShowWindow nCmdShow.
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_RESTORE          9
#define SW_SHOWDEFAULT      10

// Registry.
#define HKEY_CLASSES_ROOT   ((HKEY)(uintptr_t)0x80000000)
#define HKEY_CURRENT_USER   ((HKEY)(uintptr_t)0x80000001)
#define HKEY_LOCAL_MACHINE  ((HKEY)(uintptr_t)0x80000002)
#define HKEY_USERS          ((HKEY)(uintptr_t)0x80000003)
#define KEY_READ            0x20019u
#define KEY_WRITE           0x20006u
#define KEY_ALL_ACCESS      0xF003Fu
#define REG_NONE            0u
#define REG_SZ              1u
#define REG_EXPAND_SZ       2u
#define REG_BINARY          3u
#define REG_DWORD           4u
#define ERROR_SUCCESS       0L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_MORE_DATA     234L
#define REG_OPTION_NON_VOLATILE 0u

//===========================================================================
// 5. Word/byte extraction + helper macros.
//===========================================================================
#define MAKEWORD(a, b)  ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)  ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)       ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)       ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)       ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)       ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h) ((LRESULT)(DWORD)MAKELONG(l, h))
#define MAKEINTRESOURCE(i)  ((LPSTR)(uintptr_t)(WORD)(i))
#define MAKEINTRESOURCEA(i) ((LPSTR)(uintptr_t)(WORD)(i))
#define IS_INTRESOURCE(r)   (((uintptr_t)(r) >> 16) == 0)
#define GET_X_LPARAM(lp)  ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)  ((int)(short)HIWORD(lp))
#define RGB(r, g, b)    ((COLORREF)(((BYTE)(r)) | (((WORD)((BYTE)(g))) << 8) | (((DWORD)((BYTE)(b))) << 16)))
#define GetRValue(c)    ((BYTE)((c) & 0xff))
#define GetGValue(c)    ((BYTE)(((c) >> 8) & 0xff))
#define GetBValue(c)    ((BYTE)(((c) >> 16) & 0xff))

// min/max: MSVC's <windows.h> defines these as textual macros (NOMINMAX is not
// set in this project) and ~19 files call them unqualified, sometimes with mixed
// int/unsigned args. We CANNOT use macros on Linux: libstdc++ headers (<vector>,
// <map>, ...) call unqualified max()/min() internally and get mangled — MSVC's
// STL tolerates the macros, libstdc++ does not. Instead provide GLOBAL function
// templates: unqualified min(a,b)/max(a,b) bind to these (mixed types handled via
// common_type), while std::min(...) call sites and std's own internal usage bind
// to std:: and are untouched. (A TU doing `using namespace std;` + unqualified
// min/max would be ambiguous — none currently do; fix per-site if one appears.)
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
// MSVC intrinsics used directly in scenario.cpp / sprtinit.cpp / vehoppo.cpp.
// Safe as macros: nothing in libstdc++ spells __min/__max.
#ifndef __max
#define __max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ZeroMemory
#define ZeroMemory(p, n)   memset((p), 0, (n))
#endif
#ifndef CopyMemory
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#endif
#ifndef MoveMemory
#define MoveMemory(d, s, n) memmove((d), (s), (n))
#endif
#ifndef FillMemory
#define FillMemory(p, n, v) memset((p), (v), (n))
#endif

//===========================================================================
// 6. Geometry structs (POINT/SIZE/RECT/...) — mfc_compat.h's CPoint/CSize/
//    CRect derive from these, so the layout must match Win32 exactly.
//===========================================================================
typedef struct tagPOINT { LONG x; LONG y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagPOINTS { SHORT x; SHORT y; } POINTS;
typedef struct tagSIZE  { LONG cx; LONG cy; } SIZE, *PSIZE, *LPSIZE;
typedef struct tagRECT  { LONG left; LONG top; LONG right; LONG bottom; } RECT, *PRECT, *LPRECT;
typedef const RECT*     LPCRECT;

typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *PMSG, *LPMSG;

typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _SYSTEMTIME {
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef union _LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; };   // anonymous (—fms-extensions): .LowPart/.HighPart direct
    struct { DWORD LowPart; LONG HighPart; } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct { DWORD LowPart; DWORD HighPart; };
    struct { DWORD LowPart; DWORD HighPart; } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD  nLength;
    LPVOID lpSecurityDescriptor;
    BOOL   bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

// CRITICAL_SECTION is opaque on Linux; the shim allocates the real object.
// Size is generous so any accidental by-value copy doesn't truncate.
typedef struct _RTL_CRITICAL_SECTION { void* opaque; } CRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    char  cFileName[MAX_PATH];
    char  cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

//===========================================================================
// 7. Window-message and input constants (the subset the game uses).
//===========================================================================
#define WM_NULL            0x0000
#define WM_CREATE          0x0001
#define WM_DESTROY         0x0002
#define WM_MOVE            0x0003
#define WM_SIZE            0x0005
#define WM_ACTIVATE        0x0006
#define WM_SETFOCUS        0x0007
#define WM_KILLFOCUS       0x0008
#define WM_ENABLE          0x000A
#define WM_PAINT           0x000F
#define WM_CLOSE           0x0010
#define WM_QUIT            0x0012
#define WM_ERASEBKGND      0x0014
#define WM_SHOWWINDOW      0x0018
#define WM_ACTIVATEAPP     0x001C
#define WM_SETCURSOR       0x0020
#define WM_MOUSEACTIVATE   0x0021
#define WM_GETMINMAXINFO   0x0024
#define WM_NCCALCSIZE      0x0083
#define WM_NCPAINT         0x0085
#define WM_NCACTIVATE      0x0086
#define WM_KEYDOWN         0x0100
#define WM_KEYUP           0x0101
#define WM_CHAR            0x0102
#define WM_SYSKEYDOWN      0x0104
#define WM_SYSKEYUP        0x0105
#define WM_SYSCHAR         0x0106
#define WM_COMMAND         0x0111
#define WM_SYSCOMMAND      0x0112
#define WM_TIMER           0x0113
#define WM_HSCROLL         0x0114
#define WM_VSCROLL         0x0115
#define WM_MOUSEMOVE       0x0200
#define WM_LBUTTONDOWN     0x0201
#define WM_LBUTTONUP       0x0202
#define WM_LBUTTONDBLCLK   0x0203
#define WM_RBUTTONDOWN     0x0204
#define WM_RBUTTONUP       0x0205
#define WM_RBUTTONDBLCLK   0x0206
#define WM_MBUTTONDOWN     0x0207
#define WM_MBUTTONUP       0x0208
#define WM_MBUTTONDBLCLK   0x0209
#define WM_MOUSEWHEEL      0x020A
#define WM_USER            0x0400
#define WM_APP             0x8000
#define WM_KEYFIRST        0x0100
#define WM_KEYLAST         0x0108
#define WM_DISPLAYCHANGE   0x007E
#define WA_INACTIVE        0
#define WA_ACTIVE          1
#define WA_CLICKACTIVE     2
#define WINDING            2
#define ALTERNATE          1
#define SM_CXSIZEFRAME     32
#define SM_CYSIZEFRAME     33
#define SM_CXFRAME         32
#define SM_CYFRAME         33
#define TIME_ONESHOT       0x0000u
#define TIME_PERIODIC      0x0001u
#define TIME_CALLBACK_EVENT_SET 0x0010u
#define TIME_CALLBACK_FUNCTION  0x0000u

// Mouse/key virtual-state flags (wParam of mouse messages).
#define MK_LBUTTON         0x0001
#define MK_RBUTTON         0x0002
#define MK_SHIFT           0x0004
#define MK_CONTROL         0x0008
#define MK_MBUTTON         0x0010
#define WHEEL_DELTA        120

// Virtual key codes (the subset referenced).
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_CANCEL    0x03
#define VK_MBUTTON   0x04
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_PAUSE     0x13
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B

//===========================================================================
// 8. CRT spelling differences (MSVC underscored names → POSIX).
//===========================================================================
#define _stricmp     strcasecmp
#define stricmp      strcasecmp
#define _strnicmp    strncasecmp
#define strnicmp     strncasecmp
#define _strdup      strdup
#define wsprintf     sprintf
#define wsprintfA    sprintf
inline char* _strlwr(char* s) { if (s) for (char* p = s; *p; ++p) *p = (char)tolower((unsigned char)*p); return s; }
inline char* _strupr(char* s) { if (s) for (char* p = s; *p; ++p) *p = (char)toupper((unsigned char)*p); return s; }
inline char* _fullpath(char* out, const char* in, size_t size) {
    char tmp[4096]; if (!realpath(in, tmp)) { if (out && size) { strncpy(out, in, size - 1); out[size - 1] = 0; } return out; }
    if (!out) return strdup(tmp);
    strncpy(out, tmp, size - 1); out[size - 1] = 0; return out;
}
#define _vsnprintf   vsnprintf
#define _snprintf    snprintf
#define _unlink      unlink
#define _access      access
#define _getcwd      getcwd
#define _stat        stat
#ifndef _MAX_PATH
#define _MAX_PATH    MAX_PATH
#endif

//===========================================================================
// 8b. Interlocked atomics — width-agnostic inline templates so callers can pass
//     either int32_t* (DWORD/LONG) or 64-bit long* on LP64. (C++ linkage.)
//===========================================================================
template <class T> inline T InterlockedIncrement(T volatile* p)            { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
template <class T> inline T InterlockedDecrement(T volatile* p)            { return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
template <class T> inline T InterlockedExchange(T volatile* p, T v)        { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }
template <class T> inline T InterlockedExchangeAdd(T volatile* p, T v)     { return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST); }
template <class T> inline T InterlockedCompareExchange(T volatile* p, T exch, T comp) {
    __atomic_compare_exchange_n(p, &comp, exch, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return comp;
}

//===========================================================================
// 8c. Debug + secure-CRT shims.
//===========================================================================
// TRAP()/__debugbreak() is `int 3` on Windows — a breakpoint, non-fatal under a
// debugger. On Linux make it a non-fatal log (NOT __builtin_trap, which raises
// SIGILL and kills the process) so debug checkpoints don't abort the bring-up.
#define __debugbreak() ((void)fputs("[TRAP/__debugbreak]\n", stderr))
#define _TRUNCATE ((size_t)-1)
// _snprintf_s(buf, bufsize, count, fmt, ...) → snprintf (count/_TRUNCATE ignored;
// snprintf already truncates safely and null-terminates).
#define _snprintf_s(buf, bufsize, count, ...) snprintf((buf), (bufsize), __VA_ARGS__)
// sprintf_s has TWO MSVC forms — the explicit-size form
//   sprintf_s(buf, size, fmt, ...)
// and the "secure template overload" that deduces size from a fixed array
//   sprintf_s(char (&buf)[N], fmt, ...)
// Both are used throughout the tree, so we cannot model this with a macro (a macro
// would shove the format string into the size slot for the template form, leaving
// the first vararg as the format — e.g. sprintf_s(buf,"%d%%",0) → snprintf with a
// NULL format → crash). Provide real overloads instead; resolution is unambiguous
// because const char* (the format) does not implicitly convert to size_t.
inline int sprintf_s(char* buf, size_t size, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(buf, size, fmt ? fmt : "", ap);
    va_end(ap);
    return r;
}
template <size_t N> inline int sprintf_s(char (&buf)[N], const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(buf, N, fmt ? fmt : "", ap);
    va_end(ap);
    return r;
}
inline int strncpy_s(char* d, size_t dsz, const char* s, size_t cnt) {
    if (!d || dsz == 0) return 22;
    size_t n = (cnt == (size_t)-1 || cnt >= dsz) ? dsz - 1 : cnt;
    strncpy(d, s ? s : "", n); d[n] = '\0'; return 0;
}
inline int strcpy_s(char* d, size_t dsz, const char* s) {
    if (!d || dsz == 0) return 22;
    strncpy(d, s ? s : "", dsz - 1); d[dsz - 1] = '\0'; return 0;
}
inline int strcat_s(char* d, size_t dsz, const char* s) {
    if (!d || dsz == 0) return 22;
    size_t dl = strnlen(d, dsz); if (dl >= dsz) return 22;
    strncat(d, s ? s : "", dsz - dl - 1); return 0;
}
template <size_t N> inline int strncpy_s(char (&d)[N], const char* s, size_t cnt) { return strncpy_s(d, N, s, cnt); }
template <size_t N> inline int strcpy_s(char (&d)[N], const char* s) { return strcpy_s(d, N, s); }
#define vsnprintf_s(buf, bufsize, count, fmt, ap) vsnprintf((buf), (bufsize), (fmt), (ap))
#define _vsnprintf_s(buf, bufsize, count, fmt, ap) vsnprintf((buf), (bufsize), (fmt), (ap))

//===========================================================================
// 8d. GDI / USER types + constants used by mfc_compat.h's CWnd/CDC stubs.
//===========================================================================
#define LF_FACESIZE 32
typedef struct tagBITMAP { LONG bmType, bmWidth, bmHeight, bmWidthBytes; WORD bmPlanes, bmBitsPixel; LPVOID bmBits; } BITMAP, *LPBITMAP;
typedef struct tagRGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD, *LPRGBQUAD;
typedef struct tagRGBTRIPLE { BYTE rgbtBlue, rgbtGreen, rgbtRed; } RGBTRIPLE;
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount;
    DWORD biCompression, biSizeImage; LONG biXPelsPerMeter, biYPelsPerMeter;
    DWORD biClrUsed, biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
typedef struct tagBITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; } BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;
typedef struct tagPALETTEENTRY { BYTE peRed, peGreen, peBlue, peFlags; } PALETTEENTRY, *LPPALETTEENTRY;
typedef struct tagLOGPALETTE { WORD palVersion, palNumEntries; PALETTEENTRY palPalEntry[1]; } LOGPALETTE, *LPLOGPALETTE;
#define BI_RGB 0
#define CBM_INIT 0x04u
typedef VOID (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER { WORD bfType; DWORD bfSize; WORD bfReserved1, bfReserved2; DWORD bfOffBits; } BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
#pragma pack(pop)
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
typedef struct tagDRAWITEMSTRUCT {
    UINT CtlType, CtlID, itemID, itemAction, itemState;
    HWND hwndItem; HDC hDC; RECT rcItem; ULONG_PTR itemData;
} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT;
typedef struct tagMEASUREITEMSTRUCT {
    UINT CtlType, CtlID, itemID, itemWidth, itemHeight; ULONG_PTR itemData;
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;
typedef struct tagCOMPAREITEMSTRUCT {
    UINT CtlType, CtlID; HWND hwndItem; UINT itemID1; ULONG_PTR itemData1;
    UINT itemID2; ULONG_PTR itemData2; DWORD dwLocaleId;
} COMPAREITEMSTRUCT, *LPCOMPAREITEMSTRUCT;
typedef struct tagDELETEITEMSTRUCT {
    UINT CtlType, CtlID, itemID; HWND hwndItem; ULONG_PTR itemData;
} DELETEITEMSTRUCT, *LPDELETEITEMSTRUCT;
typedef void* HHOOK;
typedef LRESULT (*HOOKPROC)(int, WPARAM, LPARAM);
typedef struct _ICONINFO { BOOL fIcon; DWORD xHotspot, yHotspot; HBITMAP hbmMask, hbmColor; } ICONINFO, *PICONINFO;
typedef struct _CURSORINFO { DWORD cbSize, flags; HCURSOR hCursor; POINT ptScreenPos; } CURSORINFO, *PCURSORINFO;
typedef int (*_CRT_ALLOC_HOOK)(int, void*, size_t, int, long, const unsigned char*);
#define _HOOK_ALLOC   1
#define _HOOK_REALLOC 2
#define _HOOK_FREE    3
#ifndef _S_IFREG
#define _S_IFREG 0100000
#endif
#ifndef _S_IFDIR
#define _S_IFDIR 0040000
#endif
// VirtualAlloc flags.
#define MEM_COMMIT   0x1000u
#define MEM_RESERVE  0x2000u
#define MEM_RELEASE  0x8000u
#define PAGE_READWRITE 0x04u
#define SB_LINELEFT  0
#define SB_LINERIGHT 1
#define SB_PAGELEFT  2
#define SB_PAGERIGHT 3
#define SB_LEFT      6
#define SB_RIGHT     7
#define SBS_HORZ     0x0000u
#define SBS_VERT     0x0001u
// Structured-exception types (for crash-handler signatures; never actually
// raised on Linux — the handlers are gated / unused).
typedef struct _EXCEPTION_RECORD {
    DWORD ExceptionCode, ExceptionFlags; struct _EXCEPTION_RECORD* ExceptionRecord;
    void* ExceptionAddress; DWORD NumberParameters; ULONG_PTR ExceptionInformation[15];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;
// CONTEXT — exposes the stack-pointer fields the crash handler reads. Both Esp
// (x86) and Rsp (x64) are present so either spelling compiles; values are never
// meaningful on Linux (the SEH handlers are dead at runtime).
typedef struct _CONTEXT {
    ULONG_PTR Eip, Esp, Ebp;
    ULONG_PTR Rip, Rsp, Rbp;
    ULONG_PTR opaque[24];
} CONTEXT, *PCONTEXT;
typedef INT_PTR (*FARPROC)(void);
typedef struct tagMOUSEHOOKSTRUCT { POINT pt; HWND hwnd; UINT wHitTestCode; ULONG_PTR dwExtraInfo; } MOUSEHOOKSTRUCT, *LPMOUSEHOOKSTRUCT, *PMOUSEHOOKSTRUCT;
typedef struct tagCWPSTRUCT { LPARAM lParam; WPARAM wParam; UINT message; HWND hwnd; } CWPSTRUCT, *LPCWPSTRUCT, *PCWPSTRUCT;
#define HCBT_CREATEWND 3
// NTSTATUS exception codes.
#define STATUS_ACCESS_VIOLATION          0xC0000005u
#define STATUS_IN_PAGE_ERROR             0xC0000006u
#define STATUS_ILLEGAL_INSTRUCTION       0xC000001Du
#define STATUS_FLOAT_INVALID_OPERATION   0xC0000090u
#define STATUS_FLOAT_DIVIDE_BY_ZERO      0xC000008Eu
#define STATUS_INTEGER_DIVIDE_BY_ZERO    0xC0000094u
#define STATUS_STACK_OVERFLOW            0xC00000FDu
#define STATUS_BREAKPOINT                0x80000003u
// HWND ordering sentinels.
#define HWND_TOP       ((HWND)(intptr_t)0)
#define HWND_BOTTOM    ((HWND)(intptr_t)1)
#define HWND_TOPMOST   ((HWND)(intptr_t)-1)
#define HWND_NOTOPMOST ((HWND)(intptr_t)-2)
#define HWND_DESKTOP   ((HWND)(intptr_t)0)
#define HWND_MESSAGE   ((HWND)(intptr_t)-3)
#define CDS_TEST       0x00000002u
#define CDS_UPDATEREGISTRY 0x00000001u
#define MB_SETFOREGROUND 0x00010000u
#ifndef MAXDWORD
#define MAXDWORD       0xFFFFFFFFu
#endif
#define IDC_APPSTARTING MAKEINTRESOURCE(32650)
#define LANGIDFROMLCID(lcid) ((WORD)(lcid))
typedef struct _EXCEPTION_POINTERS { PEXCEPTION_RECORD ExceptionRecord; PCONTEXT ContextRecord; } EXCEPTION_POINTERS, *PEXCEPTION_POINTERS, *LPEXCEPTION_POINTERS;
typedef LONG (*PTOP_LEVEL_EXCEPTION_FILTER)(EXCEPTION_POINTERS*);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;
#define EXCEPTION_EXECUTE_HANDLER     1
#define EXCEPTION_CONTINUE_SEARCH     0
#define EXCEPTION_CONTINUE_EXECUTION (-1)
// Layered-window / scroll-metric constants.
#define WS_EX_LAYERED      0x00080000u
#define WS_EX_TRANSPARENT  0x00000020u
#define LWA_COLORKEY       0x00000001u
#define LWA_ALPHA          0x00000002u
#define SM_CXVSCROLL       2
#define SM_CYHSCROLL       3
#define SM_CYCAPTION       4
#define SM_CXBORDER        5
#define SM_CYBORDER        6
#define SM_CYMENU          15
// SetWindowPos / SysCommand / owner-draw state / class-long / scroll / hit-test /
// help / ChildWindowFromPointEx / button-style / button-notify / listbox-style.
#define SWP_NOREDRAW     0x0008u
#define SWP_FRAMECHANGED 0x0020u
#define SWP_HIDEWINDOW   0x0080u
#define SC_SIZE      0xF000u
#define SC_MOVE      0xF010u
#define SC_MINIMIZE  0xF020u
#define SC_MAXIMIZE  0xF030u
#define SC_CLOSE     0xF060u
#define SC_RESTORE   0xF120u
#define ODS_SELECTED 0x0001u
#define ODS_GRAYED   0x0002u
#define ODS_DISABLED 0x0004u
#define ODS_CHECKED  0x0008u
#define ODS_FOCUS    0x0010u
#define GCL_HCURSOR  (-12)
#define GCLP_HCURSOR (-12)
#define GCLP_HICON   (-14)
#define GCL_HICON    (-14)
#define CS_DBLCLKS   0x0008u
#define SB_LINEUP 0
#define SB_LINEDOWN 1
#define SB_PAGEUP 2
#define SB_PAGEDOWN 3
#define SB_THUMBPOSITION 4
#define SB_THUMBTRACK 5
#define SB_TOP 6
#define SB_BOTTOM 7
#define SB_ENDSCROLL 8
#define HTCLIENT  1
#define HTCAPTION 2
#define HELP_CONTEXT  1
#define HELP_CONTENTS 3
#define HELP_FINDER   11
#define CWP_ALL 0
#define CWP_SKIPINVISIBLE 1
#define CWP_SKIPDISABLED  2
#define CWP_SKIPTRANSPARENT 4
#define BS_PUSHBUTTON    0x00u
#define BS_DEFPUSHBUTTON 0x01u
#define BS_CHECKBOX      0x02u
#define BS_AUTOCHECKBOX  0x03u
#define BS_RADIOBUTTON   0x04u
#define BS_GROUPBOX      0x07u
#define BS_AUTORADIOBUTTON 0x09u
#define BS_OWNERDRAW     0x0Bu
#define BN_CLICKED       0
#define BN_DOUBLECLICKED 5
#define LBS_NOTIFY       0x0001u
#define LBS_SORT         0x0002u
#define LBS_NOREDRAW     0x0004u
#define LBS_MULTIPLESEL  0x0008u
#define LBS_OWNERDRAWFIXED 0x0010u
#define LBS_OWNERDRAWVARIABLE 0x0020u
#define LBS_HASSTRINGS   0x0040u
#define LBS_USETABSTOPS  0x0080u
#define LBS_NOINTEGRALHEIGHT 0x0100u
#define LBS_MULTICOLUMN  0x0200u
#define LBS_EXTENDEDSEL  0x0800u
// MSVC <io.h> _findfirst family.
typedef long _fsize_t;
struct _finddata_t { unsigned attrib; long time_create, time_access, time_write; _fsize_t size; char name[260]; };
#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH   0x20
#define PALETTERGB(r, g, b) (0x02000000u | RGB(r, g, b))
#define GetCurrentTime()    GetTickCount()
// GetSystemMetrics indices (the few referenced).
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXFULLSCREEN 16
#define SM_CYFULLSCREEN 17
// GetSysColor indices (full winuser.h set).
#define COLOR_SCROLLBAR 0
#define COLOR_BACKGROUND 1
#define COLOR_DESKTOP 1
#define COLOR_ACTIVECAPTION 2
#define COLOR_INACTIVECAPTION 3
#define COLOR_MENU 4
#define COLOR_WINDOW 5
#define COLOR_WINDOWFRAME 6
#define COLOR_MENUTEXT 7
#define COLOR_WINDOWTEXT 8
#define COLOR_CAPTIONTEXT 9
#define COLOR_ACTIVEBORDER 10
#define COLOR_INACTIVEBORDER 11
#define COLOR_APPWORKSPACE 12
#define COLOR_HIGHLIGHT 13
#define COLOR_HIGHLIGHTTEXT 14
#define COLOR_BTNFACE 15
#define COLOR_3DFACE 15
#define COLOR_BTNSHADOW 16
#define COLOR_3DSHADOW 16
#define COLOR_GRAYTEXT 17
#define COLOR_BTNTEXT 18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT 20
#define COLOR_BTNHILIGHT 20
#define COLOR_3DHIGHLIGHT 20
#define COLOR_3DHILIGHT 20
#define COLOR_3DDKSHADOW 21
#define COLOR_3DLIGHT 22
#define COLOR_INFOTEXT 23
#define COLOR_INFOBK 24
#define COLOR_HOTLIGHT 26
// Shell AppBar messages.
#define ABM_NEW 0
#define ABM_REMOVE 1
#define ABM_GETTASKBARPOS 5
// CRT debug block type.
#define _CRT_BLOCK 2
#define _NORMAL_BLOCK 1

// --- MMIO (multimedia RIFF I/O) types/constants ---
typedef DWORD FOURCC;
typedef void* HMMIO;
#define mmioFOURCC(c0, c1, c2, c3) \
    ((FOURCC)(BYTE)(c0) | ((FOURCC)(BYTE)(c1) << 8) | ((FOURCC)(BYTE)(c2) << 16) | ((FOURCC)(BYTE)(c3) << 24))
#define FOURCC_DOS mmioFOURCC('D','O','S',' ')
#define FOURCC_MEM mmioFOURCC('M','E','M',' ')
typedef struct _MMCKINFO { FOURCC ckid; DWORD cksize; FOURCC fccType; DWORD dwDataOffset; DWORD dwFlags; } MMCKINFO, *LPMMCKINFO;
typedef struct _MMIOINFO {
    DWORD dwFlags; FOURCC fccIOProc; void* pIOProc; UINT wErrorRet; void* htask;
    LONG cchBuffer; char* pchBuffer; char* pchNext; char* pchEndRead; char* pchEndWrite;
    LONG lBufOffset; LONG lDiskOffset; DWORD_PTR adwInfo[4]; DWORD dwReserved1, dwReserved2; HMMIO hmmio;
} MMIOINFO, *LPMMIOINFO;
#define MMIO_READ      0x00000000u
#define MMIO_WRITE     0x00000001u
#define MMIO_READWRITE 0x00000002u
#define MMIO_CREATE    0x00001000u
#define MMIO_DENYWRITE 0x00000020u
#define MMIO_ALLOCBUF  0x00010000u
#define MMIO_FHOPEN    0x0010u    // mmioClose flag
#define MMIO_FINDCHUNK 0x0010u    // mmioDescend flags
#define MMIO_FINDRIFF  0x0020u
#define MMIO_FINDLIST  0x0040u
typedef struct tagLOGFONTA {
    LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
    BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision,
         lfClipPrecision, lfQuality, lfPitchAndFamily;
    CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, LOGFONT, *LPLOGFONTA, *LPLOGFONT;
typedef struct tagTEXTMETRICA {
    LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading,
         tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang,
         tmDigitizedAspectX, tmDigitizedAspectY;
    BYTE tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar, tmItalic,
         tmUnderlined, tmStruckOut, tmPitchAndFamily, tmCharSet;
} TEXTMETRICA, TEXTMETRIC, *LPTEXTMETRICA, *LPTEXTMETRIC;
typedef struct tagPAINTSTRUCT { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore; BOOL fIncUpdate; BYTE rgbReserved[32]; } PAINTSTRUCT, *LPPAINTSTRUCT;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef struct tagWNDCLASSA {
    UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra;
    HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground;
    LPCSTR lpszMenuName, lpszClassName;
} WNDCLASSA, WNDCLASS, *LPWNDCLASSA;
typedef struct tagWNDCLASSEXA {
    UINT cbSize, style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra;
    HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground;
    LPCSTR lpszMenuName, lpszClassName; HICON hIconSm;
} WNDCLASSEXA, *LPWNDCLASSEXA;
typedef struct _APPBARDATA { DWORD cbSize; HWND hWnd; UINT uCallbackMessage, uEdge; RECT rc; LPARAM lParam; } APPBARDATA, *PAPPBARDATA;
typedef struct _MEMORYSTATUS {
    DWORD dwLength, dwMemoryLoad; SIZE_T dwTotalPhys, dwAvailPhys, dwTotalPageFile,
          dwAvailPageFile, dwTotalVirtual, dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;
typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId;
    CHAR szCSDVersion[128];
} OSVERSIONINFOA, OSVERSIONINFO, *LPOSVERSIONINFOA, *POSVERSIONINFOA;
#define VER_PLATFORM_WIN32_NT 2
typedef struct _devicemodeA {
    BYTE dmDeviceName[32]; WORD dmSpecVersion, dmDriverVersion, dmSize, dmDriverExtra;
    DWORD dmFields; LONG dmPositionX, dmPositionY; DWORD dmDisplayOrientation, dmDisplayFixedOutput;
    short dmColor, dmDuplex, dmYResolution, dmTTOption, dmCollate; BYTE dmFormName[32];
    WORD dmLogPixels; DWORD dmBitsPerPel, dmPelsWidth, dmPelsHeight, dmDisplayFlags, dmDisplayFrequency;
} DEVMODEA, DEVMODE, *LPDEVMODEA, *PDEVMODEA;
#define DM_BITSPERPEL  0x00040000u
#define DM_PELSWIDTH   0x00080000u
#define DM_PELSHEIGHT  0x00100000u
#define DM_DISPLAYFREQUENCY 0x00400000u
#define CDS_FULLSCREEN 0x00000004u
#define DISP_CHANGE_SUCCESSFUL 0
#define TA_LEFT   0
#define TA_RIGHT  2
#define TA_CENTER 6
#define TA_TOP    0
#define TA_BOTTOM 8
#define WM_SETTEXT      0x000C
#define WM_GETTEXT      0x000D
#define WM_GETTEXTLENGTH 0x000E
typedef void (*_se_translator_function)(unsigned int, EXCEPTION_POINTERS*);
typedef ULONG* PULONG;
typedef struct _STARTUPINFOA {
    DWORD cb; LPSTR lpReserved, lpDesktop, lpTitle; DWORD dwX, dwY, dwXSize, dwYSize,
    dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags; WORD wShowWindow, cbReserved2;
    LPBYTE lpReserved2; HANDLE hStdInput, hStdOutput, hStdError;
} STARTUPINFOA, STARTUPINFO, *LPSTARTUPINFOA;
typedef struct _PROCESS_INFORMATION { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; } PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
#define STARTF_USESHOWWINDOW 0x00000001u
typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb, PageFaultCount; SIZE_T PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage,
    QuotaPagedPoolUsage, QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;
typedef struct _PROCESS_MEMORY_COUNTERS_EX {
    DWORD cb, PageFaultCount; SIZE_T PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage,
    QuotaPagedPoolUsage, QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage,
    PeakPagefileUsage, PrivateUsage;
} PROCESS_MEMORY_COUNTERS_EX, *PPROCESS_MEMORY_COUNTERS_EX;
// Owner-draw control types + language id macros.
#define ODT_MENU    1
#define ODT_LISTBOX 2
#define ODT_COMBOBOX 3
#define ODT_BUTTON  4
#define ODT_STATIC  5
#define PRIMARYLANGID(lgid) ((WORD)(lgid) & 0x3ff)
#define SUBLANGID(lgid)     ((WORD)(lgid) >> 10)
#define LANG_ENGLISH 0x09
typedef struct tagCREATESTRUCTA {
    LPVOID lpCreateParams; HINSTANCE hInstance; HMENU hMenu; HWND hwndParent;
    int cy, cx, y, x; LONG style; LPCSTR lpszName, lpszClass; DWORD dwExStyle;
} CREATESTRUCTA, CREATESTRUCT, *LPCREATESTRUCTA, *LPCREATESTRUCT;
typedef struct tagMINMAXINFO { POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize; } MINMAXINFO, *LPMINMAXINFO, *PMINMAXINFO;
typedef struct tagWINDOWPLACEMENT { UINT length, flags, showCmd; POINT ptMinPosition, ptMaxPosition; RECT rcNormalPosition; } WINDOWPLACEMENT, *LPWINDOWPLACEMENT, *PWINDOWPLACEMENT;
typedef struct _RTL_SRWLOCK { void* Ptr; } SRWLOCK, *PSRWLOCK;
#define SRWLOCK_INIT { nullptr }
typedef enum _GET_FILEEX_INFO_LEVELS { GetFileExInfoStandard = 0, GetFileExMaxInfoLevel } GET_FILEEX_INFO_LEVELS;
typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
    DWORD nFileSizeHigh, nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

// Window styles / class / long offsets.
#define WS_OVERLAPPED 0x00000000u
#define WS_POPUP      0x80000000u
#define WS_CHILD      0x40000000u
#define WS_VISIBLE    0x10000000u
#define WS_DISABLED   0x08000000u
#define WS_CAPTION    0x00C00000u
#define WS_BORDER     0x00800000u
#define WS_SYSMENU    0x00080000u
#define WS_THICKFRAME 0x00040000u
#define WS_MINIMIZEBOX 0x00020000u
#define WS_MAXIMIZEBOX 0x00010000u
#define WS_VSCROLL    0x00200000u
#define WS_HSCROLL    0x00100000u
#define WS_DLGFRAME   0x00400000u
#define WS_CLIPSIBLINGS 0x04000000u
#define WS_CLIPCHILDREN 0x02000000u
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define WS_EX_TOOLWINDOW 0x00000080u
#define WS_EX_CLIENTEDGE 0x00000200u
#define WS_EX_STATICEDGE 0x00020000u
#define WS_EX_APPWINDOW  0x00040000u
// IDC_* standard cursors / IDI_* standard icons (MAKEINTRESOURCE values).
#define IDC_ARROW    MAKEINTRESOURCE(32512)
#define IDC_IBEAM    MAKEINTRESOURCE(32513)
#define IDC_WAIT     MAKEINTRESOURCE(32514)
#define IDC_CROSS    MAKEINTRESOURCE(32515)
#define IDC_SIZEALL  MAKEINTRESOURCE(32646)
#define IDC_HAND     MAKEINTRESOURCE(32649)
#define IDI_APPLICATION MAKEINTRESOURCE(32512)
#define IDI_HAND        MAKEINTRESOURCE(32513)
#define IDI_QUESTION    MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION MAKEINTRESOURCE(32515)
#define IDI_ASTERISK    MAKEINTRESOURCE(32516)
// Windows hooks.
#define WH_KEYBOARD     2
#define WH_GETMESSAGE   3
#define WH_CALLWNDPROC  4
#define WH_CBT          5
#define WH_MOUSE        7
#define HC_ACTION       0
#define WS_EX_NOACTIVATE 0x08000000u
#define WS_EX_TOPMOST    0x00000008u
#define GWL_STYLE     (-16)
#define GWL_EXSTYLE   (-20)
#define GWLP_USERDATA (-21)
#define GWLP_WNDPROC  (-4)
#define ICON_SMALL    0
#define ICON_BIG      1

// SetWindowPos flags / DrawText flags / text bk modes.
#define SWP_NOSIZE 0x0001u
#define SWP_NOMOVE 0x0002u
#define SWP_NOZORDER 0x0004u
#define SWP_NOACTIVATE 0x0010u
#define SWP_SHOWWINDOW 0x0040u
#define TRANSPARENT 1
#define OPAQUE 2
#define ETO_OPAQUE 0x0002u
#define ETO_CLIPPED 0x0004u
#define DT_LEFT 0x0000u
#define DT_CENTER 0x0001u
#define DT_RIGHT 0x0002u
#define DT_VCENTER 0x0004u
#define DT_SINGLELINE 0x0020u
#define DT_NOPREFIX 0x0800u
#define DT_WORDBREAK 0x0010u

// PeekMessage / BitBlt rop / StretchBlt mode / GetDeviceCaps / GlobalAlloc / VK.
#define PM_NOREMOVE 0x0000u
#define PM_REMOVE   0x0001u
#define SRCCOPY     0x00CC0020u
#define SRCAND      0x008800C6u
#define SRCPAINT    0x00EE0086u
#define BLACKNESS   0x00000042u
#define WHITENESS   0x00FF0062u
#define STRETCH_DELETESCANS 3
#define COLORONCOLOR 3
#define BITSPIXEL   12
#define PLANES      14
#define RASTERCAPS  38
#define RC_PALETTE  0x0100
#define SIZEPALETTE 104
#define SM_CXICON   11
#define SM_CYICON   12
#define GMEM_FIXED    0x0000u
#define GMEM_MOVEABLE 0x0002u
#define GMEM_ZEROINIT 0x0040u
#define GPTR          (GMEM_FIXED | GMEM_ZEROINIT)
#define GHND          (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define VK_CAPITAL  0x14
#define VK_NUMLOCK  0x90
#define VK_SCROLL   0x91

// Dialog/window extra styles + messages + misc constants.
#define WS_TABSTOP     0x00010000u
#define WS_GROUP       0x00020000u
#define WS_POPUPWINDOW (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WM_INITDIALOG  0x0110
#define WM_DRAWITEM    0x002B
#define WM_MEASUREITEM 0x002C
#define DT_CALCRECT    0x0400u
#define MB_ICONMASK    0x000000F0u
#define MB_TYPEMASK    0x0000000Fu
#define DWLP_MSGRESULT 0
#define DWLP_DLGPROC   ((int)sizeof(LRESULT))
#define DWLP_USER      ((int)(sizeof(LRESULT) + sizeof(void*)))
// GetStockObject indices.
#define WHITE_BRUSH      0
#define LTGRAY_BRUSH     1
#define GRAY_BRUSH       2
#define DKGRAY_BRUSH     3
#define BLACK_BRUSH      4
#define NULL_BRUSH       5
#define HOLLOW_BRUSH     5
#define WHITE_PEN        6
#define BLACK_PEN        7
#define NULL_PEN         8
#define ANSI_VAR_FONT    12
#define SYSTEM_FONT      13
#define DEFAULT_GUI_FONT 17

typedef void* HRSRC;
typedef struct tagVS_FIXEDFILEINFO {
    DWORD dwSignature, dwStrucVersion, dwFileVersionMS, dwFileVersionLS,
          dwProductVersionMS, dwProductVersionLS, dwFileFlagsMask, dwFileFlags,
          dwFileOS, dwFileType, dwFileSubtype, dwFileDateMS, dwFileDateLS;
} VS_FIXEDFILEINFO;

// Extra window messages.
#define WM_NCCREATE       0x0081
#define WM_NCDESTROY      0x0082
#define WM_GETICON        0x007F
#define WM_SETICON        0x0080
#define WM_SETREDRAW      0x000B
#define WM_NOTIFY         0x004E
#define WM_QUERYENDSESSION 0x0011
#define WM_QUERYNEWPALETTE 0x030F
#define WM_PALETTECHANGED  0x0311
#define WM_CTLCOLORMSGBOX   0x0132
#define WM_CTLCOLOREDIT     0x0133
#define WM_CTLCOLORLISTBOX  0x0134
#define WM_CTLCOLORBTN      0x0135
#define WM_CTLCOLORDLG      0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC   0x0138

// Button / combobox / listbox control messages.
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define BM_GETSTATE 0x00F2
#define BM_SETSTATE 0x00F3
#define BM_SETSTYLE 0x00F4
#define CB_ADDSTRING    0x0143
#define CB_DELETESTRING 0x0144
#define CB_GETCOUNT     0x0146
#define CB_GETCURSEL    0x0147
#define CB_RESETCONTENT 0x014B
#define CB_SETCURSEL    0x014E
#define LB_ADDSTRING    0x0180
#define LB_DELETESTRING 0x0182
#define LB_RESETCONTENT 0x0184
#define LB_SETSEL       0x0185
#define LB_SETCURSEL    0x0186
#define LB_GETCOUNT     0x018B
#define LB_GETCURSEL    0x0188
#define LB_GETSELCOUNT  0x0190
#define LB_GETSELITEMS  0x0191
#define LB_SETTABSTOPS  0x0192
#define LB_GETTOPINDEX  0x018E
#define LB_SETTOPINDEX  0x0197
#define LB_GETITEMDATA  0x0199
#define LB_SETITEMDATA  0x019A
#define LB_GETITEMHEIGHT 0x01A1
#define LB_SETITEMHEIGHT 0x01A0

//===========================================================================
// 8e. GDI / USER / window function STUBS (inline; dead at runtime on the SDL2
//     path — the game renders via SDL_Renderer, not GDI). They exist only so
//     mfc_compat.h's legacy CWnd/CDC/control method bodies compile.
//===========================================================================
// Window / USER.
LRESULT SendMessageA(HWND, UINT, WPARAM, LPARAM);
LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM);
BOOL    PostMessageA(HWND, UINT, WPARAM, LPARAM);
BOOL    PostMessage(HWND, UINT, WPARAM, LPARAM);
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM);
inline BOOL    ShowWindow(HWND, int) { return TRUE; }
inline BOOL    UpdateWindow(HWND) { return TRUE; }
BOOL    MoveWindow(HWND, int, int, int, int, BOOL);   // real impl (fires WM_SIZE)
BOOL    DestroyWindow(HWND);
inline BOOL    EnableWindow(HWND, BOOL) { return TRUE; }
BOOL    IsWindow(HWND);
inline BOOL    IsWindowVisible(HWND) { return FALSE; }
inline BOOL    IsWindowEnabled(HWND) { return TRUE; }
inline BOOL    IsIconic(HWND) { return FALSE; }
inline HWND    GetParent(HWND) { return NULL; }
inline HWND    GetDlgItem(HWND, int) { return NULL; }
inline HWND    GetActiveWindow(void) { return NULL; }
inline HWND    SetActiveWindow(HWND) { return NULL; }
inline HWND    SetFocus(HWND) { return NULL; }
inline HWND    FindWindowA(LPCSTR, LPCSTR) { return NULL; }
inline HWND    ChildWindowFromPoint(HWND, POINT) { return NULL; }
inline BOOL    SetForegroundWindow(HWND) { return TRUE; }
inline BOOL    BringWindowToTop(HWND) { return TRUE; }
inline HWND    SetCapture(HWND) { return NULL; }
inline BOOL    ReleaseCapture(void) { return TRUE; }
BOOL    GetClientRect(HWND, LPRECT r);    // real impl (returns stored window size)
BOOL    GetWindowRect(HWND, LPRECT r);    // real impl (returns stored window rect)
inline BOOL    ClientToScreen(HWND, LPPOINT) { return TRUE; }
inline BOOL    ScreenToClient(HWND, LPPOINT) { return TRUE; }
inline BOOL    InvalidateRect(HWND, const RECT*, BOOL) { return TRUE; }
inline BOOL    ValidateRect(HWND, const RECT*) { return TRUE; }
BOOL    SetWindowPos(HWND, HWND, int, int, int, int, UINT);   // real impl (fires WM_SIZE)
inline int     GetWindowTextA(HWND, LPSTR s, int n) { if (s && n) s[0]=0; return 0; }
inline int     GetWindowTextLengthA(HWND) { return 0; }
inline BOOL    SetWindowTextA(HWND, LPCSTR) { return TRUE; }
inline UINT    GetDlgItemTextA(HWND, int, LPSTR s, int n) { if (s && n) s[0]=0; return 0; }
inline BOOL    SetDlgItemTextA(HWND, int, LPCSTR) { return TRUE; }
inline int     GetDlgCtrlID(HWND) { return 0; }
LONG_PTR GetWindowLongPtr(HWND, int);
LONG_PTR SetWindowLongPtr(HWND, int, LONG_PTR);
LONG    GetWindowLong(HWND, int);
LONG    GetWindowLongA(HWND, int);
inline int     GetScrollPos(HWND, int) { return 0; }
inline int     SetScrollPos(HWND, int, int, BOOL) { return 0; }
inline BOOL    GetScrollRange(HWND, int, LPINT, LPINT) { return TRUE; }
inline BOOL    SetScrollRange(HWND, int, int, int, BOOL) { return TRUE; }
inline UINT_PTR SetTimer(HWND, UINT_PTR, UINT, TIMERPROC) { return 1; }
inline BOOL    KillTimer(HWND, UINT_PTR) { return TRUE; }
inline BOOL    SetWindowPlacement(HWND, const WINDOWPLACEMENT*) { return TRUE; }
inline BOOL    GetWindowPlacement(HWND, WINDOWPLACEMENT*) { return TRUE; }
HWND    CreateWindowExA(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
HWND    CreateWindowEx(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
ATOM    RegisterClass(const WNDCLASS*);
ATOM    RegisterClassA(const WNDCLASS*);
BOOL    GetMessage(LPMSG, HWND, UINT, UINT);
BOOL    TranslateMessage(const MSG*);
LRESULT DispatchMessage(const MSG*);
// GDI.
inline HDC     GetDC(HWND) { return NULL; }
inline HDC     GetWindowDC(HWND) { return NULL; }
inline int     ReleaseDC(HWND, HDC) { return 1; }
inline HDC     BeginPaint(HWND, PAINTSTRUCT* ps) { if (ps) { ps->hdc=NULL; } return NULL; }
inline BOOL    EndPaint(HWND, const PAINTSTRUCT*) { return TRUE; }
inline HGDIOBJ SelectObject(HDC, HGDIOBJ) { return NULL; }
inline BOOL    DeleteObject(HGDIOBJ) { return TRUE; }
inline int     GetObjectA(HGDIOBJ, int, LPVOID) { return 0; }
inline HPEN    CreatePen(int, int, COLORREF) { return NULL; }
inline HBRUSH  CreateSolidBrush(COLORREF) { return NULL; }
inline HFONT   CreateFontIndirectA(const LOGFONTA*) { return NULL; }
inline HPALETTE CreatePalette(const LOGPALETTE*) { return NULL; }
inline HRGN    CreateRectRgn(int, int, int, int) { return NULL; }
inline HRGN    CreateEllipticRgn(int, int, int, int) { return NULL; }
inline HRGN    CreatePolygonRgn(const POINT*, int, int) { return NULL; }
inline COLORREF SetBkColor(HDC, COLORREF) { return 0; }
inline int     SetBkMode(HDC, int) { return 0; }
inline COLORREF SetTextColor(HDC, COLORREF) { return 0; }
inline COLORREF GetTextColor(HDC) { return 0; }
inline COLORREF GetNearestColor(HDC, COLORREF c) { return c; }
// Report a 32-bpp truecolor display. Returning 0 for everything (the old stub)
// made CColorFormat::CalcScreenFormat compute 0 bits-per-pixel, which pushed the
// engine onto degraded 8-bit/low-detail asset paths (e.g. the menu loading the
// 96x96 WL tile + MN08 instead of the full MN24 background, and the wrong
// bytes-per-pixel for the terrain/sprite blits). 32 bpp / 1 plane selects the
// 24-bit asset variants the data file ships.
inline int     GetDeviceCaps(HDC, int index) {
    switch (index) {
        case BITSPIXEL: return 32;   // 12: bits per pixel
        case PLANES:    return 1;    // 14: colour planes
        default:        return 0;
    }
}
inline BOOL    BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return TRUE; }
inline BOOL    TextOutA(HDC, int, int, LPCSTR, int) { return TRUE; }
inline BOOL    ExtTextOutA(HDC, int, int, UINT, const RECT*, LPCSTR, UINT, const INT*) { return TRUE; }
inline int     DrawTextA(HDC, LPCSTR, int, LPRECT, UINT) { return 0; }
inline BOOL    GetTextExtentPoint32A(HDC, LPCSTR, int, LPSIZE sz) { if (sz) { sz->cx=0; sz->cy=0; } return TRUE; }
inline BOOL    GetTextMetricsA(HDC, TEXTMETRICA*) { return TRUE; }
inline int     FillRect(HDC, const RECT*, HBRUSH) { return 1; }
inline BOOL    FillRgn(HDC, HRGN, HBRUSH) { return TRUE; }
inline BOOL    Rectangle(HDC, int, int, int, int) { return TRUE; }
inline BOOL    RoundRect(HDC, int, int, int, int, int, int) { return TRUE; }
inline BOOL    LineTo(HDC, int, int) { return TRUE; }
inline BOOL    MoveToEx(HDC, int, int, LPPOINT) { return TRUE; }
inline BOOL    RectVisible(HDC, const RECT*) { return TRUE; }
inline HPALETTE SelectPalette(HDC, HPALETTE, BOOL) { return NULL; }
inline UINT    RealizePalette(HDC) { return 0; }
// Global/Local heap (legacy) → malloc. HGLOBAL handle IS the pointer (GMEM_FIXED
// semantics); GlobalLock returns it unchanged.
inline HGLOBAL GlobalAlloc(UINT flags, SIZE_T n) { void* p = malloc(n); if (p && (flags & 0x0040u)) memset(p, 0, n); return (HGLOBAL)p; }
inline HGLOBAL GlobalReAlloc(HGLOBAL h, SIZE_T n, UINT) { return (HGLOBAL)realloc((void*)h, n); }
inline LPVOID  GlobalLock(HGLOBAL h) { return (LPVOID)h; }
inline BOOL    GlobalUnlock(HGLOBAL) { return TRUE; }
inline HGLOBAL GlobalFree(HGLOBAL h) { free((void*)h); return NULL; }
inline SIZE_T  GlobalSize(HGLOBAL) { return 0; }
inline HLOCAL  LocalAlloc(UINT flags, SIZE_T n) { void* p = malloc(n); if (p && (flags & 0x0040u)) memset(p, 0, n); return (HLOCAL)p; }
inline HLOCAL  LocalFree(HLOCAL h) { free((void*)h); return NULL; }
inline HGDIOBJ GetStockObject(int) { return NULL; }
inline int     GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, BITMAPINFO*, UINT) { return 0; }
inline int     SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT, const void*, const BITMAPINFO*, UINT) { return 0; }
inline SHORT   GetKeyState(int) { return 0; }
inline SHORT   GetAsyncKeyState(int) { return 0; }
// Resource API (legacy CResource/CGlobal::LoadResource path — no module resources on Linux).
inline HRSRC   FindResourceA(HMODULE, LPCSTR, LPCSTR) { return NULL; }
inline HGLOBAL LoadResource(HMODULE, HRSRC) { return NULL; }
inline LPVOID  LockResource(HGLOBAL) { return NULL; }
inline BOOL    UnlockResource(HGLOBAL) { return TRUE; }
inline BOOL    FreeResource(HGLOBAL) { return FALSE; }
inline DWORD   SizeofResource(HMODULE, HRSRC) { return 0; }
// Version-info API (version.lib) — stubbed; the game reads its own version data.
inline DWORD   GetFileVersionInfoSizeA(LPCSTR, LPDWORD h) { if (h) *h = 0; return 0; }
inline BOOL    GetFileVersionInfoA(LPCSTR, DWORD, DWORD, LPVOID) { return FALSE; }
inline BOOL    VerQueryValueA(LPCVOID, LPCSTR, LPVOID* p, PUINT n) { if (p) *p = NULL; if (n) *n = 0; return FALSE; }
// Wave-out device query (audio is SDL_mixer; report no legacy wave devices).
inline UINT    waveOutGetNumDevs(void) { return 0; }
// More GDI (legacy DIB/DC path — dead on SDL_Renderer).
inline HDC     CreateCompatibleDC(HDC) { return NULL; }
inline BOOL    DeleteDC(HDC) { return TRUE; }
inline HBITMAP CreateCompatibleBitmap(HDC, int, int) { return NULL; }
inline HBITMAP CreateDIBSection(HDC, const BITMAPINFO*, UINT, void** bits, HANDLE, DWORD) { if (bits) *bits = NULL; return NULL; }
inline HBITMAP CreateDIBitmap(HDC, const BITMAPINFOHEADER*, DWORD, const void*, const BITMAPINFO*, UINT) { return NULL; }
inline BOOL    GdiFlush(void) { return TRUE; }
inline DWORD   GetClassLong(HWND, int) { return 0; }
inline DWORD   SetClassLong(HWND, int, LONG) { return 0; }
// Shell / file-system queries.
inline UINT    GetDriveTypeA(LPCSTR) { return DRIVE_FIXED; }
inline BOOL    GetVolumeInformationA(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD) { return FALSE; }
inline UINT    GetSystemDirectoryA(LPSTR buf, UINT n) { if (buf && n) buf[0] = 0; return 0; }
inline UINT    GetWindowsDirectoryA(LPSTR buf, UINT n) { if (buf && n) buf[0] = 0; return 0; }
inline BOOL    GetDiskFreeSpaceA(LPCSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD) { return FALSE; }
inline LPSTR   GetCommandLineA(void) { static char empty[1] = {0}; return empty; }
inline LPSTR   PathGetArgsA(LPCSTR p) { return (LPSTR)(p ? p : ""); }
inline HCURSOR SetCursor(HCURSOR) { return NULL; }
inline BOOL    GetClassInfoA(HINSTANCE, LPCSTR, void*) { return FALSE; }
inline HHOOK   SetWindowsHookExA(int, HOOKPROC, HINSTANCE, DWORD) { return NULL; }
inline BOOL    UnhookWindowsHookEx(HHOOK) { return TRUE; }
inline LRESULT CallNextHookEx(HHOOK, int, WPARAM, LPARAM) { return 0; }
// GetCursorPos returned (0,0) — so the area map's SetMouseState (which does
// GetCursorPos+ScreenToClient to find the hovered hex/unit) always read the
// top-left corner and never showed the select/goto/target cursors. We track the
// live mouse position from SDL motion events (en_SetCursorPos, called with the
// area window's CLIENT coords) and return it here; ScreenToClient stays a no-op,
// so GetCursorPos()+ScreenToClient() yields the client position the game expects.
extern int g_enCursorX;
extern int g_enCursorY;
void       en_SetCursorPos(int x, int y);
inline BOOL    GetCursorPos(LPPOINT p) { if (p) { p->x = g_enCursorX; p->y = g_enCursorY; } return TRUE; }
inline BOOL    SetCursorPos(int, int) { return TRUE; }
LONG    SetWindowLongA(HWND, int, LONG);
LONG    SetWindowLong(HWND, int, LONG);
inline BOOL    SetLayeredWindowAttributes(HWND, COLORREF, BYTE, DWORD) { return TRUE; }
inline BOOL    ClipCursor(const RECT*) { return TRUE; }
void           PostQuitMessage(int);   // real impl: enqueues WM_QUIT so the loop exits
inline BOOL    AdjustWindowRect(LPRECT, DWORD, BOOL) { return TRUE; }
inline BOOL    AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD) { return TRUE; }
inline LPTOP_LEVEL_EXCEPTION_FILTER SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER) { return NULL; }
inline BOOL    SetSysColors(int, const INT*, const COLORREF*) { return TRUE; }
inline LONG_PTR SetClassLongPtr(HWND, int, LONG_PTR) { return 0; }
inline LONG_PTR GetClassLongPtr(HWND, int) { return 0; }
inline int     TranslateAcceleratorA(HWND, HACCEL, LPMSG) { return 0; }
inline int     MapWindowPoints(HWND, HWND, LPPOINT, UINT) { return 0; }
inline HACCEL  LoadAcceleratorsA(HINSTANCE, LPCSTR) { return NULL; }
inline HWND    ChildWindowFromPointEx(HWND, POINT, UINT) { return NULL; }
// Profile (.ini) APIs — the game uses its own EnWriteProfile*; these raw forms
// still appear and are stubbed (no real .ini I/O on the Linux bring-up).
inline BOOL WritePrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPCSTR) { return TRUE; }
inline UINT GetPrivateProfileIntA(LPCSTR, LPCSTR, INT def, LPCSTR) { return (UINT)def; }
inline DWORD GetPrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR def, LPSTR buf, DWORD n, LPCSTR) {
    const char* d = def ? def : ""; DWORD len = (DWORD)strlen(d);
    if (buf && n) { if (len > n - 1) len = n - 1; memcpy(buf, d, len); buf[len] = 0; }
    return len;
}
inline int    fopen_s(FILE** pf, const char* name, const char* mode) { if (!pf) return 22; *pf = fopen(name, mode); return *pf ? 0 : 1; }
// (CreateSemaphoreA/ReleaseSemaphore are defined after the extern "C" block —
//  they call CreateEventA/SetEvent which are declared there.)
inline int     SetDIBits(HDC, HBITMAP, UINT, UINT, const void*, const BITMAPINFO*, UINT) { return 0; }
inline UINT    SetDIBColorTable(HDC, UINT, UINT, const RGBQUAD*) { return 0; }
inline HICON   LoadIcon(HINSTANCE, LPCSTR) { return NULL; }
// Return a non-NULL sentinel (the resource id/name pointer) rather than NULL.
// The area map treats a NULL HCURSOR as "build-placement: hide the cursor"; if
// every game cursor loads as NULL the pointer is hidden over the whole play area.
// A non-NULL handle makes AreaApplyCursor show a (system arrow) cursor instead.
inline HCURSOR LoadCursor(HINSTANCE, LPCSTR name) { return (HCURSOR)(void*)name; }
inline BOOL    IsDialogMessage(HWND, LPMSG) { return FALSE; }
inline BOOL    IsDialogMessageA(HWND, LPMSG) { return FALSE; }
inline int     MulDiv(int a, int b, int c) { return c ? (int)(((long long)a * b) / c) : -1; }
inline BOOL    SwitchToThread(void) { return FALSE; }
// Secure-CRT integer-to-string (used by EnSettings/lastplnt). radix 10/16/etc.
inline int en_itoa_radix(long long v, char* buf, int radix) {
    if (!buf) return 22; const char* d = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[66]; int i = 0; bool neg = (radix == 10 && v < 0);
    unsigned long long u = neg ? (unsigned long long)(-v) : (unsigned long long)v;
    if (u == 0) tmp[i++] = '0';
    while (u) { tmp[i++] = d[u % radix]; u /= radix; }
    int o = 0; if (neg) buf[o++] = '-';
    while (i) buf[o++] = tmp[--i];
    buf[o] = '\0'; return 0;
}
// MSVC provides both _itoa_s(value, buf, size, radix) and a template form
// _itoa_s(value, char(&buf)[N], radix). Provide both as overloads (the codebase
// uses the 3-arg template form, e.g. _itoa_s(iNum, sBuf, iRadix)).
template <size_t N> inline int _itoa_s(long long v, char (&buf)[N], int radix) { return en_itoa_radix(v, buf, radix); }
template <size_t N> inline int _ltoa_s(long long v, char (&buf)[N], int radix) { return en_itoa_radix(v, buf, radix); }
template <size_t N> inline int _ultoa_s(unsigned long long v, char (&buf)[N], int radix) { return en_itoa_radix((long long)v, buf, radix); }
inline int _itoa_s(long long v, char* buf, size_t, int radix)  { return en_itoa_radix(v, buf, radix); }
inline int _ltoa_s(long long v, char* buf, size_t, int radix)  { return en_itoa_radix(v, buf, radix); }
inline int _ultoa_s(unsigned long long v, char* buf, size_t, int radix) { return en_itoa_radix((long long)v, buf, radix); }
inline char* en_itoa(long long v, char* buf, int radix) { en_itoa_radix(v, buf, radix); return buf; }
#define _itoa(val, buf, radix)  en_itoa((long long)(val), (buf), (radix))
#define _ltoa(val, buf, radix)  en_itoa((long long)(val), (buf), (radix))
#define _ultoa(val, buf, radix) en_itoa((long long)(val), (buf), (radix))
#define itoa(val, buf, radix)   en_itoa((long long)(val), (buf), (radix))
// The game derives its window/screen size from SM_CXSCREEN/SM_CYSCREEN. These
// globals default to 1280x1024 but are overwritten at startup (linux_main) with
// the real desktop resolution via en_SetScreenMetrics, so the game launches
// full-screen at the native size.
extern int g_enScreenW;
extern int g_enScreenH;
void       en_SetScreenMetrics(int w, int h);
inline int     GetSystemMetrics(int idx) {
    switch (idx) {
        case SM_CXSCREEN: case SM_CXFULLSCREEN: return g_enScreenW;
        case SM_CYSCREEN: case SM_CYFULLSCREEN: return g_enScreenH;
        default: return 0;
    }
}
inline BOOL    StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD) { return TRUE; }
inline int     StretchDIBits(HDC, int, int, int, int, int, int, int, int, const void*, const BITMAPINFO*, UINT, DWORD) { return 0; }
inline int     SetStretchBltMode(HDC, int) { return 0; }
BOOL    PeekMessage(LPMSG, HWND, UINT, UINT, UINT);
inline BOOL    WinHelp(HWND, LPCSTR, UINT, ULONG_PTR) { return TRUE; }
inline BOOL    SetProp(HWND, LPCSTR, HANDLE) { return TRUE; }
inline HANDLE  GetProp(HWND, LPCSTR) { return NULL; }
inline HANDLE  RemoveProp(HWND, LPCSTR) { return NULL; }
inline DWORD   GetSysColor(int) { return 0; }
inline HBRUSH  GetSysColorBrush(int) { return NULL; }
inline HWND    WindowFromPoint(POINT) { return NULL; }
inline BOOL    PtVisible(HDC, int, int) { return TRUE; }

//===========================================================================
// 9. Shim function declarations (implemented in win32_compat.cpp).
//===========================================================================
#ifdef __cplusplus
extern "C" {
#endif

// --- Rect / point geometry (pure math) ---
BOOL IsRectEmpty(const RECT* prc);
BOOL PtInRect(const RECT* prc, POINT pt);
BOOL SetRect(RECT* prc, int l, int t, int r, int b);
BOOL SetRectEmpty(RECT* prc);
BOOL CopyRect(RECT* dst, const RECT* src);
BOOL OffsetRect(RECT* prc, int dx, int dy);
BOOL InflateRect(RECT* prc, int dx, int dy);
BOOL IntersectRect(RECT* dst, const RECT* a, const RECT* b);
BOOL UnionRect(RECT* dst, const RECT* a, const RECT* b);
BOOL EqualRect(const RECT* a, const RECT* b);

// --- Critical sections ---
void InitializeCriticalSection(LPCRITICAL_SECTION cs);
void DeleteCriticalSection(LPCRITICAL_SECTION cs);
void EnterCriticalSection(LPCRITICAL_SECTION cs);
void LeaveCriticalSection(LPCRITICAL_SECTION cs);
BOOL TryEnterCriticalSection(LPCRITICAL_SECTION cs);

// --- Events / mutexes / waitable handles ---
HANDLE CreateEventA(LPSECURITY_ATTRIBUTES sa, BOOL manualReset, BOOL initialState, LPCSTR name);
BOOL   SetEvent(HANDLE h);
BOOL   ResetEvent(HANDLE h);
HANDLE CreateMutexA(LPSECURITY_ATTRIBUTES sa, BOOL initialOwner, LPCSTR name);
BOOL   ReleaseMutex(HANDLE h);
DWORD  WaitForSingleObject(HANDLE h, DWORD ms);
DWORD  WaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL waitAll, DWORD ms);
BOOL   CloseHandle(HANDLE h);

// --- Threads ---
HANDLE CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stack,
                    LPTHREAD_START_ROUTINE start, LPVOID param,
                    DWORD flags, LPDWORD threadId);
DWORD  ResumeThread(HANDLE h);
DWORD  SuspendThread(HANDLE h);
BOOL   SetThreadPriority(HANDLE h, int priority);
int    GetThreadPriority(HANDLE h);
DWORD  GetCurrentThreadId(void);
HANDLE GetCurrentThread(void);
void   ExitThread(DWORD code);
BOOL   TerminateThread(HANDLE h, DWORD code);
BOOL   GetExitCodeThread(HANDLE h, LPDWORD code);
void   Sleep(DWORD ms);

// --- Timing ---
DWORD timeGetTime(void);
DWORD GetTickCount(void);
BOOL  QueryPerformanceCounter(LARGE_INTEGER* p);
BOOL  QueryPerformanceFrequency(LARGE_INTEGER* p);
void  GetLocalTime(SYSTEMTIME* st);
void  GetSystemTime(SYSTEMTIME* st);
UINT  timeBeginPeriod(UINT period);
UINT  timeEndPeriod(UINT period);
typedef void (*LPTIMECALLBACK)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
UINT  timeSetEvent(UINT delay, UINT res, LPTIMECALLBACK cb, DWORD_PTR user, UINT flags);
UINT  timeKillEvent(UINT id);
typedef UINT MMRESULT;

// --- Debug / error ---
void  OutputDebugStringA(LPCSTR s);
DWORD GetLastError(void);
void  SetLastError(DWORD err);
void  DebugBreak(void);
BOOL  IsDebuggerPresent(void);

// --- Modules / dynamic loading ---
HMODULE GetModuleHandleA(LPCSTR name);
DWORD   GetModuleFileNameA(HMODULE mod, LPSTR buf, DWORD size);
HMODULE LoadLibraryA(LPCSTR name);
BOOL    FreeLibrary(HMODULE mod);
void*   GetProcAddress(HMODULE mod, LPCSTR name);

// --- File I/O (handle-backed) ---
HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                   DWORD disposition, DWORD flags, HANDLE templ);
BOOL   ReadFile(HANDLE h, LPVOID buf, DWORD toRead, LPDWORD read, LPVOID ovl);
BOOL   WriteFile(HANDLE h, LPCVOID buf, DWORD toWrite, LPDWORD written, LPVOID ovl);
DWORD  SetFilePointer(HANDLE h, LONG dist, LONG* distHigh, DWORD method);
DWORD  GetFileSize(HANDLE h, LPDWORD sizeHigh);
BOOL   SetEndOfFile(HANDLE h);
BOOL   FlushFileBuffers(HANDLE h);
DWORD  GetFileAttributesA(LPCSTR name);
BOOL   GetFileAttributesExA(LPCSTR name, GET_FILEEX_INFO_LEVELS level, LPVOID info);
BOOL   CreateDirectoryA(LPCSTR name, LPSECURITY_ATTRIBUTES sa);
DWORD  GetCurrentDirectoryA(DWORD size, LPSTR buf);
BOOL   SetCurrentDirectoryA(LPCSTR name);
HANDLE FindFirstFileA(LPCSTR pattern, LPWIN32_FIND_DATAA data);
BOOL   FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA data);
BOOL   FindClose(HANDLE h);
BOOL   DeleteFileA(LPCSTR name);

// --- Registry (file-backed) ---
LONG RegOpenKeyExA(HKEY key, LPCSTR sub, DWORD opts, DWORD sam, PHKEY result);
LONG RegCreateKeyExA(HKEY key, LPCSTR sub, DWORD reserved, LPSTR cls, DWORD opts,
                     DWORD sam, LPSECURITY_ATTRIBUTES sa, PHKEY result, LPDWORD disp);
LONG RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type,
                      LPBYTE data, LPDWORD size);
LONG RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type,
                    const BYTE* data, DWORD size);
LONG RegDeleteValueA(HKEY key, LPCSTR name);
LONG RegCloseKey(HKEY key);

// --- MMIO (RIFF) ---
HMMIO mmioOpen(char* filename, LPMMIOINFO info, DWORD flags);
DWORD mmioClose(HMMIO h, UINT flags);
LONG  mmioRead(HMMIO h, char* buf, LONG len);
LONG  mmioWrite(HMMIO h, const char* buf, LONG len);
LONG  mmioSeek(HMMIO h, LONG offset, int origin);
DWORD mmioGetInfo(HMMIO h, LPMMIOINFO info, UINT flags);
DWORD mmioDescend(HMMIO h, LPMMCKINFO cki, const MMCKINFO* parent, UINT flags);
DWORD mmioAscend(HMMIO h, LPMMCKINFO cki, UINT flags);

// --- SRW locks (slim reader/writer) ---
void InitializeSRWLock(PSRWLOCK lock);
void AcquireSRWLockExclusive(PSRWLOCK lock);
void ReleaseSRWLockExclusive(PSRWLOCK lock);
void AcquireSRWLockShared(PSRWLOCK lock);
void ReleaseSRWLockShared(PSRWLOCK lock);

// --- Misc UI ---
int   MessageBoxA(HWND owner, LPCSTR text, LPCSTR caption, UINT type);
int   LoadStringA(HINSTANCE inst, UINT id, LPSTR buf, int bufMax);

// --- MSVC io.h find family (real, over opendir/readdir) ---
intptr_t _findfirst(const char* spec, struct _finddata_t* data);
int      _findnext(intptr_t handle, struct _finddata_t* data);
int      _findclose(intptr_t handle);

#ifdef __cplusplus
} // extern "C"
#endif

// ANSI/Unicode-neutral aliases used by the codebase (it builds _MBCS / ANSI).
#define CreateFile        CreateFileA
#define CreateEvent       CreateEventA
#define CreateMutex       CreateMutexA
#define GetModuleHandle   GetModuleHandleA
#define GetModuleFileName GetModuleFileNameA
#define LoadLibrary       LoadLibraryA
#define GetFileAttributes GetFileAttributesA
#define CreateDirectory   CreateDirectoryA
#define GetCurrentDirectory GetCurrentDirectoryA
#define SetCurrentDirectory SetCurrentDirectoryA
#define FindFirstFile     FindFirstFileA
#define FindNextFile      FindNextFileA
#define DeleteFile        DeleteFileA
#define RegOpenKeyEx      RegOpenKeyExA
#define RegCreateKeyEx    RegCreateKeyExA
#define RegQueryValueEx   RegQueryValueExA
#define RegSetValueEx     RegSetValueExA
#define RegDeleteValue    RegDeleteValueA
#define MessageBox        MessageBoxA
#define LoadString        LoadStringA
#define OutputDebugString OutputDebugStringA
#define FindResource      FindResourceA
#define GetFileVersionInfoSize GetFileVersionInfoSizeA
#define GetFileVersionInfo     GetFileVersionInfoA
#define VerQueryValue          VerQueryValueA
#define GetDriveType           GetDriveTypeA
#define GetVolumeInformation   GetVolumeInformationA
#define GetSystemDirectory     GetSystemDirectoryA
#define GetWindowsDirectory    GetWindowsDirectoryA
#define GetDiskFreeSpace       GetDiskFreeSpaceA
#define GetCommandLine         GetCommandLineA
#define PathGetArgs            PathGetArgsA
#define GetClassInfo           GetClassInfoA
#define TranslateAccelerator   TranslateAcceleratorA
#define LoadAccelerators       LoadAcceleratorsA
#define WritePrivateProfileString WritePrivateProfileStringA
#define GetPrivateProfileInt      GetPrivateProfileIntA
#define GetPrivateProfileString   GetPrivateProfileStringA
#define CreateFontIndirect     CreateFontIndirectA
#define WriteProfileString     WriteProfileStringA
#define GetProfileString       GetProfileStringA
#define GetProfileInt          GetProfileIntA
#define GetVersionEx           GetVersionExA
#define ChangeDisplaySettings  ChangeDisplaySettingsA
#define ChangeDisplaySettingsEx ChangeDisplaySettingsExA
#define EnumDisplaySettings    EnumDisplaySettingsA
#define IsBadStringPtr         IsBadStringPtrA
#define OutputDebugStringW     OutputDebugStringA
#define FindWindow             FindWindowA
#define CreateProcess          CreateProcessA
#define CreateSemaphore        CreateSemaphoreA
#define GetEnvironmentVariable GetEnvironmentVariableA
#define RegisterClassEx        RegisterClassExA
// NOTE: GetWindowText / GetObject are MFC member methods too — do NOT alias them
// (a macro would rewrite the member definitions). Global call sites use the A form.
// GDI text A-aliases — like <windows.h>, these intentionally also rewrite the
// CDC stub's member calls (dc.DrawText -> dc.DrawTextA, which the stub provides).
#define DrawText               DrawTextA
#define TextOut                TextOutA
#define ExtTextOut             ExtTextOutA
#define GetTextExtentPoint32   GetTextExtentPoint32A
#define GetTextMetrics         GetTextMetricsA
#define WIN32_FIND_DATA   WIN32_FIND_DATAA

// Semaphore → a signaled manual-reset event so waits pass through (bring-up).
// Defined here (after the extern "C" CreateEventA/SetEvent declarations).
inline HANDLE CreateSemaphoreA(LPSECURITY_ATTRIBUTES, LONG, LONG, LPCSTR) { return CreateEventA(NULL, TRUE, TRUE, NULL); }
inline BOOL   ReleaseSemaphore(HANDLE h, LONG, LPLONG) { return SetEvent(h); }
inline UINT_PTR SHAppBarMessage(DWORD, PAPPBARDATA) { return 0; }
// CRT thread spawn → CreateThread. The MSVC proc is `unsigned __stdcall(void*)`;
// CreateThread's is `DWORD(LPVOID)` — same shape on x86-64, so cast.
typedef unsigned (*_en_beginthreadex_proc)(void*);
inline uintptr_t _beginthreadex(void*, unsigned, _en_beginthreadex_proc start, void* arg, unsigned flags, unsigned* thrdaddr) {
    DWORD tid = 0;
    HANDLE h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, arg, flags, &tid);
    if (thrdaddr) *thrdaddr = (unsigned)tid;
    return (uintptr_t)h;
}
inline void _endthreadex(unsigned code) { ExitThread((DWORD)code); }
inline BOOL    GetIconInfo(HICON, PICONINFO ii) { if (ii) memset(ii, 0, sizeof(*ii)); return FALSE; }
inline BOOL    GetCursorInfo(PCURSORINFO) { return FALSE; }
inline HANDLE  GetCurrentProcess(void) { return (HANDLE)(intptr_t)-1; }
inline DWORD   GetCurrentProcessId(void) { return (DWORD)getpid(); }
// RtlCaptureStackBackTrace is declared `extern "C"` by callers (Perf.cpp), so it
// is DEFINED in win32_compat.cpp (not here) to avoid a linkage conflict.
inline _CRT_ALLOC_HOOK _CrtSetAllocHook(_CRT_ALLOC_HOOK) { return NULL; }
inline LPVOID  VirtualAlloc(LPVOID, SIZE_T n, DWORD, DWORD) { return malloc(n); }
inline BOOL    VirtualFree(LPVOID p, SIZE_T, DWORD) { free(p); return TRUE; }
inline BOOL    ScrollWindow(HWND, int, int, const RECT*, const RECT*) { return TRUE; }
inline HHOOK   SetWindowsHookEx(int, HOOKPROC, HINSTANCE, DWORD) { return NULL; }
inline void    GlobalMemoryStatus(LPMEMORYSTATUS s) { if (s) { memset(s, 0, sizeof(*s)); s->dwLength = sizeof(*s); } }
inline BOOL    GetVersionExA(OSVERSIONINFOA* p) { if (p) { p->dwMajorVersion = 6; p->dwMinorVersion = 2; p->dwPlatformId = VER_PLATFORM_WIN32_NT; p->szCSDVersion[0] = 0; } return TRUE; }
inline LONG    ChangeDisplaySettingsA(DEVMODEA*, DWORD) { return DISP_CHANGE_SUCCESSFUL; }
inline LONG    ChangeDisplaySettingsExA(LPCSTR, DEVMODEA*, HWND, DWORD, LPVOID) { return DISP_CHANGE_SUCCESSFUL; }
inline BOOL    EnumDisplaySettingsA(LPCSTR, DWORD, DEVMODEA*) { return FALSE; }
inline UINT    SetTextAlign(HDC, UINT) { return 0; }
inline BOOL    WriteProfileStringA(LPCSTR, LPCSTR, LPCSTR) { return TRUE; }
inline DWORD   GetProfileStringA(LPCSTR, LPCSTR, LPCSTR def, LPSTR buf, DWORD n) { const char* d = def ? def : ""; DWORD l = (DWORD)strlen(d); if (buf && n) { if (l > n-1) l = n-1; memcpy(buf, d, l); buf[l] = 0; } return l; }
inline UINT    GetProfileIntA(LPCSTR, LPCSTR, INT def) { return (UINT)def; }
inline _se_translator_function _set_se_translator(_se_translator_function) { return NULL; }
inline BOOL    IsBadWritePtr(LPVOID, UINT_PTR) { return FALSE; }
inline BOOL    IsBadReadPtr(const void*, UINT_PTR) { return FALSE; }
inline BOOL    IsBadStringPtrA(LPCSTR, UINT_PTR) { return FALSE; }
inline BOOL    PrintWindow(HWND, HDC, UINT) { return FALSE; }
ATOM    RegisterClassExA(const WNDCLASSEXA*);
inline int     ShowCursor(BOOL) { return 0; }
inline BOOL    GetProcessMemoryInfo(HANDLE, PPROCESS_MEMORY_COUNTERS c, DWORD) { if (c) memset(c, 0, sizeof(*c)); return FALSE; }
inline WORD    GetUserDefaultLangID(void) { return 0x0409; }   // en-US
inline BOOL    MessageBeep(UINT) { return TRUE; }
inline HWND    GetFocus(void) { return NULL; }
inline LONG    GetDialogBaseUnits(void) { return MAKELONG(8, 16); }   // typical 8x16
inline DWORD   GetLogicalDrives(void) { return 0; }
inline WORD    GetWindowWord(HWND, int) { return 0; }
inline WORD    SetWindowWord(HWND, int, WORD) { return 0; }
inline BOOL    IsBadCodePtr(FARPROC) { return FALSE; }
inline DWORD   GetVersion(void) { return 0x0006; }   // major 6
inline DWORD   GetUserDefaultLCID(void) { return 0x0409; }
inline DWORD   GetEnvironmentVariableA(LPCSTR name, LPSTR buf, DWORD n) {
    const char* v = name ? getenv(name) : nullptr;
    if (!v) { if (buf && n) buf[0] = 0; return 0; }
    DWORD l = (DWORD)strlen(v);
    if (buf && n) { if (l > n-1) l = n-1; memcpy(buf, v, l); buf[l] = 0; }
    return l;
}
inline BOOL CreateProcessA(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL,
                           DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION pi) {
    if (pi) { pi->hProcess = NULL; pi->hThread = NULL; pi->dwProcessId = 0; pi->dwThreadId = 0; }
    return FALSE;   // process spawning not supported on the Linux bring-up
}
inline WORD    GetSystemDefaultLangID(void) { return 0x0409; }
typedef int (*_PNH)(size_t);
inline _PNH    _set_new_handler(_PNH) { return NULL; }
inline int     _set_new_mode(int) { return 0; }

#endif // WIN32_COMPAT_H
