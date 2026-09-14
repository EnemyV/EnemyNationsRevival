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
#include <string>

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
// vehicle.h:50,91 - only reached inside WaitForMover's reversing-retreat branch,
// which none of this fixture's scenes take (both trucks here are forward-moving),
// so the value is never exercised; it exists to let the extracted body compile.
const int JAM_WINDOW_FRAMES = 24 * 30;
const DWORD TRAFFIC_WAIT_MOVER = 24;

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
// Off by default (no hex matches (-1,-1), and CanEnterHex stays TRUE) so every
// existing fixture is untouched; the arrival-approach-diagonal case below is
// the only one that arms them, and resets them immediately after.
CHexCoord g_bldgHex(-1, -1);
BOOL g_bldgAngleOK = TRUE;
// A truck by default, so every existing fixture is untouched; the [ROAD-DIAG]
// probe fixture is the only one that clears it, and restores it immediately.
BOOL g_isTransport = TRUE;
struct CTransportData {
    enum { FL1hex = 1 };
    BOOL IsBoat() const { return FALSE; }
    BOOL IsTransport() const { return g_isTransport; }
    BOOL IsCrane() const { return FALSE; }
    int GetVehFlags() const { return 0; }
    int GetWheelType() const { return 1; }
    BOOL CanEnterHex(CHexCoord, CHexCoord, BOOL, BOOL) const { return g_bldgAngleOK; }
};
struct Game {
    BOOL IsNetGame() const { return FALSE; }
    DWORD GettimeGetTime() const { return 100000; }
} theGame;
struct CBuilding {};
struct BuildingMap {
    CBuilding *_GetBuilding(CHexCoord h) {
        static CBuilding b;
        return (h == g_bldgHex) ? &b : nullptr;
    }
} theBuildingHex;

int MyRand() { return 0; }
int RandNum(int iMax) { return iMax / 2; }

int g_traffic = 63;
int TrafficOpts() { return g_traffic; }

// any recovery path taken means the recorded step did NOT come from the step
// arithmetic under test, so the scenes must never reach one
int g_recovery = 0;
int g_roadTurns = 0;
int g_roadExempt = 0;
int g_roadDiag = 0;
char g_lastTurn[256] = "";
char g_lastExempt[256] = "";
char g_lastDiag[256] = "";
// WaitForMover/ResumeWaitedStep probes: the occupied-T cases assert on these so a
// passing "no overlap" result is known to come from the real [WAIT]/[YIELD]/
// [RESUME] decision path, not from a scene that happened not to exercise it.
int g_waitLogged = 0, g_yieldLogged = 0, g_resumeLogged = 0;
char g_lastWait[256] = "";
char g_lastYield[256] = "";
char g_lastResume[256] = "";
void WaitLog(const char *fmt, ...) {
    char line[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(line, sizeof(line), fmt, va);
    va_end(va);
    if (std::strstr(line, "[ROAD-TURN]") != nullptr) {
        ++g_roadTurns;
        if (g_lastTurn[0] == 0)
            std::snprintf(g_lastTurn, sizeof(g_lastTurn), "%s", line);
    }
    if (std::strstr(line, "[ROAD-TURN-EXEMPT]") != nullptr) {
        ++g_roadExempt;
        if (g_lastExempt[0] == 0)
            std::snprintf(g_lastExempt, sizeof(g_lastExempt), "%s", line);
    }
    if (std::strstr(line, "[ROAD-DIAG]") != nullptr) {
        ++g_roadDiag;
        std::snprintf(g_lastDiag, sizeof(g_lastDiag), "%s", line);
    }
    if (std::strstr(line, "[WAIT]") != nullptr) {
        ++g_waitLogged;
        std::snprintf(g_lastWait, sizeof(g_lastWait), "%s", line);
    }
    if (std::strstr(line, "[YIELD]") != nullptr) {
        ++g_yieldLogged;
        std::snprintf(g_lastYield, sizeof(g_lastYield), "%s", line);
    }
    if (std::strstr(line, "[RESUME]") != nullptr) {
        ++g_resumeLogged;
        std::snprintf(g_lastResume, sizeof(g_lastResume), "%s", line);
    }
}
// Always on here: the harness's own WaitLog stub above has no EN_WAIT_LOG file
// to agree with, so this fixture exercises the guarded blocks unconditionally
// (production gates them on the real WaitLogEnabled(), checked separately by
// the fixture lint in main()).
BOOL WaitLogEnabled() { return TRUE; }

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
    // vehicle.inl's CVehicleHex::GrabHex also does AddSubOwned bookkeeping and ORs a
    // per-sub-hex occupancy bit into the CHex; neither is read by anything the
    // extracted ResumeWaitedStep or GetNextHex bodies check, so grabbing here is
    // just marking occupancy - the same thing Set() already does everywhere else
    // in this fixture.
    void GrabHex(CSubHex s, CVehicle *v) { Set(s, v); }
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
    // vehicle.h:725-745 - state the extracted WaitForMover/ResumeWaitedStep bodies
    // read and write. x<0 is "not holding a step", matching production's own
    // uninitialised-to-negative convention (ResumeWaitedStep's own guard checks it).
    BOOL m_bWaitedForMover = FALSE;
    CSubHex m_subWaitNext{-1, -1};
    DWORD m_dwTimeBlocked = 0, m_dwTrafficWait = 0;
    int m_iJamClear = 0, m_iJamFwd = 0;

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
    // vehicle.h:357, verbatim: on the move if actually moving, or holding a step
    // in traffic mode (the state WaitForMover puts a waiting vehicle into).
    BOOL IsOnTheMove() const { return (m_cMode == moving) || (m_cMode == traffic); }
    // AskToMove is production's "ask a STOPPED blocker to get going" call.
    // WaitForMover only reaches it when the blocker is NOT IsOnTheMove(), and every
    // occupied-T scene here uses a moving bar truck as the blocker, so this stub is
    // never exercised - it exists only so the extracted WaitForMover body compiles.
    BOOL AskToMove(CVehicle *) { return TRUE; }
    void SetMoveParams(BOOL) {}
    // SetHexDest's real body (vehmove.cpp:1658) is arrival-mode bookkeeping keyed
    // off m_iDestMode, which this fixture's CVehicle does not model (no scene here
    // calls ResumeWaitedStep within 1 hex of m_hexDest). No-op, matching how
    // CheckNextHex is already stubbed above for the same reason.
    void SetHexDest() {}
    BOOL WaitForMover();
    BOOL ResumeWaitedStep();
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
        std::fprintf(stderr, "FAIL: check %d: %s\n", checks, msg);
    }
}

