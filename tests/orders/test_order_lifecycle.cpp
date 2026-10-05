// test_order_lifecycle.cpp -- the PRODUCTION order-queue bodies, with only the scene
// replaced. Built by run-order-lifecycle.py, which extracts the methods verbatim from
// enations_latest/src/vehicle.cpp into lifecycle_actual.inc (tests/traffic's technique).
//
// This is the suite that answers WinAstra finding 6: tests/orders/test_order_dispatch.cpp
// checks a hand-written mirror, and a mirror is green by construction for a defect that
// lives in the body it mirrors. Everything below drives the shipped code.
//
// The scene is a stand-in: a list, a hex, a route entry, a building, a game clock. It is
// NOT the engine - nothing here proves pathing, the wire, or a real save replay.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define EN_GAMEPLAY_PROBES 0          // the real gate the extracted bodies are written for

using BOOL  = int;
using BYTE  = unsigned char;
using DWORD = unsigned long;
constexpr BOOL TRUE = 1, FALSE = 0;

#define ASSERT(x)              ((void)0)
#define ASSERT_VALID(x)        ((void)0)
#define ASSERT_STRICT(x)       ((void)0)
#define ASSERT_STRICT_VALID(x) ((void)0)
static int g_traps = 0;
#define TRAP()                 (++g_traps)

enum { EVENT_CONST_CANT = 11, EVENT_WARN = 2, EVENT_ROAD_HALTED = 18, EVENT_GOTO_CANT = 23 };

// --------------------------------------------------------------------------- hex
struct CHexCoord {
    short xx, yy;
    CHexCoord(): xx(0), yy(0) {}
    CHexCoord(int a, int b): xx((short)a), yy((short)b) {}
    short  X() const { return xx; }
    short  Y() const { return yy; }
    short &X() { return xx; }
    short &Y() { return yy; }
    BOOL operator==(CHexCoord const &o) const { return (xx == o.xx) && (yy == o.yy); }
    BOOL operator!=(CHexCoord const &o) const { return !(*this == o); }
    void Wrap() {}                 // the scene map is not a torus
};

// --------------------------------------------------------------------------- route
class CRoute {
  public:
    enum { waypoint, unload, load, build, build_road, repair, move };
    CRoute(CHexCoord const &hex, int iType, int iBldgType = 0, int iDir = 0)
        : m_hex(hex), m_hexEnd(hex), m_iType((BYTE)iType),
          m_iBldgType((BYTE)iBldgType), m_iDir((BYTE)iDir) {}
    CHexCoord const &GetCoord() const { return m_hex; }
    int              GetRouteType() const { return m_iType; }
    int              GetBldgType() const { return m_iBldgType; }
    int              GetBldgDir() const { return m_iDir; }
    CHexCoord const &GetEndCoord() const { return m_hexEnd; }
    void             SetEndCoord(CHexCoord const &h) { m_hexEnd = h; }
    static BOOL      IsOrder(int iType) { return (iType >= build); }

  private:
    CHexCoord m_hex, m_hexEnd;
    BYTE      m_iType, m_iBldgType, m_iDir;
};

// ------------------------------------------------------- the CList<T,T> API used here
struct Node { CRoute *val; Node *prev, *next; };
using POSITION = Node *;

struct RouteList {
    Node *head = nullptr, *tail = nullptr;
    int   n    = 0;

    POSITION AddTail(CRoute *v) {
        Node *p = new Node{ v, tail, nullptr };
        (tail ? tail->next : head) = p;
        tail = p; ++n; return p;
    }
    POSITION AddHead(CRoute *v) {
        Node *p = new Node{ v, nullptr, head };
        (head ? head->prev : tail) = p;
        head = p; ++n; return p;
    }
    POSITION InsertAfter(POSITION at, CRoute *v) {
        if (at == nullptr) return AddHead(v);
        if (at == tail) return AddTail(v);
        Node *p = new Node{ v, at, at->next };
        at->next->prev = p;
        at->next = p; ++n; return p;
    }
    POSITION GetHeadPosition() const { return head; }
    POSITION GetTailPosition() const { return tail; }
    CRoute  *GetAt(POSITION p) const { return p->val; }
    CRoute  *GetNext(POSITION &p) const { CRoute *v = p->val; p = p->next; return v; }
    void     RemoveAt(POSITION p) {
        (p->prev ? p->prev->next : head) = p->next;
        (p->next ? p->next->prev : tail) = p->prev;
        delete p; --n;
    }
    int  GetCount() const { return n; }
    BOOL IsEmpty() const { return n == 0; }
};

// --------------------------------------------------------------------------- world
struct Owner {
    bool local = true, me = true;
    BOOL IsLocal() const { return local; }
    BOOL IsMe() const { return me; }
};

struct BldgData { int type = 0; int GetType() const { return type; } };

struct CBuilding {
    CHexCoord hex;
    BldgData  data;
    int cx = 1, cy = 1; // supplementary scene footprint, default preserves existing cases
    CHexCoord const &GetHex() const { return hex; }
    BldgData const  *GetData() const { return &data; }
    CHexCoord        GetExitHex() const { return hex; }   // SetLocation redirects onto these
    CHexCoord        GetShipHex() const { return hex; }
};

struct BuildingHex {
    std::vector<CBuilding *> live;
    CBuilding *_GetBuilding(CHexCoord h) const {
        for (CBuilding *b : live)
            if (h.X() >= b->hex.X() && h.X() < b->hex.X()+b->cx &&
                h.Y() >= b->hex.Y() && h.Y() < b->hex.Y()+b->cy) return b;
        return nullptr;
    }
    void Add(CBuilding *b) { live.push_back(b); }
    void Kill(CBuilding *b) {
        for (size_t i = 0; i < live.size(); ++i)
            if (live[i] == b) { live.erase(live.begin() + i); return; }
    }
} theBuildingHex;

struct CBridge { bool built = false; BOOL IsBuilt() const { return built; } };
struct CBridgeUnit { CBridge *parent = nullptr; CBridge *GetParent() const { return parent; } };
struct BridgeHex {
    CHexCoord    at;
    CBridgeUnit *unit = nullptr;
    CBridgeUnit *GetBridge(CHexCoord h) const { return (unit && (at == h)) ? unit : nullptr; }
} theBridgeHex;

