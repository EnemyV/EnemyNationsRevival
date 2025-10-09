//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "player.h"
#include "ai.h"
#include "lastplnt.h"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


BOOL CVehicle::NextVisible (int * * ppiOn, int * piRange, int & iXmax, int iYmin, int iYmax, CHexCoord const & hexOrig, CHexCoord & hexDest, int & iMode, int iDif)
{

	// how far away we got stopped tells us how many other points on the circle we don't
	// need to walk to
	iDif = (iDif / 2) + 1;
	while (iDif--)
		switch (iMode)
		  {
			case 0 :		// 12:00 - 3:00
				if (hexDest.X () != iXmax)
					hexDest.Xinc ();
				else
					if (hexDest.Y () != hexOrig.Y ())
						{
						hexDest.Yinc ();
						(*ppiOn)++;
						iXmax = CHexCoord::Wrap (hexOrig.X () + **ppiOn);
						}
					else
						{
						iMode = 1;
						hexDest.Yinc ();
						(*ppiOn)++;
						hexDest.X (hexOrig.X () + **ppiOn);
						(*ppiOn)++;
						iXmax = CHexCoord::Wrap (hexOrig.X () + **ppiOn);
						}
			break;

			case 1 :		// 3:00 - 6:00
				if (hexDest.X () != iXmax)
					hexDest.Xdec ();
				else
					{
					if (hexDest.Y () != iYmax)
						{
						hexDest.Yinc ();
						(*ppiOn)++;
						iXmax = CHexCoord::Wrap (hexOrig.X () + **ppiOn);
						}
					else
						{
						iMode = 2;
						(*ppiOn) = piRange;
						iXmax = CHexCoord::Wrap (hexOrig.X () - **ppiOn);
						}
					}
				break;

			case 2 :		// 6:00 - 9:00
				if (hexDest.X () != iXmax)
					hexDest.Xdec ();
				else
					if (hexDest.Y () != hexOrig.Y ())
						{
						hexDest.Ydec ();
						(*ppiOn)++;
						iXmax = CHexCoord::Wrap (hexOrig.X () - **ppiOn);
						}
					else
						{
						iMode = 3;
						hexDest.Ydec ();
						(*ppiOn)++;
						hexDest.X (hexOrig.X () - **ppiOn);
						(*ppiOn)++;
						iXmax = CHexCoord::Wrap (hexOrig.X () - **ppiOn);
						}
			break;

			case 3 :		// 9:00 - 12:00
				if (hexDest.X () != iXmax)
					hexDest.Xinc ();
				else
					if (hexDest.Y () != iYmin)
						{
						hexDest.Ydec ();
						(*ppiOn)++;
						iXmax = CHexCoord::Wrap (hexOrig.X () - **ppiOn);
						}
					else
						{
						if (hexDest.X () != hexOrig.X ())
							hexDest.Xinc ();
						else
							return (TRUE);
						}
				break;

#ifdef _DEBUG
			default:
				ASSERT (FALSE);
				break;
#endif
		  }

	return (FALSE);
}

