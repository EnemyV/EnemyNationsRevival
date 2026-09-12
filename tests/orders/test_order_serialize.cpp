// test_order_serialize.cpp
//
// #38 order queue, save-stream half. Two things are checked:
//
//  1. the MODEL (order_model.h): a v7 stream is byte-for-byte the shipped stream, and
//     a v8 stream carries the order payload and the loop flag and round-trips;
//  2. the PRODUCTION SOURCE: a lint over new_unit.cpp / vehicle.h / version.h, so a
//     model that has drifted away from the code it mirrors is caught here rather than
//     by a save that will not load.
//
// Build & run via run-order-tests.ps1. Links no game code.

#include "order_model.h"
#include "../ai/microtest.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace orders;

// ---------------------------------------------------------------------------
// 1. v7 is unchanged, byte for byte
// ---------------------------------------------------------------------------
static void test_v7_stream_is_the_shipped_stream() {
    // every entry kind, including order kinds with a payload set: at counter 7 the
    // payload must not reach the stream at all
    Route rs[] = {
        Route(Hex(0, 0), waypoint),
        Route(Hex(12, 34), unload),
        Route(Hex(-5, 1000), load),
        Route(Hex(7, 9), build, 17, 3),
        Route(Hex(1, 2), build_road, 0, 0),
        Route(Hex(300, 400), repair, 0, 0),
    };

    for (int i = 0; i < 6; ++i) {
        Ar aOld, aNew;
        RouteStoreShipped(aOld, rs[i]);
        RouteStore(aNew, rs[i], 7);
        CHECK_EQ(aNew.buf.size(), aOld.buf.size());
        CHECK(aNew.buf == aOld.buf);
    }

    // the same for a whole vehicle route block, cursor in every position
    for (int cur = CUR_STALE; cur <= 2; ++cur) {
        VehRoute v;
        v.route.push_back(Route(Hex(1, 1), waypoint));
        v.route.push_back(Route(Hex(2, 2), build, 9, 1));
        v.route.push_back(Route(Hex(3, 3), unload));
        v.cursor = cur;
        v.loop   = false;                 // would flip a byte if it were written

        Ar aNew;
        VehRouteStore(aNew, v, 7);

        // hand-built expectation: count, three SHIPPED entries, the cursor word
        Ar aExp;
        aExp.put16(3);
        for (int i = 0; i < 3; ++i)
            RouteStoreShipped(aExp, v.route[i]);
        int iPos = 0;
        if ((cur >= 0) && (cur < 3))
            iPos = cur;
        else if (cur == CUR_NULL)
            // BUGS #99 (this integration) replaced #88's N-1 with an out-of-range
            // sentinel, and writes it UNCONDITIONALLY. So this is the one field of a
            // counter-7 stream that is no longer byte-for-byte the shipped stream - and
            // the difference is unreachable, because the writer always stamps counter 8
            // (VER_RELEASE), and a counter <= 7 LOADER ignores the sentinel and walks to
            // the tail, which is the N-1 answer it always gave. Every entry byte, and the
            // cursor word for every non-NULL cursor, is still identical.
            iPos = (int)ROUTE_POS_NONE;
        aExp.put16((unsigned)iPos);

        CHECK(aNew.buf == aExp.buf);
    }
}

// ---------------------------------------------------------------------------
// 2. v8 carries the payload and round-trips
// ---------------------------------------------------------------------------
static void test_v8_payload_round_trip() {
    Route in(Hex(101, -202), build, 23, 2);
    Ar    ar;
    RouteStore(ar, in, 8);

    // exactly five bytes more than v7: bldg type, dir, and the end hex
    Ar v7;
    RouteStore(v7, in, 7);
    CHECK_EQ(ar.buf.size(), v7.buf.size() + 6);

    Route out;
    RouteLoad(ar, out, 8);
    CHECK(ar.Drained());
    CHECK(out.hex == in.hex);
    CHECK_EQ(out.type, in.type);
    CHECK_EQ(out.bldgType, in.bldgType);
    CHECK_EQ(out.dir, in.dir);
    CHECK(out.hexEnd == in.hexEnd);

    // a road order keeps BOTH ends
    Route road(Hex(4, 5), build_road);
    road.hexEnd = Hex(40, 50);
    Ar ar2;
    RouteStore(ar2, road, 8);
    Route out2;
    RouteLoad(ar2, out2, 8);
    CHECK(ar2.Drained());
    CHECK(out2.hex == Hex(4, 5));
    CHECK(out2.hexEnd == Hex(40, 50));
}

// A v7 stream read by the new reader must leave the payload ZERO and the end hex
// equal to the stop's hex -- never garbage read out of the next field.
static void test_v7_load_zeroes_the_payload() {
    Route in(Hex(9, 9), waypoint);
    in.bldgType = 0;
    Ar ar;
    RouteStoreShipped(ar, in);

    Route out;
    out.bldgType = 77;                    // poisoned, to prove it is overwritten
    out.dir      = 88;
    out.hexEnd   = Hex(-1, -1);
    RouteLoad(ar, out, 7);
    CHECK(ar.Drained());
    CHECK_EQ(out.bldgType, 0);
    CHECK_EQ(out.dir, 0);
    CHECK(out.hexEnd == out.hex);
}

