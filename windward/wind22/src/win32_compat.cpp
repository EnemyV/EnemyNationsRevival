//---------------------------------------------------------------------------
// win32_compat.cpp — POSIX implementation of the Win32 shim declared in
// win32_compat.h. Linux build only (the file is excluded from the MSVC build).
//
// Handle model: HANDLE is a pointer to a small polymorphic EnObject. CreateFile
// returns a FileObj, CreateEvent an EventObj, CreateThread a ThreadObj, etc.
// CloseHandle and WaitForSingleObject dispatch on the object kind.
//---------------------------------------------------------------------------

#ifdef _WIN32
#error "win32_compat.cpp is the Linux shim and must not be compiled on Windows"
#endif

#include "win32_compat.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include <csignal>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <sched.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>   // _NSGetExecutablePath (no /proc on macOS)
#endif

//===========================================================================
// Last-error (per-thread).
//===========================================================================
static thread_local DWORD g_lastError = 0;
extern "C" DWORD GetLastError(void)            { return g_lastError; }
extern "C" void  SetLastError(DWORD err)       { g_lastError = err; }

//===========================================================================
// Rect / point geometry (pure math).
//===========================================================================
extern "C" BOOL IsRectEmpty(const RECT* p) { return (!p || p->right <= p->left || p->bottom <= p->top) ? TRUE : FALSE; }
extern "C" BOOL PtInRect(const RECT* p, POINT pt) {
    return (p && pt.x >= p->left && pt.x < p->right && pt.y >= p->top && pt.y < p->bottom) ? TRUE : FALSE;
}
extern "C" BOOL SetRect(RECT* p, int l, int t, int r, int b) { if (!p) return FALSE; p->left=l; p->top=t; p->right=r; p->bottom=b; return TRUE; }
extern "C" BOOL SetRectEmpty(RECT* p) { if (!p) return FALSE; p->left=p->top=p->right=p->bottom=0; return TRUE; }
extern "C" BOOL CopyRect(RECT* d, const RECT* s) { if (!d || !s) return FALSE; *d = *s; return TRUE; }
extern "C" BOOL OffsetRect(RECT* p, int dx, int dy) { if (!p) return FALSE; p->left+=dx; p->right+=dx; p->top+=dy; p->bottom+=dy; return TRUE; }
extern "C" BOOL InflateRect(RECT* p, int dx, int dy) { if (!p) return FALSE; p->left-=dx; p->right+=dx; p->top-=dy; p->bottom+=dy; return TRUE; }
extern "C" BOOL EqualRect(const RECT* a, const RECT* b) {
    return (a && b && a->left==b->left && a->top==b->top && a->right==b->right && a->bottom==b->bottom) ? TRUE : FALSE;
}
extern "C" BOOL IntersectRect(RECT* d, const RECT* a, const RECT* b) {
    if (!d || !a || !b) return FALSE;
    LONG l = a->left > b->left ? a->left : b->left;
    LONG t = a->top  > b->top  ? a->top  : b->top;
    LONG r = a->right < b->right ? a->right : b->right;
    LONG bo= a->bottom< b->bottom? a->bottom: b->bottom;
    if (l < r && t < bo) { d->left=l; d->top=t; d->right=r; d->bottom=bo; return TRUE; }
    SetRectEmpty(d); return FALSE;
}
extern "C" BOOL UnionRect(RECT* d, const RECT* a, const RECT* b) {
    if (!d || !a || !b) return FALSE;
    BOOL ea = IsRectEmpty(a), eb = IsRectEmpty(b);
    if (ea && eb) { SetRectEmpty(d); return FALSE; }
    if (ea) { *d = *b; return TRUE; }
    if (eb) { *d = *a; return TRUE; }
    d->left   = a->left   < b->left   ? a->left   : b->left;
    d->top    = a->top    < b->top    ? a->top    : b->top;
    d->right  = a->right  > b->right  ? a->right  : b->right;
    d->bottom = a->bottom > b->bottom ? a->bottom : b->bottom;
    return TRUE;
}

//===========================================================================
// Critical sections → std::recursive_mutex (CRITICAL_SECTION is recursive).
//===========================================================================
extern "C" void InitializeCriticalSection(LPCRITICAL_SECTION cs) { if (cs) cs->opaque = new std::recursive_mutex(); }
extern "C" void DeleteCriticalSection(LPCRITICAL_SECTION cs)     { if (cs && cs->opaque) { delete static_cast<std::recursive_mutex*>(cs->opaque); cs->opaque = nullptr; } }
extern "C" void EnterCriticalSection(LPCRITICAL_SECTION cs)      { if (cs && cs->opaque) static_cast<std::recursive_mutex*>(cs->opaque)->lock(); }
extern "C" void LeaveCriticalSection(LPCRITICAL_SECTION cs)      { if (cs && cs->opaque) static_cast<std::recursive_mutex*>(cs->opaque)->unlock(); }
extern "C" BOOL TryEnterCriticalSection(LPCRITICAL_SECTION cs)   { return (cs && cs->opaque && static_cast<std::recursive_mutex*>(cs->opaque)->try_lock()) ? TRUE : FALSE; }

//===========================================================================
// Polymorphic kernel-object model behind HANDLE.
//===========================================================================
namespace {

struct EnObject {
    enum Kind { K_EVENT, K_MUTEX, K_THREAD, K_FILE, K_FIND } kind;
    explicit EnObject(Kind k) : kind(k) {}
    virtual ~EnObject() {}
};

struct EventObj : EnObject {
    std::mutex m;
    std::condition_variable cv;
    bool signaled;
    bool manual;
    EventObj(bool manualReset, bool initial) : EnObject(K_EVENT), signaled(initial), manual(manualReset) {}
};

struct MutexObj : EnObject {
    std::timed_mutex m;
    MutexObj() : EnObject(K_MUTEX) {}
};

struct ThreadObj : EnObject {
    pthread_t tid;
    LPTHREAD_START_ROUTINE start;
    LPVOID param;
    DWORD exitCode;
    bool finished;
    bool joined;
    // CREATE_SUSPENDED start gate.
    std::mutex gm;
    std::condition_variable gcv;
    int suspendCount;   // >0 means the start trampoline is gated
    std::mutex donem;
    std::condition_variable donecv;
    ThreadObj() : EnObject(K_THREAD), tid(0), start(nullptr), param(nullptr),
                  exitCode(0), finished(false), joined(false), suspendCount(0) {}
};

struct FileObj : EnObject {
    int fd;
    explicit FileObj(int f) : EnObject(K_FILE), fd(f) {}
};

struct FindObj : EnObject {
    DIR* dir;
    std::string dirPath;
    std::string pattern;   // basename glob
    FindObj() : EnObject(K_FIND), dir(nullptr) {}
};

void* thread_trampoline(void* arg) {
    ThreadObj* t = static_cast<ThreadObj*>(arg);
    // Honor CREATE_SUSPENDED: wait until ResumeThread drops the suspend count.
    {
        std::unique_lock<std::mutex> lk(t->gm);
        t->gcv.wait(lk, [t]{ return t->suspendCount <= 0; });
    }
    DWORD rc = t->start ? t->start(t->param) : 0;
    {
        std::lock_guard<std::mutex> lk(t->donem);
        t->exitCode = rc;
        t->finished = true;
    }
    t->donecv.notify_all();
    return nullptr;
}

} // namespace

//===========================================================================
// Events / mutexes / waits.
//===========================================================================
extern "C" HANDLE CreateEventA(LPSECURITY_ATTRIBUTES, BOOL manualReset, BOOL initialState, LPCSTR) {
    return new EventObj(manualReset != FALSE, initialState != FALSE);
}
extern "C" BOOL SetEvent(HANDLE h) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_EVENT) return FALSE;
    EventObj* e = static_cast<EventObj*>(o);
    { std::lock_guard<std::mutex> lk(e->m); e->signaled = true; }
    if (e->manual) e->cv.notify_all(); else e->cv.notify_one();
    return TRUE;
}
extern "C" BOOL ResetEvent(HANDLE h) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_EVENT) return FALSE;
    EventObj* e = static_cast<EventObj*>(o);
    std::lock_guard<std::mutex> lk(e->m); e->signaled = false; return TRUE;
}
extern "C" HANDLE CreateMutexA(LPSECURITY_ATTRIBUTES, BOOL initialOwner, LPCSTR) {
    MutexObj* m = new MutexObj();
    if (initialOwner) m->m.lock();
    return m;
}
extern "C" BOOL ReleaseMutex(HANDLE h) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_MUTEX) return FALSE;
    static_cast<MutexObj*>(o)->m.unlock(); return TRUE;
}

