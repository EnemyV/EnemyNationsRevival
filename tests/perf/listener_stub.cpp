#include "../../enations_latest/src/PerfHarness.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

static std::atomic<bool> stopped{false};
bool PerfHarnessLoadFixture(const char* path) { return path && *path && std::string(path) != "invalid-fixture"; }
bool PerfHarnessReturnToMenu() { return true; }
std::string PerfHarnessControl(const std::string& command) { return "OK " + command + "\n"; }
void PerfHarnessSnapshot(char* out, size_t size, unsigned long long frames)
{
    const char* mode = std::getenv("EN_P1_TEST_STATUS_MODE");
    if (mode && std::strcmp(mode, "unterminated") == 0) {
        std::memset(out, 'x', size);
    } else if (mode && (std::strcmp(mode, "boundary") == 0 || std::strcmp(mode, "short") == 0)) {
        const size_t length = size - (std::strcmp(mode, "boundary") == 0 ? 1 : 0);
        std::string status = "OK " + std::string(length - 4, 'x') + "\n";
        std::snprintf(out, size, "%s", status.c_str());
    } else if (mode && std::strcmp(mode, "multiline") == 0) {
        std::snprintf(out, size, "OK first\nsecond\n");
    } else std::snprintf(out, size, "OK frames=%llu ingame=1 players=1\n", frames);
}
void PerfHarnessQuit() { stopped.store(true); }
int main()
{
    PerfHarnessStart();
    for (int i = 0; i != 500 && !stopped.load(); ++i) {
        PerfHarnessService();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    PerfHarnessStop();
    return stopped.load() ? 0 : 1;
}