// this function sets m_dwaSpot to the hexes this unit can see
void CVehicle::DetermineSpotting ()
{

	// only me
	if ( (m_pOwner == NULL) || (! m_pOwner->IsMe ()) )
		{
		TRAP ();
		return;
		}

	ASSERT (! m_bSpotted);

	// do we care?
	if ( ! DoSpotting () )
		return;

	// if we already have it - return
	if (m_hexVis.SameHex (m_ptHead))
		return;

	m_hexVis = m_ptHead;

	// set it to nothing spotted
	memset (m_dwaSpot, 0, sizeof (m_dwaSpot));

	// if in a building (ie, we don't own our hexes) we don't see anything
	if (! m_cOwn)
		return;

	// if <= 2 we can do this real fast
	// do a plus over the building
	int iDist = GetSpottingRange ();
	switch (iDist)
	  {
		case 0 :	// just us
			TRAP ();
			m_dwaSpot [MAX_SPOTTING] = 1 << MAX_SPOTTING;
			break;
		case 1 :	// a PLUS
			m_dwaSpot [MAX_SPOTTING-1] = 1 << MAX_SPOTTING;
			m_dwaSpot [MAX_SPOTTING] = (1 << MAX_SPOTTING) | (1 << (MAX_SPOTTING + 1)) |
																													(1 << (MAX_SPOTTING - 1));
			m_dwaSpot [MAX_SPOTTING+1] = 1 << MAX_SPOTTING;
			break;

		case 2 : {	// a 2*PLUS & 1 into the diamonds
			m_dwaSpot [MAX_SPOTTING-1] = (1 << MAX_SPOTTING) | (1 << (MAX_SPOTTING + 1)) |
																													(1 << (MAX_SPOTTING - 1));
			m_dwaSpot [MAX_SPOTTING] = (1 << MAX_SPOTTING) | (1 << (MAX_SPOTTING + 1)) |
																													(1 << (MAX_SPOTTING - 1));
			m_dwaSpot [MAX_SPOTTING+1] = (1 << MAX_SPOTTING) | (1 << (MAX_SPOTTING + 1)) |
																													(1 << (MAX_SPOTTING - 1));

			CHexCoord hexOrig (m_ptHead);
			hexOrig.Ydec ();
			m_dwaSpot [MAX_SPOTTING-2] |= 1 << MAX_SPOTTING;

			hexOrig.Y (hexOrig.Y () + 2);
			m_dwaSpot [MAX_SPOTTING+2] |= 1 << MAX_SPOTTING;

			hexOrig.Ydec ();
			hexOrig.Xdec ();
			m_dwaSpot [MAX_SPOTTING] |= 1 << (MAX_SPOTTING - 2);

			hexOrig.X (hexOrig.X () + 2);
			m_dwaSpot [MAX_SPOTTING] |= 1 << (MAX_SPOTTING + 2);
			break; }

		default: {	// spotting > 2
			// now, find which hexes we can see
			// we do this by walking out to each edge point then falling back till we
			// are on the path to the next edge point
			TRAP (iDist >= CVehicle::m_iMaxRange);
			iDist = __min ( iDist, CVehicle::m_iMaxRange - 1 );
			int * piRange = CVehicle::m_apiWid [iDist];
			int * piOn = piRange;
			CHexCoord hexDest (m_ptHead.x / 2 - *piRange, m_ptHead.y / 2 - iDist);
			hexDest.Wrap ();
			CHexCoord hexOrig (m_ptHead);
			CHexCoord hexOn (hexOrig);
			int iAltHighest = 0, iAltLast = 128;
			int iXmax = CHexCoord::Wrap (hexOrig.X () + *piRange);
			int iYmin = hexDest.Y ();
			int iYmax = CHexCoord::Wrap (m_ptHead.y / 2 + iDist);
			int iMode = 0;
		
			// set the hex we are on to visible
			m_dwaSpot [MAX_SPOTTING] = 1 << MAX_SPOTTING;
			BOOL bBlocked = FALSE;
		
			for (; TRUE; )
				{
				// figure where to move
				int xDif = CHexCoord::Diff (hexDest.X() - hexOn.X());
				int yDif = CHexCoord::Diff (hexDest.Y() - hexOn.Y());
				if (abs (xDif) == abs (yDif))
					{
					// made it to this one
					if (xDif == 0)
						{
						if (NextVisible (&piOn, piRange, iXmax, iYmin, iYmax, hexOrig, hexDest, iMode, 0))
							break;
						bBlocked = FALSE;
						hexOn = hexOrig;
						iAltHighest = 0;
						iAltLast = 128;
						continue;
						}
		
					hexOn.X() += xDif > 0 ? 1 : -1;
					hexOn.Y() += yDif > 0 ? 1 : -1;
					hexOn.Wrap ();
					}
				else
					if (abs (xDif) > abs (yDif))
						{
						hexOn.X() += xDif > 0 ? 1 : -1;
						hexOn.WrapX ();
						}
					else
						if (abs (xDif) < abs (yDif))
						  {
							hexOn.Y() += yDif > 0 ? 1 : -1;
							hexOn.WrapY ();
						  }
		
				if (bBlocked)
					{
					int iInd = CHexCoord::Diff (hexOn.Y () - hexOrig.Y ()) + MAX_SPOTTING;
					int iShft = CHexCoord::Diff (hexOn.X () - hexOrig.X ()) + MAX_SPOTTING;
					ASSERT ((0 <= iInd) && (iInd < SPOTTING_LINE));
					ASSERT ((0 <= iShft) && (iShft < SPOTTING_LINE));
					m_dwaSpot [iInd] |= 1 << iShft;
					if (NextVisible (&piOn, piRange, iXmax, iYmin, iYmax, hexOrig, hexDest, iMode, max (abs (xDif), abs (yDif))))
						break;
					bBlocked = FALSE;
					hexOn = hexOrig;
					iAltHighest = 0;
					iAltLast = 128;
					continue;
					}
		
				// a forest or building blocks us (but we see the hex that blocks PLUS 1)
				CHex * pHex = theMap._GetHex (hexOn);
				if ((pHex->GetUnits () & CHex::bldg) || (pHex->GetType () == CHex::forest))
					bBlocked = TRUE;

				// cresting a peak blocks us
			  int iAltOn = pHex->GetSeaAlt ();
				if ((iAltOn > iAltLast) && (iAltOn > iAltHighest))
					iAltHighest = iAltOn;
				iAltLast = iAltOn;

				if (iAltOn >= iAltHighest-1)
					{
					int iInd = CHexCoord::Diff (hexOn.Y () - hexOrig.Y ()) + MAX_SPOTTING;
					int iShft = CHexCoord::Diff (hexOn.X () - hexOrig.X ()) + MAX_SPOTTING;
					ASSERT ((0 <= iInd) && (iInd < SPOTTING_LINE));
					ASSERT ((0 <= iShft) && (iShft < SPOTTING_LINE));
					m_dwaSpot [iInd] |= 1 << iShft;
					}
				}
			
			break; }
	  }	// end spotting > 2
}

