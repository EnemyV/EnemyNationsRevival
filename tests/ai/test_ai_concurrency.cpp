// test_ai_concurrency.cpp
//
// Concurrency MODELS for the AI thread layer. These do NOT run the real
// CAIMgr::Manage (which needs theGame / theVehicleMap / locks / a game thread);
// they are faithful, standalone reimplementations of the *patterns* the AI uses,
// exercised with real std::thread so the locking discipline can be tested.
//
// HARD CAVEATS (read before trusting a green run):
//   * This tests the MODEL, not the game code. A pass means "this locking
//     discipline is race-free under stress", not "CAIMgr::Manage is correct".
//   * MSVC has no usable ThreadSanitizer, so race detection here is stress-based
//     (probabilistic), not sound. Repeat counts trade runtime for confidence.
//   * sleeps are used ONLY to widen race windows, never to assert on timing.
//     Timeout LOGIC is tested with a mock clock (deterministic, no sleep).
//
// Build & run: run-ai-concurrency.ps1

#include "microtest.h"

#include <windows.h>  // SRWLOCK (the snapshot-swap model mirrors aisnap.cpp)

#include <atomic>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>

// ===========================================================================
// 1) Message queue: producer/consumer with the tmp->main FIFO hand-off and the
//    backlog gauge. Models caimgr.cpp m_plTmpQueue/m_plMsgQueue + g_aiMsgBacklog
//    (InterlockedIncrement on enqueue, InterlockedDecrement on process) and the
//    stub PrioritizeMessage (FIFO move, one per call).
// ===========================================================================
struct MsgQueue {
    std::mutex        m;
    std::queue<int>   tmp;       // arrival queue (producers)
    std::queue<int>   main;      // prioritized queue (consumer)
    std::atomic<long> backlog{0};

    void Enqueue(int v) {
        std::lock_guard<std::mutex> g(m);
        tmp.push(v);
        backlog.fetch_add(1, std::memory_order_relaxed);
    }
    // PrioritizeMessage stub: move one tmp -> main (pure FIFO, no reordering).
    void PrioritizeOne() {
        std::lock_guard<std::mutex> g(m);
        if (!tmp.empty()) { main.push(tmp.front()); tmp.pop(); }
    }
    bool Dequeue(int& out) {
        std::lock_guard<std::mutex> g(m);
        if (main.empty()) return false;
        out = main.front();
        main.pop();
        backlog.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }
};

static void test_msgqueue_no_loss_and_fifo() {
    const int P = 4;     // producer threads
    const int N = 500;   // messages each
    const int total = P * N;

    for (int rep = 0; rep < 3; ++rep) {
        MsgQueue q;
        std::vector<std::thread> producers;
        for (int p = 0; p < P; ++p) {
            producers.emplace_back([&q, p, N]() {
                for (int s = 0; s < N; ++s) q.Enqueue(p * N + s);  // value encodes (producer, seq)
            });
        }

        std::vector<char> seen(total, 0);
        std::vector<int>  lastSeq(P, -1);
        int collected = 0, dup = 0, outOfOrder = 0;

        // Consumer runs concurrently with producers.
        while (collected < total) {
            q.PrioritizeOne();
            int v;
            if (q.Dequeue(v)) {
                if (seen[v]) ++dup;
                seen[v] = 1;
                int p = v / N, s = v % N;
                if (s <= lastSeq[p]) ++outOfOrder;   // per-producer FIFO must hold
                lastSeq[p] = s;
                ++collected;
            } else {
                std::this_thread::yield();
            }
        }
        for (auto& t : producers) t.join();

        CHECK_EQ(collected, total);
        CHECK_EQ(dup, 0);             // no message delivered twice
        CHECK_EQ(outOfOrder, 0);      // FIFO preserved within each producer
        int missing = 0;
        for (char c : seen) if (!c) ++missing;
        CHECK_EQ(missing, 0);         // no message lost
        CHECK_EQ((long)q.backlog.load(), 0L);  // gauge returns to zero (enq == proc)
    }
}

