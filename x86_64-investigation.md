# Enemy Nations: 32-bit to 64-bit (x86_64) Build Investigation

## Overview

Enemy Nations is currently built exclusively as a 32-bit (Win32/x86) application. This document catalogues every change required to produce a 64-bit (x64) build. The codebase has **zero** existing 64-bit awareness — no `#ifdef _WIN64`, no `DWORD_PTR`/`LONG_PTR` usage (except one instance in player.cpp:842).

---

## 1. Build System (CMake & Scripts)

### Changes required

| What | File | Line | Current | Required |
|------|------|------|---------|----------|
| Architecture flag | `BuildRelease.bat` | 84 | `-A Win32` | `-A x64` |
| Host toolset | `enations_latest/src/CMakeLists.txt` | 97 | `host=x86` | `host=x64` |
| Host toolset | `tools/vdmplay/VPDIAG/CMakeLists.txt` | 18 | `host=x86` | `host=x64` |
| `_X86_` define | `enations_latest/src/CMakeLists.txt` | 135 | defined | remove |
| `_X86_` define | `windward/wind22/CMakeLists.txt` | 56 | defined | remove |
| `_X86_` define | `tools/vdmplay/CMakeLists.txt` | 50 | defined | remove |
| `_X86_` define | `tools/vdmplay/VPDIAG/CMakeLists.txt` | 46 | defined | remove |
| SafeSEH | `enations_latest/src/CMakeLists.txt` | 182-186 | `/SAFESEH:NO` | remove (invalid for x64; code comment says to remove) |
| 32-bit lib dir | `enations_latest/src/CMakeLists.txt` | 208 | `../../tools/lib` | remove; use Windows SDK libs directly |
| nafxcw exclusion | `enations_latest/src/CMakeLists.txt` | 190 | `/NODEFAULTLIB:nafxcw.lib` | keep (same name in 64-bit), verify still needed |

---

## 2. Third-Party Libraries

### Miles Sound System (MSS) — BLOCKER
- `tools/mss/MSS32.DLL` — 32-bit PE binary, no source
- `tools/mss/MSS32.LIB` / `MSS.LIB` — 32-bit import/static libs
- Included via `enations_latest/src/stdafx.h:33` and `windward/wind22/include/stdafx.h:42`
- Linked: `enations_latest/src/CMakeLists.txt` line 168
- DLL copied to output: CMakeLists.txt line 218

**Fix:** Replace with SDL audio.
We should use SDL for this!

### WinG (wing32.lib) — BLOCKER
- `tools/lib/WING32.LIB` — 32-bit import lib for a Windows 3.1-era blitting API. No 64-bit version exists.
- Linked: `enations_latest/src/CMakeLists.txt` line 169
- Loaded dynamically: `windward/wind22/src/blt.cpp:233` — `LoadLibrary("wing32")`

**Fix:** WinG provides fast DIB-to-screen blitting. Replace with GDI `StretchDIBits`/`BitBlt` or DirectDraw blitting (the game already uses ddraw).

### Legacy DirectX Libraries
Located in `tools/lib/`: `ddraw.lib`, `dinput.lib`, `dsound.lib`, `d3drm.lib`, `dxguid.lib` — all 32-bit, from the DirectX 5/6 SDK era.

**Fix:** The modern Windows SDK (shipped with VS2022) provides 64-bit versions of `ddraw.lib`, `dsound.lib`, `dinput8.lib`, and `dxguid.lib`. Remove `target_link_directories` pointing to `tools/lib/` and link the SDK versions.

### DirectPlay (`dplay.lib`, `dplayx.lib`)
DirectPlay is used — confirmed: `tools/vdmplay/dpnet.cpp` dynamically loads `dplay.dll` and calls `DirectPlayCreate`/`DirectPlayEnumerate`. Also `enations_latest/src/advanced.cpp:37` includes `dplay.h`. However, the game's networking is modular (vdmplay wraps TCP/IP, modem, NetBIOS, and DirectPlay as separate transports).

