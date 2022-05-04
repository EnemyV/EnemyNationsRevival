////////////////////////////////////////////////////////////////////////////
//
//  ai.cpp :  AI interface implementation
//            Divide and Conquer AI
//               
//  Last update:    10/30/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "threads.h"
#include "ai.h"
#include "lastplnt.h"
#include "player.h"
#include "racedata.h"

#include "CAIData.hpp"
#include "CAIMgr.hpp"
#include "CAISavLd.hpp"
#include "CPathMap.h"

extern CRITICAL_SECTION cs;
extern CPathMap thePathMap;		// the map pathfinding object (no yield)

// standard exception for yielding
CException *pException = NULL;

CAITaskList *plTaskList = NULL;	// standard CAITask list
CAIGoalList *plGoalList = NULL;	// standard CAIGoal list

CWordArray *pwaIG = NULL;		// initial goals

// external global reference for game data interface
CAIData *pGameData = NULL;

// the AI Manager List, a list of CAIMgr objects
// which is the central point for AI access by the game       
CAIMgrList *plAIMgrList = NULL;

static int iPlyrNum = 1;

void AiCityCenter (CHexCoord &  /* _hex */ )
{
}

// called by server each time a topic completes
int AiNextRsrch (CPlayer *pPlyr, int /*iCompleted*/ )
{
	CAIMgr *pAIMgr = (CAIMgr *)pPlyr->GetAiHdl();
	if( pAIMgr == NULL )
		return(0);

	return( pAIMgr->NextTopic(pPlyr) );
}


//
// a human player has droped out of a multi-player game
// so now the AI must take over and manage in the HP's place
//
BOOL AiTakeOverPlayer( CPlayer *pPlr ) 
{
	// create an CAIMgr for the new player
	if( AiNewPlayer(pPlr) )
		return( FALSE );

	int iID = pPlr->GetPlyrNum();
	
	CAIMgr *pAIMgr = plAIMgrList->GetManager( iID );
	if( pAIMgr == NULL )
		return( FALSE );

	// get the initial location of the HP Player
	int iX = pPlr->m_hexMapStart.X();
	int iY = pPlr->m_hexMapStart.Y();

	// initialize AI units
	// initialize AI maps
	// initialize AI managers
	pAIMgr->AssumeControl( iX, iY );
		
	return (TRUE);
}

// This is the first call to the AI when creating a game
// iSmart == 0 (Easy to beat) - 3 (Impossible) - how smart the AI is
//   You also get a multiplier on initial points of .8, .95, 1.05, 1.2 (this may change)
// iNumAi - number of AI players in game
// iNumHuman - number of human players in the game
// return TRUE if you can't start.
BOOL AiInit (int iSmart, int iNumAi, int iNumHuman, int iStartPos)
{
	if( iSmart < 0 ||
		iNumAi < 0 ||
		iNumHuman < 0 ||
		iStartPos < 0 )
		return TRUE;

	if( pGameData != NULL )
		delete pGameData;

	// attempt to create the game data interface object	
	TRY
	{
		pException = new CException();

		// BUGBUG the CAIData constructor should be changed
		// as int iHexPerBlk and int iBlkPerSide are not
		// being passed and are not available now
		pGameData = new CAIData( iSmart, iNumAi, iNumHuman );
		pGameData->m_iStartPos = iStartPos;
	}
	CATCH( CException, e )
	{
		if( pException != NULL )
		{
			pException->Delete();
			pException = NULL;
		}

		if( pGameData != NULL )
		{
			delete pGameData;
			pGameData = NULL;
   		}

		return TRUE;
	}
	END_CATCH

	if( plAIMgrList != NULL )
		delete plAIMgrList;

	// attempt to create an CAIMgr list
	TRY
	{
		plAIMgrList = new CAIMgrList();
	}
	CATCH( CException, e )
	{
		if( plAIMgrList != NULL )
		{
			delete plAIMgrList;
			plAIMgrList = NULL;
   		}
		
		if( pGameData != NULL )
		{
			delete pGameData;
			pGameData = NULL;
   		}

		if( pException != NULL )
		{
			pException->Delete();
			pException = NULL;
		}

		return TRUE;
	}
	END_CATCH

	iPlyrNum = 1;
	return (FALSE);
}