// finds opportunity fire for the unit that just moved
void CVehicle::DetermineOppo ()
{

	// do we care?
	if (! GetOwner()->IsLocal ())
		return;

	ASSERT_VALID (this);

	// do we care? (AI needs to be called because of see_unit)
	if ( ! GetOwner()->IsAI ())
		if ( (m_pUnitOppo != NULL) || (GetRange() <= 0) )
			return;

	// oppo fire for human player
	if (GetOwner()->IsMe ())
		{
		int * piOn = CVehicle::m_apiWid [GetRange ()];
		CHexCoord _hex (m_ptHead.x / 2, m_ptHead.y / 2 - GetRange ());
		_hex.Wrap ();
		int iDamageOppo = 0;
		CUnit * pOppo = NULL;

		for (int y=-GetRange (); y<GetRange(); y++)
			{
			_hex.X (m_ptHead.x / 2 - *piOn);
			CHex * pHex = theMap._GetHex (_hex);

			for (int x=(*piOn)*2; x>=0; x--)
				{
				if ((pHex->GetVisibility ()) &&
									(pHex->GetUnits () & (CHex::ul | CHex::ur | CHex::ll | CHex::lr | CHex::bldg)))
					{
					if (pHex->GetUnits () & CHex::bldg)
						CheckOppo (theBuildingHex._GetBuilding (_hex), iDamageOppo, &pOppo);
					CSubHex _sub (_hex.X () * 2, _hex.Y () * 2);
					if (pHex->GetUnits () & CHex::ul)
						CheckOppo (theVehicleHex._GetVehicle (_sub), iDamageOppo, &pOppo);
					_sub.x++;
					if (pHex->GetUnits () & CHex::ur)
						CheckOppo (theVehicleHex._GetVehicle (_sub), iDamageOppo, &pOppo);
					_sub.y++;
					if (pHex->GetUnits () & CHex::lr)
						CheckOppo (theVehicleHex._GetVehicle (_sub), iDamageOppo, &pOppo);
					_sub.x--;
					if (pHex->GetUnits () & CHex::ll)
						CheckOppo (theVehicleHex._GetVehicle (_sub), iDamageOppo, &pOppo);
					if ((pOppo != NULL) && (iDamageOppo == 2))
						{
						SetOppo (pOppo);
						return;
						}
					}

				_hex.Xinc ();
				pHex = theMap._Xinc (pHex);
				}
			_hex.Yinc ();
			piOn++;
			}

		// something is better than nothing
		if (pOppo != NULL)
			SetOppo (pOppo);
		return;
		}

	// oppo fire for AI
	BOOL bIsSet;
	if ( (m_pUnitOppo != NULL) || (GetRange() <= 0) )
		bIsSet = TRUE;
	else
		bIsSet = FALSE;

	// we do the bigger of spotting/shooting range cause of IncSee
	int iMax = __max ( GetRange (), GetSpottingRange () );
	int * piOn = CVehicle::m_apiWid [ iMax ];
	CHexCoord _hex (m_ptHead.x / 2, m_ptHead.y / 2 - iMax);
	_hex.Wrap ();
	int iDamageOppo = 0;
	CUnit * pOppo = NULL;

	for (int y=-iMax; y<iMax; y++)
		{
		_hex.X (m_ptHead.x / 2 - *piOn);
		CHex * pHex = theMap._GetHex (_hex);
		for (int x=(*piOn)*2; x>=0; x--)
			{
			if (pHex->GetUnits () & (CHex::ul | CHex::ur | CHex::ll | CHex::lr | CHex::bldg))
				{
				if (pHex->GetUnits () & CHex::bldg)
					CheckAiOppo (theBuildingHex._GetBuilding (_hex), &pOppo, bIsSet);
				CSubHex _sub (_hex.X () * 2, _hex.Y () * 2);
				if (pHex->GetUnits () & CHex::ul)
					CheckAiOppo (theVehicleHex._GetVehicle (_sub), &pOppo, bIsSet);
				_sub.x++;
				if (pHex->GetUnits () & CHex::ur)
					CheckAiOppo (theVehicleHex._GetVehicle (_sub), &pOppo, bIsSet);
				_sub.y++;
				if (pHex->GetUnits () & CHex::lr)
					CheckAiOppo (theVehicleHex._GetVehicle (_sub), &pOppo, bIsSet);
				_sub.x--;
				if (pHex->GetUnits () & CHex::ll)
					CheckAiOppo (theVehicleHex._GetVehicle (_sub), &pOppo, bIsSet);
				if ( ( ! bIsSet ) && ( pOppo != NULL ) )
					{
					bIsSet = TRUE;
					SetOppo (pOppo);
					}
				}
			_hex.Xinc ();
			pHex = theMap._Xinc (pHex);
			}
		_hex.Yinc ();
		piOn++;
		}
}
// finds if anyone local can oppo fire on unit that just moved
void CUnit::OtherOppo( )
{
    // get the position of the unit for fast testing
    const CHexCoord _hex( GetUnitType( ) == CUnit::vehicle ? ( (CVehicle*)this )->GetHexHead( )
                                                           : ( (CBuilding*)this )->GetHex( ) );

    // Cache commonly used values
    const int hexX     = _hex.X( );
    const int hexY     = _hex.Y( );
    CPlayer*  pMyOwner = GetOwner( );

    // Process buildings
    POSITION pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );

        // Early exit checks - combined and optimized
        if ( ( pBldg->GetFlags( ) & CUnit::dying ) != 0 )
            continue;

        CPlayer* pBldgOwner = pBldg->GetOwner( );
        if ( !pBldgOwner->IsLocal( ) || pBldgOwner == pMyOwner )
            continue;

        // Cache building position
        const CHexCoord& bldgHex = pBldg->GetHex( );
        int              diffX   = CHexCoord::Diff( hexX - bldgHex.X( ) );
        int              diffY   = CHexCoord::Diff( hexY - bldgHex.Y( ) );

        // Quick distance check before expensive operations
        if ( diffX > MAX_SPOTTING || diffY > MAX_SPOTTING )
            continue;

        // Opportunity fire check
        if ( pBldg->m_pUnitOppo == NULL && pBldg->GetRange( ) > 0 )
        {
            if ( theMap.GetRangeDistance( this, pBldg ) <= pBldg->GetRange( ) )
            {
                CUnit* pOppo = NULL;
                if ( pBldgOwner->IsAI( ) )
                    pBldg->CheckAiOppo( this, &pOppo, FALSE );
                else
                {
                    int iDamageOppo = 0;
                    pBldg->CheckOppo( this, iDamageOppo, &pOppo );
                }
                if ( pOppo != NULL )
                    pBldg->SetOppo( this );
            }
        }

        // AI spotting check
        if ( m_pdwPlyrsSee[pBldgOwner->GetPlyrNum( )] == 0 )
        {
            int spotRange = pBldg->GetSpottingRange( );
            if ( diffX <= spotRange && diffY <= spotRange )
            {
                if ( theMap.GetRangeDistance( this, pBldg ) <= spotRange )
                    IncSee( pBldg );
            }
        }
    }

    // Process vehicles
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
        ASSERT_STRICT_VALID( pVeh );

        // Early exit checks
        if ( ( pVeh->GetFlags( ) & CUnit::dying ) != 0 )
            continue;

        CPlayer* pVehOwner = pVeh->GetOwner( );
        if ( !pVehOwner->IsLocal( ) || pVehOwner == pMyOwner )
            continue;

        // Cache vehicle position (divided by 2)
        const POINT& vehPt   = pVeh->GetPtHead( );
        int          vehHexX = vehPt.x / 2;
        int          vehHexY = vehPt.y / 2;
        int          diffX   = CHexCoord::Diff( hexX - vehHexX );
        int          diffY   = CHexCoord::Diff( hexY - vehHexY );

        // Quick distance check
        if ( diffX > MAX_SPOTTING || diffY > MAX_SPOTTING )
            continue;

        // Opportunity fire check
        if ( pVeh->m_pUnitOppo == NULL && pVeh->GetRange( ) > 0 )
        {
            if ( theMap.GetRangeDistance( this, pVeh ) <= pVeh->GetRange( ) )
            {
                CUnit* pOppo = NULL;
                if ( pVehOwner->IsAI( ) )
                    pVeh->CheckAiOppo( this, &pOppo, FALSE );
                else
                {
                    int iDamageOppo = 0;
                    pVeh->CheckOppo( this, iDamageOppo, &pOppo );
                }
                if ( pOppo != NULL )
                    pVeh->SetOppo( this );
            }
        }

        // AI spotting check
        if ( m_pdwPlyrsSee[pVehOwner->GetPlyrNum( )] == 0 )
        {
            int spotRange = pVeh->GetSpottingRange( );
            if ( diffX <= spotRange && diffY <= spotRange )
            {
                int iLOS = theMap.LineOfSight( this, pVeh );
                if ( iLOS >= 0 && iLOS <= spotRange )
                    IncSee( pVeh );
            }
        }
    }
}

