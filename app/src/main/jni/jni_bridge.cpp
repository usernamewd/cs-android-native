// Copyright (c) 2026 cs-android-native contributors. MIT License.
//
// This is the only translation unit that exposes JNI symbols. Every other .so
// symbol is hidden by -fvisibility=hidden, so the externally visible surface of
// libcsnative.so is intentionally tiny:
//
//     Java_com_devin_cs_NativeBridge_*
//     JNI_OnLoad
//
// All gameplay/engine pointers stay inside C++ and are never handed back over
// JNI.
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>

#include "engine/app.h"
#include "engine/asset.h"
#include "engine/input.h"

#define EXPORT extern "C" __attribute__((visibility("default"))) JNIEXPORT

EXPORT jint JNI_OnLoad(JavaVM* /*vm*/, void* /*reserved*/) {
    return JNI_VERSION_1_6;
}

namespace {
inline std::string jstring_to_std(JNIEnv* env, jstring s) {
    if (!s) return {};
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out(c ? c : "");
    if (c) env->ReleaseStringUTFChars(s, c);
    return out;
}
} // namespace

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onCreate(JNIEnv* env, jclass /*clz*/,
                                        jobject asset_manager,
                                        jstring files_dir,
                                        jstring apk_path) {
    AAssetManager* mgr = AAssetManager_fromJava(env, asset_manager);
    eng::AssetIO::instance().attach(mgr);
    eng::App::instance().on_create(jstring_to_std(env, files_dir),
                                   jstring_to_std(env, apk_path));
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onSurfaceCreated(JNIEnv*, jclass) {
    eng::App::instance().on_surface_created();
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onSurfaceChanged(JNIEnv*, jclass,
                                                jint w, jint h) {
    eng::App::instance().on_surface_changed(w, h);
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onDrawFrame(JNIEnv*, jclass) {
    eng::App::instance().on_draw_frame();
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onResume(JNIEnv*, jclass) {
    eng::App::instance().on_resume();
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onPause(JNIEnv*, jclass) {
    eng::App::instance().on_pause();
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onDestroy(JNIEnv*, jclass) {
    eng::App::instance().on_destroy();
}

EXPORT void JNICALL
Java_com_devin_cs_NativeBridge_onTouchEvent(JNIEnv*, jclass,
                                            jint action, jint pid,
                                            jfloat x, jfloat y) {
    using A = eng::TouchInput::Action;
    A a;
    switch (action) {
        case 0: a = A::Down; break;
        case 1: a = A::Move; break;
        default: a = A::Up;  break;
    }
    eng::App::instance().input().on_touch(a, pid, x, y);
}
