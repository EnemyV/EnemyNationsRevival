//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "lastplnt.h"
#include "chproute.hpp"
#include "event.h"
#include "cpathmgr.h"
#include "player.h"
#include "area.h"
#include "bridge.h"

#include "terrain.inl"
#include "vehicle.inl"
#include "building.inl"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


const int MAX_TIMES_CIRCLE = 16;


// the vehicle is moving
void CVehicle::Move() {

#ifdef STRICTER_ASSERTS
    ASSERT ((m_cMode == moving) && (m_cOwn));
#endif

    ASSERT_VALID (this);
#ifndef _GG
#ifdef STRICTER_ASSERTS
    ASSERT( m_ptNext != m_ptHead );
#endif
#endif

#ifdef STRICTER_ASSERTS
    ASSERT( theVehicleHex.GetVehicle( m_ptNext ) == this );
#endif

#ifdef TEST_TRAFFIC
    TRAP ( ! m_cOwn );
    TRAP (theVehicleHex._GetVehicle (m_ptNext) != this);
    TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
    TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
        for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
            if ( ( SubsOwned[iInd].x != -1 ) &&
                        (SubsOwned[iInd] != m_ptNext) &&
                        (SubsOwned[iInd] != m_ptHead) &&
                        (SubsOwned[iInd] != m_ptTail) )
            TRAP ();
#endif

    // for world map
    CHexCoord _hexHead(m_ptHead);
    BOOL bVis = IsVisible();

    // operations this turn
    m_fVehMove += (m_fDamPerfMult * 0.9 * (float) (theGame.GetOpersElapsed() * GetData()->GetSpeed())) /
                  (float) AVG_SPEED_MUL;

    // moving toward the next hex
    if (m_iStepsLeft > 0)
        if (!MoveInHex()) {
            ASSERT_VALID (this);
            return;
        }

#ifdef TEST_TRAFFIC
    TRAP ( ! m_cOwn );
    TRAP (theVehicleHex._GetVehicle (m_ptNext) != this);
    TRAP (theVehicleHex._GetVehicle (m_ptHead) != this);
    TRAP (theVehicleHex._GetVehicle (m_ptTail) != this);
        for (iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
            if ( ( SubsOwned[iInd].x != -1 ) &&
                        (SubsOwned[iInd] != m_ptNext) &&
                        (SubsOwned[iInd] != m_ptHead) &&
                        (SubsOwned[iInd] != m_ptTail) )
            TRAP ();
#endif

    // not arrived at dest yet - keep moving
    if (m_ptDest != m_ptHead) {
        // we've arrived at ptNext
        //   inside if for special case of already there
        ArrivedNextHex();

        // We've made it to m_hexNext
        // if its not ours, we wait
        if (!GetOwner()->IsLocal()) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d wait for owner", GetID());
#endif

            _SetRouteMode(stop);
            ASSERT_VALID (this);
            SetMoveParams(FALSE);
            theApp.m_wndWorld.InvalidateWindow(CWndWorld::visible | CWndWorld::other_units);
            return;
        }

        // move to the next hex (returns FALSE if stuck)
        if (!FindNextHex()) {
            ASSERT_VALID (this);
#ifndef _GG
#ifdef STRICTER_ASSERTS
            ASSERT (m_cMode == blocked);
#endif
#endif
            goto Done;
        }
        ASSERT ((m_cMode != moving) || (m_ptDest == m_ptHead) || (m_ptNext != m_ptHead));
    }

    // we've arrived
    if (m_ptDest == m_ptHead) {
        ASSERT ((m_iDestMode == sub) || (m_ptHead.SameHex(m_ptDest)));
        ASSERT (m_ptHead.SameHex(m_hexDest));
        ArrivedDest();
        ASSERT_VALID (this);
        goto Done;
    }

    // if we have movement points left move it
    if ((int) m_fVehMove >= m_iSpeed)
        MoveInHex();

    ASSERT ((m_cMode != moving) || (m_ptNext != m_ptHead));

    // we're all done moving - fix spotting & tell everyone
    Done:
    ASSERT ((m_cMode != moving) || (m_ptNext != m_ptHead));
    ASSERT ((m_cMode != moving) || (theVehicleHex.GetVehicle(m_ptNext) == this));

    // we have to redo the world map for visibility
    if (!_hexHead.SameHex(m_ptHead)) {
        // if it's ours and not in a building respot
        if (DoSpotting() && (!SpottingOn()) && m_cOwn) {
            DetermineSpotting();
            IncrementSpotting(GetHexHead());
        }

        // update oppo fire
        if ((GetOwner()->IsLocal()) && (m_cOwn))
            OppoAndOthers();

        if (bVis || IsVisible())
            theApp.m_wndWorld.InvalidateWindow(CWndWorld::visible |
                                               (GetOwner()->IsMe() ? CWndWorld::my_units : CWndWorld::other_units));
    }

    // tell everyone
    AtNewLoc();

    ASSERT_VALID_LOC (this);
}

// return TRUE if moved out of hex
BOOL CVehicle::MoveInHex() {

    ASSERT (m_ptNext != m_ptHead);

    int iStep = (int) (m_fVehMove / (float) m_iSpeed);
    BOOL bDone;
    if (iStep < m_iStepsLeft) {
        if (iStep == 0)
            return (FALSE);
        m_iStepsLeft -= iStep;
        bDone = FALSE;
    } else {
        iStep = m_iStepsLeft;
        m_iStepsLeft = 0;
        bDone = TRUE;
    }
    m_fVehMove -= (float) (iStep * m_iSpeed);

    // move it
    m_maploc.x += iStep * m_iXadd;
    m_maploc.x = __roll(0, MAX_HEX_HT * theMap.Get_eX(), m_maploc.x);
    m_maploc.y += iStep * m_iYadd;
    m_maploc.y = __roll(0, MAX_HEX_HT * theMap.Get_eY(), m_maploc.y);
    m_iDir += iStep * m_iDadd;
    m_iDir = __roll(0, FULL_ROT, m_iDir);

    if (GetTurret())
        GetTurret()->m_iDir = __roll(0, FULL_ROT, GetTurret()->m_iDir + iStep * m_iTadd);

    return (bDone);
}

// We've made it to m_hexNext - what next?
void CVehicle::ArrivedNextHex() {

#ifdef _LOGOUT
    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d arrived next sub (%d,%d)", GetID(), m_ptHead.x, m_ptHead.y);
#endif

    ASSERT_VALID (this);
    ASSERT (GetOwner()->IsLocal());
#ifndef _GG
    ASSERT (m_ptNext != m_ptHead);
#endif
    ASSERT ((abs(CSubHex::Diff(m_ptNext.x - m_ptHead.x)) < 2) &&
            (abs(CSubHex::Diff(m_ptNext.y - m_ptHead.y)) < 2));
    ASSERT (m_cOwn);
    ASSERT (theVehicleHex.GetVehicle(m_ptNext) == this);
    ASSERT (theVehicleHex.GetVehicle(m_ptHead) == this);
    ASSERT (theVehicleHex.GetVehicle(m_ptTail) == this);

    // stuff to avoid looping, etc.
    if (abs(m_subOn.Dist(m_ptNext)) <= MAX_TIMES_CIRCLE / 4) {
        m_iTimesOn++;
        if (m_subOn == m_ptNext)
            m_iTimesOn += 2;
    } else {
        m_iTimesOn = 0;
        m_subOn = m_ptNext;
    }

    // turn off spotting for resetting later
    if ((GetOwner()->IsMe()) && (m_bSpotted) && (!m_ptHead.SameHex(m_ptNext)))
        DecrementSpotting();

    // if next/head not the same water/land set on_water
    //   if on a bridge we leave it alone - it's already set for what it should be
    if (!m_ptHead.SameHex(m_ptNext)) {
        CHex *pHexOn = theMap._GetHex(m_ptHead);
        if (!(pHexOn->GetUnits() & CHex::bridge)) {
            BOOL bDestWater = theMap._GetHex(m_ptNext)->IsWater();
            if (bDestWater != pHexOn->IsWater())
                SetOnWater(bDestWater);
        }
    }

    // set new values
    if (GetData()->GetVehFlags() & CTransportData::FL1hex) {
        ASSERT (m_ptHead == m_ptTail);
        if (m_ptHead != m_ptNext)
            theVehicleHex.ReleaseHex(m_ptHead, this);
        m_ptTail = m_ptHead = m_ptNext;
    } else {
        if (m_ptTail != m_ptHead)
            theVehicleHex.ReleaseHex(m_ptTail, this);
        m_ptTail = m_ptHead;
        m_ptHead = m_ptNext;
    }

    // if we're attacking and we can hit - stop
    // if a vehicle we want to be able to get them if they move 1 hex
    if ((m_iEvent == attack) && (m_pUnitTarget != NULL) && (m_pUnitTarget == m_pUnitOppo))
        if ((m_pUnitTarget->GetUnitType() != CUnit::vehicle) || (abs(m_iLOS) < GetRange()))
            m_hexDest = m_ptDest = m_ptHead;
}

// we've arrived at our destination
void CVehicle::ArrivedDest() {

#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d arrived dest sub (%d,%d)", GetID(), m_ptHead.x, m_ptHead.y);
#endif

    ASSERT ((m_ptHead == m_ptDest) || (m_iEvent == load));
#ifdef STRICTER_ASSERTS
    ASSERT ((m_cMode != moving) || (theVehicleHex.GetVehicle(m_ptNext) == this));
#endif

    // we're stopped
    _SetRouteMode(stop);

    // set vars correctly, use up remaining time
    SetMoveParams(FALSE);
    ZeroMoveParams();

    DeletePath();

    // if it's a building - disappear into it
    CBuilding *pBldgDest = theBuildingHex._GetBuilding(GetHexHead());
    if (pBldgDest != NULL)
        EnterBuilding();

    if (!GetOwner()->IsLocal())
        return;

    // tell AI/router we've arrived
    // we do NOT tell the router if it's a damaged vehicle arriving at a repair center
    if ((!GetOwner()->IsMe()) || (m_iEvent != repair_self) || (pBldgDest == NULL) ||
        ((pBldgDest->GetData()->GetUnionType() != CStructureData::UTrepair) &&
         (pBldgDest->GetData()->GetUnionType() != CStructureData::UTshipyard)))
        PostArrivedOrBlocked();
    else
        m_bFlags |= told_ai_stop;

    // may need to update window
    if (!GetOwner()->IsAI())
        theAreaList.MaterialChange(this);

    switch (m_iEvent) {
        case route : {
            if (m_route.IsEmpty())
                break;
            // handle this stop
            CRoute *pR = m_route.GetAt(m_pos);
            ASSERT_VALID (pR);
            if (pR != NULL) {
                ASSERT (pR->GetCoord().SameHex(m_ptHead));

                switch (pR->GetRouteType()) {
                    case CRoute::load :
                        Load();
                        break;
                    case CRoute::unload :
                        Unload();
                        break;
#ifdef _DEBUG
                        case CRoute::waypoint :
                            break;
                        default:
                            TRAP ();
                            break;
#endif
                }
            }

            // next dest
            if (m_pos == NULL)
                m_pos = m_route.GetHeadPosition();
            else {
                m_route.GetNext(m_pos);
                if (m_pos == NULL)
                    m_pos = m_route.GetHeadPosition();
            }

            // go on to the next dest
            if (m_pos != NULL) {
                pR = m_route.GetAt(m_pos);
                if (pR != NULL) {
                    ASSERT_VALID (pR);
                    SetRoutePos(m_pos);
                    SetDest(pR->GetCoord());
                } else {
                    m_iEvent = none;
                    if (m_cOwn)
                        SetDest(m_ptHead);
                    else
                        ExitBuilding();
                }
            }
            break;
        }

        case repair_self : {
            CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
            ASSERT_VALID (pBldg);
            if (pBldg != NULL) {
                if (pBldg->GetData()->GetBldgType() == CStructureData::repair)
                    ((CRepairBuilding *) pBldg)->RepairVehicle(this);
                else if (pBldg->GetData()->GetBldgType() == CStructureData::shipyard)
                    ((CShipyardBuilding *) pBldg)->RepairVehicle(this);
            }
            break;
        }

        case build :
            BuildBldg();
            break;
        case build_road :
            BuildRoad();
            break;

        case repair_bldg : {
            CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
            if (pBldg != NULL) {
                ASSERT_VALID (pBldg);
                StartConst(pBldg);
                break;
            }

            CBridgeUnit *pBu = theBridgeHex.GetBridge(m_ptHead);
            if (pBu != NULL) {
                CBridge *pBridge = pBu->GetParent();
                if ((pBridge != NULL) && (!pBridge->IsBuilt()))
                    SetEventAndRoute(CVehicle::build_road, CVehicle::run);
            }
            break;
        }

        case none : {
            // nothing to do if not me (AI does it's own thing)
            if (!GetOwner()->IsMe())
                return;

            // nothing to do if not a building
            CBuilding *pBldgDest = theBuildingHex._GetBuilding(m_ptHead);
            if (pBldgDest == NULL)
                break;

            if (m_bFlags & dump_contents) {
                DumpContents();
                break;
            }

            // freighter arrives at seaport - put all stopped vehicles in building that will fit on it
            if ((GetData()->IsCarrier()) && (GetData()->IsBoat()) &&
                (pBldgDest->GetData()->GetType() == CStructureData::seaport)) {
                // unload everyone we are carrying
                UnloadCarrier();

                POSITION pos = theVehicleMap.GetStartPosition();
                while (pos != NULL) {
                    DWORD dwID;
                    CVehicle *pVeh;
                    theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
                    // it's ours, it's in a building, and it's stopped (not unloading/deploying, etc)
                    if ((pVeh != this) && (pVeh->GetTransport() == NULL) && (pVeh->GetOwner()->IsMe()) &&
                        (!pVeh->GetHexOwnership()) && (pVeh->GetRouteMode() == CVehicle::stop))
                        if (pBldgDest == theBuildingHex._GetBuilding(pVeh->GetPtHead())) {
                            int iSize;
                            if (pVeh->GetData()->IsPeople())
                                iSize = 1;
                            else
                                iSize = MAX_CARGO;
                            if (m_iCargoSize + iSize > GetData()->GetPeopleCarry())
                                continue;

                            // put it on the boat
                            pVeh->SetTransport(this);

                            // see if we're full
                            if (m_iCargoSize >= GetData()->GetPeopleCarry())
                                break;
                        }
                }
                break;
            }

            // if we're controlling the truck
            if ((GetData()->IsTransport()) && ((m_bFlags & hp_controls) != 0))
                GetDlgLoad();
            break;
        }

        case load :
            // in case the carrier is dead
            if (m_pVehLoadOn == NULL) {
                TRAP();
                SetEvent(none);
                break;
            }

            // can only load if not already loaded
            if (m_pTransport != NULL) {
                TRAP();
                SetEvent(none);
                break;
            }

            CSubHex _exit;
            if (m_pVehLoadOn->GetData()->GetVehFlags() & CTransportData::FLload_front)
                _exit = m_pVehLoadOn->m_ptHead;
            else
                _exit = m_pVehLoadOn->m_ptTail;

            // it's not there
            if ((abs(CSubHex::Diff(_exit.x - m_ptHead.x)) > 1) ||
                (abs(CSubHex::Diff(_exit.y - m_ptHead.y)) > 1)) {
                SetDest(_exit);
                break;
            }

            int iAdd = GetData()->IsPeople() ? 1 : MAX_CARGO;

            // not carrier OR no room
            if ((!m_pVehLoadOn->GetData()->IsCarrier()) ||
                (m_pVehLoadOn->m_iCargoSize + iAdd > m_pVehLoadOn->GetData()->GetPeopleCarry())) {
                SetEvent(none);
                break;
            }

            // landing craft - must have room, be LC carryable
            if (m_pVehLoadOn->GetData()->IsBoat()) {
                if ((!(GetData()->GetVehFlags() & CTransportData::FLlc_carryable)) &&
                    (!GetData()->IsCarryable())) {
                    SetEvent(none);
                    break;
                }
            } else
                // infantry carrier - must be a carryable unit
            if (!(GetData()->IsCarryable())) {
                TRAP();
                SetEvent(none);
                break;
            }

            SetEvent(none);

            CMsgLoadCarrier msg(this, m_pVehLoadOn);
            theGame.PostToAll(&msg, sizeof(msg), FALSE);
            break;
    }

    if (pBldgDest != NULL) {
        pBldgDest->EventOff();
        pBldgDest->MaterialChange();
        MaterialChange();
    }
}