// Fixture lint, not a behaviour test: the runtime checks above prove the
// [ROAD-DIAG] block still produces the right diagnosis, but this fixture's own
// WaitLogEnabled() stub returns TRUE unconditionally, so no runtime check here
// can tell "gated" from "ungated" - a stub that always says yes exercises the
// block either way. Reading the extracted production source text directly and
// requiring the WaitLogEnabled() guard to be the thing that opens the block's
// scope is what actually proves the diagnostic work - not just WaitLog's own
// output - is skipped when EN_WAIT_LOG is unset.
void checkRoadDiagGuarded() {
    std::FILE *f = std::fopen("junction_actual.inc", "rb");
    check(f != nullptr, "fixture lint: junction_actual.inc readable");
    if (f == nullptr)
        return;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string text(static_cast<size_t>(sz), '\0');
    size_t got = std::fread(&text[0], 1, static_cast<size_t>(sz), f);
    std::fclose(f);
    text.resize(got);

    // Anchor on the block's first statement (unique text) and require nothing
    // but the guard and its own opening brace between it and that statement.
    const char *anchorText = "int dxOut = CSubHex::Diff(m_ptNext.x - m_ptHead.x);";
    const char *guardText = "if (WaitLogEnabled())";
    size_t anchor = text.find(anchorText);
    check(anchor != std::string::npos, "fixture lint: [ROAD-DIAG] block found in extracted source");
    if (anchor == std::string::npos)
        return;
    size_t guard = text.rfind(guardText, anchor);
    bool guarded = false;
    if (guard != std::string::npos) {
        size_t brace = text.find('{', guard + std::strlen(guardText));
        guarded = (brace != std::string::npos) && (brace < anchor) &&
                  (text.find_first_not_of(" \t\r\n", brace + 1) == anchor);
    }
    check(guarded, "fixture lint: [ROAD-DIAG] block source begins with the WaitLogEnabled() guard");
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

// Same scene, but the trucks decide and commit IN TURN, and B holds on the
// first tick: the later truck sees the position the earlier one has already
// taken. Records whether anyone asks for an occupied sub-hex.
bool RunStaggered(const BendCase &c, int traffic, CSubHex req[2][TICKS], int &clash);

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

enum Tweak { plainGround = 1, bridgeDeck = 2, corridor = 4, nearDest = 8, backing = 16,
             angledHull = 32 };

CSubHex OneStep(const BendCase &c, const TruckPlan &p, int traffic, int tweak,
                const CSubHex *pDest = nullptr, const CHexCoord *pHexDest = nullptr) {
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
    // head and tail differing on BOTH axes is an angled hull: no lane to carry
    if (tweak & angledHull) {
        if (v.m_ptTail.x == v.m_ptHead.x)
            v.m_ptTail.x = v.m_ptHead.x - 1;
        if (v.m_ptTail.y == v.m_ptHead.y)
            v.m_ptTail.y = v.m_ptHead.y - 1;
        v.m_ptTail.Wrap();
    }
    if (pDest != nullptr)
        v.m_ptDest = *pDest;
    if (pHexDest != nullptr)
        v.m_hexDest = *pHexDest;
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

bool RunStaggered(const BendCase &c, int traffic, CSubHex req[2][TICKS], int &clash) {
    g_traffic = traffic;
    clash = -1;
    ClearMap();
    for (int i = 0; c.paved[i][0] >= 0; ++i)
        theMap.hex[c.paved[i][0]][c.paved[i][1]].type = CHex::road;

    static CVehicle v[2];
    int ri[2] = {0, 0};
    for (int i = 0; i < 2; ++i)
        Build(v[i], c.t[i], i + 1);
    for (int i = 0; i < 2; ++i)
        Occupy(&v[i], &v[i]);

    for (int tick = 0; tick < TICKS; ++tick)
        for (int i = 0; i < 2; ++i) {
            if ((i == 1) && (tick == 0)) {   // B is one tick behind
                req[i][tick] = v[i].m_ptHead;
                continue;
            }
            if (!v[i].GetNextHex(FALSE))
                return false;
            CSubHex want = v[i].m_ptNext;
            req[i][tick] = want;
            CVehicle *on = theVehicleHex._GetVehicle(want);
            if ((on != nullptr) && (on != &v[i]) && (clash < 0))
                clash = tick;
            Occupy(&v[i], nullptr);
            v[i].m_ptTail = v[i].m_ptHead;
            v[i].m_ptHead = want;
            Occupy(&v[i], &v[i]);
            if ((ri[i] + 1 < 3) && v[i].m_hexNext.SameHex(v[i].m_ptHead)) {
                ++ri[i];
                v[i].m_hexNext = CHexCoord(c.t[i].route[ri[i]][0], c.t[i].route[ri[i]][1]);
            }
        }
    return true;
}

// ------------------------------------------ L, T and X: every approach ----
//
// The bends above are WinAstra's witness geometry. This section is the whole
// junction family, built as REAL ORTHOGONAL ROADS - one junction hex plus
// three-hex arms, grass everywhere else - and driven from every approach:
//   L   two arms, all four orientations
//   T   a bar and a stem, from both bar ends and from the stem, turning both ways
//   X   four arms, turning left, right and straight from each of them
// Two hull states for each (in-arm, out-arm) pair, both IN LANE:
//   "along"  - the truck travels down the in-arm towards the junction. This is
//              the approach bodies 1 and 2 already correct, and it must STAY
//              corrected.
//   "across" - the truck stands in the in-arm hex beside the junction with its
//              hull already on the OUTGOING axis, so the sub-hex straight ahead
//              is the unpaved corner between the two arms. On ea59494f the
//              junction block bails out at OnPavement(_ahead) before the corner
//              test is reached, and the raw diagonal - out of this arm into the
//              other one, across that unpaved corner - is taken unexamined.
//              These are the checks that fail against --baseline-ref ea59494f.
// Both lane parities are covered: a lane is fixed by its direction of travel, so
// driving every arm in both directions drives both parities of every arm.

const int JX = 11, JY = 10;                      // the junction hex

struct ArmSet { const char *name; int n; int d[4][2]; };
const ArmSet g_armSets[7] = {
    {"L-WS", 2, {{-1,0},{0, 1},{0,0},{0,0}}},
    {"L-ES", 2, {{ 1,0},{0, 1},{0,0},{0,0}}},
    {"L-WN", 2, {{-1,0},{0,-1},{0,0},{0,0}}},
    {"L-EN", 2, {{ 1,0},{0,-1},{0,0},{0,0}}},
    {"T-stemS", 3, {{-1,0},{1,0},{0, 1},{0,0}}},
    {"T-stemN", 3, {{-1,0},{1,0},{0,-1},{0,0}}},
    {"X",      4, {{-1,0},{1,0},{0,-1},{0,1}}},
};

// -2 the junction hex, -1 not road, otherwise the arm the hex belongs to
int ArmIndex(const ArmSet &S, int hx, int hy) {
    if ((hx == JX) && (hy == JY))
        return -2;
    for (int a = 0; a < S.n; ++a)
        for (int k = 1; k <= 3; ++k)
            if ((hx == JX + S.d[a][0]*k) && (hy == JY + S.d[a][1]*k))
                return a;
    return -1;
}
void PaveShape(const ArmSet &S) {
    ClearMap();
    theMap.hex[JX][JY].type = CHex::road;
    for (int a = 0; a < S.n; ++a)
        for (int k = 1; k <= 3; ++k)
            theMap.hex[JX + S.d[a][0]*k][JY + S.d[a][1]*k].type = CHex::road;
}

const int DRIVE_TICKS = 4;
struct DriveResult {
    bool recovered;     // a step came from a recovery path, not the step arithmetic
    bool offPavement;   // a requested step targets grass
    bool cornerCut;     // a requested diagonal crosses an unpaved corner
    bool opposing;      // a requested step lands in the outgoing arm's wrong lane
    bool revisit;       // the hull returns to a sub-hex it already left
    bool reached;       // the head gets inside an outgoing-arm hex
    bool firstDiagonal; // the step out of the bend is a diagonal
};

DriveResult RunApproach(const ArmSet &S, int in, int out, bool across) {
    DriveResult r = {false, false, false, false, false, false, false};
    PaveShape(S);
    const int *di = S.d[in], *dov = S.d[out];
    int Hx = JX + di[0], Hy = JY + di[1];
    // The hull sits in the in-arm hex beside the junction, IN ITS OWN LANE, at the
    // leading edge of that hex for the way it points. Only the heading differs:
    // "along" points down the arm at the junction, "across" points on the OUTGOING
    // axis - which in the in-arm means pointing at the unpaved corner.
    int tdx = across ? dov[0] : -di[0];
    int tdy = across ? dov[1] : -di[1];
    int hxs, hys;
    if (tdx != 0) { hxs = Hx*2 + ((tdx > 0) ? 1 : 0); hys = Hy*2 + ((tdx > 0) ? 1 : 0); }
    else          { hys = Hy*2 + ((tdy > 0) ? 1 : 0); hxs = Hx*2 + ((tdy > 0) ? 0 : 1); }
    int Fx = JX + dov[0]*3, Fy = JY + dov[1]*3;
    int destX = Fx*2 + ((dov[0] != 0) ? ((dov[0] > 0) ? 1 : 0) : ((dov[1] > 0) ? 0 : 1));
    int destY = Fy*2 + ((dov[1] != 0) ? ((dov[1] > 0) ? 1 : 0) : ((dov[0] > 0) ? 1 : 0));

    static CVehicle v;
    v = CVehicle();
    v.id = 70;
    v.m_ptHead = CSubHex(hxs, hys);
    v.m_ptTail = CSubHex(hxs - tdx, hys - tdy);
    v.m_ptNext = v.m_ptHead;
    v.m_ptDest = CSubHex(destX, destY);
    v.m_hexDest = CHexCoord(destX >> 1, destY >> 1);
    v.m_hexNext = CHexCoord(JX + dov[0], JY + dov[1]);
    v.m_cMode = CVehicle::moving;
    theVehicleHex.Set(v.m_ptHead, &v);
    theVehicleHex.Set(v.m_ptTail, &v);

    CSubHex seen[DRIVE_TICKS + 2];
    int nSeen = 0;
    seen[nSeen++] = v.m_ptHead;
    int ri = 0;
    g_recovery = 0;
    g_traffic = 63;
    for (int t = 0; t < DRIVE_TICKS; ++t) {
        if (!v.GetNextHex(FALSE)) { r.recovered = true; break; }
        CSubHex n = v.m_ptNext;
        if (n == v.m_ptHead)
            break;
        int sx = CSubHex::Diff(n.x - v.m_ptHead.x), sy = CSubHex::Diff(n.y - v.m_ptHead.y);
        if (!v.OnPavement(n))
            r.offPavement = true;
        if ((sx != 0) && (sy != 0)) {
            CSubHex _sideX(v.m_ptHead.x + sx, v.m_ptHead.y), _sideY(v.m_ptHead.x, v.m_ptHead.y + sy);
            _sideX.Wrap(); _sideY.Wrap();
            if ((!v.OnPavement(_sideX)) || (!v.OnPavement(_sideY)))
                r.cornerCut = true;
            if (t == 0)
                r.firstDiagonal = true;
        }
        // only the OUTGOING arm carries a lane for this truck: the junction hex is
        // shared by both arms and the in-arm is behind us
        if (ArmIndex(S, n.x >> 1, n.y >> 1) == out) {
            BOOL inLane = (dov[0] != 0) ? ((n.y & 1) == ((dov[0] > 0) ? 1 : 0))
                                        : ((n.x & 1) == ((dov[1] > 0) ? 0 : 1));
            if (!inLane)
                r.opposing = true;
        }
        for (int k = 0; k < nSeen; ++k)
            if (seen[k] == n)
                r.revisit = true;
        seen[nSeen++] = n;

        theVehicleHex.Set(v.m_ptHead, nullptr);
        theVehicleHex.Set(v.m_ptTail, nullptr);
        v.m_ptTail = v.m_ptHead;
        v.m_ptHead = n;
        v.m_ptNext = n;
        theVehicleHex.Set(v.m_ptHead, &v);
        theVehicleHex.Set(v.m_ptTail, &v);

        if (ArmIndex(S, v.m_ptHead.x >> 1, v.m_ptHead.y >> 1) == out)
            r.reached = true;
        if (v.m_hexNext.SameHex(v.m_ptHead)) {
            if (ri + 1 >= 3)
                break;                       // route exhausted: stop rather than stall
            ++ri;
            v.m_hexNext = CHexCoord(JX + dov[0]*(ri + 1), JY + dov[1]*(ri + 1));
        }
    }
    if (g_recovery != 0)
        r.recovered = true;
    theVehicleHex.Set(v.m_ptHead, nullptr);
    theVehicleHex.Set(v.m_ptTail, nullptr);
    return r;
}

void CheckApproaches() {
    for (int si = 0; si < 7; ++si) {
        const ArmSet &S = g_armSets[si];
        for (int in = 0; in < S.n; ++in)
            for (int out = 0; out < S.n; ++out) {
                if (in == out)
                    continue;
                bool perp = ((S.d[in][0] == 0) != (S.d[out][0] == 0));
                for (int mode = 0; mode < (perp ? 2 : 1); ++mode) {
                    bool across = (mode == 1);
                    DriveResult r = RunApproach(S, in, out, across);
                    char line[200];
                    const char *how = across ? "across" : "along";
                    const char *what[7] = {
                        "every step comes from the step arithmetic",
                        "no requested step targets grass",
                        "no requested step cuts an unpaved corner",
                        "no requested step takes the opposing lane",
                        "the hull never returns to a sub-hex it left",
                        "the truck reaches the outgoing arm",
                        "the step out of the bend is axis-aligned"};
                    bool bad[7] = {r.recovered, r.offPavement, r.cornerCut, r.opposing,
                                   r.revisit, !r.reached, r.firstDiagonal};
                    for (int k = 0; k < (across ? 7 : 6); ++k) {
                        std::snprintf(line, sizeof(line), "%s %s in%d out%d: %s",
                                      S.name, how, in, out, what[k]);
                        check(!bad[k], line);
                    }
                }
            }
    }
}

// ------------------------------------------- the STAIRCASE CORRIDOR ----
//
// Everything above builds arms RADIATING FROM ONE JUNCTION, and every approach
// starts on the LEADING corner sub-hex of its hex. The live map is neither.
// RoadStepToward (vehicle.cpp:1888-1911) advances exactly one axis one hex per
// call, so every road it lays is a 4-CONNECTED STAIRCASE - (x,y) (x+1,y)
// (x+1,y+1) (x+2,y+1) ... - and the path re-diagonalises it, because GetCellAt
// expands the diagonal neighbours and GetCellCosts prices only the destination
// hex, never the two corners a diagonal passes between. A truck walking a
// staircase is therefore handed a DIAGONALLY ADJACENT m_hexNext over and over.
//
// That makes the TRAILING corner sub-hex - the one furthest from the route hex
// on BOTH axes - a routine place to stand, and it is the hole:
//   tick 1  xDif == yDif == 2, so the far-target split zeroes NEITHER axis and
//           the clamp hands back a diagonal that stays INSIDE the truck's own
//           hex. _ahead is that hex too, so body 1's same-hex guard declines it,
//           body 2 wants a single-axis route and body 3 wants a different hex
//           ahead. No body examines it; [ROAD-DIAG] cannot see it either,
//           because both axis neighbours of an intra-hex diagonal are that same
//           paved hex.
//   tick 2  the head is on the leading corner with a DIAGONAL HULL, so the whole
//           block is shut out by the angle guard, and the raw diagonal crosses
//           the SHARED CORNER of two road hexes - the corner ride QA reports -
//           instead of walking through the paved corner hex between them.
// Measured over a 1,247 s capture: 98.5% of all 20,381 angled-hull events, on
// 100% of the transports that appear in [ROAD-DIAG] at all.
//
// Driven here in all four staircase directions and both chain phases (x step
// first / y step first). A lane is fixed by the direction of travel, so the four
// directions cover both sub-hex parities of both axes. In each the chain is
// scanned for the first hex whose incoming hull axis makes the trailing corner
// IN LANE - exactly one of the two bend types on a given staircase can be, since
// the convention is "+x rides odd y, +y rides even x" and the staircase's own
// two axis directions fix which - and the truck is placed there.
//
//   "pre"  runs with TrafficOpts bit 16 clear: the junction block is off, which
//          is the geometry as it stood before any of it existed. It records the
//          defect and must keep recording it.
//   "post" runs with the block on. Before body 1b it behaves exactly like "pre"
//          (no body examines the step), which is what --baseline-ref proves.

const int STAIR_N = 8;                 // hexes in the corridor
const int STAIR_BX = 20, STAIR_BY = 20;

struct Stair {
    int n;
    CHexCoord h[STAIR_N];
};

// alternate one-axis steps, the only shape RoadStepToward can lay
Stair BuildStair(int sx, int sy, bool xFirst) {
    Stair S;
    S.n = STAIR_N;
    S.h[0] = CHexCoord(STAIR_BX, STAIR_BY);
    for (int k = 0; k + 1 < STAIR_N; ++k) {
        bool takeX = xFirst ? ((k % 2) == 0) : ((k % 2) == 1);
        S.h[k + 1] = CHexCoord(S.h[k].X() + (takeX ? sx : 0), S.h[k].Y() + (takeX ? 0 : sy));
    }
    return S;
}
void PaveStair(const Stair &S) {
    ClearMap();
    for (int k = 0; k < S.n; ++k)
        theMap.hex[S.h[k].X()][S.h[k].Y()].type = CHex::road;
}
int StairIndex(const Stair &S, CHexCoord c) {
    for (int k = 0; k < S.n; ++k)
        if (S.h[k] == c)
            return k;
    return -1;
}

struct StairResult {
    bool valid;          // a placement satisfying the geometry was found
    bool recovered;      // a step came from a recovery path, not the step arithmetic
    bool offPavement;    // a requested step targets something unpaved
    bool offCorridor;    // the head leaves the staircase's own hexes
    bool intraFirst;     // tick 1's step is a diagonal that stays in the head's hex
    bool axialFirst;     // tick 1's step is axis-aligned
    bool laneFirst;      // tick 1's step lands in the lane for the hull's direction
    bool cornerRide;     // some step crosses a hex CORNER (both hex axes change)
    bool diagAtCorner;   // the head stands on a shared-corner sub-hex with a diagonal hull
    bool walkedCorner;   // the head enters the corner hex the staircase actually paves
    bool revisit;        // the hull returns to a sub-hex it already left
    bool converged;      // the head reaches the diagonal route hex
    int ticks;
    char why[64];        // the reason keyword of tick 1's correction, if any
};

// One trailing-corner approach on one staircase, driven until it reaches the
// route hex that was diagonally adjacent when it started.
StairResult RunStair(int sx, int sy, bool xFirst, int traffic, bool blockAhead = false) {
    StairResult r;
    std::memset(&r, 0, sizeof(r));
    Stair S = BuildStair(sx, sy, xFirst);
    PaveStair(S);
    g_traffic = traffic;
    g_recovery = 0;

    // The in-lane trailing corner exists on exactly one of the staircase's two
    // bend types. Find it rather than assert which: pick the first hex i with a
    // predecessor and two successors whose trailing corner is in lane for the
    // hull it arrives with.
    int i = -1, hx = 0, hy = 0, hxs = 0, hys = 0;
    for (int k = 1; k + 2 < S.n; ++k) {
        int dx = S.h[k].X() - S.h[k - 1].X(), dy = S.h[k].Y() - S.h[k - 1].Y();
        int cx = S.h[k + 2].X() - S.h[k].X(), cy = S.h[k + 2].Y() - S.h[k].Y();
        if ((cx == 0) || (cy == 0))
            continue;                                  // not a diagonal route hex
        // the trailing corner: furthest from the route hex on BOTH axes
        int px = S.h[k].X() * 2 + ((cx > 0) ? 0 : 1);
        int py = S.h[k].Y() * 2 + ((cy > 0) ? 0 : 1);
        BOOL inLane = (dx != 0) ? ((py & 1) == ((dx > 0) ? 1 : 0))
                                : ((px & 1) == ((dy > 0) ? 0 : 1));
        if (!inLane)
            continue;
        i = k; hx = dx; hy = dy; hxs = px; hys = py;
        break;
    }
    if (i < 0)
        return r;                                      // r.valid stays false
    r.valid = true;

    static CVehicle v;
    v = CVehicle();
    v.id = 80;
    v.m_ptHead = CSubHex(hxs, hys);
    v.m_ptTail = CSubHex(hxs - hx, hys - hy);
    v.m_ptNext = v.m_ptHead;
    // the far end of the corridor: never within the near-destination relaxation
    v.m_ptDest = CSubHex(S.h[S.n - 1].X() * 2, S.h[S.n - 1].Y() * 2);
    v.m_hexDest = S.h[S.n - 1];
    v.m_hexNext = S.h[i + 2];
    v.m_cMode = CVehicle::moving;
    theVehicleHex.Set(v.m_ptHead, &v);
    theVehicleHex.Set(v.m_ptTail, &v);

    // a parked truck on the deferral target - the leading corner of this hex
    static CVehicle blocker;
    if (blockAhead) {
        blocker = CVehicle();
        blocker.id = 81;
        blocker.m_ptHead = CSubHex(hxs + hx, hys + hy);
        blocker.m_ptTail = CSubHex(hxs + hx * 2, hys + hy * 2);
        blocker.m_cMode = CVehicle::stop;
        theVehicleHex.Set(blocker.m_ptHead, &blocker);
        theVehicleHex.Set(blocker.m_ptTail, &blocker);
    }

    const CHexCoord hexTarget = S.h[i + 2];            // the diagonal route hex
    const CHexCoord hexCorner = S.h[i + 1];            // the paved corner hex between
    CSubHex seen[12];
    int nSeen = 0;
    seen[nSeen++] = v.m_ptHead;
    g_lastTurn[0] = 0;

    for (int t = 0; t < 8; ++t) {
        // the pathfinder on a staircase: take the diagonal hop when there is one,
        // which is what GetCellCosts' destination-only pricing always prefers
        int k = StairIndex(S, CHexCoord(v.m_ptHead));
        if (k < 0) { r.offCorridor = true; break; }
        if (k + 1 >= S.n)
            break;
        int j = k + 1;
        if ((k + 2 < S.n) && (S.h[k + 2].X() != S.h[k].X()) && (S.h[k + 2].Y() != S.h[k].Y()))
            j = k + 2;
        v.m_hexNext = S.h[j];

        if (!v.GetNextHex(FALSE)) { r.recovered = true; break; }
        CSubHex n = v.m_ptNext;
        if (n == v.m_ptHead)
            break;
        int stx = CSubHex::Diff(n.x - v.m_ptHead.x), sty = CSubHex::Diff(n.y - v.m_ptHead.y);
        if (!v.OnPavement(n))
            r.offPavement = true;
        if ((stx != 0) && (sty != 0) && (!n.SameHex(v.m_ptHead)) &&
            ((n.x >> 1) != (v.m_ptHead.x >> 1)) && ((n.y >> 1) != (v.m_ptHead.y >> 1)))
            r.cornerRide = true;                       // crossed the shared hex corner
        if (t == 0) {
            r.intraFirst = (stx != 0) && (sty != 0) && n.SameHex(v.m_ptHead);
            r.axialFirst = (stx == 0) || (sty == 0);
            r.laneFirst = (hx != 0) ? ((n.y & 1) == ((hx > 0) ? 1 : 0))
                                    : ((n.x & 1) == ((hy > 0) ? 0 : 1));
            std::snprintf(r.why, sizeof(r.why), "%s",
                          (std::strstr(g_lastTurn, "reason ") != nullptr)
                              ? std::strstr(g_lastTurn, "reason ") + 7 : "-");
            for (char *p = r.why; *p; ++p)
                if (*p == ' ') { *p = 0; break; }
        }
        for (int q = 0; q < nSeen; ++q)
            if (seen[q] == n)
                r.revisit = true;
        if (nSeen < 12)
            seen[nSeen++] = n;

        theVehicleHex.Set(v.m_ptHead, nullptr);
        theVehicleHex.Set(v.m_ptTail, nullptr);
        v.m_ptTail = v.m_ptHead;
        v.m_ptHead = n;
        v.m_ptNext = n;
        theVehicleHex.Set(v.m_ptHead, &v);
        theVehicleHex.Set(v.m_ptTail, &v);
        r.ticks = t + 1;

        // standing on a sub-hex that is a corner shared with an UNPAVED hex,
        // carrying a diagonal hull - the state the angle guard can never correct
        int uhx = CSubHex::Diff(v.m_ptHead.x - v.m_ptTail.x);
        int uhy = CSubHex::Diff(v.m_ptHead.y - v.m_ptTail.y);
        if ((uhx != 0) && (uhy != 0)) {
            CSubHex _sideX(v.m_ptHead.x + uhx, v.m_ptHead.y), _sideY(v.m_ptHead.x, v.m_ptHead.y + uhy);
            _sideX.Wrap(); _sideY.Wrap();
            if ((!v.OnPavement(_sideX)) || (!v.OnPavement(_sideY)))
                r.diagAtCorner = true;
        }
        if (CHexCoord(v.m_ptHead) == hexCorner)
            r.walkedCorner = true;
        if (CHexCoord(v.m_ptHead) == hexTarget) {
            r.converged = true;
            break;
        }
    }
    if (g_recovery != 0)
        r.recovered = true;
    theVehicleHex.Set(v.m_ptHead, nullptr);
    theVehicleHex.Set(v.m_ptTail, nullptr);
    return r;
}

void CheckStaircase() {
    for (int d = 0; d < 4; ++d) {
        int sx = (d & 1) ? -1 : 1, sy = (d & 2) ? -1 : 1;
        for (int ph = 0; ph < 2; ++ph) {
            bool xFirst = (ph == 0);
            char tag[48], line[220];
            std::snprintf(tag, sizeof(tag), "staircase %+d,%+d %s", sx, sy, xFirst ? "x-first" : "y-first");

            StairResult pre = RunStair(sx, sy, xFirst, 63 & ~16);
            StairResult post = RunStair(sx, sy, xFirst, 63);

            std::snprintf(line, sizeof(line), "%s: a trailing-corner approach exists on this corridor", tag);
            check(pre.valid && post.valid, line);
            if (!pre.valid || !post.valid)
                continue;

            // the defect, with the junction block off: the tick-1 step is the
            // intra-hex diagonal, and the corner is ridden on the tick after.
            std::snprintf(line, sizeof(line), "%s pre: tick 1 is a diagonal that stays inside the hex", tag);
            check(pre.intraFirst, line);
            std::snprintf(line, sizeof(line), "%s pre: tick 2 rides the shared corner of two road hexes", tag);
            check(pre.cornerRide, line);
            std::snprintf(line, sizeof(line), "%s pre: the hull stands on a shared corner sub-hex angled", tag);
            check(pre.diagAtCorner, line);
            std::snprintf(line, sizeof(line), "%s pre: the paved corner hex is never entered", tag);
            check(!pre.walkedCorner, line);

            // the fix: tick 1 is axial and in lane, and the corner hex is walked
            std::snprintf(line, sizeof(line), "%s post: tick 1 is axis-aligned, not the intra-hex diagonal", tag);
            check(post.axialFirst && !post.intraFirst, line);
            std::snprintf(line, sizeof(line), "%s post: tick 1 stays in the lane for the hull's direction", tag);
            check(post.laneFirst, line);
            std::snprintf(line, sizeof(line), "%s post: tick 1's correction is named corner-deferred-intra", tag);
            check(std::strcmp(post.why, "corner-deferred-intra") == 0, line);
            std::snprintf(line, sizeof(line), "%s post: no step crosses the shared corner of two road hexes", tag);
            check(!post.cornerRide, line);
            std::snprintf(line, sizeof(line), "%s post: never angled on a sub-hex cornering unpaved ground", tag);
            check(!post.diagAtCorner, line);
            std::snprintf(line, sizeof(line), "%s post: the truck walks through the paved corner hex", tag);
            check(post.walkedCorner, line);
            std::snprintf(line, sizeof(line), "%s post: no requested step targets grass", tag);
            check(!post.offPavement, line);
            std::snprintf(line, sizeof(line), "%s post: the head never leaves the corridor's own hexes", tag);
            check(!post.offCorridor, line);
            std::snprintf(line, sizeof(line), "%s post: no oscillation - the hull never returns to a sub-hex it left", tag);
            check(!post.revisit, line);
            std::snprintf(line, sizeof(line), "%s post: every step comes from the step arithmetic", tag);
            check(!post.recovered, line);
            std::snprintf(line, sizeof(line), "%s post: the truck reaches the diagonal route hex within 8 ticks", tag);
            check(post.converged, line);

            std::printf("%-28s pre %s/%d  post %s/%d ticks %d\n", tag,
                        pre.cornerRide ? "corner-ride" : "clean", pre.ticks,
                        post.cornerRide ? "corner-ride" : "clean", post.ticks, post.ticks);
        }
    }

    // THE DEFERRAL TARGET OCCUPIED. CanEnter(_ahead) is a term of the new body,
    // so a truck parked on the leading corner of this hex declines it and the
    // step is left exactly as it was - the pre-fix intra-hex diagonal, which is
    // still a legal move into the truck's own paved hex. The new body never
    // turns a move into a wait; it only ever changes which paved sub-hex is
    // asked for.
    for (int d = 0; d < 4; ++d) {
        int sx = (d & 1) ? -1 : 1, sy = (d & 2) ? -1 : 1;
        char line[220];
        StairResult blocked = RunStair(sx, sy, true, 63, true);
        std::snprintf(line, sizeof(line),
                      "staircase %+d,%+d occupied: the approach is still the trailing-corner one", sx, sy);
        check(blocked.valid, line);
        if (!blocked.valid)
            continue;
        std::snprintf(line, sizeof(line),
                      "staircase %+d,%+d occupied: an occupied deferral target leaves the step alone", sx, sy);
        check(blocked.intraFirst, line);
        std::snprintf(line, sizeof(line),
                      "staircase %+d,%+d occupied: declining the deferral is not a blocked/FindSub recovery", sx, sy);
        check(!blocked.recovered, line);
        std::snprintf(line, sizeof(line),
                      "staircase %+d,%+d occupied: the untouched step still targets pavement", sx, sy);
        check(!blocked.offPavement, line);
    }
}

// ------------------------------------------------- occupied-T (WinAstra) ----
//
// WinAstra's 2026-09-13 review of bbcc2327: the corner-cut fix's body-3
// "bend-turn" step (vehmove.cpp:1398-1411) puts a turning truck into the
// sub-hex the T's two arms SHARE - and that shared hex also carries the bar's
// through lane. RunApproach's single-truck drive cannot see this: it only
// checks the OUTGOING arm's lane parity, and the shared hex "belongs to
// neither arm" by design (LaneVerdict, ArmIndex). These cases put a real bar
// truck in that hex and check the STEM truck's actual occupancy/wait response,
// via the REAL WaitForMover/ResumeWaitedStep bodies extracted above - not the
// old CanEnter-occupancy-only model, which had no wait/resume path at all
// (WaitForMover was a stub that always returned FALSE and flagged a recovery).
//
// Geometry is WinAstra's own witness, made three-way: T-stemS at JX,JY (11,10)
// - west arm (10,10)(9,10)(8,10), east arm (12,10)(13,10)(14,10), stem south
// arm (11,11)(11,12)(11,13). The stem truck (head 23,22 tail 23,23, hexNext
// 10,10 - the west arm's near hex, reached only diagonally from here) is
// g_bends[0].t[1] "B S>W": already proven above to take the bend-turn step to
// (23,21) - inside the junction hex, on sub-row y=21, the EASTBOUND bar's lane
// row (LaneVerdict: +X rides odd y). That is exactly the sub-hex an eastbound
// bar truck passing through the T would occupy or ask for.
const int STEM_HX = 23, STEM_HY = 22, STEM_TX = 23, STEM_TY = 23;
const int CONTESTED_X = 23, CONTESTED_Y = 21;

void PaveOccupiedT() {
    ClearMap();
    theMap.hex[11][10].type = CHex::road;                                                     // junction
    theMap.hex[10][10].type = theMap.hex[9][10].type  = theMap.hex[8][10].type  = CHex::road;  // west arm
    theMap.hex[12][10].type = theMap.hex[13][10].type = theMap.hex[14][10].type = CHex::road;  // east arm
    theMap.hex[11][11].type = theMap.hex[11][12].type = theMap.hex[11][13].type = CHex::road;  // stem
}

void BuildStem(CVehicle &s, int id) {
    s = CVehicle();
    s.id = id;
    s.m_ptHead = CSubHex(STEM_HX, STEM_HY);
    s.m_ptTail = CSubHex(STEM_TX, STEM_TY);
    s.m_ptNext = s.m_ptHead;
    s.m_ptDest = CSubHex(17, 20);
    s.m_hexDest = CHexCoord(8, 10);
    s.m_hexNext = CHexCoord(10, 10);
    s.m_cMode = CVehicle::moving;
}

// Drive the stem truck up to 4 ticks after a resume, and require: it reaches
// the west arm, it never revisits a sub-hex it already left (no oscillation),
// and once inside the west arm's own hex it stays in that arm's lane (-X rides
// even y - LaneVerdict's convention, same as every other case in this file).
void DriveStemToWestArm(CVehicle &stem) {
    CSubHex seen[6];
    int nSeen = 0;
    seen[nSeen++] = stem.m_ptHead;
    bool reachedWestArm = false, revisited = false, offLane = false;
    int ri = 0;
    const int route[3][2] = {{10, 10}, {9, 10}, {8, 10}};
    g_recovery = 0;
    for (int t = 0; t < 4; ++t) {
        BOOL step = stem.GetNextHex(FALSE);
        check(step != FALSE, "occupied-T: after resuming the stem truck keeps producing a step every tick");
        if (!step)
            break;
        CSubHex n = stem.m_ptNext;
        for (int k = 0; k < nSeen; ++k)
            if (seen[k] == n)
                revisited = true;
        seen[nSeen++] = n;
        if (((n.x >> 1) == 10) && ((n.y >> 1) == 10) && ((n.y & 1) != 0))
            offLane = true;               // west/-X rides EVEN y
        Occupy(&stem, nullptr);
        stem.m_ptTail = stem.m_ptHead;
        stem.m_ptHead = n;
        Occupy(&stem, &stem);
        if ((stem.m_ptHead.x >> 1) <= 10)
            reachedWestArm = true;
        if (stem.m_hexNext.SameHex(stem.m_ptHead) && (ri + 1 < 3)) {
            ++ri;
            stem.m_hexNext = CHexCoord(route[ri][0], route[ri][1]);
        }
    }
    check(reachedWestArm, "occupied-T: within 4 ticks of release the stem truck reaches the west arm");
    check(!revisited, "occupied-T: no oscillation - the hull never returns to a sub-hex it left");
    check(!offLane, "occupied-T: progress into the west arm stays in its lane (-X rides even y)");
    check(g_recovery == 0, "occupied-T: the post-release progress is step arithmetic, not a recovery path");
}

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

    // WinAstra's round-4 witness: the SAME bend, but both destinations now sit
    // inside the three-sub-hex neighbourhood that used to switch the correction
    // off wholesale. A(22,24) is one hex past the turn, B(20,20) is the first hex
    // of the arm it is turning into. Neither step is the arrival move, so lane
    // guidance must still apply and the pair must still be separated.
    {
        BendCase c = g_bends[0];
        c.t[0].dest[0] = 22; c.t[0].dest[1] = 24;
        c.t[1].dest[0] = 20; c.t[1].dest[1] = 20;
        Trace tr[2];
        int hit = -1;
        g_recovery = 0;
        RunBend(c, 63, tr, hit);
        std::printf("near-dest hit=%d A=%d,%d B=%d,%d\n", hit, tr[0].req[0].x, tr[0].req[0].y,
                    tr[1].req[0].x, tr[1].req[0].y);
        check(hit < 0, "near-dest witness: the two trucks never ask for the same sub-hex");
        check(!tr[0].opposing[0] && !tr[1].opposing[0],
              "near-dest witness: neither first step lands in the opposing arm lane");
        check(g_recovery == 0, "near-dest witness: every step comes from the step arithmetic");
    }

    // the same witness with an east arm added, so the junction is a three-way.
    // The far-destination control must keep clearing, and the near pair must be
    // separated as well.
    {
        BendCase c = g_bends[0];
        c.paved[7][0] = 12; c.paved[7][1] = 10;
        c.paved[8][0] = -1; c.paved[8][1] = -1;
        Trace farTr[2];
        int farHit = -1;
        RunBend(c, 63, farTr, farHit);
        check(farHit < 0, "three-way: the far-destination pair is still separated");
        c.t[0].dest[0] = 22; c.t[0].dest[1] = 24;
        c.t[1].dest[0] = 20; c.t[1].dest[1] = 20;
        Trace tr[2];
        int hit = -1;
        g_recovery = 0;
        RunBend(c, 63, tr, hit);
        std::printf("three-way near-dest hit=%d A=%d,%d B=%d,%d; far hit=%d\n", hit,
                    tr[0].req[0].x, tr[0].req[0].y, tr[1].req[0].x, tr[1].req[0].y, farHit);
        check(hit < 0, "three-way near-dest witness: the two trucks never ask for the same sub-hex");
        check(!tr[0].opposing[0] && !tr[1].opposing[0],
              "three-way near-dest witness: neither first step lands in the opposing arm lane");
        check(g_recovery == 0, "three-way near-dest witness: every step comes from the step arithmetic");
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
        check((std::strstr(g_lastTurn, "head ") != nullptr) &&
                  (std::strstr(g_lastTurn, "tail ") != nullptr) &&
                  (std::strstr(g_lastTurn, "next ") != nullptr) &&
                  (std::strstr(g_lastTurn, "dest ") != nullptr) &&
                  (std::strstr(g_lastTurn, "reason ") != nullptr),
              "probe: the corrected line carries head, tail, next, dest and a reason");
        std::printf("probe %s\n", g_lastTurn);
    }

    // controls: unchanged where the correction must not reach
    const char *why[] = {"off pavement", "on a bridge deck", "where MustKeepLane governs",
                         "while reversing", "with an angled hull"};
    int tweaks[] = {plainGround, bridgeDeck, corridor, backing, angledHull};
    for (int k = 0; k < 5; ++k)
        for (int i = 0; i < 2; ++i) {
            CSubHex off = OneStep(g_bends[0], g_bends[0].t[i], 63 & ~16, tweaks[k]);
            CSubHex on = OneStep(g_bends[0], g_bends[0].t[i], 63, tweaks[k]);
            char msg[128];
            std::snprintf(msg, sizeof(msg), "control: step is unchanged %s", why[k]);
            check(off == on, msg);
        }

    // An angled hull has no lane to carry, near the destination as anywhere else.
    for (int i = 0; i < 2; ++i) {
        CSubHex off = OneStep(g_bends[0], g_bends[0].t[i], 63 & ~16, angledHull | nearDest);
        CSubHex on = OneStep(g_bends[0], g_bends[0].t[i], 63, angledHull | nearDest);
        check(off == on, "control: an angled hull near the destination is unchanged");
    }

    // The approach is now guided even when the destination is within three
    // sub-hexes, because that step is not the arrival move. This check is the
    // inverse of the one it replaces: asserting that this step is unguided IS the
    // defect WinAstra reported.
    for (int i = 0; i < 2; ++i) {
        CSubHex off = OneStep(g_bends[0], g_bends[0].t[i], 63 & ~16, nearDest);
        CSubHex on = OneStep(g_bends[0], g_bends[0].t[i], 63, nearDest);
        check(off != on, "near destination but not arriving: the approach is still guided");
        check(on != CSubHex(22, 22), "near destination but not arriving: the corner sub-hex is not taken");
    }

    // ...and the arrival move itself keeps its freedom. A: the corner step IS the
    // step that enters the destination hex - a building's entry diagonal.
    {
        CSubHex destA(23, 23);
        CHexCoord hexA(11, 11);
        g_roadExempt = 0;
        g_lastExempt[0] = '\0';
        CSubHex offA = OneStep(g_bends[0], g_bends[0].t[0], 63 & ~16, 0, &destA, &hexA);
        CSubHex onA = OneStep(g_bends[0], g_bends[0].t[0], 63, 0, &destA, &hexA);
        // the exempted case is named, not silent: QA greps this line for a cut
        check(g_roadExempt == 1, "probe: the exempted arrival step is logged once");
        check(std::strstr(g_lastExempt, "corner-cut") != nullptr,
              "probe: the exempt line names the corner-cut geometry");
        check((std::strstr(g_lastExempt, "head ") != nullptr) &&
                  (std::strstr(g_lastExempt, "tail ") != nullptr) &&
                  (std::strstr(g_lastExempt, "next ") != nullptr) &&
                  (std::strstr(g_lastExempt, "dest ") != nullptr),
              "probe: the exempt line carries head, tail, next and dest");
        std::printf("exempt %s\n", g_lastExempt);
        check(offA == onA, "control: the step that enters the destination hex is unchanged");
        check(onA == CSubHex(22, 22), "control: that entry step is still the direct diagonal");

        // B used to be "a truck already inside the destination hex" with a
        // distant m_ptDest (17,20 while m_hexDest was 11,11) - a state
        // SetHexDest never produces: m_hexDest is always ToCoord(m_ptDest)
        // (unit.cpp SetDestAndMode, vehmove.cpp SetHexDest), and ToCoord is
        // x>>1,y>>1, so a hex's own sub-hexes span only {2H,2H+1} on each
        // axis. The invariant-consistent version of "already inside the
        // destination hex" is this: m_ptDest in the SAME hex the truck
        // occupies is necessarily within 1 sub-hex of m_ptHead on each axis,
        // so the distance clause of bArrival already exempts it - no same-hex
        // clause of its own is needed.
        CSubHex destB(23, 22);
        CHexCoord hexB(11, 11);
        CSubHex offB = OneStep(g_bends[0], g_bends[0].t[1], 63 & ~16, 0, &destB, &hexB);
        CSubHex onB = OneStep(g_bends[0], g_bends[0].t[1], 63, 0, &destB, &hexB);
        check(offB == onB, "control: already inside the destination hex, dest within one sub-hex, is unchanged");
    }

    // The arrival-approach diagonal: a live run logged this exact geometry at
    // five separate sites. The truck is one sub-hex short of its exact
    // destination point but has NOT yet crossed into m_hexDest (a building's
    // entry angle is wrong from here, so bAtDest gets reset off after the
    // distance test already passed it) - and the route hex the pathfinder
    // handed it sits one full hex further on, diagonally off the corner it
    // is about to cut. bCorner sees a genuine corner-cut candidate, and the
    // old distance clause of bArrival exempted it.
    //
    // It is NOT an arrival. The step does not enter m_hexDest; it enters the
    // hex to the LEFT of it, and it leaves the truck's lane (heading (-1,0)
    // rides even y, and (21,23) is odd y - the oncoming lane) a whole hex
    // short of the destination. Deferring one sub-hex along the heading to
    // (21,24) - pavement, guaranteed by the block's own OnPavement(_ahead)
    // guard - reaches the same route hex (10,11) on the next tick, in lane.
    // So the diagonal was never required, and bArrival is now the entry
    // clause alone: this case is CORRECTED, not exempted.
    {
        static CVehicle v;
        v = CVehicle();
        v.id = 9;
        v.m_ptHead = CSubHex(22, 24);
        v.m_ptTail = CSubHex(23, 24);               // heading (-1,0)
        v.m_ptNext = v.m_ptHead;
        v.m_ptDest = CSubHex(22, 23);                // one sub-hex above head
        v.m_hexDest = CHexCoord(11, 11);
        v.m_hexNext = CHexCoord(10, 11);             // hexdest + (-1,0)
        v.m_cMode = CVehicle::moving;

        g_traffic = 63;
        ClearMap();
        theMap.hex[11][12].type = CHex::road;        // head's hex
        theMap.hex[10][12].type = CHex::road;        // heading-ahead hex
        theMap.hex[10][11].type = CHex::road;        // the corner-cut hex
        theMap.hex[11][11].type = CHex::road;        // the destination hex
        theVehicleHex.Set(v.m_ptHead, &v);
        theVehicleHex.Set(v.m_ptTail, &v);

        g_roadExempt = 0;
        g_roadTurns = 0;
        g_lastExempt[0] = '\0';
        g_lastTurn[0] = '\0';
        g_bldgHex = v.m_hexDest;   // a building sits at the destination hex...
        g_bldgAngleOK = FALSE;     // ...and this angle can't enter it yet
        g_recovery = 0;
        BOOL got = v.GetNextHex(FALSE);
        g_bldgAngleOK = TRUE;      // armed only for this one fixture
        g_bldgHex = CHexCoord(-1, -1);

        theVehicleHex.Set(v.m_ptHead, nullptr);
        theVehicleHex.Set(v.m_ptTail, nullptr);
        check((got != FALSE) && (v.m_ptNext == CSubHex(21, 24)),
              "arrival-approach diagonal: deferred one sub-hex along the heading, not cut");
        check((g_roadExempt == 0) && (g_roadTurns == 1) &&
                  (std::strstr(g_lastTurn, "corner-deferred") != nullptr),
              "arrival-approach diagonal: corrected, and logged as corner-deferred not exempt");
        check(g_recovery == 0,
              "arrival-approach diagonal: the deferral is a step, not a recovery path");
        // and it stays in lane: heading (-1,0) rides even y
        check((v.m_ptNext.y & 1) == 0,
              "arrival-approach diagonal: the deferred step keeps the -X lane");

        // DEFERRING MUST NOT COST THE ARRIVAL. Advance the hull onto the
        // deferred sub-hex and ask again from the same scene: the route hex
        // (10,11) is now SQUARELY adjacent, so the corner predicate is false
        // and the ordinary step takes it - one tick later than the diagonal
        // would have, at the same sub-hex, without ever leaving the lane.
        v.m_ptTail = v.m_ptHead;            // (22,24)
        v.m_ptHead = CSubHex(21, 24);       // the deferred step, now taken
        v.m_ptNext = v.m_ptHead;
        theVehicleHex.Set(v.m_ptHead, &v);
        theVehicleHex.Set(v.m_ptTail, &v);
        g_roadTurns = 0;
        g_roadExempt = 0;
        g_recovery = 0;
        g_bldgHex = v.m_hexDest;
        g_bldgAngleOK = FALSE;              // the angle is still refused from here
        BOOL got2 = v.GetNextHex(FALSE);
        g_bldgAngleOK = TRUE;
        g_bldgHex = CHexCoord(-1, -1);
        theVehicleHex.Set(v.m_ptHead, nullptr);
        theVehicleHex.Set(v.m_ptTail, nullptr);
        check((got2 != FALSE) && (v.m_ptNext == CSubHex(21, 23)),
              "arrival-approach diagonal: the next tick reaches the route hex the diagonal wanted");
        check(g_recovery == 0,
              "arrival-approach diagonal: and reaches it by stepping, not by a recovery path");
        check(CHexCoord(v.m_ptNext) == v.m_hexNext,
              "arrival-approach diagonal: two in-lane steps land in the route hex (10,11)");
    }

    // Staggered arrival: B decides a tick after A, from the scene A's step left.
    {
        BendCase c = g_bends[0];
        c.t[0].dest[0] = 22; c.t[0].dest[1] = 24;
        c.t[1].dest[0] = 20; c.t[1].dest[1] = 20;
        CSubHex req[2][TICKS];
        int clash = -1;
        g_recovery = 0;
        check(RunStaggered(c, 63, req, clash), "staggered: both trucks produce a step every tick");
        check(clash < 0, "staggered: the later truck never asks for an occupied sub-hex");
        check(g_recovery == 0, "staggered: no recovery path is needed");
    }

    // Paired occupancy: A is already sitting in the contested corner sub-hex when
    // B decides. B must avoid it by lane guidance, not by the retry ladder.
    {
        g_traffic = 63;
        ClearMap();
        for (int i = 0; g_bends[0].paved[i][0] >= 0; ++i)
            theMap.hex[g_bends[0].paved[i][0]][g_bends[0].paved[i][1]].type = CHex::road;
        static CVehicle a, b;
        Build(a, g_bends[0].t[0], 1);
        a.m_ptHead = CSubHex(22, 22);
        a.m_ptTail = CSubHex(21, 22);
        a.m_ptNext = a.m_ptHead;
        Build(b, g_bends[0].t[1], 2);
        b.m_ptDest = CSubHex(20, 20);
        b.m_hexDest = CHexCoord(10, 10);
        Occupy(&a, &a);
        Occupy(&b, &b);
        g_recovery = 0;
        check(b.GetNextHex(FALSE) != FALSE, "paired occupancy: B still produces a step");
        check(b.m_ptNext != a.m_ptHead, "paired occupancy: B does not ask for the sub-hex A is in");
        check(g_recovery == 0, "paired occupancy: B avoids it by lane guidance, not a recovery path");
        Occupy(&a, nullptr);
        Occupy(&b, nullptr);
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

    // [ROAD-DIAG]: the corner cut the junction block never looks at. Same scene
    // twice - a vehicle heading -X out of a paved hex whose route hex is up-left,
    // where the sub-hex straight ahead is NOT paved and the diagonal's two
    // axis-aligned neighbours are not both paved either. That step drives across
    // the inside of the bend, and NOTHING corrects it: once as a military vehicle
    // (outside the transport/crane class test entirely) and once as a truck (the
    // OnPavement(_ahead) guard bails out before the corner predicate is reached).
    // The probe names both; no existing line does.
    {
        g_traffic = 63;
        ClearMap();
        theMap.hex[11][12].type = CHex::road;   // the hex the vehicle stands in
        theMap.hex[10][11].type = CHex::road;   // the route hex, diagonally off it
        // hex 10,12 and hex 11,11 stay plain: the inside of the bend

        for (int military = 1; military >= 0; --military) {
            static CVehicle v;
            v = CVehicle();
            v.id = 40 + military;
            v.m_ptHead = CSubHex(22, 24);
            v.m_ptTail = CSubHex(23, 24);        // heading (-1,0)
            v.m_ptNext = v.m_ptHead;
            v.m_ptDest = CSubHex(16, 16);        // far away: bAtDest stays FALSE
            v.m_hexDest = CHexCoord(8, 8);
            v.m_hexNext = CHexCoord(10, 11);     // diagonally up-left
            v.m_cMode = CVehicle::moving;
            theVehicleHex.Set(v.m_ptHead, &v);
            theVehicleHex.Set(v.m_ptTail, &v);

            g_isTransport = military ? FALSE : TRUE;
            g_roadDiag = 0;
            g_roadTurns = 0;
            g_lastDiag[0] = 0;
            g_recovery = 0;
            BOOL got = v.GetNextHex(FALSE);
            g_isTransport = TRUE;                // armed only for this fixture

            theVehicleHex.Set(v.m_ptHead, nullptr);
            theVehicleHex.Set(v.m_ptTail, nullptr);

            check((got != FALSE) && (v.m_ptNext == CSubHex(21, 23)) && (g_recovery == 0),
                  military ? "road-diag: a military vehicle takes the corner-cutting diagonal"
                           : "road-diag: a truck takes it too when the sub-hex ahead is unpaved");
            check(g_roadTurns == 0,
                  military ? "road-diag: no [ROAD-TURN] correction is even considered for it"
                           : "road-diag: the correction block bails out before the corner test");
            check(g_roadDiag == 1,
                  military ? "road-diag: the probe names the military cut"
                           : "road-diag: the probe names the unpaved-ahead cut");
            check(std::strstr(g_lastDiag,
                              military ? "reason not-transport" : "reason offpavement") != nullptr,
                  military ? "road-diag: reason not-transport" : "road-diag: reason offpavement");
            check(std::strstr(g_lastDiag,
                              military ? "class military" : "class transport") != nullptr,
                  military ? "road-diag: class military" : "road-diag: class transport");
            std::printf("road-diag %s\n", g_lastDiag);
        }
    }

    // the probe is a probe: an ordinary step on fully paved road must not trip it
    {
        g_roadDiag = 0;
        OneStep(g_cross, g_cross.t[0], 63, 0);
        check(g_roadDiag == 0, "road-diag: a straight step through a paved crossroads is silent");
        g_roadDiag = 0;
        OneStep(g_bends[0], g_bends[0].t[0], 63, 0);
        check(g_roadDiag == 0, "road-diag: a corrected step on a fully paved bend is silent");
    }

    // every approach of every L, T and X
    CheckApproaches();

    // the staircase corridor: the trailing-corner approach
    CheckStaircase();

    // one named bend-turn line, so QA can grep the reason keyword
    {
        g_roadTurns = 0;
        g_lastTurn[0] = 0;
        RunApproach(g_armSets[0], 1, 0, true);      // L-WS, south arm across to west
        check(g_roadTurns >= 1, "bend: the corrected step is logged as a ROAD-TURN");
        check(std::strstr(g_lastTurn, "reason bend-turn") != nullptr,
              "bend: the reason keyword is bend-turn");
        std::printf("bend %s\n", g_lastTurn);
    }

    // THE GRASS INSIDE THE BEND IS NOT REACHABLE, and this is the proof rather
    // than the assertion. The residual report's case (c) is a diagonal whose own
    // target is the unpaved hex inside the bend - bCorner failing on
    // OnPavement(_turn) alone, with body 2 inapplicable because the route hex is
    // diagonal. For that to happen the diagonal has to land in the hex BETWEEN the
    // truck's hex and the route hex, and the step arithmetic above cannot produce
    // such a step: every diagonal it asks for lands either in the truck's own hex
    // or in m_hexNext, both of which are road wherever a road turn is being made.
    // Driven here over a fully paved map so no pavement guard can interfere, for
    // every sub-hex position within a hex, every hull direction and every route hex
    // within two on each axis.
    {
        // bit 16 CLEARED: the junction block is switched off, so m_ptNext is the raw
        // step the corner test would have examined as _turn - which is what this
        // sweep is about. With the block on, body 2's own pavement-checked turn-lane
        // diagonal would be counted instead.
        g_traffic = 63 & ~16;
        ClearMap();
        for (int x = 0; x < HEXES; ++x)
            for (int y = 0; y < HEXES; ++y)
                theMap.hex[x][y].type = CHex::road;
        int strayDiagonals = 0, sampled = 0;
        const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (int sx = 0; sx < 2; ++sx)
          for (int sy = 0; sy < 2; ++sy)
            for (int d = 0; d < 4; ++d)
              for (int rdx = -2; rdx <= 2; ++rdx)
                for (int rdy = -2; rdy <= 2; ++rdy) {
                    if ((rdx == 0) && (rdy == 0))
                        continue;
                    static CVehicle v;
                    v = CVehicle();
                    v.id = 72;
                    v.m_ptHead = CSubHex(22 + sx, 20 + sy);
                    v.m_ptTail = CSubHex(22 + sx - dirs[d][0], 20 + sy - dirs[d][1]);
                    v.m_ptNext = v.m_ptHead;
                    v.m_hexNext = CHexCoord(11 + rdx, 10 + rdy);
                    v.m_ptDest = CSubHex(2, 2);            // far away: bAtDest stays FALSE
                    v.m_hexDest = CHexCoord(1, 1);
                    v.m_cMode = CVehicle::moving;
                    theVehicleHex.Set(v.m_ptHead, &v);
                    theVehicleHex.Set(v.m_ptTail, &v);
                    g_recovery = 0;
                    BOOL got = v.GetNextHex(FALSE);
                    theVehicleHex.Set(v.m_ptHead, nullptr);
                    theVehicleHex.Set(v.m_ptTail, nullptr);
                    if ((got == FALSE) || (g_recovery != 0))
                        continue;                          // a recovery path, not a step
                    int stepX = CSubHex::Diff(v.m_ptNext.x - v.m_ptHead.x);
                    int stepY = CSubHex::Diff(v.m_ptNext.y - v.m_ptHead.y);
                    if ((stepX == 0) || (stepY == 0))
                        continue;
                    ++sampled;
                    if ((!v.m_ptNext.SameHex(v.m_ptHead)) &&
                        (!v.m_ptNext.SameHex(v.m_hexNext)))
                        ++strayDiagonals;
                }
        std::printf("diagonal-target sweep: %d diagonals, %d landing outside own/route hex\n",
                    sampled, strayDiagonals);
        check(sampled > 0, "diagonal-target sweep: the sweep actually produced diagonals");
        check(strayDiagonals == 0,
              "diagonal-target sweep: no diagonal lands in the hex between - case (c) is unreachable");
        g_traffic = 63;
        g_recovery = 0;
    }

    // occupied-T case 1: the bar truck is ALREADY sitting in the contested
    // sub-hex when the stem truck decides.
    {
        PaveOccupiedT();
        static CVehicle stem, bar;
        BuildStem(stem, 1);
        bar = CVehicle();
        bar.id = 2;
        bar.m_ptHead = CSubHex(CONTESTED_X, CONTESTED_Y);          // 23,21
        bar.m_ptTail = CSubHex(CONTESTED_X - 1, CONTESTED_Y);      // 22,21 - heading east
        bar.m_ptNext = bar.m_ptHead;
        bar.m_ptDest = CSubHex(29, 21);
        bar.m_hexDest = CHexCoord(14, 10);
        bar.m_hexNext = CHexCoord(12, 10);
        bar.m_cMode = CVehicle::moving;
        Occupy(&stem, &stem);
        Occupy(&bar, &bar);

        g_recovery = 0;
        g_waitLogged = 0;
        g_lastWait[0] = 0;
        BOOL got = stem.GetNextHex(FALSE);
        check(got == FALSE, "occupied-T static: the stem truck's turn into the bar's sub-hex WAITS, it does not move");
        check(stem.m_ptNext == stem.m_ptHead,
              "occupied-T static: while waiting m_ptNext stays at m_ptHead, no step is taken");
        check(stem.m_cMode == CVehicle::traffic, "occupied-T static: the wait puts the stem truck into traffic mode");
        check(stem.m_subWaitNext == CSubHex(CONTESTED_X, CONTESTED_Y),
              "occupied-T static: the held step is the contested sub-hex, ready to resume");
        check(theVehicleHex.GetVehicle(CSubHex(CONTESTED_X, CONTESTED_Y)) == &bar,
              "occupied-T static: no overlap - the bar truck alone still occupies the contested sub-hex");
        check(g_waitLogged >= 1, "occupied-T static: the real WaitForMover path is what produced the wait ([WAIT] logged)");
        check(g_recovery == 0, "occupied-T static: waiting is not a FindSub/blocked recovery path");
        std::printf("occupied-T static %s\n", g_lastWait);

        // Release: the bar truck moves on, clear of the contested sub-hex (both
        // head and tail - a single-step advance would leave the tail sitting on
        // it, which is not what "moved on" means).
        Occupy(&bar, nullptr);
        bar.m_ptTail = CSubHex(CONTESTED_X + 1, CONTESTED_Y);
        bar.m_ptHead = CSubHex(CONTESTED_X + 2, CONTESTED_Y);
        Occupy(&bar, &bar);

        g_resumeLogged = 0;
        g_lastResume[0] = 0;
        BOOL resumed = stem.ResumeWaitedStep();
        check(resumed != FALSE, "occupied-T static: after release, ResumeWaitedStep succeeds");
        check(stem.m_ptNext == CSubHex(CONTESTED_X, CONTESTED_Y),
              "occupied-T static: it resumes into the SAME step it was holding for");
        check(stem.m_cMode == CVehicle::moving, "occupied-T static: resuming returns the stem truck to moving mode");
        check(g_resumeLogged >= 1, "occupied-T static: the resume is the real ResumeWaitedStep path ([RESUME] logged)");
        std::printf("occupied-T resume %s\n", g_lastResume);

        Occupy(&stem, nullptr);
        stem.m_ptTail = stem.m_ptHead;
        stem.m_ptHead = stem.m_ptNext;
        Occupy(&stem, &stem);
        DriveStemToWestArm(stem);

        Occupy(&stem, nullptr);
        Occupy(&bar, nullptr);
    }

    // occupied-T case 2: the bar truck does not yet occupy the contested
    // sub-hex, but its OWN natural step this same tick is also the contested
    // sub-hex - straight through traffic, no turn. Run both processing orders:
    // whichever truck updates first legitimately takes the free sub-hex; the
    // other must then see it occupied and wait, in BOTH orders - never both
    // stepping in.
    for (int order = 0; order < 2; ++order) {
        PaveOccupiedT();
        static CVehicle stem, bar;
        BuildStem(stem, 1);
        bar = CVehicle();
        bar.id = 2;
        bar.m_ptHead = CSubHex(CONTESTED_X - 1, CONTESTED_Y);      // 22,21
        bar.m_ptTail = CSubHex(CONTESTED_X - 2, CONTESTED_Y);      // 21,21 - heading east
        bar.m_ptNext = bar.m_ptHead;
        bar.m_ptDest = CSubHex(29, 21);
        bar.m_hexDest = CHexCoord(14, 10);
        bar.m_hexNext = CHexCoord(12, 10);
        bar.m_cMode = CVehicle::moving;
        Occupy(&stem, &stem);
        Occupy(&bar, &bar);

        CVehicle *first = (order == 0) ? &stem : &bar;
        CVehicle *second = (order == 0) ? &bar : &stem;
        const char *firstName = (order == 0) ? "stem" : "bar";
        const char *secondName = (order == 0) ? "bar" : "stem";

        g_recovery = 0;
        BOOL gotFirst = first->GetNextHex(FALSE);
        check(gotFirst != FALSE, "occupied-T same-tick: the first-processed truck produces a step");
        check(first->m_ptNext == CSubHex(CONTESTED_X, CONTESTED_Y),
              "occupied-T same-tick: unobstructed, the first-processed truck's natural step is the contested sub-hex");
        // commit the first truck's step so the second one decides against the
        // scene it actually left, exactly as a single-threaded tick processes
        // vehicles one at a time
        Occupy(first, nullptr);
        first->m_ptTail = first->m_ptHead;
        first->m_ptHead = first->m_ptNext;
        Occupy(first, first);

        g_waitLogged = 0;
        BOOL gotSecond = second->GetNextHex(FALSE);
        char msg[176];
        std::snprintf(msg, sizeof(msg),
                      "occupied-T same-tick (%s first): %s waits for the contested sub-hex rather than overlapping",
                      firstName, secondName);
        check((gotSecond == FALSE) && (second->m_ptNext == second->m_ptHead) &&
                  (second->m_subWaitNext == CSubHex(CONTESTED_X, CONTESTED_Y)),
              msg);
        std::snprintf(msg, sizeof(msg),
                      "occupied-T same-tick (%s first): no overlap - %s and %s are never in the same sub-hex",
                      firstName, firstName, secondName);
        check(first->m_ptHead != second->m_ptHead, msg);
        check(g_recovery == 0, "occupied-T same-tick: the second truck's wait is not a FindSub/blocked recovery path");
        check(g_waitLogged >= 1, "occupied-T same-tick: the second truck's wait is the real WaitForMover path");

        // release: the first truck moves on, clear of the contested sub-hex;
        // the truck that waited must resume into it.
        Occupy(first, nullptr);
        first->m_ptTail = CSubHex(CONTESTED_X + 1, CONTESTED_Y);
        first->m_ptHead = CSubHex(CONTESTED_X + 2, CONTESTED_Y);
        Occupy(first, first);

        g_resumeLogged = 0;
        BOOL resumed = second->ResumeWaitedStep();
        std::snprintf(msg, sizeof(msg), "occupied-T same-tick (%s first): after release, %s resumes",
                      firstName, secondName);
        check(resumed != FALSE, msg);
        check(second->m_ptNext == CSubHex(CONTESTED_X, CONTESTED_Y),
              "occupied-T same-tick: it resumes into the contested sub-hex it was holding for");
        check(second->m_cMode == CVehicle::moving, "occupied-T same-tick: resuming returns it to moving mode");
        check(g_resumeLogged >= 1, "occupied-T same-tick: the resume is the real ResumeWaitedStep path");

        Occupy(&stem, nullptr);
        Occupy(&bar, nullptr);
    }

    // occupied-T case 3 (the "reserved-step" variant): a genuine mutual
    // conflict - the bar truck's own m_ptNext this tick is the stem truck's
    // CURRENT head, at the same time the stem truck wants the bar truck's
    // current sub-hex. This is the one place WaitForMover looks past plain
    // occupancy at another vehicle's held/requested step - the stable-priority
    // tie-break behind TrafficOpts bit 2 (vehmove.cpp:2773-2791): the lower id
    // holds and waits, the higher id yields.
    for (int lowerIsStem = 0; lowerIsStem < 2; ++lowerIsStem) {
        PaveOccupiedT();
        static CVehicle stem, bar;
        BuildStem(stem, lowerIsStem ? 1 : 2);
        bar = CVehicle();
        bar.id = lowerIsStem ? 2 : 1;
        bar.m_ptHead = CSubHex(CONTESTED_X, CONTESTED_Y);          // 23,21
        bar.m_ptTail = CSubHex(CONTESTED_X, CONTESTED_Y - 1);      // 23,20
        bar.m_ptNext = stem.m_ptHead;    // bar has already decided to step onto the stem truck's cell
        bar.m_ptDest = CSubHex(23, 15);
        bar.m_hexDest = CHexCoord(11, 7);
        bar.m_hexNext = CHexCoord(11, 9);
        bar.m_cMode = CVehicle::moving;
        Occupy(&stem, &stem);
        Occupy(&bar, &bar);

        g_recovery = 0;
        g_waitLogged = 0;
        g_yieldLogged = 0;
        g_lastWait[0] = 0;
        g_lastYield[0] = 0;
        BOOL got = stem.GetNextHex(FALSE);

        if (lowerIsStem) {
            check(got == FALSE, "occupied-T reserved-step: lower id HOLDS - the stem truck still waits");
            check(stem.m_ptNext == stem.m_ptHead, "occupied-T reserved-step: holding takes no step");
            check(stem.m_subWaitNext == CSubHex(CONTESTED_X, CONTESTED_Y),
                  "occupied-T reserved-step: the lower id holds for the same contested sub-hex");
            check(g_waitLogged >= 1, "occupied-T reserved-step: the hold is logged as [WAIT]");

            Occupy(&bar, nullptr);
            bar.m_ptTail = CSubHex(CONTESTED_X + 1, CONTESTED_Y);
            bar.m_ptHead = CSubHex(CONTESTED_X + 2, CONTESTED_Y);
            Occupy(&bar, &bar);
            g_resumeLogged = 0;
            BOOL resumed = stem.ResumeWaitedStep();
            check(resumed != FALSE, "occupied-T reserved-step: after release, the holding truck resumes");
            check(stem.m_ptNext == CSubHex(CONTESTED_X, CONTESTED_Y),
                  "occupied-T reserved-step: it resumes into the contested sub-hex it held for");
            check(g_resumeLogged >= 1, "occupied-T reserved-step: the resume is the real ResumeWaitedStep path");
        } else {
            check(got != FALSE, "occupied-T reserved-step: higher id YIELDS - GetNextHex does not report a wait");
            check(g_yieldLogged >= 1, "occupied-T reserved-step: the yield is logged as [YIELD]");
            check(!stem.m_bWaitedForMover,
                  "occupied-T reserved-step: yielding does not arm a wait - the higher id goes around instead");
            // NOT ASSERTED: what m_ptNext becomes after a yield. Production sends
            // a yielding vehicle into FindSub's real go-around search; this
            // fixture's FindSub is still the pre-existing stub (always returns
            // TRUE, never rerouting - see the file header), so the step left
            // behind here is not a faithful model of the real detour. g_recovery,
            // not m_ptNext, is the honest signal that a stub path was taken.
            check(g_recovery != 0,
                  "occupied-T reserved-step: yield's go-around is the FindSub stub, not modelled here");
        }
        std::printf("occupied-T reserved-step lowerIsStem=%d %s\n", lowerIsStem,
                    lowerIsStem ? g_lastWait : g_lastYield);

        Occupy(&stem, nullptr);
        Occupy(&bar, nullptr);
    }

    checkRoadDiagGuarded();

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