extern "C" DWORD WaitForSingleObject(HANDLE h, DWORD ms) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o) return WAIT_FAILED;
    switch (o->kind) {
        case EnObject::K_EVENT: {
            EventObj* e = static_cast<EventObj*>(o);
            std::unique_lock<std::mutex> lk(e->m);
            // ms==0 is a NON-BLOCKING poll (Win32: return immediately with current
            // state). Must NOT touch the condvar — cv.wait_for(0ms) still does a full
            // pthread_cond_timedwait syscall (~0.5ms/call here), and CheckYield polls
            // this ~1600x/s from the sim loop (was ~790ms/s = the FPS killer).
            if (ms == 0) {
                if (!e->signaled) return WAIT_TIMEOUT;
                if (!e->manual) e->signaled = false;   // auto-reset
                return WAIT_OBJECT_0;
            }
            if (ms == INFINITE) {
                e->cv.wait(lk, [e]{ return e->signaled; });
            } else if (!e->cv.wait_for(lk, std::chrono::milliseconds(ms), [e]{ return e->signaled; })) {
                return WAIT_TIMEOUT;
            }
            if (!e->manual) e->signaled = false;   // auto-reset
            return WAIT_OBJECT_0;
        }
        case EnObject::K_MUTEX: {
            MutexObj* m = static_cast<MutexObj*>(o);
            if (ms == INFINITE) { m->m.lock(); return WAIT_OBJECT_0; }
            return m->m.try_lock_for(std::chrono::milliseconds(ms)) ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
        }
        case EnObject::K_THREAD: {
            ThreadObj* t = static_cast<ThreadObj*>(o);
            std::unique_lock<std::mutex> lk(t->donem);
            if (ms == INFINITE) { t->donecv.wait(lk, [t]{ return t->finished; }); return WAIT_OBJECT_0; }
            return t->donecv.wait_for(lk, std::chrono::milliseconds(ms), [t]{ return t->finished; })
                       ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
        }
        default: return WAIT_FAILED;
    }
}

extern "C" DWORD WaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL waitAll, DWORD ms) {
    // Minimal: the codebase uses single-object waits almost exclusively. Support
    // waitAll by waiting each in turn (INFINITE only); otherwise poll briefly.
    if (!handles || count == 0) return WAIT_FAILED;
    if (waitAll) {
        for (DWORD i = 0; i < count; ++i)
            if (WaitForSingleObject(handles[i], ms) == WAIT_FAILED) return WAIT_FAILED;
        return WAIT_OBJECT_0;
    }
    for (;;) {
        for (DWORD i = 0; i < count; ++i)
            if (WaitForSingleObject(handles[i], 0) == WAIT_OBJECT_0) return WAIT_OBJECT_0 + i;
        if (ms != INFINITE) return WAIT_TIMEOUT;
        struct timespec ts = {0, 1000000}; nanosleep(&ts, nullptr);
    }
}

extern "C" BOOL CloseHandle(HANDLE h) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || h == INVALID_HANDLE_VALUE) return FALSE;
    switch (o->kind) {
        case EnObject::K_THREAD: {
            ThreadObj* t = static_cast<ThreadObj*>(o);
            if (t->tid && !t->joined) { pthread_detach(t->tid); t->joined = true; }
            break;
        }
        case EnObject::K_FILE: {
            FileObj* f = static_cast<FileObj*>(o);
            if (f->fd >= 0) ::close(f->fd);
            break;
        }
        case EnObject::K_FIND: {
            FindObj* fi = static_cast<FindObj*>(o);
            if (fi->dir) ::closedir(fi->dir);
            break;
        }
        default: break;
    }
    delete o;
    return TRUE;
}

//===========================================================================
// Threads.
//===========================================================================
extern "C" HANDLE CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T,
                               LPTHREAD_START_ROUTINE start, LPVOID param,
                               DWORD flags, LPDWORD threadId) {
    ThreadObj* t = new ThreadObj();
    t->start = start;
    t->param = param;
    t->suspendCount = (flags & CREATE_SUSPENDED) ? 1 : 0;
    if (pthread_create(&t->tid, nullptr, thread_trampoline, t) != 0) {
        g_lastError = (DWORD)errno;
        delete t;
        return nullptr;
    }
    if (threadId) *threadId = (DWORD)(uintptr_t)t->tid;
    return t;
}

extern "C" DWORD ResumeThread(HANDLE h) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_THREAD) return (DWORD)-1;
    ThreadObj* t = static_cast<ThreadObj*>(o);
    DWORD prev;
    { std::lock_guard<std::mutex> lk(t->gm); prev = (DWORD)t->suspendCount; if (t->suspendCount > 0) t->suspendCount--; }
    t->gcv.notify_all();
    return prev;
}

extern "C" DWORD SuspendThread(HANDLE h) {
    // True preemptive suspension of a running thread has no portable pthreads
    // equivalent. We honor it only for the not-yet-started (CREATE_SUSPENDED)
    // case; once running, this is a counted no-op (logged once).
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_THREAD) return (DWORD)-1;
    ThreadObj* t = static_cast<ThreadObj*>(o);
    DWORD prev;
    { std::lock_guard<std::mutex> lk(t->gm); prev = (DWORD)t->suspendCount; t->suspendCount++; }
    return prev;
}

extern "C" BOOL SetThreadPriority(HANDLE, int) { return TRUE; }   // best-effort no-op
extern "C" int  GetThreadPriority(HANDLE)      { return THREAD_PRIORITY_NORMAL; }
extern "C" DWORD GetCurrentThreadId(void)      { return (DWORD)(uintptr_t)pthread_self(); }
extern "C" HANDLE GetCurrentThread(void)       { return (HANDLE)(uintptr_t)pthread_self(); }
extern "C" void ExitThread(DWORD code)         { pthread_exit((void*)(uintptr_t)code); }
extern "C" BOOL TerminateThread(HANDLE h, DWORD) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_THREAD) return FALSE;
    ThreadObj* t = static_cast<ThreadObj*>(o);
    if (t->tid) pthread_cancel(t->tid);
    return TRUE;
}
extern "C" BOOL GetExitCodeThread(HANDLE h, LPDWORD code) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_THREAD) return FALSE;
    ThreadObj* t = static_cast<ThreadObj*>(o);
    if (code) *code = t->finished ? t->exitCode : 0x103 /*STILL_ACTIVE*/;
    return TRUE;
}
extern "C" void Sleep(DWORD ms) {
    if (ms == 0) { sched_yield(); return; }
    struct timespec ts = { (time_t)(ms / 1000), (long)((ms % 1000) * 1000000L) };
    nanosleep(&ts, nullptr);
}

