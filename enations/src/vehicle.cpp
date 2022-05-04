//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "event.h"
#include "lastplnt.h"
#include "cpathmgr.h"
#include "player.h"
#include "chproute.hpp"
#include "bridge.h"
#include "area.h"

#include "building.inl"
#include "vehicle.inl"
#include "terrain.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


int aiBaseDir[9] = { 7, 6, 5, 0, 0, 4, 1, 2, 3 };
int aiDir[9] = { 7*EIGHTH_ROT, 6*EIGHTH_ROT, 5*EIGHTH_ROT, 0, 0, 4*EIGHTH_ROT, 1*EIGHTH_ROT, 2*EIGHTH_ROT, 3*EIGHTH_ROT };


BOOL CVehicle::TestStuck ()
{

  // only do this for local AI vehicles
	if ( (! GetOwner()->IsLocal ()) || (GetOwner()->IsAI ()) )
		return FALSE;

  // only for trucks & cranes		
	if ( ( ! GetData()->IsTransport () ) && ( ! GetData()->IsCrane () ) )
		return FALSE;

  // are we there?
	if ( m_hexDest.SameHex ( m_ptHead ) )
		return FALSE;

	if ( m_dwTimeJump > theGame.GettimeGetTime () )
		return FALSE;
	m_dwTimeJump = theGame.GettimeGetTime () + 1000 * TRUCK_JUMP_TIME;

  // ok, we transport to the dest if we can
	CBuilding * pBldg = theBuildingHex._GetBuilding ( m_ptDest );
	if ( pBldg != NULL )
	  {
	  if ( ! CanEnterBldg (pBldg) )
      {
      TRAP ();
  		return FALSE;
      }

    if ( m_pTransport != NULL )
      {
      TRAP ();
			POSITION pos = m_pTransport->m_lstCargo.Find ( this );
      if ( pos != NULL )
  			m_pTransport->m_lstCargo.RemoveAt ( pos );
			m_pTransport = NULL;
      }

		ReleaseOwnership ();
		ForceAtDest ();
		ArrivedDest ();
 		return TRUE;
    }

  // if no path left, give up
	if ( (m_phexPath == NULL) || (m_iPathLen < m_iPathOff+5) )
		return FALSE;

  // not a building, can we find a clear set of hexes along it's path?
	CHexCoord * pHexOn = m_phexPath + m_iPathOff + 2;
  int iNumTries = m_iPathLen - m_iPathOff - 3;
  while ( iNumTries > 0 )
    {
    // do we have a clear hex?
	  if ( ! ( theMap._GetHex ( *pHexOn )->GetUnits () & CHex::unit ) )
      {
      // put it here pointing at the next hex
  		ReleaseOwnership ();
      if ( pHexOn->X () < (pHexOn+1)->X () )
        {
        TRAP ();
		    m_ptTail.x = pHexOn->X () * 2;
		    m_ptHead.y = m_ptTail.y = pHexOn->Y () * 2 + 1;
		    m_ptHead.x = m_ptTail.x + 1;
        }
      else
        if ( pHexOn->X () > (pHexOn+1)->X () )
          {
          TRAP ();
		      m_ptHead.x = pHexOn->X () * 2;
		      m_ptHead.y = m_ptTail.y = pHexOn->Y () * 2;
  		    m_ptTail.x = m_ptHead.x + 1;
          }
        else
          if ( pHexOn->Y () < (pHexOn+1)->Y () )
            {
            TRAP ();
		        m_ptHead.y = pHexOn->Y () * 2 + 1;
		        m_ptHead.x = m_ptTail.x = pHexOn->X () * 2;
    		    m_ptTail.y = m_ptHead.y - 1;
            }
          else
            {
		        m_ptHead.y = pHexOn->Y () * 2;
		        m_ptHead.x = m_ptTail.x = pHexOn->X () * 2 + 1;
    		    m_ptTail.y = m_ptHead.y + 1;
            }

      // get it going
      if ( m_pTransport != NULL )
        {
        TRAP ();
			  POSITION pos = m_pTransport->m_lstCargo.Find ( this );
        if ( pos != NULL )
    			m_pTransport->m_lstCargo.RemoveAt ( pos );
		  	m_pTransport = NULL;
        }

      m_ptNext = m_ptHead;
			m_hexNext =	* (pHexOn + 1);
      SetMoveParams ( FALSE );
    	AtNewLoc ();
			TakeOwnership ();
		  _SetRouteMode (moving);
    	return TRUE;
      }

    TRAP ();
    pHexOn ++;
    }

  // we failed
  TRAP ();
	return FALSE;
}

