// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <string>

namespace cs::hard {

// Native-side anti-tamper:
//   1. CRC32 of the .text segment of libcsnative.so itself, computed at startup
//      and re-verified every few seconds. If a frida hook overwrites a function
//      prologue, the CRC drifts.
//   2. APK signature pinning -- caller hands us the apk path; we compute a
//      SHA-256 of the v2 signing block and compare to a baked-in expected
//      digest (configured at release-build time; baseline computed on first
//      run for development).
class AntiTamper {
public:
    static AntiTamper& instance();

    void initialize(const std::string& apk_path);
    void shutdown();

    bool tainted() const { return tainted_; }

private:
    AntiTamper() = default;

    void capture_text_baseline();
    void verify_loop();

    std::string apk_path_;
    uint32_t baseline_crc_{0};
    const void* text_start_{nullptr};
    size_t text_size_{0};
    bool tainted_{false};
    bool running_{false};
};

} // namespace cs::hard