//===========================================================================
// (Interlocked* live as width-agnostic inline templates in win32_compat.h —
//  callers pass either int32_t* or 64-bit long* on LP64.)
//===========================================================================
// Timing.
//===========================================================================
static uint64_t mono_ns(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
extern "C" DWORD timeGetTime(void) { return (DWORD)(mono_ns() / 1000000ull); }
extern "C" DWORD GetTickCount(void) { return (DWORD)(mono_ns() / 1000000ull); }
extern "C" BOOL QueryPerformanceCounter(LARGE_INTEGER* p)   { if (p) p->QuadPart = (LONGLONG)mono_ns(); return TRUE; }
extern "C" BOOL QueryPerformanceFrequency(LARGE_INTEGER* p) { if (p) p->QuadPart = 1000000000ll; return TRUE; }
extern "C" UINT timeBeginPeriod(UINT) { return 0; }
extern "C" UINT timeEndPeriod(UINT)   { return 0; }

// Multimedia timer → a background pthread that fires the callback. Supports
// TIME_ONESHOT and TIME_PERIODIC. timeKillEvent stops it.
namespace {
struct MMTimer {
    LPTIMECALLBACK cb; DWORD_PTR user; UINT delay; bool periodic; UINT flags;
    std::atomic<bool> stop; UINT id;
};
// Leaked on purpose (never destroyed). Timers are created PTHREAD_CREATE_DETACHED,
// so a periodic timer thread can still be alive at process exit. If static
// destruction destroyed these first, the thread's next lock would hit a destroyed
// std::mutex -> std::mutex::lock() throws std::system_error -> std::terminate ->
// SIGABRT (observed at shutdown via mm_timer_thread). A never-destroyed heap
// singleton outlives every thread, so the lock is always valid.
static std::mutex& g_mmMutex() {
    static std::mutex* m = new std::mutex();
    return *m;
}
static std::map<UINT, MMTimer*>& g_mmTimers() {
    static std::map<UINT, MMTimer*>* m = new std::map<UINT, MMTimer*>();
    return *m;
}
UINT g_mmNextId = 1;
void* mm_timer_thread(void* arg) {
    MMTimer* t = static_cast<MMTimer*>(arg);
    do {
        struct timespec ts = { (time_t)(t->delay / 1000), (long)((t->delay % 1000) * 1000000L) };
        nanosleep(&ts, nullptr);
        if (t->stop.load()) break;
        // The callback parameter's meaning depends on the flags. With
        // TIME_CALLBACK_EVENT_SET/PULSE (0x10/0x20) `cb` is actually an event
        // HANDLE to signal — calling it as a function would jump to a data
        // address and crash. Only TIME_CALLBACK_FUNCTION (0x00) is a real fn.
        const UINT cbtype = t->flags & 0x0030u;
        if (cbtype == 0x0010u || cbtype == 0x0020u) {   // EVENT_SET / EVENT_PULSE
            if (t->cb) SetEvent((HANDLE)t->cb);
        } else {                                         // CALLBACK_FUNCTION
            if (t->cb) t->cb(t->id, 0, t->user, 0, 0);
        }
    } while (t->periodic && !t->stop.load());
    // The thread owns the timer once started: drop it from the registry and
    // free it here (covers both one-shot self-completion and timeKillEvent).
    // Done under the lock so a concurrent timeKillEvent either still finds us
    // (and just sets stop) or misses us entirely — never a use-after-free.
    { std::lock_guard<std::mutex> lk(g_mmMutex()); g_mmTimers().erase(t->id); }
    delete t;
    return nullptr;
}
} // namespace

extern "C" UINT timeSetEvent(UINT delay, UINT, LPTIMECALLBACK cb, DWORD_PTR user, UINT flags) {
    MMTimer* t = new MMTimer();
    t->cb = cb; t->user = user; t->delay = delay ? delay : 1; t->flags = flags;
    t->periodic = (flags & 0x0001u) != 0;   // TIME_PERIODIC
    t->stop.store(false);
    UINT myid;
    { std::lock_guard<std::mutex> lk(g_mmMutex()); myid = t->id = g_mmNextId++; g_mmTimers()[t->id] = t; }
    // Detached from birth: the timer thread frees itself on exit, so nothing
    // joins it and the MMTimer's lifetime never depends on a stored pthread_t
    // (which a fast one-shot could free before pthread_create even returned).
    pthread_attr_t attr; pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t th;
    int rc = pthread_create(&th, &attr, mm_timer_thread, t);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        std::lock_guard<std::mutex> lk(g_mmMutex()); g_mmTimers().erase(myid); delete t; return 0;
    }
    return myid;
}
extern "C" UINT timeKillEvent(UINT id) {
    std::lock_guard<std::mutex> lk(g_mmMutex());
    auto it = g_mmTimers().find(id);
    if (it == g_mmTimers().end()) return 11;   // MMSYSERR_INVALPARAM (unknown timer id)
    // Signal stop only; the timer thread removes itself from the map and frees
    // its MMTimer when it next wakes (it was created detached — no join here).
    it->second->stop.store(true);
    return 0;                                 // TIMERR_NOERROR
}
static void fill_systemtime(SYSTEMTIME* st, const struct tm& tmv, long ms) {
    if (!st) return;
    st->wYear=(WORD)(tmv.tm_year+1900); st->wMonth=(WORD)(tmv.tm_mon+1); st->wDayOfWeek=(WORD)tmv.tm_wday;
    st->wDay=(WORD)tmv.tm_mday; st->wHour=(WORD)tmv.tm_hour; st->wMinute=(WORD)tmv.tm_min;
    st->wSecond=(WORD)tmv.tm_sec; st->wMilliseconds=(WORD)ms;
}
extern "C" void GetLocalTime(SYSTEMTIME* st) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); struct tm tmv; localtime_r(&ts.tv_sec, &tmv);
    fill_systemtime(st, tmv, ts.tv_nsec / 1000000L);
}
extern "C" void GetSystemTime(SYSTEMTIME* st) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); struct tm tmv; gmtime_r(&ts.tv_sec, &tmv);
    fill_systemtime(st, tmv, ts.tv_nsec / 1000000L);
}

//===========================================================================
// Debug / diagnostics.
//===========================================================================
extern "C" void OutputDebugStringA(LPCSTR s) { if (s) { fputs(s, stderr); } }
// Matches Perf.cpp's `extern "C" ... RtlCaptureStackBackTrace(...)` declaration.
extern "C" USHORT RtlCaptureStackBackTrace(DWORD, DWORD, PVOID*, PDWORD) { return 0; }
extern "C" void DebugBreak(void)             { raise(SIGTRAP); }
extern "C" BOOL IsDebuggerPresent(void)      { return FALSE; }

//===========================================================================
// Modules / dynamic loading.
//===========================================================================
extern "C" HMODULE GetModuleHandleA(LPCSTR name) {
    if (!name) return (HMODULE)1;            // sentinel "main module" handle
    void* h = dlopen(name, RTLD_NOW | RTLD_NOLOAD);
    return (HMODULE)h;
}
extern "C" DWORD GetModuleFileNameA(HMODULE, LPSTR buf, DWORD size) {
    if (!buf || size == 0) return 0;
#ifdef __APPLE__
    // macOS has no /proc; ask dyld for the executable path.
    uint32_t bufsize = (uint32_t)size;
    if (_NSGetExecutablePath(buf, &bufsize) != 0) { buf[0] = 0; return 0; }
    return (DWORD)strlen(buf);
#else
    ssize_t n = readlink("/proc/self/exe", buf, size - 1);
    if (n < 0) { buf[0] = 0; return 0; }
    buf[n] = 0;
    return (DWORD)n;
#endif
}
extern "C" HMODULE LoadLibraryA(LPCSTR name)        { return (HMODULE)dlopen(name, RTLD_NOW); }
extern "C" BOOL    FreeLibrary(HMODULE m)           { return (m && dlclose(m) == 0) ? TRUE : FALSE; }
extern "C" void*   GetProcAddress(HMODULE m, LPCSTR name) { return (m && name) ? dlsym(m, name) : nullptr; }

//===========================================================================
// File I/O.
//===========================================================================
// Game data uses Windows-style backslash separators (e.g. ".\ENations.dat",
// "data\terrain"). Translate '\' -> '/' for the host filesystem. Returns a
// thread-local buffer valid until the next call on the same thread.
namespace {
const char* en_normpath(const char* p) {
    if (!p) return p;
    static thread_local std::string buf;
    buf.assign(p);
    for (char& c : buf) if (c == '\\') c = '/';
    return buf.c_str();
}
} // namespace

