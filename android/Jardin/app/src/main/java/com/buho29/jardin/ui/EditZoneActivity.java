package com.buho29.jardin.ui;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin;
import com.buho29.jardin.model.ModelJardin.*;
import com.buho29.jardin.utils.Builder;
import com.buho29.jardin.utils.FormatterDate24;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

public class EditZoneActivity extends AbsActivity implements AbsObservable.OnChangedListener {

    private int mZoneIndex = -1;
    private Zone mZone;

    @BindView(R.id.tvTitle) TextView mTitle;
    @BindView(R.id.btRemoveZone) ImageButton mBtRemoveZone;
    @BindView(R.id.etName) EditText mEtname;
    @BindView(R.id.btAddAlarm)ImageButton mBtAddAlarm;
    @BindView(R.id.btEditAlarm)ImageButton mBtEditAlarm;
    @BindView(R.id.btRemoveAlarm) ImageButton mBtRemoveAlarm;
    @BindView(R.id.listAlarms) ListView mListAlarms;
    @BindView(R.id.btSave) ImageButton mbtSave;
    @BindView(R.id.btCancel) Button mBtCancel;

    private boolean saved = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mModel.addListener(EventType.All,this);

        Bundle extras = getIntent().getExtras();
        if(savedInstanceState != null){
            mZoneIndex = savedInstanceState.getInt("zone");
        } else if(extras != null) {
            mZoneIndex = extras.getInt("zone");
        }

        if(mZoneIndex > -1){
            mTitle.setText(getText(R.string.edit_zone));
            mZone = mModel.getZone(mZoneIndex);

            mEtname.setText(mZone.getName());

            mModel.callGetAlarms(mZoneIndex);
        }

        mBtAddAlarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mZone != null){
                    int index = mListAlarms.getSelectedItemPosition();
                    Intent i = new Intent(EditZoneActivity.this,EditAlarm.class);
                    i.putExtra("zone",mZoneIndex);
                    i.putExtra("alarm",index);
                    startActivityForResult(i,0);
                }else {
                    Builder.makeDialog(EditZoneActivity.this,R.string.error_no_saved);
                }
            }
        });


        mBtEditAlarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mZone != null){
                    int index = mListAlarms.getCheckedItemPosition();
                    if(index < 0){
                        Builder.makeDialog(EditZoneActivity.this,R.string.error_no_selected);
                    }else{
                        Intent i = new Intent(EditZoneActivity.this,EditAlarm.class);
                        i.putExtra("zone",mZoneIndex);
                        i.putExtra("alarm",index);
                        startActivityForResult(i,0);
                    }
                }else {
                    Builder.makeDialog(EditZoneActivity.this,R.string.error_no_saved);
                }
            }
        });

        mBtRemoveAlarm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mZone != null){
                    int index = mListAlarms.getCheckedItemPosition();
                    if(index < 0)
                        Builder.makeDialog(EditZoneActivity.this,R.string.error_no_selected);
                    else
                        mModel.callRemoveAlarm(mZoneIndex,index);
                }else
                    Builder.makeDialog(EditZoneActivity.this,R.string.error_no_saved);
            }
        });

        mBtCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setResult(RESULT_CANCELED);
                finish();
            }
        });

        mbtSave.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v) {
                String name = mEtname.getText().toString();

                // esta vacio
                if(name.isEmpty()){
                    Builder.makeDialog(EditZoneActivity.this,R.string.error_name_empty);
                }// el nombre ya existe
                else if (mModel.getZoneIndex(name) > -1) {
                    Builder.makeDialog(EditZoneActivity.this,R.string.error_name_exist);
                }else if((mZone != null && !name.equals(mZone.getName())) // edita y es diferente
                        || mZone == null){//nuevo
                    mModel.callSaveZone(mZoneIndex,name);
                }
            }
        });

        mBtRemoveZone.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(mZone != null){
                    mModel.callRemoveZone(mZoneIndex);
                    setResult(RESULT_OK);
                    finish();
                }
            }
        });
    }

    @Override
    public int getLayoutId() {
        return R.layout.activity_edit_zone;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == RESULT_CANCELED){

        }else if(data != null){
            //TODO
            //refrescamos el dataset?
        }else{
            Log.w("editZoneResultAlarm","no deberia pasar");
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt("zone", mZoneIndex);
    }

    @Override
    public void onChanged(AbsObservable.Event event) {

        if(event.getType() == EventType.Alarm){
            List<Alarm> alarms = (List<Alarm>) event.getData();

            //mListAlarms.getCheckedItemPosition();
            mListAlarms.setAdapter(new AlarmAdapter(EditZoneActivity.this,
                    alarms));
        }else if(event.getType() == EventType.Zone){
            if(mZoneIndex > -1){
                mZone = mModel.getZone(mZoneIndex);
                mEtname.setText(mZone.getName());
            }else{
                mZoneIndex = mModel.getZoneIndex(mEtname.getText().toString());
                mZone = mModel.getZone(mZoneIndex);
            }
        }
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        mModel.removeListener(this);
    }

    public class AlarmAdapter extends ArrayAdapter<Alarm> {

        public AlarmAdapter(Context context, List<Alarm> alarms) {
            super(context, 0, alarms);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {

            // Check if an existing view is being reused, otherwise inflate the view
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(R.layout.alarm_item, parent, false);
            }

            // Get the data item for this position
            Alarm alarm = (Alarm) getItem(position);

            TextView tv = (TextView) convertView.findViewById(R.id.tv);

            FormatterDate24 f = new FormatterDate24("%02d:%02d:%02d");
            tv.setText(f.format(alarm.getTime())+" - "+(int)(alarm.getDuration()/60F)+" min - "+alarm.getPin());

            // Return the completed view to render on screen
            return convertView;
        }
    }

}