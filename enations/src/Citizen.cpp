//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "player.h"
#include "minerals.h"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

// iDir here is:
//	0 ; x++
//	1 : y++
//  2	: x--
//  3 : y--

// iTurn is:
//  -1 : left
//  0  : straight
//  1  : right


CCitCon::CCitCon (int iBldg, CHexCoord const & hex, int iDir) 
							: m_iBldg (iBldg), m_hex (hex), m_iDir (iDir)
{

	m_pData = theStructures.GetData (iBldg);

	m_iTryLeft = 64;
	m_iStraight = m_iLeft = m_iRight = m_iNumRoads = 0;
	m_bTurn = FALSE;
	m_iBldgDir = RandNum (3);
}


// build apartments & offices
void CPlayer::CitizenConstruction ()
{
const int PPL_MUL = 100;

	ASSERT_VALID (this);
	ASSERT (IsLocal ());

	// 1 building ever 15 sec max
	if (m_iTillNext > 0)
		{
		m_iTillNext--;
		return;
		}
	m_iTillNext = 15;

	// do we need both?
	BOOL bAptOk = TRUE;
	if ((m_iAptCap < m_iPplHave+m_iPplVeh) && (m_iOfcCap < m_iPplHave))
		bAptOk = RandNum (1);

	// do we need an apartment?
	if ((bAptOk) && (m_iAptCap < m_iPplHave+m_iPplVeh))
		{
		int iBldg;
		if (m_iPplHave+m_iPplVeh < 5 * PPL_MUL)
			iBldg = CStructureData::apartment_base + RandNum (1);
		else
			if (m_iPplHave+m_iPplVeh < 15 * PPL_MUL)
				iBldg = CStructureData::apartment_base + RandNum (3);
			else
				iBldg = CStructureData::apartment_base + RandNum (CStructureData::num_apartments-1);

		if (IsAI ())
			{
			m_iAptCap += theStructures.GetData (iBldg)->GetPopHoused ();
			CMsgBuildCiv msg (this, iBldg);
			theGame.PostToClient (GetPlyrNum (), &msg, sizeof (msg));
			return;
			}

		if (BuildCcBldg (iBldg))
			m_iAptCap += theStructures.GetData (iBldg)->GetPopHoused ();
		else
			m_iTillNext = RandNum (8);
		return;
		}

	// do we need an office?
	if (m_iOfcCap < m_iPplHave)
		{
		int iBldg;
		if (m_iPplHave+m_iPplVeh < 12 * PPL_MUL)
			iBldg = CStructureData::office_base + RandNum (3);
		else
			iBldg = CStructureData::office_base + RandNum (CStructureData::num_offices-1);

		if (IsAI ())
			{
			m_iOfcCap += theStructures.GetData (iBldg)->GetPopHoused ();
			CMsgBuildCiv msg (this, iBldg);
			theGame.PostToClient (GetPlyrNum (), &msg, sizeof (msg));
			return;
			}

		if (BuildCcBldg (iBldg))
			m_iOfcCap += theStructures.GetData (iBldg)->GetPopHoused ();
		else
			m_iTillNext = RandNum (8);
		}
}

// create our initial intersection
BOOL CPlayer::InitialRoad ()
{

	CHex * pHex = theMap._GetHex (m_hexCity);

	// if we're blocked - find a new spot
	if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::bldg))
		{
		m_hexCity.Xdec ();
		m_hexCity.Ydec ();
		int eX = 3;
		int iNum = 2;
		int iDir = 0;

		while (eX <= 9)
			{
			CHex * pHex = theMap._GetHex (m_hexCity);
			if ((! pHex->IsWater ()) && (! (pHex->GetUnits () & CHex::bldg)))
				break;

			switch (iDir)
			  {
				case 0 :
					m_hexCity.Xinc ();
					if (--iNum <= 0)
						{
						iNum = eX - 1;
						iDir = 1;
						}
					break;
				case 1 :
					m_hexCity.Yinc ();
					if (--iNum <= 0)
						{
						iNum = eX - 1;
						iDir = 2;
						}
					break;
				case 2 :
					m_hexCity.Xdec ();
					if (--iNum <= 0)
						{
						iNum = eX;
						iDir = 3;
						}
					break;
				case 3 :
					m_hexCity.Ydec ();
					if (--iNum <= 0)
						{
						eX += 2;
						iNum = eX - 2;
						iDir = 0;
						}
					break;
			  }
			}

		// did we succeed? if not give up
		CHex * pHex = theMap._GetHex (m_hexCity);
		if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::bldg))
			{
			TRAP ();
			return (FALSE);
			}
		} // end find a new city center

	// ok, lets drop a crossroads
	pHex = theMap._GetHex (m_hexCity);
	pHex->ChangeToRoad (m_hexCity);

	CHexCoord _hex (m_hexCity);
	int iNum = 1 + RandNum (4);
	for (int iOn=0; iOn<iNum; iOn++)
		{
		_hex.Xinc ();
		CHex * pHex = theMap._GetHex (_hex);
		if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::bldg))
			break;
		pHex->ChangeToRoad (_hex);
		}
	_hex = m_hexCity;
	iNum = 1 + RandNum (4);
	for (iOn=0; iOn<iNum; iOn++)
		{
		_hex.Xdec ();
		CHex * pHex = theMap._GetHex (_hex);
		if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::bldg))
			break;
		pHex->ChangeToRoad (_hex);
		}
	_hex = m_hexCity;
	iNum = 1 + RandNum (4);
	for (iOn=0; iOn<iNum; iOn++)
		{
		_hex.Yinc ();
		CHex * pHex = theMap._GetHex (_hex);
		if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::bldg))
			break;
		pHex->ChangeToRoad (_hex);
		}
	_hex = m_hexCity;
	iNum = 1 + RandNum (4);
	for (iOn=0; iOn<iNum; iOn++)
		{
		_hex.Ydec ();
		CHex * pHex = theMap._GetHex (_hex);
		if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::bldg))
			break;
		pHex->ChangeToRoad (_hex);
		}

	return (TRUE);
}