struct Game {
    DWORD ms = 1000;
    int   warnings = 0;
    int   byEvent[64] = {};            // every warning, by event id
    DWORD GettimeGetTime() const { return ms; }
    void  Event(int ev, int, void *) { if (ev == EVENT_CONST_CANT) ++warnings; ++byEvent[ev & 63]; }
    void  ClearEvents() { warnings = 0; for (int &n : byEvent) n = 0; }
} theGame;

struct RouteWindow { int refreshes = 0; void RefreshRoute() { ++refreshes; } };
class CVehicle;
struct OldRouteWindow { void NewRoute(CVehicle *) {} };

// --------------------------------------------------------------------------- vehicle
// CVehicle::ResumeUnit / StopUnit chain to their base, so the scene supplies the flag.
struct CUnit {
    enum { vehicle = 1 };
    int  GetUnitType() const { return vehicle; }
    bool stopped = false;
    void ResumeUnit() { stopped = false; }
    void StopUnit() { stopped = true; }
};

struct VehData { bool boat = false; BOOL IsBoat() const { return boat; } };

class CVehicle : public CUnit {
  public:
    enum ORDER_STATE { order_none, order_sent, order_work, order_road, order_done };
    enum VEH_EVENT { none, route, build, build_road, attack, load, repair_self, repair_bldg };
    enum VEH_MODE { stop, moving, run };
    enum VEH_POS { sub, full, center };

    // ---- production methods (bodies come from lifecycle_actual.inc) ----
    void AddOrder(CHexCoord const &hex, int iType, int iBldgType, int iDir,
                  CHexCoord const *pHexEnd = nullptr);
    void ClearOrders();
    BOOL RepairTargetLives(CHexCoord const &hex) const;
    void ReestablishOrderIdentity();
    BOOL ArmedForOrder() const;
    int  OrderGiveUpEvent() const;
    void OrderArrivalFailed();
    void CheckOrderStall();
    BOOL NextOrder();
    void OrderEnded();
    void OrderComplete();
    void OrderFailed(CHexCoord const &hex, int iBldgType);
    void TickCompletion();
    void TickIdlePoll();
    void TestRepairArrival();      // ArrivedDest's repair_bldg case, verbatim
    void SetLocation(CHexCoord &hex, POSITION pos, int iType);   // unit.cpp
    BOOL HasMoveStops() const;                                   // unit.cpp
    void ResumeUnit();                                           // unit.cpp
    void StopUnit();                                             // unit.cpp

    // ---- scene ----
    RouteList    m_route;
    POSITION     m_pos          = nullptr;
    BOOL         m_bRouteLoop   = TRUE;
    BYTE         m_iOrderState  = order_none;
    CHexCoord    m_hexOrder;
    BYTE         m_iOrderKind   = CRoute::waypoint;   // the ctor value finding 3 is about
    DWORD        m_dwOrderStall = 0;
    BYTE         m_iOrderRetry  = 0;
    CBuilding   *m_pBldg        = nullptr;
    int          m_iEvent       = none;
    int          m_cMode        = stop;
    BOOL         m_bResume      = FALSE;
    int          m_iHoldFrames  = 0;
    // ArmedForOrder's move case: still driving.
    BOOL         IsOnTheMove() const { return (m_cMode == moving); }
    CHexCoord    m_hexStart, m_hexEnd;
    CHexCoord    m_hexBldg;
    int          m_iBldgType = 0, m_iBuildDir = 0;
    RouteWindow *m_pSdlRoute = nullptr;
    OldRouteWindow *m_pWndRoute = nullptr;
    Owner       *owner       = nullptr;
    BOOL         m_bDispatched = FALSE;    // did TickIdlePoll dispatch on this call?
    int          requests      = 0;        // BuildBldg-equivalent sends (wire traffic)
    int          resumes       = 0;

    Owner   *GetOwner() const { return owner; }
    POSITION GetRoutePos() const { return m_pos; }
    void     SetRoutePos(POSITION p) { m_pos = p; }
    VehData   m_data;
    VehData  *GetData() const { return const_cast<VehData *>(&m_data); }
    RouteList &GetRouteList() { return m_route; }
    RouteList const &GetRouteList() const { return m_route; }
    void     SetEvent(int e) { m_iEvent = e; }
    void     SetEventAndRoute(int e, int m) { m_iEvent = e; m_cMode = m; }
    void     SetBuilding(CHexCoord const &h, int t, int d) { m_hexBldg = h; m_iBldgType = t; m_iBuildDir = d; }
    void     SetDest(CHexCoord const &) { m_cMode = moving; }
    void     SetDestAndMode(CHexCoord const &, int) { m_cMode = moving; m_bResume = FALSE; m_iHoldFrames = 0; }
    void     SetRoad(CHexCoord const &a, CHexCoord const &b) {
        m_hexStart = a; m_hexEnd = b; m_iEvent = build_road; m_cMode = moving;
    }
    void     StartConst(CBuilding *b) { m_pBldg = b; m_iEvent = build; m_cMode = run; }
    CHexCoord m_ptHead;            // ArrivedDest reads the arrival square from here
    CHexCoord GetPtNext() const { return m_ptHead; }   // StopUnit's SetDest: stand still
    void      EndReverse() {}

    // ---- world events, the engine transitions the lifecycle needs ----
    void ArriveAndSend() {                 // ArrivedDest -> BuildBldg: event cleared, sent
        m_iEvent = none; m_cMode = stop; m_iOrderState = order_sent; ++requests;
    }
    void ServerAccept(CBuilding *b) {      // BldgNew -> StartConst
        m_pBldg = b; m_iEvent = build; m_cMode = run;
    }
    void WorkTick() {                      // ConstructBuilding, minus the production guard
        ReestablishOrderIdentity();
        m_iOrderState = order_work;
    }
    void RoadTick() {                      // ConstructRoad
        ReestablishOrderIdentity();
        m_iOrderState = order_road;
    }
    void SiteGone() { m_pBldg = nullptr; m_iEvent = none; m_cMode = stop; }
    // ArrivedDest, case none: the MOVE-order half, which is all of that case a queued
    // move touches. The vehicle stops where it was sent and the dispatch is consumed.
    void ArriveMoveDest() {
        m_cMode = stop;
        if ((m_iOrderState == order_sent) && (m_iOrderKind == CRoute::move))
            OrderEnded();
    }
    void GiveUpShortOfDest() { m_cMode = stop; OrderArrivalFailed(); }

