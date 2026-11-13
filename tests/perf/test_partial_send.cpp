#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <string>

static std::string captured;
static int calls = 0;
static bool failAfterFirst = false;
static int simulated_send(int, const void* bytes, size_t length, int)
{
    ++calls;
    if (failAfterFirst && calls > 1) return -1;
    const size_t count = std::min(length, size_t(3));
    captured.append(static_cast<const char*>(bytes), count);
    return static_cast<int>(count);
}
#define send simulated_send
#include "../../enations_latest/src/PerfHarness.cpp"
#undef send
bool PerfHarnessLoadFixture(const char*) { return true; }
bool PerfHarnessReturnToMenu() { return true; }
std::string PerfHarnessControl(const std::string&) { return {}; }
void PerfHarnessSnapshot(char*, size_t, unsigned long long) {}
void PerfHarnessQuit() {}
int main()
{
    reply(-1, "abcdefgh\n");
    assert(captured == "abcdefgh\n" && calls == 3);
    captured.clear(); calls = 0; failAfterFirst = true;
    reply(-1, "abcdefgh\n");
    assert(captured == "abc" && calls == 2);
}
#endif