extern "C" HANDLE CreateFileA(LPCSTR rawname, DWORD access, DWORD, LPSECURITY_ATTRIBUTES,
                              DWORD disposition, DWORD, HANDLE) {
    if (!rawname) { g_lastError = ERROR_FILE_NOT_FOUND; return INVALID_HANDLE_VALUE; }
    const char* name = en_normpath(rawname);
    int flags = 0;
    bool wr = (access & GENERIC_WRITE) != 0, rd = (access & GENERIC_READ) != 0;
    if (wr && rd) flags = O_RDWR; else if (wr) flags = O_WRONLY; else flags = O_RDONLY;
    switch (disposition) {
        case CREATE_NEW:        flags |= O_CREAT | O_EXCL; break;
        case CREATE_ALWAYS:     flags |= O_CREAT | O_TRUNC; break;
        case OPEN_EXISTING:     break;
        case OPEN_ALWAYS:       flags |= O_CREAT; break;
        case TRUNCATE_EXISTING: flags |= O_TRUNC; break;
        default: break;
    }
    int fd = ::open(name, flags, 0644);
    if (fd < 0) { g_lastError = (DWORD)errno; return INVALID_HANDLE_VALUE; }
    return new FileObj(fd);
}
static int file_fd(HANDLE h) {
    EnObject* o = static_cast<EnObject*>(h);
    return (o && o->kind == EnObject::K_FILE) ? static_cast<FileObj*>(o)->fd : -1;
}
extern "C" BOOL ReadFile(HANDLE h, LPVOID buf, DWORD toRead, LPDWORD read, LPVOID) {
    int fd = file_fd(h); if (fd < 0) return FALSE;
    ssize_t n = ::read(fd, buf, toRead);
    if (n < 0) { g_lastError = (DWORD)errno; if (read) *read = 0; return FALSE; }
    if (read) *read = (DWORD)n;
    return TRUE;
}
extern "C" BOOL WriteFile(HANDLE h, LPCVOID buf, DWORD toWrite, LPDWORD written, LPVOID) {
    int fd = file_fd(h); if (fd < 0) return FALSE;
    ssize_t n = ::write(fd, buf, toWrite);
    if (n < 0) { g_lastError = (DWORD)errno; if (written) *written = 0; return FALSE; }
    if (written) *written = (DWORD)n;
    return TRUE;
}
extern "C" DWORD SetFilePointer(HANDLE h, LONG dist, LONG* distHigh, DWORD method) {
    int fd = file_fd(h); if (fd < 0) return INVALID_SET_FILE_POINTER;
    int whence = method == FILE_CURRENT ? SEEK_CUR : method == FILE_END ? SEEK_END : SEEK_SET;
    int64_t off = (int64_t)dist;
    if (distHigh) off |= ((int64_t)*distHigh) << 32;
    off_t r = ::lseek(fd, (off_t)off, whence);
    if (r == (off_t)-1) { g_lastError = (DWORD)errno; return INVALID_SET_FILE_POINTER; }
    if (distHigh) *distHigh = (LONG)((int64_t)r >> 32);
    return (DWORD)((uint64_t)r & 0xFFFFFFFFu);
}
extern "C" DWORD GetFileSize(HANDLE h, LPDWORD sizeHigh) {
    int fd = file_fd(h); if (fd < 0) return INVALID_FILE_ATTRIBUTES;
    struct stat stv; if (fstat(fd, &stv) != 0) { g_lastError = (DWORD)errno; return INVALID_FILE_ATTRIBUTES; }
    if (sizeHigh) *sizeHigh = (DWORD)((uint64_t)stv.st_size >> 32);
    return (DWORD)((uint64_t)stv.st_size & 0xFFFFFFFFu);
}
extern "C" BOOL SetEndOfFile(HANDLE h) {
    int fd = file_fd(h); if (fd < 0) return FALSE;
    off_t pos = ::lseek(fd, 0, SEEK_CUR);
    return (ftruncate(fd, pos) == 0) ? TRUE : FALSE;
}
extern "C" BOOL FlushFileBuffers(HANDLE h) { int fd = file_fd(h); return (fd >= 0 && fsync(fd) == 0) ? TRUE : FALSE; }
extern "C" DWORD GetFileAttributesA(LPCSTR name) {
    struct stat stv; if (!name || stat(en_normpath(name), &stv) != 0) return INVALID_FILE_ATTRIBUTES;
    return S_ISDIR(stv.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}
extern "C" BOOL GetFileAttributesExA(LPCSTR name, GET_FILEEX_INFO_LEVELS, LPVOID info) {
    if (!name || !info) return FALSE;
    struct stat stv; if (!name || stat(en_normpath(name), &stv) != 0) { g_lastError = (DWORD)errno; return FALSE; }
    WIN32_FILE_ATTRIBUTE_DATA* d = (WIN32_FILE_ATTRIBUTE_DATA*)info;
    memset(d, 0, sizeof(*d));
    d->dwFileAttributes = S_ISDIR(stv.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    d->nFileSizeLow  = (DWORD)((uint64_t)stv.st_size & 0xFFFFFFFFu);
    d->nFileSizeHigh = (DWORD)((uint64_t)stv.st_size >> 32);
    return TRUE;
}
extern "C" BOOL CreateDirectoryA(LPCSTR name, LPSECURITY_ATTRIBUTES) { return (name && mkdir(en_normpath(name), 0755) == 0) ? TRUE : FALSE; }
extern "C" DWORD GetCurrentDirectoryA(DWORD size, LPSTR buf) {
    if (!buf) return 0; if (!getcwd(buf, size)) return 0; return (DWORD)strlen(buf);
}
extern "C" BOOL SetCurrentDirectoryA(LPCSTR name) { return (name && chdir(en_normpath(name)) == 0) ? TRUE : FALSE; }
extern "C" BOOL DeleteFileA(LPCSTR name) { return (name && unlink(en_normpath(name)) == 0) ? TRUE : FALSE; }

static void fill_find(WIN32_FIND_DATAA* d, const char* dirPath, struct dirent* de) {
    memset(d, 0, sizeof(*d));
    std::string full = std::string(dirPath) + "/" + de->d_name;
    struct stat stv;
    if (stat(full.c_str(), &stv) == 0) {
        d->dwFileAttributes = S_ISDIR(stv.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        d->nFileSizeLow  = (DWORD)((uint64_t)stv.st_size & 0xFFFFFFFFu);
        d->nFileSizeHigh = (DWORD)((uint64_t)stv.st_size >> 32);
    } else {
        d->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    }
    strncpy(d->cFileName, de->d_name, MAX_PATH - 1);
}
// Win32 FindFirstFile and the CRT _findfirst are case-INSENSITIVE; emulate that
// with FNM_CASEFOLD so asset/save enumeration behaves the same on case-sensitive
// Linux/macOS filesystems. Fall back to case-sensitive if the platform lacks it.
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif
// Split a Win32-style glob into (dir, name), translating '\' separators to '/'
// so a pattern like "saves\\*.SAV" opens the right directory on a POSIX host.
namespace {
void split_glob(const char* pattern, std::string& dir, std::string& glob) {
    std::string pat(pattern);
    for (char& c : pat) if (c == '\\') c = '/';
    size_t slash = pat.find_last_of('/');
    if (slash == std::string::npos) { dir = "."; glob = pat; }
    else { dir = pat.substr(0, slash); glob = pat.substr(slash + 1); if (dir.empty()) dir = "/"; }
}
} // namespace
extern "C" HANDLE FindFirstFileA(LPCSTR pattern, LPWIN32_FIND_DATAA data) {
    if (!pattern || !data) return INVALID_HANDLE_VALUE;
    std::string dir, glob; split_glob(pattern, dir, glob);
    DIR* dp = opendir(dir.c_str());
    if (!dp) { g_lastError = ERROR_FILE_NOT_FOUND; return INVALID_HANDLE_VALUE; }
    FindObj* f = new FindObj(); f->dir = dp; f->dirPath = dir; f->pattern = glob;
    struct dirent* de;
    while ((de = readdir(dp)) != nullptr) {
        if (fnmatch(f->pattern.c_str(), de->d_name, FNM_CASEFOLD) == 0) { fill_find(data, f->dirPath.c_str(), de); return f; }
    }
    closedir(dp); delete f; g_lastError = ERROR_FILE_NOT_FOUND; return INVALID_HANDLE_VALUE;
}
extern "C" BOOL FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA data) {
    EnObject* o = static_cast<EnObject*>(h);
    if (!o || o->kind != EnObject::K_FIND || !data) return FALSE;
    FindObj* f = static_cast<FindObj*>(o);
    struct dirent* de;
    while ((de = readdir(f->dir)) != nullptr) {
        if (fnmatch(f->pattern.c_str(), de->d_name, FNM_CASEFOLD) == 0) { fill_find(data, f->dirPath.c_str(), de); return TRUE; }
    }
    return FALSE;
}
extern "C" BOOL FindClose(HANDLE h) { return CloseHandle(h); }

// --- MSVC io.h _findfirst family over opendir/readdir/fnmatch ---
namespace {
struct FindData { DIR* dir; std::string dirPath, pattern; };
void fill_finddata(struct _finddata_t* d, const std::string& dirPath, struct dirent* de) {
    memset(d, 0, sizeof(*d));
    std::string full = dirPath + "/" + de->d_name;
    struct stat stv;
    if (stat(full.c_str(), &stv) == 0) {
        d->attrib = S_ISDIR(stv.st_mode) ? _A_SUBDIR : _A_NORMAL;
        d->size = (_fsize_t)stv.st_size;
        d->time_write = (long)stv.st_mtime;
    }
    strncpy(d->name, de->d_name, sizeof(d->name) - 1);
}
} // namespace
extern "C" intptr_t _findfirst(const char* spec, struct _finddata_t* data) {
    if (!spec || !data) return -1;
    std::string dir, glob; split_glob(spec, dir, glob);
    DIR* dp = opendir(dir.c_str());
    if (!dp) { g_lastError = ERROR_FILE_NOT_FOUND; return -1; }
    FindData* f = new FindData{ dp, dir, glob };
    struct dirent* de;
    while ((de = readdir(dp)) != nullptr) {
        if (fnmatch(f->pattern.c_str(), de->d_name, FNM_CASEFOLD) == 0) { fill_finddata(data, f->dirPath, de); return (intptr_t)f; }
    }
    closedir(dp); delete f; g_lastError = ERROR_FILE_NOT_FOUND; return -1;
}
extern "C" int _findnext(intptr_t handle, struct _finddata_t* data) {
    FindData* f = (FindData*)handle; if (!f || !data) return -1;
    struct dirent* de;
    while ((de = readdir(f->dir)) != nullptr) {
        if (fnmatch(f->pattern.c_str(), de->d_name, FNM_CASEFOLD) == 0) { fill_finddata(data, f->dirPath, de); return 0; }
    }
    return -1;
}
extern "C" int _findclose(intptr_t handle) {
    FindData* f = (FindData*)handle; if (!f) return -1;
    if (f->dir) closedir(f->dir); delete f; return 0;
}

//===========================================================================
// Registry → flat file at $HOME/.config/enations/registry.ini.
// Store format per line:  ROOT\subkey\value=<type>:<payload>
//===========================================================================
namespace {

std::mutex g_regMutex;
std::map<std::string, std::string> g_reg;   // fullpath -> "type:payload"
bool g_regLoaded = false;

struct RegKey { std::string path; };

std::string config_path() {
    const char* home = getenv("HOME");
    if (!home) { struct passwd* pw = getpwuid(getuid()); home = pw ? pw->pw_dir : "/tmp"; }
    return std::string(home) + "/.config/enations";
}
std::string registry_file() { return config_path() + "/registry.ini"; }

std::string root_name(HKEY k) {
    if (k == HKEY_CLASSES_ROOT)  return "HKCR";
    if (k == HKEY_CURRENT_USER)  return "HKCU";
    if (k == HKEY_LOCAL_MACHINE) return "HKLM";
    if (k == HKEY_USERS)         return "HKU";
    return std::string();   // not a predefined root
}
bool is_predefined(HKEY k) {
    return k == HKEY_CLASSES_ROOT || k == HKEY_CURRENT_USER ||
           k == HKEY_LOCAL_MACHINE || k == HKEY_USERS;
}
std::string key_path(HKEY k) {
    std::string r = root_name(k);
    if (!r.empty()) return r;
    return static_cast<RegKey*>(k)->path;   // opened key
}
void load_registry() {
    if (g_regLoaded) return;
    g_regLoaded = true;
    FILE* fp = fopen(registry_file().c_str(), "r");
    if (!fp) return;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char* nl = strchr(line, '\n'); if (nl) *nl = 0;
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        g_reg[line] = eq + 1;
    }
    fclose(fp);
}
void save_registry() {
    std::string dir = config_path();
    // mkdir -p (two levels at most: ~/.config and ~/.config/enations)
    std::string parent = dir.substr(0, dir.find_last_of('/'));
    mkdir(parent.c_str(), 0755);
    mkdir(dir.c_str(), 0755);
    FILE* fp = fopen(registry_file().c_str(), "w");
    if (!fp) return;
    for (std::map<std::string,std::string>::iterator it = g_reg.begin(); it != g_reg.end(); ++it)
        fprintf(fp, "%s=%s\n", it->first.c_str(), it->second.c_str());
    fclose(fp);
}

// Values are stored as "<type>:<payload>" on a single line. REG_DWORD payloads
// are decimal and REG_BINARY payloads are lowercase hex; every other type is the
// raw string (kept human-readable). A string carrying bytes that would break the
// line-oriented file (NUL/CR/LF) is promoted to REG_BINARY hex so it still
// round-trips byte-for-byte. Type-prefixed (not mode-prefixed) so a value that
// itself contains ':' — e.g. a Windows path — never needs escaping.
std::string hex_encode(const unsigned char* d, size_t n) {
    static const char* H = "0123456789abcdef";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s.push_back(H[d[i] >> 4]); s.push_back(H[d[i] & 0xF]); }
    return s;
}
std::string hex_decode(const std::string& s) {
    auto val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string out; out.reserve(s.size() / 2);
    for (size_t i = 0; i + 1 < s.size(); i += 2) {
        int hi = val(s[i]), lo = val(s[i + 1]);
        if (hi < 0 || lo < 0) break;
        out.push_back((char)((hi << 4) | lo));
    }
    return out;
}
bool needs_hex(const unsigned char* d, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (d[i] == '\0' || d[i] == '\n' || d[i] == '\r') return true;
    return false;
}

} // namespace

