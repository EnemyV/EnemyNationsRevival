//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "en_logpath.h"   // EnLogPath - logs to the launch dir, not the exe dir
#include "SDL2GameDialogs.h"
#include "event.h"
#include "lastplnt.h"
#include "cpathmgr.h"
#include "player.h"
#include "chproute.hpp"
#include "bridge.h"
#include "area.h"
#include "enprobes.h"

#include "building.inl"
#include "vehicle.inl"
#include "terrain.inl"
#include "en_harness.h"   // HarnessDumpTraffic (the `traffic` verb lives here, not area.cpp)

#include <map>            // TrafficCensus per-player buckets (must precede DEBUG_NEW)


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

// [NONOTIFY]/[ARRIVEMISS] caller tag (015 R12/R13 traffic probes). Defined in
// vehmove.cpp; the three CVehicle::PostArrivedOrBlocked calls in THIS file are in
// another translation unit, which is why the R12 records were full of "?". They now
// name themselves, using the same set-around-the-call pattern as the vehmove.cpp
// callers. INSTRUMENT ONLY: nothing reads it but those two log lines.
extern const char *g_pszPostWhy;

#if EN_GAMEPLAY_PROBES
// Diagnostic sink: appends to bridge_debug.log in the run dir. Deliberately NOT
// OutputDebugString — that needs a live DBWIN listener, and a dead or duplicated one
// silently drops the line AND stalls the calling thread ~10s (July ODS-gap lag bug).
void EnBridgeDbgLog( const char* pszMsg )
{
    FILE* f = fopen( EnLogPath( "bridge_debug.log" ).c_str(), "a" );
    if ( f != NULL ) { fputs( pszMsg, f ); fclose( f ); }
}
#endif

// Traffic probes (docs/plans/015-focus-investigation.md 1.3, discussion repo). A file
// sink switched at RUNTIME by EN_TRAFFIC_LOG, so the same build serves every seat and
// the probes can stay in Release: the compile-gated ODS meters they replace were
// Windows-only, AI-only and needed a DBWIN listener. Same shape as the HP router log
// (chproute.cpp): GetEnvironmentVariableA/GetTickCount are shimmed on POSIX, and each
// line is append+close so a crash keeps what was written.
bool EnTrafficLogOn()
{
    static int s_iOn = -1;
    if (s_iOn < 0) {
        char szBuf[8];
        s_iOn = (GetEnvironmentVariableA("EN_TRAFFIC_LOG", szBuf, sizeof(szBuf)) > 0 && szBuf[0] != '0') ? 1 : 0;
    }
    return s_iOn != 0;
}

// EN_TRAFFIC_VEH is now a COMMA-SEPARATED list (015 R19 [STEP] probe). The single-id
// [FOLLOW] probe below keeps working: it takes the FIRST id of the list. The buffer is
// read once into s_szVehList - a 16-byte buffer would have made GetEnvironmentVariableA
// return "needed size" and write NOTHING for a list, silently zeroing the follow id.
static char s_szTrafVehList[EN_TRAF_VEH_BUF] = {0};
static bool s_bTrafVehRead = false;

static const char *EnTrafficVehEnv()
{
    if (!s_bTrafVehRead) {
        s_bTrafVehRead = true;
        if (GetEnvironmentVariableA("EN_TRAFFIC_VEH", s_szTrafVehList, sizeof(s_szTrafVehList)) >= sizeof(s_szTrafVehList))
            s_szTrafVehList[0] = 0;   // over-long list: buffer untouched by the API, do not read garbage
    }
    return s_szTrafVehList;
}

DWORD EnTrafficFollowId()
{
    static long s_lId = -1;
    if (s_lId < 0) {
        s_lId = atol(EnTrafficVehEnv());   // first id of the list
        if (s_lId < 0)
            s_lId = 0;
    }
    return (DWORD) s_lId;
}

// [STEP] (015 R19) membership test for EN_TRAFFIC_VEH. Parsed ONCE into a fixed array of
// at most EN_TRAF_VEH_MAX ids; unset/empty = probe off (returns false for every id, so
// the [STEP] stream cannot flood). Read-only, no game state touched.
bool EnTrafficVehListed(DWORD dwId)
{
    static DWORD s_adwIds[EN_TRAF_VEH_MAX];
    static int   s_iCount = -1;
    if (s_iCount < 0) {
        s_iCount = 0;
        const char *psz = EnTrafficVehEnv();
        while ((*psz != 0) && (s_iCount < EN_TRAF_VEH_MAX)) {
            while ((*psz == ' ') || (*psz == ',') || (*psz == '	'))
                psz++;
            if ((*psz < '0') || (*psz > '9'))
                break;
            long lId = atol(psz);
            if (lId > 0)
                s_adwIds[s_iCount++] = (DWORD) lId;
            while ((*psz >= '0') && (*psz <= '9'))
                psz++;
        }
    }
    for (int i = 0; i < s_iCount; i++)
        if (s_adwIds[i] == dwId)
            return true;
    return false;
}

void EnTrafficLog(const char *pszFmt, ...)
{
    if (!EnTrafficLogOn())
        return;
    FILE *pFile = fopen(EnLogPath("traffic.log").c_str(), "a");
    if (pFile == NULL)
        return;
    fprintf(pFile, "[%9lu] ", (unsigned long) GetTickCount());
    va_list args;
    va_start(args, pszFmt);
    vfprintf(pFile, pszFmt, args);
    va_end(args);
    fputc('\n', pFile);
    fclose(pFile);
}

// ---------------------------------------------------------------------------
// R10 traffic denominators (015 T2 item 4). Declared in vehicle.h with the field
// documentation; player numbers are handed out at runtime with no compile-time
// ceiling, so EVERY access is range-checked against EN_AI_TICK_PLYRS - the same
// rule as ai.cpp's g_alAiManageTicks and caitmgr.cpp's seek-scan counters.
// These are EXACT event counts and are deliberately kept separate from the
// sampled [SUBATTEMPT]/[STUCK] detail: a rate needs a denominator that is not
// itself throttled. Written from the AI threads (orders/sweep) and the game
// thread (steps); read unlocked by TrafficCensus, which is the same
// single-word-read discipline the AI counters above already use.
// ---------------------------------------------------------------------------
volatile long g_alTrafOrdersOk[EN_AI_TICK_PLYRS]   = { 0 };
volatile long g_alTrafOrdersWake[EN_AI_TICK_PLYRS] = { 0 };
volatile long g_alTrafSteps[EN_AI_TICK_PLYRS]      = { 0 };
volatile long g_alTrafDeliveries[EN_AI_TICK_PLYRS] = { 0 };
volatile long g_alTrafSweepMsMax[EN_AI_TICK_PLYRS] = { 0 };

// T2b GetPath return classes (015 T2 item 1). Declared in vehicle.h with the class
// documentation; stamped in unit.cpp at every exit of CVehicle::GetPath, exactly once
// per call. [player][mode]. bNoOcc goes straight into CPathMgr::GetPath's `bVehBlock`
// ("TRUE means vehicles will block", cpathmgr.h:134-135, applied at cpathmgr.cpp:1072),
// so the T2b comments had this BACKWARDS - corrected 015 R11 item 3:
//   mode 0 = bNoOcc FALSE = vehicle-FREE  (census suffix `_free`)
//   mode 1 = bNoOcc TRUE  = vehicle-AWARE (census suffix `_aware`)
// Written from whichever thread called GetPath, read unlocked by TrafficCensus - the
// same single-word-read discipline as the counters above.
volatile long g_alTrafPathEmpty[EN_AI_TICK_PLYRS][2]   = { { 0 } };
volatile long g_alTrafPathRepPart[EN_AI_TICK_PLYRS][2] = { { 0 } };
volatile long g_alTrafPathDegen[EN_AI_TICK_PLYRS][2]   = { { 0 } };
volatile long g_alTrafPathAccPart[EN_AI_TICK_PLYRS][2] = { { 0 } };
volatile long g_alTrafPathFull[EN_AI_TICK_PLYRS][2]    = { { 0 } };