// ===========================================================================
// 2) Shared path map: readers vs. a rebuilder. Models the thePathMap bug class
//    (Init() tearing down shared cells while AI threads read concurrently). Here
//    the rebuild holds the lock across the WHOLE teardown+rebuild -- the fix --
//    with a mock-pathing sleep INSIDE the critical section to widen the window.
//    Invariant: a locked reader never sees a half-rebuilt (mixed-generation) map.
// ===========================================================================
struct SharedPathMap {
    std::mutex       m;
    std::vector<int> cells;

    void RebuildLocked(int gen, int windowUs) {
        std::lock_guard<std::mutex> g(m);          // held across the entire rebuild
        cells.clear();
        for (int i = 0; i < 64; ++i) {
            cells.push_back(gen);
            if (i == 32 && windowUs)               // mock pathing latency mid-rebuild
                std::this_thread::sleep_for(std::chrono::microseconds(windowUs));
        }
    }
    bool ReadConsistentLocked() {
        std::lock_guard<std::mutex> g(m);
        if (cells.empty()) return true;
        int v = cells[0];
        for (size_t i = 1; i < cells.size(); ++i)
            if (cells[i] != v) return false;       // mixed generations == torn read
        return true;
    }
};

static void test_shared_map_lock_discipline() {
    SharedPathMap map;
    map.RebuildLocked(0, 0);

    std::atomic<bool> stop{false};
    std::atomic<int>  violations{0};
    std::atomic<int>  reads{0};

    std::vector<std::thread> readers;
    for (int r = 0; r < 4; ++r) {
        readers.emplace_back([&]() {
            while (!stop.load(std::memory_order_relaxed)) {
                if (!map.ReadConsistentLocked())
                    violations.fetch_add(1, std::memory_order_relaxed);
                reads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Rebuilder: 200 rebuilds, each with a 50us window to provoke interleaving.
    for (int gen = 1; gen <= 200; ++gen) map.RebuildLocked(gen, 50);

    stop.store(true);
    for (auto& t : readers) t.join();

    CHECK(reads.load() > 0);          // readers actually ran
    CHECK_EQ(violations.load(), 0);   // lock-across-rebuild => never a torn read
}

// ===========================================================================
// 3) Stuck-vehicle timeout thresholds. Models caimgr.cpp HandleStuckVehicles
//    (5-min and 10-min branches) with a MOCK CLOCK -- deterministic, no sleep.
//      if (!lastTime || lastTime > now)        -> record current time
//      d = now - lastTime
//      if (d > 300000 && d < 600000)           -> 5-min action
//      if (d > 600000)                         -> 10-min action
// ===========================================================================
enum StuckAction { SA_None, SA_RecordTime, SA_ClearAtDest, SA_Resend5, SA_Resend10 };

static StuckAction CheckStuck(uint64_t now, uint64_t lastTime, bool atDest) {
    if (!lastTime || lastTime > now) return SA_RecordTime;
    uint64_t d = now - lastTime;
    if (d > 300000 && d < 600000) return atDest ? SA_ClearAtDest : SA_Resend5;
    if (d > 600000)               return atDest ? SA_ClearAtDest : SA_Resend10;
    return SA_None;
}

static void test_stuck_vehicle_timeout() {
    CHECK_EQ(CheckStuck(1000, 0, false),         SA_RecordTime);  // never timed
    CHECK_EQ(CheckStuck(1000, 5000, false),      SA_RecordTime);  // clock went backwards
    CHECK_EQ(CheckStuck(240000, 1, false),       SA_None);        // ~4 min: nothing yet
    CHECK_EQ(CheckStuck(300011, 10, false),      SA_Resend5);     // d>300000, not at dest
    CHECK_EQ(CheckStuck(300011, 10, true),       SA_ClearAtDest); // d>300000, arrived
    CHECK_EQ(CheckStuck(600011, 10, false),      SA_Resend10);    // d>600000, not at dest
    CHECK_EQ(CheckStuck(600011, 10, true),       SA_ClearAtDest); // d>600000, arrived

    // Faithful reproduction of a boundary quirk: the conditions are strict >, so
    // an interval of EXACTLY 300000 or EXACTLY 600000 ms falls in neither branch
    // -> the vehicle gets no action on that tick.
    CHECK_EQ(CheckStuck(300000, 0, false), SA_RecordTime); // lastTime==0 guard
    CHECK_EQ(CheckStuck(300001, 1, false), SA_None);       // d == 300000 exactly -> none
    CHECK_EQ(CheckStuck(600001, 1, false), SA_None);       // d == 600000 exactly -> none
    CHECK_EQ(CheckStuck(600002, 1, false), SA_Resend10);   // d == 600001 -> 10-min
}

// ===========================================================================
// 4) Tier-B world snapshot: double buffer + SRW-guarded front swap. Models
//    aisnap.cpp exactly: the writer fills the BACK buffer unlocked (readers
//    can't reach it), then swaps the front index under the SRW held exclusive;
//    readers hold it shared across find+copy. Invariant: a reader never
//    observes a mixed-generation buffer (all fields of every entry agree).
// ===========================================================================
struct SnapModel {
    struct Entry { int a, b, c; };          // "fields" that must agree
    static const int kEntries = 64;
    Entry   buf[2][kEntries];
    int     iFront;
    SRWLOCK lock;

    SnapModel() : iFront(0) {
        InitializeSRWLock(&lock);
        for (int i = 0; i < kEntries; ++i) { buf[0][i].a = buf[0][i].b = buf[0][i].c = 0; }
        memcpy(buf[1], buf[0], sizeof(buf[0]));
    }
    void Publish(int gen, int windowUs) {
        int back = 1 - iFront;
        for (int i = 0; i < kEntries; ++i) {     // unlocked back-buffer fill
            buf[back][i].a = gen;
            buf[back][i].b = gen;
            if (i == kEntries / 2 && windowUs)   // widen the race window
                std::this_thread::sleep_for(std::chrono::microseconds(windowUs));
            buf[back][i].c = gen;
        }
        AcquireSRWLockExclusive(&lock);          // swap only, never the fill
        iFront = back;
        ReleaseSRWLockExclusive(&lock);
    }
    bool ReadConsistent() {
        bool ok = true;
        AcquireSRWLockShared(&lock);             // shared across the whole read
        const Entry* e = buf[iFront];
        int gen = e[0].a;
        for (int i = 0; i < kEntries && ok; ++i)
            ok = (e[i].a == gen && e[i].b == gen && e[i].c == gen);
        ReleaseSRWLockShared(&lock);
        return ok;
    }
};

static void test_snapshot_swap_consistency() {
    SnapModel snap;
    std::atomic<bool> stop{false};
    std::atomic<int>  violations{0};
    std::atomic<int>  reads{0};

    std::vector<std::thread> readers;
    for (int r = 0; r < 4; ++r) {
        readers.emplace_back([&]() {
            while (!stop.load(std::memory_order_relaxed)) {
                if (!snap.ReadConsistent())
                    violations.fetch_add(1, std::memory_order_relaxed);
                reads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // 300 publishes with a 50us mid-fill window to provoke interleaving
    for (int gen = 1; gen <= 300; ++gen) snap.Publish(gen, 50);

    stop.store(true);
    for (auto& t : readers) t.join();

    CHECK(reads.load() > 0);
    CHECK_EQ(violations.load(), 0);  // no torn (mixed-generation) snapshot ever
}

int main() {
    test_msgqueue_no_loss_and_fifo();
    test_shared_map_lock_discipline();
    test_stuck_vehicle_timeout();
    test_snapshot_swap_consistency();
    return microtest::Summary();
}
