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
    CHECK(!v.Tick());                 // <- the whole reason m_iOrderState exists
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
    CHECK(v.Tick());                  // consumes #1 and starts #2
    CHECK_EQ((int)v.route.size(), 2);
    CHECK(v.route[0].hex == Hex(20, 20));
    CHECK(v.orderHex == Hex(20, 20));
    CHECK_EQ(v.state, order_sent);

    RunOneBuildToCompletion(v);
    CHECK(v.Tick());                  // consumes #2 and starts #3
    CHECK_EQ((int)v.route.size(), 1);
    CHECK(v.orderHex == Hex(30, 30));

    RunOneBuildToCompletion(v);
    CHECK(!v.Tick());                 // consumes #3, nothing left
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

    CHECK(v.Tick());
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
    CHECK(!v.Tick());
    CHECK_EQ((int)v.route.size(), 2);

    v.OrderEnded();                        // NextRoadHex: the run reached its far end
    v.event = ev_none;
    v.mode  = md_stop;
    CHECK(v.Tick());
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
    CHECK(v.Tick());
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
    CHECK(!v.Tick());
    CHECK_EQ(v.dispatches, 0);

    // even with a remote copy that somehow reached a work state
    v.state = order_work;
    v.site  = false;
    CHECK(!v.Tick());
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
    CHECK(!v.Tick());
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
    v.Tick();

    CHECK_EQ((int)v.route.size(), 1);                // the waypoint survived
    CHECK_EQ(v.route[0].type, waypoint);
}

// ---------------------------------------------------------------------------
// mixed kinds on one list
// ---------------------------------------------------------------------------
static void test_repair_and_road_dispatch_their_own_way() {
    Veh v;
    v.repairTargets.push_back(Hex(60, 60));   // the repair target is still standing
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
    CHECK(v.Tick());
    CHECK_EQ(v.event, ev_repair_bldg);
    CHECK_EQ((int)v.route.size(), 1);

    v.ArriveAndSend();
    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();
    CHECK(!v.Tick());
    CHECK_EQ((int)v.route.size(), 0);
}

// ---------------------------------------------------------------------------
// one list, one meaning: a movement command takes the list over from the orders
// ---------------------------------------------------------------------------
static void test_move_command_clears_the_order_queue() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 0);
    CHECK_EQ((int)v.route.size(), 2);

    // a Shift+move on the same crane -- SetLocation is THE movement-append entry point
    v.AddMoveStop(Hex(77, 77), waypoint);

    CHECK_EQ((int)v.route.size(), 1);            // zero orders left...
    CHECK_EQ(v.route[0].type, waypoint);         // ...and one waypoint
    CHECK(v.route[0].hex == Hex(77, 77));
    CHECK_EQ(v.cursor, 0);
    CHECK_EQ(v.state, order_none);               // and the crane is free again

    // the list can never hold both kinds at once
    for (std::size_t i = 0; i < v.route.size(); ++i)
        CHECK(!IsOrder(v.route[i].type));
}

// ... and an order appended onto a running movement route must not be driven to as if
// it were a stop either. The input layer hands the list over (HasMoveStops ->
// CWndArea::StopRoute) before the append, so the vehicle ends up holding orders only.
static void test_order_takes_the_list_over_from_a_route() {
    Veh v;
    v.AddMoveStop(Hex(1, 1), waypoint);
    v.AddMoveStop(Hex(2, 2), waypoint);
    CHECK_EQ((int)v.route.size(), 2);

    v.route.clear();                             // what StopRoute does to the list
    v.cursor = CUR_NULL;
    v.ClearOrders();
    v.AddOrder(Hex(9, 9), build, 3, 0);

    CHECK_EQ((int)v.route.size(), 1);
    for (std::size_t i = 0; i < v.route.size(); ++i)
        CHECK(IsOrder(v.route[i].type));
    CHECK(v.NextOrder());
}

// ---------------------------------------------------------------------------
// a repair order whose target is gone is dropped at dispatch, not waited on
// ---------------------------------------------------------------------------
static void test_dead_repair_target_is_skipped() {
    Veh v;
    v.AddOrder(Hex(50, 50), repair);             // nothing at this hex any more
    v.AddOrder(Hex(60, 60), build, 4, 0);

    CHECK(v.NextOrder());                        // drops the repair, starts the build
    CHECK_EQ((int)v.route.size(), 1);
    CHECK_EQ(v.route[0].type, build);
    CHECK_EQ(v.event, ev_build);
    CHECK(v.orderHex == Hex(60, 60));

    // a repair whose target IS still there dispatches normally
    Veh w;
    w.repairTargets.push_back(Hex(50, 50));
    w.AddOrder(Hex(50, 50), repair);
    CHECK(w.NextOrder());
    CHECK_EQ(w.event, ev_repair_bldg);
    CHECK_EQ((int)w.route.size(), 1);
}

