// test_route_window_teardown.cpp -- the PRODUCTION vehicle window teardown and the
// PRODUCTION SDL2RouteWindow Close/~/Hide, with only the scene replaced. Built by
// run-route-window-teardown.py, which extracts the bodies verbatim into
// teardown_actual.inc (tests/orders/run-order-lifecycle.py's technique).
//
// Bug: CVehicle::DestroyAllWindows closed the build and load dialogs but never the SDL
// Routes window, so the window kept its CVehicle* after the vehicle died and every
// button (Loop, Delete, Goto, Start...) plus Close() itself dereferenced freed memory.
//
// The scene is a stand-in: an area-window list, a compositor that records removals, and
// vehicles living in static storage so a window that outlives its vehicle can be detected
// by comparing pointer VALUES (never dereferencing the dead vehicle).

#include <cstddef>
#include <cstdio>
#include <new>
#include <vector>
#include <functional>

#define ASSERT_STRICT_VALID(x) ((void)0)

// --------------------------------------------------------------------------- SDL stubs
struct TTF_Font {};
struct SDL_Surface {};
static int g_fontsClosed = 0, g_surfacesFreed = 0;
static void TTF_CloseFont(TTF_Font*) { ++g_fontsClosed; }
static void SDL_FreeSurface(SDL_Surface*) { ++g_surfacesFreed; }

struct SDL2Panel {
    bool visible = true;
    void SetVisible(bool b) { visible = b; }
};

struct SDL2Compositor {
    std::vector<SDL2Panel*> removed;
    void RemovePanel(SDL2Panel* p) { removed.push_back(p); }
};

struct GameWindow {
    SDL2Compositor comp;
    SDL2Compositor* GetCompositor() { return &comp; }
};

// --------------------------------------------------------------------------- units / area
class CUnit { public: virtual ~CUnit() {} };

class CWndArea {
  public:
    enum { normal, veh_route, build_loc };
    CUnit* m_pUnit = nullptr;
    int    m_iMode = normal;
    int    m_iSelectOffs = 0;
    CUnit* GetUnit() const { return m_pUnit; }
    int    GetMode() const { return m_iMode; }
    void   SelectOff() { ++m_iSelectOffs; m_iMode = normal; }
};

typedef CWndArea** POSITION;
class CAreaList {
  public:
    std::vector<CWndArea*> v;
    POSITION GetHeadPosition() { return v.empty() ? nullptr : v.data(); }
    CWndArea* GetNext(POSITION& pos) {
        CWndArea* p = *pos;
        ++pos;
        if (pos == v.data() + v.size()) pos = nullptr;
        return p;
    }
};
static CAreaList theAreaList;

// --------------------------------------------------------------------------- dialogs
struct CWndRoute {               // legacy MFC route window: never created on the SDL build
    void* m_hWnd = nullptr;
    void  DestroyWindow() {}
};

// Non-modal dialog stand-in: EndDialog fires onDone (which nulls the owner's pointer),
// the way SDL2Dialog::EndDialog does; GameWindow's cleanup pass would delete it.
struct SDL2NonModal {
    std::function<void(int)> onDone;
    int ended = 0;
    void EndDialog(int r) { ++ended; if (onDone) { auto f = onDone; onDone = nullptr; f(r); } }
};
using SDL2BuildStructure  = SDL2NonModal;
using SDL2LoadTruckDialog = SDL2NonModal;

class CVehicle;

// --------------------------------------------------------------------------- route window
class SDL2RouteWindow {
  public:
    SDL2RouteWindow(GameWindow* gw, CVehicle* pVeh);
    ~SDL2RouteWindow();
    void Hide();
    void Close();

    GameWindow*  m_gameWindow;
    CVehicle*    m_pVeh;
    SDL2Panel*   m_panel = nullptr;
    TTF_Font*    m_fontSmall = nullptr;
    SDL_Surface* m_bgGold = nullptr;
    SDL_Surface* m_borderH = nullptr;
    SDL_Surface* m_borderV = nullptr;
};

// --------------------------------------------------------------------------- vehicle
class CVehicle : public CUnit {
  public:
    CWndRoute*           m_pWndRoute = nullptr;
    SDL2RouteWindow*     m_pSdlRoute = nullptr;
    SDL2BuildStructure*  m_pSdlBuild = nullptr;
    SDL2LoadTruckDialog* m_pSdlLoad  = nullptr;
    void*                m_pDlgStructure = nullptr;

    void DestroyRouteWindow();
    void DestroyBuildWindow();
    void DestroyAllWindows();

    // the two production call sites: CVehicle::PrepareToDie and CVehicle::~CVehicle
    // (and DestroyWorld, which calls DestroyAllWindows before deleting every vehicle)
    void PrepareToDie() { DestroyAllWindows(); }
    ~CVehicle() override { DestroyAllWindows(); }
};

#include "teardown_actual.inc"

static std::vector<SDL2Panel*> g_panels;

