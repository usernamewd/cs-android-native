// Copyright (c) 2026 cs-android-native contributors. MIT License.
#pragma once

#include <android/log.h>

// In release builds we silence everything except fatal errors. The TAG and
// format strings stay as compile-time literals so the obfuscation pass can
// substitute them when -mllvm -sobf is on; in non-OLLVM builds we still keep
// strings short and non-descriptive on purpose.
#ifndef CSN_LOG_TAG
#define CSN_LOG_TAG "x"
#endif

// Errors and warnings always emit so we can debug release builds.
// Debug/info logs are silenced in release to keep the log surface tiny.
#ifdef NDEBUG
#define LOGD(...) ((void)0)
#define LOGI(...) ((void)0)
#else
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, CSN_LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  CSN_LOG_TAG, __VA_ARGS__)
#endif

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  CSN_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, CSN_LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, CSN_LOG_TAG, __VA_ARGS__)