void CVehicle::Operate ()
{

	xASSERT_VALID (ASSERT_PRI_ANAL, ASSERT_VEH_MOVE, this);

	if (m_iFrameHit > 0)
		{
		m_iFrameHit -= theGame.GetFramesElapsed ();
		if (m_iFrameHit <= 0)
			{
			m_iFrameHit = 0;
			theApp.m_wndWorld.SetVehHit ();
			}
		}

	if ((m_unitFlags & (dying | stopped)) || (GetOwner () == NULL))
		return;

	// are we destroying?
	if ( GetOwner()->IsLocal () && ( m_unitFlags & CUnit::destroying ) )
		{
		int iNum = (int) theGame.GetOpersElapsed () + m_lOperMod;
		div_t dtKill = div ( iNum, AVG_SPEED_MUL );
		m_lOperMod = dtKill.rem;
		if ( dtKill.quot > 0 )
			AddDamageThisTurn ( this, dtKill.quot);
		}

	// first we handle combat stuff
	HandleCombat ();

#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif

	switch (m_cMode)
	  {
		// nothing happening
		case stop : {
			if (! GetOwner()->IsLocal ())
				return;

      if ( TestStuck () )
        return;

			xASSERT (ASSERT_PRI_ANAL, ASSERT_VEH_MOVE, (! m_cOwn) || (theBuildingHex.GetBuilding (m_ptHead) == NULL));
			if (! (m_bFlags & told_ai_stop))
				{
#ifdef _LOGOUT
				logPrintf (LOG_PRI_CRITICAL, LOG_VEH_MOVE, "!! backup method - AI told vehicle %d at sub (%d,%d) stopped", GetID (), m_ptHead.x, m_ptHead.y);
#endif
				PostArrivedOrBlocked ();
				}
			ASSERT ((m_bFlags & told_ai_stop) || (m_ptDest == m_ptHead));

			// handle setup fire here - we just 0 the reload
			int iSetup = GetData()->GetSetupFire ();
			if ( (iSetup > 0) && (m_FireSetupMod < iSetup) )
					{
					m_FireSetupMod += (int) ((float) theGame.GetOpersElapsed () * m_fDamPerfMult) / AVG_SPEED_MUL;
					m_dwReloadMod = 0;
					}
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
			return; }

		case moving :
			ASSERT (m_cOwn);
			Move ();
			m_FireSetupMod = 0;
			break;

		case contention :
			TRAP ();
			// BUGBUG - write this
      if ( TestStuck () )
        return;
			return;

		case traffic : {
      if ( TestStuck () )
        return;

			if (GetOwner()->IsLocal ())
				{
				m_dwTimeBlocked += theGame.GetFramesElapsed ();
				if (m_dwTimeBlocked < 24)
					return;
				_SetRouteMode (blocked);
				m_dwTimeBlocked = 0;
				m_iNumRetries = m_iBlockCount = 0;
				return;
				}

			// remote - see if the last know location is avail
			CVehicle * pVehHead = theVehicleHex._GetVehicle (m_mvlTraffic.m_ptHead);
			if ((pVehHead == NULL) || (pVehHead == this))
				{
				CVehicle * pVehTail = theVehicleHex._GetVehicle (m_mvlTraffic.m_ptTail);
				if ((pVehTail == NULL) || (pVehTail == this))
					{
					// we just assume it's changing hexes
					if ( DoSpotting () && GetHexOwnership () )
						{
						TRAP ();
						DecrementSpotting ();
						}
					ReleaseOwnership ();

					// if next is free we move. Otherwise we just sit here
					CVehicle * pVehNext = theVehicleHex._GetVehicle (m_mvlTraffic.m_ptNext);
					if ((pVehNext == NULL) || (pVehNext == this))
						SetFromMsg (&m_mvlTraffic, FALSE);
					else
						{
						m_mvlTraffic.m_ptNext = m_mvlTraffic.m_ptHead;
						SetFromMsg (&m_mvlTraffic, FALSE);
						_SetRouteMode (stop);
						}

					// grab the hexes in the new location
					if (m_cOwn)
						{
						TRAP ((m_ptNext.x > 1280) || (m_ptNext.y > 1280));
						theVehicleHex.GrabHex (m_ptNext, this);
						theVehicleHex.GrabHex (m_ptHead, this);
						theVehicleHex.GrabHex (m_ptTail, this);

						if ( DoSpotting () )
							{
							TRAP ();
							DetermineSpotting ();
							IncrementSpotting ( m_ptHead );
							}
						}
					}
				}
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
			return; }

		case blocked :
      if ( TestStuck () )
        return;

			if (GetOwner()->IsLocal ())
				{
				HandleBlocked ();

#ifdef _DEBUG
				if (m_cMode != blocked)
					{
					ASSERT_VALID_LOC (this);
					ASSERT_STRICT (m_ptNext != m_ptHead);
					ASSERT_STRICT ((abs (CSubHex::Diff (m_ptNext.x - m_ptHead.x)) < 2) && 
												(abs (CSubHex::Diff (m_ptNext.y - m_ptHead.y)) < 2));
					}
#endif
				}
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
			return;

		// figures out when we can deploy
		case cant_deploy : {
			if (! GetOwner()->IsLocal ())
				return;

      if ( TestStuck () )
        return;

			m_FireSetupMod = 0;

			if ( m_iDelay > 0 )
				{
				m_iDelay -= theGame.GetOpersElapsed ();
				return;
				}

			// if a moving vehicle is blocking us we wait
			CVehicle * pVehNext = theVehicleHex._GetVehicle (m_ptNext);
			CVehicle * pVehHead = theVehicleHex._GetVehicle (m_ptHead);
			CVehicle * pVehTail = theVehicleHex._GetVehicle (m_ptTail);
			if ( ( (pVehNext != NULL) && (pVehNext->m_cMode == moving) ) ||
									( (pVehHead != NULL) && (pVehHead->m_cMode == moving) ) ||
									( (pVehTail != NULL) && (pVehTail->m_cMode == moving) ) )
				{
				m_dwTimeBlocked = 0;
				return;
				}

#ifdef _LOGOUT
			logPrintf (LOG_PRI_VERBOSE, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) trying to deploy", GetID (), m_ptHead.x, m_ptHead.y);
#endif
			
			// we don't do anything 1 time out of 4 to avoid deadlocks
			if ((MyRand () & 0x1100) == 0x1100)
				return;

			// if it's been over 30 seconds we tell the AI
			if ((m_dwTimeBlocked > 24 * 30) && (! (m_bFlags & told_ai_stop)))
				{
				CheckAroundBuilding ();
#ifdef _LOGOUT
				logPrintf (LOG_PRI_CRITICAL, LOG_VEH_MOVE, "!! backup method - AI told vehicle %d at sub (%d,%d) cant_deploy", GetID (), m_ptHead.x, m_ptHead.y);
#endif
				_SetRouteMode (stop);
				PostArrivedOrBlocked ();
				}

			// time we've been blocked
			m_dwTimeBlocked += theGame.GetFramesElapsed ();

			// if the hexes are taken make sure it's legit
			if (pVehHead != NULL)
				theVehicleHex.CheckHex (m_ptHead);
			else
				if (pVehTail != NULL)
					theVehicleHex.CheckHex (m_ptTail);
				else
					{
					// make sure we can stick our nose out
					BOOL bCanEnter = TRUE;
					if (! CanEnter (m_ptNext, FALSE))
						{
						bCanEnter = FALSE;
						int iDir, iMax;
						if (GetData()->GetVehFlags () & CTransportData::FL1hex)
							{
							iDir = -4;
							iMax = 3;
							}
						else
							{
							iDir = -2;
							iMax = 2;
							}
						for (; iDir<=iMax; iDir++)
							{
							CSubHex _next;
							if (GetData()->GetVehFlags () & CTransportData::FL1hex)
								_next = ::Rotate (iDir, m_ptHead, m_ptHead);
							else
								_next = Rotate (iDir);
							if ((! theBuildingHex._GetBuilding (_next)) && (CanEnter (_next, FALSE)))
								{
								m_ptNext = _next;
								bCanEnter = TRUE;
								break;
								}
							}
						}

					// deploy if available
					if (! bCanEnter)
						theVehicleHex.CheckHex (m_ptNext);
					else
						{
						TakeOwnership ();
						ASSERT_VALID_LOC (this);
						_SetRouteMode (deploy_it);
						ASSERT (m_cOwn);
						}
					}
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
			return; }

		case deploy_it :
			if (! GetOwner()->IsLocal ())
				return;

			m_FireSetupMod = 0;

			MaterialChange ();

			ASSERT (GetOwner()->IsLocal ());
			ASSERT (m_cOwn);
			ASSERT_VALID_LOC (this);
#ifdef _LOGOUT
			logPrintf (LOG_PRI_USEFUL, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) deploying", GetID (), m_ptHead.x, m_ptHead.y);
#endif

			// if we are there we don't need to move (from place_veh)
			if (m_hexDest.SameHex (m_ptHead))
				{
#ifdef _LOGOUT
				logPrintf (LOG_PRI_USEFUL, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) deployed at dest", GetID (), m_ptHead.x, m_ptHead.y);
#endif
				_SetRouteMode (stop);
				if (theBuildingHex._GetBuilding (m_ptHead) != NULL)
					EnterBuilding ();

				// tell the AI
				PostArrivedOrBlocked ();
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
				return;
				}

			// we already have a path, dest, etc
			if (HavePathOrNext ())
				{
				_SetRouteMode (moving);
				StartTravel (FALSE);
				ASSERT (m_cOwn);
				}
			else

				// no path yet - this will call GetPath
			  {
				_SetRouteMode (stop);
				DeletePath ();
				SetDest (m_hexDest);
				ASSERT (m_cMode != stop);
			  }

			// if its blocked go back to can't deploy
			if ((m_cMode == blocked) && (theBuildingHex._GetBuilding (m_ptHead) != NULL))
				{
#ifdef _LOGOUT
				logPrintf (LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) failed deploy", GetID (), m_ptHead.x, m_ptHead.y);
#endif
				ReleaseOwnership ();
				_SetRouteMode (cant_deploy);
				m_dwTimeBlocked = 0;
				CheckAroundBuilding ();
				}

#ifdef _DEBUG
			if ((m_cMode != blocked) && (m_cMode != cant_deploy))
				{
				ASSERT_VALID_LOC (this);
				ASSERT (m_ptNext != m_ptHead);
				ASSERT ((abs (CSubHex::Diff (m_ptNext.x - m_ptHead.x)) < 2) && 
											(abs (CSubHex::Diff (m_ptNext.y - m_ptHead.y)) < 2));
				}
#endif
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
			return;

		case run :
			switch (m_iEvent)
			  {
				case build :
					ConstructBuilding ();
					break;
				case build_road :
					ConstructRoad ();
					break;

#ifdef _DEBUG
				case load :
					TRAP (); // BUGBUG - I don't think this ever happens
					break;
#endif
			  }
			ASSERT_STRICT_VALID (this);
#ifdef TEST_TRAFFIC
	if ( m_cOwn )
		{
		TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
		TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
		for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
			if ( ( SubsOwned[iInd].x != -1 ) &&
						(SubsOwned[iInd] != m_ptNext) &&
						(SubsOwned[iInd] != m_ptHead) &&
						(SubsOwned[iInd] != m_ptTail) )
			TRAP ();
		}
#endif
			return;
	  }

	ASSERT_VALID (this);
}