**Fix:** DirectPlay is deprecated with no 64-bit support. Disable/remove the DirectPlay transport; keep TCP/IP.

### MFC
Already using dynamic MFC via `_AFXDLL`. `nafxcw.lib` is explicitly excluded (not linked). MSVC 2022 ships 64-bit MFC. No action needed.

### Libraries built from source
`vdmplay.dll` and `wind22.lib` are compiled from source and will automatically become 64-bit.

---

## 3. Pointer ↔ Integer Truncation (CRITICAL)

In 64-bit, pointers are 8 bytes. Casting to `DWORD`/`LONG`/`int` (4 bytes) silently truncates, causing crashes or data corruption.

### 3a. AI Handle System — pointer stored as DWORD

The AI system stores a `CAIMgr*` pointer in `CPlayer::m_dwAiHdl` (a `DWORD` field, player.h:524) and casts it back to a pointer throughout the code. This is the most pervasive pointer-truncation issue in the codebase.

| File | Line | Code |
|------|------|------|
| `player.h` | 524 | `DWORD m_dwAiHdl;` — **root cause: storage field is 4 bytes** |
| `ai.cpp` | 196, 589 | `pPlr->SetAiHdl( (DWORD)pAIMgr );` |
| `ai.cpp` | 51, 235, 606 | `CAIMgr* pAIMgr = (CAIMgr*)pPlyr->GetAiHdl();` |
| `ai.cpp` | 279-281 | `void AiDeletePlayer(DWORD dwID) { CAIMgr* pAIMgr = (CAIMgr*)dwID; }` |
| `caimgr.cpp` | 135 | `AiDeletePlayer( (DWORD)this );` |
| `netapi.cpp` | 1642, 1829, 1849, 2055, 3245 | `AiMessage( pPlr->GetAiHdl(), ... )` |
| `newworld.cpp` | 682, 711 | `pPlr->ai.dwHdl = pPlr->GetAiHdl()` |

**Fix:** Change `m_dwAiHdl` to `DWORD_PTR`. Update `SetAiHdl`/`GetAiHdl` signatures, `AiDeletePlayer`, and `AiMessage` to use `DWORD_PTR`. (~15 call sites)

### 3b. Save/Load FixUp — integer IDs stored in pointer fields

During deserialization, integer IDs are temporarily stored in pointer fields, then resolved via lookup. The `(DWORD)` cast extracts the ID.

| File | Line | Code |
|------|------|------|
| `new_unit.cpp` | 4018, 4326 | `m_pVehRepairing = theVehicleMap.GetVehicle( (DWORD)m_pVehRepairing );` |
| `new_unit.cpp` | 5435, 5437 | `m_pUnitTarget = ::GetUnit( (DWORD)m_pUnitTarget );` / `m_pUnitOppo` |
| `new_unit.cpp` | 5879, 5883, 5898 | `m_pBldg`, `m_pTransport`, `m_pVehLoadOn` — same pattern |

**Fix:** The stored value is a small integer, so on 64-bit the upper 32 bits will be zero — these will work in practice but should be changed to `(DWORD)(DWORD_PTR)` or better, use a union/separate ID field.

### 3c. Sprite init — pointer compared/cast as int

| File | Line | Code |
|------|------|------|
| `sprtinit.cpp` | 234, 239 | `if ((int) m_pProjSprite == -1)` / `m_pExpSprite` |
| `sprtinit.cpp` | 237, 242 | `theEffects.GetSprite(CEffect::projectile, (int) m_pProjSprite)` |

**Fix:** Same pattern as FixUp — integer sentinel stored in pointer field. Use `(INT_PTR)` for the -1 comparison.

### 3d. MCI API — `(DWORD)&struct` and `MAKELONG(m_hWnd)`

`movie.cpp` has 12 calls to `mciSendCommand` that pass struct pointers as `(DWORD)&struct`. The 4th parameter of `mciSendCommand` is `DWORD_PTR` on 64-bit.

