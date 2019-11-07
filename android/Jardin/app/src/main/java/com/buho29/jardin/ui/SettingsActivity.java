package com.buho29.jardin.ui;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.InputFilter;
import android.text.Spanned;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.buho29.jardin.R;

import butterknife.BindView;

public class SettingsActivity extends AbsActivity {

    @BindView(R.id.save)
    Button mBtSave;

    @BindView(R.id.cancel)
    Button mBtCancel;


    @BindView(R.id.etIP) EditText mEtIp;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

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
        mEtIp.setFilters(filters);

        SharedPreferences prefs =
                getSharedPreferences("Settings", Context.MODE_PRIVATE);

        String ip = prefs.getString("ip", "192.168.8.100");
        mEtIp.setText(ip);

        mBtSave.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                //Guardamos las preferencias
                SharedPreferences prefs =
                        getSharedPreferences("Settings", Context.MODE_PRIVATE);

                SharedPreferences.Editor editor = prefs.edit();
                String ip = mEtIp.getText().toString();
                editor.putString("ip", ip);
                editor.commit();
                mModel.setServerIp(ip);
            }
        });


        mBtCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setResult(RESULT_CANCELED);
                finish();
            }
        });
    }

    @Override
    public int getLayoutId() {
        return R.layout.activity_settings;
    }
}