BOOL CVehicle::FindNextHex() {

    int iNum = 0;
    BOOL bRtn = TRUE;

    // keep getting the next one until we've used up our movement points
    do {
        // we've left one hex for the next
        if (iNum > 0) {
            m_fVehMove -= (float) (STEPS_HEX * m_iSpeed);
            ArrivedNextHex();
        }

        // have we arrived?
        if (m_ptDest == m_ptHead) {
            ZeroMoveParams();
            break;
        }

            // on to the next one
        else {
            if (!GetNextHex(iNum > 0)) {
                bRtn = FALSE;
                break;
            }

            if (m_ptDest != m_ptHead) {
                ASSERT (m_ptNext != m_ptHead);
                ASSERT (theVehicleHex.GetVehicle(m_ptNext) == NULL);

                // grab the hex
                theVehicleHex.GrabHex(m_ptNext, this);
            } else
                m_ptNext = m_ptHead;
        }

        // gas it up
        if (GetData()->GetWheelType() != CWheelTypes::walk)
            GetOwner()->FuelVehicle();

        iNum++;

        // if we've arrived we're done
        if (m_ptDest == m_ptHead)
            break;

        // see if we are done
    } while (m_fVehMove >= m_iSpeed * STEPS_HEX);

    // if blocked we are done
    if (!bRtn) {
#ifndef _GGASSERT
        #if STRICTER_ASSERTS
        ASSERT (m_cMode == blocked);
        #endif
#endif
        ZeroMoveParams();
#ifdef _LOGOUT
        logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d FindNext failed n,h,t sub (%d,%d),(%d,%d),(%d,%d)", GetID(),
                  m_ptNext.x, m_ptNext.y, m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y);
#endif
        m_ptNext = m_ptHead;
        return (FALSE);
    }

    SetMoveParams(iNum > 1);
    ASSERT (theVehicleHex.GetVehicle(m_ptNext) == this);
    return (TRUE);
}

void CVehicle::ZeroMoveParams() {

    m_iStepsLeft = m_iTadd = m_iDadd = m_iXadd = m_iYadd = 0;
    m_fVehMove = 0;
    m_lOperMod = 0;
}

// move our cargo
void CVehicle::MoveCargo(BOOL bCarried) {

    if (m_lstCargo.GetCount() > 0) {
        POSITION pos = m_lstCargo.GetHeadPosition();
        while (pos != NULL) {
            CVehicle *pVeh = m_lstCargo.GetNext(pos);
            if (pVeh == this)
                continue;

            pVeh->m_hexDest = pVeh->m_ptDest = pVeh->m_ptHead = m_ptHead;
            if (pVeh->GetData()->GetVehFlags() & CTransportData::FL1hex)
                pVeh->m_ptTail = m_ptHead;
            else
                pVeh->m_ptTail = m_ptTail;
            pVeh->m_maploc = m_maploc;
            pVeh->m_iDir = m_iDir;

            // move it's cargo
            if (!bCarried)
                pVeh->MoveCargo(TRUE);
        }
    }
}

// set the params 
void CVehicle::SetMoveParams(BOOL bFixTurret) {

    SetLoc(TRUE);

    // move our cargo
    MoveCargo(FALSE);

    // if more than 1 hex we lock the turret on the target
    if ((bFixTurret) && (GetTurret())) {
        if (m_pUnitOppo == NULL)
            GetTurret()->SetDir(m_iDir);
        else
            GetTurret()->SetDir(FastATan(CMapLoc::Diff(m_pUnitOppo->GetWorldPixels().x - m_maploc.x),
                                         CMapLoc::Diff(m_pUnitOppo->GetWorldPixels().y - m_maploc.y)));
    }

    m_iStepsLeft = STEPS_HEX;
    if (GetData()->GetVehFlags() & CTransportData::FL1hex) {
        if (m_ptNext != m_ptHead)
            m_iDir = CalcNextDir();
        m_iDadd = 0;
        m_iXadd = CSubHex::Diff(m_ptNext.x - m_ptHead.x) * 2;
        m_iYadd = CSubHex::Diff(m_ptNext.y - m_ptHead.y) * 2;
    } else {
        m_iDadd = GetAngle(m_ptNext, m_ptHead, m_ptHead, m_ptTail);
        m_iXadd = CSubHex::Diff((m_ptNext.x + m_ptHead.x) - (m_ptHead.x + m_ptTail.x));
        m_iYadd = CSubHex::Diff((m_ptNext.y + m_ptHead.y) - (m_ptHead.y + m_ptTail.y));
    }
    ASSERT ((abs(m_iXadd) <= 2) && (abs(m_iYadd) <= 2));

    // turret - on target if shooting, else with tank
    if (GetTurret()) {
        if (m_pUnitOppo == NULL)
            m_iTadd = m_iDadd;
        else {
            // get what it should be when we arrive
            CMapLoc ml(m_maploc);
            ml.x += STEPS_HEX * m_iXadd;
            ml.y += STEPS_HEX * m_iYadd;
            int iNew = FastATan(CMapLoc::Diff(m_pUnitOppo->GetWorldPixels().x - ml.x),
                                CMapLoc::Diff(m_pUnitOppo->GetWorldPixels().y - ml.y));

            int iDiff = (((iNew - GetTurret()->GetDir()) + 64) & 127) - 64;
            if (iDiff < -32)
                iDiff = -32;
            else if (iDiff > 32)
                iDiff = 32;
            if (iDiff >= 0)
                m_iTadd = (iDiff + STEPS_HEX / 2) / STEPS_HEX;
            else
                m_iTadd = (iDiff - STEPS_HEX / 2) / STEPS_HEX;
        }
    }

    DetermineSpeed(FALSE);

    ASSERT_VALID_LOC (this);
    ASSERT ((!m_cOwn) || (theVehicleHex.GetVehicle(m_ptHead) == this));
    ASSERT ((!m_cOwn) || (theVehicleHex.GetVehicle(m_ptTail) == this));
}

// this gets the next hex for the vehicle - returns TRUE if it could do it
BOOL CVehicle::GetNextHex(BOOL bNew) {

    ASSERT_VALID (this);
    ASSERT (GetOwner()->IsLocal());
    ASSERT (m_cMode == moving);

    SetLoc(bNew);

    // if we are circling lets force it out
    if ((m_iTimesOn > MAX_TIMES_CIRCLE) && (MyRand() & 0x0100)) {
        if (GetData()->GetVehFlags() & CTransportData::FL1hex)
            m_ptNext = Rotate(RandNum(7) - 4);
        else
            m_ptNext = Rotate(RandNum(4) - 2);
#ifdef _LOGOUT
        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d circled 10 times", GetID());
#endif
        GetPath(TRUE);
        MakeBlocked();
        m_iTimesOn = MAX_TIMES_CIRCLE / 2;
        return (FALSE);
    }

    // now we need to find the next sub-hex to go to
    //   step to next - if > 1 then we have another sub hex to go in this hex
    int xStep, yStep;
    int iNumTries = 0;

    fig_step:
    int xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptHead.x);
    int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptHead.y);
    xStep = (xDif >= 0) ? xDif : xDif + 1;
    yStep = (yDif >= 0) ? yDif : yDif + 1;

    // if we have been heading into the last subhex - then we're done
    if ((m_bFlags & at_end_of_path) && ((abs(xStep) <= 1) && (abs(yStep) <= 1))) {
        m_bFlags &= ~at_end_of_path;
        _SetRouteMode(stop);
        PostArrivedOrBlocked();
        return (FALSE);
    }

    // if on a diag wait until we are able to shoot into m_hexNext
    BOOL bCheckStreet;
    if ((abs(xStep) > 1) || (abs(yStep) > 1)) {
        bCheckStreet = FALSE;

        if (abs(xStep) > abs(yStep))
            yStep = 0;
        else if (abs(xStep) < abs(yStep))
            xStep = 0;
    } else
        bCheckStreet = TRUE;

    // check how close to dest
    BOOL bAtDest = FALSE;
    int xDest = CSubHex::Diff(m_ptDest.x - m_ptHead.x);
    int yDest = CSubHex::Diff(m_ptDest.y - m_ptHead.y);

    // if we're withing 3 of dest no more right side of the street
    if ((abs(xDest) <= 3) && (abs(yDest) <= 3))
        bCheckStreet = FALSE;

    // see if we're about to hit the dest
    if ((abs(xDest) <= 1) && (abs(yDest) <= 1)) {
        bAtDest = TRUE;

        // going to load on a carrier
        if (m_iEvent == load) {
            m_hexDest = m_ptDest = m_ptNext = m_ptHead;
            return (TRUE);
        }

        // going to enter a building
        CBuilding *pBldgDest = theBuildingHex._GetBuilding(m_hexDest);
        if (pBldgDest != NULL) {
            // can't enter another's building
            if (!CanEnterBldg(pBldgDest)) {
                m_hexDest = m_ptDest = m_ptNext = m_ptHead;
                return (TRUE);
            } else

                // if we're not in, check the angle
            if (theBuildingHex._GetBuilding(m_ptHead) != pBldgDest)
                if (!GetData()->CanEnterHex(m_ptHead, m_ptDest, IsOnWater(), TRUE))
                    bAtDest = FALSE;
        }

        if (bAtDest) {
            xStep = xDest;
            yStep = yDest;
        }
    }

    // not at dest yet
    if (!bAtDest) {
        // are we stopped/blocked?
        // if we're at the end (including no new path) see if we've gone as far as we can go
        if ((xStep == 0) && (yStep == 0)) {
            CHexCoord _hexHead(m_ptHead);

            if (HavePathOrNext())
                PathNextHex();
            else {
                GetPath(FALSE);

                // if the new end location is no closer - we're done
                if ((m_phexPath != NULL) && (m_iPathLen > 0)) {
                    CHexCoord *pHexEnd = m_phexPath + m_iPathLen - 1;
                    if (abs(CHexCoord::Diff(_hexHead.X() - m_hexDest.X())) +
                        abs(CHexCoord::Diff(_hexHead.Y() - m_hexDest.Y())) <=
                        abs(CHexCoord::Diff(pHexEnd->X() - m_hexDest.X())) +
                        abs(CHexCoord::Diff(pHexEnd->Y() - m_hexDest.Y())))
                        goto SetNext;
                }
            }

            // see if the next location is impassible
            if ((m_iPathOff >= m_iPathLen) || (m_phexPath == NULL)) {
                if ((m_phexPath == NULL) && (m_hexDest != m_hexNext)) {
                    SetNext:
                    int iDif = CHexCoord::Diff(m_hexDest.X() - _hexHead.X());
                    m_hexNext.X() = (iDif > 0) ? _hexHead.X() + 1 : ((iDif < 0) ? _hexHead.X() - 1 : _hexHead.X());
                    iDif = CHexCoord::Diff(m_hexDest.Y() - _hexHead.Y());
                    m_hexNext.Y() = (iDif > 0) ? _hexHead.Y() + 1 : ((iDif < 0) ? _hexHead.Y() - 1 : _hexHead.Y());
                    m_hexNext.Wrap();
                }

                // if next is impassabile we're done - maybe
                if (!GetData()->CanEnterHex(m_ptHead, m_hexNext, IsOnWater(), TRUE)) {
                    // we need to move as close to next as we can
                    int xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptHead.x);
                    int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptHead.y);
                    xStep = (xDif >= 0) ? xDif : xDif + 1;
                    yStep = (yDif >= 0) ? yDif : yDif + 1;
                    if ((xStep != 0) || (yStep != 0)) {
                        m_bFlags |= at_end_of_path;
                        if (++iNumTries < 2)
                            goto fig_step;
                    }

                    // if we already tried twice then lets just stop
                    _SetRouteMode(stop);
                    PostArrivedOrBlocked();
                    return (FALSE);
                }

                // if no path - we're blocked
                if (m_iPathLen <= 0) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d out of path", GetID());
#endif
                    MakeBlocked();
                    return (FALSE);
                }

                // we don't want to do this twice so this better be good
                if (m_hexNext.SameHex(m_ptHead))
                    iNumTries = 2;
            }

            if (++iNumTries < 2)
                goto fig_step;

            // don't know why but it can't get the path
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d is at next", GetID());
#endif
            MakeBlocked();
            return (FALSE);
        }

