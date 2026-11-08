#ifndef __STDAFX_H__
#define __STDAFX_H__

#ifdef _WIN32
#pragma warning (disable : 4201)
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>
#include <nb30.h>   // NetBIOS (NCB) — Windows-only; CNetbios path only
#pragma warning (disable : 4704)
#else
// POSIX build (Linux/macOS): no windows.h / nb30.h. Pull the Win32-on-POSIX shim
// so the portable CSockets translation units (davenet.cpp / sockets.cpp) compile.
// CNetbios is #ifdef _WIN32-guarded out in _davenet.h, so nb30.h isn't needed here.
#include <stdlib.h>
#include "win32_compat.h"
#endif

#endif
