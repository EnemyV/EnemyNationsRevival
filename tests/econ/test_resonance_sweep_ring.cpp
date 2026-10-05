// Ring fixture: production SweepLight/SweepUnlight callback code is extracted verbatim by the
// runner. The small map doubles expose visibility counters and building reveal side effects.
#include <cstdio>
#include <vector>

using BOOL = int;
constexpr BOOL TRUE = 1, FALSE = 0;
constexpr int RESONANCE_SWEEP_LIT_SECS = 10;
constexpr int RELATIONS_ALLIANCE = 0;
#define ASSERT(x) do { if (!(x)) std::fprintf(stderr, "ASSERT: %s\n", #x); } while (0)

class CHexCoord
{
public:
    CHexCoord(int x = 0, int y = 0) : x_(x), y_(y) {}
    int X() const { return x_; }
    int Y() const { return y_; }
    void Wrap() { x_ = (x_ % 32 + 32) % 32; y_ = (y_ % 32 + 32) % 32; }
    void SetInvalidated() {}
private:
    int x_, y_;
};

class CHex
{
public:
    enum { bldg = 1 };
    unsigned char visible = 0;
    unsigned char units = 0;
    void IncVisible() { ++visible; }
    void DecVisible() { --visible; }
    int GetVisible() const { return visible; }
    BOOL GetVisibility() const { return visible != 0; }
    int GetUnits() const { return units; }
};

class CPlayer
{
public:
    BOOL m_bSweepLit = FALSE;
    int m_iSweepLitMillis = 0, m_iSweepLitCX = 0, m_iSweepLitCY = 0;
    CHexCoord m_hexSweepLit;
    std::vector<CHexCoord> m_aSweepLitHexes;
    int relation = 3;
    int ringSize = 2;
    int GetSweepRings() const { return ringSize; }
    int GetTheirRelations() const { return relation; }
    void SweepLight(class CBuilding* pRocket);
    void SweepUnlight();
};

class CStructureData
{
public:
    enum { rocket = 1, ordinary = 2 };
    explicit CStructureData(int type) : type_(type) {}
    int GetType() const { return type_; }
private:
    int type_;
};

class CBuilding
{
public:
    CBuilding(CPlayer* owner, CStructureData* data, CHexCoord hex, int cx = 1, int cy = 1)
        : owner_(owner), data_(data), hex_(hex), cx_(cx), cy_(cy) {}
    CHexCoord GetHex() const { return hex_; }
    int GetCX() const { return cx_; }
    int GetCY() const { return cy_; }
    CPlayer* GetOwner() const { return owner_; }
    CStructureData* GetData() const { return data_; }
    int GetTheirRelations() const { return 3; }
    bool IsLive() const { return true; }
    void PauseAnimations(BOOL) {}
private:
    CPlayer* owner_;
    CStructureData* data_;
    CHexCoord hex_;
    int cx_, cy_;
};

class CBuildingHex
{
public:
    CBuilding* at[32][32] = {};
    CBuilding* _GetBuilding(CHexCoord c) const { c.Wrap(); return at[c.X()][c.Y()]; }
};
class CGameMap
{
public:
    CHex hex[32][32];
    CHex* _GetHex(CHexCoord c) { c.Wrap(); return &hex[c.X()][c.Y()]; }
    void EnumHexes(CHexCoord origin, int cx, int cy,
                   int (*fn)(CHex*, CHexCoord, void*), void* data)
    {
        origin.Wrap();
        for (int y = 0; y < cy; ++y)
            for (int x = 0; x < cx; ++x)
            {
                CHexCoord c(origin.X() + x, origin.Y() + y);
                c.Wrap();
                if (fn(_GetHex(c), c, data)) return;
            }
    }
};

CGameMap theMap;
CBuildingHex theBuildingHex;
CBuilding* g_otherRocket = nullptr;
CBuilding* g_ordinaryBuilding = nullptr;
bool g_otherRocketRevealed = false;
bool g_ordinaryRevealed = false;
void EnHexBecameVisible(CHex*, CHexCoord c, CPlayer*)
{
    CBuilding* p = theBuildingHex._GetBuilding(c);
    if (p == g_otherRocket) g_otherRocketRevealed = true;
    if (p == g_ordinaryBuilding) g_ordinaryRevealed = true;
}
#include "ring_actual.inc"

static int checks = 0, failures = 0;
static void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}

static void resetMap()
{
    for (int x = 0; x < 32; ++x)
        for (int y = 0; y < 32; ++y)
        {
            theMap.hex[x][y].visible = 0;
            theMap.hex[x][y].units = 0;
            theBuildingHex.at[x][y] = nullptr;
        }
    g_otherRocketRevealed = false;
    g_ordinaryRevealed = false;
}

