//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// scenario.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "scenario.h"
#include "help.h"
#include "cpathmap.h"
#include "area.h"
#include "relation.h"
#include "chproute.hpp"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// find the HP rocket
static CBuilding * GetRocket ()
{

	return (theBuildingMap.GetBldg (theGame.GetMe ()->m_dwIDRocket) );
}

void GiveUnits ( CBuilding * pRocket, int * piTypes, int iNumUnits )
{

	for (int iInd=0; iInd<iNumUnits; iInd++, piTypes++)
		{
		CTransportData const * pData = theTransports.GetData ( * piTypes );
		CHexCoord hex = pRocket->GetExitDest ( pData, TRUE );
		CMsgPlaceVeh msg (pRocket, hex, pRocket->GetOwner()->GetPlyrNum (), * piTypes );
		msg.ToNew ();
		msg.m_dwID = theGame.GetID ();
		theGame.PostToAll (&msg, sizeof (msg));
		theGame.ProcessAllMessages ();
		ASSERT (theVehicleMap.GetVehicle (msg.m_dwID) != NULL);
		}
}

void GiveAiUnits ( int * piTypes, int iNumUnits )
{

	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_STRICT_VALID (pPlr);
		if (pPlr->IsAI ())
			{
			CBuilding * pRocket = theBuildingMap.GetBldg ( pPlr->m_dwIDRocket );
			if ( pRocket != NULL )
				GiveUnits ( pRocket, piTypes, iNumUnits );
			}
		}
}

// start scenario 1
static void Start1 ()
{

	CBuilding * pHpRckt = GetRocket ();
	if (pHpRckt == NULL)
		{
		ASSERT (FALSE);
		return;
		}

	// ok, we give the player 5 infantry & 2 spotters
	static int aiType[7] = { CTransportData::infantry, 
																		CTransportData::infantry, CTransportData::infantry, 
																		CTransportData::infantry, CTransportData::infantry,
																		CTransportData::med_scout, CTransportData::med_scout };
	GiveUnits ( pHpRckt, aiType, 7 );

	// we give the AI players infantry
	static int aiType2[9] = { CTransportData::infantry, 
																		CTransportData::infantry, CTransportData::infantry, 
																		CTransportData::infantry, CTransportData::infantry,
																		CTransportData::infantry, CTransportData::infantry,
																		CTransportData::infantry, CTransportData::infantry };
	GiveAiUnits ( aiType2, __min (9, 2 + theGame.m_iAi + theGame.m_iAi / 3 ) );

	// we need to find the closest AI and have it send an attack
	CBuilding * pBldgAtk = NULL;
	int iDist = -1;

	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner()->IsAI ()) && (pBldg->GetUnitType () == CStructureData::rocket))
			{
			if (pBldgAtk == NULL)
				{
				pBldgAtk = pBldg;
				iDist = CHexCoord::Dist (pHpRckt->GetHex (), pBldg->GetHex ());
				}
			else
				{
				int iTmp = CHexCoord::Dist (pHpRckt->GetHex (), pBldg->GetHex ());
				if (iTmp < iDist)
					{
					pBldgAtk = pBldg;
					iDist = iTmp;
					}
				}
			}
		}
	if (pBldgAtk == NULL)
		{
		ASSERT (FALSE);
		return;
		}

	// drop them iSideSize out
	CHexCoord hexFrom ( pHpRckt->GetExitHex () ), hexTo;
	int x = CHexCoord::Diff (pBldgAtk->GetExitHex().X() - hexFrom.X());
	x = __minmax ( -1, 1, x );
	x = hexFrom.X () + theGame.GetSideSize () * x;
	int y = CHexCoord::Diff (pBldgAtk->GetExitHex().Y() - hexFrom.Y());
	y = __minmax ( -1, 1, y );
	y = hexFrom.Y () + theGame.GetSideSize () * y;

	int iTrys = 5;
	while ( iTrys-- > 0 )
		{
		hexTo = CHexCoord ( x, y );
		hexTo.Wrap ();
		if ( thePathMap.GetPath ( hexFrom, hexTo, 0, 0, NULL, CTransportData::infantry, TRUE) &&
					thePathMap.GetPath ( hexFrom, hexTo, 0, 0, NULL, CTransportData::light_scout, TRUE) )
			goto GotIt;
		x += theGame.GetSideSize () / 2 - RandNum ( theGame.GetSideSize () );
		y += theGame.GetSideSize () / 2 - RandNum ( theGame.GetSideSize () );
		}
	hexTo = pBldgAtk->GetExitHex ();

GotIt:

	// ok, give them two infantry, a light scout and attack orders
	static int aiAtkType[3] = { CTransportData::light_scout, CTransportData::infantry, CTransportData::infantry };
	for (int iInd=0; iInd<3; iInd++)
		{
		CMsgPlaceVeh msg (hexTo, pHpRckt->GetExitHex (), pBldgAtk->GetOwner()->GetPlyrNum (), aiAtkType[iInd]);
		msg.ToNew ();
		msg.m_dwID = theGame.GetID ();
		theGame.PostToAll (&msg, sizeof (msg));
		theGame.ProcessAllMessages ();

		CMsgScenarioAtk msg2 (pHpRckt, pBldgAtk->GetOwner()->GetPlyrNum(), msg.m_dwID, CMsgScenarioAtk::must);
		theGame.PostToClient (pBldgAtk->GetOwner(), &msg2, sizeof (msg2));

		ASSERT (theVehicleMap.GetVehicle (msg.m_dwID) != NULL);
		}
}