#ifdef BUGBUG
        // handle cornering on roads
        if ((xStep != 0) && (yStep != 0))
            if ((theMap._GetHex (m_ptHead)->GetType () == CHex::road) &&
                                                (theMap._GetHex (m_hexNext)->GetType () == CHex::road))
                {
                CSubHex _test = Rotate (0);
                if (theMap._GetHex (_test)->GetType () == CHex::road)
                    {
                    xStep = _test.x - m_ptHead.x;
                    yStep = _test.y - m_ptHead.y;
                    bCheckStreet = FALSE;
                    }
                }
#endif

        // handle wrong lane
        if (bCheckStreet) {
            if (xStep == 0) {
                if ((m_ptHead.x & 1) && (yStep > 0))
                    xStep = -1;
                else if ((!(m_ptHead.x & 1)) && (yStep < 0))
                    xStep = 1;
            }
            if (yStep == 0) {
                if ((m_ptHead.y & 1) && (xStep < 0))
                    yStep = -1;
                else if ((!(m_ptHead.y & 1)) && (xStep > 0))
                    yStep = 1;
            }
        }
    }    // end not bAtDest

    // we may be REAL off - get a new path
    int iTmp = abs(xStep) + abs(yStep);
    if (((iTmp == 0) && (!bAtDest)) || (iTmp >= 5)) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d step = %d", GetID(), iTmp);
#endif
        MakeBlocked();
        return (FALSE);
    }

    // 1 sub-hex at a time
    xStep = (xStep < -1) ? -1 : (xStep > 1 ? 1 : xStep);
    yStep = (yStep < -1) ? -1 : (yStep > 1 ? 1 : yStep);
    ASSERT ((bAtDest) || (xStep != 0) || (yStep != 0));

    // figure out the next pt
    m_ptNext.x = m_ptHead.x + xStep;
    m_ptNext.y = m_ptHead.y + yStep;
    m_ptNext.Wrap();

    // check angle
    // can't rotate 180 degrees - want to U-turn
    if (!(GetData()->GetVehFlags() & CTransportData::FL1hex)) {
        int iDir;
        if (m_ptNext == m_ptTail)
            iDir = MyRand() & 0x1000 ? -4 : 4;
        else
            iDir = GetAngle(m_ptNext, m_ptHead, m_ptHead, m_ptTail);
        int iMax = GetData()->IsBoat() ? 3 : 2;
        if (iDir > iMax)
            m_ptNext = Rotate(iMax);
        else if (iDir < -iMax)
            m_ptNext = Rotate(-iMax);
    }

    // stop X, cutting a building
    if ((m_ptNext.x != m_ptHead.x) && (m_ptNext.y != m_ptHead.y)) {
        CUnit *pBlk;
        CSubHex _sub(m_ptNext.x, m_ptHead.y);
        if ((!bAtDest) && (theBuildingHex._GetBuilding(_sub) != NULL))
            goto BadNews;
        pBlk = theVehicleHex._GetVehicle(_sub);
        if (pBlk != NULL) {
            _sub = CSubHex(m_ptHead.x, m_ptNext.y);
            if ((pBlk == theVehicleHex._GetVehicle(_sub)) ||
                ((!bAtDest) && (theBuildingHex._GetBuilding(_sub) != NULL))) {
                BadNews:

                // try closest angle
                if (GetData()->GetVehFlags() & CTransportData::FL1hex)
                    m_ptNext = Rotate(0x07 & (m_iDir / EIGHTH_ROT + ((MyRand() & 0x1000) ? -1 : 1)));
                else {
                    int iDir = GetAngle(m_ptNext, m_ptHead, m_ptHead, m_ptTail);
                    if (iDir <= -2)
                        m_ptNext = Rotate(-1);
                    else if (iDir >= 2)
                        m_ptNext = Rotate(1);
                    else
                        m_ptNext = Rotate(iDir + ((MyRand() & 0x1000) ? -1 : 1));
                }
            }
        }
    }

    // if outside hex on and hex next test speed (ie don't go into mountains)
    if (!(m_ptNext.SameHex(m_ptHead)) && !(m_ptNext.SameHex(m_hexNext))) {
        CHexCoord _hex = m_ptNext.ToCoord();
        CHexCoord _hexHead(GetHexHead());
        int iSpeedOn = theMap.GetTerrainCost(_hexHead, _hexHead, CalcBaseDir(), GetData()->GetWheelType());
        int iSpeedNext = theMap.GetTerrainCost(_hexHead, _hex, CalcNextBaseDir(), GetData()->GetWheelType());
        if ((iSpeedOn != 0) && ((iSpeedNext > iSpeedOn * 4) || (iSpeedNext == 0))) {

            // ok - lets see if another direction is better
            CSubHex _old(m_ptNext);
            FindSub();
            if (m_ptNext != _old) {
                // if not faster switch it back
                int iSpeedNew = theMap.GetTerrainCost(_hexHead, m_ptNext, CalcNextBaseDir(), GetData()->GetWheelType());
                if ((iSpeedNext != 0) && ((iSpeedNew == 0) || (iSpeedNew > iSpeedNext)))
                    m_ptNext = _old;
            }
        }
    }

    // can we enter it?
    if (!CanEnter(m_ptNext)) {
        // try to find a new one
        if (!FindSub()) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d CanEnter/FindSub failed", GetID());
#endif
            MakeBlocked();
            return (FALSE);
        }

        // If we were circling
        // we only insist on closer if we are not on or going to a building/bridge
        if (m_iTimesOn >= MAX_TIMES_CIRCLE / 2)
            if ((!(theMap._GetHex(m_ptNext)->GetUnits() & (CHex::bldg | CHex::bridge))) &&
                (!(theMap._GetHex(m_ptHead)->GetUnits() & (CHex::bldg | CHex::bridge)))) {
                // check the new sub-hex distance - have to get closer
                int xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptNext.x);
                int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptNext.y);
                int iNew = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));
                xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptHead.x);
                yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptHead.y);
                int iOld = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));
                if ((iOld <= iNew) && (iNew != 0)) {
                    m_ptNext = m_ptHead;
                    if (!FindSub(TRUE)) {
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d moving away from next", GetID());
#endif
                        MakeBlocked();
                        return (FALSE);
                    }
                }
            }
    }

    CheckNextHex();
    return (TRUE);
}

void CVehicle::CheckNextHex() {

    SetHexDest();

    ASSERT_VALID (this);
    ASSERT (m_ptNext != m_ptHead);
    ASSERT ((abs(CSubHex::Diff(m_ptNext.x - m_ptHead.x)) < 2) &&
            (abs(CSubHex::Diff(m_ptNext.y - m_ptHead.y)) < 2));

    DetermineSpeed(TRUE);

    // make sure it's free
    // we grab it outside of here - this just says we CAN take this one
    ASSERT (theVehicleHex.GetVehicle(m_ptNext) == NULL);
}

void CVehicle::SetHexDest() {

    // if we're at the dest hex its time to figure out the final position
    CHexCoord _next(m_ptNext);
    if ((abs(CHexCoord::Diff(_next.X() - m_hexDest.X())) <= 1) &&
        (abs(CHexCoord::Diff(_next.Y() - m_hexDest.Y())) <= 1)) {
        switch (m_iDestMode) {
            // as long as we are in the hex
            case any :
                TRAP();
                // if this puts us in the hex - we're done
                if (m_hexDest.SameHex(m_ptNext)) {
                    TRAP();
                    m_ptDest = m_ptNext;
                }
                break;

                // set ptDest so we go fully in
            case center :
                // if the tail will end up in - we're done
                if ((m_hexDest.SameHex(m_ptHead)) && (m_hexDest.SameHex(m_ptNext))) {
                    m_ptDest = m_ptNext;
                    break;
                }
                // if next in line, make it the furthest away one
                if (m_ptNext.x / 2 == m_hexDest.X())
                    m_ptDest.x = (m_ptNext.x & 1) ? m_ptNext.x - 1 : m_ptNext.x + 1;
                if (m_ptNext.y / 2 == m_hexDest.Y())
                    m_ptDest.y = (m_ptNext.y & 1) ? m_ptNext.y - 1 : m_ptNext.y + 1;
                break;

            case full :
                // if the tail will end up in - we're done
                if ((m_hexDest.SameHex(m_ptHead)) && (m_hexDest.SameHex(m_ptNext))) {
                    m_ptDest = m_ptNext;
                    break;
                }
                m_ptDest.x = m_ptNext.x + (m_ptNext.x - m_ptHead.x);
                m_ptDest.y = m_ptNext.y + (m_ptNext.y - m_ptHead.y);
                m_ptDest.Wrap();
                break;

#ifdef _DEBUG
                case sub :
                    break;
                default:
                    ASSERT (FALSE);
                    break;
#endif
        }

        m_ptDest.x = __minmax(m_hexDest.X() * 2, m_hexDest.X() * 2 + 1, m_ptDest.x);
        m_ptDest.y = __minmax(m_hexDest.Y() * 2, m_hexDest.Y() * 2 + 1, m_ptDest.y);
        m_ptDest.Wrap();
    }

    ASSERT (m_hexDest.SameHex(m_ptDest));
}

// look for the next m_ptNext
//   return TRUE if found
// bCloser - new location must be closer
BOOL CVehicle::FindSub(BOOL bCloser) {

    // save off the old m_ptNext so we don't pick it again
    CSubHex shOldNext(m_ptNext);

    BOOL bRtn = FALSE;
    BOOL bOk = FALSE;

    CHexCoord _hexHead(GetHexHead());
    CHexCoord _hexTmp(m_ptNext.ToCoord());
    int iBestSpeed = theMap.GetTerrainCost(_hexTmp, _hexHead,
                                           CalcNextBaseDir(), GetData()->GetWheelType()) * 2;
    if (iBestSpeed <= 0)
        iBestSpeed = SLOWEST_SPEED;

    // we don't want to turn back into the building - but we may have it as our dest
    CBuilding *pBldgOn = theBuildingHex._GetBuilding(_hexHead);
    if (theBuildingHex._GetBuilding(m_ptDest) == pBldgOn)
        pBldgOn = NULL;

    int xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptHead.x);
    int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptHead.y);
    int iOldDist = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));

    // lets walk every possibility
    int iDir, iMax;
    if (GetData()->GetVehFlags() & CTransportData::FL1hex) {
        iDir = -4;
        iMax = 3;
    } else if (GetData()->IsBoat()) {
        iDir = -3;
        iMax = 3;
    } else {
        iDir = -2;
        iMax = 2;
    }

    int iNumTries = 1;
    for (; iDir <= iMax; iDir++) {
        CSubHex _next;
        if (GetData()->GetVehFlags() & CTransportData::FL1hex)
            _next = ::Rotate(iDir, m_ptHead, m_ptHead);
        else
            _next = Rotate(iDir);

        // don't even consider the last m_ptNext at all
        if (_next == shOldNext)
            continue;

        // must be closer to m_hexNext
        if (bCloser) {
            int xDif = CSubHex::Diff(m_hexNext.X() * 2 - _next.x);
            int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - _next.y);
            int iNewDist = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));
            if (iOldDist <= iNewDist)
                continue;
        }

        // must be able to enter
        if (!CanEnter(_next))
            continue;

        // test for going back into building
        if (pBldgOn != NULL)
            if (theBuildingHex._GetBuilding(_next) == pBldgOn)
                continue;

        // test for X
        if ((_next.x != m_ptHead.x) && (_next.y != m_ptHead.y)) {
            CSubHex _sub(_next.x, m_ptHead.y);
            CUnit *pBlk = theVehicleHex._GetVehicle(_sub);
            if (pBlk != NULL) {
                CSubHex _sub(m_ptHead.x, _next.y);
                if (pBlk == theVehicleHex._GetVehicle(_sub))
                    continue;
            }
        }

        CHexCoord _hexNext(_next.ToCoord());
        int iSpeed = theMap.GetTerrainCost(_hexNext, _hexHead, CalcNextBaseDir(), GetData()->GetWheelType());
        if (iSpeed == 0)
            continue;

        // if we have nothing we aren't too picky - mostly can it travel at all
        if (!bOk) {
            if (iSpeed < iBestSpeed * 8) {
                iBestSpeed = iSpeed;
                m_ptNext = _next;
                m_iDadd = iDir;
                bOk = TRUE;
                bRtn = TRUE;
            }
        }

            // ok, we now see if we try a different path. This goes on random numbers
        else {
            int xDif = CSubHex::Diff(m_hexNext.X() * 2 - _next.x);
            int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - _next.y);
            int iNew = (((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1))) * iSpeed;

            // weight to find the new dest
            if (RandNum(iNew * iNumTries) <= RandNum(iOldDist)) {
                iBestSpeed = iSpeed;
                m_ptNext = _next;
                m_iDadd = iDir;
                bRtn = TRUE;
            }
            iNumTries++;
        }
    }

