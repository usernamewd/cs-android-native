// Copyright (c) 2026 cs-android-native contributors. MIT License.
#include "engine/asset.h"

namespace eng {

AssetIO& AssetIO::instance() { static AssetIO s; return s; }

void AssetIO::attach(AAssetManager* mgr) { mgr_ = mgr; }

bool AssetIO::load_text(const char* path, std::string& out) const {
    if (!mgr_) return false;
    AAsset* a = AAssetManager_open(mgr_, path, AASSET_MODE_BUFFER);
    if (!a) return false;
    off_t sz = AAsset_getLength(a);
    out.assign(static_cast<const char*>(AAsset_getBuffer(a)), static_cast<size_t>(sz));
    AAsset_close(a);
    return true;
}

bool AssetIO::load_bytes(const char* path, std::vector<uint8_t>& out) const {
    if (!mgr_) return false;
    AAsset* a = AAssetManager_open(mgr_, path, AASSET_MODE_BUFFER);
    if (!a) return false;
    off_t sz = AAsset_getLength(a);
    out.assign(static_cast<const uint8_t*>(AAsset_getBuffer(a)),
               static_cast<const uint8_t*>(AAsset_getBuffer(a)) + sz);
    AAsset_close(a);
    return true;
}

} // namespace eng