// start scenario 2
static void Start2 ()
{

	CBuilding * pHpTrgt = GetRocket ();
	if (pHpTrgt == NULL)
		{
		ASSERT (FALSE);
		return;
		}

	// ok, we give the player 3 light tanks and 1 arty
	static int aiType3[4] = { CTransportData::light_art, CTransportData::light_tank,
																		CTransportData::light_tank, CTransportData::light_tank };
	GiveUnits ( pHpTrgt, aiType3, 4 );

	// same for the AI
	static int aiType2[8] = { CTransportData::light_tank, CTransportData::light_art,
																		CTransportData::light_tank, CTransportData::light_art,
																		CTransportData::light_tank, CTransportData::light_art,
																		CTransportData::light_tank, CTransportData::light_art };
	GiveAiUnits ( aiType2, __min (8, 1 + theGame.m_iAi + theGame.m_iAi / 3 ) );

	// we need to find the second & third closest AI and have each send an attack
	CBuilding * apBldgAtk[3] = { NULL, NULL, NULL };
	int aiDist[3];

	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner()->IsAI ()) && (pBldg->GetUnitType () == CStructureData::rocket))
			{
			int iDist = CHexCoord::Dist (pHpTrgt->GetHex (), pBldg->GetHex ());
			ASSERT (iDist >= 0);
			for (int iInd=0; iInd<3; iInd++)
				{
				if (apBldgAtk[iInd] == NULL)
					{
					apBldgAtk[iInd] = pBldg;
					aiDist[iInd] = iDist;
					}
				else
					{
					if (iDist < aiDist[iInd])
						{
						for (int iMov=2; iMov>iInd; iMov--)
							{
							apBldgAtk[iMov] = apBldgAtk[iMov-1];
							aiDist[iMov] = aiDist[iMov-1];
							}
						apBldgAtk[iInd] = pBldg;
						aiDist[iInd] = iDist;
						break;
						}
					}
				}
			}
		}

	// ok, give them two med_scouts, a light scout and attack orders
	static int aiType[5] = { CTransportData::med_scout, CTransportData::med_scout, CTransportData::light_scout, CTransportData::light_tank, CTransportData::light_tank };
	for (int iPlyr=1; iPlyr<3; iPlyr++)
		if (apBldgAtk[iPlyr] != NULL)
			{
			int iMax = __min (5, 1 + theGame.m_iAi + theGame.m_iAi / 3 );
			int iInd = 0;
			if ( iPlyr == 1 )
				{
				iInd = 2;
				iMax --;
				}
			for (; iInd<iMax; iInd++)
				{
				CMsgPlaceVeh msg (apBldgAtk[iPlyr], apBldgAtk[iPlyr]->GetExitHex (), apBldgAtk[iPlyr]->GetOwner()->GetPlyrNum (), aiType[iInd]);
				msg.ToNew ();
				msg.m_dwID = theGame.GetID ();
				theGame.PostToAll (&msg, sizeof (msg));

				CMsgScenarioAtk msg2 (pHpTrgt, apBldgAtk[iPlyr]->GetOwner()->GetPlyrNum(), msg.m_dwID, CMsgScenarioAtk::must);
				theGame.PostToClient (apBldgAtk[iPlyr]->GetOwner(), &msg2, sizeof (msg2));
				ASSERT (theBuildingMap.GetBldg (msg.m_dwID) != NULL);
				}
			}

	// damage the smelter
	pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((! pBldg->GetOwner()->IsAI ()) && (pBldg->GetData()->GetType () == CStructureData::smelter))
			{
			int iDamage = pBldg->GetDamagePoints () - pBldg->GetData()->GetDamagePoints () / 2;
			if (iDamage > 0)
				pBldg->DecDamagePoints (iDamage);
			theGame.m_adwScenarioUnits [0] = pBldg->GetID ();
			break;
			}
		}
}

// start scenario 3
static void Start3 ()
{

	// give them some outriders
	static int aiType2[4] = { CTransportData::light_scout, CTransportData::light_scout,
																		CTransportData::light_scout, CTransportData::light_scout };
	GiveAiUnits ( aiType2, __min (4, 1 + theGame.m_iAi ) );
}

static int fnEnumShow (CHex *pHex, CHexCoord, void *)
{

	pHex->IncVisible ();

	return (FALSE);
}

const int SHOW_DIST = 2;

void ShowBuilding (int iInd, CBuilding * pBldg)
{

	theGame.m_adwScenarioUnits [iInd] = pBldg->GetID ();
	pBldg->MakeBldgVisible ();

	CHexCoord _hex (pBldg->GetHex().X () - SHOW_DIST, pBldg->GetHex().Y () - SHOW_DIST);
	_hex.Wrap ();
	theMap.EnumHexes (_hex, pBldg->GetCX () + 2 * SHOW_DIST, pBldg->GetCY () + 2 * SHOW_DIST, fnEnumShow, NULL);
}

static int fnEnumUnshow (CHex *pHex, CHexCoord, void *)
{

	pHex->DecVisible ();

	return (FALSE);
}

