package vip.ruoyun.ffmpeg;

import android.content.Context;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class XPlay extends SurfaceView implements SurfaceHolder.Callback {

    public static final String TAG = XPlay.class.getSimpleName();

    static {
        System.loadLibrary("native-lib");
    }

    public XPlay(Context context) {
        super(context);
        init();
    }

    public XPlay(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        new Thread(new XPlayRunnable()).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    private class XPlayRunnable implements Runnable {

        @Override
        public void run() {
            Log.e(TAG, Environment.getExternalStorageDirectory().getPath());
            open("/sdcard/1080.mp4", getHolder().getSurface());
        }
    }

    public native void open(String url, Surface surface);
}
