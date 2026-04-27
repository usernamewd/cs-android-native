// Copyright (c) 2026 cs-android-native contributors. MIT License.
package com.devin.cs;

import android.content.res.AssetManager;

/** All native entrypoints. The JNI symbol table is intentionally tiny; the rest
 * of the engine lives behind dlsym-resolved internal pointers in C++. */
public final class NativeBridge {
    static { System.loadLibrary("csnative"); }
    private NativeBridge() {}

    public static native void onCreate(AssetManager assets, String filesDir, String apkPath);
    public static native void onSurfaceCreated();
    public static native void onSurfaceChanged(int width, int height);
    public static native void onDrawFrame();
    public static native void onResume();
    public static native void onPause();
    public static native void onDestroy();

    /** action: 0=down, 1=move, 2=up. */
    public static native void onTouchEvent(int action, int pointerId, float x, float y);
}
