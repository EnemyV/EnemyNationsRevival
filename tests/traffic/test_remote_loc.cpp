// Compile the real SetLoc and SetFromMsg, replacing only their scene dependencies.
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <initializer_list>
using BOOL = int;
constexpr BOOL TRUE = 1, FALSE = 0;
constexpr int FULL_ROT = 128, MAX_HEX_HT = 64;
int __roll(int low, int high, int value) {
    int r = (value - low) % (high - low);
    return low + (r < 0 ? r + high - low : r);
}
struct Point { int x = 100, y = 100; };
bool operator!=(Point a, Point b) { return a.x != b.x || a.y != b.y; }
struct CPlayer { bool local = false; BOOL IsLocal() const { return local; } };
struct CTransportData {
    enum { FL1hex = 1 };
    int flags = 0;
    int GetVehFlags() const { return flags; }
};
struct Turret { int m_iDir = 0; void SetDir(int d) { m_iDir = d; } };
struct Map { int Get_eX() const { return 1024; } int Get_eY() const { return 1024; } } theMap;
struct CWndWorld {
    enum { visible = 1, other_units = 2 };
    int invalidations = 0;
    void InvalidateWindow(int) { ++invalidations; }
};
struct App { CWndWorld m_wndWorld; } theApp;
struct CMsgVehLoc {
    Point m_ptDest, m_ptNext, m_ptHead, m_ptTail, m_hexNext, m_hexDest;
    int m_iDir = 0, m_iTurretDir = 0, m_iXadd = 1, m_iYadd = 2;
    int m_iDadd = 0, m_iTadd = 0, m_iMode = 1, m_iOwn = 1;
    int m_iStepsLeft = 8, m_iSpeed = 12;
    BOOL m_bOnWater = FALSE;
};
class CVehicle {
public:
    enum VEH_MODE { stop = 0, moving = 1 };
    Point m_ptDest, m_ptNext, m_ptHead, m_ptTail, m_hexNext, m_hexDest, m_maploc;
    int m_iDir = 0, m_iXadd = 0, m_iYadd = 0, m_iDadd = 0, m_iTadd = 0;
    int m_cOwn = 0, m_iStepsLeft = 0, m_iSpeed = 0;
    VEH_MODE m_cMode = stop;
    BOOL m_bReversing = FALSE;
    void* m_pUnitOppo = nullptr;
    CPlayer owner;
    CTransportData data;
    Turret turret;
    bool hasTurret = true, visible = true;
    int bodyDir = 32, nextDir = 48, wheelChanges = 0;
    bool wheelsMoving = false, water = false;
    CPlayer* GetOwner() { return &owner; }
    CTransportData* GetData() { return &data; }
    Turret* GetTurret() { return hasTurret ? &turret : nullptr; }
    int CalcDir() { return bodyDir; }
    int CalcNextDir() { return nextDir; }
    BOOL IsVisible() { return visible; }
    void Wheels(BOOL on, BOOL) { ++wheelChanges; wheelsMoving = on != 0; }
    void SetOnWater(BOOL on) { water = on != 0; }
    void SetLoc(BOOL);
    void SetFromMsg(CMsgVehLoc*, BOOL);
};
#include "remote_loc_actual.inc"

int failures = 0, checks = 0;
void check(bool ok, const char* what) {
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", what); }
}
int main() {
    // Forward, reverse, and partially turning packets: the remote does not
    // decide facing from endpoint labels or need the owner's recovery flags.
    for (int dir : {32, 96, 110}) {
        CVehicle remote;
        CMsgVehLoc msg;
        msg.m_iDir = dir;
        msg.m_iTurretDir = 73;
        msg.m_iDadd = -2;
        msg.m_iStepsLeft = 3;
        msg.m_hexDest = {899, 325};
        remote.SetFromMsg(&msg, FALSE);
        check(remote.m_iDir == dir, "packet facing survives apply");
        check(remote.turret.m_iDir == 73, "packet turret facing survives apply");
        check(remote.m_iDadd == -2 && remote.m_iStepsLeft == 3, "interpolation is preserved");
        check(remote.m_hexDest.x == 899 && remote.m_hexDest.y == 325, "hex destination preserved");
        check(remote.wheelsMoving && remote.wheelChanges == 1, "remote wheels start");
        remote.SetLoc(TRUE); // endpoint refresh while waiting for the next packet
        check(remote.m_iDir == dir, "remote endpoint refresh preserves facing");
        msg.m_iMode = CVehicle::stop;
        remote.SetFromMsg(&msg, TRUE);
        check(!remote.wheelsMoving && remote.wheelChanges == 2, "remote wheels stop");
    }
    CVehicle local;
    local.owner.local = true;
    local.SetLoc(TRUE);
    check(local.m_iDir == 32, "local forward still derives facing");
    local.m_bReversing = TRUE;
    local.SetLoc(TRUE);
    check(local.m_iDir == 96, "local reverse retains nose direction");
    local.data.flags = CTransportData::FL1hex;
    local.m_ptNext.x++;
    local.SetLoc(TRUE);
    check(local.m_iDir == 48, "local single-cell facing unchanged");
    CVehicle remoteNoTurret;
    remoteNoTurret.hasTurret = false;
    CMsgVehLoc msg;
    msg.m_iDir = 96;
    remoteNoTurret.SetFromMsg(&msg, TRUE);
    check(remoteNoTurret.m_iDir == 96, "remote without turret preserves facing");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
