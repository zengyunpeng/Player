package com.example.player;

import android.media.MediaPlayer;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class DNPlayer implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("native-lib");
    }

    SurfaceHolder holder;
    private String datasource;
    private OnPrepareListener listener;


    /**
     * 让使用设置播放的文件或者直播地址
     */
    public void setDataSource(String str) {
        datasource = str;
    }


    public void start() {
        native_start();
    }

    public void stop() {

    }

    /**
     * 设置播放显示的画布
     *
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    public void prepare() {
        native_prepare(datasource);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i("tag", "surfaceCreated");

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i("tag", "surfaceChanged");
        native_setSurface(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i("tag", "surfaceDestroyed");
    }


    //适配加载的回调
    public void onError(int errorCode) {
        Log.i("tag", "Java接到回调 errorCode: " + errorCode);
    }

    /**
     * 给C代码层回调的方法
     */
    public void onPrepare() {
        if (listener != null) {
            listener.onPrepare();
        }
    }


    public void setOnPreparedListener(OnPrepareListener l) {
        this.listener = l;
    }

    public interface OnPrepareListener {
        void onPrepare();
    }


//    public interface CallBack {
//        void onError(int errorCode);
//    }


    native void native_prepare(String datasource);

    native void native_start();

    native void native_setSurface(Surface surface);
}