// set a [3] array with 0-2 in random order
static void RandDir (int * piDir)
{
const int MAX_NUM = 2;

	// set them
	for (int iOn=0; iOn<=MAX_NUM; iOn++)
		*(piDir+iOn) = RandNum (MAX_NUM - iOn);

	// eliminate dups
	for (int iTries=0; iTries<=MAX_NUM; iTries++)
		for (iOn=0; iOn<=MAX_NUM; iOn++)
			for (int iTst=iOn+1; iTst<=MAX_NUM; iTst++)
				if (*(piDir+iTst) == *(piDir+iOn))
					(*(piDir + iTst)) ++;
}

// used to get the next hex to the left, right, or straight ahead
static CHexCoord HexNext (CCitCon * pCc, int iTurn)
{

	CHexCoord hex (pCc->m_hex);
	int iDir = (pCc->m_iDir + iTurn) & 0x03;

	if (iDir & 1)
		hex.Y (hex.Y () - iDir + 2);
	else
		hex.X (hex.X () - iDir + 1);

	return (hex);
}

static int fnEnumRoad (CHex *pHex, CHexCoord, void *pData)
{

	// we're done if it's a road (test for units too for speed)
	if ((pHex->GetType () == CHex::road) || (pHex->GetUnits () & CHex::bldg))
		{
		*((BOOL *) pData) = FALSE;
		return (TRUE);
		}

	return (FALSE);
}

#ifdef BUGBUG
static int fnEnumMin (CHex *pHex, CHexCoord, void *pData)
{

	// won't build on minerals
	if (pHex->GetUnits () & CHex::minerals)
		{
		*((BOOL *) pData) = FALSE;
		return (TRUE);
		}

	return (FALSE);
}
#endif

// can we build at this location?
BOOL CPlayer::CanBuild (CHexCoord const & hexTry, CCitCon * pCc)
{
const int MIN_DIST = 5;
int MIN_ENEMY_DIST = theGame.GetSideSize () / 4;

	// test for no building on roads (FoundCost allows it)
	BOOL bOk = TRUE;
	theMap.EnumHexes (hexTry, pCc->m_pData->GetCX(), pCc->m_pData->GetCY(), fnEnumRoad, &bOk);
	if (! bOk)
		return (FALSE);

#ifdef BUGBUG
	// not next to minerals
	CHexCoord _hexTmp (hexTry.X () - 2, hexTry.Y () - 2);
	_hexTmp.Wrap ();
	theMap.EnumHexes (_hexTmp, pCc->m_pData->GetCX()+4, pCc->m_pData->GetCY()+4, fnEnumMin, &bOk);
	if (! bOk)
		return (FALSE);
#endif

	int iCost = theMap.FoundationCost (hexTry, pCc->m_iBldg, 0, (CVehicle *) -1);
	if (iCost < 0)
		return (FALSE);

	// no MY building that needs vehicles within 5 hexes
	// no enemy building within iSideSize / 4 hexes
	POSITION pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner () == this) && (pBldg->GetData()->HasVehExit ()))
			{
			int xDif = CHexCoord::Diff (pBldg->GetExitHex().X() - hexTry.X());
			if (((xDif >= 0) && (xDif < MIN_DIST)) || ((xDif < 0) && (-xDif < MIN_DIST)))
				{
				int yDif = CHexCoord::Diff (pBldg->GetExitHex().Y() - hexTry.Y());
				if (((yDif >= 0) && (yDif < MIN_DIST)) || ((yDif < 0) && (-yDif < MIN_DIST)))
					return (FALSE);
				}
			}

		if (pBldg->GetOwner () != this)
			{
			int xDif = CHexCoord::Diff (pBldg->GetHex().X() - hexTry.X());
			if (((xDif >= 0) && (xDif < MIN_ENEMY_DIST+pCc->m_pData->GetCX ())) ||
						((xDif < 0) && (-xDif < MIN_ENEMY_DIST+pBldg->GetData()->GetCX ())))
				{
				int yDif = CHexCoord::Diff (pBldg->GetHex().Y() - hexTry.Y());
				if (((yDif >= 0) && (yDif < MIN_ENEMY_DIST+pCc->m_pData->GetCY ())) ||
								((yDif < 0) && (-yDif < MIN_ENEMY_DIST+pBldg->GetData()->GetCY ())))
					return (FALSE);
				}
			}
		}

	return (TRUE);
}