// This call is made once for each AI player to create it.
// m_iPlyrNum is set on entry
// m_attrib must be set before exit
// m_AiHdl must be set before exit
// return TRUE if you can't start.
BOOL AiNewPlayer (CPlayer *pPlr)
{
	if( plAIMgrList == NULL )
		return TRUE;
	
	ASSERT_VALID( plAIMgrList );
	
	// BUGBUG will the CPlayer have valid data by this time?  Yes!
	int iID = pPlr->GetPlyrNum();
	
	CAIMgr *pAIMgr = plAIMgrList->GetManager( iID );
	if( pAIMgr == NULL )
	{
		TRY
		{
			// CAIMgr( int iPlayer )
			pAIMgr = new CAIMgr( iID );
			plAIMgrList->AddTail( (CObject *)pAIMgr );
		}
		CATCH( CException, e )
		{
			if( pAIMgr != NULL )
				delete pAIMgr;

			return TRUE;
	    }
	    END_CATCH
	}
	
	// make pointer to this CAIMgr available to the game
	pPlr->SetAiHdl ((DWORD)pAIMgr );

	// BUGBUG consider if the player is human and flag CAIMgr
	pAIMgr->SetAI( pPlr->IsAI() );
	
	// considering pGameData->m_iSmart, make adjustments to
	// CPlayer attribs to reflect "dumbness"
	// make legal adjustments to player racial characteristics
	pAIMgr->AdjustAttribs( pPlr );
	
	return (FALSE);
}

// iHexPerBlk - the number of hex's a block is wide or high (a block is ALWAYS square).
//   This is presently 64, 128, or 256
// iBlkPerSide - the number of blocks a map is wide or high (the map is always square).
//   Therefore the number of hexes wide (or high) a map is is iHexPerBlk * iBlkPerSide
BOOL AiWorldSize (int iHexPerBlk, int iBlkPerSide)
{
	if( iHexPerBlk < 0 ||
		iBlkPerSide < 0 )
		return TRUE;

	ASSERT_VALID( pGameData );

	// handle global game data object and exception for yielding
	if( pGameData == NULL )
		pGameData = new CAIData( 0,0,0 );

	// BUGBUG this is a different way to set these values
	// than anticipated.  See AiThread()
	pGameData->SetWorldSize( iHexPerBlk, iBlkPerSide );
	return (FALSE);
}

void AiSetup (CPlayer * pPlr)
{
	CAIMgr *pAIMgr = (CAIMgr *)pPlr->GetAiHdl();
	if( pAIMgr == NULL )
		return;

	// by this time, the pAiI->hex should have 
	// the initial location of the AI Player
	int iX = pPlr->m_hexMapStart.X();
	int iY = pPlr->m_hexMapStart.Y();
	
	// now the pAIMgr must create its CAIMap map, and 
	// CAIUnits list and CAIGoalMgr and CAITaskMgr objects
	try
	{
		pAIMgr->CreateData( iX, iY );
		theApp.BaseYield();

		pAIMgr->SetInitialPos();
		theApp.BaseYield();
	}
	catch( CException e )
	{
		throw(ERR_CAI_BAD_NEW);
	}
}

//
// this call is made by the game to tell the AI player
// thread to end itself and send a message back indicating
// the player is dead
//
// dwID is the ID returned from AiNewPlayer
//
void AiKillPlayer (DWORD dwID)
{
	CAIMgr *pAIMgr = (CAIMgr *)dwID;
	pAIMgr->SetDead();
}

// 
// this call is made by the game to actually delete the
// CAIMgr object
//
// dwID is the ID returned from AiNewPlayer
//
void AiDeletePlayer (DWORD dwID)
{
	CAIMgr *pAIMgr = (CAIMgr *)dwID;
	ASSERT_VALID( pAIMgr );
	
	if( plAIMgrList != NULL )
	{
		ASSERT_VALID( plAIMgrList );
		EnterCriticalSection(&cs);
		theGame.AiPlayerIsDead ( theGame.GetPlayerByPlyr ( pAIMgr->GetPlayer() ) );
		plAIMgrList->RemoveManager( pAIMgr->GetPlayer() );
		LeaveCriticalSection(&cs);
	}
}

