package com.lijm1358.myapplication;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;

import android.Manifest;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.Settings;
import android.text.Html;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    private static final int PERMISSION_REQUEST_CODE = 1;
    TextView mTvBluetoothStatus;
    TextView mTvReceiveData;
    TextView mTvSendData;
    TextView mTvWriteMessages;
    TextView mTvStatusOut;
    Button mBtnConnect;
    Button mBtnUnlock;

    BluetoothAdapter mBluetoothAdapter;
    Set<BluetoothDevice> mPairedDevices;
    List<String> mListPairedDevices;

    Handler mBluetoothHandler;
    ConnectedBluetoothThread mThreadConnectedBluetooth;
    BluetoothDevice mBluetoothDevice;
    BluetoothSocket mBluetoothSocket;

    Intent myIntent;

    public MainActivity activity;

    public BluetoothSocket mmSocket;
    public InputStream mmInStream;
    public OutputStream mmOutStream;

    Intent intent;

    String imagestr = "";
    boolean imageMode = false;
    byte[] bytes = new byte[16384];
    int byteIndex = 0;
    final static int BT_REQUEST_ENABLE = 1;
    final static int BT_MESSAGE_READ = 2;
    final static int BT_CONNECTING_STATUS = 3;
    final static UUID BT_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        intent = new Intent(getApplicationContext(), SubActivity.class);

        mTvBluetoothStatus = (TextView) findViewById(R.id.tvBluetoothStatus);
        mTvReceiveData = (TextView) findViewById(R.id.tvReceiveData);
        mBtnConnect = (Button) findViewById(R.id.btnConnect);
        mBtnUnlock = (Button) findViewById(R.id.btnUnlock);
        mTvWriteMessages = (TextView) findViewById(R.id.writeMessages);
        mTvStatusOut = (TextView) findViewById(R.id.statusOut);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (Build.VERSION.SDK_INT >= 30){
            if (!Environment.isExternalStorageManager()){
                Intent getpermission = new Intent();
                getpermission.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                startActivity(getpermission);
            }
        }

        mBtnConnect.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                listPairedDevices();
            }
        });

        mBtnUnlock.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mThreadConnectedBluetooth.write("c");
            }
        });

        mBluetoothHandler = new Handler() {
            public void handleMessage(android.os.Message msg){
                if(msg.what == BT_MESSAGE_READ){
                    String readMessage = null;
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    //readMessage = new String((byte[]) msg.obj, "UTF-8");
                    try {
                        outputStream.write((byte[]) msg.obj);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }


                    Log.d("byteoutput", "size : "+ String.valueOf(outputStream.size()));
                    Log.d("byteoutput", new String(outputStream.toByteArray()));
                    outputStream.reset();
                    //mTvReceiveData.setText(readMessage);
                }
            }

        };
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case BT_REQUEST_ENABLE:
                if (resultCode == RESULT_OK) { // 블루투스 활성화를 확인을 클릭하였다면
                    Toast.makeText(getApplicationContext(), "블루투스 활성화", Toast.LENGTH_LONG).show();
                    mTvBluetoothStatus.setText("활성화");
                } else if (resultCode == RESULT_CANCELED) { // 블루투스 활성화를 취소를 클릭하였다면
                    Toast.makeText(getApplicationContext(), "취소", Toast.LENGTH_LONG).show();
                    mTvBluetoothStatus.setText("비활성화");
                }
                break;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    void listPairedDevices() {
        if (mBluetoothAdapter.isEnabled()) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                Log.d("bluetooth_stat", "listpair2");
                List<String> permissionList = new ArrayList<>();
                permissionList.add(Manifest.permission.BLUETOOTH_CONNECT);

                String[] requestPermissions = permissionList.toArray(new String[permissionList.size()]);

                // You can directly ask for the permission.
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    ActivityCompat.requestPermissions(this, requestPermissions, PERMISSION_REQUEST_CODE);
                }
                //return;
            }
            mPairedDevices = mBluetoothAdapter.getBondedDevices();

            if (mPairedDevices.size() > 0) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("장치 선택");

                mListPairedDevices = new ArrayList<String>();
                for (BluetoothDevice device : mPairedDevices) {
                    mListPairedDevices.add(device.getName());
                    //mListPairedDevices.add(device.getName() + "\n" + device.getAddress());
                }
                final CharSequence[] items = mListPairedDevices.toArray(new CharSequence[mListPairedDevices.size()]);
                mListPairedDevices.toArray(new CharSequence[mListPairedDevices.size()]);

                builder.setItems(items, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int item) {
                        connectSelectedDevice(items[item].toString());
                    }
                });
                AlertDialog alert = builder.create();
                alert.show();
            } else {
                Log.d("bluetooth_stat", "페어링된 장치가 없습니다.");
                Toast.makeText(getApplicationContext(), "페어링된 장치가 없습니다.", Toast.LENGTH_LONG).show();
            }
        } else {
            Log.d("bluetooth_stat", "블루투스가 비활성화 되어 있습니다.");
            Toast.makeText(getApplicationContext(), "블루투스가 비활성화 되어 있습니다.", Toast.LENGTH_SHORT).show();
        }
    }

    void connectSelectedDevice(String selectedDeviceName) {
        for (BluetoothDevice tempDevice : mPairedDevices) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                //return;
            }
            if (selectedDeviceName.equals(tempDevice.getName())) {
                mBluetoothDevice = tempDevice;
                break;
            }
        }
        try {
            mBluetoothSocket = mBluetoothDevice.createRfcommSocketToServiceRecord(BT_UUID);
            mBluetoothSocket.connect();
            mThreadConnectedBluetooth = new ConnectedBluetoothThread(mBluetoothSocket);
            mThreadConnectedBluetooth.start();
            mTvStatusOut.setText("Connected");
            mThreadConnectedBluetooth.write("test");
            //mBluetoothHandler.obtainMessage(BT_CONNECTING_STATUS, 1, -1).sendToTarget();
        } catch (IOException e) {
            Toast.makeText(getApplicationContext(), "블루투스 연결 중 오류가 발생했습니다.", Toast.LENGTH_LONG).show();
            mTvStatusOut.setText("Disconnected");
        }
    }
    protected class ConnectedBluetoothThread extends Thread {
        private DataInputStream mmDataInStream;
        private BufferedOutputStream mmBufferedOutStream;


        public ConnectedBluetoothThread(BluetoothSocket socket) {
            mmSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) {
                Toast.makeText(getApplicationContext(), "소켓 연결 중 오류가 발생했습니다.", Toast.LENGTH_LONG).show();
            }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }
        public void run() {
            byte[] buffer = new byte[1024];

            int isavailable;

            while (true) {
                try {
                    isavailable = mmInStream.available();
                    if (isavailable != 0) {
                        //SystemClock.sleep(10);
                        mmInStream.read(buffer);
                        //Log.d("byteoutputinteger", "inputlen? " + String.valueOf(isavailable));
                        //Log.d("byteoutputinteger", new String(buffer));
                        for(int i=0;i<isavailable;i++) {
                            bytes[byteIndex++] = buffer[i];
                        }
                        /*
                        for(int i=0;i<byteIndex;i++) {
                            Log.d("byteoutput", Integer.toHexString(bytes[i]));
                        }*/
                        //mBluetoothHandler.obtainMessage(BT_MESSAGE_READ, bytes, -1, buffer).sendToTarget();
                        buffer = new byte[1024];
                    }
                } catch (IOException e) {
                    break;
                }

                if(byteIndex > 1 && bytes[byteIndex-1] == 's' && bytes[byteIndex-2] == 'e') {
                    String readString = new String(bytes);
                    Log.d("byteoutputresult", readString);
                    Date currentTime = Calendar.getInstance().getTime();

                    if(readString.substring(0, 4).equals("open")) {
                        mTvWriteMessages.append(Html.fromHtml("<p><font color=#00FF00> open</font> " + currentTime + "</p"));
                    }
                    else if(readString.substring(0, 5).equals("close")) {
                        mTvWriteMessages.append(Html.fromHtml("<p><font color=#FF8040> close</font> " + currentTime + "</p>"));
                    }
                    else if(readString.substring(0, 8).equals("lockdown")) {
                        mTvWriteMessages.append(Html.fromHtml("<p><strong><font color=#FF0000> lockdown</font></strong> " + currentTime + "</p>"));
                        imageMode = true;
                    }
                    else if(imageMode) {
                        try {
                            String path = Environment.getExternalStorageDirectory().getPath() + "/embedded_vault/intruder.jpg";
                            File file = new File(path);
                            if (!file.exists()) {
                                file.createNewFile();
                            }
                            FileOutputStream stream = new FileOutputStream(path);
                            if(bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF && bytes[3] == 0xE0)
                                Log.d("imagestat", "normal");
                            else if(bytes[1] == 0xFF && bytes[2] == 0xD8 && bytes[3] == 0xFF && bytes[4] == 0xE0) {
                                Log.d("imagestat", "abnormal1");
                                bytes = Arrays.copyOfRange(bytes, 1, 8191);
                            } else if(bytes[0] != 0xFF) {
                                Log.d("imagestat", "abnormal2");
                                bytes[0] = (byte) 0xFF;
                            }
                            stream.write(bytes);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        imageMode = false;
                        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
                        NotificationCompat.Builder builder = null;

                        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                            String channelID = "channel_01";
                            String channelName = "intruderAlarmChannel";

                            NotificationChannel channel = new NotificationChannel(channelID, channelName, NotificationManager.IMPORTANCE_DEFAULT);
                            notificationManager.createNotificationChannel(channel);

                            builder = new NotificationCompat.Builder(getApplicationContext(), channelID);
                        } else{
                            builder = new NotificationCompat.Builder(getApplicationContext(), (String) null);
                        }

                        builder.setSmallIcon(android.R.drawable.ic_lock_lock);
                        builder.setContentTitle("금고");
                        builder.setContentText("금고 침입이 감지되었습니다.");
                        Notification notification = builder.build();

                        notificationManager.notify(1, notification);
                        startActivity(intent);
                    }
                    byteIndex = 0;
                    bytes = new byte[8192];
                }
            }
        }
        public void write(String str) {
            byte[] bytes = str.getBytes();
            try {
                mmOutStream.write(bytes);
                Log.d("byteoutput", "send");
            } catch (IOException e) {
                Toast.makeText(getApplicationContext(), "데이터 전송 중 오류가 발생했습니다.", Toast.LENGTH_LONG).show();
            }
        }
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) {
                Toast.makeText(getApplicationContext(), "소켓 해제 중 오류가 발생했습니다.", Toast.LENGTH_LONG).show();
            }
        }
    }
}