// One census line per LOCAL player. "inbldg_notdest" is the count the DOORSTEP
// hypothesis predicts (a vehicle inside a building that is neither its destination
// nor its construction site); the retry histogram over blocked vehicles shows how
// many sit in the turn/180 rungs (>= 11) that make the narrow-street dance.
// inbldg_wrong = vehicles still inside a building they entered wrongly (stamped by
// [ENTERWRONG], cleared by ExitBuilding), maxwrong_ms = the longest such dwell.
// maxstag_ms counts BLOCKED vehicles only; oldstamp_* names the oldest stagnation
// stamp in any mode (a stamp nothing cleared on the way out of blocked reads as a
// huge age forever, which is why the two are now separate numbers). aiticks is the
// AI worker's Manage() counter, 0 for humans. Also emits [STUCK] per blocked vehicle
// stagnant over 60 s, naming the vehicle sitting on the hex it wants next.
void CVehicle::TrafficCensus(std::string &out)
{
    struct CCensus {
        int iAI, iTrucks, iCranes, iMoving, iBlocked, iStop, iCantDeploy, iTraffic, iInBldg, iInBldgNotDest;
        int iInBldgWrong;
        int iContention, iDeployIt, iRun;
        int iStranded;         // R10: vehicles WITH an order sitting in stop/blocked right now
        int aiRetries[4];
        DWORD dwMaxStag;
        DWORD dwMaxWrong;
        DWORD dwOldStamp;      // oldest surviving stagnation stamp, ANY mode
        CVehicle *pOldStamp;   // the vehicle holding it (NULL = none)
        CCensus() { memset(this, 0, sizeof(*this)); }
    };
    std::map<int, CCensus> mapPlyr;
    DWORD dwNow = theGame.GettimeGetTime();   // m_dwStagnantSince is stamped from this clock
    // [SUBATTEMPT] ages are stamped from the RAW clock in FindSub, not the frame-cached
    // game clock, so the STUCK context needs its own read of the same clock.
    DWORD dwRawNow = timeGetTime();

    POSITION pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        if ((pVeh == NULL) || (pVeh->GetOwner() == NULL) || (!pVeh->GetOwner()->IsLocal()))
            continue;

        CCensus &c = mapPlyr[pVeh->GetOwner()->GetPlyrNum()];
        c.iAI = pVeh->GetOwner()->IsAI() ? 1 : 0;

        CTransportData const *pData = pVeh->GetData();
        if (pData != NULL) {
            if (pData->IsTransport())
                c.iTrucks++;
            if (pData->IsCrane())
                c.iCranes++;
        }

        switch (pVeh->m_cMode) {
            case moving :       c.iMoving++;      break;
            case blocked :      c.iBlocked++;     break;
            case stop :         c.iStop++;        break;
            case cant_deploy :  c.iCantDeploy++;  break;
            case traffic :      c.iTraffic++;     break;
            case contention :   c.iContention++;  break;
            case deploy_it :    c.iDeployIt++;    break;
            case run :          c.iRun++;         break;
            default :           break;
        }
        if (pVeh->m_cMode == blocked) {
            int iR = pVeh->m_iNumRetries;
            c.aiRetries[iR < 5 ? 0 : (iR < 10 ? 1 : (iR < 15 ? 2 : 3))]++;
        }

        // R10 `stranded_ms` cohort (015 T2 item 4): a vehicle that HAS somewhere to be
        // and is going nowhere - stop or blocked, while it still holds a destination it
        // has not reached, a pending arrival event, or an unfinished route. Counted here
        // as a headcount; the TIME is integrated below over the elapsed census interval,
        // never by re-summing current ages (which double-counts every pass).
        if (((pVeh->m_cMode == stop) || (pVeh->m_cMode == blocked)) &&
            ((pVeh->m_ptDest != pVeh->m_ptHead) || (pVeh->m_iEvent != none) || (pVeh->m_pos != NULL)))
            c.iStranded++;

        if (pVeh->IsInBuilding()) {
            c.iInBldg++;
            CBuilding *pOn = theBuildingHex._GetBuilding(pVeh->m_ptHead);
            if ((pOn != NULL) && (pOn != theBuildingHex._GetBuilding(pVeh->m_hexDest)) && (pOn != pVeh->m_pBldg))
                c.iInBldgNotDest++;
        }

        if (pVeh->m_dwEnteredWrongAt != 0) {
            c.iInBldgWrong++;
            DWORD dwAge = dwNow - pVeh->m_dwEnteredWrongAt;
            if (dwAge > c.dwMaxWrong)
                c.dwMaxWrong = dwAge;
        }

        if (pVeh->m_dwStagnantSince != 0) {
            DWORD dwAge = dwNow - pVeh->m_dwStagnantSince;
            // maxstag_ms is the BLOCKED-mode stagnation the give-up ladder acts on. It used to
            // be taken over every stamped vehicle, so a stamp left behind by a mode change
            // (nothing clears it on the way out of blocked) inflated it forever.
            if ((pVeh->m_cMode == blocked) && (dwAge > c.dwMaxStag))
                c.dwMaxStag = dwAge;
            // oldstamp_* keeps that stale-stamp signal, now named: the oldest stamp in ANY
            // mode plus who holds it, so a leaked stamp can be told from a real jam.
            if (dwAge > c.dwOldStamp) {
                c.dwOldStamp = dwAge;
                c.pOldStamp  = pVeh;
            }
            // [STUCK]: one line per blocked vehicle stagnant over a minute, emitted once per
            // census pass (30 s), naming whoever is sitting on the hex it wants next.
            if (EnTrafficLogOn() && (pVeh->m_cMode == blocked) && (dwAge > 60000)) {
                CVehicle *pBlk = theVehicleHex._GetVehicle(pVeh->m_ptNext);

                // [SUBATTEMPT] cross-reference (015 T2 item 2): the sequence/age of the
                // LAST real selector attempt on this vehicle. `unknown` = FindSub never
                // ran under the probe for it; `stale` = the attempt is older than 60 s,
                // i.e. it says nothing about the neighbourhood printed below. The two
                // labels exist so a later snapshot is never read back onto an earlier
                // failed attempt.
                char szSeq[32], szAge[32];
                if (pVeh->m_subAttempt.dwSeq == 0) {
                    strcpy(szSeq, "unknown");
                    strcpy(szAge, "unknown");
                } else {
                    DWORD dwSubAge = dwRawNow - pVeh->m_subAttempt.dwTimeMs;
                    sprintf(szSeq, "%lu", (unsigned long) pVeh->m_subAttempt.dwSeq);
                    if (dwSubAge > 60000)
                        strcpy(szAge, "stale");
                    else
                        sprintf(szAge, "%lu", (unsigned long) dwSubAge);
                }

                // Eight-neighbour snapshot around the HEAD sub, as CONTEXT ONLY - it is a
                // snapshot, not a cause. Every neighbour carries its terrain type AND its
                // building AND its occupant, because occupancy does not rule out a
                // simultaneous terrain/door constraint, and an unoccupied passable grass
                // sub prints a terrain type too (WinAstra review 10). No verdict is
                // derived here and none should be derived from it.
                static const int aiCtxDx[8] = { -1,  0,  1, -1,  1, -1,  0,  1 };
                static const int aiCtxDy[8] = { -1, -1, -1,  0,  0,  1,  1,  1 };
                char szCtx[512];
                int  iCtxOff = 0;
                szCtx[0] = 0;
                for (int iN = 0; iN < 8; iN++) {
                    CSubHex _sN(pVeh->m_ptHead.x + aiCtxDx[iN], pVeh->m_ptHead.y + aiCtxDy[iN]);
                    _sN.Wrap();
                    CVehicle  *pVN = theVehicleHex._GetVehicle(_sN);
                    CBuilding *pBN = theBuildingHex._GetBuilding(_sN);
                    CHex      *pHN = theMap._GetHex(_sN);
                    int iLeft = (int) sizeof(szCtx) - iCtxOff;
                    if (iLeft <= 1)
                        break;
                    int iPut = snprintf(szCtx + iCtxOff, iLeft, "%s%d,%d t%d b%lu v%lu m%d s%d",
                                        (iN ? " " : ""), aiCtxDx[iN], aiCtxDy[iN],
                                        pHN != NULL ? pHN->GetType() : -1,
                                        pBN != NULL ? (unsigned long) pBN->GetID() : 0UL,
                                        pVN != NULL ? (unsigned long) pVN->GetID() : 0UL,
                                        pVN != NULL ? (int) pVN->m_cMode : -1,
                                        (pVN == pVeh) ? 1 : 0);
                    if ((iPut < 0) || (iPut >= iLeft))
                        break;
                    iCtxOff += iPut;
                }

                EnTrafficLog("[STUCK] veh %lu vtype %d plyr %d ai %d stagnant_ms %lu retries %d bc %ld "
                             "head %d,%d next %d,%d blocker %lu blocker_mode %d blocker_next %d,%d "
                             "lastsub %s lastsub_age_ms %s ctx %s",
                             (unsigned long) pVeh->GetID(), pData != NULL ? pData->GetType() : -1,
                             pVeh->GetOwner()->GetPlyrNum(), pVeh->GetOwner()->IsAI() ? 1 : 0,
                             (unsigned long) dwAge, pVeh->m_iNumRetries, (long) pVeh->m_iBlockCount,
                             pVeh->m_ptHead.x, pVeh->m_ptHead.y, pVeh->m_ptNext.x, pVeh->m_ptNext.y,
                             pBlk != NULL ? (unsigned long) pBlk->GetID() : 0UL,
                             pBlk != NULL ? (int) pBlk->m_cMode : -1,
                             pBlk != NULL ? pBlk->m_ptNext.x : 0,
                             pBlk != NULL ? pBlk->m_ptNext.y : 0,
                             szSeq, szAge, szCtx);
            }
        }
    }

    // scans-per-minute is a ROLLING rate: the delta in g_alAiScansRun over the
    // wall time since this function last looked at that player. Sampling the
    // census on demand (harness `traffic`) just makes the window shorter, not
    // the rate wrong; the first sample after start has no window and prints 0.
    static long  s_alScansPrev[EN_AI_TICK_PLYRS] = { 0 };
    static DWORD s_adwScansAt[EN_AI_TICK_PLYRS]  = { 0 };

    // stranded_ms is INTEGRATED, not sampled: each pass adds (this player's stranded
    // headcount) x (ms since the previous pass for that player). Re-summing current
    // ages every pass would count the same wait over and over; this does not. The
    // harness `traffic` verb just makes the intervals shorter, which is still correct.
    // Unit: vehicle-milliseconds. The first pass for a player has no window and adds 0.
    static DWORD s_adwStrandAt[EN_AI_TICK_PLYRS] = { 0 };
    static DWORD s_adwStrandMs[EN_AI_TICK_PLYRS] = { 0 };

    char szLine[1536];
    for (std::map<int, CCensus>::const_iterator it = mapPlyr.begin(); it != mapPlyr.end(); ++it) {
        CCensus const &c = it->second;
        CHexCoord hexOld = (c.pOldStamp != NULL) ? c.pOldStamp->GetHexHead() : CHexCoord(0, 0);
        CTransportData const *pOldData = (c.pOldStamp != NULL) ? c.pOldStamp->GetData() : NULL;
        // aiticks: the AI worker's Manage() counter for this player (ai.cpp). A frozen
        // counter with live vehicles means the AI thread died, not that traffic jammed.
        unsigned long ulAiTicks = 0;
        // seek-scan block (caitmgr.cpp/caigmgr.cpp): scans run/skipped and what
        // they cost, walks started vs finished, the cs takes part C leaves in
        // place, targets held and the scan rate. All zero for human players.
        unsigned long ulScans = 0, ulSkipB = 0, ulScanAvg = 0, ulScanMax = 0;
        unsigned long ulWalks = 0, ulWalksDone = 0;
        unsigned long ulCandAvg = 0, ulCandMax = 0, ulOpForAvg = 0, ulOpForMax = 0;
        unsigned long ulTgtHeld = 0, ulScansPm = 0, ulSeekLoops = 0;
        if (c.iAI && (it->first >= 0) && (it->first < EN_AI_TICK_PLYRS)) {
            int const iP = it->first;
            ulAiTicks   = (unsigned long) g_alAiManageTicks[iP];
            ulScans     = (unsigned long) g_alAiScansRun[iP];
            ulSkipB     = (unsigned long) g_alAiScansSkipBudget[iP];
            ulScanMax   = (unsigned long) g_alAiScanMsMax[iP];
            ulWalks     = (unsigned long) g_alAiWalksStarted[iP];
            ulWalksDone = (unsigned long) g_alAiWalksDone[iP];
            ulCandMax   = (unsigned long) g_alAiScanCandMax[iP];
            ulOpForMax  = (unsigned long) g_alAiScanOpForMax[iP];
            ulTgtHeld   = (unsigned long) g_alAiSeekTargets[iP];
            ulSeekLoops = (unsigned long) g_alAiSeekLoops[iP];
            if (ulScans) {
                ulScanAvg  = ((unsigned long) g_alAiScanMsTotal[iP]) / ulScans;
                ulCandAvg  = ((unsigned long) g_alAiScanCandTotal[iP]) / ulScans;
                ulOpForAvg = ((unsigned long) g_alAiScanOpForTotal[iP]) / ulScans;
            }
            DWORD dwRateNow = timeGetTime();   // raw: dwNow is the frame-cached clock
            if (s_adwScansAt[iP] != 0) {
                DWORD dwSpan = dwRateNow - s_adwScansAt[iP];
                if (dwSpan >= 1000) {
                    long lDelta = g_alAiScansRun[iP] - s_alScansPrev[iP];
                    if (lDelta < 0)
                        lDelta = 0;
                    ulScansPm = (unsigned long) ((60000.0 * (double) lDelta) / (double) dwSpan);
                }
            }
            s_alScansPrev[iP] = g_alAiScansRun[iP];
            s_adwScansAt[iP]  = dwRateNow;
        }
        // R10 denominators (015 T2 item 4). Range-checked like every other per-player
        // probe read; a player number outside the bound prints zeros rather than
        // indexing off the end. `deliveries` counts BOTH routers' unload paths - the
        // AI one in caimgr.cpp and the human one in chproute.cpp - see the vehicle.h note.
        unsigned long ulOrdersOk = 0, ulOrdersWake = 0, ulSteps = 0, ulDeliv = 0;
        unsigned long ulStrandMs = 0, ulSweepMax = 0;
        // T2b GetPath return classes: path_<class>_<free|aware>. bNoOcc is CPathMgr's
        // `bVehBlock`, so index 0 (bNoOcc FALSE) is the vehicle-FREE search and index 1
        // (bNoOcc TRUE) is the vehicle-AWARE one - the T2b suffixes `_a`/`_f` named these
        // the wrong way round and are renamed here (015 R11 item 3). Exactly one class is
        // stamped per GetPath call, so these ten sum to the call count for that player.
        unsigned long ulPathEFr = 0, ulPathEAw = 0, ulPathRFr = 0, ulPathRAw = 0;
        unsigned long ulPathDFr = 0, ulPathDAw = 0, ulPathPFr = 0, ulPathPAw = 0;
        unsigned long ulPathFFr = 0, ulPathFAw = 0;
        if ((it->first >= 0) && (it->first < EN_AI_TICK_PLYRS)) {
            int const iPd = it->first;
            ulOrdersOk   = (unsigned long) g_alTrafOrdersOk[iPd];
            ulOrdersWake = (unsigned long) g_alTrafOrdersWake[iPd];
            ulSteps      = (unsigned long) g_alTrafSteps[iPd];
            ulDeliv      = (unsigned long) g_alTrafDeliveries[iPd];
            ulSweepMax   = (unsigned long) g_alTrafSweepMsMax[iPd];
            ulPathEFr    = (unsigned long) g_alTrafPathEmpty[iPd][0];
            ulPathEAw    = (unsigned long) g_alTrafPathEmpty[iPd][1];
            ulPathRFr    = (unsigned long) g_alTrafPathRepPart[iPd][0];
            ulPathRAw    = (unsigned long) g_alTrafPathRepPart[iPd][1];
            ulPathDFr    = (unsigned long) g_alTrafPathDegen[iPd][0];
            ulPathDAw    = (unsigned long) g_alTrafPathDegen[iPd][1];
            ulPathPFr    = (unsigned long) g_alTrafPathAccPart[iPd][0];
            ulPathPAw    = (unsigned long) g_alTrafPathAccPart[iPd][1];
            ulPathFFr    = (unsigned long) g_alTrafPathFull[iPd][0];
            ulPathFAw    = (unsigned long) g_alTrafPathFull[iPd][1];
            if (s_adwStrandAt[iPd] != 0)
                s_adwStrandMs[iPd] += (dwRawNow - s_adwStrandAt[iPd]) * (DWORD) c.iStranded;
            s_adwStrandAt[iPd] = dwRawNow;
            ulStrandMs         = (unsigned long) s_adwStrandMs[iPd];
        }
        snprintf(szLine, sizeof(szLine),
                 "traffic plyr %d ai %d trucks %d cranes %d moving %d blocked %d stop %d cantdeploy %d traffic %d "
                 "inbldg %d inbldg_notdest %d r0_4 %d r5_9 %d r10_14 %d r15p %d maxstag_ms %lu "
                 "inbldg_wrong %d maxwrong_ms %lu oldstamp_ms %lu oldstamp_veh %lu oldstamp_mode %d "
                 "oldstamp_type %d oldstamp_hex %d,%d contention %d deployit %d run %d aiticks %lu "
                 "scans %lu skipb %lu scanms_avg %lu scanms_max %lu walks %lu walksdone %lu "
                 "cand_avg %lu cand_max %lu opfor_avg %lu opfor_max %lu tgtheld %lu scanspm %lu seekloops %lu "
                 "orders_ok %lu orders_wake %lu steps %lu deliveries %lu stranded %d stranded_ms %lu "
                 "sweep_ms_max %lu path_e_free %lu path_e_aware %lu path_rp_free %lu path_rp_aware %lu "
                 "path_d_free %lu path_d_aware %lu path_ap_free %lu path_ap_aware %lu "
                 "path_f_free %lu path_f_aware %lu\n",
                 it->first, c.iAI, c.iTrucks, c.iCranes, c.iMoving, c.iBlocked, c.iStop, c.iCantDeploy, c.iTraffic,
                 c.iInBldg, c.iInBldgNotDest, c.aiRetries[0], c.aiRetries[1], c.aiRetries[2], c.aiRetries[3],
                 (unsigned long) c.dwMaxStag, c.iInBldgWrong, (unsigned long) c.dwMaxWrong,
                 (unsigned long) c.dwOldStamp,
                 c.pOldStamp != NULL ? (unsigned long) c.pOldStamp->GetID() : 0UL,
                 c.pOldStamp != NULL ? (int) c.pOldStamp->m_cMode : 0,
                 pOldData != NULL ? pOldData->GetType() : 0,
                 c.pOldStamp != NULL ? (int) hexOld.X() : 0, c.pOldStamp != NULL ? (int) hexOld.Y() : 0,
                 c.iContention, c.iDeployIt, c.iRun, ulAiTicks,
                 ulScans, ulSkipB, ulScanAvg, ulScanMax, ulWalks, ulWalksDone,
                 ulCandAvg, ulCandMax, ulOpForAvg, ulOpForMax, ulTgtHeld, ulScansPm, ulSeekLoops,
                 ulOrdersOk, ulOrdersWake, ulSteps, ulDeliv, c.iStranded, ulStrandMs, ulSweepMax,
                 ulPathEFr, ulPathEAw, ulPathRFr, ulPathRAw, ulPathDFr, ulPathDAw,
                 ulPathPFr, ulPathPAw, ulPathFFr, ulPathFAw);
        out += szLine;
    }
}

