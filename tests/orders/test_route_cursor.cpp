// test_route_cursor.cpp -- standalone fixture for BUGS #99 (save format Release 8).
//
// Mirrors, in dependency-free C++, the exact logic of the two production sites the
// #99 batch touches, so the format decision can be checked without the game, MFC,
// CArchive or a save file:
//
//   * CVehicle::Serialize (new_unit.cpp) -- the WORD route-cursor field: the shipped
//     writer (post-#88: a NULL cursor stores N-1), the Release-8 writer (a NULL cursor
//     stores the 0xFFFF sentinel), the shipped reader (walk to index wNR) and the
//     Release-8 reader (the sentinel means "no cursor" only when m_dwVer >= 8).
//   * CVehicle::Load (vehicle.cpp) -- the cycle over the route that stops when it
//     returns to the cursor it started from; with a NULL cursor the shipped walk can
//     never terminate, which is what the companion guard fixes.
//
// The mirrors are hand-kept copies, NOT the production code: they prove the DECISION
// (which word goes on the wire, what comes back, does the walk stop), not that the
// production text still says what it said. Keeping old and new side by side is the
// point -- every old/new cell is evaluated on the same input.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>
#include <vector>

typedef unsigned short WORD;

// ---------------------------------------------------------------------------
// A CObList stand-in: a singly linked list whose POSITION is the node pointer,
// exactly the shape CVehicle::m_route / m_pos have (afxcoll.h: a POSITION is the
// node address, GetNext advances it and returns the value, and it goes NULL off
// the tail).
// ---------------------------------------------------------------------------
struct Node {
    Node* next;
    int   id;         // index in the route, for readable failures
    bool  bUnload;    // CRoute::unload, for the Load() walk
};

struct Route {
    std::vector<Node*> nodes;

    ~Route() {
        for (size_t i = 0; i < nodes.size(); ++i) delete nodes[i];
    }
    void Build(int n) {
        for (int i = 0; i < n; ++i) {
            Node* p = new Node;
            p->next = NULL; p->id = i; p->bUnload = false;
            if (!nodes.empty()) nodes.back()->next = p;
            nodes.push_back(p);
        }
    }
    int   GetCount() const { return (int)nodes.size(); }
    Node* GetHeadPosition() const { return nodes.empty() ? NULL : nodes.front(); }
    Node* GetTailPosition() const { return nodes.empty() ? NULL : nodes.back(); }
    // CObList::GetNext( pos ) -- advance pos, return the value that WAS at pos.
    Node* GetNext(Node*& pos) const { Node* p = pos; pos = p->next; return p; }
    int   IndexOf(const Node* p) const {
        for (size_t i = 0; i < nodes.size(); ++i)
            if (nodes[i] == p) return (int)i;
        return -1;   // NULL, or a stale POSITION in no node of this list
    }
};

static const WORD wROUTE_POS_NONE = (WORD)-1;   // 0xFFFF, as new_unit.cpp declares it

// ---------------------------------------------------------------------------
// The four wire halves.
// ---------------------------------------------------------------------------

// SHIPPED writer (new_unit.cpp after #88 747e5f9c): compare before GetNext advances,
// and restore the accidental N-1 for a NULL cursor.
static WORD WriteOld(const Route& r, Node* m_pos) {
    int   iPos = 0, iOn = 0;
    Node* pos  = r.GetHeadPosition();
    while (pos != NULL) {
        if (pos == m_pos) iPos = iOn;
        r.GetNext(pos);
        iOn++;
    }
    if ((m_pos == NULL) && (iOn > 0)) iPos = iOn - 1;
    return (WORD)iPos;
}

// RELEASE 8 writer: a NULL cursor means NO cursor, so store the out-of-range sentinel.
// The `iOn > 0` guard is gone deliberately -- an empty route with a NULL cursor now
// stores the sentinel too, and both readers land on the same NULL either way.
static WORD WriteNew(const Route& r, Node* m_pos) {
    int   iPos = 0, iOn = 0;
    Node* pos  = r.GetHeadPosition();
    while (pos != NULL) {
        if (pos == m_pos) iPos = iOn;
        r.GetNext(pos);
        iOn++;
    }
    if (m_pos == NULL) iPos = (int)wROUTE_POS_NONE;
    return (WORD)iPos;
}

// SHIPPED reader: walk wNR nodes from the head. Returns the restored cursor.
static Node* ReadOld(const Route& r, WORD wNR) {
    Node* m_pos = NULL;
    if (r.GetCount() > 0) {
        Node* pos = r.GetHeadPosition();
        while (pos != NULL) {
            if (wNR <= 0) { m_pos = pos; break; }
            wNR--;
            r.GetNext(pos);
        }
    }
    return m_pos;
}