// build the building
BOOL CPlayer::BuildIt (CHexCoord const & hexTry, CCitCon * pCc)
{

	int iDir;
	if (pCc->m_pData->GetCX () == pCc->m_pData->GetCY ())
		iDir = RandNum (3);
	else
		iDir = 0 + 2 * RandNum (1);

	CMsgBuildBldg msg (NULL, hexTry, iDir, pCc->m_iBldg);
	msg.m_iPlyrNum = GetPlyrNum ();

	theGame.PostToServer (&msg, sizeof (msg));

	return (TRUE);
}

static CHexCoord HexBldgLeft (CCitCon * pCc)
{

	CHexCoord hex (pCc->m_hex);

	switch (pCc->m_iDir)
	  {
		case 0 :
			hex.Y () -= pCc->m_pData->GetCY ();
			break;
		case 1 :
			hex.X () += 1;
			break;
		case 2 :
			hex.X () -= (pCc->m_pData->GetCX () - 1);
			hex.Y () += 1;
			break;
		case 3 :
			hex.X () -= pCc->m_pData->GetCX ();
			hex.Y () -= (pCc->m_pData->GetCY () - 1);
			break;
	  }

	hex.Wrap ();
	return (hex);
}

static CHexCoord HexBldgRight (CCitCon * pCc)
{

	CHexCoord hex (pCc->m_hex);

	switch (pCc->m_iDir)
	  {
		case 0 :
			hex.Y () += 1;
			break;
		case 1 :
			hex.X () -= pCc->m_pData->GetCX ();
			break;
		case 2 :
			hex.X () -= (pCc->m_pData->GetCX () - 1);
			hex.Y () -= pCc->m_pData->GetCY ();
			break;
		case 3 :
			hex.X () += 1;
			hex.Y () -= (pCc->m_pData->GetCY () - 1);
			break;
	  }

	hex.Wrap ();
	return (hex);
}

void StepHex (CCitCon * pCc, int iTurn, CHexCoord const & hex)
{

	if (iTurn == 0)
		{
		pCc->m_iStraight ++;
		pCc->m_iLeft ++;
		pCc->m_iRight ++;
		}
	else
		{
		pCc->m_iStraight = 0;
		if (iTurn < 0)
			pCc->m_iLeft = 0;
		else
			pCc->m_iRight = 0;
		}
	pCc->m_iDir = (pCc->m_iDir + iTurn) & 0x03;
	pCc->m_hex = hex;
}

