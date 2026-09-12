// Scrounging terrain cache vs Slash and Burn.
//
// The four CWarehouseBuilding::m_iScr* multipliers are scanned once and cached behind a negative
// sentinel. Slash and Burn retypes forest hexes to plain underneath them, so ApplySlash has to
// invalidate every warehouse whose scan ring covers the cut hex or the warehouse keeps paying the
// pre-cut rate until something else rebuilds the cache (a save/load, silently changing the yield).
//
// Everything below the include is scene; the production bodies come in VERBATIM from new_unit.cpp
// and altoutput.cpp via run-scrounge-cache-test.py. Nothing here mirrors the scan weights, the ring
// geometry, the rounding or the invalidation -- those are the things under test.
//
// The terrain FarmMult table is a STAND-IN: the shipped one lives in ENATIONS.DAT, not in source.
// Only its shape matters here (forest and plain differ, city/road/water are 0, rough beats forest),
// because every assertion compares the cached answer against a fresh scan through the same table.

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <utility>

using BOOL = int;
using DWORD = unsigned long;
using LONG  = long;
using POSITION = void*;
constexpr BOOL TRUE = 1, FALSE = 0;
#ifndef NULL
#define NULL 0
#endif

// ---------------------------------------------------------------------------------------
// scene: map

const int MAPDIM = 64;

class CHexCoord
{
    int m_x = 0, m_y = 0;
public:
    CHexCoord() = default;
    CHexCoord(int x, int y): m_x(x), m_y(y) {}
    int  X() const { return m_x; }
    int  Y() const { return m_y; }
    void X(int v) { m_x = v; }
    void Y(int v) { m_y = v; }
    void Xdec() { --m_x; }
    void Ydec() { --m_y; }
    void Wrap() { m_x = ((m_x % MAPDIM) + MAPDIM) % MAPDIM; m_y = ((m_y % MAPDIM) + MAPDIM) % MAPDIM; }
    void SetInvalidated() { }
    // Signed wrap-around difference, the shape CHexCoord::Diff has in terrain.h.
    static int Diff(int d) { return ((d % MAPDIM) + MAPDIM + MAPDIM / 2) % MAPDIM - MAPDIM / 2; }
};

class CHex
{
public:
    enum { plain, forest, rough, hill, mountain, desert, swamp, lake, ocean, river,
           coastline, city, road, fields, NUMTYPES };
    enum { bldg = 1, bridge = 2 };

    int   m_iType = plain;
    int   m_iVisType = plain;
    int   m_iTree = 0;
    int   m_iUnits = 0;
    int   m_iVis = 1;
    void* m_psprite = nullptr;

    int  GetType() const { return m_iType; }
    // The shipped SetType overwrites the visible type on every exit path (that is why SlashHex
    // restores it afterwards for fogged hexes); keep that, the fog branch depends on it.
    void SetType(int t) { m_iType = t; m_iVisType = t; }
    void SetTree(int t) { m_iTree = t; }
    int  GetUnits() const { return m_iUnits; }
    int  GetVisibility() const { return m_iVis; }
    void SetVisibleType(int t) { m_iVisType = t; }
    int  GetVisibleType() const { return m_iVisType; }
};

struct CTerrainData
{
    int m_iFarmMult = 0;
    int GetFarmMult() const { return m_iFarmMult; }
};

// Stand-in for the ENATIONS.DAT terrain table (see the file header).
struct CTerrain
{
    CTerrainData m_a[CHex::NUMTYPES];
    CTerrain()
    {
        m_a[CHex::plain].m_iFarmMult     = 8;
        m_a[CHex::forest].m_iFarmMult    = 4;
        m_a[CHex::rough].m_iFarmMult     = 6;
        m_a[CHex::hill].m_iFarmMult      = 3;
        m_a[CHex::mountain].m_iFarmMult  = 0;
        m_a[CHex::desert].m_iFarmMult    = 1;
        m_a[CHex::swamp].m_iFarmMult     = 2;
        m_a[CHex::fields].m_iFarmMult    = 8;
        // lake / ocean / river / coastline / city / road stay 0.
    }
    CTerrainData const& GetData(int iType) const { return m_a[iType]; }
    int   GetCount(int) const { return 1; }
    void* GetSprite(int, int) const { return nullptr; }
} theTerrain;

using FNENUMHEX = int (CHex* pHex, CHexCoord hex, void* pData);

