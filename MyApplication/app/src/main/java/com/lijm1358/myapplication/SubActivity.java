package com.lijm1358.myapplication;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothSocket;
import android.os.Bundle;
import android.Manifest;
import android.os.Environment;
import android.os.SystemClock;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SubActivity extends MainActivity {
    ImageView mImageView;
    Button mSendBtn;
    String msg = "u";

    BluetoothAdapter btAdapter;
    private final static int REQUEST_ENABLE_BT = 1;

    @Override
    protected void onCreate(Bundle savedInsanceState) {
        super.onCreate(savedInsanceState);
        setContentView(R.layout.activity_sub);

        mImageView = (ImageView) findViewById(R.id.image);
        //mSendBtn = (Button) findViewById(R.id.sendingBtn);

        try {
            Bitmap bmp = BitmapFactory.decodeFile(Environment.getExternalStorageDirectory().getPath() + "/embedded_vault/intruder.jpg");
            mImageView.setImageBitmap(bmp);
        } catch(Exception e) {
            e.printStackTrace();
        }
        btAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    /*
    public void onClickButtonSend(View view) {
        mThreadConnectedBluetooth.write("c");
    }*/
}