// Backs the harness `traffic` verb (control_socket.cpp cannot see CVehicle).
void HarnessDumpTraffic(std::string &out)
{
    out.clear();
    CVehicle::TrafficCensus(out);
    if (out.empty())
        out = "err no local vehicles (not in-game?)\n";
}

int aiBaseDir[9] = {7, 6, 5, 0, 0, 4, 1, 2, 3};
int aiDir[9] = {7 * EIGHTH_ROT, 6 * EIGHTH_ROT, 5 * EIGHTH_ROT, 0, 0, 4 * EIGHTH_ROT, 1 * EIGHTH_ROT, 2 * EIGHTH_ROT,
                3 * EIGHTH_ROT};


BOOL CVehicle::TestStuck() {

    // VANILLA GUARD RESTORED (operator, 2026-07-16: 'horrible change, revert
    // for sure'): 2cc7163c flipped this so the 6-min stuck-escape ran for AI
    // vehicles - TELEPORTING AI trucks/cranes into their destination buildings
    if ((!GetOwner()->IsLocal()) || (GetOwner()->IsAI()))
        return FALSE;

    // only for trucks & cranes
    if ((!GetData()->IsTransport()) && (!GetData()->IsCrane()))
        return FALSE;

    // are we there?
    if (m_hexDest.SameHex(m_ptHead))
        return FALSE;

    if (m_dwTimeJump > theGame.GettimeGetTime())
        return FALSE;
    m_dwTimeJump = theGame.GettimeGetTime() + 1000 * TRUCK_JUMP_TIME;

    // ok, we transport to the dest if we can
    // NOTE: the TRAPs below were 1996 curiosity breakpoints in a path that was
    // dead for AI units until the gate ruling; they fire on ROUTINE states for a
    // 6-min-stuck unit (occupied hexes, unenterable dest) and killed Debug
    // observation runs. Logged instead - TestStuck only; TRAP stays fatal elsewhere.
    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptDest);
    if (pBldg != NULL) {
        if (!CanEnterBldg(pBldg)) {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            { char szT[64]; sprintf(szT, "[TSTRAP] veh %lu entercant\n", (unsigned long)GetID()); OutputDebugStringA(szT); }
#endif
            return FALSE;
        }

        if (m_pTransport != NULL) {
#if EN_AI_PROBES_ECON && defined(_WIN32)
            { char szT[64]; sprintf(szT, "[TSTRAP] veh %lu carried\n", (unsigned long)GetID()); OutputDebugStringA(szT); }
#endif
            POSITION pos = m_pTransport->m_lstCargo.Find(this);
            if (pos != NULL)
                m_pTransport->m_lstCargo.RemoveAt(pos);
            m_pTransport = NULL;
        }

        ReleaseOwnership();
        ForceAtDest();
        ArrivedDest();
#if EN_AI_PROBES_ECON && defined(_WIN32)
        {
            char szJ[80];
            sprintf(szJ, "[JUMP] plyr %d veh %lu into bldg %lu\n", GetOwner()->GetPlyrNum(),
                    (unsigned long)GetID(), (unsigned long)pBldg->GetID());
            OutputDebugStringA(szJ);
        }
#endif
        return TRUE;
    }

    // if no path left, give up
    if ((m_phexPath == NULL) || (m_iPathLen < m_iPathOff + 5))
        return FALSE;

    // not a building, can we find a clear set of hexes along it's path?
    CHexCoord *pHexOn = m_phexPath + m_iPathOff + 2;
    int iNumTries = m_iPathLen - m_iPathOff - 3;
    while (iNumTries > 0) {
        // do we have a clear hex?
        if (!(theMap._GetHex(*pHexOn)->GetUnits() & CHex::unit)) {
            // put it here pointing at the next hex
            CSubHex const _subHeadWas = m_ptHead;   // [STEP] (015 R20)
            ReleaseOwnership();
            if (pHexOn->X() < (pHexOn + 1)->X()) {
                m_ptTail.x = pHexOn->X() * 2;
                m_ptHead.y = m_ptTail.y = pHexOn->Y() * 2 + 1;
                m_ptHead.x = m_ptTail.x + 1;
            } else if (pHexOn->X() > (pHexOn + 1)->X()) {
                m_ptHead.x = pHexOn->X() * 2;
                m_ptHead.y = m_ptTail.y = pHexOn->Y() * 2;
                m_ptTail.x = m_ptHead.x + 1;
            } else if (pHexOn->Y() < (pHexOn + 1)->Y()) {
                m_ptHead.y = pHexOn->Y() * 2 + 1;
                m_ptHead.x = m_ptTail.x = pHexOn->X() * 2;
                m_ptTail.y = m_ptHead.y - 1;
            } else {
                m_ptHead.y = pHexOn->Y() * 2;
                m_ptHead.x = m_ptTail.x = pHexOn->X() * 2 + 1;
                m_ptTail.y = m_ptHead.y + 1;
            }

            // get it going
            if (m_pTransport != NULL) {
#if EN_AI_PROBES_ECON && defined(_WIN32)
                { char szT[64]; sprintf(szT, "[TSTRAP] veh %lu carried2\n", (unsigned long)GetID()); OutputDebugStringA(szT); }
#endif
                POSITION pos = m_pTransport->m_lstCargo.Find(this);
                if (pos != NULL)
                    m_pTransport->m_lstCargo.RemoveAt(pos);
                m_pTransport = NULL;
            }

            m_pszSelWhy = "stuckhop";   // [STEP] selector tag: 6-min stuck fallback hop
            m_ptNext = m_ptHead;
            m_hexNext = *(pHexOn + 1);

            // [STEP] (015 R20): the 6-minute fallback TELEPORTS the head along the path -
            // it never reaches the ArrivedNextHex commit, so record it as a relocation.
            EnTrafStepLog(this, "stuckhop_relocate", _subHeadWas, m_ptHead, (int) m_iDir, (int) m_cMode);

            SetMoveParams(FALSE);
            AtNewLoc();
            TakeOwnership();
            _SetRouteMode(moving);
#if EN_AI_PROBES_ECON && defined(_WIN32)
            { char szF[96]; sprintf(szF, "[STUCKHOP] veh %lu hopped along path (6-min fallback)\n",
                                    (unsigned long)GetID()); OutputDebugStringA(szF); }
#endif
            return TRUE;
        }

        pHexOn++;
        iNumTries--;  // was never decremented (since 1996): loop walked past the path array
    }

    // we failed