    // A SAVE/LOAD round trip. The serializer restores the queue, the cursor, the loop
    // flag, the job (event/mode) and the site - and NOT the runtime order identity or
    // state, which is exactly the shape finding 3 is about. Save format 8 is already
    // distributed, so nothing here may add a field.
    void SaveLoadRoundTrip() {
        m_iOrderState  = order_none;              // ctor value (new_unit.cpp:5432-5434)
        m_iOrderKind   = CRoute::waypoint;
        m_hexOrder     = CHexCoord(0, 0);
        m_dwOrderStall = 0;
        m_iOrderRetry  = 0;
        m_bResume      = FALSE;
        m_iHoldFrames  = 0;
    }
};

#include "lifecycle_missing.inc"

#ifdef MISSING_ReestablishOrderIdentity
void CVehicle::ReestablishOrderIdentity() {}
#endif
#ifdef MISSING_ArmedForOrder
BOOL CVehicle::ArmedForOrder() const { return FALSE; }
#endif
#ifdef MISSING_OrderArrivalFailed
void CVehicle::OrderArrivalFailed() {}
#endif
#ifdef MISSING_OrderGiveUpEvent
// the pre-fix behaviour: every give-up raised the building warning
int CVehicle::OrderGiveUpEvent() const { return EVENT_CONST_CANT; }
#endif
#ifdef MISSING_CheckOrderStall
void CVehicle::CheckOrderStall() {}
#endif
#ifdef MISSING_StopUnit
void CVehicle::StopUnit() { CUnit::StopUnit(); SetEvent(none); SetDest(GetPtNext()); }
#endif
#ifdef MISSING_HasMoveStops
// the pre-fix rule: any non-empty list counted as a route
BOOL CVehicle::HasMoveStops() const { return (m_route.GetCount() > 0); }
#endif

// BuildBldgDest picks the footprint hex closest to the crane. The scene keeps the order's
// own hex, which is what a 1x1 site gives and what every assertion below is written for.
static void BuildBldgDest(CVehicle *, int, int, CHexCoord &) {}

// The area window, as far as OnLButtonUp's veh_route case reads it (route window
// Waypoint/Load/Unload: the next map click inserts a stop after m_posRoute).
struct CWndArea {
    enum { normal, veh_route };
    int       m_iMode      = veh_route;
    CUnit    *m_pUnit      = nullptr;
    POSITION  m_posRoute   = nullptr;
    int       m_iRouteType = CRoute::waypoint;
    int       buttons      = 0;
    void SetButtonState() { ++buttons; }
    void RouteClick(CHexCoord hex);      // the shipped case body, verbatim
};

#include "lifecycle_actual.inc"

