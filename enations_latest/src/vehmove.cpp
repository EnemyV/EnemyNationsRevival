//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "enprobes.h"
#include "stdafx.h"
#include "lastplnt.h"
#include "chproute.hpp"
#include "event.h"
#include "cpathmgr.h"
#include "player.h"
#include "area.h"
#include "bridge.h"
#include "SDL2RouteWindow.h"   // refresh the route window as the queue progresses/consumes

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
#ifdef STRICTER_ASSERTS2
    // i *think* this happens when a vehicle cant path to its destination, like if its trying to go to an island or something
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

    // operations this turn. vehicle_speed research scales the base move rate
    // (GetSpeedPct: 100% + 2% per level), so the owner's teched vehicles move faster.
    float fSpeedMul = (float) GetOwner()->GetSpeedPct() / 100.0f;
    // Turbochargers edict: +move speed for fuel-consuming units (infantry/walk unaffected).
    if (GetData()->GetWheelType() != CWheelTypes::walk)
        fSpeedMul *= GetOwner()->GetEdictMoveMult();
    m_fVehMove += (m_fDamPerfMult * 0.9 * (float) (theGame.GetOpersElapsed() * GetData()->GetSpeed()) * fSpeedMul) /
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
        if (m_ptNext != m_ptHead)
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
            // BUGBUG i think its ok to have stop here? -vtier
            ASSERT ((m_cMode == blocked) || (m_cMode == stop));
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
    // A backing truck follows the body's turn too; only its nose is reversed.
    m_iDir += iStep * m_iDadd;
    m_iDir = __roll(0, FULL_ROT, m_iDir);

    if (GetTurret())
        GetTurret()->m_iDir = __roll(0, FULL_ROT, GetTurret()->m_iDir + iStep * m_iTadd);

    return (bDone);
}

// We've made it to m_hexNext - what next?
void CVehicle::ArrivedNextHex() {

    // we moved, so we are entitled to wait again at the next bump
    m_bWaitedForMover = FALSE;

#ifdef _LOGOUT
    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d arrived next sub (%d,%d)", GetID(), m_ptHead.x, m_ptHead.y);
#endif

    ASSERT_VALID (this);
    ASSERT (theGame.IsNetGame() || GetOwner()->IsLocal());  // MP: client legitimately simulates remote units (Task#14)
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
// Put the vehicle back on the job traffic moved it off. Shared by the immediate
// rejoin and by the timed hold at the end of a retreat, so both spend the same
// retreat budget and name the same destination.
// Is this sub-hex out of everyone's way - off the span AND off the pavement?
BOOL CVehicle::ClearOfRoad(CSubHex const &_sub) {

    if (theMap._GetHex(_sub)->GetUnits() & CHex::bridge)
        return (FALSE);
    return (!OnPavement(_sub));
}

// Failure to reach a temporary parking target is not failure of the saved haul.
// At a settled, safe pose we can finish the detour and use the normal hold/rejoin.
BOOL CVehicle::FinishClearDetour() {
    if (!m_bResume || !(m_bReversing || m_bForwardEscape || m_iHoldFrames > 0) ||
        !JamEligible() || GetTransport() != NULL || m_iStepsLeft != 0)
        return (FALSE);
    CHex *pHead = theMap._GetHex(m_ptHead);
    CHex *pTail = theMap._GetHex(m_ptTail);
    if (!ClearOfRoad(m_ptHead) || !ClearOfRoad(m_ptTail) ||
        ((pHead->GetUnits() | pTail->GetUnits()) & CHex::bldg) ||
        !GetData()->CanTravelHex(pHead) || !GetData()->CanTravelHex(pTail))
        return (FALSE);
    WaitLog("[PARK-HERE] veh %d failed parking %d,%d; clear at head %d,%d tail %d,%d, job %d,%d",
            GetID(), m_ptDest.x, m_ptDest.y, m_ptHead.x, m_ptHead.y,
            m_ptTail.x, m_ptTail.y, m_subResume.x, m_subResume.y);
    if (m_ptNext != m_ptHead && m_ptNext != m_ptTail &&
        theVehicleHex._GetVehicle(m_ptNext) == this)
        theVehicleHex.ReleaseHex(m_ptNext, this);
    m_ptNext = m_ptDest = m_ptHead;
    m_hexDest = m_ptHead.ToCoord();
    ArrivedDest();
    return (TRUE);
}

BOOL CVehicle::ResumeJob() {

    if (!m_bResume)
        return (FALSE);

    // A hold with time left on it outranks a rejoin. Without this a courtesy nudge
    // mid-hold ended with the truck arriving, resuming its haul and driving straight
    // back into the gap it had just made - the second hold leak WinAstra found.
    if (m_iHoldFrames > 0)
        return (FALSE);

    m_bResume     = FALSE;

    // EXACT sub, not the hex. Arriving anywhere in the goal hex used to count
    // as already-there and skip the resume entirely, which silently dropped
    // every job whose destination named a particular sub.
    if (m_subResume == m_ptHead)
        return (FALSE);

    WaitLog("[REJOIN] veh %d back at hex %d,%d, resuming dest sub %d,%d", GetID(),
            GetHexHead().X(), GetHexHead().Y(), m_subResume.x, m_subResume.y);
    CSubHex _dest(m_subResume);
    // Resuming the SAME job is not a fresh order, so it must not hand back a
    // fresh retreat budget. It did: SetDestAndMode clears m_iBackUps, so the
    // cap of 2 was defeated by the cycle retreat-retreat-rejoin-repeat, which
    // is 1384 retreat events across 57 trucks in eight minutes.
    int iBudget = m_iBackUps;
    SetDestAndMode(_dest, (VEH_POS) m_iResumeMode);
    m_iBackUps = iBudget;
    return (TRUE);
}

void CVehicle::ArrivedDest() {

#ifdef _LOGOUT
    logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d arrived dest sub (%d,%d)", GetID(), m_ptHead.x, m_ptHead.y);
#endif

    ASSERT ((m_ptHead == m_ptDest) || (m_iEvent == load));
#ifdef STRICTER_ASSERTS
    ASSERT ((m_cMode != moving) || (theVehicleHex.GetVehicle(m_ptNext) == this));
#endif

    // Keep reverse geometry until any tail-clear step has finished. The final
    // arrival restores forward endpoint labels before holding or resuming.
    BOOL bWasReversing = m_bReversing;
    BOOL bWasForwardEscape = m_bForwardEscape;
    m_bForwardEscape = FALSE;

    // A RETREAT has ended. Hold here before rejoining the haul: the whole point of
    // backing off is to give the trucks in front somewhere to go, and one that
    // arrives and drives straight back in has given them nothing. Park quietly -
    // no PostArrivedOrBlocked, so the router does not re-task us mid-hold - and let
    // Operate() put us back on the job when the clock runs out. The per-truck offset
    // stops a queue that retreated together from re-entering together.
    //
    // Only where we are ACTUALLY out of the way. A partial retreat ends on the deck,
    // and holding there would free the truck in front at the price of parking on the
    // span in front of everyone behind us - so a partial retreat rejoins at once and
    // only a full one, off the span and off the pavement, earns the hold.
    // THE WHOLE BODY has to be clear, not just the head. Testing the head alone let a
    // truck hold with its tail still lying across the roadway, which is the one thing
    // the hold exists to stop.
    BOOL bClear = ClearOfRoad(m_ptHead) && ClearOfRoad(m_ptTail);
    // A safe destination for the head may leave the tail across the road.
    // Finish with one legal local step: its tail will occupy our current safe
    // head square. Keep the saved job and start/continue the hold only once clear.
    if ((TrafficOpts() & 8) && !bClear && ClearOfRoad(m_ptHead) &&
        (((bWasReversing || bWasForwardEscape) && m_bResume) || m_iHoldFrames > 0)) {
        const int turns[] = { 0, -1, 1, -2, 2, -3, 3 };
        for (int i = 0; i < 7; ++i) {
            CSubHex next = Rotate(turns[i]);
            if (next == m_ptHead || !ClearOfRoad(next) ||
                (theMap._GetHex(next)->GetUnits() & CHex::bldg) || !CanEnter(next))
                continue;
            if (m_iHoldFrames <= 0)
                m_iHoldFrames = HOLD_FRAMES + (int) (GetID() % 4) * 24;
            WaitLog("[TAIL-CLEAR] veh %d head %d,%d tail %d,%d taking sub %d,%d, hold %d",
                    GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                    next.x, next.y, m_iHoldFrames);
            m_bReversing = bWasReversing;
            m_bForwardEscape = bWasForwardEscape;
            DetourTo(next, FALSE);
            return;
        }
    }
    if (bWasReversing) {
        EndReverse();
        // This temporary detour has arrived; the saved job still names its
        // original destination. Its arrival point follows the relabelled head.
        m_ptDest = m_ptHead;
        m_hexDest = m_ptHead.ToCoord();
    }
    if ((bWasReversing || bWasForwardEscape) && bClear && m_bResume && (TrafficOpts() & 8))
        m_iHoldFrames = HOLD_FRAMES + (int) (GetID() % 4) * 24;

    // Park for whatever is LEFT of the hold - whether we just armed it, or a courtesy
    // request moved us part way through one and this is where we ended up.
    //
    // BUT ONLY IF THIS POSE IS ACTUALLY CLEAR. bClear used to gate only the arming of
    // a NEW hold, so a courtesy step taken part way through an existing one could set
    // the truck down again across the roadway and leave it there for the rest of the
    // deadline - a truck parked on the bridge, which is the one thing the hold exists
    // to prevent. WinAstra's review48 defect. If we are not clear we do not park:
    // fall through to the ordinary arrival path, which includes the on-stop check
    // that moves an idle truck off the road.
    if ((m_iHoldFrames > 0) && (!bClear))
        m_iHoldFrames = 0;

    if (m_iHoldFrames > 0) {
        WaitLog("[HOLD] veh %d holding at hex %d,%d head %d,%d tail %d,%d for %d frames, then dest sub %d,%d",
                GetID(), GetHexHead().X(), GetHexHead().Y(), m_ptHead.x, m_ptHead.y,
                m_ptTail.x, m_ptTail.y, m_iHoldFrames, m_subResume.x, m_subResume.y);
        _SetRouteMode(stop);

        // AND STAY SILENT. Leaving PostArrivedOrBlocked out of this function was not
        // enough: _SetRouteMode(stop) CLEARS told_ai_stop (unit.cpp:3612), and the
        // next Operate's ordinary stop branch then posts the arrival itself
        // (vehicle.cpp:246, commented "backup method"). The router re-tasked the truck
        // within a frame - WinAstra measured the rejoin 43ms into a 10s hold. Setting
        // the flag AFTER _SetRouteMode is what actually suppresses the notification.
        m_bFlags |= told_ai_stop;

        SetMoveParams(FALSE);
        ZeroMoveParams();
        DeletePath();
        return;
    }

    // A traffic detour has finished. Put the vehicle back on the job it was doing
    // before we moved it out of the way, instead of leaving it parked here.
    if (ResumeJob())
        return;

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

    // ON STOP: never come to rest in the roadway. One test at the moment a vehicle
    // stops, which is far cheaper than having every passer-by scan its neighbours,
    // and it prevents the obstacle instead of clearing it up afterwards.
    //
    // This block MUST stay clear of the arrival if/else above: putting it between
    // them silently rebound that `else` to this predicate, which set told_ai_stop on
    // ordinary arrivals and dropped the repair-arrival suppression it was written
    // for - a behaviour change that applied even with every traffic rule disabled.
    //
    // Trucks and cranes only: military units park where the player put them.
    // Never when a human sent it here, and never when it was ORDERED to stand still
    // (CUnit::StopUnit sets the existing `stopped` flag, unit.h:500).
    //
    // Being inside a building is NOT a reason to move: EnterBuilding already calls
    // ReleaseOwnership, so such a vehicle occupies no road hex and blocks nobody.
    if ((m_cMode == stop) && (!IsHpControl()) && (!IsFlag(stopped)) &&
        (pBldgDest == NULL) && (GetData()->IsTransport() || GetData()->IsCrane()))
        if (OnPavement(m_ptHead))
            LeaveRoad();

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

            // advance to the next stop
            if (m_bRouteLoop) {
                // looping: cycle the position, keep every stop in the list
                if (m_pos == NULL)
                    m_pos = m_route.GetHeadPosition();
                else {
                    m_route.GetNext(m_pos);
                    if (m_pos == NULL)
                        m_pos = m_route.GetHeadPosition();   // loop back to the start
                }
            } else {
                // (F1) one-shot: CONSUME the stop we just reached — remove it so the queue
                // shrinks as the vehicle progresses, then move on to the new head.
                if (m_pos != NULL) {
                    CRoute *pReached = m_route.GetAt(m_pos);
                    m_route.RemoveAt(m_pos);
                    delete pReached;
                }
                m_pos = m_route.GetHeadPosition();
            }

            // reflect the progression / consumption in an open route window
            if (m_pSdlRoute != NULL)
                m_pSdlRoute->RefreshRoute();

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
            } else {
                // one-shot route complete (queue empty) — halt here.
                SetRoutePos(NULL);
                m_iEvent = none;
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
                    // it's ours, it's in a building, and it's stopped (not unloading/deploying, etc).
                    // Skip other boats: a cargo ship must never load another ship (cargo ship,
                    // gunboat, ...) — that nests carriers and gets them stuck. Boats carry land cargo only.
                    if ((pVeh != this) && (pVeh->GetTransport() == NULL) && (pVeh->GetOwner()->IsMe()) &&
                        (!pVeh->GetData()->IsBoat()) &&
                        (!pVeh->GetHexOwnership()) && (pVeh->GetRouteMode() == CVehicle::stop))
                        if (pBldgDest == theBuildingHex._GetBuilding(pVeh->GetPtHead())) {
                            int iSize;
                            if (pVeh->GetData()->IsPeople())
                                iSize = 1;
                            else
                                iSize = MAX_CARGO;
                            if (m_iCargoSize + iSize > GetEffPeopleCarry())
                                continue;

                            // put it on the boat
                            pVeh->SetTransport(this);

                            // see if we're full
                            if (m_iCargoSize >= GetEffPeopleCarry())
                                break;
                        }
                }
                break;
            }

            // if we're controlling the truck
            if ((GetData()->IsTransport()) && ((m_bFlags & hp_controls) != 0))
                ShowLoadDialog();
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
                (m_pVehLoadOn->m_iCargoSize + iAdd > m_pVehLoadOn->GetEffPeopleCarry())) {
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
        // BUGBUG this often gets here with stopped, but i think its ok -vtier
        ASSERT( ( m_cMode == blocked ) || ( m_cMode == stop) );  // or stopped?
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

            // A carried unit should own no road squares. Bound this diagnostic
            // across all carriers; it must not turn a bad cargo state into log spam.
            if (pVeh->m_cOwn) {
                static DWORD lastCargoOwnedLog = 0;
                DWORD now = theGame.GettimeGetTime();
                if (now - lastCargoOwnedLog >= 1000) {
                    lastCargoOwnedLog = now;
                    WaitLog("[CARGO-OWNED] carrier %d cargo %d transport %d mode %d head %d,%d next %d,%d resume %d hold %d",
                            GetID(), pVeh->GetID(), pVeh->GetTransport() ? pVeh->GetTransport()->GetID() : 0,
                            (int)pVeh->m_cMode, pVeh->m_ptHead.x, pVeh->m_ptHead.y,
                            pVeh->m_ptNext.x, pVeh->m_ptNext.y, (int)pVeh->m_bResume, pVeh->m_iHoldFrames);
                }
            }

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

    // Arrival and blocked movement can leave next==head. There is no step to
    // interpolate: animating it pulls the tail into the head and invents a turn.
    if (m_ptNext == m_ptHead)
        ZeroMoveParams();

    ASSERT_VALID_LOC (this);
    ASSERT ((!m_cOwn) || (theVehicleHex.GetVehicle(m_ptHead) == this));
    ASSERT ((!m_cOwn) || (theVehicleHex.GetVehicle(m_ptTail) == this));
}

