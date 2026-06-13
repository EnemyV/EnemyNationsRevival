# Voice comms design — Enemy Nations SDL2

Three tiers of player voice, all sharing one codec and SDL2 audio. Cross-platform
by construction (SDL2 audio + pure-math μ-law + the existing vdmplay transport).

| Tier | What | Gated by | Transport model |
|------|------|----------|-----------------|
| 1. Voice mail | record clip → send → recipient plays later | **Telephone** | store-and-forward (reliable, chunked) |
| 2. Live call (1:1) | real-time two-way voice | **Telephone** | streaming (datagram preferred) |
| 3. Conference call | live group voice (3+) | **Conference Calling** (NEW tech) | streaming mesh |
| Quality upgrade | double the sample rate (8 → 16 kHz) | **HD Audio** (NEW tech, prereq Conference Calling) | n/a — applies to all of the above |

Tech tree: **Telephone → Conference Calling → HD Audio**. Telephone unlocks voice
mail + 1:1 live calls (it already gates chat — `CPlayer::CanChat()`). Conference
Calling unlocks the group form. HD Audio is a quality modifier that lifts every
voice feature from 8 kHz to 16 kHz.

---

## Implementation philosophy: all our own code, zero new libraries

Everything that touches the bytes is hand-written, portable, integer-only:
**μ-law codec, jitter buffer, mixer, resampler, packetization, the call protocol.**
No Opus/Speex/PortAudio/libsndfile — none of it. It's all small, deterministic
integer DSP that compiles identically on every platform.

The *only* thing that genuinely needs a platform service is handing PCM to/from the
sound card (open mic, open speaker, get a buffer callback). Two ways to draw that
line:

- **Use SDL2's audio device API for just that tap** (`SDL_OpenAudioDevice`).
  Recommended. SDL2 is already the engine's cross-platform layer — this adds **zero
  new dependencies**, and we still write 100% of the actual audio processing. SDL
  here is nothing but a portable `open device → callback(buffer)`; the buffer that
  flows through it is entirely our code.
- **Write three tiny raw backends ourselves** (WASAPI/WinMM, ALSA, CoreAudio) behind
  a 4-function interface (`open/close/read/write`). Truly zero libraries, but ~200–
  400 lines of fiddly per-OS code each, for no audible benefit over SDL's tap. Keep
  it as a fallback option, not the plan.

So: **all DSP + protocol = ours and library-free; device I/O = SDL's one-call tap
(no new dep), with a hand-rolled-backend escape hatch if "no SDL at all" is ever a
hard requirement.** A `IVoiceDevice { open/close/read/write }` abstraction makes the
two interchangeable, so the choice never leaks into the rest of the code.

### Efficiency (real-time-safe, integer-only)
- **μ-law**: 256-byte decode LUT; branch-light encode (`~15` ops). No float anywhere.
- **Mix** (conference): sum decoded `int16` to `int32`, saturate to `int16`. Integer.
- **Resample** (only when mixing 8 kHz + 16 kHz sources): 16→8 = drop alternate
  samples; 8→16 = linear-interp doubling. A few adds/shifts per sample.
- **Jitter buffer / capture ring**: fixed-size, preallocated. **No heap allocation,
  no locks in the audio callback** — lock-free SPSC ring between the callback and the
  net thread. Hot path is a handful of integer ops per sample.
- Memory: a couple of fixed ring buffers (a few KB each). Bandwidth as specced
  (64/128 kbps). Nothing per-frame allocates.

---

## Shared: codec + audio

- **G.711 μ-law, mono, 8 or 16 kHz.** 16-bit linear ↔ 8-bit log. ~25 lines,
  stateless, zero dependencies. μ-law is sample-rate-agnostic — the rate is just
  metadata, so the *same* codec serves both quality levels.
  - 8 kHz = **64 kbps (8 KB/s)**, telephone quality (base).
  - 16 kHz = **128 kbps (16 KB/s)**, wideband ("HD Audio" tech).
- **The sample rate is carried in every clip/packet header**, never hard-coded, so
  a session can mix participants of different tech levels (see negotiation below).
  A `#define` only sets the *defaults* (8 kHz base, 16 kHz HD); 4 kHz remains a
  fallback knob for "absolute minimum."
- **Capture/playback through the `IVoiceDevice` tap** (SDL impl by default), spec
  `{rate, S16, mono}` where `rate` comes from the negotiated/clip value. Encode each
  captured S16 → μ-law byte; on the way out decode μ-law → S16. All of that is our
  code; the device only hands us a buffer.
