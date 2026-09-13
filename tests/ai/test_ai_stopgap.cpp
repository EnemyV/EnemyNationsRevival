// test_ai_stopgap.cpp
//
// Guards the "AI late-game stop-gap" changes in caigmgr.cpp (WinFable, 2026-07-05):
// a handful of flat difficulty caps replaced by time/wealth-scaled, hard-bounded
// ones, plus one runaway clamp (apartments) and a housing-priority yield.
// See discussion repo docs/design/ai-stopgap-late-game-playable.md (S1..S8).
//
// Two layers, on purpose:
//  (1) ARITHMETIC MIRROR -- pure copies of each capped expression, asserting the
//      intended bounds / saturation points. Documents "what the number must do".
//  (2) SOURCE LINT -- parses caigmgr.cpp and asserts the real expressions are
//      present (and the pre-change *frozen* forms are gone), so the mirror can't
//      silently drift from the shipped code and an accidental revert is caught.
//      Whitespace is squeezed out first, so reformatting can't break the match.
//
// Standalone: links no game code, touches no game build. Safe to run anytime.
//
// Usage: ai_stopgap_tests.exe [<path-to-caigmgr.cpp>]
//   The arithmetic mirror always runs; the source lint runs only when the path
//   is given and readable (skips cleanly otherwise, like the other AI tests).
// Exit:  0 all pass, 1 a check failed, 2 cannot-open the source (skip).

#define _CRT_SECURE_NO_WARNINGS

#include "microtest.h"

#include <cstdio>
#include <cstring>
#include <string>

// ---------------------------------------------------------------------------
// (1) Arithmetic mirror -- MUST match caigmgr.cpp exactly. The source lint in
//     part (2) keeps these copies honest against the shipped expressions.
// ---------------------------------------------------------------------------
namespace mirror {

// S7: extra fleet for a rich AI, bounded to +4. m_iWealthLevel is always >= 0.
static int WealthBonus(int wealth) { return wealth < 4 ? wealth : 4; }

// S1: apartment/office headroom multiplier, clamped to 6 (was unbounded).
static int AptMulti(int hoursPassed, int wealthLevel) {
    int multi = 3 + hoursPassed / 2;
    multi *= wealthLevel;
    if (multi > 6) multi = 6;
    return multi;
}

// S2: crane goal ceiling. base 2*iCnt, +1/hr up to +12, +up to +4 for wealth.
static int CraneCap(int iCnt, int hrs, int wealth) {
    return 2 * iCnt + (hrs < 12 ? hrs : 12) + WealthBonus(wealth);
}

// S3+S7: hauling-truck bonus, +1/hr up to +20, +2 per wealth up to +8.
static int TruckBonus(int hrs, int wealth) {
    return (hrs < 20 ? hrs : 20) + 2 * WealthBonus(wealth);
}

// S3: heavy-truck goal assignment (4*iCnt with a weapons factory, else 3*iCnt).
static int TruckGoal(int iCnt, int hrs, int wealth, bool hasWeaponsFac) {
    return (hasWeaponsFac ? 4 * iCnt : 3 * iCnt) + TruckBonus(hrs, wealth);
}

// S4: mine goal (oil/iron/coal), +1/hr up to +10 (was frozen at iCnt).
static int MineGoal(int iCnt, int hrs) {
    return iCnt + (hrs < 10 ? hrs : 10);
}

// S8: apartment construction-task priority tier.
//  99 = real shortage, workers unhoused -> top priority
//   1 = rich-nation expansion -> last (builds only with spare cranes)
static int AptPriority(bool aptCritical) {
    return aptCritical ? 99 : 1;
}

} // namespace mirror

using namespace mirror;

// ---- S1 apartment/office clamp ----
static void test_apt_multi_clamp() {
    // never exceeds 6, however rich the AI or however long the game runs
    CHECK_EQ(AptMulti(0, 3), 6);    // (3+0)*3 = 9  -> 6
    CHECK_EQ(AptMulti(40, 8), 6);   // huge         -> 6
    CHECK_EQ(AptMulti(6, 1), 6);    // (3+3)*1 = 6  exactly (boundary)
    // a poor-but-present economy still gets its baseline headroom, un-clamped
    CHECK_EQ(AptMulti(0, 1), 3);    // (3+0)*1 = 3
    CHECK_EQ(AptMulti(2, 1), 4);    // (3+1)*1 = 4
    // for any wealth>=1 the multiplier stays within [3,6] -- no runaway
    for (int h = 0; h <= 60; ++h)
        for (int w = 1; w <= 8; ++w) {
            int m = AptMulti(h, w);
            CHECK(m >= 3 && m <= 6);
        }
}