#ifdef _DEBUG
    if (bRtn)
        {
        ASSERT_VALID (this);
        ASSERT (m_ptNext != m_ptHead);
        ASSERT ((abs (CSubHex::Diff (m_ptNext.x - m_ptHead.x)) < 2) &&
                                                (abs (CSubHex::Diff (m_ptNext.y - m_ptHead.y)) < 2));
        }
#endif

    return (bRtn);
}

BOOL CVehicle::CanEnterBldg(CBuilding *pBldg) const {

    // if no building we can enter
    if (pBldg == NULL)
        return (TRUE);

    // trucks can enter any building
    if (GetData()->IsTransport())
        return (TRUE);

    // must be ours
    if (pBldg->GetOwner() != GetOwner())
        return (FALSE);

    // cranes can always enter
    if (GetData()->IsCrane())
        return (TRUE);

    // not if under construction (cranes/trucks allowed above)
    if (pBldg->IsConstructing())
        return (FALSE);

    // must have an exit
    if ((!GetData()->IsBoat()) && (!pBldg->GetData()->HasVehExit()))
        return (FALSE);
    if ((GetData()->IsBoat()) && (!pBldg->GetData()->HasShipExit()))
        return (FALSE);

    // can never enter if attacking
    if (m_pUnitTarget == pBldg)
        return (FALSE);

    // anything can enter a seaport
    if (pBldg->GetData()->GetBldgType() == CStructureData::seaport)
        return (TRUE);

    // if damaged can enter a repair center
    if (pBldg->GetData()->GetBldgType() == CStructureData::repair)
        if (!GetData()->IsBoat())
            if (GetDamagePer() < 100)
                return (TRUE);
    if (pBldg->GetData()->GetBldgType() == CStructureData::shipyard)
        if (GetData()->IsBoat())
            if (GetDamagePer() < 100)
                return (TRUE);

    // otherwise NO
    return (FALSE);
}

// return TRUE if is passable (ie CanEnter is TRUE if no vehicle there)
BOOL CVehicle::IsPassable(CSubHex const &_sub, BOOL bStrict) {

    // dest building must be ours
    if (!CanEnterBldg(theBuildingHex._GetBuilding(_sub)))
        return (FALSE);

    return (GetData()->CanEnterHex(m_ptHead, _sub, IsOnWater(), bStrict));
}

// return TRUE if can enter sub-hex
BOOL CVehicle::CanEnter(CSubHex const &_sub, BOOL bStrict) {

    // if vehicle there then NO
    if (theVehicleHex._GetVehicle(_sub) != NULL)
        return (FALSE);

    return (IsPassable(_sub, bStrict));
}

// set the ptLoc based on the hexes so if we got off its fixed each hex
#ifdef _DEBUG
void CVehicle::SetLoc (BOOL bNew)
#else

void CVehicle::SetLoc(BOOL)
#endif
{

#ifdef _DEBUG
    ASSERT_VALID (this);
    int x = m_maploc.x;
    int y = m_maploc.y;
    int d = m_iDir;
#endif

    int iDir = m_iDir;

    if (abs(m_ptHead.x - m_ptTail.x) <= 2)
        m_maploc.x = (m_ptHead.x + m_ptTail.x) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
    else {
        m_maploc.x = (m_ptHead.x + m_ptTail.x + theMap.Get_eX() * 2) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
        if (m_maploc.x >= theMap.Get_eX() * MAX_HEX_HT)
            m_maploc.x -= theMap.Get_eX() * MAX_HEX_HT;
    }

    if (abs(m_ptHead.y - m_ptTail.y) <= 2)
        m_maploc.y = (m_ptHead.y + m_ptTail.y) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
    else {
        m_maploc.y = (m_ptHead.y + m_ptTail.y + theMap.Get_eY() * 2) * MAX_HEX_HT / 4 + MAX_HEX_HT / 4;
        if (m_maploc.y >= theMap.Get_eY() * MAX_HEX_HT)
            m_maploc.y -= theMap.Get_eY() * MAX_HEX_HT;
    }

    if (GetData()->GetVehFlags() & CTransportData::FL1hex) {
        if (m_ptNext != m_ptHead)
            m_iDir = CalcNextDir();
    } else
        m_iDir = CalcDir();

    // turret - on target if shooting, else with tank
    if (GetTurret()) {
        if (m_pUnitOppo == NULL) {
            int iTmp = GetTurret()->m_iDir + (m_iDir - iDir);
            GetTurret()->m_iDir = __roll(0, FULL_ROT, iTmp);
        }
#ifdef BUGBUG    // may still be swinging over
        else
            {
            // follow the target
            GetTurret()->m_iDir = FastATan (CMapLoc::Diff (m_pUnitOppo->GetWorldPixels().x - m_maploc.x),
                                                CMapLoc::Diff (m_pUnitOppo->GetWorldPixels().y - m_maploc.y));
            GetTurret()->m_iDir = __roll (0, FULL_ROT, GetTurret()->m_iDir);
            }
#endif
    }

#ifdef _DEBUG
    if ( !bNew )
    {
#ifdef STRICTER_ASSERTS
        ASSERT( x == m_maploc.x );
        ASSERT( y == m_maploc.y );
#endif

#ifdef STRICTER_ASSERTS
        if ( !( GetData( )->GetVehFlags( ) & CTransportData::FL1hex ) )
            ASSERT( d == m_iDir );
#endif
    }
        
#endif
}

void CVehicle::GetExitLoc(CBuilding const *pBldg, int iType, CSubHex &subNext, CSubHex &subHead, CSubHex &subTail) {
    const int aiAdd[4][4] = {1, 0, 1, 1,
                             1, 1, 0, 1,
                             0, 1, 0, 0,
                             0, 0, 1, 0};

    CTransportData const *pData = theTransports.GetData(iType);
    ASSERT_VALID (pData);

    if (pData->IsBoat()) {
        ASSERT ((0 <= pBldg->GetShipDir()) && (pBldg->GetShipDir() < 4));
        subHead.x = pBldg->GetShipHex().X() * 2 + aiAdd[pBldg->GetShipDir()][0];
        subHead.y = pBldg->GetShipHex().Y() * 2 + aiAdd[pBldg->GetShipDir()][1];
        subTail.x = pBldg->GetShipHex().X() * 2 + aiAdd[pBldg->GetShipDir()][2];
        subTail.y = pBldg->GetShipHex().Y() * 2 + aiAdd[pBldg->GetShipDir()][3];
    } else {
        ASSERT ((0 <= pBldg->GetExitDir()) && (pBldg->GetExitDir() < 4));
        subHead.x = pBldg->GetExitHex().X() * 2 + aiAdd[pBldg->GetExitDir()][0];
        subHead.y = pBldg->GetExitHex().Y() * 2 + aiAdd[pBldg->GetExitDir()][1];
        subTail.x = pBldg->GetExitHex().X() * 2 + aiAdd[pBldg->GetExitDir()][2];
        subTail.y = pBldg->GetExitHex().Y() * 2 + aiAdd[pBldg->GetExitDir()][3];
    }

    subNext = ::Rotate(0, subHead, subTail);

    if (pData->GetVehFlags() & CTransportData::FL1hex)
        subTail = subHead;

    ASSERT (theBuildingHex.GetBuilding(subNext) == NULL);
}

void CVehicle::EnterBuilding() {

    ASSERT_VALID (this);
    ASSERT ((!m_cOwn) || (theVehicleHex.GetVehicle(m_ptHead) == this));
    ASSERT ((!m_cOwn) || (theVehicleHex.GetVehicle(m_ptTail) == this));
#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) enters building", GetID(), m_ptHead.x,
              m_ptHead.y);
#endif

    ReleaseOwnership();

    // put us in the exit slot
    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
    ASSERT (pBldg != NULL);
    if (pBldg != NULL) {
        TRAP((pBldg->GetOwner() != GetOwner()) && (GetOwner()->GetRelations() != RELATIONS_ALLIANCE));

        GetExitLoc(pBldg, GetData()->GetType(), m_ptNext, m_ptHead, m_ptTail);
        CheckExit();

        m_hexNext = m_ptNext;

        // mainly crane/truck
        pBldg->MaterialChange();
    }

    // if we are in the dest, set it == for other checks
    if (theBuildingHex._GetBuilding(m_hexDest) == pBldg) {
        m_ptDest = m_ptHead;
        m_hexDest = m_ptHead;
        DeletePath();
    }

    MaterialChange();

    SetLoc(TRUE);
}

void CVehicle::ExitBuilding() {

    ASSERT_VALID (this);
#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) leaves building", GetID(), m_ptHead.x,
              m_ptHead.y);
#endif

    // building under construction
    m_pBldg = NULL;

    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
    if (pBldg != NULL) {
        // leaving a building
        pBldg->VehicleLeaving(this);

        if (!m_cOwn) {
            GetExitLoc(pBldg, GetData()->GetType(), m_ptNext, m_ptHead, m_ptTail);
            CheckExit();
            m_hexNext = m_ptNext;
        }

        // mainly crane/truck
        pBldg->MaterialChange();
    }
    SetLoc(TRUE);

    // if we have no dest kick it out to 3 away from building
    if ((pBldg != NULL) && (theBuildingHex._GetBuilding(m_hexDest) == pBldg)) {
        m_hexDest = pBldg->GetExitDest(GetData(), GetData()->GetType() != CTransportData::construction);

        m_ptDest.x = m_hexDest.X() * 2;
        m_ptDest.y = m_hexDest.Y() * 2;
        m_iDestMode = sub;
    }

    // get it going
    if (!m_cOwn)
        _SetRouteMode(cant_deploy);
    else
        _SetRouteMode(moving);

    // get a new path if needed
    if ((!HavePath()) || (*(m_phexPath + m_iPathLen - 1) != m_hexDest))
        GetPath(FALSE);
#ifdef _DEBUG
        else
            TRAP ();
#endif

    ASSERT_VALID (this);
    ASSERT ((abs(CSubHex::Diff(m_ptNext.x - m_ptHead.x)) < 2) &&
            (abs(CSubHex::Diff(m_ptNext.y - m_ptHead.y)) < 2));
}

void CVehicle::DetermineSpeed(BOOL bEvent) {

    CHexCoord _hex(GetHexHead());
    CHexCoord _next(m_ptNext);
    m_iSpeed = theMap.GetTerrainCost(_hex, _next, CalcNextBaseDir(), GetData()->GetWheelType());

    // for a crane building a road or bridge we goose it up
    if (((m_iSpeed == 0) || (m_iSpeed > 32)) && (GetData()->GetType() == CTransportData::construction) &&
        ((GetEvent() == build_road) || (m_hexDest.SameHex(m_ptNext))))
        m_iSpeed = 32;

    // out of gas -> drop to 1/4 speed for trucks, 1/16 for all else
    // NONE for walking
    if ((GetData()->GetWheelType() != CWheelTypes::walk) && (GetOwner()->GetGasHave() <= 0)) {
        if (GetData()->GetBaseType() == CTransportData::non_combat)
            m_iSpeed *= 2;
        else
            m_iSpeed *= 8;
    }

    // we now increase it for damaged vehicles
    if (GetDamagePer() < 80)
        m_iSpeed += m_iSpeed - (int) (GetDamageMult() * (float) m_iSpeed);

    if ((m_iSpeed <= 0) || (m_iSpeed >= SLOWEST_SPEED)) {
        // step 1 - if it's going to an ok hex we speed it up because it's probably landing craft stuff
        if (_hex != _next) {
            int iSpeed = theMap.GetTerrainCost(_next, _next, CalcNextBaseDir(), GetData()->GetWheelType());
            if ((iSpeed > 0) && (iSpeed < SLOWEST_SPEED / 2)) {
                m_iSpeed = iSpeed * 2;
                return;
            }
        }

#ifndef _GG
       // ASSERT (FALSE);
#endif
        m_iSpeed = SLOWEST_SPEED; // very slow but can escape eventually
        if (bEvent)
            theGame.Event(EVENT_GOTO_CANT, EVENT_WARN, this);
    }
}

