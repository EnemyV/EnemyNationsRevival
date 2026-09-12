// test_order_dispatch.cpp
//
// #38 order queue, dispatcher half: the state machine in vehicle.cpp, mirrored in
// order_model.h. Covers the cases the design names -- idle dispatches, busy appends,
// completion advances, destruction drops, a server rejection drops only the order it
// is for, a non-local owner never dispatches -- plus the Shift-append versus
// plain-replace split.
//
// Build & run via run-order-tests.ps1. Links no game code.

#include "order_model.h"
#include "../ai/microtest.h"

#include <cstdio>

using namespace orders;

// Drive a whole successful build job on the vehicle at the head of its queue.
static void RunOneBuildToCompletion(Veh &v) {
    v.ArriveAndSend();    // crane reaches the site, sends CMsgBuildBldg
    v.ServerAccept();     // server says yes, StartConst attaches the site
    v.WorkTick();         // ConstructBuilding ticks
    v.SiteGone();         // the building finishes -> StopConstruction detaches us
}

// ---------------------------------------------------------------------------
// idle + first order dispatches
// ---------------------------------------------------------------------------
static void test_idle_first_order_dispatches() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    CHECK_EQ((int)v.route.size(), 1);
    CHECK_EQ(v.cursor, 0);

    CHECK(v.NextOrder());                 // the Shift-place calls this straight away
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_build);
    CHECK_EQ(v.mode, md_moving);
    CHECK_EQ(v.orderKind, build);
    CHECK(v.orderHex == Hex(10, 10));
    CHECK_EQ((int)v.route.size(), 1);     // dispatch does NOT consume the order
}

// An order queue is one-shot even though m_bRouteLoop defaults TRUE: a looping
// build order would rebuild the same site for ever.
static void test_append_forces_one_shot() {
    Veh v;
    CHECK(v.loop);                        // ctor / every pre-8 save
    v.AddOrder(Hex(1, 1), build, 2, 0);
    CHECK(!v.loop);
}

// ---------------------------------------------------------------------------
// busy appends, never dispatches
// ---------------------------------------------------------------------------
static void test_busy_appends_only() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    CHECK(v.NextOrder());

    // second and third Shift-places while the crane is on its way
    v.AddOrder(Hex(20, 20), build, 4, 0);
    CHECK(!v.NextOrder());
    v.AddOrder(Hex(30, 30), build, 4, 0);
    CHECK(!v.NextOrder());

    CHECK_EQ((int)v.route.size(), 3);
    CHECK_EQ(v.cursor, 0);
    CHECK(v.orderHex == Hex(10, 10));     // still on the first

    // and the idle poll cannot start a second one during the request window either
    v.ArriveAndSend();                    // event none + stopped, request in flight
    CHECK_EQ(v.mode, md_stop);
    CHECK_EQ(v.event, ev_none);
    CHECK(!v.PollIdle());                 // <- the whole reason m_iOrderState exists
    CHECK_EQ(v.dispatches, 1);
}

// ---------------------------------------------------------------------------
// completion advances, in order
// ---------------------------------------------------------------------------
static void test_completion_advances_in_order() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 1);
    v.AddOrder(Hex(30, 30), build, 6, 2);
    CHECK(v.NextOrder());

    RunOneBuildToCompletion(v);
    CHECK(v.PollIdle());                  // consumes #1 and starts #2
    CHECK_EQ((int)v.route.size(), 2);
    CHECK(v.route[0].hex == Hex(20, 20));
    CHECK(v.orderHex == Hex(20, 20));
    CHECK_EQ(v.state, order_sent);

    RunOneBuildToCompletion(v);
    CHECK(v.PollIdle());                  // consumes #2 and starts #3
    CHECK_EQ((int)v.route.size(), 1);
    CHECK(v.orderHex == Hex(30, 30));

    RunOneBuildToCompletion(v);
    CHECK(!v.PollIdle());                 // consumes #3, nothing left
    CHECK_EQ((int)v.route.size(), 0);
    CHECK_EQ(v.cursor, CUR_NULL);
    CHECK_EQ(v.state, order_none);
    CHECK_EQ(v.dispatches, 3);            // exactly one request per build
}

// ---------------------------------------------------------------------------
// site destroyed mid-build: the order is DROPPED, the queue carries on
// ---------------------------------------------------------------------------
static void test_destroyed_site_drops_the_order() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 4, 0);
    CHECK(v.NextOrder());

    v.ArriveAndSend();
    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();                          // the half-built site was destroyed

    CHECK(v.PollIdle());
    CHECK_EQ((int)v.route.size(), 1);      // dropped, not retried
    CHECK(v.route[0].hex == Hex(20, 20));
    CHECK(v.orderHex == Hex(20, 20));
}