// --------------------------------------------------------------------------- harness
static int checks = 0, failures = 0;
static void check(bool ok, const char *name) {
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
static void check_eq(int got, int want, const char *name) {
    ++checks;
    if (got != want) { ++failures; std::fprintf(stderr, "FAIL: %s (got %d, want %d)\n", name, got, want); }
}

static Owner g_me;

// one Operate call on a vehicle: the completion half, then the idle poll if stopped
static bool Tick(CVehicle &v, DWORD msElapsed = 0) {
    theGame.ms += msElapsed;
    v.m_bDispatched = FALSE;
    v.TickCompletion();
    if (v.m_cMode == CVehicle::stop)
        v.TickIdlePoll();
    return v.m_bDispatched != FALSE;
}

// drive one queued build all the way through, from a dispatched order to a finished site
static void RunBuild(CVehicle &v, CBuilding &site, CHexCoord hex, int type) {
    v.ArriveAndSend();
    site.hex = hex;
    site.data.type = type;
    theBuildingHex.Add(&site);
    v.ServerAccept(&site);
    v.WorkTick();
}

int main() {
    std::printf("[orders] lifecycle suite (production bodies)\n");

    // ---------------------------------------------------------------- sanity: the
    // ordinary dispatched path still works through the real bodies.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding s1;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        v.AddOrder(CHexCoord(20, 20), CRoute::build, 5, 0);
        check(v.NextOrder() != FALSE, "dispatch: an idle crane takes the head order");
        check_eq(v.m_iOrderKind, CRoute::build, "dispatch: identity kind recorded");
        check_eq(v.m_hexOrder.X(), 10, "dispatch: identity hex recorded");

        RunBuild(v, s1, CHexCoord(10, 10), 4);
        check_eq(v.m_iOrderState, CVehicle::order_work, "work: the site is being built");

        theBuildingHex.Kill(&s1);
        v.SiteGone();
        check(Tick(v), "completion: order 1 consumed and order 2 dispatched");
        check_eq(v.m_route.GetCount(), 1, "completion: exactly one row left");
        check_eq(v.m_hexOrder.X(), 20, "completion: the new order is the identity");
        check_eq(v.requests, 1, "completion: one request for site 1 so far");
    }

    // ---------------------------------------------------------------- FINDING 3
    // A crane SAVED WHILE CONSTRUCTING comes back without the identity NextOrder gives.
    // Before the fix OrderComplete returned at its IsOrder(m_iOrderKind) guard, the
    // finished entry stayed at the head, and NextOrder re-requested a building that was
    // already there - one spurious warning and one wasted request.
    {
        theBuildingHex.live.clear();
        theGame.warnings = 0;
        CVehicle v; v.owner = &g_me;
        CBuilding s1;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        v.AddOrder(CHexCoord(20, 20), CRoute::build, 5, 0);
        check(v.NextOrder() != FALSE, "load/build: order 1 dispatched before the save");
        v.ArriveAndSend();
        s1.hex = CHexCoord(10, 10); s1.data.type = 4;
        theBuildingHex.Add(&s1);
        v.ServerAccept(&s1);
        v.WorkTick();

        v.SaveLoadRoundTrip();                 // <- saved and loaded mid-construction
        check_eq(v.m_iOrderKind, CRoute::waypoint, "load/build: identity really is lost");
        check_eq(v.m_iOrderState, CVehicle::order_none, "load/build: state really is lost");

        v.WorkTick();                          // the first ConstructBuilding after the load
        check_eq(v.m_iOrderKind, CRoute::build, "load/build: identity rebuilt from the site");
        check_eq(v.m_hexOrder.X(), 10, "load/build: rebuilt onto the RIGHT entry");

        int before = v.requests;
        theBuildingHex.Kill(&s1);
        v.SiteGone();
        check(Tick(v), "load/build: the finished order is consumed and the next starts");
        check_eq(v.m_route.GetCount(), 1, "load/build: the finished row is gone");
        check_eq(v.m_route.GetAt(v.m_route.GetHeadPosition())->GetCoord().X(), 20,
                 "load/build: the row left is site 2");
        check_eq(v.m_hexOrder.X(), 20, "load/build: dispatched site 2");
        v.ArriveAndSend();
        check_eq(v.requests - before, 1, "load/build: EXACTLY ONE request for the next site");
        check_eq(theGame.warnings, 0, "load/build: no spurious rejection warning");
    }

    // The identity is rebuilt from the crane's OWN facts, never from the cursor alone.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding other;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        (void)v.NextOrder();
        v.ArriveAndSend();
        other.hex = CHexCoord(77, 77); other.data.type = 4;    // a DIFFERENT site
        theBuildingHex.Add(&other);
        v.ServerAccept(&other);
        v.SaveLoadRoundTrip();
        v.WorkTick();
        check_eq(v.m_iOrderKind, CRoute::waypoint,
                 "load/build: a site that is not the cursor entry's is NOT labelled");
    }
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding wrong;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        (void)v.NextOrder();
        v.ArriveAndSend();
        wrong.hex = CHexCoord(10, 10); wrong.data.type = 9;    // right hex, wrong building
        theBuildingHex.Add(&wrong);
        v.ServerAccept(&wrong);
        v.SaveLoadRoundTrip();
        v.WorkTick();
        check_eq(v.m_iOrderKind, CRoute::waypoint,
                 "load/build: a building of the wrong type is NOT labelled");
    }
    {
        CVehicle v; v.owner = &g_me;                            // no queue at all
        theBuildingHex.live.clear();
        CBuilding s; s.hex = CHexCoord(3, 3); s.data.type = 1;
        theBuildingHex.Add(&s);
        v.ServerAccept(&s);
        v.WorkTick();
        check_eq(v.m_iOrderKind, CRoute::waypoint,
                 "load/build: a plain unqueued build is left alone");
    }
    {   // a movement stop at the cursor must never be labelled an order
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding s; s.hex = CHexCoord(10, 10); s.data.type = 4;
        theBuildingHex.Add(&s);
        v.m_route.AddTail(new CRoute(CHexCoord(10, 10), CRoute::waypoint, 0, 0));
        v.SetRoutePos(v.m_route.GetHeadPosition());
        v.ServerAccept(&s);
        v.WorkTick();
        check_eq(v.m_iOrderKind, CRoute::waypoint, "load/build: a waypoint is never labelled");
        check_eq(v.m_route.GetCount(), 1, "load/build: and it is not consumed");
    }

    // ROADS: the same loss, rebuilt from the run's two ends.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CHexCoord end(14, 10);
        v.AddOrder(CHexCoord(10, 10), CRoute::build_road, 0, 0, &end);
        v.AddOrder(CHexCoord(30, 30), CRoute::build, 5, 0);
        check(v.NextOrder() != FALSE, "load/road: the road run is dispatched");
        v.RoadTick();
        v.SaveLoadRoundTrip();
        v.m_hexStart = CHexCoord(10, 10);      // the serializer DOES restore these
        v.m_hexEnd   = end;
        v.m_iEvent   = CVehicle::build_road;
        v.RoadTick();
        check_eq(v.m_iOrderKind, CRoute::build_road, "load/road: identity rebuilt from the ends");
        v.OrderEnded();
        v.m_cMode = CVehicle::stop; v.m_iEvent = CVehicle::none;
        check(Tick(v), "load/road: the finished run is consumed and the build starts");
        check_eq(v.m_route.GetCount(), 1, "load/road: one row left");
        check_eq(v.m_hexOrder.X(), 30, "load/road: the build is the new identity");
    }
    {   // wrong ends -> not labelled
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CHexCoord end(14, 10);
        v.AddOrder(CHexCoord(10, 10), CRoute::build_road, 0, 0, &end);
        (void)v.NextOrder();
        v.SaveLoadRoundTrip();
        v.m_hexStart = CHexCoord(10, 10);
        v.m_hexEnd   = CHexCoord(99, 99);      // a different run
        v.RoadTick();
        check_eq(v.m_iOrderKind, CRoute::waypoint, "load/road: a different run is NOT labelled");
    }

    // REPAIRS: the order names the hex the player clicked, so the match is on the
    // INSTANCE the crane is welding, not on the building's origin hex.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding target; target.hex = CHexCoord(8, 8); target.data.type = 2;
        theBuildingHex.Add(&target);

        v.AddOrder(CHexCoord(8, 8), CRoute::repair, 0, 0);
        v.AddOrder(CHexCoord(40, 40), CRoute::build, 5, 0);
        check(v.NextOrder() != FALSE, "load/repair: the repair is dispatched");
        v.ArriveAndSend();
        v.ServerAccept(&target);
        v.SaveLoadRoundTrip();
        v.WorkTick();
        check_eq(v.m_iOrderKind, CRoute::repair, "load/repair: identity rebuilt from the site");
        theBuildingHex.Kill(&target);
        v.SiteGone();
        check(Tick(v), "load/repair: the finished repair is consumed and the build starts");
        check_eq(v.m_route.GetCount(), 1, "load/repair: one row left");
    }


    // WinAstra supplementary check: non-origin repair tile of a 2x2 site.
    // The production identity/dispatch/completion methods remain unchanged.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding target; target.hex=CHexCoord(8,8); target.cx=2; target.cy=2;
        theBuildingHex.Add(&target);
        v.AddOrder(CHexCoord(9,9), CRoute::repair, 0, 0);
        v.AddOrder(CHexCoord(40,40), CRoute::build, 5, 0);
        check(v.NextOrder()!=FALSE, "multihex: non-origin repair dispatches");
        v.ArriveAndSend(); v.ServerAccept(&target); v.SaveLoadRoundTrip(); v.WorkTick();
        check_eq(v.m_iOrderKind, CRoute::repair, "multihex: loaded repair identity restored");
        check(v.m_hexOrder==CHexCoord(9,9), "multihex: identity preserves clicked tile, not origin");
        check_eq(v.requests,1,"multihex: reidentification sends no extra request");
        theBuildingHex.Kill(&target); v.SiteGone();
        check(Tick(v),"multihex: completion dispatches the following build");
        check_eq(v.m_route.GetCount(),1,"multihex: completed repair consumed exactly once");
        check_eq(v.m_hexOrder.X(),40,"multihex: next build owns the new identity");
    }
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner=&g_me;
        CBuilding ordered, welded;
        ordered.hex=CHexCoord(8,8); ordered.cx=2; ordered.cy=2;
        welded.hex=CHexCoord(20,20);
        theBuildingHex.Add(&ordered); theBuildingHex.Add(&welded);
        v.AddOrder(CHexCoord(9,9),CRoute::repair,0,0);
        v.m_pBldg=&welded; v.ReestablishOrderIdentity();
        check_eq(v.m_iOrderKind,CRoute::waypoint,"multihex: different welded instance is not matched");
    }

    // ---------------------------------------------------------------- FINDING 4
    // A give-up short of the destination is a FAILED ARRIVAL and says so at once -
    // the watchdog's dwell is no longer the primary path.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        v.AddOrder(CHexCoord(20, 20), CRoute::build, 5, 0);
        check(v.NextOrder() != FALSE, "giveup: dispatched");

        v.GiveUpShortOfDest();
        check_eq(v.m_iOrderState, CVehicle::order_none, "giveup: reported immediately");
        check_eq(v.m_iEvent, CVehicle::none, "giveup: the stale armed event is dropped");
        check_eq(v.m_iOrderRetry, 1, "giveup: one re-drive spent");
        check(Tick(v), "giveup: the SAME order is re-driven on the next tick, no dwell");
        check_eq(v.m_hexOrder.X(), 10, "giveup: re-driven, not skipped");
        check_eq(v.m_route.GetCount(), 2, "giveup: nothing dropped");
        // the budget belongs to the ORDER: a re-drive must not hand a fresh one back, or
        // an unreachable site loops for ever. (This check caught exactly that.)
        check_eq(v.m_iOrderRetry, 1, "giveup: the spent re-drive survives the re-dispatch");
    }
    {   // ...and it is bounded: an unreachable site costs one order, not the queue
        theBuildingHex.live.clear();
        theGame.warnings = 0;
        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        v.AddOrder(CHexCoord(20, 20), CRoute::build, 5, 0);
        (void)v.NextOrder();
        for (int i = 0; i < 20; ++i) {
            v.GiveUpShortOfDest();
            Tick(v);
            if (v.m_route.GetCount() == 1) break;
        }
        check_eq(v.m_route.GetCount(), 1, "giveup: the unreachable order is given up");
        check_eq(v.m_hexOrder.X(), 20, "giveup: and site 2 is dispatched");
        check_eq(theGame.warnings, 1, "giveup: exactly one warning for the dropped order");
    }
    {   // a traffic detour is a job still running - never re-driven through one
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        (void)v.NextOrder();
        v.m_bResume = TRUE; v.m_iHoldFrames = 120;
        v.GiveUpShortOfDest();
        check_eq(v.m_iOrderState, CVehicle::order_sent, "giveup: a live detour is not a failure");
        check_eq(v.m_iEvent, CVehicle::build, "giveup: and the armed event survives it");
        check_eq(v.m_iOrderRetry, 0, "giveup: no re-drive spent during a detour");
    }
    {   // a request genuinely in flight looks idle and must never be re-driven
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
        (void)v.NextOrder();
        v.ArriveAndSend();                       // event cleared, request on the wire
        v.GiveUpShortOfDest();
        check_eq(v.m_iOrderState, CVehicle::order_sent, "in flight: not treated as a failure");
        for (int i = 0; i < 6; ++i) Tick(v, 4000);
        check_eq(v.requests, 1, "in flight: EXACTLY ONE request, not one per dwell");
    }

    // The dead-repair ARRIVAL transition (vehmove.cpp ArrivedDest, case repair_bldg):
    // the target died AFTER dispatch, so the dispatch-time check could not have seen it.
    // The arrival now leaves the vehicle idle and calls OrderEnded, so the poll consumes
    // the order and the next one starts - no watchdog involved.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        CBuilding target; target.hex = CHexCoord(8, 8); target.data.type = 2;
        theBuildingHex.Add(&target);

        v.AddOrder(CHexCoord(8, 8), CRoute::repair, 0, 0);
        v.AddOrder(CHexCoord(40, 40), CRoute::build, 5, 0);
        check(v.NextOrder() != FALSE, "dead repair: dispatched while the target was ALIVE");

        theBuildingHex.Kill(&target);            // it dies during the drive


        v.m_ptHead = CHexCoord(8, 8);
        v.m_iEvent = CVehicle::repair_bldg;
        v.TestRepairArrival();               // the SHIPPED ArrivedDest case body

        check_eq(v.m_iEvent, CVehicle::none, "dead repair: the arrival goes idle");
        check_eq(v.m_cMode, CVehicle::stop, "dead repair: and stopped");
        check_eq(v.m_iOrderState, CVehicle::order_done, "dead repair: the arrival ends the order");
        check(Tick(v), "dead repair: consumed, and the build behind it starts");
        check_eq(v.m_route.GetCount(), 1, "dead repair: the repair row is gone");
        check_eq(v.m_hexOrder.X(), 40, "dead repair: the build is the new identity");
        check_eq(v.m_iOrderRetry, 0, "dead repair: no watchdog re-drive was needed");
    }

    // ...and the two branches that DO have work must be untouched by that change.
    {
        theBuildingHex.live.clear();
        theBridgeHex.unit = nullptr;
        CVehicle v; v.owner = &g_me;
        CBuilding target; target.hex = CHexCoord(8, 8); target.data.type = 2;
        theBuildingHex.Add(&target);
        v.m_ptHead = CHexCoord(8, 8);
        v.m_iEvent = CVehicle::repair_bldg;
        v.TestRepairArrival();
        check(v.m_pBldg == &target, "repair arrival: a live building still welds (StartConst)");
        check_eq(v.m_iEvent, CVehicle::build, "repair arrival: and takes the build event");
        check_eq(v.m_iOrderState, CVehicle::order_none, "repair arrival: the order is NOT ended");
    }
    {
        theBuildingHex.live.clear();
        CBridge span; span.built = false;
        CBridgeUnit bu; bu.parent = &span;
        theBridgeHex.at = CHexCoord(8, 8); theBridgeHex.unit = &bu;
        CVehicle v; v.owner = &g_me;
        v.m_ptHead = CHexCoord(8, 8);
        v.m_iEvent = CVehicle::repair_bldg;
        v.m_iOrderState = CVehicle::order_sent;
        v.TestRepairArrival();
        check_eq(v.m_iEvent, CVehicle::build_road, "repair arrival: an unfinished bridge still builds");
        check_eq(v.m_cMode, CVehicle::run, "repair arrival: and runs");
        check_eq(v.m_iOrderState, CVehicle::order_sent, "repair arrival: the order is NOT ended");

        span.built = true;                   // a bridge somebody else finished meanwhile
        CVehicle w; w.owner = &g_me;
        w.m_ptHead = CHexCoord(8, 8);
        w.m_iEvent = CVehicle::repair_bldg;
        w.m_iOrderState = CVehicle::order_sent;
        w.TestRepairArrival();
        check_eq(w.m_iEvent, CVehicle::none, "repair arrival: a finished bridge goes idle too");
        check_eq(w.m_iOrderState, CVehicle::order_done, "repair arrival: and ends the order");
        theBridgeHex.unit = nullptr;
    }

    // ---------------------------------------------------------------- the Shift-move
    // after queued roads (operator dump, Debug int3 in SetLocation).
    //
    // ResumeUnit used to arm `route` on ANY non-empty list, so a crane holding only
    // queued ORDERS was "routing": ShiftQueueMove then took its append path, read the
    // list TAIL - an order node - and handed it to SetLocation, whose ClearOrders freed
    // it before the walk looked for it. The walk fell off the end and hit the TRAP.
    {
        theBuildingHex.live.clear();
        int trapsBefore = g_traps;

        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(30, 30), CRoute::build_road, 0, 0);
        v.AddOrder(CHexCoord(31, 31), CRoute::build, 4, 0);

        v.ResumeUnit();
        check_eq(v.m_iEvent, CVehicle::none, "order-only list: ResumeUnit does NOT arm route");
        check(v.HasMoveStops() == FALSE, "order-only list: no movement stop on it");

        // ...and even reached that way, the append must survive the order purge
        CHexCoord hexWp(12, 12);
        v.SetLocation(hexWp, v.m_route.GetTailPosition(), CRoute::waypoint);
        check_eq(g_traps - trapsBefore, 0, "shift-move after orders: no TRAP");
        check_eq(v.m_route.GetCount(), 1, "shift-move after orders: the orders are gone");
        check_eq(v.m_route.GetAt(v.m_route.GetHeadPosition())->GetRouteType(),
                 CRoute::waypoint, "shift-move after orders: the waypoint is what is left");
        check_eq(v.m_route.GetAt(v.m_route.GetHeadPosition())->GetCoord().X(), 12,
                 "shift-move after orders: and it is the clicked hex");
        check(v.HasMoveStops() != FALSE, "shift-move after orders: now it IS a route");
        v.ResumeUnit();
        check_eq(v.m_iEvent, CVehicle::route, "movement stop: ResumeUnit arms route again");
    }

    // A waypoint already on the list keeps the ordinary insert-after path: the new stop
    // goes AFTER the tail, the order behind it is dropped, and nothing traps.
    {
        theBuildingHex.live.clear();
        int trapsBefore = g_traps;

        CVehicle v; v.owner = &g_me;
        CHexCoord hexA(5, 5);
        v.SetLocation(hexA, nullptr, CRoute::waypoint);
        v.AddOrder(CHexCoord(30, 30), CRoute::build_road, 0, 0);

        CHexCoord hexB(6, 6);
        v.SetLocation(hexB, v.m_route.GetTailPosition(), CRoute::waypoint);
        check_eq(g_traps - trapsBefore, 0, "append after a waypoint: no TRAP");
        check_eq(v.m_route.GetCount(), 2, "append after a waypoint: two stops");
        POSITION p = v.m_route.GetHeadPosition();
        check_eq(v.m_route.GetNext(p)->GetCoord().X(), 5, "append after a waypoint: first stop kept");
        check_eq(v.m_route.GetNext(p)->GetCoord().X(), 6, "append after a waypoint: new stop is last");
    }

    // (c) dispatch: NextOrder's own ResumeUnit must not pre-empt the order's event.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(30, 30), CRoute::build_road, 0, 0);
        check(v.NextOrder() != FALSE, "order-only list: dispatches");
        check_eq(v.m_iEvent, CVehicle::build_road, "order-only list: the ORDER's event is armed");
    }

    // ---------------------------------------------------------------- the operator's
    // Shift-move-after-queued-roads bug (2026-09-19): a queued MOVE is an ORDER, so it
    // waits its turn behind the roads instead of turning the queue into a route.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;

        v.AddOrder(CHexCoord(30, 30), CRoute::build_road, 0, 0);
        v.AddOrder(CHexCoord(40, 40), CRoute::build_road, 0, 0);
        v.AddOrder(CHexCoord(50, 50), CRoute::move, 0, 0);
        check_eq(v.m_route.GetCount(), 3, "queued move: nothing was wiped by the append");
        check(v.HasMoveStops() == FALSE, "queued move: an order queue, not a route");

        // the crane is busy laying a road right now - NextOrder must refuse
        v.m_iEvent = CVehicle::build_road;
        v.m_cMode  = CVehicle::run;
        check(v.NextOrder() == FALSE, "queued move: busy crane refuses the dispatch");
        check_eq(v.m_route.GetCount(), 3, "queued move: and the queue is untouched");

        // idle: the roads go first, in order
        v.m_iEvent = CVehicle::none;
        v.m_cMode  = CVehicle::stop;
        check(v.NextOrder() != FALSE, "queued move: idle crane dispatches");
        check_eq(v.m_iOrderKind, CRoute::build_road, "queued move: the FIRST road went first");
        check_eq(v.m_hexOrder.X(), 30, "queued move: and it is the head order");

        v.OrderEnded(); v.OrderComplete();
        v.m_iEvent = CVehicle::none;    // the road run ended: the crane is idle again
        v.m_cMode  = CVehicle::stop;
        check(v.NextOrder() != FALSE, "queued move: second road dispatches");
        check_eq(v.m_iOrderKind, CRoute::build_road, "queued move: second road is a road too");
        check_eq(v.m_hexOrder.X(), 40, "queued move: and it is the second one");

        v.OrderEnded(); v.OrderComplete();
        check_eq(v.m_route.GetCount(), 1, "queued move: only the move is left");
        v.m_iEvent = CVehicle::none;
        v.m_cMode  = CVehicle::stop;
        check(v.NextOrder() != FALSE, "queued move: the move dispatches last");
        check_eq(v.m_iOrderKind, CRoute::move, "queued move: dispatched kind is move");
        check_eq(v.m_hexOrder.X(), 50, "queued move: at the hex that was clicked");
        check_eq(v.m_iEvent, CVehicle::none, "queued move: runs under event none");
        check_eq(v.m_cMode, CVehicle::moving, "queued move: and it is driving");

        // while driving it is NOT a give-up, so the watchdogs leave it alone
        check(v.ArmedForOrder() == FALSE, "queued move: driving is not idle-and-armed");
        v.m_cMode = CVehicle::stop;
        check(v.ArmedForOrder() != FALSE, "queued move: idle and not arrived IS");

        // arrival completes it and the queue is empty
        v.m_cMode = CVehicle::moving;
        v.ArriveMoveDest();
        check_eq(v.m_iOrderState, CVehicle::order_done, "queued move: arrival ends the order");
        Tick(v);
        check_eq(v.m_route.GetCount(), 0, "queued move: the list is empty afterwards");
        check_eq(v.m_iOrderState, CVehicle::order_none, "queued move: and nothing is running");
    }

    // ---------------------------------------------------------------- GIVE-UP WARNING
    // EVENT_CONST_CANT's text is the BUILDING's name (CGame::Event reads GetBldgType), and
    // only a build dispatch sets that. A road, repair or move order given up must not
    // raise it - on a crane that never built, m_iBldgType was uninitialised and the
    // structure lookup read out of bounds. Both give-up exits are checked.
    {
        CHexCoord end(14, 10);
        CBuilding target; target.hex = CHexCoord(8, 8); target.data.type = 2;
        struct { int kind; int want; const char *name; } const kinds[] = {
            { CRoute::build_road, EVENT_ROAD_HALTED, "road" },
            { CRoute::repair,     EVENT_GOTO_CANT,   "repair" },
            { CRoute::move,       EVENT_GOTO_CANT,   "move" },
            { CRoute::build,      EVENT_CONST_CANT,  "build" },
        };
        for (auto const &k : kinds) {
            for (int exitPath = 0; exitPath < 2; ++exitPath) {
                theBuildingHex.live.clear();
                theBuildingHex.Add(&target);
                theGame.ClearEvents();
                CVehicle v; v.owner = &g_me;
                CHexCoord at = (k.kind == CRoute::repair) ? CHexCoord(8, 8) : CHexCoord(10, 10);
                v.AddOrder(at, k.kind, (k.kind == CRoute::build) ? 4 : 0, 0,
                           (k.kind == CRoute::build_road) ? &end : nullptr);
                v.AddOrder(CHexCoord(40, 40), CRoute::build, 5, 0);
                (void)v.NextOrder();
                for (int i = 0; (i < 20) && (v.m_route.GetCount() == 2); ++i) {
                    if (exitPath == 0) {
                        v.GiveUpShortOfDest();               // OrderArrivalFailed
                        Tick(v);
                    } else {
                        v.m_cMode = CVehicle::stop;          // idle and armed: the watchdog
                        Tick(v, 4000);
                    }
                }
                char msg[160];
                std::snprintf(msg, sizeof msg, "give-up warning: %s order via %s is dropped",
                              k.name, exitPath ? "CheckOrderStall" : "OrderArrivalFailed");
                check_eq(v.m_route.GetCount(), 1, msg);
                std::snprintf(msg, sizeof msg, "give-up warning: %s order via %s raises its own event",
                              k.name, exitPath ? "CheckOrderStall" : "OrderArrivalFailed");
                check_eq(theGame.byEvent[k.want & 63], 1, msg);
                if (k.kind != CRoute::build) {
                    std::snprintf(msg, sizeof msg, "give-up warning: %s order via %s never raises EVENT_CONST_CANT",
                                  k.name, exitPath ? "CheckOrderStall" : "OrderArrivalFailed");
                    check_eq(theGame.warnings, 0, msg);
                }
            }
        }
        theBuildingHex.live.clear();
        theGame.ClearEvents();
    }

    // ---------------------------------------------------------------- STOP, then RESUME
    // The Stop button clears the event of whatever the crane was doing. A dispatched
    // order must go back to the queue with it: left order_sent (event cleared, so not
    // "armed") or order_road (its terminal branches never run now), nothing ended it and
    // NextOrder's busy test refused for ever - the whole queue was dead. The order stays
    // at the cursor and the idle poll re-dispatches it after Resume.
    {
        CHexCoord end(14, 10);
        CBuilding target; target.hex = CHexCoord(8, 8); target.data.type = 2;
        struct { int kind; int ev; bool laying; const char *name; } const kinds[] = {
            { CRoute::build,      CVehicle::build,       false, "build (travelling)" },
            { CRoute::build_road, CVehicle::build_road,  false, "road (travelling)" },
            { CRoute::build_road, CVehicle::build_road,  true,  "road (laying)" },
            { CRoute::repair,     CVehicle::repair_bldg, false, "repair (travelling)" },
            { CRoute::move,       CVehicle::none,        false, "move (driving)" },
        };
        for (auto const &k : kinds) {
            theBuildingHex.live.clear();
            theBuildingHex.Add(&target);
            CVehicle v; v.owner = &g_me;
            CHexCoord at = (k.kind == CRoute::repair) ? CHexCoord(8, 8) : CHexCoord(10, 10);
            v.AddOrder(at, k.kind, (k.kind == CRoute::build) ? 4 : 0, 0,
                       (k.kind == CRoute::build_road) ? &end : nullptr);
            v.AddOrder(CHexCoord(40, 40), CRoute::build, 5, 0);
            (void)v.NextOrder();
            if (k.laying)
                v.RoadTick();                       // ConstructRoad: order_road
            v.StopUnit();                           // the Stop button
            v.ResumeUnit();                         // ...and Resume
            v.ArriveMoveDest();                     // StopUnit's SetDest(ptNext) leg ends
            for (int i = 0; i < 6; ++i)
                Tick(v, 4000);
            char msg[160];
            std::snprintf(msg, sizeof msg, "stop/resume %s: the order is still queued", k.name);
            check_eq(v.m_route.GetCount(), 2, msg);
            std::snprintf(msg, sizeof msg, "stop/resume %s: it is the order re-dispatched", k.name);
            check(v.m_hexOrder == at, msg);
            std::snprintf(msg, sizeof msg, "stop/resume %s: under way again", k.name);
            check_eq(v.m_iOrderState, CVehicle::order_sent, msg);
            std::snprintf(msg, sizeof msg, "stop/resume %s: its event is armed again", k.name);
            check_eq(v.m_iEvent, k.ev, msg);
            std::snprintf(msg, sizeof msg, "stop/resume %s: and it is driving", k.name);
            check_eq(v.m_cMode, CVehicle::moving, msg);
        }
        {   // a build request ALREADY SENT keeps waiting for the server's answer
            theBuildingHex.live.clear();
            CVehicle v; v.owner = &g_me;
            v.AddOrder(CHexCoord(10, 10), CRoute::build, 4, 0);
            (void)v.NextOrder();
            v.ArriveAndSend();
            v.StopUnit();
            check_eq(v.m_iOrderState, CVehicle::order_sent, "stop/resume: a sent build request is left in flight");
            v.ResumeUnit();
            v.ArriveMoveDest();
            Tick(v, 4000); Tick(v, 4000);
            check_eq(v.requests, 1, "stop/resume: and is not sent twice");
        }
        {   // nothing dispatched: a plain job's Stop is untouched
            CVehicle v; v.owner = &g_me;
            v.SetEvent(CVehicle::build); v.m_cMode = CVehicle::moving;
            v.StopUnit();
            check_eq(v.m_iOrderState, CVehicle::order_none, "stop/resume: a plain job has no order state");
            check_eq(v.m_iEvent, CVehicle::none, "stop/resume: and Stop still clears its event");
        }
        theBuildingHex.live.clear();
    }

    // ---------------------------------------------------------------- STALE ROUTE ROW
    // Routes window: select a row, press Waypoint (m_posRoute = that row), press Delete
    // on the same row, then click the map. The captured POSITION is no longer on the list;
    // SetLocation's walk misses it and hits its TRAP - a Debug crash. The click appends at
    // the tail instead (what Release did), and the TRAP stays for genuinely foreign input.
    {
        int trapsBefore = g_traps;
        CVehicle v; v.owner = &g_me;
        CHexCoord a(5, 5), b(6, 6), c(7, 7);
        v.SetLocation(a, nullptr, CRoute::waypoint);
        v.SetLocation(b, v.m_route.GetTailPosition(), CRoute::waypoint);
        CWndArea area; area.m_pUnit = &v;
        area.m_posRoute = v.m_route.GetTailPosition();        // Waypoint pressed on row 2
        CRoute *pDel = v.m_route.GetAt(area.m_posRoute);       // ...then Delete on row 2
        v.m_route.RemoveAt(area.m_posRoute);
        delete pDel;
        area.RouteClick(c);                                    // ...then the map click
        check_eq(g_traps - trapsBefore, 0, "stale route row: no TRAP");
        check_eq(v.m_route.GetCount(), 2, "stale route row: the stop is added");
        check(v.m_route.GetAt(v.m_route.GetTailPosition())->GetCoord() == c,
              "stale route row: appended at the tail");

        // a LIVE row is still honoured: insert after row 1, ahead of the tail
        area.m_posRoute = v.m_route.GetHeadPosition();
        CHexCoord d(8, 8);
        area.RouteClick(d);
        POSITION p = v.m_route.GetHeadPosition();
        v.m_route.GetNext(p);
        check(v.m_route.GetAt(p)->GetCoord() == d, "live route row: inserted right after it");
        // and NULL still means the head
        area.m_posRoute = nullptr;
        CHexCoord e(9, 9);
        area.RouteClick(e);
        check(v.m_route.GetAt(v.m_route.GetHeadPosition())->GetCoord() == e,
              "no route row: inserted at the head");
        check_eq(g_traps - trapsBefore, 0, "route rows: no TRAP at all");
    }

    // ---------------------------------------------------------------- STALE ORDER KIND
    // A queued move finishes, then the player places a PLAIN build (area.cpp: ClearOrders,
    // then the engine's BuildBldg sets order_sent with the event cleared). The last
    // dispatch's kind used to survive ClearOrders, so the in-flight request looked like an
    // idle move order: ArmedForOrder TRUE, re-driven by the stall watch after its dwell,
    // and not protected by StopUnit's / LeaveRoad's "request in flight" guards.
    {
        theBuildingHex.live.clear();
        CVehicle v; v.owner = &g_me;
        v.AddOrder(CHexCoord(50, 50), CRoute::move, 0, 0);
        (void)v.NextOrder();
        v.ArriveMoveDest();
        Tick(v);
        check_eq(v.m_route.GetCount(), 0, "stale kind: the queued move finished");

        v.ClearOrders();                     // the plain placement replaces the queue
        v.SetEvent(CVehicle::build); v.m_cMode = CVehicle::moving;
        v.ArriveAndSend();                   // BuildBldg: order_sent, event none, stopped
        check(v.ArmedForOrder() == FALSE, "stale kind: a plain build in flight is not an armed order");
        for (int i = 0; i < 4; ++i)
            Tick(v, 4000);
        check_eq(v.m_iOrderState, CVehicle::order_sent, "stale kind: the stall watch leaves the request in flight");
        v.StopUnit();
        check_eq(v.m_iOrderState, CVehicle::order_sent, "stale kind: and so does a Stop");
        v.ResumeUnit();
    }

    std::printf("[orders] lifecycle: %d checks, %d failures, %d traps\n", checks, failures, g_traps);
    return failures ? 1 : 0;
}
