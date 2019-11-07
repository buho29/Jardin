package com.buho29.jardin.ui;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.InputFilter;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin;
import com.buho29.jardin.model.ModelJardin.*;

import butterknife.BindView;

public class ConfigFragment extends AbsFragment {

    @BindView(R.id.tvRSSI) TextView mTvRSSI;
    @BindView(R.id.tvDistanceRSSI) TextView mTvDistanceRSSI;

    @BindView(R.id.ivRSSI) ImageView mIvRSSI;

    @BindView(R.id.btRefresh) ImageButton mBtRefresh;
    @BindView(R.id.btSaveIp) ImageButton mBtSaveIp;
    @BindView(R.id.btSaveWeather) ImageButton mBtSaveWeather;
    @BindView(R.id.btSaveWifi) ImageButton mBtSaveWifi;


    @BindView(R.id.etCityID) EditText mEtCityID;
    @BindView(R.id.etCityName) EditText mEtCityName;
    @BindView(R.id.etIP) EditText mEtIP;
    @BindView(R.id.etSSID) EditText mEtSSID;
    @BindView(R.id.etPass) EditText mEtPass;

    @BindView(R.id.btResetData) Button mBtResetData;
    @BindView(R.id.btResetDataSensor) Button mBtResetDataSensor;
    @BindView(R.id.btReBoot) Button mBtReBoot;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);

        mBtRefresh.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                model.callConfigLoadData();
            }
        });

        InputFilter[] filters = new InputFilter[1];
        filters[0] = new InputFilter() {
            @Override
            public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
                if (end > start) {
                    String destTxt = dest.toString();
                    String resultingTxt = destTxt.substring(0, dstart) + source.subSequence(start, end) + destTxt.substring(dend);
                    if (!resultingTxt.matches ("^\\d{1,3}(\\.(\\d{1,3}(\\.(\\d{1,3}(\\.(\\d{1,3})?)?)?)?)?)?")) {
                        return "";
                    } else {
                        String[] splits = resultingTxt.split("\\.");
                        for (int i=0; i<splits.length; i++) {
                            if (Integer.valueOf(splits[i]) > 255) {
                                return "";
                            }
                        }
                    }
                }
                return null;
            }
        };
        mEtIP.setFilters(filters);

        SharedPreferences prefs =
                getContext().getSharedPreferences("Settings", Context.MODE_PRIVATE);

        String ip = prefs.getString("ip", "192.168.8.100");
        mEtIP.setText(ip);

        mBtSaveIp.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //Guardamos las preferencias
                SharedPreferences prefs =
                        getContext().getSharedPreferences("Settings", Context.MODE_PRIVATE);

                SharedPreferences.Editor editor = prefs.edit();
                String ip = mEtIP.getText().toString();
                editor.putString("ip", ip);
                editor.commit();
                model.setServerIp(ip);
            }
        });

        mBtSaveWeather.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mEtCityID.getText().length()>0 && mEtCityName.getText().length()>0){
                    model.callConfigWeather(
                            mEtCityID.getText().toString(),
                            mEtCityName.getText().toString()
                    );
                }
            }
        });

        mBtSaveWifi.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mEtSSID.getText().length()>0 && mEtPass.getText().length()>0){
                    model.callConfigWiFi(
                            mEtSSID.getText().toString(),
                            mEtPass.getText().toString()
                    );
                }
            }
        });

        mBtResetData.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                model.callConfigResetData();
            }
        });

        mBtReBoot.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                model.callConfigReBoot();
            }
        });

        return view;
    }
    @Override
    public int getLayoutId() {
        return R.layout.fragment_config;
    }

    @Override
    public void onStart() {
        super.onStart();
        model.addListener(EventType.Config,this);
        model.callConfigLoadData();
    }

    @Override
    public void onChanged(AbsObservable.Event event) {
        Config config = (Config) event.getData();

        mTvRSSI.setText(config.getRssi()+"db");
        mTvDistanceRSSI.setText(config.getDistanceRSSI()+"m");

        switch (ModelJardin.calculateSignalLevel(config.getRssi(),5)){
            case 1:
                mIvRSSI.setImageResource(R.drawable.wifi_1);
                break;
            case 2:
                mIvRSSI.setImageResource(R.drawable.wifi_2);
                break;
            case 3:
                mIvRSSI.setImageResource(R.drawable.wifi_3);
                break;
            case 4:
                mIvRSSI.setImageResource(R.drawable.wifi_4);
                break;
            default:
                mIvRSSI.setImageResource(R.drawable.wifi_0);
                break;
        }

        mEtCityID.setText(config.getCityID());
        mEtCityName.setText(config.getCityName());

        mEtSSID.setText(config.getSsid());

    }
}