void UnshowBuilding (CBuilding * pBldg)
{

//BUGBUG	pBldg->DecVisible ();

	CHexCoord _hex (pBldg->GetHex().X () - SHOW_DIST, pBldg->GetHex().Y () - SHOW_DIST);
	_hex.Wrap ();
	theMap.EnumHexes (_hex, pBldg->GetCX () + 2 * SHOW_DIST, pBldg->GetCY () + 2 * SHOW_DIST, fnEnumUnshow, NULL);
}

void UnshowAllBuildings ()
{

	for (int iInd=0; iInd<5; iInd++)
		{
		CBuilding * pBldg = theBuildingMap.GetBldg (theGame.m_adwScenarioUnits [iInd]);
		if (pBldg != NULL)
			UnshowBuilding (pBldg);
		}
}

// start scenario 4
static void Start4 ()
{

	// in case there aren't enough
	for (int iInd=0; iInd<5; iInd++)
		theGame.m_adwScenarioUnits [iInd] = 0;

	// the first 5 coal mines or oil wells belonging to another player
	iInd = 0;
	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner()->IsAI ()) && (! pBldg->IsVisible ()) && (! pBldg->IsConstructing () ) &&
												((pBldg->GetData()->GetType () == CStructureData::coal) || 
												(pBldg->GetData()->GetType () == CStructureData::oil_well)))
			{
			ShowBuilding (iInd, pBldg);

			iInd++;
			if (iInd >= 5)
				break;
			}
		}

	// lumber & farms if we didn't find 5 above
	if (iInd < 5)
		{
		pos = theBuildingMap.GetStartPosition ();
		while (pos != NULL)
			{
			DWORD dwID;
			CBuilding *pBldg;
			theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
			ASSERT_STRICT_VALID (pBldg);
			if ((pBldg->GetOwner()->IsAI ()) && (! pBldg->IsVisible ()) && (! pBldg->IsConstructing () ) &&
													((pBldg->GetData()->GetType () == CStructureData::lumber) || 
													(pBldg->GetData()->GetType () == CStructureData::farm)))
				{
				ShowBuilding (iInd, pBldg);
	
				iInd++;
				if (iInd >= 5)
					break;
				}
			}
		}

	// anything
	if (iInd < 5)
		{
		pos = theBuildingMap.GetStartPosition ();
		while (pos != NULL)
			{
			DWORD dwID;
			CBuilding *pBldg;
			theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
			ASSERT_STRICT_VALID (pBldg);
			if ((pBldg->GetOwner()->IsAI ()) && (! pBldg->IsVisible ()) && (! pBldg->IsConstructing ()) )
				{
				ShowBuilding (iInd, pBldg);
	
				iInd++;
				if (iInd >= 5)
					break;
				}
			}
		}
}

// start scenario 5
static void Start5 ()
{

	// in case there aren't enough
	for (int iInd=0; iInd<5; iInd++)
		theGame.m_adwScenarioUnits [iInd] = 0;

	// the first 3 farms
	iInd = 0;
	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner()->IsAI ()) && (! pBldg->IsVisible ()) && (! pBldg->IsConstructing () ) &&
												(pBldg->GetData()->GetType () == CStructureData::farm))
			{
			ShowBuilding (iInd, pBldg);

			iInd++;
			if (iInd >= 3)
				break;
			}
		}

	// lumber mills if not enough farms
	if (iInd < 3)
		{
		pos = theBuildingMap.GetStartPosition ();
		while (pos != NULL)
			{
			DWORD dwID;
			CBuilding *pBldg;
			theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
			ASSERT_STRICT_VALID (pBldg);
			if ((pBldg->GetOwner()->IsAI ()) && (! pBldg->IsVisible ()) && (! pBldg->IsConstructing () ) &&
												(pBldg->GetData()->GetType () == CStructureData::lumber))
				{
				ShowBuilding (iInd, pBldg);

				iInd++;
				if (iInd >= 3)
					break;
				}
			}
		}

	// anything
	if (iInd < 3)
		{
		pos = theBuildingMap.GetStartPosition ();
		while (pos != NULL)
			{
			DWORD dwID;
			CBuilding *pBldg;
			theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
			ASSERT_STRICT_VALID (pBldg);
			if ((pBldg->GetOwner()->IsAI ()) && (! pBldg->IsVisible ()) && (! pBldg->IsConstructing ()) )
				{
				ShowBuilding (iInd, pBldg);

				iInd++;
				if (iInd >= 3)
					break;
				}
			}
		}

	if ( theGame.m_iAi < 1 )
		return;

	// find the weakest AI player
	int iLastHave;
	CPlayer *pPlyrWeak = NULL;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_STRICT_VALID (pPlr);
		if (pPlr->IsAI ())
			{
			if (pPlyrWeak == NULL)
				{
				pPlyrWeak = pPlr;
				iLastHave = pPlr->GetBldgsHave () * 2 + pPlr->GetVehsHave ();
				}
			else
				if (pPlr->GetBldgsHave () * 2 + pPlr->GetVehsHave () < iLastHave)
					{
					pPlyrWeak = pPlr;
					iLastHave = pPlr->GetBldgsHave () * 2 + pPlr->GetVehsHave ();
					}
			}
		}

	if (pPlyrWeak == NULL)
		return;
	CBuilding * pHpTrgt = GetRocket ();
	if (pHpTrgt == NULL)
		return;

	// tell him to come & get us
	CMsgScenarioAtk msg2 (pHpTrgt, pPlyrWeak->GetPlyrNum(), 0, CMsgScenarioAtk::important);
	msg2.m_dwIDTarget = msg2.m_dwIDAtk = 0;
	theGame.PostToClient (pPlyrWeak, &msg2, sizeof (msg2));
}

