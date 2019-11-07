package com.buho29.jardin.ui;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.NumberPicker;
import android.widget.TimePicker;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin;
import com.buho29.jardin.model.ModelJardin.*;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

public class EditAlarm extends AbsActivity {

    @BindView(R.id.hoursPicker)
    NumberPicker mHoursPicker;

    @BindView(R.id.minutesPicker)
    NumberPicker mMinutesPicker;

    @BindView(R.id.durationPicker)
    NumberPicker mDurationPicker;

    @BindView(R.id.tapPicker)
    NumberPicker mTapsPicker;

    @BindView(R.id.save)
    Button mBtSave;

    @BindView(R.id.cancel)
    Button mBtCancel;

    private int mZoneIndex = -1;
    private int mAlarmIndex = -1;

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt("zone", mZoneIndex);
        outState.putInt("alarm", mAlarmIndex);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Bundle extras = getIntent().getExtras();
        if(savedInstanceState != null){
            mZoneIndex = savedInstanceState.getInt("zone");
            mAlarmIndex = savedInstanceState.getInt("alarm");
        } else if(extras != null) {
            mZoneIndex = extras.getInt("zone");
            mAlarmIndex = extras.getInt("alarm");
        }

        if(mZoneIndex <0){
            Log.w("EdiAlarm","no existe ZoneIndex");
            setResult(RESULT_CANCELED);
            finish();
        }

        mHoursPicker.setMinValue(0);
        mHoursPicker.setMaxValue(23);

        mMinutesPicker.setMinValue(0);
        mMinutesPicker.setMaxValue(59);

        mDurationPicker.setMinValue(1);
        mDurationPicker.setMaxValue(120);

        List<Tap> taps = mModel.getTaps();
        String[] arrayPicker = new String[taps.size()];
        for (int i = 0; i < taps.size(); i++) {
            Tap tap = taps.get(i);
            arrayPicker[i] = tap.getPin()+"";
        }

        //set min value zero
        mTapsPicker.setMinValue(0);
        //set max value from length array string reduced 1
        mTapsPicker.setMaxValue(arrayPicker.length - 1);
        //implement array string to number picker
        mTapsPicker.setDisplayedValues(arrayPicker);
        //disable soft keyboard
        mTapsPicker.setDescendantFocusability(NumberPicker.FOCUS_BLOCK_DESCENDANTS);
        //set wrap true or false, try it you will know the difference
        mTapsPicker.setWrapSelectorWheel(false);

        //bind
        Alarm alarm = null;
        boolean newAlarm = false;
        if(mAlarmIndex>-1){//edit
            alarm = mModel.getAlarm(mZoneIndex,mAlarmIndex);
        }else{//new
            alarm = mModel.getLastAlarm(mZoneIndex);
            newAlarm = true;
        }

        if(alarm != null){
            fillAlarm(alarm,newAlarm);
        }

        mBtCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setResult(RESULT_CANCELED);
                finish();
            }
        });

        mBtSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int h = mHoursPicker.getValue()*60*60;
                int m = mMinutesPicker.getValue()*60;
                int d = mDurationPicker.getValue()*60;
                int init = h + m;

                int pinIndex = mTapsPicker.getValue();
                Tap tap = mModel.getTap(pinIndex);

                Alarm alarm = new Alarm(tap.getPin(),init,d);
                mModel.callSaveAlarm(mZoneIndex,alarm,mAlarmIndex);
                setResult(RESULT_OK);
                finish();
            }
        });
    }

    @Override
    public int getLayoutId() {
        return R.layout.activity_edit_alarm;
    }

    private void fillAlarm(Alarm alarm,boolean newAlarm) {
        int time = alarm.getTime();
        int h, m;

        if(newAlarm){
            time += alarm.getDuration();
            mHoursPicker.setEnabled(false);
            mMinutesPicker.setEnabled(false);
        }

        time /= 60;
        m = time % 60;
        time /= 60;
        h = time % 24;

        mHoursPicker.setValue(h);
        mMinutesPicker.setValue(m);
        mDurationPicker.setValue(alarm.getDuration()/60);

        int index = mModel.getIndexTap(alarm.getPin());

        mTapsPicker.setValue(index);
    }

}