#if EN_AI_PROBES_ECON && defined(_WIN32)
    { char szT[64]; sprintf(szT, "[TSTRAP] veh %lu nofree\n", (unsigned long)GetID()); OutputDebugStringA(szT); }
#endif
    return FALSE;
}

void CVehicle::Operate() {

    xASSERT_VALID (ASSERT_PRI_ANAL, ASSERT_VEH_MOVE, this);

    if (m_iFrameHit > 0) {
        m_iFrameHit -= theGame.GetFramesElapsed();
        if (m_iFrameHit <= 0) {
            m_iFrameHit = 0;
            theApp.m_wndWorld.SetVehHit();
        }
    }

    if ((m_unitFlags & (dying | stopped)) || (GetOwner() == NULL))
        return;

    // are we destroying?
    if (GetOwner()->IsLocal() && (m_unitFlags & CUnit::destroying)) {
        int iNum = (int) theGame.GetOpersElapsed() + m_lOperMod;
        div_t dtKill = div(iNum, AVG_SPEED_MUL);
        m_lOperMod = dtKill.rem;
        if (dtKill.quot > 0)
            AddDamageThisTurn(this, dtKill.quot);
    }

    // first we handle combat stuff
    HandleCombat();

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

    switch (m_cMode) {
        // nothing happening
        case stop : {
            if (!GetOwner()->IsLocal())
                return;

            if (TestStuck())
                return;

            xASSERT (ASSERT_PRI_ANAL, ASSERT_VEH_MOVE, (!m_cOwn) || (theBuildingHex.GetBuilding(m_ptHead) == NULL));
            if (!(m_bFlags & told_ai_stop)) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE,
                          "!! backup method - AI told vehicle %d at sub (%d,%d) stopped", GetID(), m_ptHead.x,
                          m_ptHead.y);
#endif
                g_pszPostWhy = "stopbackup";   // [NONOTIFY]/[ARRIVEMISS] caller tag
                PostArrivedOrBlocked();
                g_pszPostWhy = NULL;
            }
            ASSERT ((m_bFlags & told_ai_stop) || (m_ptDest == m_ptHead));

            // handle setup fire here - we just 0 the reload
            int iSetup = GetData()->GetSetupFire();
            if ((iSetup > 0) && (m_FireSetupMod < iSetup)) {
                m_FireSetupMod += (int) ((float) theGame.GetOpersElapsed() * m_fDamPerfMult) / AVG_SPEED_MUL;
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
            return;
        }

        case moving :
#ifdef STRICTER_ASSERTS
            ASSERT (m_cOwn);
#endif
            Move();
            m_FireSetupMod = 0;
            break;

        case contention :
            TRAP();
            // BUGBUG - write this
            if (TestStuck())
                return;
            return;

        case traffic : {
            if (TestStuck())
                return;

            if (GetOwner()->IsLocal()) {
                m_dwTimeBlocked += theGame.GetFramesElapsed();
                if (m_dwTimeBlocked < 24)
                    return;
                _SetRouteMode(blocked);
                m_dwTimeBlocked = 0;
                m_iNumRetries = m_iBlockCount = 0;
                return;
            }

            // remote - see if the last know location is avail
            CVehicle *pVehHead = theVehicleHex._GetVehicle(m_mvlTraffic.m_ptHead);
            if ((pVehHead == NULL) || (pVehHead == this)) {
                CVehicle *pVehTail = theVehicleHex._GetVehicle(m_mvlTraffic.m_ptTail);
                if ((pVehTail == NULL) || (pVehTail == this)) {
                    // we just assume it's changing hexes
                    if (DoSpotting() && GetHexOwnership()) {
                        TRAP();
                        DecrementSpotting();
                    }
                    ReleaseOwnership();

                    // if next is free we move. Otherwise we just sit here
                    CVehicle *pVehNext = theVehicleHex._GetVehicle(m_mvlTraffic.m_ptNext);
                    if ((pVehNext == NULL) || (pVehNext == this))
                        SetFromMsg(&m_mvlTraffic, FALSE);
                    else {
                        m_mvlTraffic.m_ptNext = m_mvlTraffic.m_ptHead;
                        SetFromMsg(&m_mvlTraffic, FALSE);
                        _SetRouteMode(stop);
                    }

                    // grab the hexes in the new location
                    if (m_cOwn) {
                        TRAP((m_ptNext.x > 1280) || (m_ptNext.y > 1280));
                        theVehicleHex.GrabHex(m_ptNext, this);
                        theVehicleHex.GrabHex(m_ptHead, this);
                        theVehicleHex.GrabHex(m_ptTail, this);

                        if (DoSpotting()) {
                            TRAP();
                            DetermineSpotting();
                            IncrementSpotting(m_ptHead);
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
            return;
        }

        case blocked :
            if (TestStuck())
                return;

            if (GetOwner()->IsLocal()) {
                HandleBlocked();

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
            if (!GetOwner()->IsLocal())
                return;

            if (TestStuck())
                return;

            m_FireSetupMod = 0;

            if (m_iDelay > 0) {
                m_iDelay -= theGame.GetOpersElapsed();
                return;
            }

            // if a moving vehicle is blocking us we wait
            CVehicle *pVehNext = theVehicleHex._GetVehicle(m_ptNext);
            CVehicle *pVehHead = theVehicleHex._GetVehicle(m_ptHead);
            CVehicle *pVehTail = theVehicleHex._GetVehicle(m_ptTail);
            if (((pVehNext != NULL) && (pVehNext->m_cMode == moving)) ||
                ((pVehHead != NULL) && (pVehHead->m_cMode == moving)) ||
                ((pVehTail != NULL) && (pVehTail->m_cMode == moving))) {
                m_dwTimeBlocked = 0;
                return;
            }

#ifdef _LOGOUT
            logPrintf(LOG_PRI_VERBOSE, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) trying to deploy", GetID(), m_ptHead.x,
                      m_ptHead.y);
#endif

            // we don't do anything 1 time out of 4 to avoid deadlocks
            if ((MyRand() & 0x1100) == 0x1100)
                return;

            // if it's been over 30 seconds we tell the AI
            if ((m_dwTimeBlocked > 24 * 30) && (!(m_bFlags & told_ai_stop))) {
                CheckAroundBuilding();
#ifdef _LOGOUT
                logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE,
                          "!! backup method - AI told vehicle %d at sub (%d,%d) cant_deploy", GetID(), m_ptHead.x,
                          m_ptHead.y);
#endif
                _SetRouteMode(stop);
                g_pszPostWhy = "cantdeploy30s";   // [NONOTIFY]/[ARRIVEMISS] caller tag
                PostArrivedOrBlocked();
                g_pszPostWhy = NULL;
            }

            // time we've been blocked
            m_dwTimeBlocked += theGame.GetFramesElapsed();

            // if the hexes are taken make sure it's legit
            if (pVehHead != NULL)
                theVehicleHex.CheckHex(m_ptHead);
            else if (pVehTail != NULL)
                theVehicleHex.CheckHex(m_ptTail);
            else {
                // make sure we can stick our nose out
                BOOL bCanEnter = TRUE;
                if (!CanEnter(m_ptNext, FALSE)) {
                    bCanEnter = FALSE;
                    int iDir, iMax;
                    if (GetData()->GetVehFlags() & CTransportData::FL1hex) {
                        iDir = -4;
                        iMax = 3;
                    } else {
                        iDir = -2;
                        iMax = 2;
                    }
                    for (; iDir <= iMax; iDir++) {
                        CSubHex _next;
                        if (GetData()->GetVehFlags() & CTransportData::FL1hex)
                            _next = ::Rotate(iDir, m_ptHead, m_ptHead);
                        else
                            _next = Rotate(iDir);
                        if ((!theBuildingHex._GetBuilding(_next)) && (CanEnter(_next, FALSE))) {
                            m_pszSelWhy = "deploy_nose";   // [STEP] selector tag
                            m_ptNext = _next;
                            bCanEnter = TRUE;
                            break;
                        }
                    }
                }

                // deploy if available
                if (!bCanEnter)
                    theVehicleHex.CheckHex(m_ptNext);
                else {
                    TakeOwnership();
                    ASSERT_VALID_LOC (this);
                    _SetRouteMode(deploy_it);
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
            return;
        }

        case deploy_it :
            if (!GetOwner()->IsLocal())
                return;

            m_FireSetupMod = 0;

            MaterialChange();

            ASSERT (theGame.IsNetGame() || GetOwner()->IsLocal());  // MP: remote units (Task#14)
            ASSERT (m_cOwn);
            ASSERT_VALID_LOC (this);
#ifdef _LOGOUT
            logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) deploying", GetID(), m_ptHead.x,
                      m_ptHead.y);
#endif

            // if we are there we don't need to move (from place_veh)
            if (m_hexDest.SameHex(m_ptHead)) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_USEFUL, LOG_VEH_MOVE, "vehicle %d at sub (%d,%d) deployed at dest", GetID(),
                          m_ptHead.x, m_ptHead.y);
#endif
                _SetRouteMode(stop);
                if (theBuildingHex._GetBuilding(m_ptHead) != NULL)
                    EnterBuilding();

                // tell the AI. [ARRIVEMISS]: this caller decided "at dest" with the HEX
                // test above, PostArrivedOrBlocked decides with a SUB test - so on a
                // different sub-hex of the same hex it takes the BLOCKED branch.
                g_pszPostWhy = "deployed";   // [NONOTIFY]/[ARRIVEMISS] caller tag
                PostArrivedOrBlocked();
                g_pszPostWhy = NULL;
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
            if (HavePathOrNext()) {
                _SetRouteMode(moving);
                StartTravel(FALSE);
                ASSERT (m_cOwn);
            } else

                // no path yet - this will call GetPath
            {
                _SetRouteMode(stop);
                DeletePath();
                SetDest(m_hexDest);
                ASSERT (m_cMode != stop);
            }

            // if its blocked go back to can't deploy
            if ((m_cMode == blocked) && (theBuildingHex._GetBuilding(m_ptHead) != NULL)) {
#ifdef _LOGOUT
                logPrintf(LOG_PRI_CRITICAL, LOG_VEH_MOVE, "Vehicle %d at sub (%d,%d) failed deploy", GetID(),
                          m_ptHead.x, m_ptHead.y);
#endif
                ReleaseOwnership();
                _SetRouteMode(cant_deploy);
                m_dwTimeBlocked = 0;
                CheckAroundBuilding();
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
            switch (m_iEvent) {
                case build :
                    ConstructBuilding();
                    break;
                case build_road :
                    ConstructRoad();
                    break;

#ifdef _DEBUG
                    case load :
                        TRAP (); // BUGBUG - I don't think this ever happens
                        break;
#endif
                default :
                    // run mode with no build job (e.g. StopConstruction clears
                    // event+bldg but never the mode): same broken state, same
                    // resolution
                    SetEventAndRoute(none, stop);
                    break;
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

void CVehicle::BuildBldg() {

    m_iEvent = none;
    if (!GetOwner()->IsLocal())
        return;
    ASSERT_STRICT (m_ptHead.SameHex(m_ptTail));

    CMsgBuildBldg msg(this, m_hexBldg, m_iBuildDir, m_iBldgType);

    // make sure not on water or another city
    int iWhy;
    int iRtn = theMap.FoundationCost(m_hexBldg, m_iBldgType, m_iBuildDir, this, NULL, &iWhy);

    if (theApp.m_pLogFile != NULL) {
        char sBuf[80];
        sprintf(sBuf, "Request building %d at %d,%d = cost: %d, why: %d",
                m_iBldgType, m_hexBldg.X(), m_hexBldg.Y(), iRtn, iWhy);
        theApp.Log(sBuf);
    }

    msg.m_iWhy = (signed char) iWhy;
    if (iRtn < 0) {
        _SetRouteMode(stop);
        msg.ToErr();
        theGame.PostToClient(GetOwner(), &msg, sizeof(msg));
        return;
    }

    // send a message telling it to start building
    theGame.PostToServer(&msg, sizeof(msg));
}

void CVehicle::BuildRoad() {

    m_iEvent = none;
    if (!GetOwner()->IsLocal()) {
        ASSERT_STRICT (FALSE);
        return;
    }
    ASSERT_STRICT (m_ptHead.SameHex(m_ptTail));

    // if can't build here go on to the next
    CHex *pHex = theMap._GetHex(GetHexHead());
    if ((pHex->IsWater()) || (pHex->GetType() == CHex::road) ||
        (pHex->GetUnits() & (CHex::bridge | CHex::bldg)))
        if (!NextRoadHex())
            return;

    // if no water - just a road
    CHexCoord _hex(m_ptHead);
    CHexCoord _hexNext = _NextRoadHex(_hex);
    if (theMap._GetHex(_hexNext)->CanRoad()) {
        // send a message telling it to start building
        CMsgBuildRoad msg(this);
        theGame.PostToServer(&msg, sizeof(msg));
        return;
    }

    if (!GetOwner()->CanBridge()) {
        if (GetOwner()->IsMe())
            theGame.Event(EVENT_CANT_BRIDGE, EVENT_NOTIFY);
        return;
    }

    // done?
    if (m_hexEnd == _hex) {
        TRAP();
        return;
    }

    // we need to place a bridge
    //   what direction
    int x = CHexCoord::Diff(m_hexEnd.X() - _hex.X());
    int y = CHexCoord::Diff(m_hexEnd.Y() - _hex.Y());
    if (abs(x) >= abs(y)) {
        x = __minmax(-1, 1, x);
        y = 0;
    } else {
        x = 0;
        y = __minmax(-1, 1, y);
    }

    CHexCoord _hexEnd(_hexNext);
    int const iMaxSpan = GetOwner()->GetMaxSpan();   // bridge research tier
    int iLen = 0;
    // <= (not <): placement allows a span of exactly iMaxSpan, so the scan must
    // step across iMaxSpan water hexes to reach the far-bank land hex. Stopping at
    // iLen<iMaxSpan aborted every max-span bridge one hex short of the far bank
    // (short bridges, span<iMaxSpan, were unaffected). See bug #37.
    while (iLen <= iMaxSpan) {
        _hexEnd.X() += x;
        _hexEnd.Y() += y;
        _hexEnd.Wrap();

        // if we run into a bridge or building we're done
        CHex *pHexTest = theMap._GetHex(_hexEnd);
        if (pHexTest->GetUnits() & (CHex::bridge | CHex::bldg)) {
            iLen = iMaxSpan;
            break;
        }

        if (pHexTest->IsWater()) {
            iLen++;
            if (iLen > iMaxSpan)
                break;
        } else {
            iLen = 0;
            if (pHexTest->CanRoad())
                break;
        }
    }

    // if we failed - end it
    if (iLen >= iMaxSpan) {
#if EN_GAMEPLAY_PROBES
        {
            CHex *pStop = theMap._GetHex(_hexEnd);
            char szH[200];
            sprintf(szH, "[HALT-SITE A: client bridge-span scan] veh %lu head %d,%d runend %d,%d "
                         "scanstop %d,%d type %d units 0x%x iLen %d maxspan %d\n",
                    (unsigned long)GetID(), m_ptHead.x / 2, m_ptHead.y / 2,
                    m_hexEnd.X(), m_hexEnd.Y(), _hexEnd.X(), _hexEnd.Y(),
                    pStop ? (int)pStop->GetType() : -1,
                    pStop ? (unsigned)pStop->GetUnits() : 0u, iLen, iMaxSpan);
            EnBridgeDbgLog(szH);
        }
#endif
        if (GetOwner()->IsMe())
            theGame.Event(EVENT_ROAD_HALTED, EVENT_WARN, this);
        else {
            CMsgBuildRoad msg(this);
            msg.ToErr();
            theGame.PostToClient(GetOwner(), &msg, sizeof(msg));
        }
        return;
    }

    // let's build a bridge
    CMsgBuildBridge msg(this, _hexEnd);
    theGame.PostToServer(&msg, sizeof(msg));
}

// Effective cargo capacity = base (per-type data) scaled by the owner's
// cargo_handling research (+10% per level, CPlayer::GetCargoPct). Routed through
// here so auto/route AND hand-loaded trucks share the same teched limit, and the
// over-capacity TRAP (CUnit::GetTotalStore) checks the correct value.
int CVehicle::GetMaxMaterials() const {
    return ( GetData()->GetMaxMaterials() * GetOwner()->GetCargoPct() ) / 100;
}

// Effective unit-hold capacity = base (per-type data GetPeopleCarry) plus the owner's
// Landing Craft 2/3 research (+1 each), applied ONLY to a landing craft (a boat that
// carries units — IsBoat && IsCarrier; cargo ships are IsBoat && IsTransport). Routed
// through here so EVERY capacity gate (auto-load, the net load handler + its assert,
// and the UI) agrees on the teched limit.
int CVehicle::GetEffPeopleCarry() {
    int iBase = GetData()->GetPeopleCarry();
    if ( GetData()->IsBoat() && GetData()->IsCarrier() && GetOwner() )
        // Scale by MAX_CARGO: each tech = +1 unit slot (a vehicle costs MAX_CARGO). The raw
        // +1/+2 left partial room the "enter" click accepted but the load gate rejected.
        iBase += GetOwner()->GetLandingCraftBonus() * MAX_CARGO;
    return iBase;
}

void CVehicle::Load() {

    ASSERT_STRICT (GetOwner()->IsLocal());

    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
    if (pBldg == NULL) {
        ASSERT_STRICT (FALSE);
        return;
    }
    ASSERT_STRICT_VALID (pBldg);

    // see if full
    int iLeft = GetMaxMaterials() - GetTotalStore();
    if (iLeft <= 0)
        return;

    // if under construction we don't take anything
    if (pBldg->IsConstructing())
        return;

    int iCant[CMaterialTypes::num_types];
    pBldg->GetInputs(iCant);

    // take a look at where we are going to figure out what to load up
    // this is generally only a problem if loading at a warehouse
    // BUGS #99: this walk is a cycle over the route that stops when it returns to the
    // cursor it started from - with m_pos NULL the (pos == m_pos) test can never fire
    // (pos is re-seated to the head every time it runs off the tail), so a NULL cursor
    // spins forever unless an unload stop happens to be found. #99 makes a saved NULL
    // cursor load back as NULL, so take the same heal SetEvent(route) uses (unit.cpp,
    // case route: if (m_pos == NULL) m_pos = m_route.GetHeadPosition()) and treat the
    // head as the cursor for this walk. posStart == m_pos whenever m_pos is non-NULL,
    // so the loop below is unchanged in every pre-existing case.
    POSITION posStart = m_pos;
    if (posStart == NULL)
        posStart = m_route.GetHeadPosition();
    POSITION pos = posStart;
    CBuilding *pBldgDest = NULL;
    while (TRUE) {
        if (pos == NULL)
            pos = m_route.GetHeadPosition();
        else {
            m_route.GetNext(pos);
            if (pos == NULL)
                pos = m_route.GetHeadPosition();
        }
        if ((pos == posStart) || (pos == NULL))
            break;

        // only care about unloads
        if (m_route.GetAt(pos)->GetRouteType() == CRoute::unload) {
            pBldgDest = theBuildingHex._GetBuilding((m_route.GetAt(pos))->GetCoord());
            if (pBldgDest)
                break;
        }
    }

    // ok, we know where we are going
    if (pBldgDest != NULL) {
        if (pBldgDest->IsConstructing()) {
            int iNeed = 0;
            for (int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes(); iOn++)
                if (pBldgDest->GetStore(iOn) < pBldgDest->NeedToBuild(iOn))
                    iNeed += pBldgDest->GetBldgMatReq(iOn, TRUE);
            if (iNeed > 0) {
                float fMul = __min (1.0f, (float) (iLeft / iNeed));
                for (int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes(); iOn++)
                    if (pBldgDest->GetStore(iOn) < pBldgDest->NeedToBuild(iOn)) {
                        iNeed = (int) ((float) pBldgDest->GetBldgMatReq(iOn, TRUE) * fMul);
                        iNeed = __min (iNeed, pBldg->GetStore(iOn));
                        iNeed = __min (iNeed, pBldgDest->GetBldgMatReq(iOn, TRUE));
                        pBldg->AddToStore(iOn, -iNeed);
                        AddToStore(iOn, iNeed);
                        iLeft -= iNeed;
                    }

                // return if full
                if (iLeft <= 0) {
                    // its an event
                    ASSERT_STRICT (iLeft >= 0);
                    pBldg->EventOff();
                    pBldg->MaterialChange();
                    MaterialChange();
                    return;
                }
            }
        }

        // ok, load up with what the dest needs
        // determine what units the dest building uses that this building doesn't
        // the GetAccepts stuff is for warehouses
        int iUse[CMaterialTypes::num_types];
        pBldgDest->GetInputs(iUse);
        int iAcpts[CMaterialTypes::num_types];
        pBldgDest->GetAccepts(iAcpts);
        for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
            iUse[iOn] = __max (iUse[iOn], iAcpts[iOn]);
        for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
            if (iCant[iOn] > 0)
                iUse[iOn] = 0;

        // figure out how many output units the building can make
        float fMul[CMaterialTypes::num_types];
        float fBiggest = 0;
        for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
            if (iUse[iOn] <= 0)
                fMul[iOn] = 0;
            else if ((fMul[iOn] = (float) pBldgDest->GetStore(iOn) / (float) iUse[iOn]) > fBiggest)
                fBiggest = fMul[iOn];

        // if its empty then no point in doing the below
        if (fBiggest > 0.0) {
            // set fMul to what is needed to make them all even
            for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
                if (iUse[iOn] > 0)
                    fMul[iOn] = fBiggest - fMul[iOn];
            int iTotal = 0;
            for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
                if (fMul[iOn] > 0.0)
                    iTotal += (int) ((float) iUse[iOn] * fMul[iOn]);
            // reduce the multiplier if not enough room
            if (iTotal > iLeft) {
                float fReduce = (float) iLeft / (float) iTotal;
                for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
                    if (fMul[iOn] > 0.0)
                        fMul[iOn] *= fReduce;
            }
            // FINALLY - stock up to make everyone even
            for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
                if (fMul[iOn] > 0.0) {
                    int iNum = __min (iLeft, (int) ((float) iUse[iOn] * fMul[iOn]));
                    iNum = __min (iNum, pBldg->GetStore(iOn));
                    pBldg->AddToStore(iOn, -iNum);
                    AddToStore(iOn, iNum);
                    iLeft -= iNum;
                }

            // return if full
            if (iLeft <= 0) {
                // its an event
                ASSERT_STRICT (iLeft >= 0);
                pBldg->EventOff();
                pBldg->MaterialChange();
                MaterialChange();
                return;
            }
        }    // we are brought up even

        // ok, now fill the rest proportionally
        int iTotal = 0;
        for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
            iTotal += iUse[iOn];
        if (iTotal > 0)
            for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
                if (iUse[iOn] > 0) {
                    int iNeed = (iUse[iOn] * iLeft) / iTotal;
                    iTotal -= iUse[iOn];
                    iNeed = __min (iNeed, iLeft);
                    iNeed = __min (iNeed, pBldg->GetStore(iOn));
                    pBldg->AddToStore(iOn, -iNeed);
                    AddToStore(iOn, iNeed);
                    iLeft -= iNeed;
                }

        // return if full
        ASSERT_STRICT (iLeft >= 0);
        pBldg->EventOff();
        pBldg->MaterialChange();
        MaterialChange();
        return;
    }    // if pBldgDest

    // ok, if we don't know the dest, we go for equal amounts
    int iTypesHave = CMaterialTypes::GetNumTypes();
    for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
        if (iCant[iOn] > 0)
            iTypesHave--;

    int iTypesLeft = iTypesHave;
    for (int iTry = 0; (iTry < iTypesHave) && (iLeft > 0); iTry++) {
        if (iTypesLeft <= 0)
            break;
        int iNeed = __max (1, iLeft / iTypesLeft);
        iTypesLeft = iTypesHave;
        for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
            if (iCant[iOn] == 0) {
                if (pBldg->GetStore(iOn) == 0)
                    iTypesLeft--;
                else {
                    int iNum = __min (iNeed, pBldg->GetStore(iOn));
                    pBldg->AddToStore(iOn, -iNum);
                    AddToStore(iOn, iNum);
                    iLeft -= iNum;
                    if (iLeft <= 0)
                        break;
                }
            }
    }

    ASSERT_STRICT (iLeft >= 0);
    pBldg->EventOff();
    pBldg->MaterialChange();
    MaterialChange();
}

// give this building all it can hold of everything it uses that this truck has
//   note: if under construction this is a larger list.
void CVehicle::Unload() {

    ASSERT_STRICT (GetOwner()->IsLocal());

    CBuilding *pBldg = theBuildingHex._GetBuilding(m_ptHead);
    if (pBldg == NULL) {
        ASSERT_STRICT (FALSE);
        return;
    }
    ASSERT_STRICT_VALID (pBldg);

    // construction
    if (pBldg->IsConstructing()) {
        for (int iOn = 0; iOn < CMaterialTypes::GetNumBuildTypes(); iOn++) {
            int iNum = pBldg->NeedToBuild(iOn);
            AddToStore(iOn, -iNum);
            pBldg->AddToStore(iOn, iNum);
        }
        // we drop through for materials the building will need when done
    }

    // not construction
    int iAccepts[CMaterialTypes::num_types];
    pBldg->GetAccepts(iAccepts);
    for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
        if (iAccepts[iOn] >= 0) {
            pBldg->AddToStore(iOn, GetStore(iOn));
            SetStore(iOn, 0);
            if ((theGame.GetScenario() == 7) && (pBldg->GetStore(CMaterialTypes::copper) > 0))
                theGame.m_iScenarioVar++;
        }

    // its an event
    pBldg->EventOff();
    pBldg->MaterialChange();
    MaterialChange();
}

void CVehicle::ConstructBuilding() {

    ASSERT_STRICT_VALID (this);

    if (m_pBldg == NULL) {
        // run+build with no building = broken state (crane reports "working"
        // forever; every idle check skips it). Resolve where detected: go
        // genuinely idle so the normal idle->job machinery recovers it.
        SetEventAndRoute(none, stop);
        return;
    }
    ASSERT_STRICT_VALID (m_pBldg);

    // get change based on everything
    int iInc = GetProd(GetOwner()->GetConstProd());
    // Edicts v1: "Fortify Border" (civ-wide) speeds construction of FORTS specifically.
    if (GetOwner()->GetEdictFortBuildMult() != 1.0f
        && m_pBldg->GetData()->GetBldgType() == CStructureData::fort)
        iInc = (int)(iInc * GetOwner()->GetEdictFortBuildMult());

#if EN_AI_PROBES_ECON && defined(_WIN32)
    // welded-crane instrument: is the work tick producing anything at an
    // abandoned host, and from what inputs?
    if (GetOwner()->IsAI() && m_pBldg->IsFlag(CUnit::abandoned)) {
        static DWORD s_dwNextCwLog = 0;
        if (theGame.GettimeGetTime() >= s_dwNextCwLog) {
            s_dwNextCwLog = theGame.GettimeGetTime() + 5000;
            char szCw[144];
            sprintf(szCw, "[CONSTABAND] crane %lu host %lu iInc %d dampf %.3f constprod %.3f opers %ld\n",
                    (unsigned long)GetID(), (unsigned long)m_pBldg->GetID(), iInc,
                    m_fDamPerfMult, GetOwner()->GetConstProd(), (long)theGame.GetOpersElapsed());
            OutputDebugStringA(szCw);
        }
    }
#endif

    if (iInc <= 0)
        return;

    m_pBldg->AddConstDone(iInc);

    // update the status
    MaterialChange();
}

void CVehicle::ConstructRoad() {

    ASSERT_STRICT_VALID (this);

    int iInc = GetProd(GetOwner()->GetConstProd());
    if (iInc <= 0)
        return;

    CHexCoord _hexHead(GetHexHead());
    CHex *pHex = theMap._GetHex(_hexHead);

    // if we are starting see if we can find gas to build with
    if ((m_iBuildDone <= 0) && (GetOwner()->IsLocal())) {
        if (!GetOwner()->BuildRoad()) {
#if EN_GAMEPLAY_PROBES
            {
                char szG[160];
                sprintf(szG, "[HALT-SITE B: ConstructRoad gas check] veh %lu at %d,%d gas %d need %d\n",
                        (unsigned long)GetID(), _hexHead.X(), _hexHead.Y(),
                        GetOwner()->GetGasHave(), GAS_PER_ROAD);
                EnBridgeDbgLog(szG);
            }
#endif
            if (GetOwner()->IsMe())
                theGame.Event(EVENT_ROAD_HALTED, EVENT_WARN, this);
            else {
                CMsgBuildRoad msg(this);
                msg.ToErr();
                theGame.PostToClient(GetOwner(), &msg, sizeof(msg));
            }
            SetEventAndRoute(none, stop);
            return;
        }

        // set to under construction
        pHex->ChangeToRoad(_hexHead, FALSE, GetOwner()->IsMe());

        if (pHex->GetVisibility() || GetOwner()->IsMe()) {
            pHex->m_psprite = theTerrain.GetSprite(CHex::road, CHex::r_path);
            _hexHead.SetInvalidated();
        }
    }

    m_iBuildDone += iInc;

    // see if we are done
    int iTotal;
    CBridge *pBridge = NULL;
    if (pHex->GetUnits() & CHex::bridge) {
        CBridgeUnit *pBu = theBridgeHex.GetBridge(_hexHead);
        if (pBu != NULL)
            pBridge = pBu->GetParent();
    }
    if (pBridge != NULL)
        iTotal = pBridge->GetConstTotal();
    else
        iTotal = theTerrain.GetData(pHex->GetType()).GetBuildMult() * CTerrainData::GetBuildRoadTime();

    if (m_iBuildDone >= iTotal) {
        // if its not ours we need to wait for a message saying its done
        if (!GetOwner()->IsLocal()) {
            m_iBuildDone = iTotal - 1;
            if (m_iLastPer < 99)
                m_iLastPer = 99;
            _SetEventAndRoute(none, stop);
            return;
        }

        m_iLastPer = 100;
        m_iBuildDone = -1;

        // invalidate
        // 		mark the bridge as completed
        if (pBridge != NULL)
            pBridge->BridgeBuilt();

        // tell everyone
        CMsgRoadDone msg(this);
        theGame.PostToAll(&msg, sizeof(msg));

        // update the status
        MaterialChange();

        // do the next hex
        NextRoadHex();
        return;
    }

    int iPer = (m_iBuildDone * 100) / iTotal;
    if (iPer == m_iLastPer)
        return;
    m_iLastPer = iPer;

    // set the bridges
    if (pBridge != NULL)
        pBridge->_SetPer(iPer);

    // update the status
    MaterialChange();
}

CHexCoord CVehicle::_NextRoadHex(CHexCoord const &_hexOn) {

    // if we're there - return it
    if (_hexOn == m_hexEnd)
        return (_hexOn);

    // nope, go to the next hex
    // move closer on the longest one (so we go diaganol)
    int x = CHexCoord::Diff(m_hexEnd.X() - _hexOn.X());
    int y = CHexCoord::Diff(m_hexEnd.Y() - _hexOn.Y());
    CHexCoord hex(_hexOn);
    if (abs(x) >= abs(y)) {
        if (x > 0)
            hex.Xinc();
        else
            hex.Xdec();
    } else {
        if (y > 0)
            hex.Yinc();
        else
            hex.Ydec();
    }

    return (hex);
}

BOOL CVehicle::NextRoadHex() {

    // nope, go to the next hex
    CHexCoord _hex(m_ptHead);

    // a bridge crossing can jump PAST m_hexEnd and then bounce between the
    // bridge ends forever (an end hex covered by a bridge never == _hex) -
    // bound the walk; no legit walk exceeds the map size
    CSize sizeMap = theMap.GetSize();
    int iStepsLeft = sizeMap.cx + sizeMap.cy;

    // skip by buildings
    while (TRUE) {
        // if this was the last, shut us down
        if (m_ptHead.SameHex(m_hexEnd) || (--iStepsLeft < 0)) {
            _SetEventAndRoute(none, stop);
            theGame.Event(EVENT_ROAD_DONE, EVENT_NOTIFY, this);
            return (FALSE);
        }

        // if we're on a bridge - cross it AND OFF. GOAL-DIRECTED: cross toward
        // whichever landing is nearer the run's destination, and if we already
        // stand on that side, don't cross at all. (v1 crossed blindly to the
        // END landing and redirected off self - a crane commuted back and
        // forth over a finished span forever; operator eyes-on 07-13 21:30.)
        CBridgeUnit *pBu = theBridgeHex.GetBridge(_hex);
        if (pBu != NULL) {
            CBridge *pBr = pBu->GetParent();
            CHexCoord landEnd = pBr->GetHexEnd();
            int iExitDir = pBr->GetUnitEnd()->GetExit();
            landEnd.X() += (iExitDir & 1) ? 2 - iExitDir : 0;
            landEnd.Y() += (!(iExitDir & 1)) ? iExitDir - 1 : 0;
            landEnd.Wrap();
            CHexCoord landStart = pBr->GetHexStart();
            iExitDir = pBr->GetUnitStart()->GetExit();
            landStart.X() += (iExitDir & 1) ? 2 - iExitDir : 0;
            landStart.Y() += (!(iExitDir & 1)) ? iExitDir - 1 : 0;
            landStart.Wrap();

            CHexCoord landTo = (CHexCoord::Dist(landEnd, m_hexEnd) <= CHexCoord::Dist(landStart, m_hexEnd))
                                   ? landEnd : landStart;
            // the 1996 jump fabricates the landing with no land check - a
            // water landTo flowed into SetDestAndMode as an undrivable dest.
            // Try the other landing; both wet = the run is done (designed exit)
            if (theMap._GetHex(landTo)->IsWater()) {
                CHexCoord landOther = (landTo == landEnd) ? landStart : landEnd;
                if (!theMap._GetHex(landOther)->IsWater())
                    landTo = landOther;
                else {
                    _SetEventAndRoute(none, stop);
                    theGame.Event(EVENT_ROAD_DONE, EVENT_NOTIFY, this);
                    return (FALSE);
                }
            }
#if EN_AI_PROBES_ECON && defined(_WIN32)
            { char szJ[128]; sprintf(szJ, "[BRIDGEPONG] veh %lu at %d,%d jump-> %d,%d (runend %d,%d)\n",
                    (unsigned long)GetID(), m_ptHead.x / 2, m_ptHead.y / 2, landTo.X(), landTo.Y(),
                    m_hexEnd.X(), m_hexEnd.Y()); OutputDebugStringA(szJ); }
#endif
            if (m_ptHead.SameHex(landTo))
                // already on the destination side: never re-cross; take the
                // plain geometric step instead (bounded walk handles the rest)
                _hex = _NextRoadHex(_hex);
            else
                _hex = landTo;
        } else
            _hex = _NextRoadHex(_hex);

        // done?
        if (_hex == m_hexEnd)
            break;

        // skip around buildings & bridges
        if (!(theMap._GetHex(_hex)->GetUnits() & (CHex::bldg | CHex::bridge)))
            break;
    }

#if EN_AI_PROBES_ECON && defined(_WIN32)
    // ROADLOOP probe: a next-hex resolving to the crane's own hex re-runs
    // ConstructRoad on the same spot forever; a water resolve is undrivable
    {
        CHex *pHexN = theMap._GetHex(_hex);
        BOOL bSelf = m_ptHead.SameHex(_hex);
        BOOL bWater = (pHexN != NULL) && pHexN->IsWater();
        if (bSelf || bWater) {
            CBridgeUnit *pBuL = theBridgeHex.GetBridge(_hex);
            char szL[176];
            sprintf(szL, "[ROADLOOP] veh %lu head %d,%d next %d,%d end %d,%d self %d water %d nextbridge %d exit %d\n",
                    (unsigned long)GetID(), m_ptHead.x / 2, m_ptHead.y / 2, _hex.X(), _hex.Y(),
                    m_hexEnd.X(), m_hexEnd.Y(), (int)bSelf, (int)bWater, (int)(pBuL != NULL),
                    pBuL ? (pBuL->GetParent())->GetUnitEnd()->GetExit() : -1);
            OutputDebugStringA(szL);
        }
    }
#endif
    SetDestAndMode(_hex, center);
    return (TRUE);
}

// can't shoot at this guy anymore
void CVehicle::StopShooting(CUnit *pNewTarget) {

    // if any vehicles are shooting at us - have them hit our owner
    POSITION pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        ASSERT_STRICT_VALID (pVeh);
        if (pVeh->GetTarget() == this) {
            pVeh->_SetTarget(pNewTarget);
            if (pVeh->GetOwner()->IsAI()) {
                CMsgOutOfLos msg(pVeh, this);
                theGame.PostToClient(pVeh->GetOwner(), &msg, sizeof(msg));
            }
        }
    }

    // same for buildings
    pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        ASSERT_STRICT_VALID (pBldg);
        if (pBldg->GetTarget() == this) {
            pBldg->_SetTarget(pNewTarget);
            if (pBldg->GetOwner()->IsAI()) {
                CMsgOutOfLos msg(pBldg, this);
                // BUGS #104: the out-of-LOS message names the BUILDING as the attacker (ai.cpp gates delivery on attacker-owner agreement),
                // so it must go to the building's owner, as the vehicle branch above does; it was posted to the victim's owner and always rejected.
                theGame.PostToClient(pBldg->GetOwner(), &msg, sizeof(msg));
            }
        }
    }

}

BOOL CVehicle::CanShootAt(CUnit *pTarget) {

    if (pTarget == NULL) {
        TRAP();
        return (FALSE);
    }

    int iDir;
    if (GetTurret() != NULL) {
        TRAP();
        iDir = GetTurret()->m_iDir;
    } else
        iDir = m_iDir;

    int iTargetDir = FastATan(CMapLoc::Diff(pTarget->GetWorldPixels().x - m_maploc.x),
                              CMapLoc::Diff(pTarget->GetWorldPixels().y - m_maploc.y));

    int iSwing;
    // these units can shoot in a 45 degree angle off their front
    if (GetData()->GetVehFlags() & CTransportData::FLshoot180)
        iSwing = FULL_ROT / 8;
    else
        iSwing = FULL_ROT / 32;

    int iRange = (((iTargetDir - iDir) + 64) & 127) - 64;
    // we must be within 45 degrees (stop next to problems with below)
    if (abs(iRange) > FULL_ROT / 8)
        return (FALSE);

    iRange = __minmax(-iSwing, iSwing, iRange);
    iDir += iRange;
    iDir = __roll(0, FULL_ROT, iDir);

    // why waste time below?
    if (iDir == iTargetDir)
        return (TRUE);

    // find dist to target
    int xDif = abs(CMapLoc::Diff(pTarget->GetWorldPixels().x - m_maploc.x));
    int yDif = abs(CMapLoc::Diff(pTarget->GetWorldPixels().y - m_maploc.y));
    int iDif = __max (xDif, yDif);
    int xDir;
    if (iDir < 32)
        xDir = 64 - iDir;
    else if (iDir > 96)
        xDir = 192 - iDir;
    else
        xDir = iDir;
    int yDir = iDir <= 64 ? iDir : 128 - iDir;

    // find hit maploc
    CMapLoc ml;
    ml.x = m_maploc.x + (iDif * (64 - xDir)) / 32;
    ml.y = m_maploc.y - (iDif * (32 - yDir)) / 32;
    ml.Wrap();

    // how far away from target - iDif is measure of how far from target
    //   so a target further away must be a smaller angle off
    int iShift = 0;
    iDif >>= 2;
    while (iDif) {
        iDif >>= 1;
        iShift++;
    }
    xDif = abs(CMapLoc::Diff(pTarget->GetWorldPixels().x - ml.x)) >> iShift;
    yDif = abs(CMapLoc::Diff(pTarget->GetWorldPixels().y - ml.y)) >> iShift;
    iDif = __max (xDif, yDif);

    // see if it's us
    if (pTarget->GetUnitType() == CUnit::building) {
        // if within the furthest possible square...
        CBuilding *pBldg = (CBuilding *) pTarget;
        if (iDif / 2 <= __max (pTarget->GetData()->GetCX(), pTarget->GetData()->GetCY()))
            return (TRUE);
        return (FALSE);
    }

    // vehicle
    CVehicle *pVeh = (CVehicle *) pTarget;
    if (GetData()->GetVehFlags() & CTransportData::FL1hex)
        return (iDif <= 1);

    return (iDif <= 2);
}

void CVehicle::ShowLoadDialog() {

    if (!GetData()->IsTransport()) {
        TRAP();
        return;
    }

    // Non-modal so the game (and, in MP, the network loop) keeps running while it's
    // open — DoModal froze the simulation. Owned by this vehicle; closed on death via
    // DestroyAllWindows; onDone clears the pointer and GameWindow deletes the object.
    if (!theApp.m_gameWindow)
        return;
    if (!m_pSdlLoad) {
        m_pSdlLoad = new SDL2LoadTruckDialog(theApp.m_gameWindow.get(), this);
        m_pSdlLoad->ShowNonModal([this](int) { m_pSdlLoad = nullptr; });
    } else {
        m_pSdlLoad->RaiseAndAlert();   // already open → bring it forward
    }
}

void CVehicle::SetBridgeHex(CHexCoord const &hexStart, CHexCoord const &hexEnd, DWORD dwID, int iAlt) {

    // put bridge buildings there
    CBridge::Create(hexStart, hexEnd, dwID, iAlt);

    // set the vehicle to bridge building
    ASSERT (hexStart.SameHex(m_ptHead));
    SetEvent(build_road);
}

void CVehicle::SetTransport(CVehicle *pVehCarrier) {

    // if we're on another vehicle, don't do it
    if (m_pTransport != NULL)
        return;

    // can't load on ourself
    if (pVehCarrier == this) {
        TRAP();
        return;
    }

    // do we have room
    int iSize;
    if (GetData()->IsPeople())
        iSize = 1;
    else
        iSize = MAX_CARGO;
    if (pVehCarrier->m_iCargoSize + iSize > pVehCarrier->GetEffPeopleCarry()) {
        TRAP();
        return;
    }

    _SetEventAndRoute(CVehicle::none, CVehicle::stop);
    m_pTransport = pVehCarrier;

    m_hexDest = m_ptDest = m_ptHead = pVehCarrier->m_ptHead;
    if (GetData()->GetVehFlags() & CTransportData::FL1hex)
        m_ptTail = m_ptHead;
    else
        m_ptTail = pVehCarrier->m_ptTail;
    SetLoc(TRUE);
    ZeroMoveParams();
    DeletePath();

    pVehCarrier->m_lstCargo.AddTail(this);
    pVehCarrier->m_iCargoSize += iSize;

    m_pVehLoadOn = NULL;
}

void CVehicle::DestroyAllWindows() {

    DestroyRouteWindow();
    DestroyBuildWindow();

    // Close the non-modal load-cargo dialog if open, so it can't outlive this
    // vehicle (EndDialog destroys its window + triggers GameWindow cleanup; the
    // onDone lambda nulls m_pSdlLoad).
    if (m_pSdlLoad)
        m_pSdlLoad->EndDialog(0);
}

// dump the contents of this truck in the nearest warehouse and give to auto router
void CVehicle::DumpContents() {

    // deselect it
    CWndArea *pWndArea = theAreaList.GetTop();
    if (pWndArea)
        pWndArea->SubSelectUnit(this);

    // if no materials - turn it over
    if (GetTotalStore() == 0) {
        m_bFlags &= ~(hp_controls | dump_contents);
        theGame.m_pHpRtr->MsgGiveVeh(this);
        theGame.m_pHpRtr->MsgArrived(this);
        return;
    }

    // if we are in any building - dump here
    CBuilding *pBldg = theBuildingHex.GetBuilding(m_ptHead);
    if (pBldg != NULL) {
        // unload it
        for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++) {
            pBldg->AddToStore(iOn, GetStore(iOn));
            SetStore(iOn, 0);
            if ((theGame.GetScenario() == 7) && (pBldg->GetStore(CMaterialTypes::copper) > 0))
                theGame.m_iScenarioVar++;
        }

        pBldg->EventOff();
        m_bFlags &= ~(hp_controls | dump_contents);
        theGame.m_pHpRtr->MsgGiveVeh(this);
        theGame.m_pHpRtr->MsgArrived(this);
        return;
    }

    // find the closest warehouse
    int iDist = 200000;
    CBuilding *pDest = NULL;
    POSITION pos = theBuildingMap.GetStartPosition();
    CHexCoord _hexTruck(m_ptHead);
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        ASSERT_STRICT_VALID (pBldg);
        if ((pBldg->GetOwner()->IsMe()) && ((pBldg->GetData()->GetType() == CStructureData::rocket) ||
                                            (pBldg->GetData()->GetType() == CStructureData::warehouse))) {
            int _iDist = CHexCoord::Dist(_hexTruck, pBldg->GetHex());
            if ((pDest == NULL) || (_iDist < iDist)) {
                iDist = _iDist;
                pDest = pBldg;
            }
        }
    }

    // if didn't find - hand it off. 1996 TRAP assumed "me" always owns a
    // rocket/warehouse; an OBSERVER player (never landed) owns nothing, so a
    // gifted loaded truck hits this legitimately - recovery is right below
    if (pDest == NULL) {
        EN_TRAP_REMOVED("DumpContents: me owns no rocket/warehouse (observer) - handing veh to router");
        m_bFlags &= ~(hp_controls | dump_contents);
        theGame.m_pHpRtr->MsgGiveVeh(this);
        theGame.m_pHpRtr->MsgArrived(this);
        return;
    }

    // send it there
    m_bFlags |= dump_contents;
    SetDest(pDest->GetHex());
}