struct CMap
{
    CHex m_a[MAPDIM][MAPDIM];

    CHex* _GetHex(CHexCoord hex) { hex.Wrap(); return &m_a[hex.Y()][hex.X()]; }
    void  EnumHexes(CHexCoord const& hex, int ex, int ey, FNENUMHEX fnEnum, void* pData = NULL)
    {
        for (int dy = 0; dy < ey; ++dy)
            for (int dx = 0; dx < ex; ++dx)
            {
                CHexCoord h(hex.X() + dx, hex.Y() + dy);
                h.Wrap();
                if (fnEnum(_GetHex(h), h, pData))
                    return;
            }
    }
} theMap;

unsigned g_enStaticDirtyGen = 0;

// ---------------------------------------------------------------------------------------
// scene: structure data + buildings

struct CMaterialTypes { enum { lumber = 0, food, iron, coal, oil, NUMMATS }; };

class CStructureData
{
public:
    enum BLDG_UNION_TYPE { UTmaterials, UTfarm, UTwarehouse, UTmine, UTother };
    enum BLDG_TYPE { lumber, farm, warehouse, hq, NUMBLDGS };

    int m_iType = hq, m_iUnion = UTother, m_cx = 1, m_cy = 1;

    int             GetCX() const { return m_cx; }
    int             GetCY() const { return m_cy; }
    int             GetType() const { return m_iType; }
    BLDG_UNION_TYPE GetUnionType() const { return (BLDG_UNION_TYPE)m_iUnion; }
};

struct CStructures
{
    CStructureData m_a[CStructureData::NUMBLDGS];
    CStructures()
    {
        m_a[CStructureData::lumber]    = { CStructureData::lumber,    CStructureData::UTfarm,      2, 2 };
        m_a[CStructureData::farm]      = { CStructureData::farm,      CStructureData::UTfarm,      3, 3 };
        m_a[CStructureData::warehouse] = { CStructureData::warehouse, CStructureData::UTwarehouse, 3, 3 };
        m_a[CStructureData::hq]        = { CStructureData::hq,        CStructureData::UTother,     2, 2 };
    }
    CStructureData const* GetData(int iTyp) const { return &m_a[iTyp]; }
} theStructures;

class CBuilding
{
public:
    CHexCoord m_hex;
    int       m_iTyp = CStructureData::hq;
    int       m_iDir = 0;

    virtual ~CBuilding() { }
    CHexCoord             GetHex() const { return m_hex; }
    int                   GetDir() const { return m_iDir; }
    CStructureData const* GetData() const { return theStructures.GetData(m_iTyp); }
};

class CFarmBuilding : public CBuilding
{
public:
    LONG m_iTerMult = 0;

    static int  LandMult(CHexCoord _hex, int iTyp, int iDir);
    static int  ForestMultAt(CHexCoord _hex, int iTyp, int iDir, int iScale = 1);
    static int  SoilMultAt(CHexCoord _hex, int iTyp, int iDir);
    static void ScrapMultsAt(CHexCoord _hex, int iTyp, int iDir, int* piIron, int* piCoal, int iScale = 1);
    static int  ScroungeSoilMultAt(CHexCoord _hex, int iTyp, int iDir, int iScale = 1);
    void        UpdateFarm();
    static BOOL ApplySlash(CHexCoord hex);
};

class CWarehouseBuilding : public CBuilding
{
public:
    // public here only so the fixture can poison them; protected in building.h.
    LONG m_iScrForest = -1;
    LONG m_iScrSoil   = -1;
    LONG m_iScrIron   = -1;
    LONG m_iScrCoal   = -1;

    int  GetScroungeForestMult();
    int  GetScroungeSoilMult();
    int  GetScroungeIronMult();
    int  GetScroungeCoalMult();
    void UpdateScrounge();
    void InvalidateScrounge();
};

struct CBuildingMap
{
    std::vector<std::pair<DWORD, CBuilding*> > m_v;

    POSITION GetStartPosition() const
    {
        return m_v.empty() ? NULL : (POSITION)(uintptr_t)1;
    }
    void GetNextAssoc(POSITION& pos, DWORD& dwID, CBuilding*& pBldg) const
    {
        size_t i = (size_t)(uintptr_t)pos - 1;
        dwID  = m_v[i].first;
        pBldg = m_v[i].second;
        pos   = (i + 1 < m_v.size()) ? (POSITION)(uintptr_t)(i + 2) : NULL;
    }
} theBuildingMap;

