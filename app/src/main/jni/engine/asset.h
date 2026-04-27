// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <android/asset_manager.h>
#include <string>
#include <vector>

namespace eng {

class AssetIO {
public:
    static AssetIO& instance();
    void attach(AAssetManager* mgr);
    bool load_text(const char* path, std::string& out) const;
    bool load_bytes(const char* path, std::vector<uint8_t>& out) const;
private:
    AAssetManager* mgr_{nullptr};
};

} // namespace eng
