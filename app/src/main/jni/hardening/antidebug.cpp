// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "hardening/antidebug.h"
#include "hardening/obfuscate.h"

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

namespace cs::hard {

namespace {

bool ptrace_self_attach() {
    // Calling PTRACE_TRACEME from the process itself: if a debugger is already
    // attached this returns -1 with errno=EPERM. If nothing is attached we
    // succeed and immediately detach so a real debugger can't attach later
    // without us noticing the second TRACEME failing.
    long r = ::syscall(__NR_ptrace, PTRACE_TRACEME, 0, 0, 0);
    if (r == -1) return true; // someone is already there
    // Detach silently.
    ::syscall(__NR_ptrace, PTRACE_DETACH, 0, 0, 0);
    return false;
}

bool tracer_pid_set() {
    int fd = ::open("/proc/self/status", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    char buf[4096];
    ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (n <= 0) return false;
    buf[n] = 0;
    const char* needle = "TracerPid:";
    char* p = std::strstr(buf, needle);
    if (!p) return false;
    p += std::strlen(needle);
    while (*p == ' ' || *p == '\t') ++p;
    return *p && *p != '0';
}

bool maps_contains_known_tools() {
    int fd = ::open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    char buf[8192];
    ssize_t n;
    bool found = false;
    // We scan in chunks; the file can be > 8K. Keyword strings are obfuscated
    // so a strings(1) dump of the .so won't reveal the watchlist.
    auto k1 = OBF("frida");
    auto k2 = OBF("gdbserver");
    auto k3 = OBF("gum-js-loop");
    auto k4 = OBF("lldb-server");
    auto k5 = OBF("xposed");
    while ((n = ::read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        if (std::strstr(buf, k1.c_str()) || std::strstr(buf, k2.c_str()) ||
            std::strstr(buf, k3.c_str()) || std::strstr(buf, k4.c_str()) ||
            std::strstr(buf, k5.c_str())) {
            found = true;
            break;
        }
    }
    ::close(fd);
    return found;
}

bool timing_canary_tripped() {
    // Run a tight loop and measure wall time. On a real device the bound is
    // sub-millisecond; under a single-stepping debugger it explodes.
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    volatile uint64_t acc = 0;
    for (uint64_t i = 0; i < 200000ULL; ++i) acc += i ^ (i << 1);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    uint64_t ns = (t1.tv_sec - t0.tv_sec) * 1000000000ULL + (t1.tv_nsec - t0.tv_nsec);
    (void)acc;
    // > 50 ms for 200k iterations means *something* is very wrong.
    return ns > 50ULL * 1000000ULL;
}

} // namespace

AntiDebug& AntiDebug::instance() {
    static AntiDebug s;
    return s;
}

void AntiDebug::initialize() {
    // Layer 1: forbid further ptrace via prctl. Most useful on Android < 10
    // where SET_DUMPABLE-based attacks are still common.
    ::prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    // PR_SET_PTRACER(0) means "no process is allowed to ptrace us".
    ::prctl(PR_SET_PTRACER, 0, 0, 0, 0);

    if (ptrace_self_attach() || tracer_pid_set() || maps_contains_known_tools()) {
        tainted_.store(true, std::memory_order_relaxed);
    }

    running_.store(true, std::memory_order_relaxed);
    std::thread([this]() { thread_main(); }).detach();
}

void AntiDebug::shutdown() {
    running_.store(false, std::memory_order_relaxed);
}

void AntiDebug::thread_main() {
    ::pthread_setname_np(::pthread_self(), "csn-aux");
    while (running_.load(std::memory_order_relaxed)) {
        if (tracer_pid_set() || maps_contains_known_tools() || timing_canary_tripped()) {
            tainted_.store(true, std::memory_order_relaxed);
        }
        // Sleep 250-750ms with jitter so polling intervals are not predictable.
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        unsigned r = static_cast<unsigned>(ts.tv_nsec) ^ static_cast<unsigned>(ts.tv_sec);
        long ms = 250 + (r % 500);
        struct timespec req { ms / 1000, (ms % 1000) * 1000000L };
        nanosleep(&req, nullptr);
    }
}

} // namespace cs::hard