// start scenario 6
static void Start6 ()
{

	// in case there is no rocket
	for (int iInd=0; iInd<5; iInd++)
		theGame.m_adwScenarioUnits [iInd] = (DWORD) -1;

	CBuilding * pBldgLoc = NULL;
	CBuilding * pHpRckt = GetRocket ();
	if (pHpRckt == NULL)
		return;

	// the furthest AI rocket
	int iDist = 0;
	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner()->IsAI ()) && (pBldg->GetData()->GetType () == CStructureData::rocket))
			{
			int _x = CHexCoord::Diff (pHpRckt->GetHex().X() - pBldg->GetHex().X());
			int _y = CHexCoord::Diff (pHpRckt->GetHex().Y() - pBldg->GetHex().Y());
			int iTmp = _x * _x + _y * _y;
			if ((pBldgLoc == NULL) || (iTmp > iDist))
				{
				pBldgLoc = pBldg;
				iDist = iTmp;
				}
			}
		}

	if (pBldgLoc == NULL)
		return;

	// can we get there?
	CHexCoord hexSrc ( pHpRckt->GetHex () );
	CHexCoord hexDest ( pBldgLoc->GetHex () );
	int iTrys = 10;
	while ( iTrys-- > 0 )
		{
		if ( thePathMap.GetPath ( hexSrc, hexDest, 0, 0, NULL, CTransportData::light_tank, TRUE) )
			break;
		hexDest.X () += RandNum (theGame.GetSideSize ()) - theGame.GetSideSize () / 2;
		hexDest.Y () += RandNum (theGame.GetSideSize ()) - theGame.GetSideSize () / 2;
		hexDest.Wrap ();
		}

	// center window on enemy rocket, select dropped units
	CWndArea * pWnd = theAreaList.GetTop ();
	if (pWnd != NULL)
		{
		pWnd->Center (hexDest);
		pWnd->SelectNone ();
		}

	// units for HP and each AI
	int aiVeh[5] = {CTransportData::light_tank, CTransportData::med_scout, CTransportData::light_scout, CTransportData::light_tank, CTransportData::med_scout };

	// AI gets them in their rocket
	GiveAiUnits ( aiVeh,  __min (5, 1 + theGame.m_iAi + theGame.m_iAi / 3 ) );

	// random walk dropping units
	iInd = 0;
	CHexCoord _hex (hexDest);
	int iPlyrAi = pBldgLoc->GetOwner()->GetPlyrNum();
	CTransportData const * pTd = theTransports.GetData (aiVeh[0]);

	// drop an AI unit in their middle, set us at war if not at easy level
	int iTotal = 5;
	if ( theGame.m_iAi > 0 )
		{
		CDlgRelations::NewRelations ( pBldgLoc->GetOwner(), RELATIONS_WAR );
		iTotal = 6;
		}

	CVehicle * pVehLast;
	while (iInd < iTotal)
		{
		_hex.X () += RandNum (4) - 2;
		_hex.Y () += RandNum (4) - 2;
		_hex.Wrap ();

		// can we get there?
		for (int iTry=0; iTry<10; iTry++)
			{	
			if ( thePathMap.GetPath ( hexDest, _hex, 0, 0, NULL, aiVeh[iInd], TRUE) )
				{
				CHex * pHex = theMap._GetHex (_hex);
				// hex clear and vehicle can travel on it
				if ((! (pHex->GetUnits () & CHex::unit)) &&
							(theTerrain.GetData (pHex->GetType ()).GetWheelMult (pTd->GetWheelType ()) > 0))
					break;
				}

			// try again
			_hex.X () += RandNum (4) - 2;
			_hex.Y () += RandNum (4) - 2;
			_hex.Wrap ();
			}

		CHex * pHex = theMap._GetHex (_hex);
		// hex clear and vehicle can travel on it
		if ((! (pHex->GetUnits () & CHex::unit)) &&
							(theTerrain.GetData (pHex->GetType ()).GetWheelMult (pTd->GetWheelType ()) > 0))
			{

			// if it's the AI's turn just drop it
			if ( iInd >= 5 )
				{
				CMsgPlaceVeh msg ( _hex, _hex, iPlyrAi, CTransportData::light_tank + theGame.m_iAi - 1 );
				msg.ToNew ();
				msg.m_dwID = theGame.GetID ();
				theGame.PostToAll (&msg, sizeof (msg));
				theGame.ProcessAllMessages ();

				// attack the closest one
				if ( pVehLast != NULL )
					{
					CVehicle * pVeh = theVehicleMap.GetVehicle (msg.m_dwID);
					if ( pVeh )
						pVeh->MsgSetTarget ( pVehLast );
					}
				break;
				}

			// drop a unit
			CMsgPlaceVeh msg (_hex, pHpRckt->GetHex (), pHpRckt->GetOwner()->GetPlyrNum (), aiVeh[iInd]);
			msg.ToNew ();
			msg.m_dwID = theGame.GetID ();
			theGame.PostToAll (&msg, sizeof (msg));
			theGame.ProcessAllMessages ();

			CVehicle * pVeh = theVehicleMap.GetVehicle (msg.m_dwID);
			if (pVeh)
				{
				// damage it (so it can be repaired even if never shot at)
				int iDamage = ( ( iInd + 1 ) * pVeh->GetData()->GetDamagePoints () ) / 10;
				iDamage += ( theGame.m_iAi * pVeh->GetData()->GetDamagePoints () ) / 10 ;
				pVeh->DecDamagePoints ( iDamage );

				// tell AI to attack it
				CMsgScenarioAtk msg2 (pVeh, iPlyrAi, msg.m_dwID, CMsgScenarioAtk::important);
				theGame.PostToClient (iPlyrAi, &msg2, sizeof (msg2));

				// select it
				pWnd->AddSelectUnit (pVeh);
				theGame.m_adwScenarioUnits [iInd] = msg.m_dwID;
				pVehLast = pVeh;
				}

			iInd++;
			pTd = theTransports.GetData (aiVeh[iInd]);
			}
		}
}