void CUnit::CheckOppo (CUnit * pTarget, int & iDamageOppo, CUnit * * ppOppo)
{

	if (pTarget == NULL)
		return;

	// it's not a change
	if ( *ppOppo == pTarget )
		return;

	// if we're dying forget it
	if (pTarget->GetFlags() & CUnit::dying)
		return;

	// get this on the server w/ AI players
	if (GetOwner() == pTarget->GetOwner())
		return;

	// are we friends?
	if (pTarget->GetOwner()->GetRelations () <= RELATIONS_NEUTRAL)
		return;

	// vehicle stuff
	BOOL bCanShootAt = TRUE;
	if (GetUnitType () == CUnit::vehicle)
		{
		CVehicle * pVeh = (CVehicle *) this;
		// can we hit it? (fixed turret) - we only care if we are moving - if stopped we'll turn
		if ( (pVeh->GetTurret () == NULL) ||
								 ( (pVeh->GetData()->GetVehFlags () & CTransportData::FL1hex) &&
										(pVeh->GetRouteMode () != CVehicle::stop) ) )
			if (! pVeh->CanShootAt (pTarget))
				bCanShootAt = FALSE;

		// can't shoot, can't swing to shoot, and moving so no time to turn
		if ( ( ! bCanShootAt ) && ( pVeh->GetRouteMode () != CVehicle::stop ) )
			return;

		// not within LOS and moving so can't move within range
		if (pVeh->GetData()->GetBaseType () != CTransportData::artillery)
			if ( pVeh->GetRouteMode () != CVehicle::stop )
				if (theMap.LineOfSight (this, pTarget) < 0)
					return;
		}

	// get the damage it would inflict
	int iDamageNew = GetDamageInflicted ( pTarget, FALSE );

	// some target is better than none
	if (*ppOppo == NULL)
		{
		*ppOppo = pTarget;
		iDamageOppo = iDamageNew;
		return;
		}

	// we assume the target we have we can shoot at so no point in picking one we have to turn for
	if ( ! bCanShootAt )
		return;

	// if the damage is <= 0 and not better than what we have - forget it
	if ((iDamageNew <= 0) && (iDamageNew <= iDamageOppo))
		return;

	// if target shooting at us (and target so far wasn't) - we shoot back
	// AND the oppo unit we have so far isn't shooting at us
	if (((*ppOppo)->GetTarget () != this) && (pTarget->GetTarget () == this))
		{
		*ppOppo = pTarget;
		iDamageOppo = iDamageNew;
		return;
		}

	// if our existing oppo shoots at us - we're done
	if (((*ppOppo)->GetTarget () == this) && (iDamageOppo >= 0))
		return;

	// if our existing/new target can shoot and the other can't - we take the offensive one
	if ( ( (*ppOppo)->GetFireRate () != 0 ) != ( pTarget->GetFireRate () != 0 ) )
		{
		if ( (*ppOppo)->GetFireRate () != 0 )
			return;
		*ppOppo = pTarget;
		iDamageOppo = iDamageNew;
		return;
		}

	// want 2:1
 	if (((iDamageOppo < 2) && (iDamageNew > iDamageOppo)) ||
												  	((iDamageOppo > 2) && (iDamageNew >= 2)))
		{
		*ppOppo = pTarget;
		iDamageOppo = iDamageNew;
		}
}

