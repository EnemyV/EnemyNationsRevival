// order_model.h -- a self-contained MIRROR of the #38 order-queue logic.
//
// The production code cannot be linked standalone (MFC, the whole unit tree, a live
// CGame), so the two pieces worth testing are reproduced here and the tests assert on
// THIS: (1) the save-stream arithmetic of CRoute::Serialize and the route block of
// CVehicle::Serialize (new_unit.cpp), and (2) the order dispatcher state machine in
// vehicle.cpp (AddOrder / ClearOrders / NextOrder / OrderComplete / OrderFailed /
// OrderEnded and the idle poll in Operate's `stop` branch).
//
// A mirror can drift from what it mirrors, so test_order_serialize.cpp also LINTS the
// production sources for the lines this file claims they contain. Read the two
// together: the lint says the gate is still there, the model says the gate is right.
//
// Deliberately NOT modelled: CHexCoord's real on-disk width. It is a stand-in of two
// 16-bit fields. Every claim below is about what the gate ADDS or OMITS around it, so
// the stand-in is sound for that and proves nothing about CHexCoord itself.

#ifndef ORDER_MODEL_H
#define ORDER_MODEL_H

#include <vector>
#include <cstddef>

namespace orders {

// ---------------------------------------------------------------------------
// CRoute (vehicle.h)
// ---------------------------------------------------------------------------
enum { waypoint, unload, load, build, build_road, repair };

inline bool IsOrder(int iType) { return iType >= build; }   // CRoute::IsOrder

struct Hex {
    short x, y;
    Hex(): x(0), y(0) {}
    Hex(int _x, int _y): x((short)_x), y((short)_y) {}
    bool operator==(Hex const &o) const { return (x == o.x) && (y == o.y); }
    bool operator!=(Hex const &o) const { return !(*this == o); }
};

struct Route {
    Hex           hex;
    unsigned char type;
    unsigned char bldgType;     // order payload
    unsigned char dir;          // order payload
    Hex           hexEnd;       // order payload (build_road's far end)

    Route(): type(waypoint), bldgType(0), dir(0) {}
    Route(Hex h, int t, int bt = 0, int d = 0)
        : hex(h), type((unsigned char)t), bldgType((unsigned char)bt),
          dir((unsigned char)d), hexEnd(h) {}
};

// ---------------------------------------------------------------------------
// A byte stream standing in for CArchive.
// ---------------------------------------------------------------------------
struct Ar {
    std::vector<unsigned char> buf;
    std::size_t                rd;

    Ar(): rd(0) {}
    void     put8(unsigned v) { buf.push_back((unsigned char)(v & 0xff)); }
    void     put16(unsigned v) { put8(v); put8(v >> 8); }
    void     putHex(Hex h) { put16((unsigned short)h.x); put16((unsigned short)h.y); }
    unsigned get8() { return (rd < buf.size()) ? buf[rd++] : 0u; }
    unsigned get16() { unsigned a = get8(); return a | (get8() << 8); }
    Hex      getHex() { unsigned a = get16(), b = get16(); return Hex((short)a, (short)b); }
    bool     Drained() const { return rd == buf.size(); }
};

// ---------------------------------------------------------------------------
// CRoute::Serialize -- the SHIPPED (pre-#38) writer, kept so the tests can prove a
// v7 stream is byte-for-byte what it was.
// ---------------------------------------------------------------------------
inline void RouteStoreShipped(Ar &ar, Route const &r) {
    ar.putHex(r.hex);
    ar.put8(r.type);
}

// CRoute::Serialize -- current. The payload is gated on the save-format counter on
// BOTH sides, because VER_RELEASE is still 7: while it is, nothing extra is written.
inline void RouteStore(Ar &ar, Route const &r, unsigned ver) {
    ar.putHex(r.hex);
    ar.put8(r.type);
    if (ver >= 8) {
        ar.put8(r.bldgType);
        ar.put8(r.dir);
        ar.putHex(r.hexEnd);
    }
}

inline void RouteLoad(Ar &ar, Route &r, unsigned ver) {
    r.hex  = ar.getHex();
    r.type = (unsigned char)ar.get8();
    if (ver >= 8) {
        r.bldgType = (unsigned char)ar.get8();
        r.dir      = (unsigned char)ar.get8();
        r.hexEnd   = ar.getHex();
    } else {
        r.bldgType = r.dir = 0;
        r.hexEnd   = r.hex;
    }
}

// ---------------------------------------------------------------------------
// CVehicle::Serialize -- the route block only.
//
// cursor: index of m_pos; CUR_NULL = a NULL cursor, CUR_STALE = a non-NULL cursor
// that is in no node of this list (both are real shipped states).
// ---------------------------------------------------------------------------
enum { CUR_NULL = -1, CUR_STALE = -2 };

// BUGS #99 (integration): a NULL cursor now stores an out-of-range SENTINEL, and the
// loader honours it only on a counter >= 8 save. Counter <= 7 writers still stored N-1
// and counter <= 7 loaders still walk to it, so the shipped behaviour of every old save
// is unchanged. VER_RELEASE is 8 on this branch, so the sentinel is what a new save
// carries.
enum { ROUTE_POS_NONE = 0xffff };

struct VehRoute {
    std::vector<Route> route;
    int                cursor;
    bool               loop;