// start scenario 7
static void Start7 ()
{

	CBuilding * pHpRckt = GetRocket ();
	if (pHpRckt == NULL)
		return;

	// the furthest point away from rocket
	CHexCoord hexDest (pHpRckt->GetHex().X() + theMap.Get_eX () / 2, pHpRckt->GetHex().Y() + theMap.Get_eY () / 2);
	hexDest.Wrap ();

	int iSize = theMap.GetSideSize () / 2;

	// not near another building
	for (int iTry=0; iTry<20; iTry++)
		{
		BOOL bOk = TRUE;
		POSITION pos = theBuildingMap.GetStartPosition ();
		while (pos != NULL)
			{
			DWORD dwID;
			CBuilding *pBldg;
			theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
			ASSERT_STRICT_VALID (pBldg);
			if ( ! pBldg->GetOwner()->IsMe () )
				{
				int _x = CHexCoord::Diff (hexDest.X() - pBldg->GetHex().X());
				int _y = CHexCoord::Diff (hexDest.Y() - pBldg->GetHex().Y());
				if ( abs (_x) + abs (_y) < iSize )
					{
					bOk = FALSE;
					break;
					}
				}
			}

		if (bOk)
			break;

		hexDest.X () += RandNum (iSize) - iSize / 2;
		hexDest.Y () += RandNum (iSize) - iSize / 2;
		hexDest.Wrap ();
		}

	// get us onto plains/accessable
	for (iTry=0; iTry<50; iTry++)
		{
		int iTyp = theMap._GetHex (hexDest)->GetType ();
		if ( (iTyp == CHex::plain) || (iTyp == CHex::road) )
			if ( thePathMap.GetPath ( hexDest, pHpRckt->GetHex (), 0, 0, NULL, CTransportData::heavy_truck, TRUE) )
				break;

		if (iTyp == CHex::ocean)
			iSize = theMap.GetSideSize ();
		else
			iSize = theMap.GetSideSize () / 4;

		if (MyRand () & 0x100)
			hexDest.X () += RandNum (iSize) - iSize / 2;
		else
			hexDest.Y () += RandNum (iSize) - iSize / 2;
		hexDest.Wrap ();
		}

	// center window on the drop location
	CWndArea * pWnd = theAreaList.GetTop ();
	if (pWnd != NULL)
		{
		pWnd->Center (hexDest);
		pWnd->SelectNone ();
		}

	// vehicles for both
	int aiVeh[5] = {CTransportData::heavy_truck, CTransportData::light_tank, CTransportData::med_scout, CTransportData::light_tank, CTransportData::med_scout};

	// AI gets them in their rocket
	GiveAiUnits ( aiVeh,  __min (5, 1 + theGame.m_iAi + theGame.m_iAi / 3 ) );

	// AI get's copper in it's rocket
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_STRICT_VALID (pPlr);
		if (pPlr->IsAI ())
			{
			CBuilding * pRocket = theBuildingMap.GetBldg ( pPlr->m_dwIDRocket );
			if ( pRocket != NULL )
				pRocket->AddToStore ( CMaterialTypes::copper, 200 + 50 * theGame.m_iAi );
			}
		}

	// random walk dropping units
	int iInd = 0;
	CTransportData const * pTd = theTransports.GetData (aiVeh[0]);
	CHexCoord _hex ( hexDest );

	while (iInd < 5)
		{
		if (MyRand () & 0x100)
			_hex.X () += RandNum (4) - 2;
		else
			_hex.Y () += RandNum (4) - 2;
		_hex.Wrap ();

		// can we get there?
		for (iTry=0; iTry<10; iTry++)
			{	
			if ( thePathMap.GetPath ( hexDest, _hex, 0, 0, NULL, aiVeh[iInd], TRUE) )
				{
				CHex * pHex = theMap._GetHex (_hex);
				// hex clear and vehicle can travel on it
				if ((! (pHex->GetUnits () & CHex::unit)) &&
							(theTerrain.GetData (pHex->GetType ()).GetWheelMult (pTd->GetWheelType ()) > 0))
					break;
				}

			// try again
			_hex.X () += RandNum (4) - 2;
			_hex.Y () += RandNum (4) - 2;
			_hex.Wrap ();
			}

		// drop a unit
		CMsgPlaceVeh msg (_hex, pHpRckt->GetHex (), pHpRckt->GetOwner()->GetPlyrNum (), aiVeh[iInd]);
		msg.ToNew ();
		msg.m_dwID = theGame.GetID ();
		theGame.PostToAll (&msg, sizeof (msg));
		theGame.ProcessAllMessages ();

		CVehicle * pVeh = theVehicleMap.GetVehicle (msg.m_dwID);
		if (pVeh)
			{
			// tell all the AIs to attack it
			POSITION pos;
			for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
				{
				CPlayer *pPlr = theGame.GetAll().GetNext (pos);
				ASSERT_STRICT_VALID (pPlr);
				if (pPlr->IsAI ())
					{

					// difficult + put outriders from the enemy there
					if ( (iInd == 4) && (theGame.m_iAi > 1) )
						{
						TRAP ();
						// set us at war
						CDlgRelations::NewRelations ( pPlr, RELATIONS_WAR );

						// find a clear hex nearby
						CHexCoord _hexAI ( _hex.X() + RandNum (4) - 2, _hex.Y() + RandNum (4) - 2 );
						for (int iTry2=0; iTry2<10; iTry2++)
							{
							CHex * pHex = theMap._GetHex ( _hexAI );
							// hex clear and vehicle can travel on it
							if ((! (pHex->GetUnits () & CHex::unit)) &&
									(theTerrain.GetData (pHex->GetType ()).GetWheelMult (pTd->GetWheelType ()) > 0))
								break;
							TRAP ();
							_hexAI.X () += RandNum (4) - 2;
							_hexAI.Y () += RandNum (4) - 2;
							_hexAI.Wrap ();
							}

						CMsgPlaceVeh msg ( _hexAI, _hex, pPlr->GetPlyrNum (), CTransportData::light_scout );
						msg.ToNew ();
						msg.m_dwID = theGame.GetID ();
						theGame.PostToAll (&msg, sizeof (msg));
						theGame.ProcessAllMessages ();
						}

					CMsgScenarioAtk msg2 (pVeh, pPlr->GetPlyrNum (), msg.m_dwID, 
									iInd == 4 ? CMsgScenarioAtk::important : CMsgScenarioAtk::suggestion);
					theGame.PostToClient (pPlr, &msg2, sizeof (msg2));
					}
				}

			// select it
			pWnd->AddSelectUnit (pVeh);
			theGame.m_adwScenarioUnits [iInd] = msg.m_dwID;

			// fill the truck with copper
			if ( pVeh->GetData()->GetType () == CTransportData::heavy_truck )
				{
				pVeh->AddToStore ( CMaterialTypes::copper, 200 );
				theGame.m_pHpRtr->MsgTakeVeh (pVeh);
				pVeh->HpControlOn ();
				}
			}

		iInd++;
		pTd = theTransports.GetData (aiVeh[iInd]);
		}
}

