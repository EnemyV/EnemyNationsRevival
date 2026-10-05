// Timer regression fixture. Production methods/prefix are extracted verbatim by
// run-resonance-sweep-test.py; this file only supplies the player state seam.
#include <cstdio>
#include <cstdint>
#include <vector>

using BOOL = int;
constexpr BOOL TRUE = 1, FALSE = 0;
constexpr int EDICT_RESONANCE_SWEEP = 17;
constexpr int RESONANCE_SWEEP_RELOAD_SECS = 30;
constexpr int RESONANCE_SWEEP_MAX_RELOAD = 600;
using DWORD = unsigned long;
using POSITION = void*;
constexpr int RELATIONS_ALLIANCE = 4;

class CStructureData
{
public:
    enum { rocket = 1, other = 2 };
    int type;
    explicit CStructureData(int t): type(t) {}
    int GetType() const { return type; }
};

class CPlayer;
class CBuilding
{
public:
    CStructureData* data;
    CPlayer* owner;
    CBuilding(CStructureData* d, CPlayer* p): data(d), owner(p) {}
    CStructureData* GetData() const { return data; }
    CPlayer* GetOwner() const { return owner; }
};

class CBuildingMap
{
public:
    std::vector<CBuilding*> entries;
    POSITION GetStartPosition() const { return entries.empty() ? nullptr : reinterpret_cast<POSITION>(uintptr_t(1)); }
    void GetNextAssoc(POSITION& pos, DWORD& id, CBuilding*& building)
    {
        uintptr_t index = reinterpret_cast<uintptr_t>(pos) - 1;
        id = static_cast<DWORD>(index);
        building = entries[index];
        pos = index + 1 < entries.size() ? reinterpret_cast<POSITION>(index + 2) : nullptr;
    }
};

extern CBuildingMap theBuildingMap;

class CPlayer
{
public:
    float m_fPwrMult = 1.0f;
    int m_iSweepMillis = 0;
    int m_iSweepLitMillis = 0;
    BOOL m_bSweepLit = FALSE;
    BOOL m_bMe = TRUE;
    BOOL m_bEdictOn = TRUE;
    BOOL m_bPingTriggered = FALSE;
    int m_nUnlights = 0;
    unsigned int m_uSweepRand = 1;
    int m_iRelations = 0;
    const char* m_sName = "player";

    BOOL IsMe() const { return m_bMe; }
    BOOL IsEdictActive(int) const { return m_bEdictOn; }
    int GetTheirRelations() const { return m_iRelations; }
    const char* GetName() const { return m_sName; }
    void SweepUnlight() { m_bSweepLit = FALSE; m_iSweepLitMillis = 0; ++m_nUnlights; }
    int GetSweepReloadSecs() const;
    void TimerTick(int iElapsedMillis);
    CBuilding* SelectRocketForTest();
};

CBuildingMap theBuildingMap;
#include "sweep_actual.inc"

static int checks = 0, failures = 0;
static void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}