// RELEASE 8 reader: the sentinel restores a NULL cursor, but ONLY on a save whose
// format counter is >= 8. Everything else is the shipped walk, untouched.
static Node* ReadNew(const Route& r, WORD wNR, unsigned dwVer) {
    Node*      m_pos       = NULL;
    const bool bNoRoutePos = (dwVer >= 8) && (wNR == wROUTE_POS_NONE);
    if (!bNoRoutePos && (r.GetCount() > 0)) {
        Node* pos = r.GetHeadPosition();
        while (pos != NULL) {
            if (wNR <= 0) { m_pos = pos; break; }
            wNR--;
            r.GetNext(pos);
        }
    }
    return m_pos;
}

// ---------------------------------------------------------------------------
// CVehicle::Load's route walk, shipped vs guarded. Returns the number of steps
// taken, capped: the shipped walk does not terminate on a NULL cursor, so the cap
// is the positive control that says so without hanging the fixture.
// ---------------------------------------------------------------------------
static int LoadWalkOld(const Route& r, Node* m_pos, int iCap, int* piFoundUnload) {
    int   iSteps = 0;
    Node* pos    = m_pos;
    if (piFoundUnload) *piFoundUnload = -1;
    while (iSteps < iCap) {
        iSteps++;
        if (pos == NULL) pos = r.GetHeadPosition();
        else {
            r.GetNext(pos);
            if (pos == NULL) pos = r.GetHeadPosition();
        }
        if ((pos == m_pos) || (pos == NULL)) break;
        if (pos->bUnload) { if (piFoundUnload) *piFoundUnload = pos->id; break; }
    }
    return iSteps;
}

static int LoadWalkNew(const Route& r, Node* m_pos, int iCap, int* piFoundUnload) {
    int   iSteps   = 0;
    Node* posStart = m_pos;
    if (posStart == NULL) posStart = r.GetHeadPosition();
    Node* pos = posStart;
    if (piFoundUnload) *piFoundUnload = -1;
    while (iSteps < iCap) {
        iSteps++;
        if (pos == NULL) pos = r.GetHeadPosition();
        else {
            r.GetNext(pos);
            if (pos == NULL) pos = r.GetHeadPosition();
        }
        if ((pos == posStart) || (pos == NULL)) break;
        if (pos->bUnload) { if (piFoundUnload) *piFoundUnload = pos->id; break; }
    }
    return iSteps;
}

// ---------------------------------------------------------------------------

enum CursorKind { cur_null = 0, cur_head, cur_tail, cur_stale, cur_count };

static const char* CursorName(int k) {
    switch (k) {
        case cur_null: return "NULL";
        case cur_head: return "head";
        case cur_tail: return "tail";
        default:       return "stale";
    }
}