extern "C" LONG RegOpenKeyExA(HKEY key, LPCSTR sub, DWORD, DWORD, PHKEY result) {
    if (!result) return ERROR_FILE_NOT_FOUND;
    std::lock_guard<std::mutex> lk(g_regMutex);
    load_registry();
    std::string base = key_path(key);
    std::string full = (sub && *sub) ? base + "\\" + sub : base;
    *result = new RegKey{ full };
    return ERROR_SUCCESS;
}
extern "C" LONG RegCreateKeyExA(HKEY key, LPCSTR sub, DWORD, LPSTR, DWORD, DWORD,
                                LPSECURITY_ATTRIBUTES, PHKEY result, LPDWORD disp) {
    if (disp) *disp = 1;   // REG_CREATED_NEW_KEY (we don't track key existence)
    return RegOpenKeyExA(key, sub, 0, 0, result);
}
extern "C" LONG RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD, LPDWORD type,
                                 LPBYTE data, LPDWORD size) {
    std::lock_guard<std::mutex> lk(g_regMutex);
    load_registry();
    std::string full = key_path(key) + "\\" + (name ? name : "");
    std::map<std::string,std::string>::iterator it = g_reg.find(full);
    if (it == g_reg.end()) return ERROR_FILE_NOT_FOUND;
    // Decode "<type>:<payload>". The type drives how payload is read, so a value
    // that itself contains ':' (e.g. a Windows path) stays intact: REG_DWORD is
    // decimal, REG_BINARY is hex (embedded NUL/newline-safe), everything else is
    // the raw string. A line with no ':' is treated as a bare REG_SZ string.
    const std::string& enc = it->second;
    DWORD vtype = REG_SZ;
    std::string payload = enc;
    size_t colon = enc.find(':');
    if (colon != std::string::npos) {
        vtype = (DWORD)atoi(enc.substr(0, colon).c_str());
        payload = enc.substr(colon + 1);
    }
    if (type) *type = vtype;
    if (vtype == REG_DWORD) {
        DWORD v = (DWORD)strtoul(payload.c_str(), nullptr, 10);
        if (data && size && *size >= sizeof(DWORD)) memcpy(data, &v, sizeof(DWORD));
        if (size) *size = sizeof(DWORD);
        return ERROR_SUCCESS;
    }
    std::string raw = (vtype == REG_BINARY) ? hex_decode(payload) : payload;
    // String types report length including the terminating NUL; binary does not.
    bool isString = (vtype == REG_SZ || vtype == REG_EXPAND_SZ);
    DWORD need = (DWORD)raw.size() + (isString ? 1u : 0u);
    // Win32 semantics: a size query (data==NULL) returns ERROR_SUCCESS with
    // *size set — NOT ERROR_MORE_DATA (callers like EnGetProfileStdString
    // size-query first and treat anything != ERROR_SUCCESS as "not found").
    // ERROR_MORE_DATA is only for a NON-NULL buffer that's too small.
    if (data && size && *size < need) { *size = need; return ERROR_MORE_DATA; }
    if (data && size) {
        memcpy(data, raw.data(), raw.size());
        if (isString) data[raw.size()] = '\0';
    }
    if (size) *size = need;
    return ERROR_SUCCESS;
}
extern "C" LONG RegSetValueExA(HKEY key, LPCSTR name, DWORD, DWORD type, const BYTE* data, DWORD size) {
    std::lock_guard<std::mutex> lk(g_regMutex);
    load_registry();
    std::string full = key_path(key) + "\\" + (name ? name : "");
    if (type == REG_DWORD && data) {
        DWORD v = 0; memcpy(&v, data, sizeof(DWORD));
        char buf[32]; snprintf(buf, sizeof(buf), "%u:%u", REG_DWORD, v);
        g_reg[full] = buf;
    } else if (data) {
        size_t n = size;
        // Drop exactly one trailing NUL if the caller included it (Win32 string
        // values are conventionally passed with the terminator counted in size).
        if ((type == REG_SZ || type == REG_EXPAND_SZ) && n > 0 && data[n - 1] == '\0')
            --n;
        // Store readable text for normal strings; hex for true REG_BINARY or any
        // value containing bytes that would break the line-oriented file (NUL/CR/LF).
        if (type == REG_BINARY || needs_hex(data, n)) {
            char tbuf[16]; snprintf(tbuf, sizeof(tbuf), "%u:", (unsigned)REG_BINARY);
            g_reg[full] = std::string(tbuf) + hex_encode(data, n);
        } else {
            char tbuf[16]; snprintf(tbuf, sizeof(tbuf), "%u:", (unsigned)type);
            g_reg[full] = std::string(tbuf) + std::string((const char*)data, n);
        }
    }
    save_registry();
    return ERROR_SUCCESS;
}
extern "C" LONG RegDeleteValueA(HKEY key, LPCSTR name) {
    std::lock_guard<std::mutex> lk(g_regMutex);
    load_registry();
    g_reg.erase(key_path(key) + "\\" + (name ? name : ""));
    save_registry();
    return ERROR_SUCCESS;
}
extern "C" LONG RegCloseKey(HKEY key) {
    if (key && !is_predefined(key)) delete static_cast<RegKey*>(key);
    return ERROR_SUCCESS;
}

