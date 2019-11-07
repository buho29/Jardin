package com.buho29.jardin.ui;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.DirectionalViewPager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Chronometer;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin.*;
import com.buho29.jardin.utils.FormatterDate24;

import java.util.List;

import butterknife.BindView;

public class ZoneFragment extends AbsFragment {

    @BindView(R.id.container) DirectionalViewPager mViewPager;

    @Override
    public int getLayoutId() { return R.layout.fragment_zone;}

    private int mCurrentPos = 0;
    private boolean mStoped = false;

    @Override
    public void onChanged(AbsObservable.Event event) {

        if(!mStoped)
            mCurrentPos = mViewPager.getCurrentItem();
        else mStoped = false;

        // llenamos con los nuevos datos
        mViewPager.setAdapter(new ZoneAdapter(getContext(),(List<Zone>) event.getData()));
        // seleccionamos la posicition actual
        mViewPager.setCurrentItem(mCurrentPos);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v =  super.onCreateView(inflater, container, savedInstanceState);

        ImageButton btAdd = v.findViewById(R.id.btAdd);
        btAdd.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(getContext(), EditZoneActivity.class);
                getContext().startActivity(i);
            }
        });

        return v;
    }

    @Override
    public void onStart() {
        super.onStart();
        model.addListener(EventType.Zone,this);
        model.callGetZones();
    }

    @Override
    public void onStop() {
        super.onStop();
        mCurrentPos = mViewPager.getCurrentItem();
        mStoped = true;
    }

    class ZoneAdapter extends PagerAdapter{

        private boolean mZoneRunning = false;
        private Context mContext;
        private List<Zone> mZones;

        public ZoneAdapter(Context context , List<Zone> zones) {
            mContext = context;
            mZones = zones;
        }

        @Override
        public Object instantiateItem(ViewGroup collection, final int position) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            ViewGroup layout = (ViewGroup) inflater.inflate(R.layout.zone_item, collection, false);

            Zone zone = mZones.get(position);

            TextView tvName = layout.findViewById(R.id.tvZone);
            tvName.setText(zone.getName());

            FormatterDate24 f = new FormatterDate24("%02d:%02d:%02d");
            Chronometer tvTime = layout.findViewById(R.id.tvTime);
            tvTime.setText(f.format(zone.getStart()));

            if(zone.isRunning() && !zone.isPaused()){
                //tvTime.setCountDown(true);
                tvTime.setBase(SystemClock.elapsedRealtime()-zone.getElapsed()*1000L);
                tvTime.start();
                mZoneRunning = true;
            }else if(zone.isPaused()){
                tvTime.setBase(SystemClock.elapsedRealtime()-zone.getElapsed()*1000L);
                tvTime.stop();
            }
            else tvTime.stop();


            TextView tvDuration = layout.findViewById(R.id.tvDuration);
            tvDuration.setText(zone.getDuration()/60+" min");


            final ToggleButton btPause = layout.findViewById(R.id.btPause);
            btPause.setChecked(zone.isPaused());
            btPause.setOnCheckedChangeListener( new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton toggleButton, boolean isChecked) {

                    Log.w("zone","click "+position);

                    Zone zone = model.getZone(position);
                    boolean running = zone.isRunning();
                    boolean pause = zone.isPaused();

                    if(running){
                        model.callZone(position,2);
                    }else {
                        btPause.setChecked(false);
                    }
                }
            });

            final ToggleButton btWater = layout.findViewById(R.id.btWater);
            btWater.setChecked(zone.isRunning());

            btWater.setOnCheckedChangeListener( new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton toggleButton, boolean isChecked) {

                    Zone zone = model.getZone(position);
                    boolean running = zone.isRunning();
                    boolean pause = zone.isPaused();

                    if(!mZoneRunning || running){
                        int cmd = 0;
                        if(!running) cmd = 1;
                        model.callZone(position,cmd);
                        mZoneRunning = true;
                    }else {
                        mZoneRunning = false;
                        btWater.setChecked(false);
                    }
                }
            });

            ImageButton btSetting = layout.findViewById(R.id.btSettings);
            btSetting.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent i = new Intent(getContext(), EditZoneActivity.class);
                    i.putExtra("zone", position);
                    getContext().startActivity(i);
                }
            });

            collection.addView(layout);
            return layout;
        }

        @Override
        public void destroyItem(ViewGroup collection, int position, Object view) {
            collection.removeView((View) view);
        }

        @Override
        public int getCount() {
            return mZones.size();
        }

        @Override
        public boolean isViewFromObject(View view, Object o) {
            return view == o;
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return mZones.get(position).getName();
        }
    }
}
