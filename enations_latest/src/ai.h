#ifndef __AI_H__
#define __AI_H__

#include "player.h"
#include "netapi.h"

BOOL AiInit (int iSmart, int iNumAi, int iNumHuman, int iStartPos);
BOOL AiWorldSize (int iHexPerBlk, int iBlkPerSide);
BOOL AiNewPlayer (CPlayer *pPlr);
BOOL AiTakeOverPlayer (CPlayer *pPlr);
void AiKillPlayer (DWORD_PTR dwID);   // dwID is a CAIMgr* — pointer-width (x64)
void AiDeletePlayer (DWORD_PTR dwID); // dwID is a CAIMgr* — pointer-width (x64)
void AiExit ();
void WINAPI AiThread (AI_INIT *pAiI);
void AiSetup (CPlayer * pPlr);
// AiSetup split: AiSetupPre = start-hex + light lists (uses RNG); AiSetupHeavy =
// map/manager build + initial unit placement. AiSetup runs both in order.
void AiSetupPre (CPlayer * pPlr);
void AiSetupHeavy (CPlayer * pPlr);

// New-game create speed-up: snapshot the player-independent per-hex map data
// once before the AI-setup loop, then free it after (before AI threads start).
// While the cache is live, CAIData::GetCHexData serves from it instead of
// re-scanning the locked game map for every AI. See caidata.cpp.
void AiHexCacheBuild ();
void AiHexCacheFree ();
bool AiHexCacheActive ();   // TRUE while the setup snapshot is live (caidata.cpp)
void AiMapBaseFree ();      // frees the shared per-AI base map snapshot (caimap.cpp)
void AiMessage( DWORD_PTR dwID, CNetCmd const * pMsg, int iLen);   // dwID is a CAIMgr* — pointer-width

// Fan-out filter (CGame::PostToAllAi): FALSE = this AI provably ignores this
// message (every consumer is owner-gated), so skip the alloc+enqueue+wake.
// Conservative: unknown/intel-bearing types always return TRUE. AI-inbound
// delivery is server-local (AiMessage runs only on the server), so filtering
// cannot desync clients -- AI influence travels via normal net commands.
BOOL AiMessageWanted( CPlayer* pPlr, CNetCmd const* pMsg );
void AiSaveGame( CArchive& ar );
void AiLoadGame( CArchive& ar, BOOL bLocal );
void AiLoadComplete( void );
BOOL AiOppoFire (CUnit * pUnit, CUnit const * pTarget);
int  AiNextRsrch (CPlayer * pPlyr, int iCompleted);
void AiCityCenter (CHexCoord & _hex);
int  AiTotalPathCells ();   // sum of every AI's per-AI path-map scratch cells (EN_PERF gauge)

#endif