// ---- S7 wealth bonus ----
static void test_wealth_bonus_bounded() {
    CHECK_EQ(WealthBonus(0), 0);    // poor AI gets nothing -> never over-extends
    CHECK_EQ(WealthBonus(3), 3);
    CHECK_EQ(WealthBonus(4), 4);
    CHECK_EQ(WealthBonus(100), 4);  // saturates at +4
}

// ---- S2 crane cap ----
static void test_crane_cap() {
    const int iCnt = 3;  // m_iSmart+1 on a mid difficulty
    // hour 0, poor: exactly the old flat cap 2*iCnt -- no early-game change
    CHECK_EQ(CraneCap(iCnt, 0, 0), 2 * iCnt);
    // +1/hr, saturating at +12
    CHECK_EQ(CraneCap(iCnt, 12, 0), 2 * iCnt + 12);
    CHECK_EQ(CraneCap(iCnt, 999, 0), 2 * iCnt + 12);        // never past +12
    // wealth adds up to +4 on top of the time term
    CHECK_EQ(CraneCap(iCnt, 999, 99), 2 * iCnt + 12 + 4);
    // the whole bonus above the flat base is hard-bounded to +16
    for (int h = 0; h <= 60; ++h)
        for (int w = 0; w <= 20; ++w) {
            int over = CraneCap(iCnt, h, w) - 2 * iCnt;
            CHECK(over >= 0 && over <= 16);
        }
}

// ---- S3 truck goal (the ASSIGNMENT -- not the old no-op clamp) ----
static void test_truck_goal() {
    const int iCnt = 3;
    // hour 0, poor: exactly the old flat assignment -- no early-game change
    CHECK_EQ(TruckGoal(iCnt, 0, 0, true),  4 * iCnt);
    CHECK_EQ(TruckGoal(iCnt, 0, 0, false), 3 * iCnt);
    // +1/hr up to +20, +2/wealth up to +8 -> max bonus +28
    CHECK_EQ(TruckGoal(iCnt, 20, 0, true),   4 * iCnt + 20);
    CHECK_EQ(TruckGoal(iCnt, 999, 0, true),  4 * iCnt + 20);      // hrs saturates
    CHECK_EQ(TruckGoal(iCnt, 999, 99, true), 4 * iCnt + 20 + 8);
    // bonus above base bounded to [0,28]; identical across the factory branches
    for (int h = 0; h <= 60; ++h)
        for (int w = 0; w <= 20; ++w) {
            int over4 = TruckGoal(iCnt, h, w, true)  - 4 * iCnt;
            int over3 = TruckGoal(iCnt, h, w, false) - 3 * iCnt;
            CHECK(over4 >= 0 && over4 <= 28);
            CHECK(over3 == over4);   // same bonus regardless of the factory branch
        }
}

// ---- S4 mine goals ----
static void test_mine_goal() {
    const int iCnt = 3;
    CHECK_EQ(MineGoal(iCnt, 0),   iCnt);        // hour 0: unchanged
    CHECK_EQ(MineGoal(iCnt, 10),  iCnt + 10);   // +1/hr...
    CHECK_EQ(MineGoal(iCnt, 999), iCnt + 10);   // ...saturating at +10
    for (int h = 0; h <= 60; ++h) {
        int over = MineGoal(iCnt, h) - iCnt;
        CHECK(over >= 0 && over <= 10);
    }
}

// ---- S8 housing-priority tier ----
static void test_apt_priority_tier() {
    // a genuine shortage (workers unhoused) is top priority
    CHECK_EQ(AptPriority(/*critical=*/true), 99);
    // rich-nation expansion is last -> below every productive building, spare cranes only
    CHECK_EQ(AptPriority(false), 1);
}

// ---------------------------------------------------------------------------
// (2) Source lint -- the shipped caigmgr.cpp must carry these expressions, so
//     the mirror above cannot drift and a revert to the flat caps is caught.
// ---------------------------------------------------------------------------
namespace {

std::string ReadAll(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return std::string();
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string s((size_t)(n > 0 ? n : 0), '\0');
    if (n > 0 && std::fread(&s[0], 1, (size_t)n, f) != (size_t)n) s.clear();
    std::fclose(f);
    return s;
}

// strip all ASCII whitespace so the needles are format-insensitive
std::string NoWs(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') o.push_back(c);
    }
    return o;
}

