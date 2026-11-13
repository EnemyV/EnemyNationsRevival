// Regression checks for POSIX thread handle completion and early handle close.
#include "win32_compat.h"
#include <atomic>
#include <cstdio>

static DWORD normal(void*) { return 17; }
static DWORD explicit_exit(void*) { ExitThread(23); return 99; }
static std::atomic<bool> released{false};
static DWORD after_close(void* arg) {
    WaitForSingleObject(static_cast<HANDLE>(arg), INFINITE);
    released.store(true);
    return 0;
}

int main() {
    DWORD (*functions[])(void*) = {normal, explicit_exit};
    for (auto fn : functions) {
        HANDLE h = CreateThread(nullptr, 0, fn, nullptr, 0, nullptr);
        if (!h || WaitForSingleObject(h, 5000) != WAIT_OBJECT_0) return 1;
        DWORD code = 0;
        if (!GetExitCodeThread(h, &code) || code != (fn == normal ? 17u : 23u)) return 2;
        CloseHandle(h);
    }
    HANDLE gate = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    HANDLE worker = CreateThread(nullptr, 0, after_close, gate, 0, nullptr);
    if (!gate || !worker || !CloseHandle(worker)) return 3;
    SetEvent(gate);
    for (int i = 0; i < 5000 && !released.load(); ++i) Sleep(1);
    if (!released.load()) return 4;
    CloseHandle(gate);
    std::puts("POSIX thread return, ExitThread and early-close checks passed");
    return 0;
}
