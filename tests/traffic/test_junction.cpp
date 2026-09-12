// Production GetNextHex step selection at an open paved junction, against a
// minimal scene. The step arithmetic, the angle limit, the lane parity fix and
// the road-turn correction are the shipped bodies; only the map, occupancy,
// pathfinder and building services are replaced.
//
// This is a STEP-SELECTION check. It records what each vehicle ASKS for in the
// same tick, from the same scene state, exactly as WinAstra's witness is stated.
// It does not model reservation, interpolation or the retry ladder: a pair whose
// requests coincide is stopped at that tick, because everything after a shared
// sub-hex is a scene the engine would never have produced.
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using BOOL = int;
using DWORD = unsigned long;
constexpr BOOL TRUE = 1, FALSE = 0;

#define ASSERT(x) ((void)0)
#define ASSERT_VALID(x) ((void)0)
#define TRAP(x) ((void)0)
#define __minmax(lo, hi, v) (((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v)))

// vehmove.cpp:31, base.h:74-77
const int MAX_TIMES_CIRCLE = 16;
const int MAX_HEX_HT = 64, STEPS_HEX = MAX_HEX_HT / 4, FULL_ROT = STEPS_HEX * 8, EIGHTH_ROT = FULL_ROT / 8;

// A 64x64 hex map is 128x128 sub-hexes; Diff wraps like the shipped one.
constexpr int SUBS = 128, HEXES = 64;

struct CHexCoord;
struct CPoint {
    int x = 0, y = 0;
    CPoint() = default;
    CPoint(int a, int b): x(a), y(b) {}
};
struct CSubHex : public CPoint {
    CSubHex() = default;
    CSubHex(int a, int b) { x = a; y = b; }
    CSubHex &Wrap() { x = (x + SUBS) % SUBS; y = (y + SUBS) % SUBS; return *this; }
    static int Diff(int d) { return (d + SUBS + SUBS / 2) % SUBS - SUBS / 2; }
    BOOL operator==(CSubHex s) const { return (x == s.x) && (y == s.y); }
    BOOL operator!=(CSubHex s) const { return (x != s.x) || (y != s.y); }
    BOOL SameHex(CSubHex s) const { return ((x & ~1) == (s.x & ~1)) && ((y & ~1) == (s.y & ~1)); }
    BOOL SameHex(CHexCoord s) const;
    CHexCoord ToCoord() const;
};
struct CHexCoord {
    int m_iX = 0, m_iY = 0;
    CHexCoord() = default;
    CHexCoord(int a, int b): m_iX(a), m_iY(b) {}
    CHexCoord(CSubHex const &s): m_iX(s.x >> 1), m_iY(s.y >> 1) {}
    int X() const { return m_iX; }
    int &X() { return m_iX; }
    int Y() const { return m_iY; }
    int &Y() { return m_iY; }
    CHexCoord &Wrap() { m_iX = (m_iX + HEXES) % HEXES; m_iY = (m_iY + HEXES) % HEXES; return *this; }
    static int Diff(int d) { return (d + HEXES + HEXES / 2) % HEXES - HEXES / 2; }
    BOOL SameHex(CSubHex s) const { return ((s.x >> 1) == m_iX) && ((s.y >> 1) == m_iY); }
    BOOL operator==(CHexCoord s) const { return (m_iX == s.m_iX) && (m_iY == s.m_iY); }
    BOOL operator!=(CHexCoord s) const { return (m_iX != s.m_iX) || (m_iY != s.m_iY); }
};
inline BOOL CSubHex::SameHex(CHexCoord s) const { return ((x >> 1) == s.m_iX) && ((y >> 1) == s.m_iY); }
inline CHexCoord CSubHex::ToCoord() const { return CHexCoord(x >> 1, y >> 1); }

