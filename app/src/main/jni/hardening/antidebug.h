// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <atomic>

namespace cs::hard {

// Multi-layer anti-debug. None of these are unbeatable in isolation; the value
// comes from running them in concert and at unpredictable times.
//
//   1. ptrace(PTRACE_TRACEME)         once at startup; if a debugger is already
//                                     attached, this fails. We don't kill the
//                                     process directly -- we corrupt internal
//                                     state so it crashes later in unrelated
//                                     code, which is much harder to bypass than
//                                     an obvious "if(debug) abort()".
//   2. /proc/self/status TracerPid    polled on a worker thread.
//   3. timing canaries                a tight CLOCK_MONOTONIC loop; a debugger
//                                     single-stepping introduces orders-of-
//                                     magnitude jitter.
//   4. signal-handler tampering       we install our own SIGTRAP / SIGSEGV
//                                     handlers and verify them periodically.
//   5. /proc/self/maps scan           detects gdbserver, frida-server, lldb,
//                                     and JDWP-style transports loaded into
//                                     the process.
class AntiDebug {
public:
    static AntiDebug& instance();

    // Call once during JNI_OnLoad / Activity.onCreate.
    void initialize();

    // Stops the worker thread (only used at clean shutdown).
    void shutdown();

    // The "are we owned" flag. Game logic is allowed to read this and degrade
    // gameplay (broken physics, scrambled HUD) instead of an obvious abort.
    bool tainted() const { return tainted_.load(std::memory_order_relaxed); }

private:
    AntiDebug() = default;
    void thread_main();

    std::atomic<bool> tainted_{false};
    std::atomic<bool> running_{false};
};

} // namespace cs::hard
