# Cross-Platform Multiplayer — Release 3 Plan

**Goal:** Working multiplayer across Windows + Linux + macOS for the 3.00.000 release.
**Owner/lead:** win. **Peers:** linux, mac, debugger, Linux2 (test node).
**Status:** P0 RESOLVED (see §4 risk #1) — netcode is server-authoritative **state-sync, not lockstep**,
so cross-platform determinism is NOT a blocker. P1 (CSockets) is the next work. Drive via `AGENT_SYNC.md`.

---

## 1. Current state (grounded in the code)

The networking is already abstracted behind a **polymorphic protocol interface** — we are
*adding an implementation*, not rewriting.

- `network/_davenet.h`: `class CProtocol` — abstract base with the full session API:
  `InitOk / AddName / AddGroupName / Call / Listen / Send / Receive / SendDatagram /
  ReceiveDatagram / CancelReceive / HangUp / DeleteName / Close`.
- `class CNetbios : public CProtocol` — the **Windows-only NetBIOS** implementation
  (Netapi32 NCBs). This is what makes MP Windows-only today.
- `network/davenet.h`: the C API the game calls — `naInit(ID, …)`, `naCall`, `naListen`,
  `naSend`, `naReceive`, `naSendDatagram`, `naHangUp`, `naClose`, `naHave(ID)`.
  - **`naInit` already takes a protocol ID**, and the constants already include
    **`NET_PROTO_TCP = 1`** (alongside `IPX`, `NETBIOS=3`, `MODEM`, `DIRECT`).
    The TCP path was *designed for* but never implemented.
- On POSIX the transport is currently **stubbed** → no real MP off Windows.
- Async model: NetBIOS completions post `WM_NET_COMPLETE` (HWND/PostMessage). The SDL2
  build has no HWND message pump — the port must route completions through the SDL2
  event/callback path instead.

**Net:** implement a portable socket transport as a sibling `CProtocol`, wire it to the
`NET_PROTO_TCP` path, and the game logic above davenet is largely unchanged.

## 2. Strategy

Implement **`CSockets : public CProtocol`** (new `network/sockets.{cpp,h}`) over:
- **TCP** for sessions (`Call`/`Listen`/`Send`/`Receive`) — length-prefixed framing over the
  stream so message boundaries survive.
- **UDP** for datagrams + **LAN host discovery** (`SendDatagram`/`ReceiveDatagram`, broadcast).
- Portable socket layer: `#ifdef _WIN32` winsock (`WSAStartup`, `closesocket`) vs BSD
  sockets (`<sys/socket.h>`, `close`); nonblocking + `select`/`poll` on a net thread.
- Map NetBIOS "names" → a logical name table (host/peer/player IDs) so the lobby/join logic
  is unchanged.
- Replace `WM_NET_COMPLETE`/HWND completion with the SDL2-build's event/callback dispatch.

Wire `naInit(NET_PROTO_TCP)` to construct `CSockets` on **all three** platforms.

## 3. Phases & gates

- **P0 — Design + audit (win + 1 peer).** Map exact call sites (`join.cpp`, `netcmd.cpp`,
  `netapi.cpp`, `new_game`), the host/join/lobby flow, and the completion dispatch. Inventory
  EVERY struct sent over the wire. Decide framing + discovery + name→session mapping. Output:
  a short design note on the board.
- **P1 — Portable socket transport.** Implement `CSockets`; winsock+BSD; TCP framing; UDP
  discovery; async completion via SDL2 events. Unit-smoke: loopback send/recv.
- **P2 — Serialization safety.** Audit wire structs for fixed-width types + explicit packing.
  (All targets are little-endian — x86-64 + ARM64 — so byte order is low-risk; **struct
  padding/alignment across MSVC/gcc/clang and any `long`/pointer-size assumptions are the
  real hazard.**) Add `#pragma pack`/static_asserts on sizes; add hton/ntoh only where needed.
- **P3 — Host server + lobby/join.** Get the in-tree host server actually serving: bind,
  UDP-advertise, accept joins, slot/race select, start. Direct-IP join + LAN discovery.
- **P4 — Testing gates (verify-twice, multi-agent).**
  1. **Win↔Win** (TCP replacing NetBIOS on one OS).
  2. **Linux↔Linux**, **mac↔mac**.
  3. **Cross: Win↔Linux↔mac.**
  Test harness = **two Linux VMs (Linux2 + the agent VM) + host**, plus Windows/mac.
- **P5 — Polish:** reconnect/timeout handling, error UX, docs, then fold into the pre-publish
  re-cut.

## 4. Top risks (call them out early)

1. **~~Lockstep determinism across platforms~~ — RESOLVED (not a blocker).** P0 audit (mac, verified
   by win) found the netcode is **server-authoritative state-sync, NOT lockstep**: clients `PostToServer`
   commands (netcmd.h "// sent to server", 59 sites); the server alone simulates (gated by
   `theGame.AmServer()`, 80 sites, mainloop.cpp:480/736/769/818/944/1219/1267; AI runs server-side); the
   server broadcasts authoritative **state** (`CMsgVehLoc`/`CMsgUnitDamage`/`CMsgVehNew`…, `PostToClient` 62
   sites) which clients **apply**, not re-simulate. No lockstep turn/tick barrier exists. ⇒ cross-platform
   float/RNG/compiler divergence will **not** desync. **Open sub-question (not determinism-critical):** do
   clients locally *predict* their own units? Authority is still the server's, so peer bit-determinism isn't
   required — but worth confirming for responsiveness behavior.
2. Async completion port (HWND/PostMessage → SDL2 events).
3. Struct packing across three compilers.
4. NAT/firewall for internet play (LAN-first; direct-IP/port-forward acceptable for v1).

## 5. Coordination

- **win:** lead, design (P0), Windows `CSockets` (winsock) + Win↔Win gate.
- **linux:** POSIX socket validation, Linux↔Linux gate, 2-VM harness (Linux2).
- **mac:** clang/ARM64 build + packing/endianness audit, mac gate.
- **debugger:** desync/crash QA on cross-platform sessions.
- Determinism question (risk #1) is answered in P0 before P1 code lands. No wire-format or
  save-format changes without lead + the human.