namespace AltOutput
{
    struct AltMat { int m_iMat; int m_iPerMin; };
    const int kMaxMulti = 4;
    struct AltOutputDef
    {
        AltMat m_aMulti[kMaxMulti];
        int    m_nMulti;
        bool   m_bTerrainScaled;
    };
    int MultiLinesFor(CBuilding* pBldg, const AltOutputDef* pDef, AltMat* pOut);
}

#include "scrounge_cache_actual.inc"

// ---------------------------------------------------------------------------------------
// harness

int g_checks = 0, g_fail = 0;

static void check(bool ok, const char* name)
{
    ++g_checks;
    if (!ok)
    {
        ++g_fail;
        std::fprintf(stderr, "FAIL: %s\n", name);
    }
}

static void check_eq(int got, int want, const char* name)
{
    ++g_checks;
    if (got != want)
    {
        ++g_fail;
        std::fprintf(stderr, "FAIL: %s (got %d, want %d)\n", name, got, want);
    }
}

// The scene: a 3x3 warehouse at (20,20), a 2x2 lumber mill at (17,21) whose box overlaps the
// warehouse's rings, and a band of forest across the top of those rings. The scan boxes:
//   warehouse forest ring (LumberBox, footprint+3): x 17..25, y 17..25
//   warehouse scrap + scrounge-soil rings (footprint+2): x 18..24, y 18..24
//   mill box (LumberBox on a 2x2, footprint+3): x 14..22, y 18..26
// Neither footprint touches the forest band: a hex under a building carries CHex::bldg and SlashHex
// refuses to clear it (the E21 guard), so a mill standing in the trees would silently shrink every
// "cut the whole band" count below.
const int WX = 20, WY = 20;   // warehouse top-left
const int MX = 17, MY = 21;   // mill top-left

CWarehouseBuilding g_whse;
CFarmBuilding      g_mill;

static void ResetScene()
{
    for (int y = 0; y < MAPDIM; ++y)
        for (int x = 0; x < MAPDIM; ++x)
        {
            CHex& h = theMap.m_a[y][x];
            h = CHex();
        }

    // Forest band across y = 17..19, the full width of the warehouse's forest ring.
    for (int y = 17; y <= 19; ++y)
        for (int x = 17; x <= 25; ++x)
            theMap.m_a[y][x].SetType(CHex::forest);

    // The warehouse's own pad turns to city on completion (fnBuildOnHex) and carries the bldg bit,
    // which is what makes SlashHex refuse to clear a hex under a building.
    for (int y = WY; y < WY + 3; ++y)
        for (int x = WX; x < WX + 3; ++x)
        {
            theMap.m_a[y][x].SetType(CHex::city);
            theMap.m_a[y][x].m_iUnits |= CHex::bldg;
        }
    for (int y = MY; y < MY + 2; ++y)
        for (int x = MX; x < MX + 2; ++x)
            theMap.m_a[y][x].m_iUnits |= CHex::bldg;

    // A second, unrelated forest far away -- the control.
    for (int y = 50; y <= 52; ++y)
        for (int x = 50; x <= 52; ++x)
            theMap.m_a[y][x].SetType(CHex::forest);

    g_whse = CWarehouseBuilding();
    g_whse.m_hex = CHexCoord(WX, WY);
    g_whse.m_iTyp = CStructureData::warehouse;

    g_mill = CFarmBuilding();
    g_mill.m_hex = CHexCoord(MX, MY);
    g_mill.m_iTyp = CStructureData::lumber;
    g_mill.UpdateFarm();

    theBuildingMap.m_v.clear();
    theBuildingMap.m_v.push_back(std::make_pair((DWORD)1, (CBuilding*)&g_whse));
    theBuildingMap.m_v.push_back(std::make_pair((DWORD)2, (CBuilding*)&g_mill));
}

struct Mults { int forest, soil, iron, coal; };

// A fresh scan of the CURRENT map, through the same production functions, with no cache involved.
static Mults FreshMults(CBuilding const& b)
{
    Mults m;
    m.forest = CFarmBuilding::ForestMultAt(b.GetHex(), b.GetData()->GetType(), b.GetDir(), 100);
    m.soil   = CFarmBuilding::ScroungeSoilMultAt(b.GetHex(), b.GetData()->GetType(), b.GetDir(), 100);
    CFarmBuilding::ScrapMultsAt(b.GetHex(), b.GetData()->GetType(), b.GetDir(), &m.iron, &m.coal, 100);
    return m;
}

