// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "engine/asset.h"

namespace eng {

AssetIO& AssetIO::instance() { static AssetIO s; return s; }

void AssetIO::attach(AAssetManager* mgr) { mgr_ = mgr; }

bool AssetIO::load_text(const char* path, std::string& out) const {
    if (!mgr_) return false;
    // STREAMING + AAsset_read works for both compressed and uncompressed
    // entries; AAsset_getBuffer returns nullptr for compressed entries, which
    // produced silent UB when we used to assign(nullptr, sz) on shaders that
    // aapt elected to compress.
    AAsset* a = AAssetManager_open(mgr_, path, AASSET_MODE_STREAMING);
    if (!a) return false;
    off_t sz = AAsset_getLength(a);
    out.resize(static_cast<size_t>(sz));
    int read = AAsset_read(a, out.data(), static_cast<size_t>(sz));
    AAsset_close(a);
    if (read != sz) { out.clear(); return false; }
    return true;
}

bool AssetIO::load_bytes(const char* path, std::vector<uint8_t>& out) const {
    if (!mgr_) return false;
    AAsset* a = AAssetManager_open(mgr_, path, AASSET_MODE_STREAMING);
    if (!a) return false;
    off_t sz = AAsset_getLength(a);
    out.resize(static_cast<size_t>(sz));
    int read = AAsset_read(a, out.data(), static_cast<size_t>(sz));
    AAsset_close(a);
    if (read != sz) { out.clear(); return false; }
    return true;
}

} // namespace eng