// start scenario 10
static void Start10 ()
{

	// all refineries & power plants
	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ( (pBldg->GetOwner()->IsAI ()) &&
											( (pBldg->GetData()->GetType () == CStructureData::refinery) || 
												(pBldg->GetData()->GetType () == CStructureData::power_1) || 
												(pBldg->GetData()->GetType () == CStructureData::power_2) || 
												(pBldg->GetData()->GetType () == CStructureData::power_3) ) )
			ShowBuilding (0, pBldg);
		}
}

// research needed
static int a0[] = { 2, CStructureData::farm, CStructureData::lumber };
static int a1[] = { 3, CStructureData::iron, CStructureData::coal, CStructureData::smelter };
static int a2[] = { 3, CStructureData::power_1, CStructureData::oil_well, CStructureData::refinery };
static int a3[] = { 1, CStructureData::research };
static int a4[] = { 1, CStructureData::command_center };
static int a6[] = { 1, CStructureData::repair };
static int a9[] = { 1, CStructureData::heavy };

static int * apiBldgsNeeded [NUM_SCENARIOS] = {
			a0, a1, 0, a3, a4, 0, a6, 0, 0, a9, 0, 0 };

const int aiRsrchNeeded [NUM_SCENARIOS] = {
			0, 0, 0, 9, 0, 0, 0, 43, 0, 0, 0, 0 };

void CConquerApp::ScenarioStart ()
{

	// tell the AI it's starting
	POSITION pos;
	for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL; )
		{
		CPlayer *pPlr = theGame.GetAll().GetNext (pos);
		ASSERT_STRICT_VALID (pPlr);
		if (pPlr->IsAI ())
			{
			CMsgScenario msg;
			msg.m_iPlyrAi = pPlr->GetPlyrNum ();
			theGame.PostToClient (pPlr, &msg, sizeof (msg));
			}
		}

	// research freebies (all precursors
	if (aiRsrchNeeded [theGame.GetScenario()] != 0)
		if (! theGame.GetMe()->GetRsrch (aiRsrchNeeded [theGame.GetScenario()]).m_bDiscovered)
			{
			CRsrchItem *pRi = & theRsrch [ aiRsrchNeeded [theGame.GetScenario()] ];
			int * pRr = pRi->m_piRsrchRequired;
			int iNum = pRi->m_iNumRsrchRequired;
			while ( iNum -- )
				{
				CRsrchStatus * pRs = & theGame.GetMe()->GetRsrch ( *pRr );
				if (! pRs->m_bDiscovered)
					pRs->m_bDiscovered = TRUE;
				pRr ++;
				}
			}

	// new research may be allowed
	ResearchDiscovered (0);

	theGame.m_iScenarioVar = 0;
	for (int iInd=0; iInd<5; iInd++)
		theGame.m_adwScenarioUnits [iInd] = 0;

	switch (theGame.GetScenario ())
	  {
		case 1 :
			Start1 ();
			break;
		case 2 :
			Start2 ();
			break;
		case 3 :
			Start3 ();
			break;
		case 4 :
			Start4 ();
			break;
		case 5 :
			Start5 ();
			break;
		case 6 :
			Start6 ();
			break;
		case 7 :
			Start7 ();
			break;

		case 9 :
			// to track when a player is destroyed
			theGame.m_iScenarioVar = theGame.GetAll().GetCount ();
			break;

		case 10 :
			Start10 ();
			break;
	  }
}