// A crane that never reached order_work (still travelling) and then loses its job some
// other way must not leave the queue wedged: the road/give-up paths call OrderEnded.
static void test_order_ended_releases_the_queue() {
    Veh v;
    v.AddOrder(Hex(5, 5), build_road, 0, 0);
    v.AddOrder(Hex(9, 9), build, 3, 0);
    CHECK(v.NextOrder());
    CHECK_EQ(v.event, ev_build_road);

    v.ArriveAndSend();
    v.RoadTick();                          // ConstructRoad: the road work state
    CHECK_EQ(v.state, order_road);

    // between hexes the crane is briefly idle again -- order_road must NOT be read as
    // "job over" (that is what a road-specific work state buys)
    v.event = ev_none;
    v.mode  = md_stop;
    CHECK(!v.PollIdle());
    CHECK_EQ((int)v.route.size(), 2);

    v.OrderEnded();                        // NextRoadHex: the run reached its far end
    v.event = ev_none;
    v.mode  = md_stop;
    CHECK(v.PollIdle());
    CHECK_EQ((int)v.route.size(), 1);
    CHECK(v.route[0].hex == Hex(9, 9));
}

// ---------------------------------------------------------------------------
// a rejection drops the order it is FOR, and only that one
// ---------------------------------------------------------------------------
static void test_server_error_drops_only_the_matching_order() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 0);
    CHECK(v.NextOrder());
    v.ArriveAndSend();

    // a rejection for a DIFFERENT request (a stale message, or a plain placement the
    // player made elsewhere) must not eat a queued order
    v.OrderFailed(Hex(99, 99), 4);
    CHECK_EQ((int)v.route.size(), 2);
    CHECK_EQ(v.state, order_none);

    // ... and the same hex but the wrong building type is also not a match
    v.state = order_sent;
    v.OrderFailed(Hex(10, 10), 7);
    CHECK_EQ((int)v.route.size(), 2);

    // the real one
    v.state = order_sent;
    v.OrderFailed(Hex(10, 10), 4);
    CHECK_EQ((int)v.route.size(), 1);
    CHECK(v.route[0].hex == Hex(20, 20));
    CHECK_EQ(v.state, order_none);

    // and the crane is free, so the next poll starts the survivor
    CHECK(v.PollIdle());
    CHECK(v.orderHex == Hex(20, 20));
    CHECK_EQ(v.dispatches, 1);             // the failed one sent exactly one request
}

// A rejection arriving when nothing was in flight must not drop anything.
static void test_late_error_after_the_state_moved_on() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    CHECK(v.NextOrder());
    v.ArriveAndSend();
    v.ServerAccept();
    v.WorkTick();                          // the build is already running

    v.OrderFailed(Hex(10, 10), 4);         // a late duplicate
    CHECK_EQ((int)v.route.size(), 1);      // nothing dropped
}

// ---------------------------------------------------------------------------
// owner authority
// ---------------------------------------------------------------------------
static void test_non_local_owner_never_dispatches() {
    Veh v;
    v.local = false;
    v.AddOrder(Hex(10, 10), build, 4, 0);

    CHECK(!v.NextOrder());
    CHECK_EQ(v.state, order_none);
    CHECK_EQ(v.dispatches, 0);
    CHECK_EQ(v.event, ev_none);

    // the idle poll is behind the same gate
    CHECK(!v.PollIdle());
    CHECK_EQ(v.dispatches, 0);

    // even with a remote copy that somehow reached a work state
    v.state = order_work;
    v.site  = false;
    CHECK(!v.PollIdle());
    CHECK_EQ(v.state, order_work);         // untouched: the gate returns first
    CHECK_EQ((int)v.route.size(), 1);
}

// ---------------------------------------------------------------------------
// Shift-append versus plain-replace
// ---------------------------------------------------------------------------
static void test_shift_appends_plain_replaces() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 4, 0);
    v.AddOrder(Hex(30, 30), build, 4, 0);
    CHECK_EQ((int)v.route.size(), 3);      // three Shift-places APPEND

    CHECK(v.NextOrder());
    CHECK_EQ(v.state, order_sent);

    v.ClearOrders();                       // a plain placement REPLACES
    CHECK_EQ((int)v.route.size(), 0);
    CHECK_EQ(v.cursor, CUR_NULL);
    CHECK_EQ(v.state, order_none);         // and frees the crane for the new job
}