    VehRoute(): cursor(CUR_NULL), loop(true) {}   // ctor default: m_bRouteLoop = TRUE
};

inline void VehRouteStore(Ar &ar, VehRoute const &v, unsigned ver) {
    ar.put16((unsigned)v.route.size());

    int iPos = 0, iOn = 0;
    for (std::size_t i = 0; i < v.route.size(); ++i) {
        if ((int)i == v.cursor)     // BUGS #88 (747e5f9c): compare BEFORE advancing
            iPos = iOn;
        RouteStore(ar, v.route[i], ver);
        iOn++;
    }
    // BUGS #88 restored N-1 for a NULL cursor; BUGS #99 decided the policy question it
    // left open - a NULL cursor means NO cursor. The sentinel is written unconditionally
    // (this build always stamps counter 8) and read back only at counter >= 8.
    if (v.cursor == CUR_NULL)
        iPos = (int)ROUTE_POS_NONE;
    ar.put16((unsigned)iPos);

    if (ver >= 8)                              // BUGS #95: the one-shot/loop flag
        ar.put8(v.loop ? 1 : 0);
}

inline void VehRouteLoad(Ar &ar, VehRoute &v, unsigned ver) {
    unsigned n = ar.get16();
    v.route.clear();
    for (unsigned i = 0; i < n; ++i) {
        Route r;
        RouteLoad(ar, r, ver);
        v.route.push_back(r);
    }

    unsigned w = ar.get16();
    v.cursor   = CUR_NULL;
    bool bNone = (ver >= 8) && (w == (unsigned)ROUTE_POS_NONE);
    if ((!bNone) && (!v.route.empty()))
        v.cursor = (w < v.route.size()) ? (int)w : CUR_NULL;   // walk exhausted -> NULL

    if (ver >= 8)
        v.loop = (ar.get8() != 0);
    // pre-8: the flag is not in the stream at all, so the ctor default stands
}

// ---------------------------------------------------------------------------
// The dispatcher (vehicle.cpp).
// ---------------------------------------------------------------------------
enum { order_none, order_sent, order_work, order_road, order_done };

// m_iEvent values that matter here
enum { ev_none, ev_build, ev_build_road, ev_repair_bldg };
// m_cMode values that matter here. cant_deploy is the mode ExitBuilding leaves a crane
// in when it was welded INSIDE the building it just finished (m_cOwn is FALSE there), and
// the idle branch does not run in it - which is half of bug #114.
enum { md_stop, md_moving, md_run, md_cant_deploy };

// #114 stall watch tuning, mirrored from vehicle.cpp
enum { ORDER_STALL_MS = 3000, ORDER_STALL_TRIES = 3 };

struct Veh {
    std::vector<Route> route;
    int                cursor;       // index of m_pos, CUR_NULL for none
    bool               loop;
    int                state;        // m_iOrderState
    bool               local;        // GetOwner()->IsLocal()
    int                event;        // m_iEvent
    int                mode;         // m_cMode
    bool               site;         // m_pBldg != NULL
    Hex                orderHex;     // m_hexOrder
    int                orderKind;    // m_iOrderKind
    int                dispatches;   // how many requests went out (wire traffic)
    std::vector<Hex>   repairTargets;// hexes where RepairTargetLives() says yes