| File | Lines | Code |
|------|-------|------|
| `movie.cpp` | 70, 150, 161, 170, 180, 181, 188, 207, 231, 242, 283, 356 | `mciSendCommand(..., (DWORD) &struct)` |
| `movie.cpp` | 229 | `mdpp.dwCallback = MAKELONG(m_hWnd, 0);` — **MAKELONG truncates HWND to 16 bits** |

**Fix:** Change all `(DWORD)&struct` to `(DWORD_PTR)&struct`. Replace `MAKELONG(m_hWnd, 0)` with `(DWORD_PTR)m_hWnd`.

### 3e. CoDec callback system — DWORD user-data carries pointers

The compression/decompression API passes a user-data `DWORD` through the callback chain, but callers store pointers in it:

| File | Line | Code |
|------|------|------|
| `player.cpp` | 2537 | `CoDec::Compress( ..., fnCompSave, (DWORD)&dlgMsg );` |
| `player.cpp` | 2403 | `static void fnCompSave( DWORD dwData, int iBlk )` — callback signature |
| `codec.cpp` | 37, 80, 102 | `CoDec::Compress`/`Decompress` — `DWORD dwData` parameter |
| `bpecodec.cpp` | 165 | Same pattern |
| `lzwcodec.cpp` | 21, 84 | Same pattern |
| `lzsscode.cpp` | 187, 291 | Same pattern |
| `huffmanc.cpp` | 10, 47 | Same pattern |

**Fix:** Change `DWORD dwData` to `DWORD_PTR dwData` in `FNCOMPSTAT` typedef and all CoDec method signatures. Update all callers.

### 3f. ACM and TAPI callbacks — `(DWORD)` on pointers

| File | Line | Code |
|------|------|------|
| `windward/wind22/src/acmutil.cpp` | 198 | `(DWORD)&done` — ACM format enum callback user-data |
| `tools/vdmplay/tapiutil.cpp` | 494, 688 | `(DWORD) this` — TAPI lineOpen callback instance data |

**Fix:** Change to `(DWORD_PTR)`.

### 3g. PostMessage with pointer cast to DWORD

| File | Line | Code |
|------|------|------|
| `windward/wind22/src/cache.cpp` | 277 | `::PostMessage( m_hWnd, MSG_CACHE, (DWORD)m_pCceOn, 0 );` |
| `tools/vdmplay/ipx16net.cpp` | 30 | `PostMessage(ecb->m_window, WM_COMMNOTIFY, 0, (DWORD) ecb);` |

**Fix:** Change `(DWORD)` to `(WPARAM)` or `(LPARAM)` respectively (both are pointer-width on 64-bit).

---

## 4. GetWindowLong / SetWindowLong → Ptr Variants (CRITICAL)

`GetWindowLong`/`SetWindowLong` store/retrieve values as `LONG` (4 bytes). On 64-bit, pointers stored via `DWL_USER`/`GWL_USERDATA` require the `Ptr` variants using `LONG_PTR` (8 bytes).

### All affected files (~47 call sites)

| File | Count | Notes |
|------|-------|-------|
| `enations_latest/src/advanced.cpp` | 12 | DWL_USER in dialog procs; line 717 casts `(DWORD)tapiObj` |
| `tools/vdmplay/advanced.cpp` | 12 | Nearly identical copy; line 749 casts `(DWORD)tapiObj` |
| `tools/vdmplay/vdmplay.cpp` | 2 | GWL_USERDATA; line 926 casts `(DWORD)this` |
| `tools/vdmplay/VPTEST/TCOMM.CPP` | 2 | GWL_USERDATA |
| `windward/wind22/src/msg_box.cpp` | 3 | DWL_USER; line 365 casts `(LONG)(void FAR*)(&mbd)` |
| `windward/wind22/src/flcctrl.cpp` | 1 | GWL_ID |
| `windward/wind22/src/subclass.cpp` | 10 | GWL_STYLE, GWL_EXSTYLE, GCL_WNDPROC |
| `windward/wind22/src/wndbase.cpp` | 1 | GCL_STYLE |
| `tools/vdmplay/datalog.cpp` | 2 | DWL_USER; line 52 casts `(LONG)pDd->pDl` |
| `tools/vdmplay/comstatd.cpp` | 2 | DWL_USER; line 179 casts `(LONG)lParam` |