- Single library-free module: `Voice.{h,cpp}` — `muLawEncode/Decode`, the integer
  mixer/resampler, a `VoiceCapture` (lock-free ring of μ-law bytes), a
  `VoicePlayback` (per-source jitter buffer), and the `IVoiceDevice` interface with
  one `SDLVoiceDevice` implementation (swap-in raw-OS backends later if ever needed).

### Quality / capability negotiation (HD Audio)
HD Audio doubles the rate but must interoperate with players who don't have it:
- **Voice mail:** the sender encodes at *their* best rate (16 kHz if they have HD,
  else 8 kHz) and stamps the rate in the clip header. The receiver simply plays at
  that rate — playback is rate-agnostic, so HD mail is audible to anyone. No
  receiver-side tech needed to *hear* it.
- **Live / conference:** at call setup each side advertises its max rate in the
  invite/accept (and roster join). The call runs at the **minimum common rate** —
  16 kHz only if *every* participant has HD Audio, otherwise 8 kHz. A late joiner
  without HD downgrades the whole call to 8 kHz (re-advertise on roster change).
- `CPlayer::CanHDAudio()` returns whether the *local* player may encode at 16 kHz;
  the negotiated rate is `min` across the participants' advertised maxes.

---

## Tier 1 — Voice mail (Telephone)

Already-stubbed in the original (`CMsgIPC::voice`, `IDB_VOC_MSG` icon, `rec_call`
SFX). Store-and-forward, like email but audio.

- **Record** button → capture up to a cap (5 s = 40 KB) → μ-law buffer.
- **Send**: a clip is far larger than `VP_MAXSENDDATA` (700 B), so reuse the game's
  proven chunked, reliable, reassembling **`CVPTransfer`** (the same path that
  ships the saved-game file to joining clients). No new networking.
- **Receive**: reassembled clip → new `kind=voice` item in `g_mailInbox` + `rec_call`
  SFX + "you have a call" event. **Play** button decodes + plays.

Effort: ~1–1.5 days. Lowest risk; fully self-contained.

---

## Tier 2 — Live 1:1 call (Telephone)

Real-time two-way. The hard part is **transport latency**, not the audio.

### Call control (reliable `CNetCmd`s — small, fit in 700 B)
New commands: `cmd_voice_invite`, `cmd_voice_accept`, `cmd_voice_decline`,
`cmd_voice_bye`. Flow: A invites B → B's client rings (`rec_call` SFX + accept/
decline UI) → on accept both open capture+playback and start streaming → either
side `bye` tears down. A simple in-game "call" widget shows active call + Hang Up.

### Media path
Two options; ship #1, upgrade to #2 if internet quality matters.

1. **Reliable chunks (simplest, LAN-grade).** Packetize μ-law into ≤350-byte
   `cmd_voice_data` messages (~44 ms each, ~23/s) over the existing reliable
   stream. Pro: reuses everything. Con: TCP retransmits stale audio under loss →
   latency spikes. Fine on localhost/LAN (our current target).
2. **Unreliable datagram (proper VoIP).** Add a "send unreliable to player X" path
   on vdmplay's existing **datagram link** (the one already used for discovery).
   Late/lost packets are simply dropped, not retransmitted — correct for voice.
   This is the only real new networking, and it's small (vdmplay already has the
   dg socket + addresses).

### Playback
Per-source **jitter buffer**: hold ~2 packets (~90 ms), play out at a steady rate,
drop or conceal late packets. Sequence-number each `voice_data`. Total mouth-to-ear
≈ packetization (44 ms) + network + buffer (90 ms) ≈ 150–250 ms — acceptable for
casual game voice; great on LAN.

Effort: ~2–3 days for path #1 (call control + capture/stream/jitter/playback);
+1 day for the datagram path.

---

## Tier 3 — Conference call (new "Conference Calling" tech)

Live voice among 3+ players. Same media path as Tier 2; the difference is topology
and mixing.

- **Topology: full mesh.** Each participant sends its μ-law stream to every other
  participant; each client **mixes locally** by decoding all incoming streams to
  linear, summing (with clip), and playing. No server-side mixer — simplest, and
  fine for small groups (3–5). Bandwidth scales O(N): a 4-way call = 3 out + 3 in
  per client (~24 KB/s each way at 8 kHz). Cap the conference size (e.g., 6).