// A queue of nothing but dead repairs empties itself instead of wedging.
static void test_all_dead_repairs_empty_the_queue() {
    Veh v;
    v.AddOrder(Hex(1, 1), repair);
    v.AddOrder(Hex(2, 2), repair);
    v.AddOrder(Hex(3, 3), repair);

    CHECK(!v.NextOrder());
    CHECK_EQ((int)v.route.size(), 0);
    CHECK_EQ(v.cursor, CUR_NULL);
    CHECK_EQ(v.state, order_none);
    CHECK_EQ(v.dispatches, 0);
}

// ---------------------------------------------------------------------------
// BUG #114 -- a queue that went idle with its orders still listed.
//
// Two independent terminal states, both of which the traffic series made easy to
// reach and neither of which anything in the engine could leave.
// ---------------------------------------------------------------------------

// (1) THE CRANE NEVER NOTICES. The ordinary end of a build leaves the crane welded
// INSIDE the finished building: StopConstruction detaches it and ExitBuilding finds
// m_cOwn FALSE (EnterBuilding released the hexes) and puts it in cant_deploy, not stop.
// The idle branch - where the completion test used to live - does not run there, so a
// crane that could not step straight out never learned its building was done.
static void test_completion_is_seen_from_cant_deploy() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 0);
    CHECK(v.NextOrder());
    v.ArriveAndSend();
    v.ServerAccept();
    v.WorkTick();

    v.SiteDoneWeldedInside();             // -> cant_deploy, NOT stop
    CHECK_EQ(v.mode, md_cant_deploy);

    // ticks spent stuck in the doorway still consume the finished order
    CHECK(!v.Tick());                     // cant_deploy: completion yes, dispatch no
    CHECK_EQ(v.state, order_none);
    CHECK_EQ((int)v.route.size(), 1);     // order 1 consumed
    CHECK_EQ((int)v.route[0].hex.x, 20);
    CHECK(!v.Tick());
    CHECK_EQ((int)v.route.size(), 1);     // and only once

    v.DeployOut();                        // it finally found a free square
    CHECK(v.Tick());                      // now it dispatches order 2
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_build);
    CHECK_EQ((int)v.orderHex.x, 20);
}

// (2) THE ARRIVAL NEVER HAPPENS. FindNextHex gives up short of the destination with
// _SetRouteMode(stop) + PostArrivedOrBlocked - not ArrivedDest - so BuildBldg is never
// called, the arming event is never cleared and the state never leaves order_sent.
// Before the stall watch, NextOrder's busy test refused on BOTH for ever.
static void test_a_dispatch_that_never_arrives_is_re_driven() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 0);
    CHECK(v.NextOrder());
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_build);

    v.GiveUpShortOfDest();                // parked, idle, still armed
    CHECK_EQ(v.mode, md_stop);

    CHECK(!v.Tick(0));                    // the dwell starts
    CHECK(!v.Tick(1000));                 // and is not short-circuited
    CHECK_EQ(v.state, order_sent);
    CHECK(v.Tick(2500));                  // dwell served -> re-dispatch, same order

    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_build);
    CHECK_EQ(v.mode, md_moving);
    CHECK_EQ((int)v.orderHex.x, 10);      // the SAME order, not the next one
    CHECK_EQ((int)v.route.size(), 2);     // nothing dropped
    CHECK_EQ(v.orderRetry, 1);
}

// ...and the re-drive is bounded, so a site that simply cannot be reached costs one
// order instead of the whole queue.
static void test_an_unreachable_order_is_given_up_and_the_queue_runs_on() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 0);
    CHECK(v.NextOrder());

    for (int i = 0; i < ORDER_STALL_TRIES; ++i) {
        v.GiveUpShortOfDest();
        v.Tick(0);                        // arm the dwell
        CHECK(v.Tick(ORDER_STALL_MS + 1));// re-dispatch
        CHECK_EQ(v.orderRetry, i + 1);
    }

    v.GiveUpShortOfDest();
    v.Tick(0);
    CHECK(v.Tick(ORDER_STALL_MS + 1));    // budget spent: drop #1 and start #2
    CHECK_EQ(v.givenUp, 1);
    CHECK_EQ((int)v.route.size(), 1);
    CHECK_EQ((int)v.route[0].hex.x, 20);
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ((int)v.orderHex.x, 20);
    CHECK_EQ(v.orderRetry, 0);            // the new order gets a full budget
}

// The budget is PER ORDER: a queue of hard-but-reachable sites must not run out of
// re-drives because an earlier order needed one.
static void test_the_re_drive_budget_is_per_order() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    v.AddOrder(Hex(20, 20), build, 5, 0);
    CHECK(v.NextOrder());

    v.GiveUpShortOfDest();
    v.Tick(0);
    CHECK(v.Tick(ORDER_STALL_MS + 1));
    CHECK_EQ(v.orderRetry, 1);

    v.ArriveAndSend();                    // the re-drive worked
    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();
    CHECK(v.Tick());                      // order 1 done, order 2 out
    CHECK_EQ(v.orderRetry, 0);
    CHECK_EQ((int)v.orderHex.x, 20);
}