//===========================================================================
// MMIO — a real RIFF chunk reader over a POSIX fd. CMmio (mmio.cpp) is live:
// it reads game data/asset files. Supports filename open and the FOURCC_DOS
// "embedded file" path (borrow a CreateFileA HANDLE's fd at an offset).
//===========================================================================
namespace {
struct MmioObj { int fd; bool ownsFd; };
static const FOURCC FCC_RIFF = mmioFOURCC('R','I','F','F');
static const FOURCC FCC_LIST = mmioFOURCC('L','I','S','T');

bool read_exact(int fd, void* buf, size_t n) {
    char* p = (char*)buf; size_t got = 0;
    while (got < n) { ssize_t r = ::read(fd, p + got, n - got); if (r <= 0) return false; got += (size_t)r; }
    return true;
}
bool read_dword_le(int fd, DWORD* out) {
    unsigned char b[4]; if (!read_exact(fd, b, 4)) return false;
    *out = (DWORD)b[0] | ((DWORD)b[1] << 8) | ((DWORD)b[2] << 16) | ((DWORD)b[3] << 24);
    return true;
}
} // namespace

extern "C" HMMIO mmioOpen(char* filename, LPMMIOINFO info, DWORD /*flags*/) {
    int fd = -1; bool owns = false;
    if (filename) {
        fd = ::open(en_normpath(filename), O_RDONLY);
        if (fd < 0) { g_lastError = (DWORD)errno; if (info) info->wErrorRet = 1; return NULL; }
        owns = true;
    } else if (info && info->fccIOProc == FOURCC_DOS) {
        // Embedded file: the data archive shares ONE fd across many embedded
        // CMmio's (datafile.cpp passes m_pDataFile->m_hFile; its own comment
        // flags "BUGBUG - problem"). Win32's MMIO seeks the shared OS handle per
        // I/O so they don't interfere; we instead re-open the SAME file to get an
        // INDEPENDENT open-file-description (its own position). dup() would NOT
        // work — it shares the offset.
        int borrowed = file_fd((HANDLE)info->adwInfo[0]);
        if (borrowed < 0) { if (info) info->wErrorRet = 1; return NULL; }
        char path[4096];
#ifdef __APPLE__
        // macOS has no /proc/self/fd; recover the path with fcntl(F_GETPATH).
        if (::fcntl(borrowed, F_GETPATH, path) != -1) {
            fd = ::open(path, O_RDONLY); owns = (fd >= 0);
        }
#else
        char proc[64];
        snprintf(proc, sizeof(proc), "/proc/self/fd/%d", borrowed);
        ssize_t n = ::readlink(proc, path, sizeof(path) - 1);
        if (n > 0) { path[n] = 0; fd = ::open(path, O_RDONLY); owns = (fd >= 0); }
#endif
        if (fd < 0) { fd = borrowed; owns = false; }   // fallback: share (last resort)
    } else {
        return NULL;
    }
    MmioObj* m = new MmioObj{ fd, owns };
    if (info) info->wErrorRet = 0;
    return m;
}
extern "C" DWORD mmioClose(HMMIO h, UINT flags) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m) return 0;
    if (m->ownsFd && !(flags & MMIO_FHOPEN)) ::close(m->fd);
    delete m; return 0;
}
extern "C" LONG mmioRead(HMMIO h, char* buf, LONG len) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m) return -1;
    ssize_t r = ::read(m->fd, buf, (size_t)len); return (LONG)r;
}
extern "C" LONG mmioWrite(HMMIO h, const char* buf, LONG len) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m) return -1;
    ssize_t r = ::write(m->fd, buf, (size_t)len); return (LONG)r;
}
extern "C" LONG mmioSeek(HMMIO h, LONG offset, int origin) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m) return -1;
    int whence = origin == 1 ? SEEK_CUR : origin == 2 ? SEEK_END : SEEK_SET;
    return (LONG)::lseek(m->fd, offset, whence);
}
extern "C" DWORD mmioGetInfo(HMMIO h, LPMMIOINFO info, UINT) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m || !info) return 1;
    memset(info, 0, sizeof(*info));
    info->lDiskOffset = (LONG)::lseek(m->fd, 0, SEEK_CUR);   // unbuffered: GetOffset() == disk pos
    return 0;
}
extern "C" DWORD mmioDescend(HMMIO h, LPMMCKINFO cki, const MMCKINFO* parent, UINT flags) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m || !cki) return 1;
    long parentEnd = -1;
    // parent->dwDataOffset points at the formtype (Win32 semantics) and cksize
    // includes it, so end-of-parent-data == dwDataOffset + cksize (no -4).
    if (parent) parentEnd = (long)parent->dwDataOffset + (long)parent->cksize;
    for (;;) {
        long pos = (long)::lseek(m->fd, 0, SEEK_CUR);
        if (parentEnd >= 0 && pos + 8 > parentEnd) return 1;   // no matching chunk in parent
        FOURCC ckid; DWORD cksize;
        if (!read_dword_le(m->fd, &ckid) || !read_dword_le(m->fd, &cksize)) return 1;  // EOF
        bool container = (ckid == FCC_RIFF || ckid == FCC_LIST);
        FOURCC fccType = 0;
        if (container) { if (!read_dword_le(m->fd, &fccType)) return 1; }
        long dataOffset = (long)::lseek(m->fd, 0, SEEK_CUR);    // after header (+ formtype for container)
        if (getenv("EN_MMIO_DEBUG")) {
            char k[5] = {(char)(ckid&0xff),(char)((ckid>>8)&0xff),(char)((ckid>>16)&0xff),(char)((ckid>>24)&0xff),0};
            char ft[5] = {(char)(fccType&0xff),(char)((fccType>>8)&0xff),(char)((fccType>>16)&0xff),(char)((fccType>>24)&0xff),0};
            fprintf(stderr, "[mmioDescend flags=%x] pos=%ld ckid='%s' cksize=%u fcc='%s' parentEnd=%ld\n",
                    flags, pos, k, cksize, container?ft:"", parentEnd);
        }
        bool match;
        if (flags & MMIO_FINDRIFF)      match = (ckid == FCC_RIFF && fccType == cki->fccType);
        else if (flags & MMIO_FINDLIST) match = (ckid == FCC_LIST && fccType == cki->fccType);
        else if (flags & MMIO_FINDCHUNK) match = (ckid == cki->ckid);
        else                            match = true;          // descend whatever is here
        if (match) {
            cki->ckid = ckid; cki->cksize = cksize; cki->fccType = fccType;
            // Win32 semantics: for a RIFF/LIST chunk dwDataOffset is the offset
            // of the FORM/LIST type (chunkStart+8), not the offset after it; the
            // file pointer is still left after the formtype. cksize includes the
            // formtype, so dwDataOffset + cksize == end-of-chunk (no -4 needed in
            // mmioAscend). For a plain chunk dwDataOffset is the data (chunkStart+8).
            cki->dwDataOffset = (DWORD)(container ? dataOffset - 4 : dataOffset);
            cki->dwFlags = 0;
            return 0;   // file left positioned at the chunk's data (after formtype)
        }
        long next = pos + 8 + (long)cksize + ((long)cksize & 1);   // skip chunk (+ even pad)
        ::lseek(m->fd, next, SEEK_SET);
    }
}
extern "C" DWORD mmioAscend(HMMIO h, LPMMCKINFO cki, UINT) {
    MmioObj* m = static_cast<MmioObj*>(h); if (!m || !cki) return 1;
    // dwDataOffset is at the formtype for containers (Win32 semantics) and at the
    // data for plain chunks; cksize spans from there to end in both cases.
    long end = (long)cki->dwDataOffset + (long)cki->cksize;
    if (end & 1) end++;                                            // RIFF word-padding
    if (getenv("EN_MMIO_DEBUG")) {
        char k[5] = {(char)(cki->ckid&0xff),(char)((cki->ckid>>8)&0xff),(char)((cki->ckid>>16)&0xff),(char)((cki->ckid>>24)&0xff),0};
        fprintf(stderr, "[mmioAscend] ckid='%s' dwDataOffset=%u cksize=%u -> end=%ld\n", k, cki->dwDataOffset, cki->cksize, end);
    }
    ::lseek(m->fd, end, SEEK_SET);
    return 0;
}