// return TRUE if the scenario is over
BOOL CConquerApp::ScenarioEnd ()
{

#ifdef _CHEAT
	if (theGame.GetScenario () >= _iScenarioOn)
#endif
		// check for game lost first
		switch (theGame.GetScenario ())
	  	{
			case 2 : {
				// need power plant repaired
				CBuilding * pBldg = theBuildingMap.GetBldg (theGame.m_adwScenarioUnits [0]);
				if (pBldg == NULL)
					return (-1);
				break; }

			case 6 :
				// if all 5 vehicles are dead we've lost
				if (theGame.m_adwScenarioUnits [0] == 0)
					return (-1);
				break;

			case 7 : {
				// if vehicle is dead you've lost
				CVehicle * pVeh = theVehicleMap.GetVehicle (theGame.m_adwScenarioUnits [0]);
				if (pVeh == NULL)
					return (-1);
				// if vehicle is not unloaded you haven't won yet
				if (pVeh->GetStore (CMaterialTypes::copper) >= 200)
					return (0);
				break; }
			}

	// have you built all necessary buildings
	int *piBldg = apiBldgsNeeded [theGame.GetScenario ()];
	if (piBldg != NULL)
		{
		int iNum = *piBldg++;
		while (iNum--)
			if (! theGame.GetMe()->GetExists (*piBldg++))
				return (0);
		}

	// have you completed R&D
	if (aiRsrchNeeded [theGame.GetScenario()] != 0)
		if (! theGame.GetMe()->GetRsrch (aiRsrchNeeded [theGame.GetScenario()]).m_bDiscovered)
			return (0);

	// special case stuff
	switch (theGame.GetScenario ())
	  {
		case 0 :
			// need 1000 lumber & steel
			if (theGame.GetMe()->GetMaterialMade (CMaterialTypes::lumber) < 1000)
				return (0);
			return (1); 

		case 1 :
			// need 1000 steel
			if (theGame.GetMe()->GetMaterialMade (CMaterialTypes::steel) < 1000)
				return (0);
			return (1); 
				
		case 2 : {
			// need 50 gas
			if ( theGame.GetMe()->GetMaterialMade (CMaterialTypes::gas) < 50 )
				return (0);
			if ( theGame.GetMe()->GetPwrNeed () > theGame.GetMe()->GetPwrHave () )
				return (0);
			// need smelter repaired
			CBuilding * pBldg = theBuildingMap.GetBldg (theGame.m_adwScenarioUnits [0]);
			if ( (pBldg == NULL) || (pBldg->GetDamagePer () < 99) )
				return (0);
			return (1); }

		case 4 : {
			// need 3 scouts
			if (theGame.m_iScenarioVar < 0x07)
				return (FALSE);

			// need to have spotted 3 buildings
			int iFound = 0;				
			for (int iInd=0; iInd<5; iInd++)
				if (theGame.m_adwScenarioUnits [iInd] == 0)
					iFound++;
				else
					// if destroyed then we've found it
					if (theBuildingMap.GetBldg (theGame.m_adwScenarioUnits [iInd]) == NULL)
						theGame.m_adwScenarioUnits [iInd] = 0;
			if (iFound < 3)
				return (FALSE);

			// take away the visibility of the remaining ones
			UnshowAllBuildings ();
			return (TRUE); }

		case 5 : {
			// need a tank
			if (theGame.m_iScenarioVar < 1)
				return (FALSE);

			// down to 2 or less buildings
			if (theGame.m_adwScenarioUnits [0] != 0)
				return (FALSE);
			UnshowAllBuildings ();
			return (TRUE); }

		case 6 :
			// vehicle repaired?
			if (theGame.m_iScenarioVar < 1)
				return (FALSE);
			return (TRUE);

		case 7 :
			// xil delivered?
			if (theGame.m_iScenarioVar < 1)
				return (FALSE);
			return (TRUE);
		case 8 :
			// killed a rocket
			return (theGame.m_iScenarioVar > 0);
		case 9 :
			// killed a player
			return (theGame.m_iScenarioVar > theGame.GetAll().GetCount ());

		case 10 : {
			// buildings destroyed?
			if (theGame.m_iScenarioVar < 1)
				return (FALSE);

			// unshow all refineries & power plants
			POSITION pos = theBuildingMap.GetStartPosition ();
			while (pos != NULL)
				{
				DWORD dwID;
				CBuilding *pBldg;
				theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
				ASSERT_STRICT_VALID (pBldg);
				if ( (pBldg->GetOwner()->IsAI ()) &&
											( (pBldg->GetData()->GetType () == CStructureData::refinery) || 
												(pBldg->GetData()->GetType () == CStructureData::power_1) || 
												(pBldg->GetData()->GetType () == CStructureData::power_2) || 
												(pBldg->GetData()->GetType () == CStructureData::power_3) ) )
					UnshowBuilding ( pBldg );
				}
			return (TRUE); }

		case 11 :
			if (theGame.GetMe()->GetBldgsHave () <= 0)
				return (-1);
			if (theGame.GetAll().GetCount () <= 1)
				return (TRUE);
			return (FALSE);
		}

	return (1);
}