// A TRAFFIC DETOUR is a job that is still running: the hold is counted down by Operate
// and ResumeJob drives the vehicle back to the destination the dispatch named. Re-driving
// through one would cancel the recovery that is about to deliver it.
static void test_the_stall_watch_never_fires_through_a_traffic_hold() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    CHECK(v.NextOrder());

    v.TrafficHold(120);                   // parked off-road with a saved job
    for (int i = 0; i < 6; ++i)
        CHECK(!v.Tick(ORDER_STALL_MS));
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_build);
    CHECK_EQ(v.orderRetry, 0);            // never counted a re-drive
    CHECK_EQ((int)v.route.size(), 1);

    v.HoldExpiresAndResumes();            // ResumeJob puts it back on the build dest
    v.ArriveAndSend();
    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();
    CHECK(!v.Tick());
    CHECK_EQ((int)v.route.size(), 0);
}

// The REQUEST-IN-FLIGHT window is what order_sent exists to name, and it looks idle:
// BuildBldg clears m_iEvent on the line it sends from. It must never be re-driven, or
// every queued build would go out twice.
static void test_the_stall_watch_never_fires_on_a_request_in_flight() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    CHECK(v.NextOrder());
    v.ArriveAndSend();                    // event cleared, request on the wire
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_none);
    CHECK_EQ(v.mode, md_stop);

    for (int i = 0; i < 6; ++i)
        CHECK(!v.Tick(ORDER_STALL_MS));
    CHECK_EQ(v.dispatches, 1);            // exactly one request, not six
    CHECK_EQ(v.orderRetry, 0);
    CHECK_EQ((int)v.route.size(), 1);

    v.ServerAccept();
    v.WorkTick();
    v.SiteGone();
    CHECK(!v.Tick());
    CHECK_EQ((int)v.route.size(), 0);
}

// The review's DEFERRED RESIDUAL, closed by the same watch: a repair target that dies
// while the crane is travelling. ArrivedDest's repair_bldg case finds neither a building
// nor an unbuilt bridge and falls through, leaving m_iEvent == repair_bldg on a stopped
// crane. The watch re-drives, NextOrder's RepairTargetLives rejects the dead target, the
// order is dropped and the build behind it starts.
static void test_a_repair_target_that_dies_in_flight_no_longer_wedges() {
    Veh v;
    v.repairTargets.push_back(Hex(5, 5));
    v.AddOrder(Hex(5, 5), repair);
    v.AddOrder(Hex(9, 9), build, 4, 0);
    CHECK(v.NextOrder());                 // the target was alive at dispatch
    CHECK_EQ(v.event, ev_repair_bldg);

    v.repairTargets.clear();              // it dies while the crane is on the road
    v.ArriveAtDeadRepairTarget();

    v.Tick(0);
    CHECK(v.Tick(ORDER_STALL_MS + 1));    // re-drive -> validated -> dropped -> next
    CHECK_EQ((int)v.route.size(), 1);
    CHECK_EQ((int)v.route[0].hex.x, 9);
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.event, ev_build);
    CHECK_EQ(v.givenUp, 0);               // dropped at dispatch, not by the budget
}

// A vehicle we do not own must never re-drive or give up anything, the same way it
// never dispatches. The watch lives behind PollIdle's own IsLocal gate.
static void test_a_remote_copy_never_re_drives() {
    Veh v;
    v.AddOrder(Hex(10, 10), build, 4, 0);
    CHECK(v.NextOrder());
    v.GiveUpShortOfDest();
    v.local = false;

    for (int i = 0; i < 6; ++i)
        CHECK(!v.Tick(ORDER_STALL_MS));
    CHECK_EQ(v.state, order_sent);
    CHECK_EQ(v.orderRetry, 0);
    CHECK_EQ(v.givenUp, 0);
    CHECK_EQ((int)v.route.size(), 1);
}

// A crane with NOTHING queued that gives up short of a plain, unqueued placement must be
// left exactly as 1996 left it: the watch is gated on order_sent, which only a dispatch
// sets, so an empty queue is inert.
static void test_an_unqueued_crane_is_not_touched_by_the_watch() {
    Veh v;
    v.state = order_none;
    v.event = ev_build;                   // a plain placement armed it
    v.mode  = md_stop;
    for (int i = 0; i < 6; ++i)
        CHECK(!v.Tick(ORDER_STALL_MS));
    CHECK_EQ(v.event, ev_build);          // untouched
    CHECK_EQ(v.state, order_none);
    CHECK_EQ(v.givenUp, 0);
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

    test_move_command_clears_the_order_queue();
    test_order_takes_the_list_over_from_a_route();
    test_dead_repair_target_is_skipped();
    test_all_dead_repairs_empty_the_queue();

    // bug #114
    test_completion_is_seen_from_cant_deploy();
    test_a_dispatch_that_never_arrives_is_re_driven();
    test_an_unreachable_order_is_given_up_and_the_queue_runs_on();
    test_the_re_drive_budget_is_per_order();
    test_the_stall_watch_never_fires_through_a_traffic_hold();
    test_the_stall_watch_never_fires_on_a_request_in_flight();
    test_a_repair_target_that_dies_in_flight_no_longer_wedges();
    test_a_remote_copy_never_re_drives();
    test_an_unqueued_crane_is_not_touched_by_the_watch();

    return microtest::Summary();
}