//===========================================================================
// SRW locks → pthread_rwlock, lazily allocated (supports SRWLOCK_INIT = {0}).
//===========================================================================
static std::mutex g_srwInitMutex;
static pthread_rwlock_t* srw_get(PSRWLOCK lock) {
    if (!lock) return nullptr;
    if (!lock->Ptr) {
        std::lock_guard<std::mutex> lk(g_srwInitMutex);
        if (!lock->Ptr) {
            pthread_rwlock_t* rw = new pthread_rwlock_t;
            pthread_rwlock_init(rw, nullptr);
            lock->Ptr = rw;
        }
    }
    return static_cast<pthread_rwlock_t*>(lock->Ptr);
}
extern "C" void InitializeSRWLock(PSRWLOCK lock)        { if (lock) lock->Ptr = nullptr; srw_get(lock); }
extern "C" void AcquireSRWLockExclusive(PSRWLOCK lock)  { pthread_rwlock_t* rw = srw_get(lock); if (rw) pthread_rwlock_wrlock(rw); }
extern "C" void ReleaseSRWLockExclusive(PSRWLOCK lock)  { pthread_rwlock_t* rw = srw_get(lock); if (rw) pthread_rwlock_unlock(rw); }
extern "C" void AcquireSRWLockShared(PSRWLOCK lock)     { pthread_rwlock_t* rw = srw_get(lock); if (rw) pthread_rwlock_rdlock(rw); }
extern "C" void ReleaseSRWLockShared(PSRWLOCK lock)     { pthread_rwlock_t* rw = srw_get(lock); if (rw) pthread_rwlock_unlock(rw); }

//===========================================================================
// In-process window registry + message routing. Mirrors the Win32 model the
// MFC CWndStub layer expects: RegisterClass stores a WNDPROC by class name;
// CreateWindowEx allocates an HWND, fires WM_NCCREATE/WM_CREATE through the
// class wndproc (so StaticWndProc stashes `this` via SetWindowLongPtr); and
// SendMessage routes to the window's wndproc. PostMessage queues for the pump
// (PeekMessage/GetMessage/DispatchMessage). The real on-screen window is SDL;
// these are the logical MFC windows the game still drives by message.
//===========================================================================
namespace {
struct WndRec {
    WNDPROC  wndproc;
    LONG_PTR userData;     // GWLP_USERDATA
    LONG     style, exStyle;
    HWND     parent;
    bool     valid;
    int      x, y, cx, cy; // window rect (screen x,y + client size cx,cy)
};
std::mutex g_wndMutex;
std::map<HWND, WndRec*> g_wndMap;
std::map<std::string, WNDPROC> g_classMap;
uintptr_t g_nextHwnd = 0x00010000;
struct PostedMsg { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; };
std::mutex g_msgMutex;
std::deque<PostedMsg> g_msgQueue;

WndRec* wnd_lookup(HWND h) {
    if (!h) return nullptr;
    std::lock_guard<std::mutex> lk(g_wndMutex);
    auto it = g_wndMap.find(h);
    return (it != g_wndMap.end()) ? it->second : nullptr;
}
} // namespace

ATOM RegisterClass(const WNDCLASS* wc) {
    if (wc && wc->lpszClassName) { std::lock_guard<std::mutex> lk(g_wndMutex); g_classMap[wc->lpszClassName] = wc->lpfnWndProc; }
    return 1;
}
ATOM RegisterClassA(const WNDCLASS* wc) { return RegisterClass(wc); }
ATOM RegisterClassExA(const WNDCLASSEXA* wc) {
    if (wc && wc->lpszClassName) { std::lock_guard<std::mutex> lk(g_wndMutex); g_classMap[wc->lpszClassName] = wc->lpfnWndProc; }
    return 1;
}

HWND CreateWindowExA(DWORD exStyle, LPCSTR cls, LPCSTR name, DWORD style,
                                int x, int y, int cx, int cy, HWND parent, HMENU hMenu,
                                HINSTANCE hInst, LPVOID param) {
    WndRec* w = new WndRec();
    w->wndproc = nullptr; w->userData = 0; w->style = (LONG)style; w->exStyle = (LONG)exStyle;
    w->parent = parent; w->valid = true;
    // Remember the geometry so GetClientRect/GetWindowRect report a real size.
    // CW_USEDEFAULT (0x80000000) and non-positive sizes fall back to the screen
    // so the top-level main window still reports a sane client area.
    bool defW = (cx <= 0) || (cx == (int)0x80000000);
    bool defH = (cy <= 0) || (cy == (int)0x80000000);
    w->x = (x == (int)0x80000000) ? 0 : x;
    w->y = (y == (int)0x80000000) ? 0 : y;
    w->cx = defW ? GetSystemMetrics(SM_CXSCREEN) : cx;
    w->cy = defH ? GetSystemMetrics(SM_CYSCREEN) : cy;
    HWND h;
    {
        std::lock_guard<std::mutex> lk(g_wndMutex);
        if (cls) { auto it = g_classMap.find(cls); if (it != g_classMap.end()) w->wndproc = it->second; }
        h = (HWND)(g_nextHwnd += 16);
        g_wndMap[h] = w;
    }
    // Fire WM_NCCREATE then WM_CREATE through the class wndproc so the MFC
    // StaticWndProc stashes `this` (via GWLP_USERDATA) and sets m_hWnd.
    if (w->wndproc) {
        CREATESTRUCTA cs = {};
        cs.lpCreateParams = param; cs.hInstance = hInst; cs.hMenu = hMenu; cs.hwndParent = parent;
        cs.cy = cy; cs.cx = cx; cs.y = y; cs.x = x; cs.style = (LONG)style;
        cs.lpszName = name; cs.lpszClass = cls; cs.dwExStyle = exStyle;
        w->wndproc(h, WM_NCCREATE, 0, (LPARAM)&cs);
        w->wndproc(h, WM_CREATE,   0, (LPARAM)&cs);
        // Real Windows sends WM_SIZE after WM_CREATE with the client dimensions;
        // the game's OnSize handlers compute layout (panel rects, m_iRow3, etc.)
        // from it. Deliver it so in-game windows size correctly.
        w->wndproc(h, WM_SIZE, 0 /*SIZE_RESTORED*/, (LPARAM)MAKELONG(w->cx, w->cy));
    }
    return h;
}
HWND CreateWindowEx(DWORD a, LPCSTR b, LPCSTR c, DWORD d, int e, int f, int g, int h2,
                               HWND i, HMENU j, HINSTANCE k, LPVOID l) {
    return CreateWindowExA(a, b, c, d, e, f, g, h2, i, j, k, l);
}

// Window geometry. The client rect is origin-relative (0,0,cx,cy); the window
// rect is screen-relative. We don't model non-client borders (the game's windows
// are borderless/popup), so client size == window size.
BOOL GetClientRect(HWND h, LPRECT r) {
    if (!r) return FALSE;
    WndRec* w = wnd_lookup(h);
    r->left = r->top = 0;
    if (w) { r->right = w->cx; r->bottom = w->cy; }
    else   { r->right = GetSystemMetrics(SM_CXSCREEN); r->bottom = GetSystemMetrics(SM_CYSCREEN); }
    return TRUE;
}
BOOL GetWindowRect(HWND h, LPRECT r) {
    if (!r) return FALSE;
    WndRec* w = wnd_lookup(h);
    if (w) { r->left = w->x; r->top = w->y; r->right = w->x + w->cx; r->bottom = w->y + w->cy; }
    else   { r->left = r->top = 0; r->right = GetSystemMetrics(SM_CXSCREEN); r->bottom = GetSystemMetrics(SM_CYSCREEN); }
    return TRUE;
}
BOOL MoveWindow(HWND h, int x, int y, int cx, int cy, BOOL) {
    WndRec* w = wnd_lookup(h);
    if (!w) return FALSE;
    bool sized = (cx != w->cx) || (cy != w->cy);
    w->x = x; w->y = y; w->cx = cx; w->cy = cy;
    if (sized && w->wndproc)
        w->wndproc(h, WM_SIZE, 0, (LPARAM)MAKELONG(cx, cy));
    return TRUE;
}
BOOL SetWindowPos(HWND h, HWND, int x, int y, int cx, int cy, UINT flags) {
    WndRec* w = wnd_lookup(h);
    if (!w) return FALSE;
    const UINT SWP_NOMOVE_ = 0x0002, SWP_NOSIZE_ = 0x0001;
    if (!(flags & SWP_NOMOVE_)) { w->x = x; w->y = y; }
    if (!(flags & SWP_NOSIZE_)) {
        bool sized = (cx != w->cx) || (cy != w->cy);
        w->cx = cx; w->cy = cy;
        if (sized && w->wndproc)
            w->wndproc(h, WM_SIZE, 0, (LPARAM)MAKELONG(cx, cy));
    }
    return TRUE;
}