// walk the road and build if we can
BOOL CPlayer::TryRoad (CCitCon * pCc)
{
CHexCoord hexTry;
const int MAX_ROADS = 4;

	// are we out of attempts?
	pCc->m_iTryLeft--;
	if (pCc->m_iTryLeft < 0)
		return (FALSE);

	// can I build to my left?
	CHexCoord hexNext = HexNext (pCc, -1);
	CHex * pHex = theMap._GetHex (hexNext);
	if (pHex->GetType () == CHex::road)
		pCc->m_iLeft = 0;
	else
		if ((! pHex->IsWater ()) && (! (pHex->GetUnits () & CHex::unit)))
			{
			hexTry = HexBldgLeft (pCc);
			if (CanBuild (hexTry, pCc))
				{
				if (BuildIt (hexTry, pCc))
					return (TRUE);
				}
			else
				// ok, the ground is too steep - how about a cross street?
				if ((MyRand () & 0x3F00) == 0x3F00)
					pHex->ChangeToRoad (hexNext);
			}

	// can I build to my right?
	hexNext = HexNext (pCc, 1);
	pHex = theMap._GetHex (hexNext);
	if (pHex->GetType () == CHex::road)
		pCc->m_iRight = 0;
	else
		if ((! pHex->IsWater ()) && (! (pHex->GetUnits () & CHex::unit)))
			{
			hexTry = HexBldgRight (pCc);
			if (CanBuild (hexTry, pCc))
				{
				if (BuildIt (hexTry, pCc))
					return (TRUE);
				}
			else
				// ok, the ground is too steep - how about a cross street?
				if ((MyRand () & 0x3F00) == 0x3F00)
					pHex->ChangeToRoad (hexNext);
			}

	// if road ahead - or either side - take one
	int aiTurn [3];
	RandDir (aiTurn);
	for (int iOn=0; iOn<3; iOn++)
		{
		int iTurn = aiTurn[iOn] - 1;
		hexTry = HexNext (pCc, iTurn);
		CHex * pHex = theMap._GetHex (hexTry);
		if (pHex->GetType () == CHex::road)
			{
			StepHex (pCc, iTurn, hexTry);
			return (TryRoad (pCc));
			}
		else
			// if no road ahead we need to drop below so can get it
			if (iTurn == 0)
				if ((! pHex->IsWater ()) && (! (pHex->GetUnits () & CHex::unit)))
					break;
		}

	// if we are blocked in front then we have to turn
	hexTry = HexNext (pCc, 0);
	pHex = theMap._GetHex (hexTry);
	if ((pHex->IsWater ()) || (pHex->GetUnits () & CHex::unit))
		{
		pCc->m_bTurn = FALSE;
		pCc->m_iStraight = pCc->m_iLeft = pCc->m_iRight = 20;
		}

	// ok, try our turn odds first
	if ((! pCc->m_bTurn) && (pCc->m_iStraight > 2) && (pCc->m_iStraight > RandNum (7)))
		{
		// we do NOT turn if we are 1 away from connecting to a road ahead
		hexTry = HexNext (pCc, 0);
		pHex = theMap._GetHex (hexTry);
		if (pHex->GetType () != CHex::road)
			{
			CHexCoord hexSave (pCc->m_hex);
			pCc->m_hex = hexTry;
			hexTry = HexNext (pCc, 0);
			pCc->m_hex = hexSave;
			pHex = theMap._GetHex (hexTry);
			if (pHex->GetType () != CHex::road)
				{

				// try left
				if ((pCc->m_iLeft > 2) && (pCc->m_iLeft > RandNum (6)))
					{
					hexTry = HexNext (pCc, -1);
					pHex = theMap._GetHex (hexTry);
					if ((! pHex->IsWater ()) && (pHex->GetType () != CHex::road) && 
														(! (pHex->GetUnits () & CHex::unit)))
						{
						pCc->m_bTurn = TRUE;
						StepHex (pCc, -1, hexTry);
						pHex->ChangeToRoad (hexTry);
						if (pCc->m_iNumRoads++ > MAX_ROADS)
							return (FALSE);
						return (TryRoad (pCc));
						}
					}

				// try right
				if ((pCc->m_iRight > 2) && (pCc->m_iRight > RandNum (6)))
					{
					hexTry = HexNext (pCc, 1);
					pHex = theMap._GetHex (hexTry);
					if ((! pHex->IsWater ()) && (pHex->GetType () != CHex::road) && 
														(! (pHex->GetUnits () & CHex::unit)))
						{
						pCc->m_bTurn = TRUE;
						StepHex (pCc, 1, hexTry);
						pHex->ChangeToRoad (hexTry);
						if (pCc->m_iNumRoads++ > MAX_ROADS)
							return (FALSE);
						return (TryRoad (pCc));
						}
					}
				}
			}
		}

	// all that's left is to build straight
	hexTry = HexNext (pCc, 0);
	pHex = theMap._GetHex (hexTry);
	if ((! pHex->IsWater ()) && (pHex->GetType () != CHex::road) && 
													(! (pHex->GetUnits () & CHex::unit)))
		{
		StepHex (pCc, 0, hexTry);
		pHex->ChangeToRoad (hexTry);
		if (pCc->m_iNumRoads++ > MAX_ROADS)
			return (FALSE);
		return (TryRoad (pCc));
		}

	// dead end
	return (FALSE);
}

// find a place to build this
BOOL CPlayer::BuildCcBldg (int iBldg)
{

	// step 1 - if we haven't been here before we lay down our initial crossroads
	CHex * pHex = theMap._GetHex (m_hexCity);
	if (pHex->GetType () != CHex::road)
		if (! InitialRoad ())
			return (FALSE);
			
	// step 2 - we walk out a road
	CCitCon cc (iBldg, m_hexCity, RandNum (3));
	return (TryRoad (&cc));
}