    // #114: the traffic recovery state the stall watch must not fire through, and the
    // watch's own two fields.
    bool               resume;       // m_bResume - a saved job a detour will drive back to
    int                holdFrames;   // m_iHoldFrames - a post-retreat hold is a live job
    unsigned           nowMs;        // theGame.GettimeGetTime()
    unsigned           orderStall;   // m_dwOrderStall
    int                orderRetry;   // m_iOrderRetry
    int                givenUp;      // orders the watch gave up on (EVENT_CONST_CANT)

    Veh()
        : cursor(CUR_NULL), loop(true), state(order_none), local(true),
          event(ev_none), mode(md_stop), site(false), orderKind(waypoint),
          dispatches(0), resume(false), holdFrames(0), nowMs(1000),
          orderStall(0), orderRetry(0), givenUp(0) {}

    Route *At(int i) { return ((i >= 0) && (i < (int)route.size())) ? &route[i] : 0; }

    // --- CVehicle::AddOrder ---
    void AddOrder(Hex hex, int iType, int iBldgType = 0, int iDir = 0, Hex const *pEnd = 0) {
        Route r(hex, iType, iBldgType, iDir);
        if (pEnd != 0)
            r.hexEnd = *pEnd;
        route.push_back(r);
        loop = false;                       // an order queue is ONE-SHOT, always
        if (cursor == CUR_NULL)
            cursor = 0;
    }

    // --- CVehicle::SetLocation, the MOVEMENT-append entry point ---
    // A movement route and an order queue never share one list: every movement append
    // drops the orders first.
    void AddMoveStop(Hex hex, int iType = waypoint) {
        if (!IsOrder(iType))
            ClearOrders();
        route.push_back(Route(hex, iType));
        if (cursor == CUR_NULL)
            cursor = 0;
    }

    // --- CVehicle::RepairTargetLives ---
    bool RepairTargetLives(Hex hex) const {
        for (std::size_t i = 0; i < repairTargets.size(); ++i)
            if (repairTargets[i] == hex)
                return true;
        return false;
    }

    // --- CVehicle::ClearOrders ---
    void ClearOrders() {
        bool               bCurGone = false;
        std::vector<Route> kept;
        for (std::size_t i = 0; i < route.size(); ++i) {
            if (IsOrder(route[i].type)) {
                if ((int)i == cursor)
                    bCurGone = true;
                continue;
            }
            if ((int)i == cursor)
                cursor = (int)kept.size();   // POSITION survives; the INDEX shifts
            kept.push_back(route[i]);
        }
        route.swap(kept);
        if (bCurGone)
            cursor = route.empty() ? CUR_NULL : 0;
        state      = order_none;
        orderRetry = 0;
        orderStall = 0;
    }