### API replacement table

| Old | New |
|-----|-----|
| `GetWindowLong(h, DWL_USER)` | `GetWindowLongPtr(h, DWLP_USER)` |
| `SetWindowLong(h, DWL_USER, v)` | `SetWindowLongPtr(h, DWLP_USER, (LONG_PTR)v)` |
| `GetWindowLong(h, GWL_USERDATA)` | `GetWindowLongPtr(h, GWLP_USERDATA)` |
| `SetWindowLong(h, GWL_USERDATA, v)` | `SetWindowLongPtr(h, GWLP_USERDATA, (LONG_PTR)v)` |
| `GetWindowLong(h, GWL_STYLE)` | `GetWindowLongPtr(h, GWL_STYLE)` |
| `GetWindowLong(h, GWL_EXSTYLE)` | `GetWindowLongPtr(h, GWL_EXSTYLE)` |
| `GetWindowLong(h, GWL_ID)` | `GetWindowLongPtr(h, GWLP_ID)` |

### SetClassLong / GetClassLong → Ptr variants

| File | Line | Code | Fix |
|------|------|------|-----|
| `windward/wind22/src/subclass.cpp` | 752 | `(WNDPROC)::SetClassLong(m_hwnd, GCL_WNDPROC, (DWORD)wndproc)` | `SetClassLongPtr` + `GCLP_WNDPROC` + `(LONG_PTR)` |
| `windward/wind22/src/subclass.cpp` | 774 | `::SetClassLong(m_hwnd, GWL_WNDPROC, (DWORD)m_wndprocPrev)` | Same |
| `enations_latest/src/area.cpp` | 561, 1904 | `::SetClassLong(m_hWnd, GCL_HCURSOR, NULL)` | `SetClassLongPtr` + `GCLP_HCURSOR` |
| `enations_latest/src/world.cpp` | 350 | `::SetClassLong(m_hWnd, GCL_HCURSOR, NULL)` | Same |
| `windward/wind22/src/wndbase.cpp` | 55 | `GetClassLong(m_hWnd, GCL_STYLE)` | `GetClassLongPtr` |

---

## 5. Inline Assembly & .asm Files

MSVC x64 does **not** support inline `__asm` blocks.