int main() {
    static const int aCounts[] = { 0, 1, 2, 3, 7, 32, 128 };
    static const int nCounts   = (int)(sizeof(aCounts) / sizeof(aCounts[0]));

    std::printf("[route_cursor] BUGS #99 save-format Release 8 fixture\n");

    // The sentinel must be out of range for every route a WORD count can describe:
    // the count goes on the wire as a WORD too, so the largest index a real cursor can
    // take is 0xFFFE. (Held in plain locals, not constants -- a literal comparison here
    // is a constant expression and /W4 /WX rejects it as C4127.)
    unsigned uMaxCount = 0xFFFFu;
    unsigned uMaxIndex = uMaxCount - 1u;
    CHECK_EQ(wROUTE_POS_NONE, 0xFFFF);
    CHECK_EQ(uMaxIndex, 0xFFFE);
    CHECK(uMaxIndex < (unsigned)wROUTE_POS_NONE);

    for (int iC = 0; iC < nCounts; ++iC) {
        const int N = aCounts[iC];
        Route r;
        r.Build(N);

        Node stale;                       // a POSITION in no node of this list
        stale.next = NULL; stale.id = -7; stale.bUnload = false;

        for (int k = 0; k < cur_count; ++k) {
            // head/tail of an empty route ARE NULL -- that is the cur_null case, and
            // running it twice would only hide which cell failed.
            if ((N == 0) && ((k == cur_head) || (k == cur_tail))) continue;

            Node* m_pos = NULL;
            if (k == cur_head)  m_pos = r.GetHeadPosition();
            if (k == cur_tail)  m_pos = r.GetTailPosition();
            if (k == cur_stale) m_pos = &stale;

            const WORD wOld = WriteOld(r, m_pos);
            const WORD wNew = WriteNew(r, m_pos);
            const int  iCur = r.IndexOf(m_pos);   // -1 for NULL and for the stale POSITION

            std::printf("[n=%3d %-5s] old=0x%04X new=0x%04X\n", N, CursorName(k), wOld, wNew);

            // ---- the writer changes exactly one case: a NULL cursor ----
            if (m_pos == NULL) {
                CHECK_EQ(wOld, (N > 0) ? (WORD)(N - 1) : (WORD)0);
                CHECK_EQ(wNew, wROUTE_POS_NONE);
            } else {
                CHECK_EQ(wOld, wNew);                        // byte for byte
                // a real cursor stores its index; a stale POSITION matches nothing
                // and stores 0, exactly as the shipped writer did.
                CHECK_EQ(wNew, (WORD)((iCur >= 0) ? iCur : 0));
            }

            // ---- counter 7: the new READER is the shipped reader, byte for byte ----
            // A counter-7 save carries an OLD-writer word; read under m_dwVer 7 it must
            // restore exactly what the shipped build restored...
            CHECK(ReadNew(r, wOld, 7) == ReadOld(r, wOld));
            // ...and that holds for every word a counter-7 save could carry, including
            // the sentinel value itself, which the gate must IGNORE at counter 7.
            CHECK(ReadNew(r, wROUTE_POS_NONE, 7) == ReadOld(r, wROUTE_POS_NONE));

            // ---- counter 8: the round trip this build actually performs ----
            Node* pRt = ReadNew(r, wNew, 8);
            if (N == 0)               CHECK(pRt == NULL);                   // nothing to point at
            else if (m_pos == NULL)   CHECK(pRt == NULL);                   // #99: no cursor stays no cursor
            else if (k == cur_stale)  CHECK(pRt == r.GetHeadPosition());    // pre-existing: stale -> head
            else                      CHECK(pRt == m_pos);                  // head and tail round-trip

            // ---- the gate reacts to the SENTINEL, not to the counter ----
            // An N-1 word under counter 8 still restores the last entry: raising the
            // counter on its own changes nothing.
            if (N > 0) {
                CHECK(ReadNew(r, (WORD)(N - 1), 8) == r.GetTailPosition());
                CHECK(ReadOld(r, (WORD)(N - 1))    == r.GetTailPosition());
            }

            // ---- a counter-8 word met by the SHIPPED reader (downgrade) ----
            // Not a supported load, but it must not walk into anything: the sentinel is
            // larger than any index, so the walk runs off the tail and leaves the cursor
            // NULL -- the same answer, the slow way.
            if (m_pos == NULL) CHECK(ReadOld(r, wNew) == NULL);
        }

        // -------- CVehicle::Load's walk --------
        const int iCap = 8 * N + 32;

        // (a) a NULL cursor: the shipped walk never terminates (it burns the cap), the
        //     guarded walk stops after one lap. N == 0 is the one case where the shipped
        //     walk already stopped, because the head is NULL too.
        {
            int iUnloadOld = 0, iUnloadNew = 0;
            const int iOld = LoadWalkOld(r, NULL, iCap, &iUnloadOld);
            const int iNew = LoadWalkNew(r, NULL, iCap, &iUnloadNew);
            if (N == 0) {
                CHECK_EQ(iOld, 1);
                CHECK_EQ(iNew, 1);
            } else {
                CHECK_EQ(iOld, iCap);            // positive control: did NOT terminate
                CHECK(iNew < iCap);              // guarded: terminates
                CHECK_EQ(iNew, N);               // ...after exactly one lap
            }
            CHECK_EQ(iUnloadNew, -1);
        }

        // (b) every non-NULL cursor: the guard is a provable no-op -- same step count,
        //     same stop, because posStart == m_pos.
        for (int k = cur_head; k < cur_count; ++k) {
            if ((N == 0) && (k != cur_stale)) continue;
            Node* m_pos = (k == cur_head) ? r.GetHeadPosition()
                        : (k == cur_tail) ? r.GetTailPosition()
                                          : &stale;
            int iUnloadOld = 0, iUnloadNew = 0;
            const int iOld = LoadWalkOld(r, m_pos, iCap, &iUnloadOld);
            const int iNew = LoadWalkNew(r, m_pos, iCap, &iUnloadNew);
            CHECK_EQ(iOld, iNew);
            CHECK_EQ(iUnloadOld, iUnloadNew);
            if (k != cur_stale) CHECK(iNew < iCap);
        }

        // (c) a NULL cursor with a real unload stop: the guarded walk finds it, and finds
        //     the FIRST one after the head, which is what Load() wants.
        if (N >= 3) {
            r.nodes[2]->bUnload = true;
            int iUnload = 0;
            const int iNew = LoadWalkNew(r, NULL, iCap, &iUnload);
            CHECK_EQ(iUnload, 2);
            CHECK(iNew < iCap);
            r.nodes[2]->bUnload = false;
        }
    }

    return microtest::Summary();
}
