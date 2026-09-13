// test_route_delete_cursor.cpp -- standalone fixture for BUGS #96.
//
// SDL2RouteWindow::OnDelete (SDL2RouteWindow.cpp) deletes the selected route/order
// row and, if that row was also the vehicle's route cursor, picks a fallback cursor.
// The shipped fallback:
//
//     POSITION pos_next = pos;
//     m_pVeh->GetRouteList().GetNext(pos_next);        // successor, or NULL at tail
//     if (!pos_next)
//         pos_next = m_pVeh->GetRouteList().GetHeadPosition();   // <-- read BEFORE delete
//     m_pVeh->SetRoutePos(pos_next);
//     ...
//     m_pVeh->GetRouteList().RemoveAt(pos);
//     delete pR;
//
// reads GetHeadPosition() *before* RemoveAt(pos). When pos is both the head and the
// tail of a one-element list (no successor, and the head hasn't moved yet), that read
// returns pos itself -- the very node RemoveAt/delete are about to free. SetRoutePos
// then stores a dangling POSITION, and the game later dereferences it: a
// heap-use-after-free, confirmed under ASAN (ledger/winastra-review-12/asan-results.json).
//
// The fix moves the head-fallback read to AFTER the delete (equivalently: take the
// successor before deleting; if there is none, take the new head once the node is
// actually gone; if the list is now empty, NULL).
//
// This fixture mirrors both versions with a small, dependency-free doubly linked list
// standing in for CList<CRoute*,CRoute*> (POSITION == node pointer, GetNext(pos)
// returns the value at pos and advances pos, matching MFC CList semantics -- the same
// contract test_route_cursor.cpp's Route model uses). Nodes are genuinely allocated
// and freed with `new`/`delete` so the OLD path's dangling read is a REAL
// heap-use-after-free the OS can in principle catch, not a simulated one; the fixture
// only ever compares pointer VALUES after a free, never dereferences a freed node.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// CList<CRoute*,CRoute*> stand-in: POSITION is the node pointer.
// ---------------------------------------------------------------------------
struct Node {
    Node* prev;
    Node* next;
    int   id;
};

struct RouteList {
    Node* head;
    Node* tail;
    int   count;

    RouteList() : head(NULL), tail(NULL), count(0) {}
    ~RouteList() {
        // any nodes RemoveAt already freed are gone; free whatever remains.
        Node* p = head;
        while (p) { Node* n = p->next; delete p; p = n; }
    }

    Node* PushBack(int id) {
        Node* n = new Node;
        n->id = id; n->next = NULL; n->prev = tail;
        if (tail) tail->next = n; else head = n;
        tail = n;
        count++;
        return n;
    }

    Node* GetHeadPosition() const { return head; }

    // MFC semantics: returns the value AT pos, advances pos to the next node (NULL off the tail).
    Node* GetNext(Node*& pos) const { Node* p = pos; pos = p->next; return p; }

    void RemoveAt(Node* pos) {
        if (pos->prev) pos->prev->next = pos->next; else head = pos->next;
        if (pos->next) pos->next->prev = pos->prev; else tail = pos->prev;
        delete pos;
        count--;
    }

    int GetCount() const { return count; }

    bool IsLive(Node* p) const {
        for (Node* q = head; q; q = q->next) if (q == p) return true;
        return false;
    }
};

// ---------------------------------------------------------------------------
// The two OnDelete cursor-fallback bodies. `pDeletedOut` receives the freed node's
// pointer VALUE (never dereferenced afterward) so the test can check whether the
// resulting cursor aliases freed memory.
// ---------------------------------------------------------------------------

// SHIPPED (buggy): head-fallback read BEFORE RemoveAt.
static Node* OnDeleteOld(RouteList& list, Node* pos, Node* cursor, Node** pDeletedOut) {
    Node* newCursor = cursor;
    if (pos == cursor) {
        Node* pos_next = pos;
        list.GetNext(pos_next);                 // successor, or NULL at the tail
        if (!pos_next)
            pos_next = list.GetHeadPosition();  // BUG: read before the delete below
        newCursor = pos_next;
    }
    *pDeletedOut = pos;
    list.RemoveAt(pos);
    return newCursor;
}

// FIXED: successor captured before delete; head-fallback read AFTER delete.
static Node* OnDeleteNew(RouteList& list, Node* pos, Node* cursor, Node** pDeletedOut) {
    bool  bWasCursor = (pos == cursor);
    Node* pos_succ   = pos;
    list.GetNext(pos_succ);                     // successor before delete, or NULL at the tail

    *pDeletedOut = pos;
    list.RemoveAt(pos);

    if (!bWasCursor) return cursor;

    Node* pos_next = pos_succ;
    if (!pos_next)
        pos_next = list.GetHeadPosition();      // read AFTER the delete -- the fix
    return pos_next;
}