void CVehicle::UnloadCarrier() {
    const int aiDir[] = {0, -1, 1, -2, 2, -3, 3};

    ASSERT_VALID (this);
#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Unload vehicle %d at sub (%d,%d)", GetID(), m_ptHead.x, m_ptHead.y);
#endif

    // if we are in a building we just put everything in the cant_deploy queue
    // Note: must be our building
    CBuilding *pBldgHead = theBuildingHex._GetBuilding(m_ptHead);
    if (pBldgHead != NULL) {
        if (pBldgHead->GetOwner() != GetOwner()) {
            TRAP();
            return;
        }

        // lets dump them out
        while (m_lstCargo.GetCount() > 0) {
            CVehicle *pVehOn = m_lstCargo.RemoveHead();
            pVehOn->m_pTransport = NULL;
            pVehOn->m_ptNext = pVehOn->m_ptHead = pVehOn->m_ptTail = m_ptHead;
            pVehOn->EnterBuilding();
            pVehOn->AtNewLoc();

            if (pVehOn->GetData()->IsPeople())
                m_iCargoSize -= 1;
            else
                m_iCargoSize -= MAX_CARGO;
#ifdef _LOGOUT
            logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Disgorged vehicle %d into bldg %d", pVehOn->GetID(),
                      pBldgHead->GetID());
#endif

            // send out of building
            pVehOn->SetDest(pBldgHead->GetExitDest(pVehOn->GetData(), FALSE));
        }

        if (!GetOwner()->IsAI())
            theAreaList.MaterialChange(this);
        return;
    }

    CSubHex _carHead, _carTail;
    if (GetData()->GetVehFlags() & CTransportData::FLload_front) {
        _carHead = m_ptTail;
        _carTail = m_ptHead;
    } else {
        _carHead = m_ptHead;
        _carTail = m_ptTail;
    }

    int iOn = 0;
    while (m_lstCargo.GetCount() > 0) {
        CSubHex _dest, _head, _sub;

        // find an exit+1 sub
        do {

            // find an exit sub
            do {
                if (iOn > 6)
                    return;

                _sub = ::Rotate(aiDir[iOn], _carTail, _carHead);

                iOn++;
            } while ((theVehicleHex._GetVehicle(_sub) != NULL) || (theBuildingHex._GetBuilding(_sub) != NULL));

            // find the location for the head (this must be traversable)
            int iHead = 0;
            do {
                _head = ::Rotate(aiDir[iHead], _sub, _carTail);
                iHead++;
            } while ((iHead <= 4) && ((theVehicleHex._GetVehicle(_head) != NULL) ||
                                      (theBuildingHex._GetBuilding(_sub) != NULL)));

            // if dest didn't work we go to the next sub
            _dest = _head;
            if (iHead > 4) {
                TRAP();
                continue;
            }

            // must have land within 1 sub (for infantry/outrider this becomes
            //   2 since _head is actually next)
            if (theMap._GetHex(_dest)->IsWater()) {
                _dest.x--;
                _dest.y--;
                _dest.Wrap();
                for (int x = 0; x < 3; x++) {
                    for (int y = 0; y < 3; y++) {
                        if (!theMap._GetHex(_dest)->IsWater())
                            goto GotIt;
                        _dest.y++;
                        _dest.Wrap();
                    }
                    _dest.x++;
                    _dest.y -= 3;
                    _dest.Wrap();
                }

                // if we're here there was no exit location
            }
        } while (theMap._GetHex(_dest)->IsWater());

        GotIt:
        CVehicle *pVehOn = m_lstCargo.RemoveHead();
        pVehOn->m_pTransport = NULL;
        if (pVehOn->GetData()->GetVehFlags() & CTransportData::FL1hex) {
            pVehOn->m_ptHead = pVehOn->m_ptTail = _sub;
            pVehOn->m_ptNext = _dest;
        } else {
            pVehOn->m_ptTail = _sub;
            pVehOn->m_ptNext = pVehOn->m_ptHead = _head;
        }
        pVehOn->m_hexDest = pVehOn->m_ptDest = _dest;

        pVehOn->SetLoc(TRUE);
        pVehOn->AtNewLoc();

        if (GetTurret())
            pVehOn->GetTurret()->SetDir(pVehOn->m_iDir);

        pVehOn->TakeOwnership();

        if (pVehOn->GetData()->IsPeople())
            m_iCargoSize -= 1;
        else
            m_iCargoSize -= MAX_CARGO;

        // set the speed of unloading
        pVehOn->DetermineSpeed(FALSE);
        pVehOn->m_iSpeed = __min (12, pVehOn->m_iSpeed);

#ifdef _LOGOUT
        logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Disgorged vehicle %d at sub (%d,%d)", pVehOn->GetID(),
                  pVehOn->m_ptHead.x, pVehOn->m_ptHead.y);
#endif

        pVehOn->SetDest(pVehOn->m_ptDest);
    }

    if (!GetOwner()->IsAI())
        theAreaList.MaterialChange(this);
}


int FastATan(int x, int y) {

    int absX = abs(x);
    int absY = abs(y);

    if (x == 0) {
        if (y >= 0)
            return (FULL_ROT / 2);
        return (0);
    }

    if (y == 0) {
        if (x >= 0)
            return (FULL_ROT / 4);
        return ((3 * FULL_ROT) / 4);
    }

    if (x > 0) {
        if (y < 0) {
            if (absY > absX)
                return (((absX * FULL_ROT) / absY) / 8);
            else
                return ((2 * FULL_ROT - (absY * FULL_ROT) / absX) / 8);
        } else {
            if (absX > absY)
                return ((2 * FULL_ROT + (absY * FULL_ROT) / absX) / 8);
            else
                return ((4 * FULL_ROT - (absX * FULL_ROT) / absY) / 8);
        }
    } else {
        if (y > 0) {
            if (absY > absX)
                return ((4 * FULL_ROT + (absX * FULL_ROT) / absY) / 8);
            else
                return ((6 * FULL_ROT - (absY * FULL_ROT) / absX) / 8);
        } else {
            if (absX > absY)
                return ((6 * FULL_ROT + (absY * FULL_ROT) / absX) / 8);
            else
                return ((8 * FULL_ROT - (absX * FULL_ROT) / absY) / 8);
        }
    }
}

void CVehicle::MakeBlocked() {

    // if we're already blocked - don't reset
    if (m_cMode == blocked)
        return;

#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub(n,h,t); {(%d,%d),(%d,%d),(%d,%d)} marked blocked",
              GetID(),
              m_ptNext.x, m_ptNext.y, m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y);
    CUnit *pUnit = ::GetUnit(m_ptNext);
    int ID = pUnit != NULL ? pUnit->GetID() : 0;
    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE,
              "hexNext: (%d,%d), hexDest: (%d,%d), ptDest (%d,%d), ptNext owner: %d, terrain: %d",
              m_hexNext.X(), m_hexNext.Y(), m_hexDest.X(), m_hexDest.Y(), m_ptDest.x, m_ptDest.y, ID,
              theMap.GetHex(m_ptNext)->GetType());
#endif

    _SetRouteMode(blocked);
    ZeroMoveParams();
    m_dwTimeBlocked = 0;

    CVehicle *pVehInWay = theVehicleHex._GetVehicle(m_ptNext);

    // free it if bogus
    if (pVehInWay != this) {
        if (pVehInWay != NULL)
            theVehicleHex.CheckHex(m_ptNext);
    } else
        // release if ours
    if ((m_ptNext != m_ptHead) && (m_ptNext != m_ptTail))
        theVehicleHex.ReleaseHex(m_ptNext, this);

    // are we blocked at least 8 away from before?
    // or need a new path
    if (((m_iPathLen > 0) && (m_iPathOff >= m_iPathLen) &&
         (*(m_phexPath + m_iPathLen - 1) != m_hexDest)) ||
        (abs(m_subBlocked.Dist(m_ptHead)) > 8)) {
        m_iNumRetries = m_iBlockCount = 0;
        m_subBlocked = m_ptHead;
    }
}

// we take a new ptNext if possible
BOOL CVehicle::TryNewSub(BOOL bNoNewPath) {

    // see if we can find one
    FindSub();
    if (!CanEnter(m_ptNext))
        return (FALSE);

    // step 2 - if we are now further away - do a GetPath (TRUE)
    if (!bNoNewPath) {
        int xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptNext.x);
        int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptNext.y);
        int iNew = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));
        xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptHead.x);
        yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptHead.y);
        int iOld = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));

        if (iOld <= iNew) {
            GetPath(TRUE);

            // see if we can find one using the new path
            m_ptNext = m_ptHead;        // so tries all directions
            FindSub();
            if (!CanEnter(m_ptNext))
                return (FALSE);
        }
    }

    // If we were circling
    // step 3 - if this puts us further away from our dest we don't take it
    if (m_iTimesOn >= MAX_TIMES_CIRCLE / 2) {
        int xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptNext.x);
        int yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptNext.y);
        int iNew = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));
        xDif = CSubHex::Diff(m_hexNext.X() * 2 - m_ptHead.x);
        yDif = CSubHex::Diff(m_hexNext.Y() * 2 - m_ptHead.y);
        int iOld = ((xDif >= 0) ? xDif : -(xDif + 1)) + ((yDif >= 0) ? yDif : -(yDif + 1));
        if ((iOld <= iNew) && (iNew != 0))
            if (!FindSub(TRUE))
                return (FALSE);
    }

    // set it up to go
    SetMoveParams(FALSE);
    CheckNextHex();
    theVehicleHex.GrabHex(m_ptNext, this);
    SetHexDest();
    _SetRouteMode(moving);
    ASSERT_VALID (this);
    return (TRUE);
}

class CNewDest {
public:
    CHexCoord m_hexOn;
    CHexCoord m_hexDest;
    int m_iWheelType;
    int m_iDist;
};

static int fnEnumFindNewDest(CHex *pHex, CHexCoord hex, void *pData) {

    CNewDest *pNd = (CNewDest *) pData;

    // no on a building
    if (pHex->GetUnits() & CHex::bldg)
        return (FALSE);

    // must be able to travel on it
    if (theTerrain.GetData(pHex->GetType()).GetWheelMult(pNd->m_iWheelType) <= 0)
        return (FALSE);

    // only 1 vehicle sub
    int iVeh = pHex->GetUnits() & CHex::veh;
    int iNum = 0;
    while (iVeh != 0) {
        if (iVeh & 0x01)
            if (++iNum > 1)
                return (FALSE);
        iVeh >>= 1;
    }

    int iDist = CHexCoord::Dist(hex, pNd->m_hexOn);
    if (iDist < pNd->m_iDist) {
        pNd->m_hexDest = hex;
        pNd->m_iDist = iDist;
    }

    return (FALSE);
}

// we handle blocked vehicles here
// basically we try step after step using if ( m_iNumRetries++ (==,>=) ## )
// we accomplish 2 things with this. First if a given method fails it can
// drop out and we then try the next. Second, if a given method does not
// return we use >= instead of == and we try it the next time around if
// necessary
void CVehicle::HandleBlocked() {

    ASSERT_VALID (this);

    if (!GetOwner()->IsLocal()) {
        TRAP();
        return;
    }

    // one time out of 4 we do nothing to avoid deadlock
    if ((MyRand() & 0x3000) == 0x1000)
        return;

#ifdef _LOGOUT
    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) trying to unblock, retry %d, block %d",
              GetID(), m_ptHead.x, m_ptHead.y, m_iNumRetries, m_iBlockCount);