void CVehicle::AddSubOwned(int x, int y) {

    TRAP(!m_cOwn);
    TRAP((theVehicleHex.GetVehicle(x, y) != NULL) && (theVehicleHex.GetVehicle(x, y) != this));

    // already have it?
    for (int iInd = 0; iInd < NUM_SUBS_OWNED; iInd++)
        if ((SubsOwned[iInd].x == x) && (SubsOwned[iInd].y == y))
            return;

    // find a sub to use
    for (int iInd = 0; iInd < NUM_SUBS_OWNED; iInd++)
        if (SubsOwned[iInd].x == -1) {
            SubsOwned[iInd].x = x;
            SubsOwned[iInd].y = y;
            return;
        }

    // any dups
    // WAS TRAP() — fired 99,424x in ONE 15-player Full-Military session
    // (spawn-clump churn: blocked units re-claiming hexes every veh_goto until
    // the clump disperses). The condition is fully HANDLED by the dup-scan and
    // slot-steal recovery below; as a TRAP it (a) instantly killed any
    // undebugged _DEBUG run (the operator's repeated "crashes") and (b) cost
    // ~100k debugger round-trips per start under dbgcatch (minutes of fake lag).
    EN_TRAP_REMOVED( "AddSubOwned: claim table full (spawn-clump churn) - recovered below" );
    for (int iInd = 0; iInd < NUM_SUBS_OWNED - 1; iInd++)
        for (int iChk = iInd + 1; iChk < NUM_SUBS_OWNED; iChk++)
            if (SubsOwned[iInd] == SubsOwned[iChk]) {
                EN_TRAP_REMOVED( "AddSubOwned: duplicate claim slot - reusing it" );
                SubsOwned[iInd].x = x;
                SubsOwned[iInd].y = y;
                return;
            }

    // oh-oh no free subs - we now see who should be free
    for (int iInd = 0; iInd < NUM_SUBS_OWNED; iInd++)
        if ((SubsOwned[iInd] != m_ptNext) && (SubsOwned[iInd] != m_ptHead) && (SubsOwned[iInd] != m_ptTail)) {
            // if we still own it - free it
            if (theVehicleHex.GetVehicle(SubsOwned[iInd]) == this)
                theVehicleHex.ReleaseHex(SubsOwned[iInd], this);
            SubsOwned[iInd].x = x;
            SubsOwned[iInd].y = y;
            return;
        }

    // should be impossible
    TRAP();
}

void CVehicle::RemoveSubOwned(int x, int y) {

#ifdef _TRAP
    TRAP ( ! m_cOwn );
    TRAP ( theVehicleHex.GetVehicle (x,y) != this );
    BOOL bOk = FALSE;
#endif

    // remove from the list
    for (int iInd = 0; iInd < NUM_SUBS_OWNED; iInd++)
        if ((SubsOwned[iInd].x == x) && (SubsOwned[iInd].y == y)) {
            SubsOwned[iInd].x = -1;

#ifdef _TRAP
            bOk = TRUE;
#endif
        }

    // shouldn't happen
#ifdef _TRAP
    TRAP ( ! bOk );
#endif
}
