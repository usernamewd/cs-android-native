// Copyright (c) 2026 cs-android-native contributors. MIT License.
//
// Compile-time string XOR obfuscation. When OLLVM/Hikari -sobf is unavailable
// this gives us per-string XOR encryption with a randomized key, decoded at
// first use into a stack buffer that's zeroed when the holder leaves scope.
//
// Usage:
//     auto s = OBF("plant the bomb");
//     puts(s.c_str());
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cs::obf {

// Compile-time PRNG seeded from __TIME__ so each TU gets a different key
// (cheap defense against trivial xor-search reversers).
constexpr uint32_t seed() {
    constexpr const char* t = __TIME__;
    return (static_cast<uint32_t>(t[0]) - '0') * 36000u
         + (static_cast<uint32_t>(t[1]) - '0') * 3600u
         + (static_cast<uint32_t>(t[3]) - '0') * 600u
         + (static_cast<uint32_t>(t[4]) - '0') * 60u
         + (static_cast<uint32_t>(t[6]) - '0') * 10u
         + (static_cast<uint32_t>(t[7]) - '0');
}

constexpr uint32_t lcg(uint32_t s) { return s * 1103515245u + 12345u; }

template <std::size_t N>
struct Encrypted {
    std::array<char, N> data{};
    uint8_t key{0};

    constexpr Encrypted(const char (&in)[N], uint8_t k) : key(k) {
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = static_cast<char>(in[i] ^ (k + static_cast<uint8_t>(i * 7u)));
        }
    }
};

template <std::size_t N>
class Decrypted {
public:
    Decrypted(const Encrypted<N>& e) {
        for (std::size_t i = 0; i < N; ++i) {
            buf_[i] = static_cast<char>(e.data[i] ^ (e.key + static_cast<uint8_t>(i * 7u)));
        }
    }
    ~Decrypted() {
        // Best-effort scrub. Mark volatile so the optimizer can't drop it.
        volatile char* p = buf_;
        for (std::size_t i = 0; i < N; ++i) p[i] = 0;
    }
    const char* c_str() const { return buf_; }

private:
    char buf_[N]{};
};

} // namespace cs::obf

// The actual macro. Wraps a literal in a per-callsite Encrypted blob (constexpr,
// goes into .rodata in encrypted form) plus a Decrypted RAII wrapper that
// materializes the plaintext on the stack at the point of use.
#define OBF(STR) ([](){                                                       \
    constexpr uint8_t k = static_cast<uint8_t>(::cs::obf::lcg(::cs::obf::seed() + __LINE__) >> 8); \
    constexpr ::cs::obf::Encrypted<sizeof(STR)> e(STR, k);                    \
    return ::cs::obf::Decrypted<sizeof(STR)>(e);                              \
}())