// this gets the next hex for the vehicle - returns TRUE if it could do it
BOOL CVehicle::GetNextHex(BOOL bNew) {

    ASSERT_VALID (this);
    ASSERT (theGame.IsNetGame() || GetOwner()->IsLocal());  // MP: client legitimately simulates remote units (Task#14)
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
            int oldX = xStep, oldY = yStep;
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
            if (m_bReversing && (oldX != xStep || oldY != yStep)) {
                CSubHex straight(m_ptHead.x + oldX, m_ptHead.y + oldY);
                CSubHex shifted(m_ptHead.x + xStep, m_ptHead.y + yStep);
                straight.Wrap(); shifted.Wrap();
                if (CanEnter(straight) && !CanEnter(shifted)) {
                    CVehicle* on = theVehicleHex._GetVehicle(shifted);
                    WaitLog("[REVERSE-LANE-KEPT] veh %d head %d,%d tail %d,%d free %d,%d instead of %d,%d blocker %d",
                            GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                            straight.x, straight.y, shifted.x, shifted.y, on ? on->GetID() : 0);
                }
                // Backing up keeps the lane we occupied; it does not cross to
                // the lane for vehicles driving forward in the other direction.
                xStep = oldX;
                yStep = oldY;
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
        // PROBE. Counting how often a step fails says nothing about WHY, and "why"
        // is the open question: is the mouth blocked by vehicles, or by geometry a
        // hull cannot take? Record the whole failing condition once per vehicle per
        // 5 s - our body and intended step, our live job and whether we are mid
        // detour, and either the blocker's identity and state or the terrain verdict.
        if (GetOwner()->IsMe() && (theGame.GettimeGetTime() - m_dwBlockLog > 5000)) {
            m_dwBlockLog = theGame.GettimeGetTime();
            CVehicle *pIn = theVehicleHex._GetVehicle(m_ptNext);
            if (pIn != NULL)
                WaitLog("[BLOCK] veh %d head %d,%d tail %d,%d next %d,%d dir %d mode %d dest %d,%d "
                        "resume %d | OCCUPIED by veh %d mode %d head %d,%d tail %d,%d next %d,%d",
                        GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y, m_ptNext.x,
                        m_ptNext.y, m_iDir, m_cMode, m_ptDest.x, m_ptDest.y, m_bResume ? 1 : 0,
                        pIn->GetID(), pIn->m_cMode, pIn->m_ptHead.x, pIn->m_ptHead.y, pIn->m_ptTail.x,
                        pIn->m_ptTail.y, pIn->m_ptNext.x, pIn->m_ptNext.y);
            else
                WaitLog("[BLOCK] veh %d head %d,%d tail %d,%d next %d,%d dir %d mode %d dest %d,%d "
                        "resume %d | IMPASSABLE terraincost %d",
                        GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y, m_ptNext.x,
                        m_ptNext.y, m_iDir, m_cMode, m_ptDest.x, m_ptDest.y, m_bResume ? 1 : 0,
                        theMap.GetTerrainCost(CHexCoord(m_ptNext.ToCoord()), GetHexHead(),
                                              CalcNextBaseDir(), GetData()->GetWheelType()));
        }

        // WAIT FIRST, go around second. If the thing in our way is a vehicle that
        // is itself moving it will very likely be gone before a detour would even
        // finish, and sidestepping out of lane is what turns a queue into a jam.
        // Hold position and keep the lane; when the wait expires we drop into
        // blocked and the go-around ladder runs exactly as it did before.
        if (WaitForMover())
            return (FALSE);

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
                // Escaping a jam can require moving away from a corner before
                // the route becomes reachable. Keep the chosen legal step.
                BOOL bEscape = m_bResume && (m_bReversing || m_bForwardEscape) && JamEligible();
                if ((iOld <= iNew) && (iNew != 0) && bEscape)
                    WaitLog("[ESCAPE-STEP] veh %d source next head %d,%d tail %d,%d next %d,%d "
                            "hexnext %d,%d distance %d to %d times %d",
                            GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                            m_ptNext.x, m_ptNext.y, m_hexNext.X(), m_hexNext.Y(), iOld, iNew, m_iTimesOn);
                if ((iOld <= iNew) && (iNew != 0) && !bEscape) {
                    m_ptNext = m_ptHead;
                    if (!FindSub(TRUE)) {
                        if (GetOwner()->IsMe() && m_bResume && m_iHoldFrames > 0)
                            WaitLog("[PARK-CIRCLE] veh %d source next head %d,%d tail %d,%d hexnext %d,%d "
                                    "retry %d visits %d hold %d",
                                    GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                                    m_hexNext.X(), m_hexNext.Y(), m_iNumRetries, m_iTimesOn, m_iHoldFrames);
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

// Which sub-lane should we be in?
//
// A hex is 2x2 sub-hexes, so a one-hex-wide bridge deck is really two lanes
// side by side. When both directions share a lane the traffic wait becomes
// symmetric - each vehicle politely holds for the other and neither yields -
// which is a deadlock, not a queue (2074 mutual waits measured on the v9 save).
// Tying the lane to the direction of travel puts the two directions in
// different sub-rows so they can pass each other.
//
// Returns TRUE if _next is in the lane we should be using, or if no lane
// applies here (we only do this on the bridge deck, where the corridor is
// physically one hex wide).
BOOL CVehicle::InLane(CSubHex const &_next) {

    if (!(TrafficOpts() & 16))
        return (TRUE);

    if (!(theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge))
        return (TRUE);
    if (!(theMap._GetHex(_next)->GetUnits() & CHex::bridge))
        return (TRUE);

    if (m_bReversing) {
        if (m_ptHead.y == m_ptTail.y)
            return (_next.y == m_ptHead.y);
        if (m_ptHead.x == m_ptTail.x)
            return (_next.x == m_ptHead.x);
        return (TRUE); // an angled hull still needs room to straighten
    }

    // Which way are we travelling? Not m_hexNext - that is the NEXT STEP, it is
    // rewritten by every go-around and it points backwards for a whole step after
    // a reversal, so half the lane answers on a busy span were about a direction
    // the vehicle was not going. The DESTINATION is stable for the whole journey.
    //
    // Reversing keeps the current lane above. The forward-driving preference
    // below must not send a retreat across the oncoming lane.
    int dx = CSubHex::Diff(m_hexDest.X() * 2 - m_ptHead.x);
    int dy = CSubHex::Diff(m_hexDest.Y() * 2 - m_ptHead.y);

    // the lane is the sub-row ACROSS our direction of travel
    if (abs(dx) >= abs(dy)) {
        if (dx == 0)
            return (TRUE);
        return ((_next.y & 1) == ((dx > 0) ? 1 : 0));
    }
    if (dy == 0)
        return (TRUE);
    // +Y belongs at even X and -Y at odd X, matching the convention the ordinary
    // stepping path already uses. Having avoidance disagree with stepping puts the
    // two halves of the same vehicle on different conventions.
    return ((_next.x & 1) == ((dy > 0) ? 0 : 1));
}

// look for the next m_ptNext
//   return TRUE if found
// bCloser - new location must be closer
//
// Two passes: prefer a sub-hex in our own lane, and only if there is none fall
// back to considering every direction as before. That is a PREFERENCE, not a
// restriction - nothing that used to be reachable becomes unreachable.
BOOL CVehicle::FindSub(BOOL bCloser) {

    // Refusing to overtake must retain the real blocked step for waiting and
    // clearance requests, rather than leave a rejected free side-step or self.
    CSubHex blockedStep;
    if (BlockedLaneStep(blockedStep)) {
        m_ptNext = blockedStep;
        return (FALSE);
    }

    if (FindSubEx(bCloser, TRUE))
        return (TRUE);
    return (FindSubEx(bCloser, FALSE));
}

BOOL CVehicle::FindSubEx(BOOL bCloser, BOOL bLane) {

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

    // NO SHARP TURNS IN A CONFINED PLACE. The range above offers Rotate(+-2), which is
    // +-90 degrees in one step out of eight facings - so two steps reverse the hull and
    // the truck has turned round on the bridge WITHOUT ever calling Turn180. Blocking
    // the turn-around rung did nothing about this, which is why they still pirouette.
    // Here a truck may go forward or reverse, not swing about.
    // ...but ONLY once the hull is already lined up with it. m_iCorrLen is how far the
    // corridor ran along OUR OWN axis, so a large value means we point along the deck
    // and have no business swinging about - while a truck sitting SIDEWAYS across the
    // deck scores near zero, and THAT truck must keep its sharp step, because reversing
    // moves a hull along its own axis and a sideways one would only back into the water.
    // Turns that line you up stay legal; turns that swing you off the axis do not.
    if ((m_bConfined || (theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge)) &&
        (m_iCorrLen >= 2)) {
        if (iDir < -1) iDir = -1;
        if (iMax >  1) iMax =  1;
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

        // first pass: stay in our own lane
        if (bLane && (!InLane(_next)))
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

    // A WATER vehicle is ALWAYS on the water — it passes UNDER bridges, never on the
    // deck. The A* path gates (CPathMgr::CanEnterBridge / GetCellCosts) already force
    // on-water from the wheel type so a boat can route under a span; mirror that here at
    // runtime. The mutable on_water FLAG is unreliable at a bridge-over-water hex (it
    // gets cleared as the boat steps onto the span), which left CanEnterHex re-imposing
    // the bridge deck-direction checks and stalling the boat at the bridge. Wheel type
    // is authoritative. CanEnterHex still calls CanTravelHex, so a boat is never let onto
    // land/onto an out-of-water bridge support by this.
    BOOL bOnWater = IsOnWater() || ( GetData()->GetWheelType() == CWheelTypes::water );
    return (GetData()->CanEnterHex(m_ptHead, _sub, bOnWater, bStrict));
}

// Identify the actual vehicle ahead when an aligned hull cannot overtake.
// No state changes: callers use the same step for lane policy and recovery.
BOOL CVehicle::BlockedLaneStep(CSubHex &blockedStep) {
    if (!(TrafficOpts() & 16) || !m_cOwn || IsHpControl() ||
        !(GetData()->IsTransport() || GetData()->IsCrane()) || GetData()->IsBoat())
        return (FALSE);

    int dx = CSubHex::Diff(m_ptHead.x - m_ptTail.x);
    int dy = CSubHex::Diff(m_ptHead.y - m_ptTail.y);
    if ((dx == 0) == (dy == 0)) // angled hulls must be allowed to straighten
        return (FALSE);
    CSubHex ahead(m_ptHead.x + dx, m_ptHead.y + dy);
    ahead.Wrap();
    CVehicle *blocker = theVehicleHex._GetVehicle(ahead);
    if (blocker == NULL || blocker == this)
        return (FALSE);

    BOOL confined = (theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge) &&
                    (theMap._GetHex(ahead)->GetUnits() & CHex::bridge);
    if (!confined && OnPavement(m_ptHead) && OnPavement(ahead)) {
        CHexCoord here(m_ptHead);
        CHexCoord left(here.X() + dy, here.Y() - dx);
        CHexCoord right(here.X() - dy, here.Y() + dx);
        left.Wrap(); right.Wrap();
        BOOL leftClosed = (theMap._GetHex(left)->GetUnits() & CHex::bldg) ||
            theMap.GetTerrainCost(left, left, 0, GetData()->GetWheelType()) == 0;
        BOOL rightClosed = (theMap._GetHex(right)->GetUnits() & CHex::bldg) ||
            theMap.GetTerrainCost(right, right, 0, GetData()->GetWheelType()) == 0;
        confined = leftClosed && rightClosed;
    }
    if (!confined)
        return (FALSE);
    blockedStep = ahead;
    return (TRUE);
}

// return TRUE if can enter sub-hex
BOOL CVehicle::CanEnter(CSubHex const &_sub, BOOL bStrict) {
    if (theVehicleHex._GetVehicle(_sub) != NULL)
        return (FALSE);

    int dx = CSubHex::Diff(m_ptHead.x - m_ptTail.x);
    int dy = CSubHex::Diff(m_ptHead.y - m_ptTail.y);
    int sx = CSubHex::Diff(_sub.x - m_ptHead.x);
    int sy = CSubHex::Diff(_sub.y - m_ptHead.y);
    CSubHex blockedStep;
    if (sx * dy != sy * dx && BlockedLaneStep(blockedStep))
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

    // Reverse swaps the movement head/tail, so the nose is half a turn from
    // that axis. Derive it each step: pinning an old bearing made a truck slide
    // sideways when its route curved. Straight backing keeps the same facing.
    if (GetData()->GetVehFlags() & CTransportData::FL1hex) {
        if (m_ptNext != m_ptHead)
            m_iDir = CalcNextDir();
    } else
        m_iDir = __roll(0, FULL_ROT, CalcDir() + (m_bReversing ? FULL_ROT / 2 : 0));

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

#ifdef STRICTER_ASSERTS
    ASSERT (theBuildingHex.GetBuilding(subNext) == NULL);
#endif
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

    // POSTCONDITION: an in-building vehicle must never remain in an outside
    // movement mode (it owns no hexes). Every normal caller stops the vehicle
    // first, but the blocked-traffic shove (HandleBlocked ~2197) enters a
    // vehicle with its mode untouched -> moving re-grabbed on next Move (18:25
    // crash) and blocked re-grabbed on road-clear (soak26 03:18 crash, same
    // TRAP; traffic/contention share that cycle). cant_deploy is the designed
    // in-building state (mirrors ExitBuilding's own guard) and legitimately
    // redeploys later. stop stays stop (vanilla arrival flow).
    // deploy_it included: a deploy_it vehicle shoved inside (HandleBlocked
    // same-owner shove) kept its mode after ReleaseOwnership, and Operate's
    // deploy_it resume set moving with NO ownership (soak51 MOVNOOWN veh 223,
    // caller Operate+0x9DE). cant_deploy re-enters the designed deploy ladder.
    if (m_cMode == moving || m_cMode == blocked || m_cMode == traffic || m_cMode == contention ||
        m_cMode == deploy_it)
        _SetRouteMode(cant_deploy);

    // put us in the exit slot
    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
    ASSERT (pBldg != NULL);
    if (pBldg != NULL) {
        // 1996 TRAP: entering a foreign, non-allied building. Now a LEGIT state
        // (trucks steal from enemies - fc7fe992; AI jump-rescue lands in dest).
        // Debug-only breakpoint, no-op in Release - log instead so it doesn't
        // kill observation runs.
#if EN_AI_PROBES_ECON && defined(_WIN32)
        if ((pBldg->GetOwner() != GetOwner()) && (GetOwner()->GetRelations() != RELATIONS_ALLIANCE))
        { char szE[80]; sprintf(szE, "[ENTERFOREIGN] veh %lu bldg %lu\n", (unsigned long)GetID(), (unsigned long)pBldg->GetID()); OutputDebugStringA(szE); }
#endif

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

        // entering the dest IS the arrival; the stop-mode backup never runs for
        // an in-building vehicle, so an AI owner was never told (truck waited
        // for the sweep rescue). Humans/HP-router keep their own path.
        if ((!(m_bFlags & told_ai_stop)) && GetOwner()->IsAI())
            PostArrivedOrBlocked();
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
    else
        // 1996 TRAP retired: a surviving path on exit was "impossible" only
        // because the AI wiped it with mid-weld SetDestination spam - with
        // welds no longer cancelled it's a legal state (soak83 crash). The
        // path is already valid for m_hexDest; keep it.
        EN_TRAP_REMOVED( "ExitBuilding: live path to dest survives the visit - using it" );

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
    // NONE for walking. AI cranes on build work are EXEMPT (operator 2026-07-12):
    // gasless bridge/road builds crawled to a visual standstill, and the bridge
    // is a river-split AI's only escape from the no-gas trap - same circularity
    // pontoon-at-start broke
    if ((GetData()->GetWheelType() != CWheelTypes::walk) && (GetOwner()->GetGasHave() <= 0) &&
        !(GetOwner()->IsAI() && (GetData()->GetType() == CTransportData::construction) &&
          (GetEvent() == build_road))) {
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
                // No traversable head hex among the 4 rotations: every candidate is
                // blocked by vehicles/buildings (legitimate congestion, e.g. heavy AI
                // combat). This is a recoverable runtime condition, NOT a bug — fall
                // through to the next exit sub. (Was TRAP(), a debug-only assert that
                // fired spuriously on this legal case and broke Debug-build QA runs.)
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
                        // occupancy check matches the primary exit loop above -
                        // this fallback only tested water, so a unit unloaded
                        // onto an occupied hex (claim-conflict TRAP, 17:24 crash)
                        if (!theMap._GetHex(_dest)->IsWater() &&
                            theVehicleHex._GetVehicle(_dest) == NULL &&
                            theBuildingHex._GetBuilding(_dest) == NULL)
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
        // FINAL pre-grab validation on the EXACT subs TakeOwnership will grab
        // (next/head/tail per the cargo's own FL1hex mapping, not the loop's
        // triplet - the two diverged again, soak30 07:01 cross-claim at a
        // multi-carrier beach). Same lookup as the grab; clash -> defer.
        {
            CVehicle *pCargoPeek = m_lstCargo.GetHead();
            CSubHex _pH, _pT, _pN;
            if (pCargoPeek->GetData()->GetVehFlags() & CTransportData::FL1hex) {
                _pH = _pT = _sub;
                _pN = _dest;
            } else {
                _pT = _sub;
                _pN = _pH = _head;
            }
            CVehicle *pClash = theVehicleHex.GetVehicle(_pN);
            if (pClash == NULL)
                pClash = theVehicleHex.GetVehicle(_pH);
            if (pClash == NULL)
                pClash = theVehicleHex.GetVehicle(_pT);
            // ANY claimant blocks - including this CARRIER's own subs: the
            // grab runs as the CARGO, which owns nothing, so the carrier's
            // claim is foreign to it (soak42 20:08 dump - the != this
            // exemption was the last hole in the checked set)
            if (pClash != NULL) {
#if EN_AI_PROBES_ECON && defined(_WIN32)
                { char szC[144]; sprintf(szC, "[UNLOADCLASH] carrier %lu grab-subs n(%d,%d)/h(%d,%d)/t(%d,%d) occupied by veh %lu dying %d\n",
                        (unsigned long)GetID(), _pN.x, _pN.y, _pH.x, _pH.y, _pT.x, _pT.y,
                        (unsigned long)pClash->GetID(), (int)((pClash->GetFlags() & CUnit::dying) != 0));
                  OutputDebugStringA(szC); }
#endif
                return;   // defer the unload this tick (a continue would re-pick
                          // the same subs without dequeuing = main-thread spin)
            }
        }
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

// Which traffic rules are live, from EN_TRAFFIC (default all on).
//   bit 0 (1)  wait behind a moving blocker + resume the same step
//   bit 1 (2)  two-party yield
//   bit 2 (4)  ask a stopped blocker to move
//   bit 3 (8)  bounded retreat and leaving the road on give-up
//   bit 4 (16) lane preference in FindSub
//   bit 5 (32) path through moving vehicles
//   bit 7 (128) diagnostic: suppress legacy TestStuck teleportation
int TrafficOpts() {

    static int s_iOpts = -1;
    if (s_iOpts < 0) {
        s_iOpts = 63;
        char *p = NULL;
        size_t n = 0;
        if ((_dupenv_s(&p, &n, "EN_TRAFFIC") == 0) && (p != NULL)) {
            s_iOpts = atoi(p);
            free(p);
        }
    }
    return (s_iOpts);
}

// Experimental probe for the wait/resume work: one line per event so a single
// follower can be traced end to end. Inert unless EN_WAIT_LOG names a file.
void WaitLog(const char *fmt, ...) {

    static FILE *s_fp = NULL;
    static int s_tried = 0;

    if (!s_tried) {
        s_tried = 1;
        char *p = NULL;
        size_t n = 0;
        if ((_dupenv_s(&p, &n, "EN_WAIT_LOG") == 0) && (p != NULL)) {
            fopen_s(&s_fp, p, "w");
            free(p);
        }
    }
    if (s_fp == NULL)
        return;

    // Stamp every line with game time. Without it the log cannot answer whether
    // two vehicles are blocking each other AT THE SAME MOMENT, only whether they
    // ever did - and "ever did" is what made my earlier mutual-wait claim wrong.
    fprintf(s_fp, "t=%lu ", (unsigned long) theGame.GettimeGetTime());

    va_list va;
    va_start(va, fmt);
    vfprintf(s_fp, fmt, va);
    va_end(va);
    fputc('\n', s_fp);
    fflush(s_fp);
}

// Hold position behind a vehicle that is still moving, instead of immediately
// looking for a way around it. Returns TRUE if we are waiting this tick.
//
// One wait per bump: m_bWaitedForMover is cleared only when we actually advance
// a sub-hex (ArrivedNextHex) or are given a new destination, so a vehicle cannot
// sit here forever - the wait expires into blocked and the ladder takes over.
BOOL CVehicle::WaitForMover() {

    if (!(TrafficOpts() & 1))
        return (FALSE);

    // remote vehicles are driven by messages, not by this decision
    if (!GetOwner()->IsLocal())
        return (FALSE);

    CSubHex blockedStep;
    if (BlockedLaneStep(blockedStep))
        m_ptNext = blockedStep;
    CVehicle *pVehInWay = theVehicleHex._GetVehicle(m_ptNext);
    if ((pVehInWay == NULL) || (pVehInWay == this))
        return (FALSE);

    // A retreat needs the follower behind it to make room, not keep driving
    // into it. Either participant can notice the contact. Signal the existing
    // clearance behavior; the follower starts its own legal backup on its update.
    if (m_bReversing != pVehInWay->m_bReversing && GetOwner() == pVehInWay->GetOwner()) {
        CVehicle *pRetreat = m_bReversing ? this : pVehInWay;
        CVehicle *pFollower = m_bReversing ? pVehInWay : this;
        int nx = CSubHex::Diff(pFollower->m_ptHead.x - pFollower->m_ptTail.x);
        int ny = CSubHex::Diff(pFollower->m_ptHead.y - pFollower->m_ptTail.y);
        int dx = CSubHex::Diff(pRetreat->m_ptTail.x - pFollower->m_ptHead.x);
        int dy = CSubHex::Diff(pRetreat->m_ptTail.y - pFollower->m_ptHead.y);
        // Reversing swaps movement head/tail, so its nose points tail minus head.
        if (pFollower->JamEligible() && pFollower->m_iJamClear <= 0 &&
            nx == CSubHex::Diff(pRetreat->m_ptTail.x - pRetreat->m_ptHead.x) &&
            ny == CSubHex::Diff(pRetreat->m_ptTail.y - pRetreat->m_ptHead.y) &&
            dx * nx + dy * ny > 0 &&
            (m_ptNext == pVehInWay->m_ptHead || m_ptNext == pVehInWay->m_ptTail)) {
            pFollower->m_iJamClear = pRetreat->m_iJamClear > 0 ? pRetreat->m_iJamClear : JAM_WINDOW_FRAMES;
            pFollower->m_iJamFwd = 0;
            WaitLog("[REVERSE-FOLLOW] retreat %d follower %d nose %d,%d frames %d",
                    pRetreat->GetID(), pFollower->GetID(), nx, ny, pFollower->m_iJamClear);
        }
    }
    // The elapsed counter resets when traffic waiting expires. The per-bump
    // flag does not: use it so an exhausted wait reaches the recovery ladder.
    if (m_bWaitedForMover)
        return (FALSE);

    // TWO-PARTY STANDOFF. If the vehicle in our way wants the square we are
    // standing on, we are each other's obstacle and waiting is symmetric: both
    // hold, both expire, both go around, both re-block. Somebody has to give way.
    //
    // Stable local priority: the LOWER id holds its ground, the higher id yields.
    // Both vehicles evaluate the same comparison and reach opposite conclusions,
    // so the conflict resolves without anyone coordinating them.
    // A reversing truck has already yielded by committing to an escape.
    // Keep waiting for its rear-led step instead of yielding back into recovery.
    if ((!m_bReversing) && (TrafficOpts() & 2) &&
        ((pVehInWay->m_ptNext == m_ptHead) || (pVehInWay->m_ptNext == m_ptTail) ||
         (pVehInWay->m_subWaitNext == m_ptHead) || (pVehInWay->m_subWaitNext == m_ptTail))) {
        if (GetID() > pVehInWay->GetID()) {
            WaitLog("[YIELD] veh %d at hex %d,%d gives way to veh %d", GetID(), GetHexHead().X(),
                    GetHexHead().Y(), pVehInWay->GetID());
            return (FALSE);      // we yield - go around, and recover if that fails
        }
        // we hold: fall through and wait for them to clear out of our way
    }

    // A vehicle STOPPED on the road will never clear on its own, so waiting for it
    // is pointless - ask it to move instead. If it obliges we still wait, because
    // it is now a moving blocker and our own step will open up shortly.
    if (!pVehInWay->IsOnTheMove()) {
        if (!pVehInWay->AskToMove(this))
            return (FALSE);          // it cannot move - go around it as before
    }

    BOOL bPriorWait = m_bWaitedForMover;
    DWORD dwPriorWaitTime = m_dwTimeBlocked;
    m_bWaitedForMover = TRUE;
    m_subWaitNext = m_ptNext;          // the step we intend to take when it clears
    m_ptNext = m_ptHead;
    SetMoveParams(FALSE);
    _SetRouteMode(traffic);
    m_dwTimeBlocked = 0;
    m_dwTrafficWait = TRAFFIC_WAIT_MOVER;
    WaitLog("[WAIT] veh %d hex %d,%d holds sub %d,%d for veh %d prior_wait %d prior_time %lu", GetID(), GetHexHead().X(),
            GetHexHead().Y(), m_subWaitNext.x, m_subWaitNext.y, pVehInWay->GetID(), (int) bPriorWait, (unsigned long) dwPriorWaitTime);
#ifdef _LOGOUT
    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d waiting for moving vehicle %d", GetID(),
              pVehInWay->GetID());
#endif
    return (TRUE);
}

// The slot we are holding for may have cleared. If it has, take that SAME step
// and stay in lane rather than sitting out the rest of the wait. Returns TRUE
// if we resumed.
BOOL CVehicle::ResumeWaitedStep() {

    if ((m_subWaitNext.x < 0) || (m_subWaitNext == m_ptHead))
        return (FALSE);

    if (!CanEnter(m_subWaitNext))
        return (FALSE);

    DWORD _dwHeld = m_dwTimeBlocked;   // capture before we clear it
    m_ptNext = m_subWaitNext;
    m_subWaitNext.x = m_subWaitNext.y = -1;
    SetMoveParams(FALSE);
    CheckNextHex();
    theVehicleHex.GrabHex(m_ptNext, this);
    SetHexDest();
    _SetRouteMode(moving);
    m_dwTimeBlocked = 0;
    WaitLog("[RESUME] veh %d hex %d,%d took sub %d,%d after %lu frames", GetID(), GetHexHead().X(),
            GetHexHead().Y(), m_ptNext.x, m_ptNext.y, (unsigned long) _dwHeld);
    return (TRUE);
}

// Send this vehicle somewhere as a DETOUR, remembering where it was actually
// going so it can carry on afterwards.
//
// SetDestAndMode overwrites m_hexDest, so calling it directly to nudge or back up
// a vehicle silently THROWS AWAY its haul - it shuffles one hex and then sits
// there forever with no job. That is why nudging and backing up did nothing
// visible. Anything that moves a vehicle for traffic reasons must come through
// here.
void CVehicle::DetourTo(CSubHex const &_sub, BOOL bResume) {

    WaitLog("[DETOUR] veh %d head %d,%d tail %d,%d from %d,%d to %d,%d reverse %d resume %d hold %d",
            GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
            m_ptDest.x, m_ptDest.y, _sub.x, _sub.y, (int) m_bReversing, (int) m_bResume, m_iHoldFrames);

    // Save the EXACT destination sub-hex, not its hex. A job that names a
    // particular sub - a lane, a building door - loses that when it is rounded
    // to hex*2 and comes back a different place than it left.
    CSubHex _keep     = m_ptDest;
    int     _keepMode = m_iDestMode;
    BOOL    _arm      = (bResume && (!m_bResume)) ? TRUE : m_bResume;
    if (m_bResume) {
        _keep     = m_subResume;      // already armed - keep the ORIGINAL job
        _keepMode = m_iResumeMode;
    }

    // SetDestAndMode clears the back-up budget, the retreat hold and any pending
    // resume, because a genuinely NEW order deserves a fresh budget and must not
    // drag the vehicle back to a job the player replaced. A detour is not a new
    // order, so it re-arms all three AFTERWARDS - the hold included, or a courtesy
    // move part way through one would silently end it.
    int  iBudget = m_iBackUps;
    int  iHold   = m_iHoldFrames;
    BOOL bRev    = m_bReversing;     // ...and we are still reversing afterwards
    BOOL bForward = m_bForwardEscape;
    // Retargeting a detour continues its existing reverse geometry. Only a
    // replacement order or a finished detour should normalize the endpoints.
    m_bReversing = FALSE;
    SetDestAndMode(_sub, sub);
    m_iBackUps    = iBudget;
    m_iHoldFrames = iHold;

    // SetDestAndMode calculated movement while reverse was temporarily clear.
    // Recompute the facing and turn increment after restoring the movement mode.
    m_bReversing  = bRev;
    m_bForwardEscape = bForward;
    if (bRev)
        SetMoveParams(FALSE);

    if (bResume || _arm) {
        m_subResume   = _keep;
        m_iResumeMode = _keepMode;
        m_bResume     = _arm;
    }
}

// Look for somewhere off the road to sit. Spirals outward from where we are, so a
// vehicle parked in the middle of a bridge will walk right off the span rather
// than shuffle one hex along it and stay in the way.
// TRUE if this hex has water (or anything else we cannot stand on) next door.
// Shoreline is legal to park on but a poor choice: it is one bad step from the
// drink and it clogs the coast road, so we only settle for it if there is nothing
// further inland.
BOOL CVehicle::IsShoreline(CHexCoord const &_hex) {

    for (int iX = -1; iX <= 1; iX++)
        for (int iY = -1; iY <= 1; iY++) {
            if ((iX == 0) && (iY == 0))
                continue;
            CHexCoord _n(_hex.X() + iX, _hex.Y() + iY);
            _n.Wrap();
            if (theMap.GetTerrainCost(_n, _n, 0, GetData()->GetWheelType()) == 0)
                return (TRUE);
        }
    return (FALSE);
}

// Am I in a LONG NARROW corridor, and how crowded is it?
//
// NOT "am I on a bridge". A street with a row of buildings down each side traps a
// truck in exactly the same way, and this is the whole problem class - so the test
// is geometric: walk the travel axis and ask, at each hex, whether we could step
// SIDEWAYS out of it. Both flanks blocked = that hex is confined. Water, buildings
// and impassable ground all count; vehicles do NOT, because they are temporary and
// counted separately in iVehs.
//
// Returns the length in hexes of the confined run starting ahead of us, and sets
// iVehs to the vehicles sitting in it. Zero almost everywhere, which is the point:
// off a corridor this changes nothing.
//
// Cost: CORRIDOR_LOOK_HEXES iterations of three lookups (two flanks plus one vehicle
// test) - about 36 for the current value. It is called ONLY when a vehicle gives up,
// and LeaveRoad is gated to once per 30s per vehicle, so fleet-wide that is a couple
// of calls a second. It must NEVER move into a per-frame path: there it would be 57
// trucks x 36 lookups x 24 fps to answer a question that changes on the scale of
// seconds.
int CVehicle::CorridorAhead(int &iVehs) {

    iVehs = 0;

    // Heading comes from the SUB axis, not the hex axis. Both ends of a two-sub hull
    // usually sit inside ONE hex, so hex-minus-hex was zero most of the time and the
    // whole detector silently answered "no corridor" - WinAstra's review48 defect.
    int sdx = CSubHex::Diff(m_ptHead.x - m_ptTail.x);
    int sdy = m_ptHead.y - m_ptTail.y;
    if ((sdx == 0) && (sdy == 0))
        return (0);

    int      iRun = 0;
    CSubHex  _at(m_ptHead);
    CHexCoord _prev(GetHexHead());

    // Walk in SUBS - two to a hex - and judge each new hex once.
    for (int iStep = 0; iStep < CORRIDOR_LOOK_HEXES * 2; iStep++) {
        _at.x += sdx;
        _at.y += sdy;
        _at.Wrap();

        CHexCoord _on(_at);
        _on.Wrap();
        if ((_on.X() == _prev.X()) && (_on.Y() == _prev.Y()))
            continue;                       // still inside the hex we already judged

        // hex-level heading, from the hex we just left to this one
        int adx = CHexCoord::Diff(_on.X() - _prev.X());
        int ady = _on.Y() - _prev.Y();
        _prev = _on;
        if ((adx == 0) && (ady == 0))
            continue;

        // off the end of the world stops the walk, not the corridor
        if ((_on.Y() < 0) || (_on.Y() >= theMap.Get_eY()))
            break;

        // can we stand here at all? if not we are looking past the corridor's end
        if (theMap.GetTerrainCost(_on, _on, 0, GetData()->GetWheelType()) == 0)
            break;

        // the two flanks, perpendicular to travel
        CHexCoord _l(_on.X() + ady, _on.Y() - adx);
        CHexCoord _r(_on.X() - ady, _on.Y() + adx);
        _l.Wrap();
        _r.Wrap();
        BOOL bL = (theMap.GetTerrainCost(_l, _l, 0, GetData()->GetWheelType()) != 0) &&
                  (!(theMap._GetHex(CSubHex(_l))->GetUnits() & CHex::bldg));
        BOOL bR = (theMap.GetTerrainCost(_r, _r, 0, GetData()->GetWheelType()) != 0) &&
                  (!(theMap._GetHex(CSubHex(_r))->GetUnits() & CHex::bldg));

        if (bL || bR)
            break;               // there is a way out sideways here - not confined

        iRun++;

        // DENSITY, not one corner. A hex is 2x2 subs and holds up to four vehicles,
        // so sampling only the even/even sub answered a different question than the
        // one being asked - also WinAstra's.
        for (int iSx = 0; iSx < 2; iSx++)
            for (int iSy = 0; iSy < 2; iSy++) {
                CSubHex _s(_on.X() * 2 + iSx, _on.Y() * 2 + iSy);
                _s.Wrap();
                if (theVehicleHex._GetVehicle(_s) != NULL)
                    iVehs++;
            }
    }
    return (iRun);
}

BOOL CVehicle::FindOffRoadSpot(CSubHex &_found, CVehicle *pAsker) {

    CHexCoord _hexOn(GetHexHead());

    // GET CLEAR OF THE JAM, not just off the tarmac. Parking at the first free spot
    // puts the truck beside the mouth it just left, so the moment the router re-tasks
    // it, it is back in the same queue - measured: 14 trucks leave the bridge region
    // in 8 minutes and 13 of them come back. The escape distance is therefore derived
    // from the corridor itself, which needs no per-truck retry counter to track: as
    // far out as the span is long. Off the corridor SpanAhead is 0 and this is exactly
    // the search it always was.
    // ...and how far is "clear"? As far as the corridor is long - but only when this
    // really is a long, narrow, CROWDED corridor. That is QA's rule: a truck that has
    // exhausted the earlier rungs inside a packed corridor has to get right out,
    // because parking at its mouth just puts it back in the same queue. Deriving the
    // distance from the geometry means no per-truck retry counter has to be tracked;
    // and reaching LeaveRoad at all already means wait, go-around and back-up failed.
    // ...and ONLY when this is our own exhausted recovery. FindOffRoadSpot is shared
    // with AskToMove, where pAsker is the truck politely asking us to shift: a
    // courtesy step should move us aside, not send us fleeing the whole corridor.
    // 137 of the 156 corridor fires had a NUDGE as their next event, 104 of them
    // repeating inside 30s - WinAstra's scope defect.
    int iVehs = 0;
    int iCorr = (pAsker == NULL) ? CorridorAhead(iVehs) : 0;
    // A parking retry must not turn a committed reverse back into the queue.
    // BackUp already swapped the movement endpoints: head-tail now points OUT.
    // Keep that exit side until the body is off the road as well. At an angled
    // city corner the corridor walk can return zero while a tail still blocks it.
    BOOL bKeepReverseExit = (m_bReversing || m_bForwardEscape) &&
        (m_bConfined || OnPavement(m_ptHead) || OnPavement(m_ptTail) ||
         iCorr >= CORRIDOR_MIN_HEXES);
    int iMin  = ((iCorr >= CORRIDOR_MIN_HEXES) && (iVehs >= CORRIDOR_MIN_VEHS)) ? (1 + iCorr) : 1;
    if (iMin > 1)
        WaitLog("[CORRIDOR] veh %d at hex %d,%d in a %d-hex corridor holding %d vehicles, "
                "escaping to ring %d", GetID(), GetHexHead().X(), GetHexHead().Y(), iCorr, iVehs, iMin);

    // SCATTER THE SEARCH. Checking occupancy is not enough on its own: every truck
    // evaluates its target while the others are still driving to theirs, so they all
    // see the same empty hex and all set off for it - hex 23,343 was issued 159 times
    // in one 8-minute run against a capacity of two. Reserving spots would need a
    // shared allocator; giving each truck a different starting angle costs nothing and
    // breaks the convergence, because they no longer scan the ring in the same order.
    int iSkew = (int) (GetID() % 8);

    // Pass 0 keeps clear of the shoreline; pass 1 accepts it if nothing else exists.
    for (int iPass = 0; iPass < 2; iPass++)
    for (int iRing = iMin; iRing < iMin + PARK_SEARCH_SUBS; iRing++)
        for (int xO = -iRing; xO <= iRing; xO++)
            for (int yO = -iRing; yO <= iRing; yO++) {
                // rotate each truck's scan of this ring by its own fixed offset
                int xOff = ((xO + iRing + iSkew) % (2 * iRing + 1)) - iRing;
                int yOff = ((yO + iRing + iSkew) % (2 * iRing + 1)) - iRing;
                // only the edge of the ring - the inside was covered already
                if ((abs(xOff) != iRing) && (abs(yOff) != iRing))
                    continue;

                CSubHex _cand((_hexOn.X() + xOff) * 2, (_hexOn.Y() + yOff) * 2);
                _cand.Wrap();

                if (OnPavement(_cand))
                    continue;
                if (theMap._GetHex(_cand)->GetUnits() & (CHex::bldg | CHex::bridge))
                    continue;
                // Cost AT the candidate hex - both args the same hex, direction 0, which
                // is how the rest of the code asks "can this vehicle stand here"
                // (area.cpp:5327). Passing a DISTANT hex with our current heading, as
                // this did, is meaningless and returned non-zero for WATER - so a truck
                // beside a bridge could be sent to park in the river.
                if (theMap.GetTerrainCost(CHexCoord(_cand.ToCoord()), CHexCoord(_cand.ToCoord()), 0,
                                          GetData()->GetWheelType()) == 0)
                    continue;

                if ((iPass == 0) && IsShoreline(CHexCoord(_cand.ToCoord())))
                    continue;

                // IS ANYONE ALREADY THERE? This loop checked pavement, terrain,
                // shoreline, buildings and bridge - and never occupancy. So it handed
                // every asking truck the SAME square: measured in v18, hex 23,343 was
                // issued 104 times, 22,339 78 times, 36,345 70 times, against a hex
                // that holds four vehicles. That is not a parking allocator, and it
                // is why giving up made the jam worse - 290 trucks left the queue for
                // a space that did not exist, failed to park, and retried, while the
                // queue that was pushing traffic across the bridge came apart.
                //
                // The whole body has to fit, so check the hex's four subs and require
                // a free pair for head and tail rather than a single free corner.
                {
                    int iFree = 0;
                    CSubHex firstFree(_cand);
                    for (int iSx = 0; iSx < 2; iSx++)
                        for (int iSy = 0; iSy < 2; iSy++) {
                            CSubHex _s(CHexCoord(_cand.ToCoord()).X() * 2 + iSx,
                                       CHexCoord(_cand.ToCoord()).Y() * 2 + iSy);
                            _s.Wrap();
                            CVehicle *pOn = theVehicleHex._GetVehicle(_s);
                            if (pAsker != NULL &&
                                (_s == pAsker->m_ptNext || _s == pAsker->m_ptHead))
                                continue;
                            if ((pOn == NULL) || (pOn == this)) {
                                if (iFree == 0)
                                    firstFree = _s;
                                iFree++;
                            }
                        }
                    if (iFree < 2)
                        continue;
                    _cand = firstFree; // return a free corner, not always even/even
                }

                if (bKeepReverseExit &&
                    CSubHex::Diff(_cand.x - m_ptHead.x) * CSubHex::Diff(m_ptHead.x - m_ptTail.x) +
                    CSubHex::Diff(_cand.y - m_ptHead.y) * CSubHex::Diff(m_ptHead.y - m_ptTail.y) < 0)
                    continue;

                CVehicle *pTarget = theVehicleHex._GetVehicle(_cand);
                if (pTarget != NULL && pTarget != this)
                    WaitLog("[PARK-OCCUPIED] veh %d selected sub %d,%d occupied by %d",
                            GetID(), _cand.x, _cand.y, pTarget->GetID());
                _found = _cand;
                return (TRUE);
            }

    return (FALSE);
}

// Someone has run into us while we are sitting on the road. We are not going to
// clear on our own, so step aside: take one hex to a free neighbour that is not
// the hex the asker wants. Returns TRUE if we are now moving out of the way.
//
// Deliberately narrow: only OUR OWN idle vehicles, never one that is busy doing a
// job (building, repairing, loading) and never more often than once every few
// seconds, so this cannot become a general "restart every stopped unit" sweeper.
BOOL CVehicle::AskToMove(CVehicle *pAsker) {

    if (!(TrafficOpts() & 4))
        return (FALSE);

    if ((pAsker == NULL) || (pAsker == this))
        return (FALSE);
    if (!GetOwner()->IsLocal())
        return (FALSE);
    if (GetOwner() != pAsker->GetOwner())
        return (FALSE);

    // only a vehicle that is sitting still - leave anything actually driving alone
    if ((m_cMode != stop) && (m_cMode != blocked))
        return (FALSE);

    // and never one that was ORDERED to stand still
    if (IsFlag(stopped) || IsHpControl())
        return (FALSE);

    // A HAULING truck (m_iEvent == route) is exactly the case we care about - those
    // are what park on the bridge. Only genuinely busy jobs are off limits.
    if ((m_iEvent != none) && (m_iEvent != route) &&
        !(GetData()->IsCrane() && m_iEvent == build))
        return (FALSE);
    if (m_pBldg != NULL)
        return (FALSE);

    // A blocked detour is already answering an earlier move request. Do not
    // replace its destination or reset its wait/retry state with the same request.
    if (m_bResume && m_cMode == blocked)
        return (TRUE);

    // do not nudge the same vehicle over and over
    if ((m_dwAskedToMove != 0) &&
        (theGame.GettimeGetTime() - m_dwAskedToMove < 5000))
        return (FALSE);

    // Go and PARK somewhere that is not a road. A one-hex sideways shuffle on a
    // bridge deck leaves us just as much in the way as we were.
    CSubHex _spot;
    if (FindOffRoadSpot(_spot, pAsker)) {
        m_dwAskedToMove = theGame.GettimeGetTime();
        WaitLog("[NUDGE] veh %d hex %d,%d asked by veh %d, parking off-road at hex %d,%d", GetID(),
                GetHexHead().X(), GetHexHead().Y(), pAsker->GetID(), CHexCoord(_spot.ToCoord()).X(),
                CHexCoord(_spot.ToCoord()).Y());
        // Give the requester time to use the space we clear.
        if (m_iHoldFrames <= 0 && (OnPavement(m_ptHead) || OnPavement(m_ptTail)))
            m_iHoldFrames = HOLD_FRAMES + (int) (GetID() % 4) * 24;
        // a vehicle that has ALREADY arrived is simply parked in a bad place, so the
        // new spot becomes its home. One that still has somewhere to be resumes.
        DetourTo(_spot, (m_ptHead == m_ptDest) ? FALSE : TRUE);
        return (TRUE);
    }

    // nowhere off-road within reach - settle for any free neighbour.
    //
    // Moving our HEAD out of the way is not enough: on a two-sub-hex vehicle the
    // TAIL ends up where the head was, which can be exactly the square the asker
    // wanted. Stepping there is not clearance, so require the whole body to end up
    // clear of what they asked for.
    for (int iDir = -3; iDir <= 3; iDir++) {
        CSubHex _next = Rotate(iDir);
        if ((_next == m_ptHead) || (_next == pAsker->m_ptNext) || (_next == pAsker->m_ptHead))
            continue;

        // where the tail lands once we step: for a 1-hex vehicle it follows the
        // head, otherwise it takes the square the head is leaving
        CSubHex _tail = (GetData()->GetVehFlags() & CTransportData::FL1hex) ? _next : m_ptHead;
        if ((_tail == pAsker->m_ptNext) || (_tail == pAsker->m_ptHead))
            continue;            // our tail would still be in their way

        if (!CanEnter(_next))
            continue;

        m_dwAskedToMove = theGame.GettimeGetTime();
        WaitLog("[NUDGE] veh %d hex %d,%d asked by veh %d, stepping aside to sub %d,%d", GetID(),
                GetHexHead().X(), GetHexHead().Y(), pAsker->GetID(), _next.x, _next.y);
        DetourTo(_next, (m_ptHead == m_ptDest) ? FALSE : TRUE);
        return (TRUE);
    }

    return (FALSE);
}

// LAST RESORT. Waiting did not clear it, going around did not work and turning
// around did not work - we are wedged. Reverse out the way we came.
//
// This is recovery, and only for running into other vehicles: a failed path
// search stamps m_iNumRetries straight to MAX_NUM_RETRIES and never reaches the
// rung that calls this, which is intended.
// ---- adjacent-truck clearance ("panic") -------------------------------------
// WinAstra's 015-winastra-adjacent-clearance design. QA: when one truck has been
// stuck a long time, the trucks touching it should briefly stop pursuing their own
// deliveries and make space, so a vacancy at the edge of the jam can travel inwards.
//
// Deliberately NOT a manager: nothing owns the group, nothing scans the map, and no
// truck is told which way to move. A request only ever passes between trucks that are
// physically touching, each recipient re-offers it on its own timer, and the
// REMAINING window is carried rather than refreshed - so a ring of trucks cannot keep
// handing the same request round in circles.

BOOL CVehicle::JamEligible() const {

    if (!(TrafficOpts() & 8))
        return (FALSE);
    if (GetOwner() == NULL)
        return (FALSE);
    if (!GetOwner()->IsLocal())
        return (FALSE);
    if (m_unitFlags & (dying | stopped))
        return (FALSE);
    if (!m_cOwn)                            // no physical road occupancy to clear
        return (FALSE);
    if (IsHpControl())                       // the player is driving this one
        return (FALSE);
    if (GetData()->IsBoat())
        return (FALSE);
    // An inactive crane clearing the road needs to request space too. Active
    // construction remains excluded by m_pBldg below; preserve its build order.
    if (!GetData()->IsTransport() &&
        !(GetData()->IsCrane() && (m_iEvent == none || m_iEvent == build)))
        return (FALSE);
    if (GetData()->GetVehFlags() & CTransportData::FL1hex)
        return (FALSE);
    if (m_pBldg != NULL)                     // inside a building, owns no road hex
        return (FALSE);
    return (TRUE);
}

// Pass the request to the trucks actually TOUCHING us. Bounded: the subs around our
// own two ends, nothing radial, nothing distant.
void CVehicle::JamForward() {

    // Only occupied body squares touching either end participate. A reservation
    // for a future step is not physical contact, and a nearby queue is not ours.
    int iSent = 0;
    for (int iEnd = 0; iEnd < 2; iEnd++)
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++) {
                CSubHex _n(iEnd == 0 ? m_ptHead : m_ptTail);
                _n.x += dx;
                _n.y += dy;
                _n.Wrap();
                CVehicle *pOn = theVehicleHex._GetVehicle(_n);
                if ((pOn == NULL) || (pOn == this))
                    continue;
                if (pOn->GetOwner() != GetOwner() ||
                    (_n != pOn->m_ptHead && _n != pOn->m_ptTail))
                    continue;
                if (!pOn->JamEligible())
                    continue;
                // Cooling blocks raising a request, not joining one.
                if (pOn->m_iJamClear > 0)
                    continue;                    // already in, keep its deadline
                pOn->m_iJamClear = m_iJamClear;    // carry only what is left
                pOn->m_iJamFwd = 0;
                iSent++;
                WaitLog("[CLEAR-CONTACT] from %d to %d sub %d,%d frames %d", GetID(),
                        pOn->GetID(), _n.x, _n.y, m_iJamClear);
            }

    if (iSent > 0)
        WaitLog("[JAM] veh %d at hex %d,%d passed clearance to %d trucks, %d frames left",
                GetID(), GetHexHead().X(), GetHexHead().Y(), iSent, m_iJamClear);
}

// Stagnation watch, and raising a request. Body CENTRE, not head: swapping the head
// and tail labels to reverse leaves the centre where it was, so a relabel cannot look
// like progress and cannot reset the watch either.
void CVehicle::JamWatch() {

    int iFr = (int) theGame.GetFramesElapsed();

    if (m_iJamCool > 0)
        m_iJamCool -= iFr;

    // Eligibility can change during a request (building entry, player Stop,
    // manual control, death). Never relay from a stale, unoccupied footprint.
    if (!JamEligible()) {
        m_iJamClear = m_iJamFwd = m_iJamWatch = 0;
        return;
    }

    // taking part in someone's request
    if (m_iJamClear > 0) {
        // Expire before forwarding. Otherwise a one-frame request can pass to
        // a neighbour, expire here, and be passed back later in the same update.
        // Each recipient must spend elapsed time before it can relay the request.
        m_iJamClear -= iFr;
        if (m_iJamClear <= 0) {
            m_iJamClear = 0;
            m_iJamFwd   = 0;
            m_iJamCool  = JAM_COOL_FRAMES;   // participation costs the right to start one
        } else {
            m_iJamFwd -= iFr;
            if (m_iJamFwd <= 0) {
                m_iJamFwd = JAM_FWD_EVERY;
                JamForward();
            }
        }
        return;
    }

    // Idle parking and recovery holds are not failed travel. Start a fresh
    // watch on departure; active clearance requests above still expire/relay.
    if (m_cMode == stop) {
        m_iJamWatch = 0;
        return;
    }

    int cx = m_ptHead.x + m_ptTail.x;        // doubled centre
    int cy = m_ptHead.y + m_ptTail.y;

    if (m_iJamWatch == 0) {
        m_iJamAnchorX = cx;
        m_iJamAnchorY = cy;
        m_iJamWatch   = 1;
        return;
    }

    if ((abs(CSubHex::Diff(cx - m_iJamAnchorX)) + abs(cy - m_iJamAnchorY)) >= (JAM_MOVE_SUBS * 2)) {
        m_iJamAnchorX = cx;                  // real travel - not stuck
        m_iJamAnchorY = cy;
        m_iJamWatch   = 1;
        return;
    }

    m_iJamWatch += iFr;
    if (m_iJamWatch < JAM_STUCK_FRAMES)
        return;
    if (m_iJamCool > 0)
        return;
    // only while a truck is actually in our way - a slow queue is not a jam
    // Traffic waiting parks m_ptNext on our own head. The actual blocked step
    // remains in m_subWaitNext; testing our head mistakes a stuck queue for no blocker.
    CSubHex blockedStep = (m_cMode == traffic) ? m_subWaitNext : m_ptNext;
    BlockedLaneStep(blockedStep);
    if (blockedStep.x < 0 || blockedStep.y < 0)
        return;
    CVehicle *pIn = theVehicleHex._GetVehicle(blockedStep);
    if ((pIn == NULL) || (pIn == this))
        return;

    m_iJamWatch = 0;
    // stagger the expiry per truck so the cluster does not resume all at once
    m_iJamClear = JAM_WINDOW_FRAMES + (int) (GetID() % 8) * (JAM_STAGGER_FRAMES / 8);
    m_iJamFwd   = 0;
    WaitLog("[JAM] veh %d at hex %d,%d STUCK, raising clearance request for %d frames, blocker %d traffic %d",
            GetID(), GetHexHead().X(), GetHexHead().Y(), m_iJamClear, pIn->GetID(), (int) (m_cMode == traffic));
}

BOOL CVehicle::BackUp() {

    if (m_bForwardEscape)
        return (FALSE);

    // Two retreats can meet rear-to-rear in the same lane. Keep that lane's
    // ordinary travel direction; the other drives nose-first until clear. A vehicle
    // meeting that forward escape makes the same local choice on its own update.
    // The forward commitment lasts to arrival, so another request cannot undo it.
    BOOL bForwardYield = FALSE;
    int axisX = CSubHex::Diff(m_ptHead.x - m_ptTail.x);
    int axisY = CSubHex::Diff(m_ptHead.y - m_ptTail.y);
    if (m_bReversing) {
        CSubHex blockedStep;
        if (!BlockedLaneStep(blockedStep))
            return (FALSE);
        CVehicle *pIn = theVehicleHex._GetVehicle(blockedStep);
        // Match InLane's convention. Independent pairs in one lane must choose
        // the same direction, or their escaping groups meet head-on again.
        BOOL bAgainstLane = axisX != 0 ? ((m_ptHead.y & 1) != (axisX > 0 ? 1 : 0)) :
                                       ((m_ptHead.x & 1) != (axisY > 0 ? 0 : 1));
        if (pIn == NULL || pIn->GetOwner() != GetOwner() || !pIn->JamEligible() ||
            (blockedStep != pIn->m_ptHead && blockedStep != pIn->m_ptTail) ||
            CSubHex::Diff(pIn->m_ptHead.x - pIn->m_ptTail.x) != -axisX ||
            CSubHex::Diff(pIn->m_ptHead.y - pIn->m_ptTail.y) != -axisY ||
            (!pIn->m_bForwardEscape &&
             !(m_iJamClear > 0 && pIn->m_bReversing && bAgainstLane)))
            return (FALSE);
        bForwardYield = TRUE;
    } else if (m_iJamClear > 0 && ((axisX == 0) != (axisY == 0))) {
        CSubHex behind(m_ptTail.x - axisX, m_ptTail.y - axisY);
        behind.Wrap();
        CVehicle *pBehind = theVehicleHex._GetVehicle(behind);
        bForwardYield = pBehind != NULL && pBehind != this &&
            pBehind->GetOwner() == GetOwner() && pBehind->JamEligible() &&
            pBehind->m_bForwardEscape && pBehind->m_ptHead == behind &&
            CSubHex::Diff(pBehind->m_ptHead.x - pBehind->m_ptTail.x) == axisX &&
            CSubHex::Diff(pBehind->m_ptHead.y - pBehind->m_ptTail.y) == axisY;
    }

    if (!(TrafficOpts() & 8))
        return (FALSE);

    if (GetData()->GetVehFlags() & CTransportData::FL1hex)
        return (FALSE);

    // Are we inside a big stuck cluster - a long confined stretch with a lot of
    // vehicles in it? Asked locally, by every truck, about its own surroundings.
    int iCorrVehs = 0;
    int iCorrLen  = CorridorAhead(iCorrVehs);
    BOOL bCluster = (iCorrLen >= CORRIDOR_MIN_HEXES) && (iCorrVehs >= CORRIDOR_MIN_VEHS);

    // THE TWO-ATTEMPT CAP IS WRONG INSIDE A CLUSTER. It exists so a stuck PAIR cannot
    // ping-pong forever, which is right in the open. In a packed corridor it strands
    // exactly the trucks that have to leave: the ones walled in at the front spend
    // both attempts failing while the rear is still full, and are then permanently
    // out of options even after the space behind them clears.
    //
    // Letting them keep trying is what makes a cluster drain WITHOUT any coordination.
    // Each truck independently commits to backing out; only those with clear space
    // behind can actually execute, which is the rearmost; when they go, the next
    // inherit clear space and succeed in turn. The evacuation orders itself, and no
    // truck ever instructs another - the post-retreat hold rate-limits the retries.
    // A later clearance request must still be answerable after this job spent
    // its ordinary retry budget. The active window and escape commitment bound
    // repeated attempts; the cap otherwise strands the very blocker being asked.
    if (!bForwardYield && m_iJamClear <= 0 &&
        m_iBackUps >= (bCluster ? MAX_BACK_UPS_JAM : MAX_BACK_UPS))
        return (FALSE);

    // How FAR back? Far enough to actually be out of the way. Reversing a single
    // hex on a long bridge just moves the wedge one hex along the span, so walk
    // back down our own axis until we are off the bridge (or off the pavement),
    // and make THAT the destination. Normal movement takes us there, so the wait
    // and nudge rules still apply on the way out.
    int xBack = CSubHex::Diff(m_ptTail.x - m_ptHead.x);
    int yBack = CSubHex::Diff(m_ptTail.y - m_ptHead.y);
    if (bForwardYield && !m_bReversing) {
        xBack = -xBack;
        yBack = -yBack;
    }
    if ((xBack == 0) && (yBack == 0))
        return (FALSE);

    CSubHex _back(m_ptHead);
    CSubHex _target;
    BOOL bFound = FALSE;
    BOOL bWasBridge = (theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge) ? TRUE : FALSE;

    // HOW FAR BACK IS FAR ENOUGH? As far as the confined stretch is long.
    //
    // Measured, the median retreat was SIX SUB-HEXES - three hexes, about one and a
    // half truck lengths - because the walk stopped at the first candidate that was
    // off the bridge and off the pavement. Near a mouth that is a patch of dirt right
    // beside the jam, so the truck reversed one length, parked, and was back in the
    // queue as soon as it was re-tasked. That is "get off the road", not "get out of
    // the corridor", and only the top 10%% ever reached the 40-sub cap.
    //
    // Taking the distance from the corridor's own geometry needs no per-truck retry
    // counter: a longer corridor produces a longer retreat by construction.
    int iMinBack  = bCluster ? (iCorrLen * 2) : 0;      // hexes -> subs

    BOOL bOnWater = IsOnWater() || (GetData()->GetWheelType() == CWheelTypes::water);
    for (int iStep = 0; iStep < BACK_UP_SUBS; iStep++) {
        // FOLLOW THE ROAD BACK, not the hull's exact diagonal. A truck sitting at an
        // angle has a body vector like (+1,+1), and stepping strictly along it walks
        // straight off the deck into the water - the walk then hit impassable terrain
        // on its FIRST candidate and gave up, so an angled truck could never reverse at
        // all. WinAstra named this as the reason 5532/7015 have no BACKUP records
        // despite being the core of the jam. Try the axis first, then its two cardinal
        // components, and take the first that a vehicle could actually stand on.
        CSubHex _alt[3];
        int     iTries = 0;
        _alt[iTries++] = CSubHex(_back.x + xBack, _back.y + yBack);
        if ((xBack != 0) && (yBack != 0)) {
            _alt[iTries++] = CSubHex(_back.x + xBack, _back.y);
            _alt[iTries++] = CSubHex(_back.x,        _back.y + yBack);
        }

        BOOL bStep = FALSE;
        for (int iT = 0; iT < iTries; iT++) {
            CSubHex _c(_alt[iT]);
            _c.Wrap();
            // Usable terrain beside a bridge is not necessarily a legal exit.
            // Follow the same terrain transitions as movement, without treating
            // queued vehicles as permanent obstacles or backing into buildings.
            if ((theMap._GetHex(_c)->GetUnits() & CHex::bldg) ||
                !GetData()->CanEnterHex(CHexCoord(_back), CHexCoord(_c), bOnWater, TRUE))
                continue;
            _back = _c;
            bStep = TRUE;
            break;
        }
        if (!bStep)
            break;              // nowhere behind us a vehicle could stand

        // Never retreat INTO OUR OWN BODY. The first candidate down our axis is our
        // own tail, and Turn180 makes that the head - so accepting it means arriving
        // where we already are, with the same squares occupied and one of only two
        // retreat attempts spent clearing nothing.
        if ((_back == m_ptHead) || (_back == m_ptTail))
            continue;

        BOOL bBridge = (theMap._GetHex(_back)->GetUnits() & CHex::bridge) ? TRUE : FALSE;
        _target = _back;
        bFound = TRUE;

        // Far enough = off the span, off the pavement, AND clear of the confined
        // stretch we were stuck in. The first two alone stopped the retreat at the
        // nearest scrap of dirt; the third is what actually gets the truck away.
        if ((!bBridge) && (!OnPavement(_back)) && (iStep >= iMinBack))
            break;
    }

    if (!bFound)
        return (FALSE);

    // Off the span is what we WANT - back all the way off, then hold, so the trucks
    // in front have somewhere to go. But refusing every target that is still on the
    // deck threw away the whole retreat whenever the bank was out of reach, and a
    // truck that reverses even one body-length down the deck still vacates the
    // square the vehicle ahead of it is waiting for. Take the partial; just say so.
    BOOL bPartial = (theMap._GetHex(_target)->GetUnits() & CHex::bridge) ? TRUE : FALSE;

    m_iBackUps++;
    WaitLog("[BACKUP] veh %d hex %d,%d sub %d,%d reversing to hex %d,%d sub %d,%d "
            "(%d sub-hexes back, backup %d, ondeck %d, partial %d, already_reversing %d, jam %d)", GetID(),
            GetHexHead().X(), GetHexHead().Y(), m_ptHead.x, m_ptHead.y,
            CHexCoord(_target.ToCoord()).X(), CHexCoord(_target.ToCoord()).Y(), _target.x, _target.y,
            abs(CSubHex::Diff(_target.x - m_ptHead.x)) + abs(CSubHex::Diff(_target.y - m_ptHead.y)),
            m_iBackUps, (int) bWasBridge, (int) bPartial, (int) m_bReversing, m_iJamClear);
    // Swap the movement endpoints without moving the body. SetLoc accounts for
    // the reverse-facing offset, so the nose still points the original way.
    BOOL bSwapEnds = !bForwardYield || m_bReversing;
    m_bReversing = !bForwardYield;
    m_bForwardEscape = bForwardYield;
    if (bSwapEnds)
        Turn180();
    if (bForwardYield)
        WaitLog("[FORWARD-YIELD] veh %d head %d,%d tail %d,%d target %d,%d swapped %d",
                GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                _target.x, _target.y, (int) bSwapEnds);
    DetourTo(_target, TRUE);          // and carry on with the haul afterwards
    return (TRUE);
}

// Is this sub-hex paved? Roads and city tiles are the through-routes everyone
// else needs; sitting on one makes us an obstacle.
BOOL CVehicle::OnPavement(CSubHex const &_sub) {

    // A BRIDGE DECK is the most congested through-route on the map, but its terrain
    // type is not road - the bridge is a unit flag over water. Testing terrain alone
    // meant LeaveRoad bailed out for every vehicle wedged ON the span, so the rule
    // meant to clear through-routes skipped the only one that was jammed:
    // 21 OFFROAD events on the approaches, ZERO on the deck.
    if (theMap._GetHex(_sub)->GetUnits() & CHex::bridge)
        return (TRUE);

    int iType = theMap._GetHex(_sub)->GetType();
    return ((iType == CHex::road) || (iType == CHex::city));
}

// Do not settle down in the middle of the road. A vehicle that gives up becomes a
// permanent obstacle for everyone behind it, so if we are giving up while parked
// on pavement, pull off onto adjacent open ground first.
BOOL CVehicle::LeaveRoad() {

    if (!(TrafficOpts() & 8))
        return (FALSE);

    if (!GetOwner()->IsLocal())
        return (FALSE);
    // A build order is not active construction while an idle/blocked crane has
    // no building attached. Preserve that order through the parking detour.
    if (m_iEvent != none && !(GetData()->IsCrane() && m_iEvent == build))
        return (FALSE);
    if (!m_cOwn || m_pBldg != NULL || IsFlag(stopped) || IsHpControl())
        return (FALSE);
    if (!OnPavement(m_ptHead) && !OnPavement(m_ptTail))
        return (FALSE);
    if (m_dwLeftRoad != 0 && theGame.GettimeGetTime() - m_dwLeftRoad < 30000)
        return (FALSE);

    // Ordinarily: NEVER GIVE UP INSIDE A CORRIDOR. Abandoning the haul to go and park
    // leaves a truck neither crossing nor pressing on the one in front, and the queue is
    // what pushes traffic through.
    //
    // BUT NOT DURING A PANIC. While a clearance request is live the whole objective is
    // to GET OFF the corridor, and a truck that cannot reverse - nothing behind it, or
    // an angle that will not take it - has no other way out. Forbidding this during a
    // panic shut down half the escape routes of exactly the trucks being asked to leave.
    // An already stopped unit cannot advance the queue; let it clear the road.
    if (m_iJamClear <= 0 && m_cMode != stop) {
        int iVehsHere = 0;
        if ((theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge) ||
            (CorridorAhead(iVehsHere) >= CORRIDOR_MIN_HEXES))
            return (FALSE);
    }
    // Rate-limit attempts, including a failed search. The stopped-state update
    // may retry later without rescanning the parking rings every frame.
    m_dwLeftRoad = theGame.GettimeGetTime();

    CSubHex _spot;
    if (FindOffRoadSpot(_spot, NULL)) {
        // An exit behind a confined hull needs reverse movement, not a forward
        // route that tries to turn around on the deck. Reuse the bounded backup.
        int dx = CSubHex::Diff(_spot.x - m_ptHead.x);
        int dy = CSubHex::Diff(_spot.y - m_ptHead.y);
        int iVehs = 0;
        if (dx * CSubHex::Diff(m_ptHead.x - m_ptTail.x) +
                dy * CSubHex::Diff(m_ptHead.y - m_ptTail.y) < 0 &&
            ((theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge) ||
             (theMap._GetHex(m_ptTail)->GetUnits() & CHex::bridge) ||
             CorridorAhead(iVehs) >= CORRIDOR_MIN_HEXES) && BackUp()) {
            WaitLog("[PARK-REVERSE] veh %d backing out for off-road exit %d,%d", GetID(), _spot.x, _spot.y);
            return (TRUE);
        }
        WaitLog("[OFFROAD] veh %d giving up at hex %d,%d, parking off-road at hex %d,%d", GetID(),
                GetHexHead().X(), GetHexHead().Y(), CHexCoord(_spot.ToCoord()).X(),
                CHexCoord(_spot.ToCoord()).Y());
        // AND STAY GONE for a while. Parking with no job reports "stopped" to the
        // router, which re-sends us to the same destination within the frame, so the
        // corridor we just left is refilled by the truck that left it. DetourTo
        // preserves this across SetDestAndMode; Operate only counts it down once we
        // are actually stopped, so it starts when we arrive, not when we set off.
        m_iHoldFrames = GIVEUP_HOLD_FRAMES;
        DetourTo(_spot, m_ptHead != m_ptDest);
        return (TRUE);
    }

    for (int iDir = -3; iDir <= 3; iDir++) {
        CSubHex _next = Rotate(iDir);
        if (_next == m_ptHead)
            continue;
        if (OnPavement(_next))
            continue;            // still on the road - no help to anyone
        if (!CanEnter(_next))
            continue;

        WaitLog("[OFFROAD] veh %d giving up at hex %d,%d, pulling off to sub %d,%d", GetID(),
                GetHexHead().X(), GetHexHead().Y(), _next.x, _next.y);
        m_iHoldFrames = GIVEUP_HOLD_FRAMES;   // see above - a departure that returns at once drains nothing
        DetourTo(_next, m_ptHead != m_ptDest);
        return (TRUE);
    }

    return (FALSE);
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
            // path THROUGH vehicles, not around them. bNoOcc is forwarded as
            // CPathMgr's bVehBlock: FALSE = the search treats occupied hexes as
            // passable. On a road the occupant is almost always moving and will
            // be gone by the time we arrive; planning around it either detours
            // into the oncoming lane or finds no route at all.
            GetPath(FALSE);

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
        // Apply the same escape exception as ordinary next-step selection.
        BOOL bEscape = m_bResume && (m_bReversing || m_bForwardEscape) && JamEligible();
        if ((iOld <= iNew) && (iNew != 0) && bEscape)
            WaitLog("[ESCAPE-STEP] veh %d source retry head %d,%d tail %d,%d next %d,%d "
                    "hexnext %d,%d distance %d to %d times %d",
                    GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                    m_ptNext.x, m_ptNext.y, m_hexNext.X(), m_hexNext.Y(), iOld, iNew, m_iTimesOn);
        if ((iOld <= iNew) && (iNew != 0) && !bEscape)
            if (!FindSub(TRUE)) {
                if (GetOwner()->IsMe() && m_bResume && m_iHoldFrames > 0)
                    WaitLog("[PARK-CIRCLE] veh %d source retry head %d,%d tail %d,%d hexnext %d,%d "
                            "retry %d visits %d hold %d",
                            GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                            m_hexNext.X(), m_hexNext.Y(), m_iNumRetries, m_iTimesOn, m_iHoldFrames);
                return (FALSE);
            }
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

    // The parking path failed, but we already made space. Complete this detour
    // where we stand rather than spend minutes chasing an unreachable parking
    // square. ArrivedDest supplies the usual hold and resumes the saved job.
    if (m_iPathLen == 0 && m_iNumRetries == MAX_NUM_RETRIES && FinishClearDetour())
        return;

    // ARE WE SOMEWHERE WITH NO ROOM TO MANOEUVRE? Computed ONCE per blocked call and
    // reused by the rungs below, so a truck deep in the ladder does not pay for the
    // corridor walk over and over. On a bridge or between building rows there is
    // nowhere to swing out to, so the circling rungs - random re-route, rotate,
    // turn-around - cannot succeed; they only spend retries, spin the hull on the
    // spot and hand the truck to the oncoming lane. Skip them there and let the
    // ladder fall through to the reverse, which is the only move that fits.
    int  iCorrVehs  = 0;
    int  iCorrLen   = CorridorAhead(iCorrVehs);
    BOOL bConfined  = (TrafficOpts() & 8) &&
                      ((theMap._GetHex(m_ptHead)->GetUnits() & CHex::bridge) ||
                       (iCorrLen >= CORRIDOR_MIN_HEXES));
    m_bConfined = bConfined;     // cached for FindSubEx, which must not re-walk per step
    m_iCorrLen  = iCorrLen;      // ...and HOW FAR it ran along our own axis - see FindSubEx

    // MAKING SPACE for a clearance request: skip the ladder and reverse now. The
    // recipient still chooses its own motion - if it can go forward it never reaches
    // HandleBlocked at all - and BackUp does its own legality checks, so nothing is
    // forced and no direction was dictated by the asker.
    // Reverse if we can; if we cannot, get off the road instead. Either counts as
    // making space, and a truck that can do neither simply waits - nothing is forced.
    // An active request must not restart an escape at every blocked update.
    // Both reverse and forward escapes use ordinary step retries and bounded
    // failure below. Immediate parking would replace their chosen exit route.
    if (m_bReversing && BackUp()) // only the opposing-retreat exception can restart it
        return;
    if ((m_iJamClear > 0) && (!m_bReversing) && (!m_bForwardEscape)) {
        if (BackUp())
            return;
        if (LeaveRoad())
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
        // 1996 curiosity TRAP; the arrive-here handling below IS the recovery.
        // Routinely reachable now that AI bridges build at scale (soak29 06:24)
        EN_TRAP_REMOVED( "HandleBlocked: blocked on under-construction bridge - arrived-here below" );
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

    CSubHex blockedStep;
    if (BlockedLaneStep(blockedStep))
        m_ptNext = blockedStep;
    CVehicle *pVehInWay = theVehicleHex._GetVehicle(m_ptNext);

    // is the block legit (didn't release properly?)
    if (m_iNumRetries == 5) {
        m_iNumRetries++;
        if (pVehInWay != NULL) {
            if ((pVehInWay->m_ptNext != m_ptNext) && (pVehInWay->m_ptHead != m_ptNext) &&
                (pVehInWay->m_ptTail != m_ptNext)) {
                // stale claim detected; CheckHex below scrubs it and TryNewSub
                // retries = the recovery (soak37 12:38 dump)
                EN_TRAP_REMOVED("HandleBlocked: stale claim on next - scrubbed below");
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
        // recovery clears the stagnation watch: a stale same-hex stamp made a
        // RE-block at a hex visited >6min ago an instant give-up on a fresh
        // jam (garage-door commutes; audit finding)
        m_dwStagnantSince = 0;
        ASSERT_VALID (this);
#ifdef _LOGOUT
        logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d road cleared", GetID());
#endif
        return;
    }

    // permanently-boxed detector: same hex past the 1996 escalation horizon
    // (TRUCK_JUMP_TIME) -> designed give-up. 90s aborted trips for units
    // merely queued in fresh-base gridlock; frozen units sit for hours.
    {
        CHexCoord _hexNow(m_ptHead);
        if (m_dwStagnantSince == 0 || m_hexStagnant != _hexNow) {
            m_hexStagnant     = _hexNow;
            m_dwStagnantSince = theGame.GettimeGetTime();
        } else if (theGame.GettimeGetTime() - m_dwStagnantSince > (DWORD)TRUCK_JUMP_TIME * 1000)
            goto GiveUp;
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
                    EN_TRAP_REMOVED("HandleBlocked: dest across land/water break - handled below");
                    xDif = m_ptDest.x - m_ptHead.x;
                    yDif = m_ptDest.y - m_ptHead.y;
                    xDif = __minmax(-1, 1, xDif);
                    yDif = __minmax(-1, 1, yDif);
                    CSubHex _test(m_ptHead.x + xDif, m_ptHead.y + yDif);
                    _test.Wrap();
                    if (!GetData()->CanEnterHex(m_ptHead, _test, IsOnWater(), TRUE)) {
                        EN_TRAP_REMOVED("HandleBlocked: land/water break confirmed - graceful give-up follows");
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
            // 1996: "we look for a path with no vehicles in the way". That is the
            // bug - it is the first re-path after a bump, so the vehicles in the
            // way are the ones still driving. Path THROUGH them and queue.
            GetPath(FALSE);

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
    // confined: a new route picked at random cannot help here - see HandleBlocked head
    if ((m_iNumRetries == 10) && bConfined)
        m_iNumRetries++;
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
    // confined: another randomised rotate cannot help here - see HandleBlocked head
    if ((m_iNumRetries == 11) && bConfined)
        m_iNumRetries++;
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
    // confined: a clear-route search that re-routes around vehicles cannot help here - see HandleBlocked head
    if ((m_iNumRetries == 12) && bConfined)
        m_iNumRetries++;
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

    // Trucks/cranes that passed this wait must reach the remaining recovery
    // steps instead of restarting it whenever block_count grows. MAX_NUM_RETRIES
    // is also a direct no-path marker, not evidence that this wait was completed.
    if (m_iNumRetries > 13 && m_iNumRetries < MAX_NUM_RETRIES && JamEligible() &&
        m_dwTimeBlocked < (DWORD) (3 * m_iBlockCount * m_iBlockCount * m_iBlockCount * m_iSpeed * STEPS_HEX))
        WaitLog("[RETRY-ADVANCE] veh %d head %d,%d tail %d,%d retry %d count %ld elapsed %lu",
                GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                m_iNumRetries, (long)m_iBlockCount, (unsigned long)m_dwTimeBlocked);
    if ((m_iNumRetries <= 13 || m_iNumRetries >= MAX_NUM_RETRIES || !JamEligible()) &&
        m_dwTimeBlocked < (DWORD) (3 * m_iBlockCount * m_iBlockCount * m_iBlockCount * m_iSpeed * STEPS_HEX)) {
        if (GetOwner()->IsMe() && theGame.GettimeGetTime() - m_dwBlockLog > 5000) {
            m_dwBlockLog = theGame.GettimeGetTime();
            WaitLog("[RETRY-WAIT] veh %d head %d,%d tail %d,%d next %d,%d retry %d count %ld "
                    "speed %d elapsed %lu threshold %lu reverse %d forward %d resume %d hold %d",
                    GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y, m_ptNext.x, m_ptNext.y,
                    m_iNumRetries, (long)m_iBlockCount, m_iSpeed, (unsigned long)m_dwTimeBlocked,
                    (unsigned long)(3 * m_iBlockCount * m_iBlockCount * m_iBlockCount * m_iSpeed * STEPS_HEX),
                    (int)m_bReversing, (int)m_bForwardEscape, (int)m_bResume, m_iHoldFrames);
        }
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
            // Match FindSubEx: an aligned hull must not turn across a narrow
            // corridor just because this later recovery rung was reached.
            int iTurnLimit = (bConfined && iCorrLen >= 2) ? 1 : 3;
            for (int iDir = -iTurnLimit; iDir <= iTurnLimit; iDir++) {
                CSubHex _next = Rotate(iDir);
                if (CanEnter(_next)) {
                    if (bConfined && abs(iDir) > 1)
                        WaitLog("[CONFINED-TURN] veh %d head %d,%d tail %d,%d turn %d next %d,%d reverse %d corridor %d",
                                GetID(), m_ptHead.x, m_ptHead.y, m_ptTail.x, m_ptTail.y,
                                iDir, _next.x, _next.y, (int) m_bReversing, iCorrLen);
                    m_ptNext = _next;
#ifdef _LOGOUT
                    logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "Vehicle %d turn (-)3 try 2", GetID());
#endif
                    return;
                }
            }

            // ON A BRIDGE THERE IS NO ROOM TO TURN AROUND. This rung is the stock
            // 1996 turn-and-go, and on a one-lane deck it is the wrong rung: the hull
            // pivots on the spot, still cannot get out, and spends the attempt - which
            // is the spinning that shows up on screen. Skip it on the span and let the
            // ladder fall through to rung 16, where BackUp reverses down our own axis.
            // Only the span: on open ground turning round is still the right answer.
            // before we turn let's make sure the other direction is better
            // (bConfined now covers building-lined streets too, not just the span)
            for (int iDir = -3; (!bConfined) && (iDir <= 3); iDir++) {
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

    // LAST RESORT: we waited, we tried to go around, we tried to turn around, and
    // we are still wedged behind another vehicle. Back up.
    if (m_iNumRetries == 16) {
        m_iNumRetries++;
        if ((pVehInWay != NULL) && (pVehInWay != this))
            if (BackUp())
                return;
    }

    // we tried all of the above and have been in here at least 5 times
    if ((m_iNumRetries > 15) && (m_iBlockCount > 5)) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) permanently blocked", GetID(), m_ptHead.x,
                  m_ptHead.y);
#endif

        GiveUp:
        // Every path that gives up arrives here, including the four direct
        // `goto GiveUp` jumps, so the last-resort behaviour belongs HERE and not
        // in the retries gate above (which those jumps skip entirely).
        //
        // Completely stuck: if a VEHICLE is what is in the way, back out. A failed
        // path search is deliberately NOT this case - it stamps m_iNumRetries
        // straight to MAX_NUM_RETRIES and leaves pVehInWay NULL.
        if ((pVehInWay != NULL) && (pVehInWay != this))
            if (BackUp())
                return;

        // and do not settle down in the roadway - pull off it first
        if (LeaveRoad())
            return;

#if EN_AI_PROBES_ECON && defined(_WIN32)
        // give-up rate meter: root-cause instrument for the post-knot-trio
        // throughput regression (total is exact; detail lines throttled)
        if (GetOwner()->IsAI()) {
            static DWORD s_dwGiveupTotal = 0;
            ++s_dwGiveupTotal;
            static DWORD s_dwNextGuLog = 0;
            if (theGame.GettimeGetTime() >= s_dwNextGuLog) {
                s_dwNextGuLog = theGame.GettimeGetTime() + 2000;
                char szG[144];
                sprintf(szG, "[GIVEUP] total %lu veh %lu vtype %d retries %d bc %ld at %d,%d dest %d,%d\n",
                        (unsigned long)s_dwGiveupTotal, (unsigned long)GetID(), GetData()->GetType(),
                        m_iNumRetries, (long)m_iBlockCount, m_ptHead.x, m_ptHead.y, m_ptDest.x, m_ptDest.y);
                OutputDebugStringA(szG);
            }
        }
#endif
        m_dwStagnantSince = 0;   // one give-up per stagnation window
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

    // an ownerless vehicle cannot CONTINUE a move (the grab below assumes an
    // owned head/tail) - route it through the designed re-acquire ladder
    // instead. [MOVNOOWN] upstream names each producer for its own root fix
    // (soak36 11:49: TryNextHex+0x7F was the minting site of the 6th
    // ownership crash; producers fixed so far: EnterBuilding modes,
    // dest==head shortcut, stale-next TakeOwnership, unload validation)
    if (!m_cOwn) {
        _SetRouteMode(cant_deploy);
        return (FALSE);
    }

    SetMoveParams(FALSE);
    _SetRouteMode(moving);
    theVehicleHex.GrabHex(m_ptNext, this);

    ASSERT (m_ptNext != m_ptHead);
    ASSERT_VALID_LOC (this);
    return (TRUE);
}

void CVehicle::EndReverse() {
    if (!m_bReversing)
        return;

    WaitLog("[END-REVERSE-START] veh %d mode %d owned %d head %d,%d tail %d,%d next %d,%d steps %d turn %d",
            GetID(), (int)m_cMode, (int)m_cOwn, m_ptHead.x, m_ptHead.y,
            m_ptTail.x, m_ptTail.y, m_ptNext.x, m_ptNext.y, m_iStepsLeft, m_iDadd);
    int oldDir = m_iDir;
    CMapLoc oldLoc(m_maploc);
    m_bReversing = FALSE;
    if (m_cMode == moving && m_cOwn && m_ptNext != m_ptHead &&
        theVehicleHex._GetVehicle(m_ptNext) == this &&
        m_iStepsLeft > 0 && m_iStepsLeft < STEPS_HEX) {
        // A->B->C at fraction f is C->B->A at fraction 1-f. Keep all three
        // reservations and the displayed pose; retrace the partial step nose-first.
        CSubHex oldTail(m_ptTail);
        m_ptTail = m_ptNext;
        m_ptNext = oldTail;
        m_iStepsLeft = STEPS_HEX - m_iStepsLeft;
        m_iXadd = -m_iXadd;
        m_iYadd = -m_iYadd;
        m_iDadd = -m_iDadd;
        m_iTadd = -m_iTadd;
        WaitLog("[END-REVERSE] veh %d partial dir %d>%d world %d,%d>%d,%d",
                GetID(), oldDir, m_iDir, oldLoc.x, oldLoc.y, m_maploc.x, m_maploc.y);
        return;
    }

    // A completed interpolation may still await its endpoint update.
    if (m_cMode == moving && m_cOwn && m_iStepsLeft == 0 &&
        m_ptNext != m_ptHead && theVehicleHex._GetVehicle(m_ptNext) == this)
        ArrivedNextHex();
    if (m_ptNext != m_ptHead && m_ptNext != m_ptTail &&
        theVehicleHex._GetVehicle(m_ptNext) == this)
        theVehicleHex.ReleaseHex(m_ptNext, this);
    ZeroMoveParams();
    Turn180();
    WaitLog("[END-REVERSE] veh %d settled dir %d>%d world %d,%d>%d,%d",
            GetID(), oldDir, m_iDir, oldLoc.x, oldLoc.y, m_maploc.x, m_maploc.y);
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
    if (!m_bReversing)                  // a backup keeps its facing - see SetLoc
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
    // Mirror _SetRouteMode's moving-transition side-effect. SetFromMsg is the
    // REMOTE-only apply path (both callers sit behind IsLocal early-returns) and
    // _SetRouteMode - the sole caller of Wheels() - is unreachable from here, so
    // writing m_cMode straight through left remote units' ANIM_BACK layers
    // (wheels / legs) paused from construction, forever. 1996-original bug.
    // bSfx=FALSE belt-and-braces: this isn't our unit, and Wheels' sound branch
    // is IsMe() && IsSelected() gated anyway.
    BOOL bWasMoving = ( m_cMode == moving );
    m_cMode = (CVehicle::VEH_MODE) pMsg->m_iMode;
    if ( bWasMoving != ( m_cMode == moving ) )
        Wheels( m_cMode == moving, FALSE );
    m_cOwn = pMsg->m_iOwn;
#if EN_AI_PROBES_ECON && defined(_WIN32)
    // message-borne moving-without-ownership (bypasses _SetRouteMode tripwire)
    if ( m_cMode == moving && !m_cOwn )
    {
        char szB[112];
        sprintf( szB, "[MOVNOOWN-MSG] veh %lu plyr %d at %d,%d\n", (unsigned long)GetID( ),
                 GetOwner( ) ? GetOwner( )->GetPlyrNum( ) : -1, m_ptHead.x, m_ptHead.y );
        OutputDebugStringA( szB );
    }
#endif
    m_iStepsLeft = pMsg->m_iStepsLeft;
    m_iSpeed = pMsg->m_iSpeed;
    SetOnWater(pMsg->m_bOnWater);

    SetLoc(TRUE);

    // became visible
    if (!bWorld && (IsVisible()))
        theApp.m_wndWorld.InvalidateWindow(CWndWorld::visible | CWndWorld::other_units);
}

// re-place an EXISTING vehicle at a new location (the AI 10-min stuck
// teleport posts CMsgPlaceVeh with the unit's id; CVehicle::Create has no
// relocate path and minted a DUPLICATE object on the same id - map overwrite
// orphaned the old one on its hexes = ghost unit + scrambled comp-loc deltas,
// soak38 14:18 wire TRAP). Mirrors TestStuck's hop: release, move, settle
// next=head, re-acquire through the designed deploy ladder.
void CVehicle::RelocateTo(CSubHex const &ptHead, CHexCoord const &hexDest) {

    ASSERT_VALID (this);

    // any position jump must dirty the DEPARTED painted rect: the retained GPU
    // sprite layer only repaints on dirty rects, so the origin kept a stale
    // sprite forever (operator's 'ghost crane' - renders, no hover, unhittable)
    if (IsVisible())
        theApp.m_wndWorld.InvalidateWindow(CWndWorld::visible | CWndWorld::other_units);

    ReleaseOwnership();

    m_ptHead = ptHead;
    if (GetData()->GetVehFlags() & CTransportData::FL1hex)
        m_ptTail = m_ptHead;
    else {
        m_ptTail = m_ptHead;
        m_ptTail.y += 1;
        m_ptTail.Wrap();
    }
    m_ptNext = m_ptHead;
    m_hexDest = hexDest;
    m_ptDest = CSubHex(hexDest);
    m_iDir = CalcDir();
    ZeroMoveParams();
    SetLoc(TRUE);
    AtNewLoc();

    // deploy ladder: only take the spot if it is actually free
    if ((theBuildingHex._GetBuilding(m_ptHead) == NULL) &&
        (theVehicleHex.GetVehicle(m_ptHead) == NULL) &&
        (theVehicleHex.GetVehicle(m_ptTail) == NULL)) {
        TakeOwnership();
        _SetEventAndRoute(none, stop);
    } else
        _SetEventAndRoute(none, cant_deploy);

#if EN_AI_PROBES_ECON && defined(_WIN32)
    { char szR[96]; sprintf(szR, "[RELOC] veh %lu re-placed at %d,%d own %d\n",
            (unsigned long)GetID(), m_ptHead.x / 2, m_ptHead.y / 2, (int)m_cOwn); OutputDebugStringA(szR); }
#endif

    ASSERT_VALID (this);
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
        // was TRAP+ASSERT - ReleaseOwnership frees only head/tail/next, so a
        // live vehicle entering a building can leave a stale 4th claim; this
        // checker IS the vanilla recovery (crashed the 16:08 soak)
        EN_TRAP_REMOVED( "CheckHex: claim with m_cOwn==0 - released below" );
        ReleaseHex(_sub, pVeh);
        return;
    }

    if ((pVeh->GetPtNext() != _sub) && (pVeh->GetPtHead() != _sub) && (pVeh->GetPtTail() != _sub)) {
#ifdef _LOGOUT
        logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "*** Vehicle %d at m_cOwn == 1, didn't release (%d,%d)",
                  pVeh->GetID(), _sub.x, _sub.y);
#endif
        EN_TRAP_REMOVED( "CheckHex: stale non-footprint claim - released below" );
        ReleaseHex(_sub, pVeh);
    }
}

void CVehicle::PostArrivedOrBlocked() {

    // FindNextHex can stop a failed parking path and notify directly, bypassing
    // HandleBlocked. Preserve the haul instead of reporting this temporary target
    // to the router and letting a replacement order cancel the retreat's hold.
    if (FinishClearDetour()) {
        WaitLog("[DETOUR-NOTIFY-HANDLED] veh %d", GetID());
        return;
    }
    m_bFlags |= told_ai_stop;

    // BUGS #100: a vehicle standing in its destination hex on ANOTHER sub-hex is
    // reported ARRIVED, not permanently blocked. This function decided with the
    // exact SUB test alone; run 13 saw the miss from the HandleBlocked give-up
    // (this file) and the stop backup (vehicle.cpp), where a vehicle stuck one
    // sub-hex from its exact destination sub, inside its destination hex, was told
    // BLOCKED: an AI owner got CMsgVehGoto::ToErr and abandoned a destination it had
    // reached, a human transport's router got MsgErrGoto. NOTIFICATION only: it does
    // not perform ArrivedDest, building entry, route advance or load/unload. The
    // policy per owner is unchanged - AI: CMsgVehDest, hp-router transport: MsgArrived, else nothing.
    if ((m_ptDest == m_ptHead) || m_hexDest.SameHex(m_ptHead)) {
        // tell the AI
        if (GetOwner()->IsAI()) {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            {
                // arrival-path ground truth: does the AI get told?
                char szA[112];
                sprintf(szA, "[ARRPOST] plyr %d veh %lu vtype %d mode %d at %d,%d dest %d,%d\n",
                        GetOwner()->GetPlyrNum(), (unsigned long)GetID(), GetData()->GetType(),
                        (int)m_cMode, m_ptHead.x / 2, m_ptHead.y / 2, m_ptDest.x / 2, m_ptDest.y / 2);
                OutputDebugStringA(szA);
            }
#endif
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
#if EN_AI_PROBES_ECON && defined(_WIN32)
        { char szF[96]; sprintf(szF, "[XPORTIN] veh %lu force-transported into dest bldg (fallback)\n",
                                (unsigned long)pVeh->GetID()); OutputDebugStringA(szF); }
#endif
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
#if EN_AI_PROBES_ECON && defined(_WIN32)
            { char szF[96]; sprintf(szF, "[XPORTIN] veh %lu blocked-at-bldg -> cant_deploy (fallback)\n",
                                    (unsigned long)pVeh->GetID()); OutputDebugStringA(szF); }
#endif
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
