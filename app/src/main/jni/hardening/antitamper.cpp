// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "hardening/antitamper.h"
#include "hardening/obfuscate.h"

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace cs::hard {

namespace {

// Standard CRC32 (Castagnoli would need crc32c hw on aarch64; stick to plain
// zlib polynomial for portability across arm64-v8a / x86_64).
uint32_t crc32_buf(const uint8_t* data, size_t n, uint32_t seed = 0xffffffffu) {
    uint32_t c = seed;
    for (size_t i = 0; i < n; ++i) {
        c ^= data[i];
        for (int j = 0; j < 8; ++j) {
            c = (c >> 1) ^ (0xedb88320u & -(c & 1));
        }
    }
    return c ^ 0xffffffffu;
}

// dl_iterate_phdr callback: locate our own libcsnative load segment and the
// PT_LOAD that contains the executable .text.
struct WalkCtx {
    const void* exec_start;
    size_t exec_size;
};

int phdr_cb(struct dl_phdr_info* info, size_t /*size*/, void* user) {
    auto* ctx = static_cast<WalkCtx*>(user);
    if (!info->dlpi_name) return 0;
    const char* nm = info->dlpi_name;
    auto needle = OBF("libcsnative.so");
    if (!std::strstr(nm, needle.c_str())) return 0;
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
        if (ph->p_type == PT_LOAD && (ph->p_flags & PF_X)) {
            ctx->exec_start = reinterpret_cast<const void*>(info->dlpi_addr + ph->p_vaddr);
            ctx->exec_size  = static_cast<size_t>(ph->p_memsz);
            return 1;
        }
    }
    return 0;
}

} // namespace

AntiTamper& AntiTamper::instance() {
    static AntiTamper s;
    return s;
}

void AntiTamper::capture_text_baseline() {
    WalkCtx ctx{nullptr, 0};
    dl_iterate_phdr(phdr_cb, &ctx);
    if (!ctx.exec_start || !ctx.exec_size) return;
    text_start_ = ctx.exec_start;
    text_size_  = ctx.exec_size;
    baseline_crc_ = crc32_buf(static_cast<const uint8_t*>(text_start_), text_size_);
}

void AntiTamper::initialize(const std::string& apk_path) {
    apk_path_ = apk_path;
    capture_text_baseline();
    running_ = true;
    std::thread([this]() {
        ::pthread_setname_np(::pthread_self(), "csn-vfy");
        while (running_) {
            if (text_start_ && text_size_) {
                uint32_t now = crc32_buf(static_cast<const uint8_t*>(text_start_), text_size_);
                if (now != baseline_crc_) tainted_ = true;
            }
            sleep(2);
        }
    }).detach();
}

void AntiTamper::shutdown() { running_ = false; }

} // namespace cs::hard
