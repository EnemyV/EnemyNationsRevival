#include "PerfHarness.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
static const Socket badSocket = INVALID_SOCKET;
static void closeSocket(Socket s) { closesocket(s); }
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
using Socket = int;
static const Socket badSocket = -1;
static void closeSocket(Socket s) { close(s); }
#endif
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

extern bool PerfHarnessLoadFixture(const char*);
extern bool PerfHarnessReturnToMenu();
extern std::string PerfHarnessControl(const std::string&);
extern void PerfHarnessSnapshot(char*, size_t, unsigned long long);
extern void PerfHarnessQuit();

namespace {
std::atomic<bool> enabled{false};
#ifdef _WIN32
bool winsockStarted = false;
#endif
std::thread receiver;
std::mutex lock;
std::condition_variable changed;
enum class Request { none, load, menu, control, quit };
Request request = Request::none;
std::string controlCommand;
std::string controlResponse;
bool loading = false;
bool completed = false;
bool loadOk = false;
unsigned long long frames = 0;
std::string snapshot = "OK loop_iterations=0 ingame=0 players=0 buildings=0 vehicles=0 game_frame=0 game_seconds=0\n";
std::string fixture;
std::string token;

void reply(Socket s, const std::string& text)
{
    size_t sent = 0;
    while (sent < text.size()) {
#ifdef MSG_NOSIGNAL
        int n = send(s, text.data() + sent, (int)(text.size() - sent), MSG_NOSIGNAL);
#else
        int n = send(s, text.data() + sent, (int)(text.size() - sent), 0);
#endif
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

void serve(Socket listener)
{
    while (enabled.load()) {
        fd_set ready;
        FD_ZERO(&ready);
        FD_SET(listener, &ready);
        timeval poll = {0, 200000};
        int selected = select((int)listener + 1, &ready, nullptr, nullptr, &poll);
        if (selected == 0) continue;
        if (selected < 0) break;
        Socket client = accept(listener, nullptr, nullptr);
        if (client == badSocket) break;
#ifdef _WIN32
        DWORD timeout = 2000;
#else
        timeval timeout = {2, 0};
#ifdef SO_NOSIGPIPE
        int one = 1;
        setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
#endif
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        std::string line;
        bool newline = false;
        while (line.size() < 512) {
            char c;
            if (recv(client, &c, 1, 0) != 1) break;
            if (c == '\n') { newline = true; break; }
            if (c == '\r' || c == '\0') break;
            line.push_back(c);
        }
        if (!newline) { reply(client, "ERR invalid_request\n"); closeSocket(client); continue; }
        const std::string prefix = token + " ";
        if (line.compare(0, prefix.size(), prefix) != 0) {
            reply(client, "ERR unauthorized\n"); closeSocket(client); continue;
        }
        std::string cmd = line.substr(prefix.size());
        if (cmd == "READY") reply(client, "OK protocol=1 fixture=fixture\n");
        else if (cmd == "STATUS") {
            std::lock_guard<std::mutex> guard(lock);
            reply(client, snapshot);
        } else if (cmd == "LOAD fixture" || cmd == "MENU" ||
                   cmd.compare(0, 7, "CAMERA ") == 0 || cmd.compare(0, 6, "SPEED ") == 0 ||
                   cmd.compare(0, 5, "ZOOM ") == 0 ||
                   cmd == "MARK BEGIN" || cmd == "MARK END") {
            std::unique_lock<std::mutex> guard(lock);
            if (loading || request != Request::none) reply(client, "ERR busy\n");
            else {
                completed = false;
                request = cmd == "MENU" ? Request::menu : cmd == "LOAD fixture" ? Request::load : Request::control;
                controlCommand = cmd;
                if (!changed.wait_for(guard, std::chrono::seconds(150), [] { return completed || !enabled.load(); }))
                    reply(client, "ERR load_timeout\n");
                else if (!enabled.load()) reply(client, "ERR stopping\n");
                else if (cmd == "MENU") reply(client, loadOk ? "OK menu\n" : "ERR menu_failed\n");
                else if (cmd == "LOAD fixture") reply(client, loadOk ? "OK loaded\n" : "ERR load_failed\n");
                else reply(client, controlResponse);
            }
        } else if (cmd == "QUIT") {
            std::lock_guard<std::mutex> guard(lock);
            if (loading || request != Request::none) reply(client, "ERR busy\n");
            else { request = Request::quit; reply(client, "OK quitting\n"); }
        } else reply(client, "ERR unknown_command\n");
        closeSocket(client);
    }
    closeSocket(listener);
}
}

void PerfHarnessStart()
{
    const char* t = std::getenv("EN_P1_TOKEN");
    const char* f = std::getenv("EN_P1_FIXTURE");
    const char* p = std::getenv("EN_P1_PORT");
    if (!t || !f || !p || std::strlen(t) < 16 || std::strlen(t) > 128 || !*f) return;
    for (const char* c = t; *c; ++c)
        if (!( (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') )) return;
    char* end = nullptr;
    long port = std::strtol(p, &end, 10);
    const char* portFile = std::getenv("EN_P1_PORT_FILE");
    if (!end || *end || port > 65535 || port < 0 ||
        (port != 0 && port < 1024) || (port == 0 && (!portFile || !*portFile))) return;
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return;
    winsockStarted = true;
#endif
    Socket listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == badSocket) return;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);
    if (bind(listener, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(listener, 4) != 0) {
        closeSocket(listener);
        std::fprintf(stderr, "[P1-HARNESS] listener bind failed\n");
        return;
    }
    if (port == 0) {
#ifdef _WIN32
        int addressLength = sizeof(addr);
#else
        socklen_t addressLength = sizeof(addr);
#endif
        if (getsockname(listener, (sockaddr*)&addr, &addressLength) != 0) {
            closeSocket(listener);
            return;
        }
        const std::string temporary = std::string(portFile) + ".tmp";
        FILE* file = std::fopen(temporary.c_str(), "wb");
        if (!file) { closeSocket(listener); return; }
        const bool wrote = std::fprintf(file, "%u\n", (unsigned)ntohs(addr.sin_port)) > 0;
        const bool closed = std::fclose(file) == 0;
        if (!wrote || !closed || std::rename(temporary.c_str(), portFile) != 0) {
            std::remove(temporary.c_str());
            closeSocket(listener);
            return;
        }
    }
    token = t;
    fixture = f;
    enabled.store(true);
    receiver = std::thread(serve, listener);
    std::fprintf(stderr, "[P1-HARNESS] loopback listener ready\n");
}

void PerfHarnessStop()
{
    enabled.store(false);
    changed.notify_all();
    if (receiver.joinable()) receiver.join();
#ifdef _WIN32
    if (winsockStarted) { WSACleanup(); winsockStarted = false; }
#endif
}

void PerfHarnessService()
{
    if (!enabled.load()) return;
    Request task;
    std::string command;
    {
        std::lock_guard<std::mutex> guard(lock);
        ++frames;
        char buf[1024] = {};
        PerfHarnessSnapshot(buf, sizeof(buf), frames);
        // STATUS is a line protocol. Never publish an unterminated or truncated
        // OK response: clients otherwise wait forever or accept partial fields.
        const char* end = static_cast<const char*>(std::memchr(buf, 0, sizeof(buf)));
        const size_t len = end ? static_cast<size_t>(end - buf) : 0;
        snapshot = (len > 0 && buf[len - 1] == '\n' &&
                    std::memchr(buf, '\n', len - 1) == nullptr)
                       ? std::string(buf, len) : "ERR snapshot_truncated\n";
        task = request;
        command = controlCommand;
        request = Request::none;
        if (task == Request::load || task == Request::menu || task == Request::control) loading = true;
    }
    if (task == Request::load || task == Request::menu || task == Request::control) {
        bool ok = task == Request::load ? PerfHarnessLoadFixture(fixture.c_str()) :
                  task == Request::menu ? PerfHarnessReturnToMenu() : false;
        std::string response = task == Request::control ? PerfHarnessControl(command) : std::string();
        // Publish a game-thread snapshot after control execution and before
        // acknowledging it, so subsequent STATUS cannot return an older one.
        char postControl[1024] = {};
        const bool refreshAfterMark = task == Request::control && command == "MARK END";
        if (refreshAfterMark) PerfHarnessSnapshot(postControl, sizeof(postControl), frames);
        const char* postEnd = static_cast<const char*>(std::memchr(postControl, 0, sizeof(postControl)));
        const size_t postLen = postEnd ? static_cast<size_t>(postEnd - postControl) : 0;
        std::lock_guard<std::mutex> guard(lock);
        if (refreshAfterMark) snapshot = (postLen > 0 && postControl[postLen - 1] == '\n' &&
                    std::memchr(postControl, '\n', postLen - 1) == nullptr)
                       ? std::string(postControl, postLen) : "ERR snapshot_truncated\n";
        loading = false;
        loadOk = ok;
        controlResponse = response;
        completed = true;
        changed.notify_all();
    } else if (task == Request::quit) PerfHarnessQuit();
}