static Mults CachedMults(CWarehouseBuilding& w)
{
    Mults m;
    m.forest = w.GetScroungeForestMult();
    m.soil   = w.GetScroungeSoilMult();
    m.iron   = w.GetScroungeIronMult();
    m.coal   = w.GetScroungeCoalMult();
    return m;
}

static bool Same(Mults const& a, Mults const& b)
{
    return a.forest == b.forest && a.soil == b.soil && a.iron == b.iron && a.coal == b.coal;
}

static void Report(const char* tag, Mults const& m)
{
    std::printf("  %-22s forest %4d  soil %4d  iron %4d  coal %4d\n", tag, m.forest, m.soil, m.iron, m.coal);
}

// A four-line Scrounging def. The per-minute bases are chosen so each line sits just ABOVE its
// rounding boundary on the intact site and falls (or, for food, rises) across it once the forest
// is gone -- WinAstra's point that a single cut can hide inside MultiLinesFor's one rounding, so a
// rate-level test has to cut enough hexes to move the rounded number.
static AltOutput::AltOutputDef MakeDef()
{
    AltOutput::AltOutputDef def = {};
    def.m_aMulti[0] = { CMaterialTypes::lumber, 3 };
    def.m_aMulti[1] = { CMaterialTypes::food,   4 };
    def.m_aMulti[2] = { CMaterialTypes::iron,   3 };
    def.m_aMulti[3] = { CMaterialTypes::coal,   7 };
    def.m_nMulti = 4;
    def.m_bTerrainScaled = true;
    return def;
}

struct Rates { int lumber, food, iron, coal; };

static Rates RatesFor(CWarehouseBuilding& w, AltOutput::AltOutputDef const& def)
{
    AltOutput::AltMat a[AltOutput::kMaxMulti];
    int n = AltOutput::MultiLinesFor(&w, &def, a);
    Rates r = { -1, -1, -1, -1 };
    for (int i = 0; i < n; ++i)
    {
        if (a[i].m_iMat == CMaterialTypes::lumber) r.lumber = a[i].m_iPerMin;
        else if (a[i].m_iMat == CMaterialTypes::food) r.food = a[i].m_iPerMin;
        else if (a[i].m_iMat == CMaterialTypes::iron) r.iron = a[i].m_iPerMin;
        else if (a[i].m_iMat == CMaterialTypes::coal) r.coal = a[i].m_iPerMin;
    }
    return r;
}

