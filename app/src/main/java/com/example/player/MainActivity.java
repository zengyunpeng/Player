package com.example.player;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    DNPlayer dnPlayer;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        dnPlayer = new DNPlayer();
        dnPlayer.setDataSource("rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov");
        dnPlayer.setSurfaceView(surfaceView);

        dnPlayer.setOnPreparedListener(new DNPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.i("tag", "可以开始播放了");
                        Toast.makeText(MainActivity.this, "可以开始播放了", Toast.LENGTH_LONG).show();
                    }
                });
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */

    public void start(View view) {
        dnPlayer.prepare();
    }
}
