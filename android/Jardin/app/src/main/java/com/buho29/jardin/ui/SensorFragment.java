package com.buho29.jardin.ui;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.buho29.jardin.R;
import com.buho29.jardin.model.ModelJardin;
import com.buho29.jardin.model.ModelJardin.*;
import com.buho29.jardin.model.AbsObservable.*;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;
import java.util.Timer;
import java.util.TimerTask;


import butterknife.BindView;

public class SensorFragment extends AbsFragment{

    @BindView(R.id.temp) TextView mTemp;
    @BindView(R.id.press) TextView mPress;
    @BindView(R.id.hum) TextView mHum;

    private Timer timer;

    private String TAG = SensorFragment.class.getSimpleName();

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View v = super.onCreateView(inflater, container, savedInstanceState);

        final Handler handler = new Handler();
        timer = new Timer();

        timer.scheduleAtFixedRate(new TimerTask(){
            @Override
            public void run(){
                Log.i("tag", "A Kiss every 5 seconds");

                handler.post(new Runnable() {
                    public void run() {
                        try {
                            model.callGetSensor();
                        } catch (Exception e) {
                            // TODO Auto-generated catch block
                            Log.i("SensorFragment", e.getMessage());
                        }
                    }
                });
            }
        },100,5000);

        return v;
    }


    @Override
    public void onStart() {
        super.onStart();
        model.addListener(EventType.Sensor,this);
    }

    @Override
    public int getLayoutId() {
        return R.layout.fragment_sensor;
    }

    @Override
    public void onStop() {
        super.onStop();
        timer.cancel();
    }


    @Override
    public void onChanged(Event event) {
        Sensor s = (Sensor) event.getData();
        mTemp.setText(s.getTemp()+"°");
        mPress.setText((int)s.getPress()+"");
        mHum.setText(s.getHum()+"%");
    }
}