struct CHex {
    enum { bldg = 1, bridge = 2 };
    enum { plain = 0, road = 1, city = 2, lake = 3 };
    int units = 0, type = plain;
    int GetUnits() const { return units; }
    int GetType() const { return type; }
};
struct Map {
    CHex hex[HEXES][HEXES];
    CHex *_GetHex(CHexCoord h) { return &hex[h.X() & (HEXES - 1)][h.Y() & (HEXES - 1)]; }
    CHex *_GetHex(CSubHex s) { return &hex[(s.x >> 1) & (HEXES - 1)][(s.y >> 1) & (HEXES - 1)]; }
    // uniform cost: the mid-function terrain-speed detour must stay inert here
    int GetTerrainCost(CHexCoord a, CHexCoord, int, int) { return _GetHex(a)->type == CHex::lake ? 0 : 2; }
} theMap;

struct Owner {
    BOOL IsLocal() const { return TRUE; }
    BOOL IsMe() const { return TRUE; }
    BOOL IsAI() const { return FALSE; }
};
struct CTransportData {
    enum { FL1hex = 1 };
    BOOL IsBoat() const { return FALSE; }
    BOOL IsTransport() const { return TRUE; }
    BOOL IsCrane() const { return FALSE; }
    int GetVehFlags() const { return 0; }
    int GetWheelType() const { return 1; }
    BOOL CanEnterHex(CHexCoord, CHexCoord, BOOL, BOOL) const { return TRUE; }
};
struct Game {
    BOOL IsNetGame() const { return FALSE; }
    DWORD GettimeGetTime() const { return 100000; }
} theGame;
struct CBuilding {};
struct BuildingMap {
    CBuilding *_GetBuilding(CHexCoord) { return nullptr; }
} theBuildingHex;

int MyRand() { return 0; }
int RandNum(int iMax) { return iMax / 2; }

int g_traffic = 63;
int TrafficOpts() { return g_traffic; }

// any recovery path taken means the recorded step did NOT come from the step
// arithmetic under test, so the scenes must never reach one
int g_recovery = 0;
int g_roadTurns = 0;
char g_lastTurn[256] = "";
void WaitLog(const char *fmt, ...) {
    char line[256];
    va_list va;
    va_start(va, fmt);
    vsnprintf(line, sizeof(line), fmt, va);
    va_end(va);
    if (std::strstr(line, "[ROAD-TURN]") != nullptr) {
        ++g_roadTurns;
        if (g_lastTurn[0] == 0)
            std::snprintf(g_lastTurn, sizeof(g_lastTurn), "%s", line);
    }
}

// production helpers arrive in the .inc; the class body below needs them first
extern int aiBaseDir[9];
int GetDirIndex(CPoint const &ptHead, CPoint const &ptTail);
CSubHex Rotate(int iDir, CSubHex const &ptHead, CSubHex const &ptTail);
int GetAngle(const CSubHex &, const CSubHex &, const CSubHex &, const CSubHex &);

struct CVehicle;
using CUnit = CVehicle;
struct VehicleMap {
    CVehicle *occ[SUBS][SUBS];
    VehicleMap() { std::memset(occ, 0, sizeof(occ)); }
    CVehicle *_GetVehicle(CSubHex s) { return occ[s.x & (SUBS - 1)][s.y & (SUBS - 1)]; }
    CVehicle *GetVehicle(CSubHex s) { return _GetVehicle(s); }
    void Set(CSubHex s, CVehicle *v) { occ[s.x & (SUBS - 1)][s.y & (SUBS - 1)] = v; }
} theVehicleHex;

struct CVehicle {
    enum { stop = 0, moving = 1, traffic = 2, blocked = 3 };
    enum { none = 0, route = 1, load = 5 };
    enum { at_end_of_path = 0x20 };