#endif

    m_dwTimeBlocked += theGame.GetOpersElapsed();
    m_iBlockCount++;

    // we only get the path once per time in here
    BOOL bGotPath = FALSE;

    int xLeft = abs(CSubHex::Diff(m_ptDest.x - m_ptHead.x));
    int yLeft = abs(CSubHex::Diff(m_ptDest.y - m_ptHead.y));

    // special test for carriers - within 1 then transport in (very congested)
    if (m_iEvent == load)
        if (xLeft + yLeft <= 2) {
            TRAP();
            ReleaseOwnership();
            ForceAtDest();
            ArrivedDest();
            return;
        }

    // if travelling onto a bridge under construction - the end is it
    CBridgeUnit *pBu = theBridgeHex.GetBridge(m_ptHead);
    if ((pBu != NULL) && (!pBu->GetParent()->IsBuilt()) &&
        (pBu == theBridgeHex.GetBridge(m_ptDest))) {
        TRAP();
#ifdef _LOGOUT
        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d arrived at incomplete bridge", GetID());
#endif
        ReleaseOwnership();
        ForceAtDest();
        ArrivedDest();
        return;
    }

    CBuilding *pBldgDest = theBuildingHex._GetBuilding(m_hexDest);
    CVehicle *pVehDest = theVehicleHex._GetVehicle(m_ptDest);
    CHexCoord _hexHead(m_ptHead);

    // if our dest is a building and we are next to it we transport in.
    // This is such a problem that we don't even try to go around to the entrance
    if (m_iNumRetries >= 0) {
        if (m_iNumRetries == 0)
            m_iNumRetries++;

        if ((pBldgDest != NULL) && (CanEnterBldg(pBldgDest)) && (pBldgDest != m_pUnitTarget)) {
            int xDif = CHexCoord::Diff(_hexHead.X() - pBldgDest->GetHex().X());
            if ((-1 <= xDif) && (xDif <= pBldgDest->GetCX())) {
                int yDif = CHexCoord::Diff(_hexHead.Y() - pBldgDest->GetHex().Y());
                if ((-1 <= yDif) && (yDif <= pBldgDest->GetCY())) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d transported to dest", GetID());
#endif
                    ReleaseOwnership();
                    ForceAtDest();
                    ArrivedDest();
                    ASSERT_VALID (this);
                    return;
                }
            }
        }
    }

    // if our dest is an enemy building or a target - lets surround it
    // must be close
    if (m_iNumRetries == 1) {
        m_iNumRetries++;
        if ((m_iEvent == attack) && (m_pUnitTarget != NULL))
            if (((m_pUnitTarget == pBldgDest) || (m_pUnitTarget == pVehDest)) && (xLeft + yLeft < 7)) {
                CHexCoord _hex;
                int iDist, cx, cy;
                if (m_pUnitTarget == pBldgDest) {
                    _hex = pBldgDest->GetHex();
                    iDist = GetRange();
                    cx = pBldgDest->GetCX();
                    cy = pBldgDest->GetCY();
                } else {
                    _hex = pVehDest->GetHexHead();
                    iDist = __max (1, GetRange() - 1);
                    cx = cy = 1;
                }

                // find the closest empty slot
                _hex.X() -= iDist;
                _hex.Y() -= iDist;
                _hex.Wrap();
                cx += iDist * 2;
                cy += iDist * 2;
                CNewDest nd;
                nd.m_hexOn = _hexHead;
                nd.m_hexDest = GetHexDest();
                nd.m_iWheelType = GetData()->GetWheelType();
                nd.m_iDist = INT_MAX;
                theMap.EnumHexes(_hex, cx, cy, fnEnumFindNewDest, &nd);

                if (nd.m_hexOn != _hexHead) {
                    TRAP();
                    GetPath(FALSE);
                    if (HavePathOrNext())
                        if (TryNewSub(TRUE)) {
#ifdef _LOGOUT
                            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d surround dest", GetID());
#endif
                            return;
                        }
                    bGotPath = TRUE;
                }
            }
    }

    // special test for fixed turret vehicles trying to shoot
    if (m_iNumRetries == 2) {
        m_iNumRetries++;
        if ((GetTurret() == NULL) && (m_pUnitTarget != NULL)) {
            // we presently can't shoot at the target
            // if we are within 3 sub hexes then we need to try turning away to get a better bead
            if (!CanShootAt(m_pUnitTarget)) {
                int xDif = CMapLoc::Diff(m_pUnitTarget->GetMapLoc().x - m_maploc.x) / (MAX_HEX_HT - 1);
                int yDif = CMapLoc::Diff(m_pUnitTarget->GetMapLoc().y - m_maploc.y) / (MAX_HEX_HT - 1);
                if ((abs(xDif) <= 3) && (abs(yDif) <= 3)) {
                    m_ptNext.x = m_ptHead.x - __minmax(-1, 1, xDif);
                    m_ptNext.y = m_ptHead.y - __minmax(-1, 1, yDif);
                    m_ptNext.Wrap();
                    int iDir = GetAngle(m_ptNext, m_ptHead, m_ptHead, m_ptTail);
                    if (iDir > 2)
                        m_ptNext = Rotate(2);
                    else if (iDir < -2)
                        m_ptNext = Rotate(-2);

                    // send it away if it can go
                    if (CanEnter(m_ptNext)) {
                        SetMoveParams(FALSE);
                        CheckNextHex();
                        theVehicleHex.GrabHex(m_ptNext, this);
                        SetHexDest();
                        _SetRouteMode(moving);
                        ASSERT_VALID (this);
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d fixed turret moving away to turn",
                                  GetID());
#endif
                        return;
                    }
                }
            }
        }
    }

    // I don't know why but sometimes it has advanced too far on the path and needs to be brought back
    if (m_iNumRetries == 3) {
        m_iNumRetries++;
        if ((m_iPathOff > 1) && (m_iPathOff < m_iPathLen))
            if ((abs(CHexCoord::Diff(_hexHead.X() - m_hexNext.X())) > 1) ||
                (abs(CHexCoord::Diff(_hexHead.Y() - m_hexNext.Y())) > 1)) {
                if ((abs(CHexCoord::Diff(_hexHead.X() - (m_phexPath + m_iPathOff - 2)->X())) <= 1) &&
                    (abs(CHexCoord::Diff(_hexHead.Y() - (m_phexPath + m_iPathOff - 2)->Y())) <= 1)) {
                    m_iPathOff -= 2;
                    m_hexNext = *(m_phexPath + m_iPathOff);
                    PathNextHex();
                    if (TryNewSub(TRUE)) {
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d got too far on phexPath", GetID());
#endif
                        return;
                    }
                }
            }
    }

    // we can come here because the path ran out or we got too far from it
    if (m_iNumRetries == 4) {
        m_iNumRetries++;
        if ((!HavePathOrNext()) || (m_iPathOff >= m_iPathLen) ||
            (abs(CHexCoord::Diff(m_hexNext.X() - _hexHead.X())) > 1) ||
            (abs(CHexCoord::Diff(m_hexNext.Y() - _hexHead.Y())) > 1)) {
            GetPath(FALSE);
            if (HavePathOrNext())
                if (TryNewSub(TRUE)) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d got a new path", GetID());
#endif
                    return;
                }
            bGotPath = TRUE;
        }
    }

    CVehicle *pVehInWay = theVehicleHex._GetVehicle(m_ptNext);

    // is the block legit (didn't release properly?)
    if (m_iNumRetries == 5) {
        m_iNumRetries++;
        if (pVehInWay != NULL) {
            if ((pVehInWay->m_ptNext != m_ptNext) && (pVehInWay->m_ptHead != m_ptNext) &&
                (pVehInWay->m_ptTail != m_ptNext)) {
                TRAP();
                theVehicleHex.CheckHex(m_ptNext);
                if (TryNewSub(bGotPath)) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d illegal block", GetID());
#endif
                    return;
                }
                bGotPath = TRUE;
            }

            // what if it's us (m_ptNext == m_ptHead)
            if (pVehInWay == this) {
                // can't do it if we may be fully blocked - need to test for that
                if ((pVehDest == NULL) || (pBldgDest != NULL)) {
                    if (TryNewSub(bGotPath)) {
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d blocked itself", GetID());
#endif
                        return;
                    }
                    bGotPath = TRUE;
                }
            } else

                // if a vehicle is in the way in a building - take it's ownership
            if (pVehInWay->GetOwner() == GetOwner())
                if (theBuildingHex._GetBuilding(pVehInWay->m_ptHead) != NULL) {
                    pVehInWay->EnterBuilding();
                    pVehInWay = theVehicleHex._GetVehicle(m_ptNext);

                    if (pVehInWay == NULL) {
                        theVehicleHex.GrabHex(m_ptNext, this);
                        _SetRouteMode(moving);
                        SetMoveParams(FALSE);
                        ASSERT_VALID (this);
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d blocked by exiting veh", GetID());
#endif
                        return;
                    }
                }
        }
    }

    // can we continue (the road cleared)?
    if (CanEnter(m_ptNext)) {
        // one time out of 16 we do nothing to avoid deadlock
        if ((MyRand() & 0xF000) == 0x1000) {
            m_iBlockCount--;
            return;
        }

        theVehicleHex.GrabHex(m_ptNext, this);
        SetMoveParams(FALSE);
        _SetRouteMode(moving);
        ASSERT_VALID (this);
#ifdef _LOGOUT
        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d road cleared", GetID());
#endif
        return;
    }

    // wait a bit
    if (m_dwTimeBlocked < (DWORD) (m_iBlockCount * m_iSpeed * STEPS_HEX / 4)) {
        m_iBlockCount--;
        return;
    }

    // if we're reasonably close AND THE DEST IS BLOCKED we stop (and not going to building or load)
    if (m_iNumRetries == 6) {
        m_iNumRetries++;
        if ((pVehDest != NULL) && (pBldgDest == NULL) && (m_iEvent == none)) {
            int xDif = abs(m_ptDest.x - m_ptHead.x);
            int yDif = abs(m_ptDest.y - m_ptHead.y);
            // if 1 - we're there
            if (xDif + yDif <= 2) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d can't get closer", GetID());
#endif
                goto GiveUp;
            }

            // we check N random sub hexes around the dest - if can't travel in any of them we're there
            int iCheck = xDif + yDif;
            int xBase = m_ptDest.x - xDif;
            int yBase = m_ptDest.y - yDif;
            xDif *= 2;
            yDif *= 2;

            if (iCheck < 8) {
                int iTry = 0;
                for ( ; iTry < iCheck; iTry++) {
                    CHex *pHex = theMap.GetHex(xBase + RandNum(xDif), yBase + RandNum(yDif));
                    if ((!(pHex->GetUnits() & CHex::unit)) && (GetData()->CanTravelHex(pHex)))
                        break;
                }
                if (iTry >= iCheck) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d probably can't get closer", GetID());
#endif
                    goto GiveUp;
                }

                // if it's a land/water break we're there (actually travel/no travel)
                if (!GetData()->CanEnterHex(m_ptDest, m_ptDest, IsOnWater(), TRUE)) {
                    TRAP();
                    xDif = m_ptDest.x - m_ptHead.x;
                    yDif = m_ptDest.y - m_ptHead.y;
                    xDif = __minmax(-1, 1, xDif);
                    yDif = __minmax(-1, 1, yDif);
                    CSubHex _test(m_ptHead.x + xDif, m_ptHead.y + yDif);
                    _test.Wrap();
                    if (!GetData()->CanEnterHex(m_ptHead, _test, IsOnWater(), TRUE)) {
                        TRAP();
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d hit land/water break", GetID());
#endif
                        goto GiveUp;
                    }
                }
            }

            // if we're in the way now we can test this
            if (pVehInWay == this) {
                if (TryNewSub(bGotPath)) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d road cleared 2", GetID());
#endif
                    return;
                }
                bGotPath = TRUE;
            }
        }
    }

    // see if we can find a new path (that is closer)
    if (m_iNumRetries == 7) {
        m_iNumRetries++;
        if (TryNewSub(bGotPath)) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d found new next", GetID());
#endif
            return;
        }
        bGotPath = TRUE;
    }

    // wait the time it would take this guy to move through this hex
    if (m_dwTimeBlocked < (DWORD) (m_iBlockCount * m_iSpeed * STEPS_HEX)) {
        m_iBlockCount--;
        return;
    }

    // at this point things aren't too bad so we'll get a new path if we are off
    // and see if we can just find a sub
    if (m_iNumRetries == 8) {
        m_iNumRetries++;
        if (!bGotPath) {
            // we look for a path with no vehicles in the way
            GetPath(TRUE);

            bGotPath = TRUE;
        }
    }

    // off we go again
    if (m_iNumRetries >= 9) {
        if (m_iNumRetries == 9)
            m_iNumRetries++;
        if (TryNextHex()) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d new path & next", GetID());
#endif
            return;
        }
    }

    // if the vehicle in our way is moving we double how long we'll wait
    if ((pVehInWay != NULL) && (pVehInWay->m_cMode == moving))
        if (m_dwTimeBlocked < (DWORD) (2 * m_iBlockCount * m_iBlockCount * m_iSpeed * STEPS_HEX)) {
            m_iBlockCount--;
            return;
        }

    // we try a new route picked at random
    if (m_iNumRetries == 10) {
        m_iNumRetries++;
        m_ptNext = m_ptHead;
        if (TryNewSub(bGotPath)) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d random new next", GetID());
#endif
            return;
        }
        bGotPath = TRUE;
    }

    // we randomize again
    if (MyRand() & 0x1000) {
        m_iBlockCount--;
        return;
    }

    // if not a ship or 1 hex try angle 3, -3
    if (m_iNumRetries == 11) {
        m_iNumRetries++;
        if ((!(GetData()->GetVehFlags() & CTransportData::FL1hex)) && (!GetData()->IsBoat())) {
            int iDir = (MyRand() & 0x1000) ? 3 : -3;
            m_ptNext = Rotate(iDir);
            if (TryNewSub(bGotPath)) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d turn (-)3", GetID());
#endif
                return;
            }
            m_ptNext = Rotate(-iDir);
            if (TryNewSub(TRUE)) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d turn (-)3", GetID());
#endif
                return;
            }
        }
    }

    // ok, if we got a path above we now wait till next turn
    if (bGotPath)
        return;

    // we now try to find a clear (no vehicles) route
    //   if < 5 hexes go all non-inuse
    //   else go around
    //     if new > oldlen-5 go all non-inuse
    //     else splice
    if (m_iNumRetries == 12) {
        m_iNumRetries++;

        if (pVehInWay != NULL) {
            bGotPath = TRUE;
            if (m_iPathLen - m_iPathOff < 5)
                GetPath(TRUE);
            else {
                // find the first clear hex
                CHexCoord *pHex = m_phexPath + m_iPathOff;
                int iLeft = m_iPathLen - m_iPathOff;
                while (iLeft--)
                    if ((theMap._GetHex(pHex->X(), pHex->Y())->GetUnits() & CHex::unit) == 0)
                        break;
                // if its less than 5 - go all new
                if (iLeft < 5)
                    GetPath(TRUE);
                else

                    // now we get a path to the free hex, saving the old path to merge it in
                {
                    CHexCoord _dest(m_hexDest);
                    CHexCoord _dest2(*pHex);
                    m_hexDest = *pHex;
                    GetPath(TRUE);
                    m_hexDest = _dest;

                    // if we didn't make it to the dest, then go as far as we can an then we'll try again
                    //   otherwise, we see if we want to truncate it
                    if ((m_iPathLen > 2) && (*(m_phexPath + m_iPathLen - 1) == _dest2)) {
                        // walk new path until it comes closer (straight line) to where we are
                        pHex = m_phexPath + 1;
                        iLeft = m_iPathLen - 1;
                        int iLen = 0;
                        while (iLeft--) {
                            int iNewLen = theMap.GetRangeDistance(*pHex, *m_phexPath);
                            if (iNewLen < iLen)
                                break;
                            iLen = iNewLen;
                            pHex++;
                        }

                        // we now set m_iPathLen to this distance and it will re-try from there
                        if (iLen + 5 < m_iPathLen) {
                            ASSERT (pHex - m_phexPath <= m_iPathLen);
                            ASSERT (pHex - m_phexPath > 1);
                            m_iPathLen = pHex - m_phexPath;
                        }
                    }
                }
            }
        }

        // if we found a new a path, let's see if it takes us closer
        if (HavePathOrNext()) {
            if (m_hexNext.SameHex(m_ptHead))
                PathNextHex();

            CHexCoord _hex;
            if (HavePath())
                _hex = *(m_phexPath + m_iPathLen - 1);
            else
                _hex = m_hexNext;
            int iNewDist = theMap.GetRangeDistance(_hex, m_hexNext);
            CHexCoord _head(m_ptHead);
            int iOldDist = theMap.GetRangeDistance(_head, m_hexNext);

            // we have a new route
            if ((iNewDist <= iOldDist) || (iNewDist <= 1))
                if (TryNextHex()) {
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d went around block", GetID());
#endif
                    return;
                }
        }
    }

    // at this point we wait a little longer
    if (m_dwTimeBlocked < (DWORD) (3 * m_iBlockCount * m_iBlockCount * m_iBlockCount * m_iSpeed * STEPS_HEX)) {
        m_iBlockCount--;
        return;
    }

    // try to clear it out big-time
    if (m_iNumRetries == 13) {
        m_iNumRetries++;
        CheckAroundBuilding();
        // we've got it - next time around will find path
        if (CanEnter(m_ptNext)) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d cleared around building", GetID());
#endif
            return;
        }
    }

    // ok, we now try a 180 degree turn
    // we only do this if permanently blocked in our direction and it's clear in the other
    if (m_iNumRetries == 14) {
        m_iNumRetries++;

        if (!(GetData()->GetVehFlags() & CTransportData::FL1hex)) {
            for (int iDir = -3; iDir <= 3; iDir++) {
                CSubHex _next = Rotate(iDir);
                if (CanEnter(_next)) {
                    m_ptNext = _next;
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d turn (-)3 try 2", GetID());
#endif
                    return;
                }
            }

            // before we turn let's make sure the other direction is better
            for (int iDir = -3; iDir <= 3; iDir++) {
                CSubHex _next = ::Rotate(iDir, m_ptTail, m_ptHead);
                if (CanEnter(_next)) {
                    Turn180();
                    m_ptNext = _next;
                    TryNextHex();
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d turn 180", GetID());
#endif
                    return;
                }
            }
        }
    }

    // for the AI we will actually move to the next hex if we can
    if (m_iNumRetries == 15) {
        m_iNumRetries++;
        if (GetOwner()->IsAI()) {
            // get an up to date path
            if (!bGotPath)
                GetPath(TRUE);
            bGotPath = TRUE;

            if ((m_iPathLen > 3) && (m_iPathOff + 3 < m_iPathLen)) {
                // didn't work
                if (m_hexNext.SameHex(m_ptHead))
                    PathNextHex();

                int xAdd = CHexCoord::Diff(m_hexNext.X() - _hexHead.X());
                int yAdd = CHexCoord::Diff(m_hexNext.Y() - _hexHead.Y());
                xAdd = xAdd < -1 ? -1 : (xAdd > 1 ? 1 : xAdd);
                yAdd = yAdd < -1 ? -1 : (yAdd > 1 ? 1 : yAdd);

                if ((xAdd != 0) || (yAdd != 0)) {
                    CSubHex _head(m_ptTail.x + xAdd, m_ptTail.y + yAdd);
                    _head.Wrap();
                    CSubHex _tail;
                    if (GetData()->GetVehFlags() & CTransportData::FL1hex)
                        _tail = _head;
                    else
                        _tail = m_ptTail;


                    for (int iTrys = 3; iTrys > 0; iTrys--) {
                        // see if we can find a free head/tail that we can travel on
                        CVehicle *pHead = theVehicleHex._GetVehicle(_head);
                        if ((pHead != NULL) && (pHead != this))
                            continue;
                        CVehicle *pTail = theVehicleHex._GetVehicle(_tail);
                        if ((pTail != NULL) && (pTail != this))
                            continue;
                        if (!CanEnter(_head, TRUE))
                            continue;

                        // ok, we've got a location - Scotty beam us over
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) transport to sub (%d,%d)",
                                  GetID(), m_ptHead.x, m_ptHead.y, _head.x, _head.y);
#endif
                        ReleaseOwnership();
                        m_ptNext = m_ptHead = _head;
                        m_ptTail = _tail;
                        TakeOwnership();
                        m_iNumRetries = 0;
                        TryNextHex();
#ifdef _LOGOUT
                        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d AI transport to next hex", GetID());
#endif
                        return;
                    }
                }
            }
        }
    }

    // we tried all of the above and have been in here at least 5 times
    if ((m_iNumRetries > 15) && (m_iBlockCount > 5)) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) permanently blocked", GetID(), m_ptHead.x,
                  m_ptHead.y);
