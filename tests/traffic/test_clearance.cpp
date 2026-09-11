// Real eligibility/propagation/watch methods, with only the scene replaced.
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <initializer_list>
#include <map>
#include <utility>
using BOOL = int;
constexpr BOOL TRUE = 1, FALSE = 0;
int options = 63;
int TrafficOpts() { return options; }
void WaitLog(const char*, ...) {}
struct CSubHex {
    int x = 20, y = 20;
    CSubHex() = default;
    CSubHex(int a, int b): x(a), y(b) {}
    void Wrap() { x = (x + 2048) % 2048; y = (y + 2048) % 2048; }
    static int Diff(int x) { return (x + 3072) % 2048 - 1024; }
};
bool operator==(CSubHex a, CSubHex b) { return a.x == b.x && a.y == b.y; }
bool operator!=(CSubHex a, CSubHex b) { return !(a == b); }
struct Hex { int X() const { return 10; } int Y() const { return 10; } };
struct Owner { bool local = true; BOOL IsLocal() const { return local; } };
struct Data {
    enum { FL1hex = 1 };
    bool boat = false, truck = true, crane = false;
    int flags = 0;
    BOOL IsBoat() const { return boat; }
    BOOL IsTransport() const { return truck; }
    BOOL IsCrane() const { return crane; }
    int GetVehFlags() const { return flags; }
};
using CTransportData = Data;
struct Game { int frames = 1; int GetFramesElapsed() const { return frames; } } theGame;
struct CVehicle {
    enum { stopped = 4, dying = 8, stop = 0, moving = 1, traffic = 7, none = 0, build = 2 };
    int m_unitFlags = 0, m_cOwn = 1, m_iEvent = none, m_cMode = moving;
    int m_iJamClear = 0, m_iJamFwd = 0, m_iJamWatch = 0, m_iJamCool = 0;
    int m_iJamAnchorX = 0, m_iJamAnchorY = 0, id = 1;
    void* m_pBldg = nullptr;
    bool manual = false;
    Owner* owner = nullptr;
    Data data;
    CSubHex m_ptHead{20,20}, m_ptTail{19,20}, m_ptNext{21,20}, m_subWaitNext{-1,-1};
    Owner* GetOwner() const { return owner; }
    const Data* GetData() const { return &data; }
    BOOL IsHpControl() const { return manual; }
    int GetID() const { return id; }
    Hex GetHexHead() const { return {}; }
    BOOL BlockedLaneStep(CSubHex&) { return FALSE; }
    BOOL JamEligible() const;
    void JamForward();
    void JamWatch();
};
struct VehicleMap {
    std::map<std::pair<int,int>, CVehicle*> cells;
    CVehicle* _GetVehicle(CSubHex p) const {
        auto i = cells.find({p.x,p.y}); return i == cells.end() ? nullptr : i->second;
    }
    void body(CVehicle& v) { cells[{v.m_ptHead.x,v.m_ptHead.y}] = &v; cells[{v.m_ptTail.x,v.m_ptTail.y}] = &v; }
} theVehicleHex;
#include "clearance_actual.inc"
int checks = 0, failures = 0;
void check(bool ok, const char* name) {
    ++checks; if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
int main() {
    Owner player, ai, enemy, remote; remote.local = false;
    CVehicle a, b; a.owner = b.owner = &player; b.id = 2;
    check(a.JamEligible(), "automatic local truck eligible");
    a.owner = &ai; check(a.JamEligible(), "locally simulated AI eligible");
    a.owner = &remote; check(!a.JamEligible(), "remote simulation excluded");
    a.owner = nullptr; check(!a.JamEligible(), "missing owner excluded"); a.owner = &player;
    for (int flag : {CVehicle::stopped, CVehicle::dying}) {
        a.m_unitFlags = flag; check(!a.JamEligible(), "Stop and dying excluded");
    }
    a.m_unitFlags = 0; a.m_cOwn = 0; check(!a.JamEligible(), "unowned/carried body excluded"); a.m_cOwn = 1;
    a.manual = true; check(!a.JamEligible(), "manual control excluded"); a.manual = false;
    a.data.boat = true; check(!a.JamEligible(), "boats excluded"); a.data.boat = false;
    a.data.flags = Data::FL1hex; check(!a.JamEligible(), "single-cell excluded"); a.data.flags = 0;
    a.data.truck = false; check(!a.JamEligible(), "military nontransport excluded");
    a.data.crane = true; check(a.JamEligible(), "idle crane eligible");
    a.m_iEvent = CVehicle::build; check(a.JamEligible(), "travelling crane build order eligible");
    a.m_pBldg = &a; check(!a.JamEligible(), "active building excluded"); a.m_pBldg = nullptr;
    a.data.truck = true; a.data.crane = false; a.m_iEvent = CVehicle::none;
    options = 0; check(!a.JamEligible(), "feature gate respected"); options = 63;
    b.m_ptHead = {21,20}; b.m_ptTail = {22,20};
    theVehicleHex.body(a); theVehicleHex.body(b); a.m_iJamClear = 100;
    a.JamForward(); check(b.m_iJamClear == 100, "touching body receives remaining time");
    b.m_iJamClear = 7; a.JamForward(); check(b.m_iJamClear == 7, "active recipient deadline never refreshed");
    b.m_iJamClear = 0; b.owner = &enemy; a.JamForward(); check(b.m_iJamClear == 0, "other owner not recruited"); b.owner = &player;
    b.m_unitFlags = CVehicle::stopped; a.JamForward(); check(b.m_iJamClear == 0, "explicit Stop not recruited"); b.m_unitFlags = 0;
    theVehicleHex.cells.clear(); theVehicleHex.cells[{21,20}] = &b;
    b.m_ptHead = {24,20}; b.m_ptTail = {25,20};
    a.JamForward(); check(b.m_iJamClear == 0, "future reservation is not physical contact");
    theVehicleHex.cells.clear(); b.m_ptHead = {21,20}; b.m_ptTail = {22,20}; theVehicleHex.body(a); theVehicleHex.body(b);
    a.m_cMode = b.m_cMode = CVehicle::stop; a.m_iJamClear = 1;
    a.JamWatch(); check(a.m_iJamClear == 0 && b.m_iJamClear == 0, "expire before forwarding final frame");
    a.m_iJamClear = JAM_WINDOW_FRAMES; a.m_iJamFwd = 0;
    for (int frame = 0; frame < JAM_WINDOW_FRAMES + 5; ++frame) { a.JamWatch(); b.JamWatch(); }
    check(a.m_iJamClear == 0 && b.m_iJamClear == 0, "touching cycle terminates without refreshing itself");
    a.m_iJamClear = 25; a.m_unitFlags = CVehicle::stopped; a.JamWatch();
    check(a.m_iJamClear == 0 && a.m_iJamWatch == 0, "Stop cancels stale participation"); a.m_unitFlags = 0;
    a.m_cMode = CVehicle::traffic; a.m_iJamCool = 0; a.m_iJamWatch = 1;
    a.m_iJamAnchorX = a.m_ptHead.x + a.m_ptTail.x; a.m_iJamAnchorY = a.m_ptHead.y + a.m_ptTail.y;
    a.m_ptNext = a.m_ptHead; a.m_subWaitNext = b.m_ptHead;
    theGame.frames = JAM_STUCK_FRAMES - 2; a.JamWatch();
    check(a.m_iJamClear == 0, "no early request before stagnation threshold");
    theGame.frames = 1; a.JamWatch(); check(a.m_iJamClear > 0, "traffic wait uses retained blocker square");
    a.m_iJamClear = a.m_iJamCool = 0; a.m_cMode = CVehicle::moving; a.m_iJamWatch = 10;
    std::swap(a.m_ptHead, a.m_ptTail); a.JamWatch();
    check(a.m_iJamWatch == 11, "reverse relabel is not physical progress");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