int main()
{
    const AltOutput::AltOutputDef def = MakeDef();

    // -----------------------------------------------------------------------------------
    // 0. INSTRUMENT CHECK. Before asserting the cache is refreshed, prove this fixture can see a
    //    stale one: retype a hex with SlashHex alone (the terrain edit with no invalidation) and
    //    the getters must still hand back the pre-cut answer. If this check ever passes trivially,
    //    every "cache matches a fresh scan" assertion below is vacuous.
    {
        ResetScene();
        Mults before = CachedMults(g_whse);
        Report("intact (cached)", before);
        for (int x = 17; x <= 25; ++x)
            SlashHex(CHexCoord(x, 17));
        Mults fresh = FreshMults(g_whse);
        Report("after raw SlashHex", fresh);
        check(!Same(before, fresh), "instrument: nine raw cuts DO move a fresh scan");
        check(Same(CachedMults(g_whse), before), "instrument: an un-invalidated cache stays stale");
    }

    // -----------------------------------------------------------------------------------
    // 1. One cut through ApplySlash refreshes all four multipliers immediately -- and, separately,
    //    does NOT yet move the rounded rate. That second half is the reason a rate-level test must
    //    cut more than once.
    {
        ResetScene();
        Mults intact = CachedMults(g_whse);
        Rates rIntact = RatesFor(g_whse, def);
        std::printf("  intact rates           lumber %d  food %d  iron %d  coal %d\n",
                    rIntact.lumber, rIntact.food, rIntact.iron, rIntact.coal);

        check(CFarmBuilding::ApplySlash(CHexCoord(19, 18)) == TRUE, "one slash lands");
        Mults after = CachedMults(g_whse);
        check(Same(after, FreshMults(g_whse)), "one slash: cache == fresh scan");
        check(!Same(after, intact), "one slash: all-four cache actually moved");
        check(after.forest < intact.forest, "one slash: forest cover falls");
        check(after.iron < intact.iron, "one slash: scrap iron falls (forest fed it)");
        check(after.coal < intact.coal, "one slash: scrap coal falls (forest fed it)");
        check(after.soil > intact.soil, "one slash: scrounge soil moves (forest != plain FarmMult)");

        Rates rOne = RatesFor(g_whse, def);
        check(rOne.lumber == rIntact.lumber && rOne.food == rIntact.food
              && rOne.iron == rIntact.iron && rOne.coal == rIntact.coal,
              "one slash: the rounded RATE has not moved yet (why one cut is not a test)");
    }

    // -----------------------------------------------------------------------------------
    // 2. Enough cuts to cross a rate threshold on every one of the four lines.
    {
        ResetScene();
        Mults intact = CachedMults(g_whse);
        Rates rIntact = RatesFor(g_whse, def);

        int nCut = 0;
        for (int y = 17; y <= 19; ++y)
            for (int x = 17; x <= 25; ++x)
                if (CFarmBuilding::ApplySlash(CHexCoord(x, y)))
                    ++nCut;
        check_eq(nCut, 27, "whole forest band cleared");

        Mults after = CachedMults(g_whse);
        Mults fresh = FreshMults(g_whse);
        Report("after 27 slashes", after);
        check(Same(after, fresh), "27 slashes: cache == fresh scan on the cut terrain");
        check_eq(after.forest, 0, "27 slashes: forest ring is empty");

        Rates r = RatesFor(g_whse, def);
        std::printf("  rates  intact  lumber %d food %d iron %d coal %d\n",
                    rIntact.lumber, rIntact.food, rIntact.iron, rIntact.coal);
        std::printf("  rates  cut     lumber %d food %d iron %d coal %d\n",
                    r.lumber, r.food, r.iron, r.coal);
        check(r.lumber < rIntact.lumber, "rate threshold crossed: lumber");
        check(r.food > rIntact.food, "rate threshold crossed: food");
        check(r.iron < rIntact.iron, "rate threshold crossed: iron");
        check(r.coal < rIntact.coal, "rate threshold crossed: coal");

        // And the stale-cache counterfactual: the pre-cut cache would still be paying these.
        g_whse.m_iScrForest = intact.forest;
        g_whse.m_iScrSoil   = intact.soil;
        g_whse.m_iScrIron   = intact.iron;
        g_whse.m_iScrCoal   = intact.coal;
        Rates rStale = RatesFor(g_whse, def);
        check(rStale.lumber != r.lumber || rStale.food != r.food
              || rStale.iron != r.iron || rStale.coal != r.coal,
              "the defect would have been visible in the rate, not just the multiplier");
    }

    // -----------------------------------------------------------------------------------
    // 3. CONTROL: a cut far outside every ring must not touch this warehouse's cache at all.
    //    Poisoned values rather than a recomputation: an invalidate-and-rescan would land back on
    //    the same numbers and look identical to leaving them alone.
    {
        ResetScene();
        CachedMults(g_whse);                 // fill the cache
        g_whse.m_iScrForest = 777;
        g_whse.m_iScrSoil   = 778;
        g_whse.m_iScrIron   = 779;
        g_whse.m_iScrCoal   = 780;
        g_mill.m_iTerMult   = 781;

        check(CFarmBuilding::ApplySlash(CHexCoord(51, 51)) == TRUE, "the far cut lands");
        check_eq((int)g_whse.m_iScrForest, 777, "control: distant cut leaves forest cache");
        check_eq((int)g_whse.m_iScrSoil,   778, "control: distant cut leaves soil cache");
        check_eq((int)g_whse.m_iScrIron,   779, "control: distant cut leaves iron cache");
        check_eq((int)g_whse.m_iScrCoal,   780, "control: distant cut leaves coal cache");
        check_eq((int)g_mill.m_iTerMult,   781, "control: distant cut leaves the mill multiplier");
    }

    // -----------------------------------------------------------------------------------
    // 4. The mill refresh and the warehouse invalidation share one walk; neither may break the
    //    other. The band overlaps BOTH the mill's box and the warehouse's rings, so every cut runs
    //    an eager UpdateFarm and a lazy InvalidateScrounge in the same pass over theBuildingMap.
    {
        ResetScene();
        CachedMults(g_whse);
        LONG millBefore = g_mill.m_iTerMult;
        check(millBefore > 0, "the mill starts with forest in its box");

        // One shared-coverage cut first: (19,18) is in the mill box AND the warehouse forest ring.
        check(CFarmBuilding::ApplySlash(CHexCoord(19, 18)) == TRUE, "the shared-coverage cut lands");
        check_eq((int)g_mill.m_iTerMult,
                 CFarmBuilding::LandMult(g_mill.GetHex(), g_mill.GetData()->GetType(), g_mill.GetDir()),
                 "shared walk: the mill multiplier is recomputed, not skipped");
        check(Same(CachedMults(g_whse), FreshMults(g_whse)),
              "shared walk: the warehouse cache is current after the same cut");

        // Then the rest of the band, which is enough to move the mill's truncated multiplier too.
        for (int y = 17; y <= 19; ++y)
            for (int x = 17; x <= 25; ++x)
                CFarmBuilding::ApplySlash(CHexCoord(x, y));
        check_eq((int)g_mill.m_iTerMult,
                 CFarmBuilding::LandMult(g_mill.GetHex(), g_mill.GetData()->GetType(), g_mill.GetDir()),
                 "shared walk: the mill multiplier tracks the cleared band");
        check((int)g_mill.m_iTerMult < (int)millBefore, "shared walk: the mill multiplier actually fell");
        check(Same(CachedMults(g_whse), FreshMults(g_whse)),
              "shared walk: the warehouse cache is still current after the whole band");
    }

    // -----------------------------------------------------------------------------------
    // 5. A hex in the warehouse's FOREST ring but outside its scrap/soil rings (x = 17 vs
    //    x >= 18). All four still come back correct -- over-invalidation is harmless, and the
    //    narrower rings must not be left holding a value the wider one already knows is stale.
    {
        ResetScene();
        Mults intact = CachedMults(g_whse);
        check(CFarmBuilding::ApplySlash(CHexCoord(17, 19)) == TRUE, "the forest-ring-only cut lands");
        Mults after = CachedMults(g_whse);
        check(Same(after, FreshMults(g_whse)), "forest-ring-only cut: cache == fresh scan");
        check(after.forest < intact.forest, "forest-ring-only cut: forest cover fell");
        check_eq(after.iron, intact.iron, "forest-ring-only cut: scrap ring genuinely unaffected");
        check_eq(after.soil, intact.soil, "forest-ring-only cut: soil ring genuinely unaffected");
    }

    // -----------------------------------------------------------------------------------
    // 6. EXHAUSTIVE. For every hex in a window covering both buildings and their rings: cut it and
    //    demand the cache agrees with a fresh scan afterwards. This is the general form of the
    //    finding -- it fails for ANY hex whose four rings the reach test fails to cover, whatever
    //    the ring geometry, and it also fails if a cut is handled by invalidating only some of the
    //    four fields.
    {
        int nExercised = 0, nMoved = 0;
        for (int y = 8; y <= 34; ++y)
            for (int x = 8; x <= 34; ++x)
            {
                ResetScene();
                CHex* p = theMap._GetHex(CHexCoord(x, y));
                if (p->GetUnits() & (CHex::bldg | CHex::bridge))
                    continue;
                p->SetType(CHex::forest);

                Mults before = CachedMults(g_whse);   // fill the cache against the pre-cut map
                if (!CFarmBuilding::ApplySlash(CHexCoord(x, y)))
                    continue;
                ++nExercised;

                Mults fresh = FreshMults(g_whse);
                if (!Same(before, fresh))
                    ++nMoved;
                if (!Same(CachedMults(g_whse), fresh))
                {
                    ++g_fail;
                    std::fprintf(stderr, "FAIL: stale cache after cutting (%d,%d)\n", x, y);
                }
                ++g_checks;
            }
        std::printf("  exhaustive: %d hexes cut, %d of them moved at least one multiplier\n",
                    nExercised, nMoved);
        check(nExercised > 600, "exhaustive: the sweep actually cut hexes");
        check(nMoved > 0, "exhaustive: the sweep reached hexes that change the multipliers");
    }

    std::printf("%d checks, %d failures\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