// ---------------------------------------------------------------------------

enum DelKind { del_head = 0, del_tail, del_mid, del_only, del_kind_count };

static const char* KindName(int k) {
    switch (k) {
        case del_head: return "head";
        case del_tail: return "tail";
        case del_mid:  return "mid";
        default:       return "only";
    }
}

int main() {
    std::printf("[route_delete_cursor] BUGS #96 use-after-free fixture\n");

    static const int aCounts[] = { 1, 2, 3, 7, 32 };
    static const int nCounts   = (int)(sizeof(aCounts) / sizeof(aCounts[0]));

    bool sawOldUAF = false;   // positive control: the shipped body must exhibit the bug somewhere

    for (int iC = 0; iC < nCounts; ++iC) {
        const int N = aCounts[iC];

        for (int k = 0; k < del_kind_count; ++k) {
            if (k == del_only && N != 1) continue;
            if (k != del_only && N == 1) continue;
            if (k == del_mid && N < 3) continue;

            // -------- OLD (shipped) body, on its own fresh list --------
            {
                RouteList list;
                std::vector<Node*> nodes;
                for (int i = 0; i < N; ++i) nodes.push_back(list.PushBack(i));

                int delIdx = (k == del_head) ? 0
                           : (k == del_tail) ? N - 1
                           : (k == del_only) ? 0
                                              : N / 2;
                Node* delNode = nodes[delIdx];
                Node* cursor  = delNode;   // the row under test: the deleted node IS the cursor

                Node* deleted   = NULL;
                Node* newCursor = OnDeleteOld(list, delNode, cursor, &deleted);

                bool isUAF = (newCursor == deleted);   // cursor aliases the freed node's address
                if (isUAF) sawOldUAF = true;

                std::printf("[old n=%2d %-4s] cursor-after=%s deleted-alias=%s\n",
                            N, KindName(k), newCursor ? "non-null" : "NULL", isUAF ? "YES(UAF)" : "no");

                // The single-element case is the row's exact repro: cursor==head==tail,
                // no successor, and the pre-delete head read hands back the freed node.
                if (k == del_only) {
                    CHECK(isUAF);   // positive control -- the shipped bug reproduces here
                } else {
                    // every other shape already has a distinct successor or head, so the
                    // shipped body happens to be safe there.
                    CHECK(!isUAF);
                }
            }

            // -------- NEW (fixed) body, on its own fresh list --------
            {
                RouteList list;
                std::vector<Node*> nodes;
                for (int i = 0; i < N; ++i) nodes.push_back(list.PushBack(i));

                int delIdx = (k == del_head) ? 0
                           : (k == del_tail) ? N - 1
                           : (k == del_only) ? 0
                                              : N / 2;
                Node* delNode = nodes[delIdx];
                Node* cursor  = delNode;

                Node* deleted   = NULL;
                Node* newCursor = OnDeleteNew(list, delNode, cursor, &deleted);

                bool isUAF  = (newCursor == deleted);
                bool isLive = (newCursor != NULL) && list.IsLive(newCursor);

                std::printf("[new n=%2d %-4s] cursor-after=%s live=%s\n",
                            N, KindName(k), newCursor ? "non-null" : "NULL", isLive ? "yes" : "n/a");

                CHECK(!isUAF);                                   // never aliases the freed node
                if (N == 1) CHECK(newCursor == NULL);            // single element: list now empty
                else        CHECK(isLive);                       // otherwise: lands on a live node
            }

            // -------- deleting a row that is NOT the cursor: no-op on the cursor, both bodies --------
            if (N >= 2) {
                RouteList listA, listB;
                std::vector<Node*> a, b;
                for (int i = 0; i < N; ++i) { a.push_back(listA.PushBack(i)); b.push_back(listB.PushBack(i)); }

                int delIdx = (k == del_head) ? 0 : (k == del_tail) ? N - 1 : N / 2;
                // pick a cursor index different from delIdx, deterministically: 0 unless delIdx==0, then N-1.
                int curIdx = (delIdx == 0) ? N - 1 : 0;
                Node* cursorA = a[curIdx];
                Node* cursorB = b[curIdx];

                Node* deletedA = NULL; Node* deletedB = NULL;
                Node* afterA = OnDeleteOld(listA, a[delIdx], cursorA, &deletedA);
                Node* afterB = OnDeleteNew(listB, b[delIdx], cursorB, &deletedB);

                CHECK(afterA == cursorA);   // cursor untouched when the deleted row isn't it
                CHECK(afterB == cursorB);
                CHECK(listA.IsLive(afterA));
                CHECK(listB.IsLive(afterB));
            }
        }
    }

    CHECK(sawOldUAF);   // the fixture must actually exercise the ASAN-confirmed repro shape

    return microtest::Summary();
}
