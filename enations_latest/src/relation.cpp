//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// relation.cpp : NewRelations free function (CDlgRelations removed)
//

#include "stdafx.h"
#include "lastplnt.h"
#include "relation.h"
#include "player.h"
#include "msgs.h"
#include "building.inl"
#include "vehicle.inl"
#include "unit.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

void NewRelations( CPlayer* pPlyr, int iLevel )
{
	// if no change nothing to do
	if ( pPlyr->GetRelations() == iLevel )
		return;

	if ( pPlyr->IsMe() )
		return;

	pPlyr->SetRelations( iLevel );

	CMsgSetRelations msg( theGame.GetMe(), pPlyr, iLevel );
	theGame.PostToClient( pPlyr, &msg, sizeof( msg ) );

	// things may have changed (we may want to stop/start/switch targets)
	// - redo oppo fire
	POSITION pos = theBuildingMap.GetStartPosition();
	while ( pos != NULL )
	{
		DWORD       dwID;
		CBuilding*  pBldg;
		theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
		ASSERT_STRICT_VALID( pBldg );

		if ( pBldg->GetOwner()->IsMe() )
		{
			pBldg->SetOppo( NULL );

			// tell them the contents of our buildings if alliance
			if ( iLevel == RELATIONS_ALLIANCE )
				pBldg->UpdateStore( TRUE );
		}
	}

	pos = theVehicleMap.GetStartPosition();
	while ( pos != NULL )
	{
		DWORD     dwID;
		CVehicle* pVeh;
		theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
		ASSERT_STRICT_VALID( pVeh );
		if ( pVeh->GetOwner()->IsMe() )
			pVeh->SetOppo( NULL );
	}
}