// ClearOrders must leave MOVEMENT stops alone -- they belong to the route feature.
static void test_clear_orders_keeps_movement_stops() {
    Veh v;
    v.route.push_back(Route(Hex(1, 1), waypoint));
    v.route.push_back(Route(Hex(2, 2), build, 4, 0));
    v.route.push_back(Route(Hex(3, 3), unload));
    v.route.push_back(Route(Hex(4, 4), repair));
    v.route.push_back(Route(Hex(5, 5), load));
    v.cursor = 2;                          // on the unload stop

    v.ClearOrders();
    CHECK_EQ((int)v.route.size(), 3);
    CHECK(v.route[0].hex == Hex(1, 1));
    CHECK(v.route[1].hex == Hex(3, 3));
    CHECK(v.route[2].hex == Hex(5, 5));
    CHECK_EQ(v.cursor, 1);                 // still on the SAME stop, at its new index
}

// The cursor sitting on an order that is removed must land on the head, never on a
// freed node.
static void test_clear_orders_moves_a_cursor_off_a_removed_order() {
    Veh v;
    v.route.push_back(Route(Hex(1, 1), waypoint));
    v.route.push_back(Route(Hex(2, 2), build, 4, 0));
    v.cursor = 1;
    v.ClearOrders();
    CHECK_EQ((int)v.route.size(), 1);
    CHECK_EQ(v.cursor, 0);

    Veh w;                                 // and with nothing left, no cursor at all
    w.AddOrder(Hex(2, 2), build, 4, 0);
    w.ClearOrders();
    CHECK_EQ((int)w.route.size(), 0);
    CHECK_EQ(w.cursor, CUR_NULL);
}

// ---------------------------------------------------------------------------
// the dispatcher leaves the 1996 route machinery alone
// ---------------------------------------------------------------------------
static void test_movement_stop_at_the_cursor_is_not_dispatched() {
    Veh v;
    v.route.push_back(Route(Hex(1, 1), waypoint));
    v.route.push_back(Route(Hex(2, 2), build, 4, 0));
    v.cursor = 0;

    // A stopped vehicle with waypoints left must NOT be restarted by the order poll:
    // ArrivedDest owns movement stops.
    CHECK(!v.NextOrder());
    CHECK(!v.PollIdle());
    CHECK_EQ(v.event, ev_none);
    CHECK_EQ(v.dispatches, 0);
    CHECK_EQ((int)v.route.size(), 2);
}

// A plain, unqueued build (nothing on the list) runs the same state machine and must
// not touch a movement waypoint that happens to sit at the order's default hex.
static void test_unqueued_build_leaves_waypoints_alone() {
    Veh v;
    v.route.push_back(Route(Hex(0, 0), waypoint));   // same hex as m_hexOrder's default
    v.cursor    = 0;
    v.orderKind = waypoint;                          // never dispatched anything

    v.state = order_sent;                            // BuildBldg marked the plain request
    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();
    v.PollIdle();

    CHECK_EQ((int)v.route.size(), 1);                // the waypoint survived
    CHECK_EQ(v.route[0].type, waypoint);
}

// ---------------------------------------------------------------------------
// mixed kinds on one list
// ---------------------------------------------------------------------------
static void test_repair_and_road_dispatch_their_own_way() {
    Veh v;
    Hex end(50, 50);
    v.AddOrder(Hex(40, 40), build_road, 0, 0, &end);
    v.AddOrder(Hex(60, 60), repair);

    CHECK(v.NextOrder());
    CHECK_EQ(v.event, ev_build_road);
    CHECK(v.route[0].hexEnd == Hex(50, 50));

    v.ArriveAndSend();
    v.RoadTick();
    v.OrderEnded();
    v.event = ev_none;
    v.mode  = md_stop;
    CHECK(v.PollIdle());
    CHECK_EQ(v.event, ev_repair_bldg);
    CHECK_EQ((int)v.route.size(), 1);

    v.ArriveAndSend();
    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();
    CHECK(!v.PollIdle());
    CHECK_EQ((int)v.route.size(), 0);
}

int main() {
    std::printf("[orders] dispatch suite\n");

    test_idle_first_order_dispatches();
    test_append_forces_one_shot();
    test_busy_appends_only();
    test_completion_advances_in_order();
    test_destroyed_site_drops_the_order();
    test_order_ended_releases_the_queue();
    test_server_error_drops_only_the_matching_order();
    test_late_error_after_the_state_moved_on();
    test_non_local_owner_never_dispatches();
    test_shift_appends_plain_replaces();
    test_clear_orders_keeps_movement_stops();
    test_clear_orders_moves_a_cursor_off_a_removed_order();
    test_movement_stop_at_the_cursor_is_not_dispatched();
    test_unqueued_build_leaves_waypoints_alone();
    test_repair_and_road_dispatch_their_own_way();

    return microtest::Summary();
}