// last call on game exit - shut everything down (program still running)
void AiExit ()
{

	try
		{
		// delete standard data
		if( plTaskList != NULL )
		{
			delete plTaskList;
			plTaskList = NULL;
		}
		if( plGoalList != NULL )
		{
			delete plGoalList;
			plGoalList = NULL;
		}

		if( pwaIG != NULL )
		{
			pwaIG->RemoveAll();
			delete pwaIG;
			pwaIG = NULL;
   	}
	
		// cleans up everything the AI had created
		if( plAIMgrList != NULL )
		{
			delete plAIMgrList;
			plAIMgrList = NULL;
		}
		// cleans up the game data object
		if( pGameData != NULL )
		{
			delete pGameData;
			pGameData = NULL;
		}
		if( pException != NULL )
		{
			pException->Delete();
			pException = NULL;
		}
		}

	catch (...)
		{
		plTaskList = NULL;
		plGoalList = NULL;
		pwaIG = NULL;
		plAIMgrList = NULL;
		pGameData = NULL;
		pException = NULL;
		}
}

// BUGBUG is this still necessary?
//
LRESULT CALLBACK AiWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}

// called once for each AI once the game starts. Return when the game ends
// dwID is the ID returned from AiNewPlayer
// hex is the location player will start at
void WINAPI AiThread (AI_INIT *pAiI)
{
	CAIMgr *pAIMgr = (CAIMgr *)pAiI->dwHdl;

	ASSERT_VALID( pAIMgr );

	try
	{
		// now fail into the endless loop yielding often
		while(1)
		{
			if ( theGame.GetAi ().GetCount () > 1 )
				::Sleep ( 0 );	// give time to the other AIs
			pAIMgr->Manage();
			myYieldThread();
		}
#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AiThread() player %d terminated ", pAIMgr->GetPlayer() );
#endif

	}
	catch (...)
	{

#ifdef _LOGOUT
logPrintf(LOG_PRI_ALWAYS, LOG_AI_MISC, 
"AiThread() player %d terminated ", pAIMgr->GetPlayer() );
#endif
	TRAP ();

	// ok, on the first two GPFs we start it up again
	CPlayer * pPlr = NULL;
	if ( (pAIMgr != NULL) && ( ! pAIMgr->m_bIsDead) )
		pPlr = theGame._GetPlayerByPlyr ( pAIMgr->GetPlayer() );
	if ( pPlr != NULL )
		{
		if ( pPlr->m_iNumAiGpfs < 2 )
			{
			pPlr->m_iNumAiGpfs += 1;
			// send a msg to the main thread
			CNetAiGpf msg ( pPlr );
			theGame.PostToServer (&msg, sizeof (msg));
			}
		else
			{
			// make it dead
			pPlr->m_iNumAiGpfs = 100;
			CMsgPlyrDying msg (pPlr);
			theGame.PostToAll (&msg, sizeof (msg));
			}
		}

	}

	myThreadTerminate();
}

//
// the AI message function called by the game on messages
//
void AiMessage( DWORD dwID, CNetCmd const *pMsg, int )
{
	// tell the manager that a message has come in
	CAIMgr *pAIMgr = (CAIMgr *)dwID;
	if( pAIMgr != NULL )
		pAIMgr->MessageArrived( pMsg );
}