bool Has(const std::string& hay, const char* needle) {
    return hay.find(needle) != std::string::npos;
}

} // namespace

// The live expressions must be present (whitespace-squeezed source).
static void test_source_carries_stopgap(const std::string& sq) {
    // shared elapsed-hours local that scales S2/S3/S4
    CHECK(Has(sq, "inthrs=theGame.GetElapsedSeconds()/3600;"));
    // S1 apartment/office runaway clamp
    CHECK(Has(sq, "if(multi>6)multi=6;"));
    // S7 wealth cache + bounded bonus
    CHECK(Has(sq, "m_iWealthLevel=wealthLevel;"));
    CHECK(Has(sq, "intwealthBonus=(m_iWealthLevel<4?m_iWealthLevel:4);"));
    // S2 crane cap (time + wealth scaled)
    CHECK(Has(sq, "intcraneCap=2*iCnt+(hrs<12?hrs:12)+wealthBonus;"));
    // S3 truck bonus rides the ASSIGNMENT (both factory branches) + raised clamp
    CHECK(Has(sq, "inttruckBonus=(hrs<20?hrs:20)+2*wealthBonus;"));
    CHECK(Has(sq, "heavy_truck]=(4*iCnt)+truckBonus;"));
    CHECK(Has(sq, "heavy_truck]=(3*iCnt)+truckBonus;"));
    CHECK(Has(sq, "heavy_truck]>(4*iCnt)+truckBonus)"));
    // S4 mine goals unfrozen
    CHECK(Has(sq, "intmineBonus=(hrs<10?hrs:10);"));
    CHECK(Has(sq, "oil_well]=iCnt+mineBonus;"));
    CHECK(Has(sq, "iron]=iCnt+mineBonus;"));
    CHECK(Has(sq, "coal]=iCnt+mineBonus;"));
    // S5 power headroom
    CHECK(Has(sq, "m_pwaBldgs[iBldg]>pGameData->m_iSmart+4)"));
    // S6 smelter bump
    CHECK(Has(sq, "smelter]+=iCnt;"));
    // S8 critical cache + expansion housing tiered last (99 critical / 1 expansion)
    CHECK(Has(sq, "m_bAptCritical=m_iNeedApt;"));
    CHECK(Has(sq, "if(m_bAptCritical)"));
    CHECK(Has(sq, "pTask->SetPriority((BYTE)1);"));
    // advanced factories (assault/heavy/dread) below goal build ahead of surplus housing
    CHECK(Has(sq, "pTask->SetPriority((BYTE)90);"));
}

// The pre-change *frozen* forms must be GONE -- catches a revert that would
// leave the mirror passing while the shipped code drops back to the flat caps.
static void test_source_has_no_frozen_forms(const std::string& sq) {
    CHECK(!Has(sq, "oil_well]=iCnt;"));         // S4 was frozen at iCnt
    CHECK(!Has(sq, "smelter]+=(iCnt/2);"));     // S6 was the anemic half-bump
    CHECK(!Has(sq, "heavy_truck]=(4*iCnt);"));  // S3 old flat assignment
    CHECK(!Has(sq, "heavy_truck]=(3*iCnt);"));
}

int main(int argc, char** argv) {
    // arithmetic mirror -- always runs, needs no external input
    test_apt_multi_clamp();
    test_wealth_bonus_bounded();
    test_crane_cap();
    test_truck_goal();
    test_mine_goal();
    test_apt_priority_tier();

    // source lint -- needs the caigmgr.cpp path (skip cleanly if absent)
    if (argc < 2) {
        std::printf("[ai_stopgap] no source path given -- lint SKIPPED\n");
        return microtest::Summary();
    }
    std::string src = ReadAll(argv[1]);
    if (src.empty()) {
        std::printf("[ai_stopgap] SKIP: cannot read %s\n", argv[1]);
        return 2;
    }
    std::string sq = NoWs(src);
    std::printf("[ai_stopgap] linting %s (%d bytes)\n", argv[1], (int)src.size());
    test_source_carries_stopgap(sq);
    test_source_has_no_frozen_forms(sq);

    return microtest::Summary();
}