void CVehicle::BuildBldg ()
{

	m_iEvent = none;
	if (! GetOwner()->IsLocal ())
		return;
	ASSERT_STRICT (m_ptHead.SameHex (m_ptTail));
		
	CMsgBuildBldg msg (this, m_hexBldg, m_iBuildDir, m_iBldgType);

	// make sure not on water or another city
	int iWhy;
	int iRtn = theMap.FoundationCost (m_hexBldg, m_iBldgType, m_iBuildDir, this, NULL, &iWhy);

	if ( theApp.m_pLogFile != NULL )
		{
		char sBuf [80];
		sprintf ( sBuf, "Request building %d at %d,%d = cost: %d, why: %d",
												m_iBldgType, m_hexBldg.X(), m_hexBldg.Y(), iRtn, iWhy );
		theApp.Log ( sBuf );
		}

	msg.m_iWhy = (signed char) iWhy;
	if (iRtn < 0)
		{
		_SetRouteMode (stop);
		msg.ToErr ();
		theGame.PostToClient (GetOwner(), &msg, sizeof (msg));
		return;
		}

	// send a message telling it to start building
	theGame.PostToServer (&msg, sizeof (msg));
}

void CVehicle::BuildRoad ()
{

	m_iEvent = none;
	if (! GetOwner()->IsLocal ())
		{
		ASSERT_STRICT (FALSE);
		return;
		}
	ASSERT_STRICT (m_ptHead.SameHex (m_ptTail));
		
	// if can't build here go on to the next
	CHex * pHex = theMap._GetHex (GetHexHead ());
	if ((pHex->IsWater ()) || (pHex->GetType () == CHex::road) || 
													(pHex->GetUnits () & (CHex::bridge | CHex::bldg)))
		if (! NextRoadHex ())
			return;

	// if no water - just a road
	CHexCoord _hex (m_ptHead);
	CHexCoord _hexNext = _NextRoadHex (_hex);
	if (theMap._GetHex (_hexNext)->CanRoad ())
		{
		// send a message telling it to start building
		CMsgBuildRoad msg (this);
		theGame.PostToServer (&msg, sizeof (msg));
		return;
		}

	if ( ! GetOwner()->CanBridge () )
		{
		if (GetOwner ()->IsMe ())
			theGame.Event (EVENT_CANT_BRIDGE, EVENT_NOTIFY);
		return;
		}

	// done?
	if ( m_hexEnd == _hex )
		{
		TRAP ();
		return;
		}

	// we need to place a bridge
	//   what direction
	int x = CHexCoord::Diff (m_hexEnd.X () - _hex.X ());
	int y = CHexCoord::Diff (m_hexEnd.Y () - _hex.Y ());
	if (abs (x) >= abs (y))
		{
		x = __minmax (-1, 1, x);
		y = 0;
		}
	else
	  {
		x = 0;
		y = __minmax (-1, 1, y);
	  }

	CHexCoord _hexEnd (_hexNext);
	int iLen = 0;
	while (iLen < MAX_SPAN)
		{
		_hexEnd.X () += x;
		_hexEnd.Y () += y;
		_hexEnd.Wrap ();

		// if we run into a bridge or building we're done
		CHex * pHexTest = theMap._GetHex (_hexEnd);
		if (pHexTest->GetUnits () & (CHex::bridge || CHex::bldg))
			{
			iLen = MAX_SPAN;
			break;
			}

		if ( pHexTest->IsWater () )
			{
			iLen++;
			if (iLen > MAX_SPAN)
				break;
			}
		else
			{
			iLen = 0;
			if ( pHexTest->CanRoad () )
				break;
			}
		}

	// if we failed - end it
	if (iLen >= MAX_SPAN)
		{
		if (GetOwner()->IsMe ())
			theGame.Event (EVENT_ROAD_HALTED, EVENT_WARN, this);
		else
			{
			CMsgBuildRoad msg (this);
			msg.ToErr ();
			theGame.PostToClient ( GetOwner (), &msg, sizeof (msg) );
			}
		return;
		}

	// let's build a bridge
	CMsgBuildBridge msg (this, _hexEnd);
	theGame.PostToServer (&msg, sizeof (msg));
}