    CSubHex m_ptHead, m_ptTail, m_ptNext, m_ptDest;
    CHexCoord m_hexNext, m_hexDest;
    CHexCoord *m_phexPath = nullptr;
    int m_iTimesOn = 0, m_bFlags = 0, m_iEvent = none;
    int m_iPathLen = 4, m_iPathOff = 0;
    BOOL m_bReversing = FALSE, m_bResume = FALSE, m_bForwardEscape = FALSE;
    int m_iHoldFrames = 0, m_iNumRetries = 0, m_cMode = moving;
    BOOL m_cOwn = TRUE;
    DWORD m_dwBlockLog = 0;

    int m_iDir = 0;
    int id = 1;
    BOOL manual = FALSE, keepLane = FALSE;
    int findSubCalls = 0, blockedCalls = 0, waitCalls = 0;
    Owner owner;
    CTransportData data;

    Owner *GetOwner() { return &owner; }
    CTransportData *GetData() { return &data; }
    int GetID() const { return id; }
    CHexCoord GetHexHead() const { return CHexCoord(m_ptHead); }
    BOOL IsOnWater() const { return FALSE; }
    BOOL IsHpControl() const { return manual; }
    BOOL JamEligible() const { return FALSE; }
    BOOL CanEnterBldg(CBuilding *) { return TRUE; }
    BOOL HavePathOrNext() { return TRUE; }
    void PathNextHex() {}
    void GetPath(BOOL) {}
    void MakeBlocked() { ++blockedCalls; ++g_recovery; }
    void PostArrivedOrBlocked() { ++blockedCalls; ++g_recovery; }
    void OrderArrivalFailed() {}
    void _SetRouteMode(int m) { m_cMode = m; }
    void SetLoc(BOOL) {}
    void CheckNextHex() {}
    BOOL WaitForMover() { ++waitCalls; ++g_recovery; return FALSE; }
    BOOL FindSub(BOOL = FALSE) { ++findSubCalls; ++g_recovery; return TRUE; }
    BOOL MustKeepLane(CSubHex &step) { step = m_ptNext; return keepLane; }
    BOOL OnPavement(CSubHex const &s) {
        CHex *h = theMap._GetHex(s);
        return (h->units & CHex::bridge) || (h->type == CHex::road) || (h->type == CHex::city);
    }
    BOOL CanEnter(CSubHex const &s) {
        CVehicle *on = theVehicleHex._GetVehicle(s);
        if ((on != nullptr) && (on != this))
            return FALSE;
        CHex *h = theMap._GetHex(s);
        return (h->type != CHex::lake) && (!(h->units & CHex::bldg));
    }
    int CalcBaseDir() const { return aiBaseDir[GetDirIndex(m_ptHead, m_ptTail)]; }
    int CalcNextBaseDir() const { return aiBaseDir[GetDirIndex(m_ptNext, m_ptHead)]; }
    CSubHex Rotate(int iDir) { return ::Rotate(iDir, m_ptHead, m_ptTail); }

    BOOL GetNextHex(BOOL bNew);
};

#include "junction_actual.inc"

// ---------------------------------------------------------------- scene ----

int checks = 0, failures = 0;
void check(bool ok, const char *msg) {
    ++checks;
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", msg);
    }
}

void ClearMap() {
    std::memset(theVehicleHex.occ, 0, sizeof(theVehicleHex.occ));
    for (int x = 0; x < HEXES; ++x)
        for (int y = 0; y < HEXES; ++y) {
            theMap.hex[x][y].type = CHex::plain;
            theMap.hex[x][y].units = 0;
        }
}

// hx,hy,dx,dy - the direction THIS truck travels through that arm hex
struct Lane { int hx, hy, dx, dy; };
struct TruckPlan {
    const char *name;
    int head[2], tail[2];
    int route[3][2];
    int dest[2];
    Lane lane[5];
};
struct BendCase {
    const char *name;
    int junction[2];
    int paved[9][2];
    TruckPlan t[2];
};