// ---------------------------------------------------------------------------
// 3. the loop flag (BUGS #95)
// ---------------------------------------------------------------------------
static void test_route_loop_round_trip() {
    // v8: written and read, both ways round
    for (int b = 0; b < 2; ++b) {
        VehRoute v;
        v.route.push_back(Route(Hex(1, 2), build, 5, 0));
        v.route.push_back(Route(Hex(3, 4), build, 6, 1));
        v.cursor = 1;
        v.loop   = (b != 0);

        Ar ar;
        VehRouteStore(ar, v, 8);
        VehRoute w;
        VehRouteLoad(ar, w, 8);
        CHECK(ar.Drained());
        CHECK_EQ(w.loop ? 1 : 0, b);
        CHECK_EQ((int)w.route.size(), 2);
        CHECK_EQ(w.cursor, 1);
        CHECK_EQ(w.route[0].bldgType, 5);
        CHECK_EQ(w.route[1].dir, 1);
    }

    // v7: the flag is not in the stream, so a loaded route keeps the ctor default
    // (TRUE). That IS bug #95 and it is why the flag only becomes trustworthy at 8.
    VehRoute v;
    v.route.push_back(Route(Hex(1, 2), waypoint));
    v.loop   = false;
    v.cursor = 0;
    Ar ar;
    VehRouteStore(ar, v, 7);
    VehRoute w;                            // fresh: loop == TRUE by ctor
    VehRouteLoad(ar, w, 7);
    CHECK(ar.Drained());
    CHECK(w.loop);
}