void CVehicle::Load ()
{

	ASSERT_STRICT (GetOwner()->IsLocal ());	

	CBuilding * pBldg = theBuildingHex._GetBuilding (m_ptHead);
	if (pBldg == NULL)
		{
		ASSERT_STRICT (FALSE);
		return;
		}
	ASSERT_STRICT_VALID (pBldg);

	// see if full
	int iLeft = GetData()->GetMaxMaterials () - GetTotalStore ();
	if (iLeft <= 0)
		return;

	// if under construction we don't take anything
	if (pBldg->IsConstructing ())
		return;
		
	int iCant [CMaterialTypes::num_types];
	pBldg->GetInputs (iCant);

	// take a look at where we are going to figure out what to load up
	// this is generally only a problem if loading at a warehouse
	POSITION pos = m_pos;
	CBuilding * pBldgDest = NULL;
	while ( TRUE )
		{
		if (pos == NULL)
			pos = m_route.GetHeadPosition ();
		else
			{
			m_route.GetNext (pos);
			if (pos == NULL)
				pos = m_route.GetHeadPosition ();
			}
		if ((pos == m_pos) || (pos == NULL))
			break;

		// only care about unloads
		if ( m_route.GetAt (pos)->GetRouteType () == CRoute::unload )
			{
			pBldgDest = theBuildingHex._GetBuilding ((m_route.GetAt (pos))->GetCoord ());
			if ( pBldgDest )
				break;
			}
		}

	// ok, we know where we are going
	if (pBldgDest != NULL)
		{
		if (pBldgDest->IsConstructing ())
			{
			int iNeed = 0;
			for (int iOn=0; iOn<CMaterialTypes::GetNumBuildTypes(); iOn++)
				if (pBldgDest->GetStore (iOn) < pBldgDest->NeedToBuild (iOn))
					iNeed += pBldgDest->GetBldgMatReq (iOn, TRUE);
			if (iNeed > 0)
				{
				float fMul = __min (1.0f, (float) (iLeft / iNeed));
				for (iOn=0; iOn<CMaterialTypes::GetNumBuildTypes(); iOn++)
					if (pBldgDest->GetStore (iOn) < pBldgDest->NeedToBuild (iOn))
						{
						iNeed = (int) ((float) pBldgDest->GetBldgMatReq (iOn, TRUE) * fMul);
						iNeed = __min (iNeed, pBldg->GetStore (iOn));
						iNeed = __min (iNeed, pBldgDest->GetBldgMatReq (iOn, TRUE));
						pBldg->AddToStore (iOn, -iNeed);
						AddToStore ( iOn, iNeed );
						iLeft -= iNeed;
						}

				// return if full
				if (iLeft <= 0)
					{
					// its an event
					ASSERT_STRICT (iLeft >= 0);
					pBldg->EventOff ();
					pBldg->MaterialChange ();
					MaterialChange ();
					return;
					}
				}
			}

		// ok, load up with what the dest needs
		// determine what units the dest building uses that this building doesn't
		// the GetAccepts stuff is for warehouses
		int iUse [CMaterialTypes::num_types];
		pBldgDest->GetInputs (iUse);
		int iAcpts [CMaterialTypes::num_types];
		pBldgDest->GetAccepts (iAcpts);
		for (int iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
			iUse[iOn] = __max (iUse[iOn], iAcpts[iOn]);
		for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
			if (iCant[iOn] > 0)
				iUse[iOn] = 0;

		// figure out how many output units the building can make
		float fMul [CMaterialTypes::num_types];
		float fBiggest = 0;
		for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
			if (iUse[iOn] <= 0)
				fMul[iOn] = 0;
			else
				if ((fMul[iOn] = (float) pBldgDest->GetStore (iOn) / (float) iUse[iOn]) > fBiggest)
					fBiggest = fMul[iOn];

		// if its empty then no point in doing the below
		if (fBiggest > 0.0)
			{
			// set fMul to what is needed to make them all even
			for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
				if (iUse[iOn] > 0)
					fMul[iOn] = fBiggest - fMul[iOn];
			int iTotal = 0;
			for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
				if (fMul[iOn] > 0.0)
					iTotal += (int) ((float) iUse[iOn] * fMul[iOn]);
			// reduce the multiplier if not enough room
			if (iTotal > iLeft)
				{
				float fReduce = (float) iLeft / (float) iTotal;
				for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
					if (fMul[iOn] > 0.0)
						fMul[iOn] *= fReduce;
				}
			// FINALLY - stock up to make everyone even
			for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
				if (fMul[iOn] > 0.0)
					{
					int iNum = __min (iLeft, (int) ((float) iUse[iOn] * fMul[iOn]));
					iNum = __min (iNum, pBldg->GetStore (iOn));
					pBldg->AddToStore (iOn, -iNum);
					AddToStore ( iOn, iNum );
					iLeft -= iNum;
					}

			// return if full
			if (iLeft <= 0)
				{
				// its an event
				ASSERT_STRICT (iLeft >= 0);
				pBldg->EventOff ();
				pBldg->MaterialChange ();
				MaterialChange ();
				return;
				}
			}	// we are brought up even

		// ok, now fill the rest proportionally
		int iTotal = 0;
		for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
			iTotal += iUse [iOn];
		if (iTotal > 0)
			for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
				if (iUse [iOn] > 0)
					{
					int iNeed = (iUse[iOn] * iLeft) / iTotal;
					iTotal -= iUse[iOn];
					iNeed = __min (iNeed, iLeft);
					iNeed = __min (iNeed, pBldg->GetStore (iOn));
					pBldg->AddToStore (iOn, -iNeed);
					AddToStore ( iOn, iNeed );
					iLeft -= iNeed;
					}

		// return if full
		ASSERT_STRICT (iLeft >= 0);
		pBldg->EventOff ();
		pBldg->MaterialChange ();
		MaterialChange ();
		return; 
		}	// if pBldgDest

	// ok, if we don't know the dest, we go for equal amounts
	int iTypesHave = CMaterialTypes::GetNumTypes();
	for (int iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
		if (iCant[iOn] > 0)
			iTypesHave--;

	int iTypesLeft = iTypesHave;
	for (int iTry=0; (iTry<iTypesHave)&&(iLeft>0); iTry++)
		{
		if (iTypesLeft <= 0)
			break;
		int iNeed = __max (1, iLeft / iTypesLeft);
		iTypesLeft = iTypesHave;
		for (iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
			if (iCant[iOn] == 0)
				{
				if (pBldg->GetStore (iOn) == 0)
					iTypesLeft--;
				else
				  {
					int iNum = __min (iNeed, pBldg->GetStore (iOn));
					pBldg->AddToStore (iOn, -iNum);
					AddToStore ( iOn, iNum );
					iLeft -= iNum;
					if (iLeft <= 0)
						break;
					}
				}
		}

	ASSERT_STRICT (iLeft >= 0);
	pBldg->EventOff ();
	pBldg->MaterialChange ();
	MaterialChange ();
}

// give this building all it can hold of everything it uses that this truck has
//   note: if under construction this is a larger list.
void CVehicle::Unload ()
{

	ASSERT_STRICT (GetOwner()->IsLocal ());	
	
	CBuilding * pBldg = theBuildingHex._GetBuilding (m_ptHead);
	if (pBldg == NULL)
		{
		ASSERT_STRICT (FALSE);
		return;
		}
	ASSERT_STRICT_VALID (pBldg);

	// construction
	if (pBldg->IsConstructing ())
		{
		for (int iOn=0; iOn<CMaterialTypes::GetNumBuildTypes(); iOn++)
			{
			int iNum = pBldg->NeedToBuild (iOn);
			AddToStore ( iOn, - iNum );
			pBldg->AddToStore (iOn, iNum);
			}
		// we drop through for materials the building will need when done
		}

	// not construction
	int iAccepts [CMaterialTypes::num_types];
	pBldg->GetAccepts (iAccepts);
	for (int iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
		if (iAccepts[iOn] >= 0)
			{
			pBldg->AddToStore (iOn, GetStore (iOn) );
			SetStore ( iOn, 0 );
			if ((theGame.GetScenario () == 7) && (pBldg->GetStore (CMaterialTypes::copper) > 0))
				theGame.m_iScenarioVar++;
			}

	// its an event
	pBldg->EventOff ();
	pBldg->MaterialChange ();
	MaterialChange ();
}

void CVehicle::ConstructBuilding ()
{

	ASSERT_STRICT_VALID (this);

	if (m_pBldg == NULL)
		return;
	ASSERT_STRICT_VALID (m_pBldg);

	// get change based on everything
	int iInc = GetProd (GetOwner()->GetConstProd ());
	if (iInc <= 0)
		return;

	m_pBldg->AddConstDone (iInc);

	// update the status
	MaterialChange ();
}

void CVehicle::ConstructRoad ()
{

	ASSERT_STRICT_VALID (this);

	int iInc = GetProd (GetOwner()->GetConstProd ());
	if (iInc <= 0)
		return;

	CHexCoord _hexHead (GetHexHead ());
	CHex * pHex = theMap._GetHex (_hexHead);

	// if we are starting see if we can find gas to build with
	if ((m_iBuildDone <= 0) && (GetOwner()->IsLocal ()))
		{
		if (! GetOwner ()->BuildRoad ())
			{
			if (GetOwner()->IsMe ())
				theGame.Event (EVENT_ROAD_HALTED, EVENT_WARN, this);
			else
				{
				CMsgBuildRoad msg (this);
				msg.ToErr ();
				theGame.PostToClient ( GetOwner (), &msg, sizeof (msg) );
				}
			SetEventAndRoute (none, stop);
			return;
			}

		// set to under construction
		pHex->ChangeToRoad ( _hexHead, FALSE, GetOwner()->IsMe () );

		if ( pHex->GetVisibility () || GetOwner()->IsMe () )
			{
			pHex->m_psprite = theTerrain.GetSprite ( CHex::road, CHex::r_path );
			_hexHead.SetInvalidated ();
			}
		}

	m_iBuildDone += iInc;

	// see if we are done
	int iTotal;
	CBridge * pBridge = NULL;
	if (pHex->GetUnits () & CHex::bridge)
		{
		pBridge = theBridgeHex.GetBridge (_hexHead)->GetParent();
		iTotal = pBridge->GetConstTotal ();
		}
	else
		iTotal = theTerrain.GetData (pHex->GetType ()).GetBuildMult () * CTerrainData::GetBuildRoadTime ();

	if (m_iBuildDone >= iTotal)
		{
		// if its not ours we need to wait for a message saying its done
		if (! GetOwner()->IsLocal ())
			{
			m_iBuildDone = iTotal - 1;
			if (m_iLastPer < 99)
				m_iLastPer = 99;
			_SetEventAndRoute (none, stop);
			return;
			}

		m_iLastPer = 100;
		m_iBuildDone = -1;

		// invalidate
		// 		mark the bridge as completed
		if (pBridge != NULL)
			pBridge->BridgeBuilt ();

		// tell everyone
		CMsgRoadDone msg (this);
		theGame.PostToAll (&msg, sizeof (msg));

		// update the status
		MaterialChange ();

		// do the next hex
		NextRoadHex ();
		return;
		}

	int iPer = (m_iBuildDone * 100) / iTotal;
	if (iPer == m_iLastPer)
		return;
	m_iLastPer = iPer;

	// set the bridges
	if (pBridge != NULL)
		pBridge->_SetPer (iPer);

	// update the status
	MaterialChange ();
}

CHexCoord CVehicle::_NextRoadHex (CHexCoord const & _hexOn)
{

	// if we're there - return it
	if (_hexOn == m_hexEnd)
		return (_hexOn);
		
	// nope, go to the next hex
	// move closer on the longest one (so we go diaganol)
	int x = CHexCoord::Diff (m_hexEnd.X () - _hexOn.X ());
	int y = CHexCoord::Diff (m_hexEnd.Y () - _hexOn.Y ());
	CHexCoord hex (_hexOn);
	if (abs (x) >= abs (y))
		{
		if (x > 0)
			hex.Xinc ();
		else
			hex.Xdec ();
		}
	else
	  {
		if (y > 0)
			hex.Yinc ();
		else
		  hex.Ydec ();
	  }

	return (hex);
}

BOOL CVehicle::NextRoadHex ()
{

	// nope, go to the next hex
	CHexCoord _hex (m_ptHead);

	// skip by buildings
	while (TRUE)
		{
		// if this was the last, shut us down
		if (m_ptHead.SameHex (m_hexEnd))
			{
			_SetEventAndRoute (none, stop);
			theGame.Event (EVENT_ROAD_DONE, EVENT_NOTIFY, this);
			return (FALSE);
			}

		// if we're on a bridge - cross it AND OFF
		CBridgeUnit * pBu = theBridgeHex.GetBridge (_hex);
		if (pBu != NULL)
			{
			_hex = pBu->GetParent()->GetHexEnd ();
			int iExitDir = (pBu->GetParent())->GetUnitEnd ()->GetExit ();
			_hex.X () += (iExitDir & 1) ? 2 - iExitDir : 0;
			_hex.Y () += (! (iExitDir & 1)) ? iExitDir - 1 : 0;
			_hex.Wrap ();
			}
		else
			_hex = _NextRoadHex (_hex);

		// done?
		if ( _hex == m_hexEnd )
			break;

		// skip around buildings & bridges
		if ( ! ( theMap._GetHex (_hex)->GetUnits () & (CHex::bldg | CHex::bridge) ) )
			break;
		}

	SetDestAndMode (_hex, center);
	return (TRUE);
}

// can't shoot at this guy anymore
void CVehicle::StopShooting (CUnit * pNewTarget)
{

	// if any vehicles are shooting at us - have them hit our owner
	POSITION pos = theVehicleMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CVehicle *pVeh;
		theVehicleMap.GetNextAssoc (pos, dwID, pVeh);
		ASSERT_STRICT_VALID (pVeh);
		if (pVeh->GetTarget () == this)
			{
			pVeh->_SetTarget (pNewTarget);
			if (pVeh->GetOwner()->IsAI ())
				{
				CMsgOutOfLos msg (pVeh, this);
				theGame.PostToClient (pVeh->GetOwner(), &msg, sizeof (msg));
				}
			}
		}

	// same for buildings
	pos = theBuildingMap.GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if (pBldg->GetTarget () == this)
			{
			pBldg->_SetTarget (pNewTarget);
			if (pBldg->GetOwner()->IsAI ())
				{
				CMsgOutOfLos msg (pBldg, this);
				theGame.PostToClient (GetOwner(), &msg, sizeof (msg));
				}
			}
		}

}

BOOL CVehicle::CanShootAt (CUnit * pTarget)
{

	if (pTarget == NULL)
		{
		TRAP ();
		return (FALSE);
		}

	int iDir;
	if (GetTurret () != NULL)
		{
		TRAP ();
		iDir = GetTurret()->m_iDir;
		}
	else
		iDir = m_iDir;

	int iTargetDir = FastATan (CMapLoc::Diff (pTarget->GetWorldPixels().x - m_maploc.x),
															CMapLoc::Diff (pTarget->GetWorldPixels().y - m_maploc.y));

	int iSwing;
	// these units can shoot in a 45 degree angle off their front
	if ( GetData()->GetVehFlags () & CTransportData::FLshoot180 )
		iSwing = FULL_ROT / 8;
	else
		iSwing = FULL_ROT / 32;

	int iRange = ( ( ( iTargetDir - iDir ) + 64 ) & 127 ) - 64;
	// we must be within 45 degrees (stop next to problems with below)
	if ( abs (iRange) > FULL_ROT/8 )
		return (FALSE);

	iRange = __minmax ( - iSwing, iSwing, iRange );
	iDir += iRange;
	iDir = __roll ( 0, FULL_ROT, iDir );

	// why waste time below?
	if ( iDir == iTargetDir )
		return (TRUE);

	// find dist to target
	int xDif = abs ( CMapLoc::Diff ( pTarget->GetWorldPixels().x - m_maploc.x ) );
	int yDif = abs ( CMapLoc::Diff ( pTarget->GetWorldPixels().y - m_maploc.y ) );
	int iDif = __max ( xDif, yDif );
	int xDir;
	if (iDir < 32)
		xDir = 64 - iDir;
	else
		if (iDir > 96)
			xDir = 192 - iDir;
		else
			xDir = iDir;
	int yDir = iDir <= 64 ? iDir : 128 - iDir;

	// find hit maploc
	CMapLoc ml;
	ml.x = m_maploc.x + ( iDif * ( 64 - xDir ) ) / 32;
	ml.y = m_maploc.y - ( iDif * ( 32 - yDir ) ) / 32;
	ml.Wrap ();

	// how far away from target - iDif is measure of how far from target
	//   so a target further away must be a smaller angle off
	int iShift = 0;
	iDif >>= 2;
	while (iDif)
		{
		iDif >>= 1;
		iShift ++;
		}
	xDif = abs ( CMapLoc::Diff ( pTarget->GetWorldPixels().x - ml.x ) ) >> iShift;
	yDif = abs ( CMapLoc::Diff ( pTarget->GetWorldPixels().y - ml.y ) ) >> iShift;
	iDif = __max ( xDif, yDif );

	// see if it's us
	if (pTarget->GetUnitType () == CUnit::building)
		{
		// if within the furthest possible square...
		CBuilding * pBldg = (CBuilding *) pTarget;
		if ( iDif/2 <= __max ( pTarget->GetData()->GetCX (), pTarget->GetData()->GetCY ()) )
			return (TRUE);
		return (FALSE);
		}

	// vehicle
	CVehicle * pVeh = (CVehicle *) pTarget;
	if (GetData()->GetVehFlags () & CTransportData::FL1hex)
		return ( iDif <= 1 );

	return ( iDif <= 2 );
}

void CVehicle::DestroyLoadWindow ()
{

	if ( (m_pDlgLoad != NULL) && (m_pDlgLoad->m_hWnd != NULL) )
		m_pDlgLoad->DestroyWindow ();
	else
		delete m_pDlgLoad;
	m_pDlgLoad = NULL;
}

CDlgLoadTruck * CVehicle::GetDlgLoad ()
{

	if (! GetData()->IsTransport ())
		{
		TRAP ();
		return (NULL);
		}

	CWndArea * pWndArea = theAreaList.GetTop ();
	if (m_pDlgLoad == NULL)
		m_pDlgLoad = new CDlgLoadTruck (pWndArea, this);
	if (m_pDlgLoad->m_hWnd == NULL)
		m_pDlgLoad->Create (IDD_TRUCK, pWndArea);

	m_pDlgLoad->ShowWindow (SW_RESTORE);
	m_pDlgLoad->SetFocus ();

	return (m_pDlgLoad);
}

void CVehicle::SetBridgeHex (CHexCoord const & hexStart, CHexCoord const & hexEnd, DWORD dwID, int iAlt)
{

	// put bridge buildings there
	CBridge::Create (hexStart, hexEnd, dwID, iAlt);

	// set the vehicle to bridge building
	ASSERT (hexStart.SameHex (m_ptHead));
	SetEvent (build_road);
}

void CVehicle::SetTransport ( CVehicle * pVehCarrier )
{

	// if we're on another vehicle, don't do it
	if (m_pTransport != NULL)
		return;

	// can't load on ourself
	if ( pVehCarrier == this )
		{
		TRAP ();
		return;
		}

	// do we have room
	int iSize;
	if (GetData()->IsPeople ())
		iSize = 1;
	else
		iSize = MAX_CARGO;
	if ( pVehCarrier->m_iCargoSize + iSize > pVehCarrier->GetData()->GetPeopleCarry () )
		{
		TRAP ();
		return;
		}

	_SetEventAndRoute (CVehicle::none, CVehicle::stop);
	m_pTransport = pVehCarrier;

	m_hexDest = m_ptDest = m_ptHead = pVehCarrier->m_ptHead;
	if (GetData ()->GetVehFlags () & CTransportData::FL1hex)
		m_ptTail = m_ptHead;
	else
		m_ptTail = pVehCarrier->m_ptTail;
	SetLoc (TRUE);
	ZeroMoveParams ();
	DeletePath ();

	pVehCarrier->m_lstCargo.AddTail ( this );
	pVehCarrier->m_iCargoSize += iSize;

	m_pVehLoadOn = NULL;
}

void CVehicle::DestroyAllWindows () 
{ 

	DestroyRouteWindow (); 
	DestroyBuildWindow (); 
	DestroyLoadWindow (); 
}

// dump the contents of this truck in the nearest warehouse and give to auto router
void CVehicle::DumpContents ()
{

	// deselect it
	CWndArea * pWndArea = theAreaList.GetTop ();
	if (pWndArea)
		pWndArea->SubSelectUnit ( this );

	// if no materials - turn it over
	if (GetTotalStore () == 0)
		{
		m_bFlags &= ~ ( hp_controls | dump_contents );
		theGame.m_pHpRtr->MsgGiveVeh ( this );
		theGame.m_pHpRtr->MsgArrived ( this );
		return;
		}

	// if we are in any building - dump here
	CBuilding * pBldg = theBuildingHex.GetBuilding (m_ptHead);
	if (pBldg != NULL)
		{
		// unload it
		for (int iOn=0; iOn<CMaterialTypes::GetNumTypes(); iOn++)
			{
			pBldg->AddToStore (iOn, GetStore (iOn) );
			SetStore ( iOn, 0 );
			if ((theGame.GetScenario () == 7) && (pBldg->GetStore (CMaterialTypes::copper) > 0))
				theGame.m_iScenarioVar++;
			}

		pBldg->EventOff ();
		m_bFlags &= ~ ( hp_controls | dump_contents );
		theGame.m_pHpRtr->MsgGiveVeh ( this );
		theGame.m_pHpRtr->MsgArrived ( this );
		return;
		}

	// find the closest warehouse
	int iDist = 200000;
	CBuilding * pDest = NULL;
	POSITION pos = theBuildingMap.GetStartPosition ();
	CHexCoord _hexTruck ( m_ptHead );
	while (pos != NULL)
		{
		DWORD dwID;
		CBuilding *pBldg;
		theBuildingMap.GetNextAssoc (pos, dwID, pBldg);
		ASSERT_STRICT_VALID (pBldg);
		if ((pBldg->GetOwner()->IsMe ()) && ( (pBldg->GetData()->GetType () == CStructureData::rocket) ||
																					(pBldg->GetData()->GetType () == CStructureData::warehouse) ) )
			{
			int _iDist = CHexCoord::Dist ( _hexTruck, pBldg->GetHex () );
			if ( (pDest == NULL) || (_iDist < iDist) )
				{
				iDist = _iDist;
				pDest = pBldg;
				}
			}
		}

	// if didn't find - hand it off
	if ( pDest == NULL )
		{
		TRAP ();
		m_bFlags &= ~ ( hp_controls | dump_contents );
		theGame.m_pHpRtr->MsgGiveVeh ( this );
		theGame.m_pHpRtr->MsgArrived ( this );
		return;
		}

	// send it there
	m_bFlags |= dump_contents;
	SetDest ( pDest->GetHex () );
}

void CVehicle::AddSubOwned ( int x, int y )
{

	TRAP ( ! m_cOwn );
	TRAP ( (theVehicleHex.GetVehicle (x,y) != NULL) && (theVehicleHex.GetVehicle (x,y) != this) );

	// already have it?
	for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
		if ( (SubsOwned[iInd].x == x) && (SubsOwned[iInd].y == y) )
			return;

	// find a sub to use
	for (iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
		if ( SubsOwned[iInd].x == -1 )
			{
			SubsOwned [iInd].x = x;
			SubsOwned [iInd].y = y;
			return;
			}

	// any dups
	TRAP ();
	for (iInd=0; iInd<NUM_SUBS_OWNED-1; iInd++)
		for (int iChk=iInd+1; iChk<NUM_SUBS_OWNED; iChk++)
			if ( SubsOwned[iInd] == SubsOwned[iChk] )
				{
				TRAP ();
				SubsOwned [iInd].x = x;
				SubsOwned [iInd].y = y;
				return;
				}

	// oh-oh no free subs - we now see who should be free
	for (iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
		if ( (SubsOwned[iInd] != m_ptNext) && (SubsOwned[iInd] != m_ptHead) && (SubsOwned[iInd] != m_ptTail) )
			{
			// if we still own it - free it
			if ( theVehicleHex.GetVehicle ( SubsOwned [iInd] ) == this )
				theVehicleHex.ReleaseHex ( SubsOwned [iInd], this );
			SubsOwned [iInd].x = x;
			SubsOwned [iInd].y = y;
			return;
			}

	// should be impossible
	TRAP ();
}

void CVehicle::RemoveSubOwned ( int x, int y )
{

#ifdef _TRAP
	TRAP ( ! m_cOwn );
	TRAP ( theVehicleHex.GetVehicle (x,y) != this );
	BOOL bOk = FALSE;
#endif

	// remove from the list
	for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
		if ( ( SubsOwned[iInd].x == x ) && ( SubsOwned[iInd].y == y ) )
			{
			SubsOwned [iInd].x = -1;

#ifdef _TRAP
			bOk = TRUE;
#endif
			}

	// shouldn't happen
#ifdef _TRAP
	TRAP ( ! bOk );
#endif
}