void CUnit::CheckAiOppo (CUnit * pTarget, CUnit * * ppOppo, BOOL bIsSet)
{

	if (pTarget == NULL)
		return;

	// if we only care about see_unit here, leave if it's been marked
	if ( ( bIsSet ) && ( pTarget->GetSee (this) ) )
		return;

	// it's not a change
	if ( *ppOppo == pTarget )
		return;

	// if we're dying forget it
	if (pTarget->GetFlags() & CUnit::dying)
		return;

	if (pTarget->GetOwner () == GetOwner ())
		return;

	// get LOS to hit it
	int iLOS = theMap.LineOfSight (this, pTarget);

	// are we seeing this unit for the first time
	if ( (iLOS >= 0) && (iLOS <= GetSpottingRange ()) )
		pTarget->IncSee (this);

	// if we only care about see_unit here, leave
	if ( bIsSet )
		return;

	// are we withing LOS?
	if ((GetUnitType () == CUnit::vehicle) && 
						(((CVehicle *) this)->GetData()->GetBaseType () != CTransportData::artillery))
		if (iLOS < 0)
			return;

	// can it shoot that far?
	if ( iLOS > GetRange () )
		return;

	// if it's our target - take it
	if (pTarget == m_pUnitTarget)
		{
		*ppOppo = pTarget;
		return;
		}

#ifdef _LOGOUT
	logPrintf (LOG_PRI_USEFUL, LOG_ATTACK, "Unit %d AiOppoFire on unit %d", GetID (), pTarget->GetID ());
#endif

	if (AiOppoFire (this, pTarget))
		*ppOppo = pTarget;
}