/////////////////////////////////////////////////////////////////////////////
// CCreateScenario

void CCreateScenario::Init ()
{

	ASSERT_VALID (this);
	theApp.Log ( "Create scenario game" );

	// set it as the server
	theGame.ctor ();
	theGame.SetServer (TRUE);
	theGame._SetIsNetGame ( FALSE );
	theGame.Open (TRUE);

	m_dlgScenario.Create (this, CDlgScenario::IDD, theApp.m_pMainWnd);
}

void CCreateScenario::Close ()
{

	if (m_dlgPickRace.m_hWnd != NULL)
		m_dlgPickRace.DestroyWindow ();

	if (m_dlgScenario.m_hWnd != NULL)
		m_dlgScenario.DestroyWindow ();
}

#ifdef _DEBUG
void CCreateScenario::AssertValid() const
{

	CCreateBase::AssertValid ();

	ASSERT_VALID (&m_dlgScenario);
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgScenario dialog

CDlgScenario::CDlgScenario(CWnd* pParent /*=NULL*/)
	: CDialog (CDlgScenario::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgScenario)
	m_iAi = 0;
	m_iSize = 0;
	//}}AFX_DATA_INIT
}


void CDlgScenario::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgScenario)
	DDX_Control(pDX, IDOK, m_btnOk);
	DDX_Radio(pDX, IDC_CREATE_EASY, m_iAi);
	DDX_Radio(pDX, IDC_CREATE_SMALL, m_iSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgScenario, CDialog)
	//{{AFX_MSG_MAP(CDlgScenario)
	ON_BN_CLICKED(IDX_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgScenario message handlers

void CDlgScenario::Create (CCreateScenario *pCs, UINT id, CWnd *pPar)
{

	ASSERT_VALID (this);

	m_pCs = pCs;
	CDialog::Create (id, pPar);
}

void CDlgScenario::OnCancel() 
{
	
	ASSERT_VALID (this);

	theGame.Close ();
	CDialog::OnCancel();
	theApp.CreateMain ();
}

void CDlgScenario::OnOK() 
{
	
	ASSERT_VALID (this);
	theApp.Log ( "Set game type" );

	// save the parameters we use
	UpdateData (TRUE);
	theGame.m_iAi = m_pCs->m_iAi = m_iAi;
	theGame.m_iSize = m_pCs->m_iSize = m_iSize;
	theGame.m_iPos = m_pCs->m_iPos = SCENARIO_POS;
	m_pCs->m_iNet = -1;

	// number of opponents
	m_pCs->m_iNumAi = NUM_AI_IN_SCENARIO + m_iAi;
	int iMax = __max ( 0, theApp.GetCpuSpeed () - 100 );
	iMax = 3 + iMax / 20;
	m_pCs->m_iNumAi = __min ( iMax, m_pCs->m_iNumAi );
	m_pCs->m_iNumAi = __max ( NUM_AI_IN_SCENARIO, m_pCs->m_iNumAi );

	theGame.SetScenario (0);

	theApp.WriteProfileInt ("Create", "Difficultity", m_iAi);
	theApp.WriteProfileInt ("Create", "Size", m_iSize);

	// switch to the pick race dialog
	if (m_pCs->m_dlgPickRace.m_hWnd == NULL)
		m_pCs->m_dlgPickRace.Create (m_pCs, this, CDlgPickRace::IDD, theApp.m_pMainWnd);

	m_pCs->m_dlgPickRace.ShowWindow (SW_SHOW);
	ShowWindow (SW_HIDE);
}

BOOL CDlgScenario::OnInitDialog() 
{
	
	CDialog::OnInitDialog();
	
	CenterWindow (theApp.m_pMainWnd);

	// init the radio buttons
	UpdateData (TRUE);

	m_iAi = theApp.GetProfileInt ("Create", "Difficultity", 0);
	m_iSize = theApp.GetProfileInt ("Create", "Size", 0);

#ifndef	HACK_TEST_AI
	// shareware restrictions
	if ( ( theApp.IsShareware () ) || ( theApp.IsSecondDisk () ) )
		{
		m_iAi = 1;
		m_iSize = 1;

		GetDlgItem (IDC_CREATE_EASY)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_DIFFICULT)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_IMPOSSIBLE)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_SMALL)->EnableWindow (FALSE);
		GetDlgItem (IDC_CREATE_LARGE)->EnableWindow (FALSE);
		}
#endif

	UpdateData (FALSE);

	// get rid of the main menu
	theApp.DestroyMain ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgScenario::OnHelp() 
{
	
	theApp.WinHelp (HLP_SCENARIO, HELP_CONTEXT);
}