void AiSaveGame( CArchive& ar )
{
	ar.Flush();
	CFile *pFile = ar.GetFile();

	CAIHexBuff aiHexBuff;

	// no game to save
	if( pGameData == NULL )
		{
		throw (ERR_CAI_BAD_FILE);
		return;
		}

	try
	{
		// the CGameData object stuff
		pFile->Write( (const void*)&pGameData->m_iSmart, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iNumAi, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iNumHuman, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iHexPerBlk, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iBlkPerSide, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iStructureSize, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iNumTransports, sizeof(int) );
		pFile->Write( (const void*)&pGameData->m_iNumBuildings, sizeof(int) );
		// mineral densities
		for( int i=0; i<CMaterialTypes::num_types; ++i )
		{
			CAIHex *paiHex = &pGameData->m_paihDensity[i];
			aiHexBuff.iX = paiHex->m_iX;
			aiHexBuff.iY = paiHex->m_iY;
			aiHexBuff.iUnit = paiHex->m_iUnit;
			aiHexBuff.dwUnitID = paiHex->m_dwUnitID;
			aiHexBuff.cTerrain = paiHex->m_cTerrain;

			pFile->Write( (const void*)&aiHexBuff, sizeof(CAIHexBuff) );
		}
	}
    catch( CFileException theException )
    {
		// how should write errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }


	if( plAIMgrList == NULL )
		return;

    if( plAIMgrList->GetCount() )
    {
        POSITION pos = plAIMgrList->GetHeadPosition();
        while( pos != NULL )
        {   
            CAIMgr *pMgr = (CAIMgr *)plAIMgrList->GetNext( pos );
            if( pMgr != NULL )
            {
            	ASSERT_VALID( pMgr );

                pMgr->SaveGame(pFile);
            }
        }
    }
}

void AiLoadGame( CArchive& ar, BOOL bLocal )
{

		if( plAIMgrList != NULL )
			{
			delete plAIMgrList;
			plAIMgrList = NULL;
			}

	// if not local we do just enough so we can save
	if ( bLocal )
		{
		try
		{
			plAIMgrList = new CAIMgrList();
		}
		catch( CException e )
		{
			if( plAIMgrList != NULL )
			{
				delete plAIMgrList;
				plAIMgrList = NULL;
  	 		}
			throw(ERR_CAI_BAD_NEW);
		}
	}

	// handle global game data object and exception for yielding
	if( pGameData == NULL )
		pGameData = new CAIData( 0,0,0 );
	if( pException == NULL )
		pException = new CException();

	ar.Flush();
	CFile *pFile = ar.GetFile();
	if( pFile == NULL )
		throw(ERR_CAI_BAD_FILE);

	// the CGameData stuff should be loaded separately from
	// the CAMgr stuff

	// the CGameData object stuff
	CAIHexBuff aiHexBuff;
	int iCnt;

	try
	{
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iSmart = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iNumAi = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iNumHuman = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iHexPerBlk = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iBlkPerSide = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iStructureSize = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iNumTransports = iCnt;
		pFile->Read( (void*)&iCnt, sizeof(int) );
		pGameData->m_iNumBuildings = iCnt;

		// mineral densities
		if( pGameData->m_paihDensity == NULL )
			pGameData->m_paihDensity = new CAIHex[CMaterialTypes::num_types];
		for( int i=0; i<CMaterialTypes::num_types; ++i )
		{
			int iBytes = 
				pFile->Read( 
				(void*)&aiHexBuff, sizeof(CAIHexBuff) );

			CAIHex *paiHex = &pGameData->m_paihDensity[i];

			paiHex->m_iX = aiHexBuff.iX;
			paiHex->m_iY = aiHexBuff.iY;
			paiHex->m_iUnit = aiHexBuff.iUnit;
			paiHex->m_dwUnitID = aiHexBuff.dwUnitID;
			paiHex->m_cTerrain = aiHexBuff.cTerrain;
		}
		i = pGameData->m_iHexPerBlk * pGameData->m_iBlkPerSide;

	// if not local we're done now
	if ( ! bLocal )
		return;

	thePathMap.Init( i, i );
	}
    catch( CFileException theException )
    {
		// how should read errors be reported?
        throw(ERR_CAI_BAD_FILE);
    }


	// now I guess, go through all the CPlayers looking
	// for AI players and do a create for the new CAIMgr
	// and call CAIMgr::LoadGame()
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
	{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_VALID (pPlr);

		if( pPlr->IsAI() )
		{
			CAIMgr *pAIMgr = NULL;
			try
			{
				// CAIMgr( int iPlayer )
				pAIMgr = new CAIMgr( pPlr->GetPlyrNum() );
				plAIMgrList->AddTail( (CObject *)pAIMgr );
			}
			catch( CException e )
			{
				if( pAIMgr != NULL )
					delete pAIMgr;

				throw(ERR_CAI_BAD_NEW);
	    	}
			// make pointer to this CAIMgr available to the game
			pPlr->SetAiHdl ((DWORD)pAIMgr );

			pAIMgr->LoadGame(pFile);
		}
	}
}

void AiLoadComplete( void )
{
}

//
// return TRUE if I can fire
// if return FALSE will do nothing (will NOT look for other)
//
BOOL AiOppoFire (CUnit *pUnit, CUnit const *pTarget )
{
	CAIMgr *pAIMgr = (CAIMgr *)pUnit->GetOwner()->GetAiHdl();
	if( pAIMgr == NULL )
		return(FALSE);

	return( pAIMgr->AutoFire( pUnit, pTarget ));
}

// end of ai