int main()
{
    CPlayer me, foe;
    CStructureData rocket(CStructureData::rocket), ordinary(CStructureData::ordinary);
    CBuilding selected(&foe, &rocket, CHexCoord(10, 10), 2, 2);
    CBuilding otherRocket(&foe, &rocket, CHexCoord(12, 11), 2, 1);
    CBuilding otherBuilding(&foe, &ordinary, CHexCoord(9, 12));
    g_otherRocket = &otherRocket;
    g_ordinaryBuilding = &otherBuilding;
    for (int y = 10; y < 12; ++y)
        for (int x = 10; x < 12; ++x)
        {
            theBuildingHex.at[x][y] = &selected;
            theMap.hex[x][y].units |= CHex::bldg;
        }
    for (int x = 12; x < 14; ++x)
    {
        theBuildingHex.at[x][11] = &otherRocket;
        theMap.hex[x][11].units |= CHex::bldg;
    }
    theBuildingHex.at[9][12] = &otherBuilding;
    theMap.hex[9][12].units |= CHex::bldg;

    // Independent vision must survive a ring; the adjacent rocket's whole footprint is skipped.
    CHex* overlap = theMap._GetHex(CHexCoord(8, 8));
    overlap->IncVisible();
    me.SweepLight(&selected);
    check(theMap._GetHex(CHexCoord(12, 11))->GetVisible() == 0 &&
          theMap._GetHex(CHexCoord(13, 11))->GetVisible() == 0,
          "unselected enemy rocket footprint is not newly lit");
    check(theMap._GetHex(CHexCoord(9, 12))->GetVisible() == 1,
          "ordinary enemy building remains within ring vision");
    check(overlap->GetVisible() == 2, "existing ground/vehicle vision counter is preserved");
    check(g_ordinaryRevealed, "ordinary building uses the normal reveal transition");
    check(!g_otherRocketRevealed, "adjacent enemy rocket is not passed to reveal handling");

    // A building-map change must not change the set of decrements owed by the ring.
    for (int x = 0; x < 32; ++x)
        for (int y = 0; y < 32; ++y) theBuildingHex.at[x][y] = nullptr;
    const std::vector<CHexCoord> raised = me.m_aSweepLitHexes;
    me.SweepUnlight();
    bool allReleased = true;
    for (const CHexCoord& c : raised)
        if (theMap._GetHex(c)->GetVisible() != (c.X() == 8 && c.Y() == 8 ? 1 : 0)) allReleased = false;
    check(allReleased, "exact recorded increments are released after building map changes");
    check(theMap._GetHex(CHexCoord(12, 11))->GetVisible() == 0,
          "skipped rocket footprint remains untouched on release");
    me.SweepUnlight();
    check(overlap->GetVisible() == 1, "repeated unlight is idempotent");
    check(me.m_aSweepLitHexes.empty(), "released ring discards stored coordinates");

    // Tier 1 has no ring. Tier 6 reaches five hexes and wraps correctly at a map edge; switching
    // targets first releases the previous exact ring before recording the new one.
    resetMap();
    me.ringSize = 0;
    me.SweepLight(&selected);
    check(!me.m_bSweepLit && me.m_aSweepLitHexes.empty(), "tier 1 contact produces no ring");

    me.ringSize = 2;
    me.SweepLight(&selected);
    CHex* oldOnly = theMap._GetHex(CHexCoord(8, 8));
    check(oldOnly->GetVisible() == 1, "first selected contact lights its old ring");
    CBuilding boundaryRocket(&foe, &rocket, CHexCoord(1, 1), 2, 2);
    for (int y = 1; y < 3; ++y)
        for (int x = 1; x < 3; ++x)
        {
            theBuildingHex.at[x][y] = &boundaryRocket;
            theMap.hex[x][y].units |= CHex::bldg;
        }
    me.ringSize = 5;
    me.SweepLight(&boundaryRocket);
    check(oldOnly->GetVisible() == 0, "switching rockets releases the prior ring");
    check(me.m_aSweepLitHexes.size() == 144, "tier 6 records the full 12 by 12 ring");
    check(theMap._GetHex(CHexCoord(0, 0))->GetVisible() == 1,
          "tier 6 ring wraps across the map boundary");
    check(theMap._GetHex(CHexCoord(28, 28))->GetVisible() == 1,
          "negative ring origin lights the opposite map edge");
    me.SweepUnlight();
    check(theMap._GetHex(CHexCoord(0, 0))->GetVisible() == 0,
          "wrapped ring coordinates are balanced on release");
    check(theMap._GetHex(CHexCoord(28, 28))->GetVisible() == 0,
          "opposite map edge is released too");

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
