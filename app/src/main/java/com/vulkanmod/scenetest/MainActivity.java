package com.vulkanmod.scenetest;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

public class MainActivity extends Activity implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("vulkanmod_scene_test");
    }

    private SurfaceView surfaceView;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean isRendering = false;
    private long startTime;

    private native void initNative(Object surface);
    private native void renderNative(float timeSec);
    private native void destroyNative();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);
        startTime = System.currentTimeMillis();
    }

    private final Runnable renderRunnable = new Runnable() {
        @Override
        public void run() {
            if (isRendering) {
                float timeSec = (System.currentTimeMillis() - startTime) / 1000.0f;
                renderNative(timeSec);
                handler.postDelayed(this, 16);
            }
        }
    };

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        initNative(holder.getSurface());
        isRendering = true;
        handler.post(renderRunnable);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        isRendering = false;
        handler.removeCallbacks(renderRunnable);
        destroyNative();
    }
}
