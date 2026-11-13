#include "../enations_latest/src/controller_zoom_focus.h"
#include <cstdlib>
#define CHECK(v) do { if (!(v)) std::abort(); } while (0)
int main() {
    CHECK(ControllerZoomNeedsTarget(true, true));
    CHECK(!ControllerZoomNeedsTarget(false, true)); // release/device event
    CHECK(!ControllerZoomNeedsTarget(true, false)); // unrelated button down
    CHECK(!ControllerZoomNeedsTarget(false, false)); // other SDL event
    int mainWindow, windowA, windowB, other, areaA, areaB;
    ControllerZoomFocusSelection second(&windowB, &mainWindow, &areaA, true);
    second.consider(&areaA, &windowA, true, true);
    second.consider(&areaB, &windowB, true, true);
    CHECK(second.result() == &areaB);
    ControllerZoomFocusSelection first(&windowA, &mainWindow, &areaB, true);
    first.consider(&areaA, &windowA, true, true);
    first.consider(&areaB, &windowB, true, true);
    CHECK(first.result() == &areaA);
    ControllerZoomFocusSelection attached(&mainWindow, &mainWindow, &areaB, true);
    attached.consider(&areaA, nullptr, false, true);
    attached.consider(&areaB, nullptr, false, true);
    CHECK(attached.result() == &areaB);
    ControllerZoomFocusSelection wrongTop(&mainWindow, &mainWindow, &areaB, true);
    wrongTop.consider(&areaB, &windowB, true, true);
    CHECK(!wrongTop.result());
    ControllerZoomFocusSelection hidden(&windowB, &mainWindow, &areaA, true);
    hidden.consider(&areaB, &windowB, true, false);
    CHECK(!hidden.result());
    ControllerZoomFocusSelection blocked(&windowB, &mainWindow, &areaA, false);
    blocked.consider(&areaB, &windowB, true, true);
    CHECK(!blocked.result());
    ControllerZoomFocusSelection ambiguous(&windowB, &mainWindow, &areaA, true);
    ambiguous.consider(&areaA, &windowB, true, true);
    ambiguous.consider(&areaB, &windowB, true, true);
    CHECK(!ambiguous.result());
    ControllerZoomFocusSelection nofocus(nullptr, &mainWindow, &areaA, true);
    nofocus.consider(&areaA, nullptr, false, true);
    CHECK(!nofocus.result());
    ControllerZoomFocusSelection tool(&other, &mainWindow, &areaA, true);
    tool.consider(&areaA, &windowA, true, true);
    CHECK(!tool.result());
}