### Assembly source files (exclude from build)
- `enations_latest/src/uthunk/libentry.asm` — Win16/32s thunking (obsolete)
- `tools/vdmplay/ecbpost.asm` — IPX callback (NOTE: we don't need IPX)
- `tools/vdmplay/nbpost.asm` — NetBIOS callback (NOTE: we don't need NetBIOS)

### Inline asm requiring rewrite

| File | Blocks | Description | Effort |
|------|--------|-------------|--------|
| `windward/wind22/src/dib.cpp` | 9 | Optimized pixel blitting (`rep movsd`, etc.) | **Large** |
| `windward/wind22/include/fixpoint.h` | 7 | Fixed-point math (`imul`/`idiv`) | **Large** |
| `windward/wind22/src/scanlist.cpp` | multiple | Scan list operations | Medium |
| `enations_latest/src/sprite.cpp` | multiple | Sprite blitting | Medium |
| `tools/makeriff/fixpoint.h` | 7 | Duplicate of wind22 fixpoint.h | (same fix) |

### Inline asm — trivial replacements

| File | Code | Fix |
|------|------|-----|
| `windward/wind22/include/thielen.h` (lines 84, 104) | `_asm int 3;` | `__debugbreak()` |
| `tools/mss/mssw.h` (line 38) | `__asm {int 3}` | `__debugbreak()` |
| `tools/sprite/stdafx.h` (line 18) | `_asm int 3;` | `__debugbreak()` |
| `tools/vdmplay/vdmplay.cpp` (line 1583) | `__asm int 3` | `__debugbreak()` |
| `enations_latest/src/network/netbios.cpp` | multiple | (NOTE: we don't need NetBIOS) |

---

## 6. Structure Packing & Serialization

### Packed structs with pointer members (CRITICAL)

```cpp
// sprite.h:1017-1046
#pragma pack( push, ctile, 1 )
class CTile {
    CSprite *  m_psprite;  // 4 bytes on x86, 8 bytes on x64
    BYTE       m_byType;
};
#pragma pack( pop, ctile )
```

CSimpleTile → CTile, CHex → CSimpleTile — the entire game map inherits this pointer. With pack(1), CTile is 5 bytes on x86, 9 bytes on x64.

**Impact:** CHex::Serialize uses member-by-member CArchive (terrain.cpp:3858) so **serialization is safe**. But any code assuming fixed struct size in memory (array strides, memcpy of arrays) will break.

**Other packed structs:**
| File | Pack | Pointer members? | Risk |
|------|------|-------------------|------|
| `netcmd.h:40-1493` | pack(1) | No | LOW |
| `vdmplay.h:61-315` | pack(8) | Audit needed | MEDIUM |

### Unpacked AI buffer structs with raw `ar.Write(&struct, sizeof(struct))`

These lack `#pragma pack` and are serialized by raw byte copy:

| Struct | File | Members | Risk |
|--------|------|---------|------|
| `GoalBuff` | `cai.h:546` | all `int` | LOW (uniform alignment) |
| `TaskBuff` | `cai.h:555` | all `int` | LOW |
| `MsgBuff` | `cai.h:514` | `int`, `DWORD` | LOW |
| `OpForBuff` | `cai.h:532` | `int`, `DWORD`, **`BYTE`**, `BOOL` | MEDIUM — padding after `BYTE cRelations` may differ |
| `UnitBuff` | `cai.h:485` | `int`, **`WORD`**, `DWORD` | MEDIUM — padding after `WORD wStatus` may differ |

Files performing raw struct I/O: `caigmgr.cpp` (lines 10178, 10216, 10304, 10335), `caimgr.cpp` (3004, 3062, 3201, 3258), `caiopfor.cpp` (1465, 1492), `caiunit.cpp` (1664, 1748), `caisavld.cpp` (390-797, 12 calls), `player.cpp` (2602, 2666-2673 for WINDOWPLACEMENT — safe, Windows-defined).

**Fix:** Add `#pragma pack(push, 1)` around OpForBuff and UnitBuff, or switch to member-by-member serialization.

// NOTE: we do not need to support backwards save compatibility

---

## 7. Legacy 16-bit Keywords

### `NEAR` — 7 instances (all in `enations_latest/src/caidata.cpp:19-25`)
```cpp
extern CStructure NEAR   theStructures;
extern CBuildingMap NEAR theBuildingMap;
// ... 5 more
```

### `FAR` — scattered across wing.h (7+), WHATAMI.CPP, msg_box.cpp, dpnet.cpp, tools/wing/ samples

### `FARPROC` — used in lastplnt.cpp, netbios.h, thrdapi.h, vpwinsk.cpp, vdmplay.cpp
`FARPROC` itself is fine on 64-bit (it's a function pointer typedef). The casts around it may need updating.

**Fix:** Remove all `NEAR` and `FAR` keywords (no-ops in 32-bit, meaningless in 64-bit). Leave `FARPROC` as-is.

---

## 8. Deprecated / Removed APIs

| API | File | Line | Fix |
|-----|------|------|-----|
| `IsBadWritePtr` | `enations_latest/src/lastplnt.cpp` | 106, 110 | Remove; use null checks |
| `IsBadCodePtr` | `enations_latest/src/lastplnt.cpp` | 110 | Same |
| `IsBadWritePtr` | `tools/vdmplay/tcpnet.cpp` | 973 | Same |

---

## 9. Calling Conventions

`__stdcall` is used on ~7 member functions in sprite.h (Draw methods) and various callback declarations. On x64, `__stdcall` and `__cdecl` are silently ignored — all functions use the x64 calling convention. **No code changes needed**, but be aware when debugging calling convention issues.

---

## Prioritized Implementation Plan

### Phase 1: Build system
1. `BuildRelease.bat`: `-A Win32` → `-A x64`
2. CMakeLists.txt (2 files): `host=x86` → `host=x64`
3. Remove `_X86_` define (4 files)
4. Remove `/SAFESEH:NO` linker flag
5. Remove `target_link_directories` to `tools/lib/`; link Windows SDK libs directly

### Phase 2: Library blockers
1. **MSS32 → SDL audio** — new audio backend
2. **WinG → GDI/DirectDraw** — replace blitting calls in blt.cpp
3. **DirectX** → Windows SDK 64-bit import libs
4. **DirectPlay** → disable transport in vdmplay; keep TCP/IP

### Phase 3: Pointer/integer truncation (~90 sites total)
1. AI handle system: `m_dwAiHdl` → `DWORD_PTR`, update signatures (~15 sites)
2. CoDec callback: `DWORD dwData` → `DWORD_PTR` across codec API (~10 sites)
3. GetWindowLong/SetWindowLong → Ptr variants (~47 sites)
4. SetClassLong/GetClassLong → Ptr variants (~6 sites)
5. mciSendCommand `(DWORD)&struct` → `(DWORD_PTR)` (12 sites in movie.cpp)
6. MAKELONG(m_hWnd) → `(DWORD_PTR)m_hWnd` (1 site)
7. PostMessage pointer casts → `(WPARAM)`/`(LPARAM)` (2 sites)
8. FixUp pointer-to-DWORD casts (7 sites in new_unit.cpp)
9. sprtinit.cpp pointer-to-int (4 sites)
10. ACM/TAPI callback casts (3 sites)

### Phase 4: Assembly & keywords
1. Replace `_asm int 3` with `__debugbreak()` (5 files, trivial)
2. Rewrite inline asm in dib.cpp, fixpoint.h, scanlist.cpp, sprite.cpp (the heavy lift)
3. Exclude obsolete .asm files from build
4. Remove `NEAR`/`FAR` keywords

### Phase 5: Struct layout & deprecated APIs
1. Add `#pragma pack` to OpForBuff/UnitBuff or switch to member-by-member serialization
2. Verify CTile/CHex memory layout assumptions
3. Remove `IsBadWritePtr`/`IsBadCodePtr` calls

### Phase 6: Validation
1. Compile with `/W4` — fix C4311 (pointer truncation), C4312 (int-to-pointer), C4302 (truncation)
2. Run the game and test: rendering, audio, AI, save/load, networking (TCP/IP)

---

## Estimated Scope

| Category | Sites | Effort |
|----------|-------|--------|
| Build system | 6 files | Small |
| MSS32 → SDL audio | 1 library | **Large** |
| WinG replacement | 1 library | Medium |
| DirectX/DirectPlay | SDK swap + disable 1 transport | Small |
| Pointer truncation fixes | ~90 call sites | Medium (mostly mechanical) |
| Inline assembly rewrite | 4 files | **Large** |
| Keyword/API cleanup | ~15 instances | Small |
| Struct packing | ~5 structs | Small |
| **Total** | | **Large project** |

The two biggest work items:
1. **MSS32.DLL → SDL audio** — requires implementing a new audio backend
2. **Inline assembly rewrite** — dib.cpp and fixpoint.h contain performance-critical blitting and fixed-point math
