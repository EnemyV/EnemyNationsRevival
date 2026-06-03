#ifndef __AI_H__
#define __AI_H__

#include "player.h"
#include "netapi.h"

BOOL AiInit (int iSmart, int iNumAi, int iNumHuman, int iStartPos);
BOOL AiWorldSize (int iHexPerBlk, int iBlkPerSide);
BOOL AiNewPlayer (CPlayer *pPlr);
BOOL AiTakeOverPlayer (CPlayer *pPlr);
void AiKillPlayer (DWORD dwID);
void AiDeletePlayer (DWORD dwID);
void AiExit ();
void WINAPI AiThread (AI_INIT *pAiI);
void AiSetup (CPlayer * pPlr);
void AiMessage( DWORD_PTR dwID, CNetCmd const * pMsg, int iLen);   // dwID is a CAIMgr* — pointer-width
void AiSaveGame( CArchive& ar );
void AiLoadGame( CArchive& ar, BOOL bLocal );
void AiLoadComplete( void );
BOOL AiOppoFire (CUnit * pUnit, CUnit const * pTarget);
int  AiNextRsrch (CPlayer * pPlyr, int iCompleted);
void AiCityCenter (CHexCoord & _hex);
int  AiTotalPathCells ();   // sum of every AI's per-AI path-map scratch cells (EN_PERF gauge)

#endif