- **Call control:** extend the invite/accept set with a participant roster
  (`cmd_conf_invite`, `cmd_conf_join`, `cmd_conf_leave`) so each client knows the
  current member list and streams to all of them.
- **Gating:** requires **Conference Calling** research (below). A 1:1 call only
  needs Telephone; promoting to 3+ needs Conference Calling on the *initiator*.

Effort: ~1–2 days on top of Tier 2 (roster management + local mixer).

---

## New research topics: Conference Calling + HD Audio

- Add `conference_calling` **and** `hd_audio` to the `CRsrchArray` enum **at the
  end** (before `num_types`) — appending avoids shifting existing indices, so
  serialized research state in old saves stays valid. (Add both now even if HD ships
  later, so the enum order is final.)
- Add helpers to `CPlayer` next to `CanChat()`:
  - `BOOL CanConference() { return GetRsrch(CRsrchArray::conference_calling).m_bDiscovered; }`
  - `BOOL CanHDAudio()    { return GetRsrch(CRsrchArray::hd_audio).m_bDiscovered; }`
- Research-tree data: add the two items wherever the tree is defined (RESEARCH data
  / AI research tables in caigmgr.cpp use these indices), with prereqs
  **Telephone → Conference Calling → HD Audio**, plus name/desc strings + icons.
  This tree wiring + `SDL2ResearchDialog` listing is the only non-trivial part —
  same work as adding any new tech.

---

## Open questions & unproven assumptions (read before building)

Reviewed against the code; these are the things that are *not* yet proven and could
change the plan:

1. **Adding a research tech is more than an enum value.** The research tree
   (point cost, prereqs, required buildings) is **baked into binary RIFF data**
   (`theDataFile "research"/RSRH` + names in `LANG`), and `CRsrchArray::Open()`
   asserts `iSize + 1 == num_types`. So Conference Calling + HD Audio need *either*
   the binary data regenerated *or* a **code-append after `Open()`** (push the new
   `CRsrchItem`s with hard-coded cost/prereq, grow the array, relax that assert).
   Plan on the code-append path; it's real work, not a one-liner.
   - *Good news:* old saves are fine — `CPlayer` load (player.cpp:857-865) already
     grows `m_aRsrch` to the current research count and defaults new slots.
2. **Live voice over the reliable game-message path is probably unusable, even on
   LAN.** Game data rides `AddToQueue → ProcessAllMessages`, which is coupled to the
   sim loop and has **flow control** (`CMsgPauseMsg` when the queue passes
   `MIN_NUM_MESSAGES`) plus reliable, *ordered* delivery (head-of-line blocking on a
   single lost packet). ~23 voice msg/s could trip flow-off and add latency. **This
   likely promotes the unreliable datagram path from "optional phase 4" to a
   prerequisite for Tiers 2–3.** Needs a measurement spike before committing to
   path #1 for live calls.
3. **vdmplay unreliable per-player send — ✅ RESOLVED (spike, code).** vpengine.cpp
   2032-2044: per-player `SendData` routes to `m_safeLink->Send` (reliable TCP) when
   `VP_MUSTDELIVER` is set, else `m_unsafeLink->SendTo` (**unreliable datagram**).
   Receive side exists too (`OnUnsafeData → ReceiveFrom`, both local + remote
   sessions). So the datagram media path is real; we only need a thin
   `CNetApi::SendUnreliable(playerId,...)` wrapper (call `vpSendData` **without**
   `VP_MUSTDELIVER`) + voice-packet interception in `OnNetMsg` (like the cmd_chat
   intercept). Packets must stay ≤ `VP_MAXSENDDATA` (700 B ≈ 87 ms @ 8 kHz). End-to-
   end *latency* still wants a 2-instance measurement, but the path is proven.