    // --- CVehicle::NextOrder ---
    bool NextOrder() {
        if (!local)
            return false;
        if ((state != order_none) || site || (event != ev_none) || (mode != md_stop))
            return false;

        int    pos = CUR_NULL;
        Route *pR  = 0;
        for (;;) {
            pos = (cursor != CUR_NULL) ? cursor : (route.empty() ? CUR_NULL : 0);
            if (pos == CUR_NULL)
                return false;
            pR = At(pos);
            if (pR == 0)
                return false;
            if (!IsOrder(pR->type))
                return false;

            // a repair order whose target is gone can never finish - drop it and take
            // the next order in this same call
            if ((pR->type == repair) && (!RepairTargetLives(pR->hex))) {
                route.erase(route.begin() + pos);
                cursor = route.empty() ? CUR_NULL : 0;
                continue;
            }
            break;
        }

        cursor    = pos;
        state     = order_sent;
        orderHex  = pR->hex;
        orderKind = pR->type;

        switch (pR->type) {
            case build:
                event = ev_build;
                mode  = md_moving;
                break;
            case build_road:
                event = ev_build_road;
                mode  = md_moving;
                break;
            case repair:
                event = ev_repair_bldg;
                mode  = md_moving;
                break;
            default:
                state = order_none;
                return false;
        }
        return true;
    }

    // --- CVehicle::OrderEnded ---
    void OrderEnded() {
        if (state != order_none)
            state = order_done;
    }

    // --- CVehicle::OrderComplete ---
    void OrderComplete() {
        state      = order_none;
        orderRetry = 0;             // #114: every order gets its own re-drive budget
        orderStall = 0;

        int pos = (cursor != CUR_NULL) ? cursor : (route.empty() ? CUR_NULL : 0);
        if (pos == CUR_NULL)
            return;
        Route *pR = At(pos);
        if (pR == 0)
            return;
        if (!IsOrder(orderKind) || (pR->type != orderKind) || (!(pR->hex == orderHex)))
            return;

        if (loop) {
            int next = pos + 1;
            cursor   = (next < (int)route.size()) ? next : (route.empty() ? CUR_NULL : 0);
        } else {
            route.erase(route.begin() + pos);
            cursor = route.empty() ? CUR_NULL : 0;
        }
    }

    // --- CVehicle::OrderFailed (called from ErrBuildBldg) ---
    void OrderFailed(Hex hex, int iBldgType) {
        bool bWasSent = (state == order_sent);
        state         = order_none;
        orderRetry    = 0;          // #114: as above
        orderStall    = 0;
        if (!bWasSent)
            return;

        int pos = (cursor != CUR_NULL) ? cursor : (route.empty() ? CUR_NULL : 0);
        if (pos == CUR_NULL)
            return;
        Route *pR = At(pos);
        if (pR == 0)
            return;
        if ((pR->type != build) || (!(pR->hex == hex)) || (pR->bldgType != iBldgType))
            return;

        route.erase(route.begin() + pos);
        cursor = route.empty() ? CUR_NULL : 0;
    }

    // --- CVehicle::CheckOrderStall (#114) ---
    // An order is dispatched by NextOrder and consumed by the vehicle ARRIVING, and
    // arriving is not guaranteed: FindNextHex's give-up exits stop a vehicle short of its
    // destination with _SetRouteMode(stop) + PostArrivedOrBlocked, never ArrivedDest. The
    // arming event is then never consumed and the state never leaves order_sent, so both
    // halves of NextOrder's busy test refuse for ever.
    void CheckOrderStall() {
        if (state != order_sent) {
            orderStall = 0;
            if (state == order_none)
                orderRetry = 0;
            return;
        }
        if (site) {                       // the job started after all
            orderStall = 0;
            return;
        }
        if (resume || (holdFrames > 0)) { // a traffic detour is a job STILL under way
            orderStall = 0;
            return;
        }

        int armed;
        switch (orderKind) {
            case build:      armed = ev_build; break;
            case build_road: armed = ev_build_road; break;
            case repair:     armed = ev_repair_bldg; break;
            default:         orderStall = 0; return;
        }
        // event cleared = the request is genuinely in flight to the server
        if (event != armed) {
            orderStall = 0;
            return;
        }

        if (orderStall == 0) {            // start the dwell
            orderStall = nowMs;
            return;
        }
        if (nowMs - orderStall < (unsigned)ORDER_STALL_MS)
            return;
        orderStall = 0;

        if (++orderRetry > ORDER_STALL_TRIES) {
            orderRetry = 0;
            event      = ev_none;
            mode       = md_stop;
            state      = order_done;      // OrderComplete, next line, consumes it
            givenUp++;
            return;
        }
        event = ev_none;                  // so NextOrder's busy test lets us back in
        mode  = md_stop;
        state = order_none;               // NextOrder, next line, re-dispatches it
    }