LRESULT SendMessageA(HWND h, UINT msg, WPARAM w, LPARAM l) {
    WndRec* r = wnd_lookup(h);
    return (r && r->wndproc) ? r->wndproc(h, msg, w, l) : 0;
}
LRESULT SendMessage(HWND h, UINT msg, WPARAM w, LPARAM l) { return SendMessageA(h, msg, w, l); }
BOOL PostMessageA(HWND h, UINT msg, WPARAM w, LPARAM l) {
    std::lock_guard<std::mutex> lk(g_msgMutex);
    PostedMsg pm = { h, msg, w, l }; g_msgQueue.push_back(pm); return TRUE;
}
BOOL PostMessage(HWND h, UINT msg, WPARAM w, LPARAM l) { return PostMessageA(h, msg, w, l); }
// Real implementation (was a no-op stub): enqueue WM_QUIT so CConquerApp::Run's
// PeekMessage/BaseYield sees it and returns ExitInstance(). Without this the menu
// Exit button (→ CloseApp → PostQuitMessage) never terminated the main loop.
// Desktop resolution reported by GetSystemMetrics(SM_CXSCREEN/CYSCREEN). Default
// to the classic 1280x1024; linux_main overwrites it with the real display size.
int g_enScreenW = 1280;
int g_enScreenH = 1024;
// Live mouse position (area-window client coords), fed from SDL motion events so
// GetCursorPos() returns something real (was hardcoded 0,0). See win32_compat.h.
int g_enCursorX = 0;
int g_enCursorY = 0;
void en_SetCursorPos(int x, int y) { g_enCursorX = x; g_enCursorY = y; }
void en_SetScreenMetrics(int w, int h) {
    if ( w > 0 ) g_enScreenW = w;
    if ( h > 0 ) g_enScreenH = h;
}
void PostQuitMessage(int code) {
    std::lock_guard<std::mutex> lk(g_msgMutex);
    PostedMsg pm = { NULL, (UINT)WM_QUIT, (WPARAM)code, 0 };
    g_msgQueue.push_back(pm);
}
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }

BOOL DestroyWindow(HWND h) {
    WndRec* r = wnd_lookup(h);
    if (!r) return FALSE;
    if (r->wndproc) { r->wndproc(h, WM_DESTROY, 0, 0); r->wndproc(h, WM_NCDESTROY, 0, 0); }
    { std::lock_guard<std::mutex> lk(g_wndMutex); g_wndMap.erase(h); }
    delete r;
    return TRUE;
}
BOOL IsWindow(HWND h) { return wnd_lookup(h) != nullptr ? TRUE : FALSE; }

LONG_PTR GetWindowLongPtr(HWND h, int idx) {
    WndRec* r = wnd_lookup(h); if (!r) return 0;
    switch (idx) {
        case GWLP_USERDATA: return r->userData;
        case GWL_STYLE:     return r->style;
        case GWL_EXSTYLE:   return r->exStyle;
        case GWLP_WNDPROC:  return (LONG_PTR)r->wndproc;
        default: return 0;
    }
}
LONG_PTR SetWindowLongPtr(HWND h, int idx, LONG_PTR v) {
    WndRec* r = wnd_lookup(h); if (!r) return 0;
    LONG_PTR old = 0;
    switch (idx) {
        case GWLP_USERDATA: old = r->userData; r->userData = v; break;
        case GWL_STYLE:     old = r->style;    r->style = (LONG)v; break;
        case GWL_EXSTYLE:   old = r->exStyle;  r->exStyle = (LONG)v; break;
        default: break;
    }
    return old;
}
LONG GetWindowLong(HWND h, int idx)  { return (LONG)GetWindowLongPtr(h, idx); }
LONG GetWindowLongA(HWND h, int idx) { return (LONG)GetWindowLongPtr(h, idx); }
LONG SetWindowLong(HWND h, int idx, LONG v)  { return (LONG)SetWindowLongPtr(h, idx, v); }
LONG SetWindowLongA(HWND h, int idx, LONG v) { return (LONG)SetWindowLongPtr(h, idx, v); }

BOOL PeekMessage(LPMSG msg, HWND, UINT, UINT, UINT remove) {
    std::lock_guard<std::mutex> lk(g_msgMutex);
    if (g_msgQueue.empty()) return FALSE;
    PostedMsg pm = g_msgQueue.front();
    if (remove & 0x0001u /*PM_REMOVE*/) g_msgQueue.pop_front();
    if (msg) { msg->hwnd = pm.hwnd; msg->message = pm.message; msg->wParam = pm.wParam; msg->lParam = pm.lParam; msg->time = timeGetTime(); msg->pt.x = msg->pt.y = 0; }
    return TRUE;
}
BOOL GetMessage(LPMSG msg, HWND hw, UINT a, UINT b) {
    // Blocking-ish: drain queue; if empty, report no quit but nothing to do.
    // (The game's real loop is PeekMessage-based; GetMessage is the legacy path.)
    for (;;) {
        if (PeekMessage(msg, hw, a, b, 0x0001u)) {
            if (msg && msg->message == WM_QUIT) return 0;
            return 1;
        }
        struct timespec ts = {0, 2000000}; nanosleep(&ts, nullptr);
    }
}
BOOL TranslateMessage(const MSG*) { return FALSE; }
LRESULT DispatchMessage(const MSG* m) { return m ? SendMessageA(m->hwnd, m->message, m->wParam, m->lParam) : 0; }

// Dispatch ONLY the vp* network messages (WM_VPNOTIFY/WM_VPFLOWON/WM_VPFLOWOFF)
// from the queue to their window proc, leaving every other message queued. Used by
// SDL2Dialog::DoModal (POSIX MP port): a modal dialog — the MP "Waiting for Players"
// lobby and the "Available Games" search — runs its own SDL loop that does NOT pump
// MFC messages (that would fire WM_KICKIDLE->OnIdle->nested DoModal). But the vp*
// engine delivers discovered-session / joined-player events as WM_VPNOTIFY posted to
// the game window (-> CNetApi::OnNetMsg). vpPumpNet(0) services the sockets and
// PRODUCES those notifications; this DELIVERS them, so the modal sees the network
// without the full pump. Net messages only -> no WM_KICKIDLE re-entrancy.
extern "C" void EnPumpNetMessages() {
    const UINT WM_VPNOTIFY  = WM_USER + 1000;
    const UINT WM_VPFLOWOFF = WM_USER + 1001;
    const UINT WM_VPFLOWON  = WM_USER + 1002;

    std::deque<PostedMsg> net;
    {
        std::lock_guard<std::mutex> lk(g_msgMutex);
        std::deque<PostedMsg> rest;
        while (!g_msgQueue.empty()) {
            PostedMsg pm = g_msgQueue.front();
            g_msgQueue.pop_front();
            if (pm.message == WM_VPNOTIFY || pm.message == WM_VPFLOWOFF || pm.message == WM_VPFLOWON)
                net.push_back(pm);
            else
                rest.push_back(pm);
        }
        g_msgQueue.swap(rest);
    }
    // Dispatch outside the lock — the window proc (OnNetMsg) may PostMessage again.
    while (!net.empty()) {
        PostedMsg pm = net.front();
        net.pop_front();
        SendMessageA(pm.hwnd, pm.message, pm.wParam, pm.lParam);
    }
}

//===========================================================================
// Misc UI.
//===========================================================================
extern "C" int MessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT) {
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return IDOK;
}