#endif

        GiveUp:
        _SetRouteMode(stop);
        if (theBuildingHex._GetBuilding(_hexHead) != NULL)
            EnterBuilding();

        PostArrivedOrBlocked();

        if (GetOwner()->IsMe())
            theGame.Event(EVENT_GOTO_CANT, EVENT_NOTIFY, this);
        ASSERT_VALID (this);
        return;
    }

    // nothing left. Will keep trying above till 5 attempts
    if (m_iNumRetries > 15) {
        m_iNumRetries = 0;
        m_dwTimeBlocked = 0;
    }

    ASSERT_VALID (this);
}

BOOL CVehicle::TryNextHex() {

    if (!CanEnter(m_ptNext))
        return (FALSE);

    CVehicle *pVehNext = theVehicleHex._GetVehicle(m_ptNext);
    if ((pVehNext != NULL) && (pVehNext != this))
        return (FALSE);

    SetMoveParams(FALSE);
    _SetRouteMode(moving);
    theVehicleHex.GrabHex(m_ptNext, this);

    ASSERT (m_ptNext != m_ptHead);
    ASSERT_VALID_LOC (this);
    return (TRUE);
}

void CVehicle::Turn180() {

    // turn off spotting for resetting below
    BOOL bSpotting = FALSE;
    if (m_cOwn && (!m_ptHead.SameHex(m_ptTail)) && DoSpotting() && SpottingOn()) {
        DecrementSpotting();
        bSpotting = TRUE;
    }

    CSubHex _tmp = m_ptTail;
    m_ptTail = m_ptHead;
    m_ptHead = m_ptNext = _tmp;;
    m_iDir = CalcDir();

    // set who they can now see/shoot
    if (bSpotting) {
        DetermineSpotting();
        IncrementSpotting(GetHexHead());
    }

    SetLoc(FALSE);
    AtNewLoc();
}

void CVehicle::MsgSetNextHex(CMsgVehLoc *pMsg) {

    ASSERT_VALID (this);
    ASSERT_CMD (pMsg);

    // must be remote
    if (GetOwner()->IsLocal())
        return;

#ifdef _LOGOUT
    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d MsgSetNextHex head (%d,%d), tail (%d,%d)", GetID(), m_ptHead.x,
              m_ptHead.y, m_ptTail.x, m_ptTail.y);
#endif

    // update the world map if we see it move
    BOOL bWorld = FALSE;
    if (!m_ptHead.SameHex(pMsg->m_ptHead))
        if (IsVisible()) {
            bWorld = TRUE;
            theApp.m_wndWorld.InvalidateWindow(CWndWorld::visible | CWndWorld::other_units);
        }

    /////////////////////////////////////////////////////////////////////////////
    // this gets real screwy. If we already hold m_ptNext we can stall both
    // vehicles. However, if Head or Tail conflict then we can't back up since
    // we don't know where to back up to. However, we don't want to sit on
    // the same hex either.
    //
    // So if m_ptHead or m_ptTail conflict - we do nothing. Each system will
    // show it's vehicle at the new location and the other vehicle staying
    // at the old location.
    //
    // If just m_ptNext conflicts we pull both back to head/tail and do a FindSub
    // on ours (which will attempt to find a sub other than m_ptNext). We mix
    // a random number into this to try for the same one again or block so we
    // don't dance with the other vehicle.
    //
    // Of course, if m_cOwn == 0 we have no problems <g>.

    // if it doesn't own hexes see if it did and free them
    if (!pMsg->m_iOwn) {
        if (m_cOwn) {
            ASSERT (theVehicleHex.GetVehicle(m_ptHead) == this);
            ASSERT (theVehicleHex.GetVehicle(m_ptTail) == this);

            if (DoSpotting()) {
                TRAP();
                DecrementSpotting();
            }

            // this can be anything because of F1hex vs 2 hex, jump moves, etc
            if (theVehicleHex._GetVehicle(m_ptNext) == this)
                theVehicleHex.ReleaseHex(m_ptNext, this);
            if (theVehicleHex._GetVehicle(m_ptHead) == this)
                theVehicleHex.ReleaseHex(m_ptHead, this);
            if (theVehicleHex._GetVehicle(m_ptTail) == this)
                theVehicleHex.ReleaseHex(m_ptTail, this);
        }
    } else {
        // ok - see if we can grab the new head/tail
        CVehicle *pVehHead = theVehicleHex._GetVehicle(pMsg->m_ptHead);
        CVehicle *pVehTail;
        if ((pVehHead != NULL) && (pVehHead != this)) {
            // check for bogus grab
            theVehicleHex.CheckHex(pMsg->m_ptHead);
            pVehHead = theVehicleHex._GetVehicle(pMsg->m_ptHead);
            if (pVehHead != NULL)
                goto PuntIt;
        }
        pVehTail = theVehicleHex._GetVehicle(pMsg->m_ptTail);
        if ((pVehTail != NULL) && (pVehTail != this)) {
            // check for bogus grab
            theVehicleHex.CheckHex(pMsg->m_ptTail);
            pVehTail = theVehicleHex._GetVehicle(pMsg->m_ptTail);

            // ok, we can't get the new head and/or tail - we just punt
            if (pVehTail != NULL) {
                PuntIt:
#ifdef _LOGOUT
                logPrintf(LOG_PRI_ALWAYS, LOG_VEH_MOVE,
                          "*** Vehicle %d position conflict at head (%d,%d), tail (%d,%d)", GetID(), m_ptHead.x,
                          m_ptHead.y, m_ptTail.x, m_ptTail.y);
#endif
                ASSERT_VALID_LOC (this);
                if ((pMsg->m_ptNext != m_ptHead) && (pMsg->m_ptNext != m_ptTail))
                    if (theVehicleHex._GetVehicle(pMsg->m_ptNext) == this)
                        theVehicleHex.ReleaseHex(pMsg->m_ptNext, this);
                m_mvlTraffic = *pMsg;
                _SetRouteMode(traffic);
                return;
            }
        }

        // release where we were
        BOOL bSpot = TRUE;
        if (m_cOwn) {
            ASSERT (theVehicleHex.GetVehicle(m_ptHead) == this);
            ASSERT (theVehicleHex.GetVehicle(m_ptTail) == this);

            if (DoSpotting() && (!m_ptHead.SameHex(pMsg->m_ptHead)))
                DecrementSpotting();
            else
                bSpot = FALSE;

            // this can be anything because of F1hex vs 2 hex, jump moves, etc
            if (theVehicleHex._GetVehicle(m_ptNext) == this)
                theVehicleHex.ReleaseHex(m_ptNext, this);
            if (theVehicleHex._GetVehicle(m_ptHead) == this)
                theVehicleHex.ReleaseHex(m_ptHead, this);
            if (theVehicleHex._GetVehicle(m_ptTail) == this)
                theVehicleHex.ReleaseHex(m_ptTail, this);
        }

        // oppo fire - new hex or arrived at next (so we don't know)
        BOOL bOppo = (!m_ptHead.SameHex(pMsg->m_ptHead)) || (m_ptHead == m_ptNext);

        // and grab the new head/tail
        m_cOwn = TRUE;
        theVehicleHex.GrabHex(m_ptHead = pMsg->m_ptHead, this);
        theVehicleHex.GrabHex(m_ptTail = pMsg->m_ptTail, this);

        if (DoSpotting() && bSpot) {
            DetermineSpotting();
            IncrementSpotting(m_ptHead);
        }

        // oppo fire
        if (bOppo)
            OtherOppo();

        // is Next free?
        CVehicle *pVehNext = theVehicleHex._GetVehicle(pMsg->m_ptNext);
        if ((pVehNext != NULL) && (pVehNext != this)) {
            // check for bogus grab
            theVehicleHex.CheckHex(pMsg->m_ptNext);
            pVehNext = theVehicleHex._GetVehicle(pMsg->m_ptNext);
            TRAP(pVehNext == NULL);
        }

        if ((pVehNext == NULL) || (pVehNext == this))
            theVehicleHex.GrabHex(m_ptNext = pMsg->m_ptNext, this);
        else

            // can't move to next
        {
            if (theVehicleHex._GetVehicle(pMsg->m_ptNext) == this)
                theVehicleHex.ReleaseHex(pMsg->m_ptNext, this);

#ifdef _LOGOUT
            logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "*** Vehicle %d ptNext conflict at head (%d,%d), tail (%d,%d)",
                      GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y);
#endif
            // first - this vehicle isn't going anywhere
            pMsg->m_ptNext = pMsg->m_ptHead;
            pMsg->m_iMode = traffic;

            // if Next conflicts we play some games - ONLY IF IT'S LOCAL
            if (pVehNext->GetOwner()->IsLocal()) {
                pVehNext->m_iNumRetries = pVehNext->m_iBlockCount = 0;
#ifdef TEST_TRAFFIC
                if ( pVehNext->m_cOwn )
                    {
                    TRAP (theVehicleHex._GetVehicle (pVehNext->m_ptHead) != pVehNext);
                    TRAP (theVehicleHex._GetVehicle (pVehNext->m_ptTail) != pVehNext);
                    for (int iInd=0; iInd<NUM_SUBS_OWNED; iInd++)
                        if ( ( SubsOwned[iInd].x != -1 ) &&
                                    (SubsOwned[iInd] != m_ptNext) &&
                                    (SubsOwned[iInd] != m_ptHead) &&
                                    (SubsOwned[iInd] != m_ptTail) )
                        TRAP ();
                    }
#endif

                // what do we do with our vehicle?
                if (theVehicleHex._GetVehicle(pVehNext->m_ptNext) == pVehNext)
                    if ((pVehNext->m_ptNext != pVehNext->m_ptHead) &&
                        (pVehNext->m_ptNext != pVehNext->m_ptTail))
                        theVehicleHex.ReleaseHex(pVehNext->m_ptNext, pVehNext);
                switch ((MyRand() >> 8) & 0x03) {
                    case 0 :        // 25% chance it blocks and looks next loop
                        pVehNext->m_ptNext = pVehNext->m_ptHead;
                        pVehNext->MakeBlocked();
                        break;

                    case 1 :        // 50% chance it looks for a different next
                    case 2 : {
                        CSubHex _tmp(pVehNext->m_ptNext);
                        pVehNext->FindSub();
                        if ((pVehNext->m_ptNext != _tmp) && (CanEnter(pVehNext->m_ptNext))) {
                            pVehNext->SetMoveParams(FALSE);
                            pVehNext->CheckNextHex();
                            theVehicleHex.GrabHex(pVehNext->m_ptNext, pVehNext);
                            pVehNext->SetHexDest();
                            pVehNext->_SetRouteMode(moving);
                        } else
                            pVehNext->MakeBlocked();
                        break;
                    }

                    case 3 :        // 25% chance it sits for a bit
                        pVehNext->m_ptNext = pVehNext->m_ptHead;
                        pVehNext->SetMoveParams(FALSE);
                        pVehNext->_SetRouteMode(traffic);
                        pVehNext->m_dwTimeBlocked = RandNum(24);
                        break;
                }
            }

            m_mvlTraffic = *pMsg;
            _SetRouteMode(traffic);
            return;
        }
    }

    // ok, we now set the vehicle to this location
    SetFromMsg(pMsg, bWorld);

    ASSERT_VALID_LOC (this);
}