    // --- the COMPLETION half of the poll, hoisted OUT of `case stop` (#114) ---
    // A crane's job at a site ends when its own tie to the site goes. That is a fact about
    // the crane, not about its movement mode, so it must be read in every mode: a crane
    // that cannot step straight out of the building it just finished sits in cant_deploy,
    // where the idle branch never runs.
    void PollCompletion() {
        if (!local)
            return;
        if ((state == order_work) && (!site))
            state = order_done;
        if (state == order_done)
            OrderComplete();
    }

    // --- the idle poll in CVehicle::Operate, `case stop` ---
    // Returns true when it dispatched (production returns out of Operate there).
    bool PollIdle() {
        if (!local)
            return false;                 // the shipped `if (!IsLocal()) return;`
        CheckOrderStall();
        if (state == order_done)
            OrderComplete();
        if (state == order_none)
            return NextOrder();
        return false;
    }

    // one CVehicle::Operate call: the completion half runs before the mode switch, the
    // dispatch half only in `case stop`.
    bool Tick(unsigned msElapsed = 0) {
        nowMs += msElapsed;
        PollCompletion();
        if (mode == md_stop)
            return PollIdle();
        return false;
    }

    // ---- world events, so a test can drive a whole job ----

    // arrival at the site: ArrivedDest -> BuildBldg / BuildRoad clears the event and
    // sends the request
    void ArriveAndSend() {
        event = ev_none;
        mode  = md_stop;
        state = order_sent;
        dispatches++;
    }
    // the server accepted: BldgNew -> StartConst
    void ServerAccept() {
        site  = true;
        event = ev_build;
        mode  = md_run;
    }
    // a work tick: CVehicle::ConstructBuilding
    void WorkTick() { state = order_work; }
    // a road work tick: CVehicle::ConstructRoad
    void RoadTick() { state = order_road; }
    // the site let this vehicle go: StopConstruction (finished, destroyed, repaired)
    void SiteGone() {
        site  = false;
        event = ev_none;
        mode  = md_stop;
    }
    // the ordinary end of a BUILD: the crane was welded INSIDE the building, so
    // ExitBuilding finds m_cOwn FALSE and leaves it in cant_deploy, NOT stop.
    void SiteDoneWeldedInside() {
        site  = false;
        event = ev_none;
        mode  = md_cant_deploy;
    }
    // it found a free square and drove out
    void DeployOut() { mode = md_stop; }

    // FindNextHex / HandleBlocked give up SHORT of the destination: the vehicle is
    // stopped and notified, but ArrivedDest - the only thing that consumes the arming
    // event - never runs, so m_iEvent is left exactly as the dispatch set it.
    void GiveUpShortOfDest() { mode = md_stop; }

    // a traffic detour is under way: a hold armed and a saved job to come back to
    void TrafficHold(int frames) { resume = true; holdFrames = frames; mode = md_stop; }
    void HoldExpiresAndResumes() { resume = false; holdFrames = 0; mode = md_moving; }

    // the repair target dies while the crane is travelling: ArrivedDest's repair_bldg
    // case finds neither a building nor an unbuilt bridge and falls THROUGH, leaving
    // m_iEvent == repair_bldg on a stopped crane (the review's deferred residual)
    void ArriveAtDeadRepairTarget() { mode = md_stop; }
};

}   // namespace orders

#endif   // ORDER_MODEL_H