SDL2RouteWindow::SDL2RouteWindow(GameWindow* gw, CVehicle* pVeh) : m_gameWindow(gw), m_pVeh(pVeh) {
    s_openRouteWindows.push_back(this);     // the production constructor's first statement
    m_panel = new SDL2Panel();
    g_panels.push_back(m_panel);
}

// --------------------------------------------------------------------------- harness
static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } \
                              else std::printf("  ok:   %s\n", msg); } while (0)

static GameWindow g_gw;

// Vehicles live in static storage so a destroyed vehicle's ADDRESS stays comparable.
alignas(CVehicle) static unsigned char g_store[4][sizeof(CVehicle)];
static CVehicle* MakeVehicle(int slot) { return new (g_store[slot]) CVehicle(); }

static SDL2RouteWindow* OpenRoute(CVehicle* v) {          // area.cpp ~6191 / ~6313
    if (!v->m_pSdlRoute) v->m_pSdlRoute = new SDL2RouteWindow(&g_gw, v);
    return v->m_pSdlRoute;
}

static bool WindowRefers(CVehicle const* v) {
    for (SDL2RouteWindow* rw : s_openRouteWindows)
        if (rw->m_pVeh == v) return true;
    return false;
}

int main() {
    CWndArea areaA, areaB;
    theAreaList.v = { &areaA, &areaB };

    std::printf("[1] vehicle dies with its Routes window open (PrepareToDie, then dtor)\n");
    {
        CVehicle* v = MakeVehicle(0);
        CVehicle* other = MakeVehicle(1);
        OpenRoute(v);
        SDL2RouteWindow* otherWin = OpenRoute(other);
        SDL2Panel* panel = v->m_pSdlRoute->m_panel;
        areaA.m_pUnit = v;     areaA.m_iMode = CWndArea::veh_route;
        areaB.m_pUnit = other; areaB.m_iMode = CWndArea::veh_route;
        size_t removedBefore = g_gw.comp.removed.size();

        v->PrepareToDie();
        CHECK(v->m_pSdlRoute == nullptr, "m_pSdlRoute is null after DestroyAllWindows");
        CHECK(!WindowRefers(v), "no open route window still holds the dying vehicle");
        CHECK(s_openRouteWindows.size() == 1 && s_openRouteWindows[0] == otherWin,
              "TickScrollRepeats registry keeps only the other vehicle's window");
        CHECK(g_gw.comp.removed.size() == removedBefore + 1 && g_gw.comp.removed.back() == panel,
              "the window's panel was removed from the compositor once");
        CHECK(areaA.m_iMode == CWndArea::normal && areaA.m_iSelectOffs == 1,
              "area window in veh_route for the dying vehicle was taken out (SelectOff)");
        CHECK(areaB.m_iMode == CWndArea::veh_route && areaB.m_iSelectOffs == 0,
              "area window routing a different vehicle is untouched");

        size_t removedMid = g_gw.comp.removed.size();
        int selectOffsMid = areaA.m_iSelectOffs;
        v->~CVehicle();        // second DestroyAllWindows: must be a no-op for the route window
        CHECK(g_gw.comp.removed.size() == removedMid, "dtor's DestroyAllWindows does not close twice");
        CHECK(areaA.m_iSelectOffs == selectOffsMid, "no second SelectOff from the dtor");
        CHECK(!WindowRefers(v), "after the dtor nothing references the vehicle");

        other->~CVehicle();
        CHECK(other->m_pSdlRoute == nullptr && s_openRouteWindows.empty(),
              "other vehicle's window closed with it; registry empty");
    }

    std::printf("[2] vehicle destroyed directly (DestroyWorld / dtor path only)\n");
    {
        CVehicle* v = MakeVehicle(2);
        OpenRoute(v);
        areaA.m_pUnit = nullptr; areaA.m_iMode = CWndArea::normal;
        v->~CVehicle();
        CHECK(!WindowRefers(v) && s_openRouteWindows.empty(), "window does not outlive the vehicle");
    }

    std::printf("[3] user closes the window first, then the vehicle dies\n");
    {
        CVehicle* v = MakeVehicle(3);
        SDL2NonModal build, load;
        v->m_pSdlBuild = &build; build.onDone = [v](int) { v->m_pSdlBuild = nullptr; };
        v->m_pSdlLoad  = &load;  load.onDone  = [v](int) { v->m_pSdlLoad  = nullptr; };
        OpenRoute(v);
        size_t removedBefore = g_gw.comp.removed.size();
        v->m_pSdlRoute->Close();                 // Close button / title-bar X / Start / Auto
        CHECK(v->m_pSdlRoute == nullptr && s_openRouteWindows.empty(), "user Close cleared the pointer");
        v->~CVehicle();
        CHECK(g_gw.comp.removed.size() == removedBefore + 1, "vehicle teardown does not close it again");
        CHECK(build.ended == 1 && load.ended == 1, "build and load dialogs still ended exactly once");
    }

    for (SDL2Panel* p : g_panels) delete p;
    std::printf(g_fail ? "[route_window_teardown] %d FAILED\n" : "[route_window_teardown] all pass\n", g_fail);
    return g_fail ? 1 : 0;
}