// the shipped lane convention: +X rides odd y, -X even y, +Y even x, -Y odd x
int LaneVerdict(const BendCase &c, const TruckPlan &p, CSubHex s) {
    if (((s.x >> 1) == c.junction[0]) && ((s.y >> 1) == c.junction[1]))
        return 0;  // the junction hex itself belongs to neither arm
    for (int i = 0; i < 5; ++i) {
        if (p.lane[i].hx < 0)
            break;
        if (((s.x >> 1) == p.lane[i].hx) && ((s.y >> 1) == p.lane[i].hy)) {
            if (p.lane[i].dx != 0)
                return ((s.y & 1) == ((p.lane[i].dx > 0) ? 1 : 0)) ? 1 : -1;
            return ((s.x & 1) == ((p.lane[i].dy > 0) ? 0 : 1)) ? 1 : -1;
        }
    }
    return 0;
}

void Build(CVehicle &v, const TruckPlan &p, int id) {
    v = CVehicle();
    v.id = id;
    v.m_ptHead = CSubHex(p.head[0], p.head[1]);
    v.m_ptTail = CSubHex(p.tail[0], p.tail[1]);
    v.m_ptNext = v.m_ptHead;
    v.m_ptDest = CSubHex(p.dest[0], p.dest[1]);
    v.m_hexDest = CHexCoord(p.dest[0] >> 1, p.dest[1] >> 1);
    v.m_hexNext = CHexCoord(p.route[0][0], p.route[0][1]);
    v.m_cMode = CVehicle::moving;
}
void Occupy(CVehicle *v, CVehicle *who) {
    theVehicleHex.Set(v->m_ptHead, who);
    theVehicleHex.Set(v->m_ptTail, who);
}

constexpr int TICKS = 3;
struct Trace {
    int n = 0;
    CSubHex req[TICKS];
    bool ok[TICKS] = {false, false, false};
    bool opposing[TICKS] = {false, false, false};
};

// Both trucks decide from the SAME scene state; nothing is committed until both
// have asked. A shared request ends the run - see the file header.
bool RunBend(const BendCase &c, int traffic, Trace tr[2], int &collideTick) {
    g_traffic = traffic;
    collideTick = -1;
    ClearMap();
    for (int i = 0; c.paved[i][0] >= 0; ++i)
        theMap.hex[c.paved[i][0]][c.paved[i][1]].type = CHex::road;

    static CVehicle v[2];
    int ri[2] = {0, 0};
    for (int i = 0; i < 2; ++i) {
        Build(v[i], c.t[i], i + 1);
        tr[i] = Trace();
    }
    for (int i = 0; i < 2; ++i)
        Occupy(&v[i], &v[i]);

    for (int tick = 0; tick < TICKS; ++tick) {
        CSubHex want[2];
        for (int i = 0; i < 2; ++i) {
            if (!v[i].GetNextHex(FALSE))
                return false;
            want[i] = v[i].m_ptNext;
            tr[i].req[tr[i].n] = want[i];
            tr[i].ok[tr[i].n] = LaneVerdict(c, c.t[i], want[i]) >= 0;
            tr[i].opposing[tr[i].n] = LaneVerdict(c, c.t[i], want[i]) < 0;
            ++tr[i].n;
        }
        if (want[0] == want[1]) {
            collideTick = tick;
            return true;
        }
        for (int i = 0; i < 2; ++i) {
            Occupy(&v[i], nullptr);
            v[i].m_ptTail = v[i].m_ptHead;
            v[i].m_ptHead = want[i];
        }
        for (int i = 0; i < 2; ++i)
            Occupy(&v[i], &v[i]);
        for (int i = 0; i < 2; ++i)
            if ((ri[i] + 1 < 3) && v[i].m_hexNext.SameHex(v[i].m_ptHead)) {
                ++ri[i];
                v[i].m_hexNext = CHexCoord(c.t[i].route[ri[i]][0], c.t[i].route[ri[i]][1]);
            }
    }
    return true;
}