void CVehicle::SetFromMsg(CMsgVehLoc *pMsg, BOOL bWorld) {

    m_ptDest = pMsg->m_ptDest;
    m_ptNext = pMsg->m_ptNext;
    m_ptHead = pMsg->m_ptHead;
    m_ptTail = pMsg->m_ptTail;
    m_hexNext = pMsg->m_hexNext;
    m_hexDest = pMsg->m_hexDest;
    m_iDir = pMsg->m_iDir;
    if (GetTurret() != NULL)
        GetTurret()->SetDir(pMsg->m_iTurretDir);
    m_iXadd = pMsg->m_iXadd;
    m_iYadd = pMsg->m_iYadd;
    m_iDadd = pMsg->m_iDadd;
    m_iTadd = pMsg->m_iTadd;
    m_cMode = (CVehicle::VEH_MODE) pMsg->m_iMode;
    m_cOwn = pMsg->m_iOwn;
    m_iStepsLeft = pMsg->m_iStepsLeft;
    m_iSpeed = pMsg->m_iSpeed;
    SetOnWater(pMsg->m_bOnWater);

    SetLoc(TRUE);

    // became visible
    if (!bWorld && (IsVisible()))
        theApp.m_wndWorld.InvalidateWindow(CWndWorld::visible | CWndWorld::other_units);
}

void CVehicle::CantInBldg(CBuilding const *pBldg) {

    ASSERT_VALID (this);
    TRAP();

    ReleaseOwnership();
    GetExitLoc(pBldg, GetData()->GetType(), m_ptNext, m_ptHead, m_ptTail);
    CheckExit();
    SetLoc(TRUE);
    _SetRouteMode(cant_deploy);
}

// Start it moving
void CVehicle::StartTravel(BOOL bGetPath) {

    ASSERT_VALID (this);

#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) StartTravel", GetID(), m_ptHead.x, m_ptHead.y);
#endif

    // do we need to get the path?
    if (bGetPath)
        if (!HavePathOrNext())
            GetPath(FALSE);

    // if no path we are blocked
    if (!HavePathOrNext()) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d has no path", GetID());
#endif
        MakeBlocked();
        return;
    }

    if (m_hexNext.SameHex(m_ptHead))
        PathNextHex();

    // get m_ptNext
    if ((m_ptNext == m_ptHead) || (theVehicleHex._GetVehicle(m_ptNext) != this)) {
        m_ptNext = m_ptHead;
        if (!GetNextHex(TRUE))
            if (m_cMode == moving) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d can't get a path", GetID());
#endif
                TRAP();
                MakeBlocked();
                return;
            }
    }

    // if arrived we don't need to move
    if (m_ptDest == m_ptHead) {
        if ((m_ptNext != m_ptHead) && (theVehicleHex._GetVehicle(m_ptNext) == this))
            theVehicleHex.ReleaseHex(m_ptNext, this);
        ZeroMoveParams();
        m_ptNext = m_ptHead;
        return;
    }

    // if next not avail (above doesn't work if at dest) mark blocked
    CVehicle *pVehNext = theVehicleHex._GetVehicle(m_ptNext);
    if ((pVehNext != NULL) && (pVehNext != this)) {
        if (m_cMode == moving) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d can't get a path", GetID());
#endif
            TRAP();
            MakeBlocked();
            return;
        }
        ZeroMoveParams();
        m_ptNext = m_ptHead;
        return;
    }

    // we're going
    if (m_cMode == moving) {
        ASSERT (m_ptNext != m_ptHead);
        ASSERT (m_ptNext != m_ptTail);
        ASSERT ((theVehicleHex.GetVehicle(m_ptNext) == NULL) || (theVehicleHex.GetVehicle(m_ptNext) == this));
        theVehicleHex.GrabHex(m_ptNext, this);
        SetMoveParams(FALSE);
        return;
    }

    // not moving
    if ((m_ptNext != m_ptHead) && (theVehicleHex._GetVehicle(m_ptNext) == this))
        theVehicleHex.ReleaseHex(m_ptNext, this);
    ZeroMoveParams();
    m_ptNext = m_ptHead;

    ASSERT_VALID (this);
}

// see if it's a bogus grab
void CVehicleHex::CheckHex(CSubHex const &_sub) {

    CVehicle *pVeh = GetVehicle(_sub);
    if (pVeh == NULL)
        return;

    if (!pVeh->GetHexOwnership()) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "*** Vehicle %d at m_cOwn == 0, owns (%d,%d)", pVeh->GetID(), _sub.x,
                  _sub.y);
#endif
        TRAP();
        ASSERT (FALSE);
        ReleaseHex(_sub, pVeh);
        return;
    }

    if ((pVeh->GetPtNext() != _sub) && (pVeh->GetPtHead() != _sub) && (pVeh->GetPtTail() != _sub)) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "*** Vehicle %d at m_cOwn == 1, didn't release (%d,%d)",
                  pVeh->GetID(), _sub.x, _sub.y);
#endif
        TRAP();
        ASSERT (FALSE);
        ReleaseHex(_sub, pVeh);
    }
}

void CVehicle::PostArrivedOrBlocked() {

    m_bFlags |= told_ai_stop;

    if (m_ptDest == m_ptHead) {
        // tell the AI
        if (GetOwner()->IsAI()) {
#ifdef _LOGOUT
            logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Tell AI vehicle %d arrived dest sub (%d,%d)", GetID(),
                      m_ptHead.x, m_ptHead.y);
#endif
            CMsgVehDest msg(this);
            theGame.PostToClient(GetOwner(), &msg, sizeof(msg));
        } else
            // tell our router
        if ((GetData()->IsTransport()) && (!(m_bFlags & hp_controls))) {
            ASSERT (GetOwner()->IsMe());
#ifdef _LOGOUT
            logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Tell HP router vehicle %d arrived dest sub (%d,%d)", GetID(),
                      m_ptHead.x, m_ptHead.y);
#endif
            theGame.m_pHpRtr->MsgArrived(this);
        }

        return;
    }

    // log it
#ifdef _LOGOUT
    logPrintf(LOG_PRI_ALWAYS, LOG_VEH_MOVE, "Vehicle %d permanently blocked n,h,t sub (%d,%d),(%d,%d),(%d,%d)",
              GetID(), m_ptNext.x, m_ptNext.y, m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y);
    logPrintf(LOG_PRI_ALWAYS, LOG_VEH_MOVE,
              "           hexNext: (%d,%d), hexDest: (%d,%d), ptDest (%d,%d), dest type: %d",
              m_hexNext.X(), m_hexNext.Y(), m_hexDest.X(), m_hexDest.Y(), m_ptDest.x, m_ptDest.y,
              theMap.GetHex(m_hexDest)->GetType());
    logPrintf(LOG_PRI_ALWAYS, LOG_VEH_MOVE, "           Path offset: %d, length: %d, retries: %d, block_count: %d",
              m_iPathOff, m_iPathLen, m_iNumRetries, m_iBlockCount);
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            CSubHex _sub(m_ptHead.x + x, m_ptHead.y + y);
            _sub.Wrap();
            CVehicle *pVeh = theVehicleHex._GetVehicle(_sub);
            CBuilding *pBldg = theBuildingHex._GetBuilding(_sub);
            logPrintf(LOG_PRI_ALWAYS, LOG_VEH_MOVE, "           sub (%d,%d), Veh %d, Bldg %d, terrain %d",
                      _sub.x, _sub.y, pVeh == NULL ? 0 : pVeh->GetID(), pBldg == NULL ? 0 : pBldg->GetID(),
                      theMap.GetHex(_sub)->GetType());
        }
#endif

    // tell the AI/router
    if (GetOwner()->IsAI()) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "AI told vehicle %d at sub (%d,%d) permanently blocked", GetID(),
                  m_ptHead.x, m_ptHead.y);
#endif
        CMsgVehGoto msg(this);
        msg.ToErr(theVehicleHex._GetVehicle(m_ptNext));
        theGame.PostToClient(GetOwner(), &msg, sizeof(msg));
    } else if (GetData()->IsTransport()) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "HP router told vehicle %d at sub (%d,%d) permanently blocked",
                  GetID(), m_ptHead.x, m_ptHead.y);
#endif
        theGame.m_pHpRtr->MsgErrGoto(this);
    }
}

static void feCheckSub(CSubHex const &_sub, void *pData) {

    CVehicle *pVeh = theVehicleHex._GetVehicle(_sub);
    if (pVeh == NULL)
        return;

    // transport in if waiting
    CBuilding *pBldg = theBuildingHex._GetBuilding(pVeh->GetHexDest());
    if ((pVeh->GetHexOwnership()) && (pBldg == pData) && (pBldg->GetOwner() == pVeh->GetOwner())) {
        pVeh->ReleaseOwnership();
        pVeh->_SetRouteMode(CVehicle::stop);
        pVeh->ForceAtDest();
        pVeh->ArrivedDest();
    }

    // see if any illegal holds
    theVehicleHex.CheckHex(_sub);

    // if blocked and touching building - make cant_deploy
    if (theBuildingHex._GetBuilding(pVeh->GetPtTail()) == pData)
        if (pVeh->GetRouteMode() == CVehicle::blocked) {
            pVeh->ReleaseOwnership();
            pVeh->_SetRouteMode(CVehicle::cant_deploy);
#ifdef _LOGOUT
            logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) forced to cant_deploy", pVeh->GetID(),
                      pVeh->GetPtHead().x, pVeh->GetPtHead().y);
#endif
        }
}

static int fnEnumCheckHex(CHex *pHex, CHexCoord hex, void *pData) {

    // check for holding not used hexes or dest of building
    if (pHex->GetUnits() & CHex::veh) {
        CSubHex _sub(hex.X() * 2, hex.Y() * 2);
        feCheckSub(_sub, pData);
        _sub.x++;
        feCheckSub(_sub, pData);
        _sub.y++;
        feCheckSub(_sub, pData);
        _sub.x--;
        feCheckSub(_sub, pData);
    }

    return (FALSE);
}

static void _CheckBldg(CBuilding *pBldg) {

    CHexCoord _hex(pBldg->GetHex());
    _hex.Xdec();
    _hex.Ydec();
    theMap.EnumHexes(_hex, pBldg->GetCX() + 2, pBldg->GetCY() + 2, fnEnumCheckHex, pBldg);
}

void CVehicle::CheckAroundBuilding() {

    // are we in or going to a building?
    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptNext);
    if (pBldg != NULL) {
        _CheckBldg(pBldg);
        return;
    }
    if ((pBldg = theBuildingHex._GetBuilding(m_ptHead)) != NULL) {
        _CheckBldg(pBldg);
        return;
    }
    if ((pBldg = theBuildingHex._GetBuilding(m_ptTail)) != NULL) {
        _CheckBldg(pBldg);
        return;
    }
    if ((pBldg = theBuildingHex._GetBuilding(m_ptDest)) != NULL) {
        _CheckBldg(pBldg);
        return;
    }

}

void CVehicle::ForceAtDest() {

    ASSERT (!m_cOwn);

    m_ptHead = m_ptTail = m_ptNext = m_ptDest;
    if (!(GetData()->GetVehFlags() & CTransportData::FL1hex)) {
        // in same hex
        m_ptTail.x = (m_ptHead.x & ~1) | ((m_ptHead.x + 1) & 1);
        m_ptTail.y = (m_ptHead.y & ~1) | ((m_ptHead.y + 1) & 1);
    }
    m_hexNext = m_hexDest;

    SetMoveParams(TRUE);
    AtNewLoc();
}

// call this when we have put the vehicle at a new location
void CVehicle::AtNewLoc() {

    if (!GetOwner()->IsLocal())
        return;

    CMsgVehGoto msg(this);
    theGame.PostToClientByNet(theGame.GetMyNetNum(), &msg, sizeof(msg));

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
    else
        {
        TRAP (theVehicleHex._GetVehicle (m_ptHead) == this);
        TRAP (theVehicleHex._GetVehicle (m_ptTail) == this);
        }
#endif
}
