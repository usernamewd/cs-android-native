// Copyright (c) 2026 cs-android-native contributors. MIT License.
package com.devin.cs;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Single-activity host. All gameplay/rendering lives in C++ behind the JNI wall. */
public final class GameActivity extends Activity {

    private GLSurfaceView surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        surface = new GLSurfaceView(this);
        surface.setEGLContextClientVersion(3);
        surface.setEGLConfigChooser(8, 8, 8, 0, 24, 0);
        surface.setRenderer(new Renderer());
        surface.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        surface.setOnTouchListener(new TouchForwarder());
        setContentView(surface);

        applyImmersive();

        NativeBridge.onCreate(getAssets(), getFilesDir().getAbsolutePath(),
                getPackageCodePath());
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) applyImmersive();
    }

    @SuppressWarnings("deprecation")
    private void applyImmersive() {
        Window w = getWindow();
        if (Build.VERSION.SDK_INT >= 30) {
            // Android 11+: edge-to-edge, hide system bars, swipe-to-show.
            w.setDecorFitsSystemWindows(false);
            WindowInsetsController c = w.getInsetsController();
            if (c != null) {
                c.hide(WindowInsets.Type.systemBars());
                c.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            surface.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        surface.onResume();
        NativeBridge.onResume();
    }

    @Override
    protected void onPause() {
        NativeBridge.onPause();
        surface.onPause();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        NativeBridge.onDestroy();
        super.onDestroy();
    }

    private static final class Renderer implements GLSurfaceView.Renderer {
        @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            NativeBridge.onSurfaceCreated();
        }
        @Override public void onSurfaceChanged(GL10 gl, int width, int height) {
            NativeBridge.onSurfaceChanged(width, height);
        }
        @Override public void onDrawFrame(GL10 gl) {
            NativeBridge.onDrawFrame();
        }
    }

    private static final class TouchForwarder implements View.OnTouchListener {
        @Override public boolean onTouch(View v, MotionEvent e) {
            int n = e.getPointerCount();
            int action = e.getActionMasked();
            int idx = e.getActionIndex();
            int id = e.getPointerId(idx);
            float x = e.getX(idx);
            float y = e.getY(idx);
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    NativeBridge.onTouchEvent(0, id, x, y);
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_CANCEL:
                    NativeBridge.onTouchEvent(2, id, x, y);
                    break;
                case MotionEvent.ACTION_MOVE:
                    for (int i = 0; i < n; i++) {
                        NativeBridge.onTouchEvent(1, e.getPointerId(i), e.getX(i), e.getY(i));
                    }
                    break;
                default:
                    break;
            }
            return true;
        }
    }
}