// ---------------------------------------------------------------------------
// 4. the #88 cursor arithmetic still holds with the payload attached
// ---------------------------------------------------------------------------
static void test_cursor_survives_the_payload() {
    const int sizes[] = { 1, 2, 3, 7, 32 };
    for (int s = 0; s < 5; ++s) {
        int n = sizes[s];
        for (int cur = CUR_STALE; cur < n; ++cur) {
            VehRoute v;
            for (int i = 0; i < n; ++i)
                v.route.push_back(Route(Hex(i, i * 2), build, i & 0x7f, i & 3));
            v.cursor = cur;
            v.loop   = false;

            Ar ar;
            VehRouteStore(ar, v, 8);
            VehRoute w;
            VehRouteLoad(ar, w, 8);
            CHECK(ar.Drained());
            CHECK_EQ((int)w.route.size(), n);

            int expect = cur;
            if (cur == CUR_NULL)
                expect = CUR_NULL;         // #99: a NULL cursor round-trips as NO cursor
            else if (cur == CUR_STALE)
                expect = 0;                // a cursor in no node stores 0
            CHECK_EQ(w.cursor, expect);

            for (int i = 0; i < n; ++i) {
                CHECK(w.route[i].hex == Hex(i, i * 2));
                CHECK_EQ(w.route[i].bldgType, i & 0x7f);
                CHECK_EQ(w.route[i].dir, i & 3);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 5. source lint -- the model claims these lines exist; check that they do
// ---------------------------------------------------------------------------
static bool ReadFile(const char *path, std::string &out) {
    FILE *f = std::fopen(path, "rb");
    if (!f)
        return false;
    char   buf[8192];
    size_t n;
    out.clear();
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
        out.append(buf, n);
    std::fclose(f);
    return true;
}

static void lint_needs(std::string const &src, const char *what, const char *desc) {
    ++microtest::Checks();
    if (src.find(what) == std::string::npos) {
        ++microtest::Fails();
        std::printf("FAIL lint: %s -- not found: %s\n", desc, what);
    }
}

static void lint_forbids(std::string const &src, const char *what, const char *desc) {
    ++microtest::Checks();
    if (src.find(what) != std::string::npos) {
        ++microtest::Fails();
        std::printf("FAIL lint: %s -- still present: %s\n", desc, what);
    }
}

static int test_source_lint(const char *newUnit, const char *vehicleH, const char *versionH,
                            const char *unitCpp, const char *vehicleCpp, const char *areaCpp) {
    std::string s;

    if (!ReadFile(newUnit, s)) {
        std::printf("[orders] SKIP source lint (cannot open %s)\n", newUnit);
        return 2;
    }
    lint_needs(s, "ar << m_iBldgType << m_iDir << m_hexEnd;", "v8 payload is written");
    lint_needs(s, "ar >> m_iBldgType >> m_iDir >> m_hexEnd;", "v8 payload is read");
    lint_needs(s, "ar << (BYTE)( m_bRouteLoop ? 1 : 0 );", "v8 loop flag is written");
    lint_needs(s, "m_bRouteLoop = bLoop ? TRUE : FALSE;", "v8 loop flag is read");
    lint_needs(s, "theGame.m_dwVer >= 8", "the payload sits behind a v8 gate");
    lint_forbids(s, "TRAP( m_route.GetCount( ) > 0 );", "the obsolete serialize TRAP is retired");
    lint_needs(s, "EN_TRAP_REMOVED( \"CVehicle::Serialize", "and retired with the house pattern");
    // #88's writer must still be the pre-advance compare; #99 (landed in this
    // integration) replaced #88's N-1 restore with an out-of-range sentinel, honoured on
    // load only at counter >= 8. Both halves are pinned so the model cannot drift back.
    lint_needs(s, "if ( m_pos == NULL )", "the NULL-cursor case is still handled explicitly");
    lint_needs(s, "iPos = (int)wROUTE_POS_NONE;", "#99's sentinel is what a NULL cursor stores");
    lint_needs(s, "( theGame.m_dwVer >= 8 ) && ( wNR == wROUTE_POS_NONE )",
               "and the loader honours it only on a counter >= 8 save");

    if (!ReadFile(vehicleH, s)) {
        std::printf("[orders] SKIP vehicle.h lint (cannot open %s)\n", vehicleH);
        return 2;
    }
    lint_needs(s, "enum { waypoint, unload, load, build, build_road, repair };", "the order kinds");
    lint_needs(s, "static BOOL\t\tIsOrder (int iType) { return (iType >= build); }", "IsOrder");
    lint_needs(s, "m_iBldgType;", "the payload member");
    lint_needs(s, "m_hexEnd;", "the road end member");

    if (!ReadFile(versionH, s)) {
        std::printf("[orders] SKIP version.h lint (cannot open %s)\n", versionH);
        return 2;
    }
    // The counter is 8 in this integration (the traffic series' format-8 save plus
    // BUGS #99), so the v8 gates the order payload sits behind are now LIVE: a save
    // written by this build carries the payload and the loop flag. Pinned, because the
    // serialize model's v7-versus-v8 claims are only meaningful against a known counter.
    lint_needs(s, "#define         VER_RELEASE     8", "VER_RELEASE is 8 in this integration");

    // The dispatcher half of the model has no byte stream to compare against, so these
    // pin the two invariants a fixture cannot see: one list never holds both kinds, and
    // a repair order is validated before it is dispatched.
    if (!ReadFile(unitCpp, s)) {
        std::printf("[orders] SKIP unit.cpp lint (cannot open %s)\n", unitCpp);
        return 2;
    }
    lint_needs(s, "if ( !CRoute::IsOrder( iType ) )", "SetLocation drops orders on a movement append");

    if (!ReadFile(vehicleCpp, s)) {
        std::printf("[orders] SKIP vehicle.cpp lint (cannot open %s)\n", vehicleCpp);
        return 2;
    }
    lint_needs(s, "RepairTargetLives(pR->GetCoord())", "NextOrder validates a repair target first");
    lint_needs(s, "BOOL CVehicle::RepairTargetLives", "and the predicate exists");
    // bug #114: the completion half must sit OUTSIDE the idle branch, and the stall
    // watch must exist and be called from it.
    lint_needs(s, "void CVehicle::CheckOrderStall", "the stall watch exists");
    lint_needs(s, "            CheckOrderStall();", "and the idle branch calls it");
    // The completion half must sit in Operate BEFORE the mode switch. Its comment
    // banner is the stable anchor; the `case stop` block that used to hold it now says
    // so in as many words.
    lint_needs(s, "// #38 ORDER QUEUE, COMPLETION HALF (bug #114).",
               "completion is read before the mode switch");
    lint_needs(s, "// The COMPLETION half now runs before the switch",
               "and the idle branch no longer does it");

    if (!ReadFile(newUnit, s)) {
        std::printf("[orders] SKIP new_unit.cpp #114 lint (cannot open %s)\n", newUnit);
        return 2;
    }
    lint_needs(s, "pVeh->m_bFlags &= ~told_ai_stop;",
               "StopConstruction drops the arrival's stale told_ai_stop");

    if (!ReadFile(areaCpp, s)) {
        std::printf("[orders] SKIP area.cpp lint (cannot open %s)\n", areaCpp);
        return 2;
    }
    lint_needs(s, "static BOOL HasMoveStops( CVehicle* pVeh )", "the move-stop test");
    lint_needs(s, "StopRoute( pVehBuild );", "a build order takes the list over");
    lint_needs(s, "StopRoute( pVehQ );", "a road order takes the list over");
    lint_needs(s, "StopRoute( pVehR );", "a repair order takes the list over");
    return 0;
}

int main(int argc, char **argv) {
    std::printf("[orders] serialize suite\n");

    test_v7_stream_is_the_shipped_stream();
    test_v8_payload_round_trip();
    test_v7_load_zeroes_the_payload();
    test_route_loop_round_trip();
    test_cursor_survives_the_payload();

    if (argc >= 7) {
        int rc = test_source_lint(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
        if (rc == 2)
            std::printf("[orders] (source lint skipped)\n");
    } else {
        std::printf("[orders] (no source paths given -- model-only run)\n");
    }

    return microtest::Summary();
}