// ------------------------------------------------------- the four bends ----
//
// One junction hex 11,10 with two paved arms. WinAstra's witness is bend 0:
// western hex 10,10 to southern hex 11,11, trucks 21,21 east-then-south and
// 23,22 north-then-west, both asking for 22,22.

const BendCase g_bends[4] = {
 {"W-S", {11, 10},
  {{10,10},{9,10},{8,10},{11,10},{11,11},{11,12},{11,13},{-1,-1},{-1,-1}},
  {{"A W>S", {21,21}, {20,21}, {{11,11},{11,12},{11,13}}, {22,27},
    {{10,10,1,0},{11,11,0,1},{11,12,0,1},{11,13,0,1},{-1,-1,0,0}}},
   {"B S>W", {23,22}, {23,23}, {{10,10},{9,10},{8,10}}, {17,20},
    {{11,11,0,-1},{10,10,-1,0},{9,10,-1,0},{8,10,-1,0},{-1,-1,0,0}}}}},

 {"E-S", {11, 10},
  {{12,10},{13,10},{14,10},{11,10},{11,11},{11,12},{11,13},{-1,-1},{-1,-1}},
  {{"C E>S", {24,20}, {25,20}, {{11,11},{11,12},{11,13}}, {22,27},
    {{12,10,-1,0},{11,11,0,1},{11,12,0,1},{11,13,0,1},{-1,-1,0,0}}},
   {"D S>E", {23,22}, {23,23}, {{12,10},{13,10},{14,10}}, {29,21},
    {{11,11,0,-1},{12,10,1,0},{13,10,1,0},{14,10,1,0},{-1,-1,0,0}}}}},

 {"W-N", {11, 10},
  {{10,10},{9,10},{8,10},{11,10},{11,9},{11,8},{11,7},{-1,-1},{-1,-1}},
  {{"E W>N", {21,21}, {20,21}, {{11,9},{11,8},{11,7}}, {23,15},
    {{10,10,1,0},{11,9,0,-1},{11,8,0,-1},{11,7,0,-1},{-1,-1,0,0}}},
   {"F N>W", {22,19}, {22,18}, {{10,10},{9,10},{8,10}}, {17,20},
    {{11,9,0,1},{10,10,-1,0},{9,10,-1,0},{8,10,-1,0},{-1,-1,0,0}}}}},

 {"E-N", {11, 10},
  {{12,10},{13,10},{14,10},{11,10},{11,9},{11,8},{11,7},{-1,-1},{-1,-1}},
  {{"G E>N", {24,20}, {25,20}, {{11,9},{11,8},{11,7}}, {23,15},
    {{12,10,-1,0},{11,9,0,-1},{11,8,0,-1},{11,7,0,-1},{-1,-1,0,0}}},
   {"H N>E", {22,19}, {22,18}, {{12,10},{13,10},{14,10}}, {29,21},
    {{11,9,0,1},{12,10,1,0},{13,10,1,0},{14,10,1,0},{-1,-1,0,0}}}}}
};

// ---------------------------------------------------------- the controls ---
//
// Every control asks the SAME vehicle for one step with the correction off and
// on, and requires the two answers to be identical.

enum Tweak { plainGround = 1, bridgeDeck = 2, corridor = 4, nearDest = 8, backing = 16 };

CSubHex OneStep(const BendCase &c, const TruckPlan &p, int traffic, int tweak) {
    g_traffic = traffic;
    ClearMap();
    if (!(tweak & plainGround))
        for (int i = 0; c.paved[i][0] >= 0; ++i)
            theMap.hex[c.paved[i][0]][c.paved[i][1]].type = CHex::road;
    if (tweak & bridgeDeck)
        for (int i = 0; c.paved[i][0] >= 0; ++i)
            theMap.hex[c.paved[i][0]][c.paved[i][1]].units |= CHex::bridge;
    static CVehicle v;
    Build(v, p, 9);
    if (tweak & corridor)
        v.keepLane = TRUE;
    if (tweak & backing)
        v.m_bReversing = TRUE;
    if (tweak & nearDest) {
        v.m_ptDest = CSubHex(v.m_ptHead.x, v.m_ptHead.y + 3);
        v.m_hexDest = CHexCoord(v.m_ptDest);
    }
    theVehicleHex.Set(v.m_ptHead, &v);
    theVehicleHex.Set(v.m_ptTail, &v);
    v.GetNextHex(FALSE);
    CSubHex rtn = v.m_ptNext;
    theVehicleHex.Set(v.m_ptHead, nullptr);
    theVehicleHex.Set(v.m_ptTail, nullptr);
    return rtn;
}