int main()
{
    // Full power: compare-before-add makes the boundary exact.
    {
        CPlayer p;
        p.TimerTick(29999);
        check(!p.m_bPingTriggered && p.m_iSweepMillis == 29999, "29999ms does not fire at full power");
        p.TimerTick(1);
        check(p.m_bPingTriggered && p.m_iSweepMillis == 0, "30000ms fires once and starts a fresh bank");
        p.m_bPingTriggered = FALSE;
        p.TimerTick(29999);
        check(!p.m_bPingTriggered, "a ping does not leave an immediately ready bank");
    }
    // The production reload calculator scales inversely with available power.
    {
        CPlayer p;
        p.m_fPwrMult = 0.5f;
        check(p.GetSweepReloadSecs() == 60, "half power doubles reload to 60 seconds");
        p.TimerTick(59999);
        check(!p.m_bPingTriggered, "half power does not fire at 59999ms");
        p.TimerTick(1);
        check(p.m_bPingTriggered, "half power fires at 60000ms");
        p.m_fPwrMult = 0.0f;
        check(p.GetSweepReloadSecs() == 600, "blackout is capped at 600 seconds");
        p.m_fPwrMult = 1.0e-30f;
        check(p.GetSweepReloadSecs() == 600, "tiny positive power safely caps at 600 seconds");
    }
    // An elapsed-time spike must produce one ping, discard its excess, and not overflow.
    {
        CPlayer p;
        p.TimerTick(1000000000);
        check(p.m_bPingTriggered && p.m_iSweepMillis == 0, "long stall produces one ping without banking excess");
        p.m_bPingTriggered = FALSE;
        p.TimerTick(29999);
        check(!p.m_bPingTriggered, "long stall cannot cause immediate catch-up ping");
    }
    // Off, local-view gating, and negative deltas never accumulate cooldown time.
    {
        CPlayer p;
        p.TimerTick(20000);
        p.m_bEdictOn = FALSE;
        p.TimerTick(50000);
        check(p.m_iSweepMillis == 0, "turning the edict off clears the cooldown bank");
        p.m_bEdictOn = TRUE;
        p.TimerTick(29999);
        check(!p.m_bPingTriggered, "rearming starts a fresh 30 second cycle");
        p.TimerTick(-5000);
        check(!p.m_bPingTriggered && p.m_iSweepMillis == 29999, "negative delta is clamped to zero");
        p.m_bMe = FALSE;
        p.TimerTick(30000);
        check(!p.m_bPingTriggered && p.m_iSweepMillis == 0, "non-local player does not accrue sweep time");
    }
    // A lit ring is released at 10 real seconds, and immediately when disabled.
    {
        CPlayer p;
        p.m_bSweepLit = TRUE;
        p.m_iSweepLitMillis = 10000;
        p.TimerTick(9999);
        check(p.m_bSweepLit && p.m_iSweepLitMillis == 1, "ring remains lit for 9999ms");
        p.TimerTick(1);
        check(!p.m_bSweepLit && p.m_iSweepLitMillis == 0 && p.m_nUnlights == 1,
              "ring expires at 10000ms and is unlit once");
        p.m_bSweepLit = TRUE;
        p.m_iSweepLitMillis = 5000;
        p.m_bEdictOn = FALSE;
        p.TimerTick(0);
        check(!p.m_bSweepLit && p.m_nUnlights == 2, "disabling the edict releases the ring immediately");
    }
    // Production candidate pass: only enemy rockets that are not allies are eligible.
    // Across seeds selection may vary, but every result must be one of the eligible ships.
    {
        CPlayer me;
        CPlayer enemyA, enemyB, enemyC, ally;
        enemyA.m_bMe = enemyB.m_bMe = enemyC.m_bMe = ally.m_bMe = FALSE;
        ally.m_iRelations = RELATIONS_ALLIANCE;
        CStructureData rocket(CStructureData::rocket), nonRocket(CStructureData::other);
        CBuilding a(&rocket, &enemyA), b(&rocket, &enemyB), c(&rocket, &enemyC);
        CBuilding allied(&rocket, &ally), owned(&rocket, &me), unrelated(&nonRocket, &enemyA);
        CBuilding noData(nullptr, &enemyB);
        theBuildingMap.entries = { nullptr, &allied, &owned, &unrelated, &noData, &a, &b, &c };
        bool sawA = false, sawB = false, sawC = false;
        const unsigned int initialRand = me.m_uSweepRand;
        for (int i = 0; i < 96; ++i)
        {
            CBuilding* selected = me.SelectRocketForTest();
            check(selected == &a || selected == &b || selected == &c,
                  "selection chooses only a non-allied enemy rocket");
            sawA |= selected == &a;
            sawB |= selected == &b;
            sawC |= selected == &c;
        }
        check(sawA && sawB && sawC, "private selection stream can reach every eligible enemy");
        check(me.m_uSweepRand != initialRand, "selection advances the private random stream");
        theBuildingMap.entries.clear();
        const unsigned int beforeEmpty = me.m_uSweepRand;
        check(me.SelectRocketForTest() == nullptr, "empty candidate map selects no rocket");
        check(me.m_uSweepRand == beforeEmpty, "empty map does not consume random state");
    }
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
