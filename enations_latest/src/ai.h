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
void AiMessage( DWORD dwID, CNetCmd const * pMsg, int iLen);
void AiSaveGame( CArchive& ar );
void AiLoadGame( CArchive& ar, BOOL bLocal );
void AiLoadComplete( void );
BOOL AiOppoFire (CUnit * pUnit, CUnit const * pTarget);
int  AiNextRsrch (CPlayer * pPlyr, int iCompleted);
void AiCityCenter (CHexCoord & _hex);

#endif