// A T where the outgoing arm is SQUARELY adjacent to the hex the truck is in:
// that turn is takeable from here, is already in the outgoing lane, and must not
// be deferred. Only a diagonally adjacent route hex is a corner cut.
const BendCase g_square = {
 "square", {10, 10},
 {{9,10},{10,10},{11,10},{10,9},{10,8},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
 {{"R E>N", {21,21}, {20,21}, {{10,9},{10,8},{10,8}}, {21,15},
   {{10,9,0,-1},{10,8,0,-1},{-1,-1,0,0},{-1,-1,0,0},{-1,-1,0,0}}},
  {"R E>N", {21,21}, {20,21}, {{10,9},{10,8},{10,8}}, {21,15},
   {{10,9,0,-1},{10,8,0,-1},{-1,-1,0,0},{-1,-1,0,0},{-1,-1,0,0}}}}
};

// a four-way crossroads, for the straight-through controls
const BendCase g_cross = {
 "cross", {11, 10},
 {{9,10},{10,10},{11,10},{12,10},{13,10},{11,9},{11,11},{-1,-1},{-1,-1}},
 {{"P east", {21,21}, {20,21}, {{12,10},{13,10},{13,10}}, {27,21},
   {{10,10,1,0},{11,11,0,0},{12,10,1,0},{13,10,1,0},{-1,-1,0,0}}},
  {"Q west", {24,20}, {25,20}, {{10,10},{9,10},{9,10}}, {17,20},
   {{12,10,-1,0},{11,11,0,0},{10,10,-1,0},{9,10,-1,0},{-1,-1,0,0}}}}
};

int main() {
    std::printf("bend  truck   tick  pre-fix      post-fix\n");
    for (int b = 0; b < 4; ++b) {
        Trace pre[2], post[2];
        int preHit = -1, postHit = -1;
        check(RunBend(g_bends[b], 63 & ~16, pre, preHit) != false, "pre-fix run produces a step every tick");
        check(RunBend(g_bends[b], 63, post, postHit) != false, "post-fix run produces a step every tick");

        for (int i = 0; i < 2; ++i)
            for (int t = 0; t < post[i].n; ++t) {
                char was[16] = "-";
                if (t < pre[i].n)
                    std::snprintf(was, sizeof(was), "%d,%d%s", pre[i].req[t].x, pre[i].req[t].y,
                                  pre[i].opposing[t] ? "!" : "");
                std::printf("%-5s %-7s %d     %-12s %d,%d\n", g_bends[b].name, g_bends[b].t[i].name, t,
                            was, post[i].req[t].x, post[i].req[t].y);
            }

        // the defect
        check(preHit == 0, "pre-fix: the two trucks ask for one sub-hex on the first tick");
        check(pre[0].n > 0 && pre[1].n > 0 && pre[0].req[0] == pre[1].req[0],
              "pre-fix: the shared request is the same sub-hex for both");
        check(pre[0].opposing[0] || pre[1].opposing[0],
              "pre-fix: the shared sub-hex is in one truck's opposing arm lane");

        // the fix
        check(postHit < 0, "post-fix: the two trucks never ask for the same sub-hex");
        check(post[0].n == TICKS && post[1].n == TICKS, "post-fix: both trucks keep moving through the turn");
        for (int i = 0; i < 2; ++i)
            for (int t = 0; t < post[i].n; ++t)
                check(!post[i].opposing[t], "post-fix: every requested step stays out of the opposing arm lane");
    }
    check(g_recovery == 0, "every recorded step comes from the step arithmetic, not a recovery path");

    // WinAstra's arithmetic witness, exactly as reported
    {
        Trace pre[2];
        int hit = -1;
        RunBend(g_bends[0], 63 & ~16, pre, hit);
        check(pre[0].req[0] == CSubHex(22, 22) && pre[1].req[0] == CSubHex(22, 22),
              "witness: both trucks ask for 22,22 at the 10,10-11,10-11,11 bend");
    }

    // the probe fires on a corrected turn and names the step it replaced
    {
        Trace post[2];
        int hit = -1;
        g_roadTurns = 0;
        g_lastTurn[0] = '\0';
        RunBend(g_bends[0], 63, post, hit);
        check(g_roadTurns >= 2, "probe: a road-turn correction is logged for each truck");
        check(std::strstr(g_lastTurn, "[ROAD-TURN]") != nullptr, "probe: the line is tagged ROAD-TURN");
        check(std::strstr(g_lastTurn, "step ") != nullptr, "probe: the line carries the step before and after");
        std::printf("probe %s\n", g_lastTurn);
    }

    // controls: unchanged where the correction must not reach
    const char *why[] = {"off pavement", "on a bridge deck", "where MustKeepLane governs",
                         "within three sub-hexes of the destination", "while reversing"};
    int tweaks[] = {plainGround, bridgeDeck, corridor, nearDest, backing};
    for (int k = 0; k < 5; ++k)
        for (int i = 0; i < 2; ++i) {
            CSubHex off = OneStep(g_bends[0], g_bends[0].t[i], 63 & ~16, tweaks[k]);
            CSubHex on = OneStep(g_bends[0], g_bends[0].t[i], 63, tweaks[k]);
            char msg[128];
            std::snprintf(msg, sizeof(msg), "control: step is unchanged %s", why[k]);
            check(off == on, msg);
        }

    // control: a turn onto a squarely adjacent arm is takeable from here
    {
        CSubHex off = OneStep(g_square, g_square.t[0], 63 & ~16, 0);
        CSubHex on = OneStep(g_square, g_square.t[0], 63, 0);
        check(off == on, "control: a turn onto a squarely adjacent arm is unchanged");
        check(on == CSubHex(21, 20), "control: that turn is taken now, not deferred past the arm");
    }

    // controls: straight through a crossroads, and a straight truck while the
    // other one turns, are both untouched
    for (int i = 0; i < 2; ++i) {
        CSubHex off = OneStep(g_cross, g_cross.t[i], 63 & ~16, 0);
        CSubHex on = OneStep(g_cross, g_cross.t[i], 63, 0);
        check(off == on, "control: a straight step through a junction is unchanged");
        check(off.y == g_cross.t[i].head[1] && off.x != g_cross.t[i].head[0],
              "control: the straight step stays on its own axis and its own lane");
    }
    {
        // same crossroads, but the east-bound truck is now the one turning south
        TruckPlan turner = g_cross.t[0];
        turner.route[0][0] = 11; turner.route[0][1] = 11;
        turner.dest[0] = 22; turner.dest[1] = 27;
        CSubHex turnOff = OneStep(g_cross, turner, 63 & ~16, 0);
        CSubHex turnOn = OneStep(g_cross, turner, 63, 0);
        CSubHex straightOff = OneStep(g_cross, g_cross.t[1], 63 & ~16, 0);
        CSubHex straightOn = OneStep(g_cross, g_cross.t[1], 63, 0);
        check(turnOff != turnOn, "turning-versus-straight: the turning truck is corrected");
        check(straightOff == straightOn, "turning-versus-straight: the straight truck is not");
    }

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