4. **SDL audio capture in this build — ✅ RESOLVED (spike).** `d:\tmp\voicespike`:
   SDL 2.30.12 / WASAPI opened a capture device at **exactly 8 kHz mono S16** (no
   resampling), captured 1.92 s of real buffer, opened a playback device and played
   back, and our μ-law round-tripped at 50% size. Capture works in this exact build.
   (One caveat to retest with a *separate* playback device while SDL_mixer holds the
   game's output — the spike used its own device; in-game we open a second device.)
5. **Clock drift not addressed.** Sender capture clock vs receiver playback clock
   drift over a long call → buffer slowly fills or starves. Needs trivial drift
   compensation (drop/insert a sample when the jitter buffer crosses hi/lo marks).
6. **`CVPTransfer` reuse — ✅ RESOLVED (code).** `SendDataTo(to, from, buf, len)`
   sends any buffer; it's the *same* class the game uses to ship the saved file to
   joiners, with a proven pattern: announce size (a small `CNetCmd`) → receiver
   `ReceiveDataFrom` → both call `OnTimer`+`ProcessNotification` per VP notification
   → `Done`. Documented limit: **one transfer per player-pair at a time** → serialize
   voice mails per recipient, and don't send while a player is still join-transferring.
7. **Conference mesh on the datagram path scales fine** (it no longer touches the sim
   queue — see #2). Cap conference size (~6) to bound mesh fan-out.
8. **SDL_mixer coexistence — ✅ RESOLVED (spike).** `coexistspike`: Mix_OpenAudio
   (22050 stereo, the game's config) + a capture device + a **second** playback
   device all opened together on WASAPI, capture flowed while all three were live.
9. **Threading — audio on its own thread (see "Threading model" below).** Voice runs
   off the game loop so frame-rate hitches (render spikes, sim stalls, alt-tab) never
   stutter the stream. The binding constraint: **vdmplay net I/O stays on the main
   thread** (single-threaded `WSAAsyncSelect`/`WM_WINSOCK` design), so audio threads
   never call the network directly — lock-free SPSC queues bridge the two.

**Status: design validated end-to-end** except real live-call latency numbers
(needs a 2-instance run). All four scary unknowns (capture, mixer coexistence,
unreliable datagram send+receive surfacing as `VP_READDATA`, save-load growth) are
verified. Remaining work items are effort, not risk.

## Threading model

Voice processing runs on **dedicated audio threads, not the game loop**, so it stays
smooth through frame-rate hitches. The network layer is single-threaded, so the rule
is: **audio threads never touch the network; the main thread never touches the audio
device.** Two lock-free SPSC ring buffers bridge them.

```
 [SDL capture thread]                main thread (game loop / WM_WINSOCK)            [SDL playback thread]
   mic → μ-law encode  ── outRing ──►  drain outRing → SendUnreliable()
                                       OnNetMsg(VP_READDATA voice) ── inRing ──►  jitter buf → decode → mix → speaker
```

- **Audio threads = SDL's device callbacks** (capture + playback). They're OS-paced,
  high-priority, and steady — exactly what voice wants. (The spikes used queue mode;
  for the real thing we switch to **callback mode** so the device drives the cadence.)
  Each callback does only bounded integer work and **one lock-free ring op** — no
  mutexes, no allocation, no syscalls.
- **Main thread = all net I/O.** Every game frame (and on each `WM_WINSOCK`) it drains
  `outRing` → `SendUnreliable` for each active peer, and `OnNetMsg` pushes received
  voice payloads into `inRing`. If the main thread briefly stalls, capture keeps
  filling `outRing` and playback keeps draining the jitter buffer — audio doesn't
  gap; only packet send/recv batches up for that moment.
- **Rings:** single-producer/single-consumer, fixed capacity, wait-free. `outRing`:
  producer = capture cb, consumer = main. `inRing`: producer = main, consumer =
  playback cb. One per direction; per-peer fan-in handled by tagging packets with
  `senderId` and the playback thread mixing per-source jitter buffers.
- **Optional dedicated worker** instead of SDL callbacks: a `std::thread` ticking at
  ~20 ms could poll the queue API and own encode/decode, but the callback approach is
  simpler and lower-latency (device-paced vs sleep-paced). Either way the net stays on
  the main thread.
- **Lifecycle:** open devices + start rings on call-accept; stop + close on hang-up /
  last-peer-leaves / `SDL_AUDIODEVICEREMOVED`. No threads spun while idle.

> Net-thread-safety note: the AI worker threads in this port post game commands, but
> the safe assumption for vdmplay is single-threaded access. Keep `SendUnreliable` /
> `vpSendData` on the main thread; if profiling ever shows the per-frame drain is a
> bottleneck, revisit with a vdmplay-level lock rather than calling it off-thread.

## Edge cases & integration points

**Integration hooks (where this plugs into the existing code):**
- **Send (datagram):** new `CNetApi::SendUnreliable(playerId, data, len)` = `vpSendData`
  to that player **without** `VP_MUSTDELIVER`. (`Broadcast` hard-codes MUSTDELIVER,
  so don't reuse it.)
- **Receive:** voice packets arrive as `VP_READDATA`; intercept by message type in
  `CNetApi::OnNetMsg` **before `AddToQueue`** (exactly like the `cmd_chat` intercept)
  → hands bytes straight to playback, skipping the sim queue *and* `AssertMsgValid`.
- **Voice mail:** announce (`cmd_voice_mail_start {size}`) → `CVPTransfer`; hook its
  `ProcessNotification` into `OnNetMsg` next to the existing `m_pXferFromServer` calls.
- **New message types:** add `cmd_voice_*` to the `CNetCmd` enum **before
  `last_message`** (dispatch is by type; the bound check at ProcessMessage stays
  valid). Because voice is intercepted pre-queue, **no `AssertValid` is needed** for
  them (that's what crashed bldg-stat).
- **Research:** `CanChat()` (telephone) gates voice mail + 1:1; `CanConference()` gates
  group; `CanHDAudio()` raises the negotiated rate. Entry already gated in `GotoChat`.
- **Player list / addressing:** stream to `pPlr->GetNetNum()`; iterate `theGame.GetAll()`
  for the conference roster.

**Edge cases to handle:**
- **No microphone** (`SDL_GetNumAudioDevices(1)==0`): allow listen-only — grey out
  Record/Talk, still play incoming. (Spike confirmed the API path even with no mic.)
- **Device unplugged mid-call:** SDL posts `SDL_AUDIODEVICEREMOVED`; close the source
  and either reopen the new default or drop the call gracefully (don't crash).
- **Player leaves / dies / AI-takeover mid-call:** hook the existing leave path
  (`CmdToAi` / `plyr_dying`) to remove them from call rosters and free their playback
  source — same family as the AI-borrowed-list UAF we fixed; don't hold a freed
  `CPlayer*`.
- **Self-loopback:** never play your own outgoing audio (send only to *other*
  participants; drop packets whose `senderId == me`).
- **Datagram loss / reorder:** sequence-number each packet; jitter buffer reorders
  within a small window and conceals gaps with silence. Expected and fine.
- **HD↔base mixed session:** run at `min` advertised rate; if a non-HD player joins an
  HD conference, **renegotiate down to 8 kHz** on the roster-change message.
- **Concurrent voice mails to one recipient / overlap with join transfer:** serialize
  per player-pair (CVPTransfer limit); queue locally and send next on `Done`.
- **Clock drift on long calls:** when the jitter buffer crosses a hi/lo watermark,
  drop/insert one sample to resync — keeps it from slowly filling or starving.
- **In-game only:** voice is in-game (the lobby has its own text chat); the
  `GotoChat`/comm-window entry already requires a net game + comm tech.
- **Packet size:** keep `cmd_voice_data` payload ≤ `VP_MAXSENDDATA` (700 B) — that's
  ~87 ms @ 8 kHz / ~43 ms @ 16 kHz per packet; pick a packet duration under that.

## Cross-platform summary

- **Signal path (codec, mixer, resampler, jitter buffer, packetization, protocol):**
  100% our own integer code — identical on every platform, no library.
- **Device I/O:** one `IVoiceDevice` tap (default `SDLVoiceDevice` → zero new deps;
  optional hand-rolled WASAPI/ALSA/CoreAudio backends if "no SDL" is ever required).
- **Transport:** Tier 1 + Tier 2-path-1 reuse the existing vdmplay path (already the
  cross-platform networking layer being ported). Tier 2-path-2 adds an unreliable
  datagram send to the *same* vdmplay socket — no new OS sockets.
- **No new third-party dependencies at any tier**, and nothing platform-specific in
  the audio processing itself.

## Suggested phasing

1. `Voice.{h,cpp}` (codec + SDL capture/playback, rate carried per clip) + **Tier 1
   voice mail**. Self-contained, testable solo-ish, unblocks the comms UI.
2. **Tier 2 live 1:1** over reliable chunks (path #1) + rate negotiation hooks —
   works on LAN today.
3. **Conference Calling** tech + **Tier 3 mesh** group calls (roster + local mixer).
4. **HD Audio** tech — flip the negotiated rate to 16 kHz when all participants
   have it. Tiny once the rate is already per-clip/negotiated (steps 1–2 build that
   in), so this is mostly the research-tree entry + the `min`-rate check.
5. (Optional) datagram media path (#2) for internet-grade latency.

Because steps 1–2 carry the sample rate in the header and negotiate it, **HD Audio
costs almost nothing to add** — it's a research unlock that raises one negotiated
number, not a new code path.
