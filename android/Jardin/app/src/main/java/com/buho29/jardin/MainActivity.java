package com.buho29.jardin;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import android.util.Log;

import java.util.ArrayList;

import com.buho29.jardin.model.ModelJardin;
import com.buho29.jardin.ui.AbsActivity;
import com.buho29.jardin.ui.ConfigFragment;
import com.buho29.jardin.ui.ModesFragment;
import com.buho29.jardin.ui.SensorLogFragment;
import com.buho29.jardin.ui.TapsFragment;
import com.buho29.jardin.ui.SensorFragment;
import com.buho29.jardin.ui.ZoneFragment;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

public class MainActivity extends AbsActivity
{

    private ViewPagerAdapter mSectionsPagerAdapter;

    private String TAG = MainActivity.class.getSimpleName();

    @BindView(R.id.container)
    ViewPager mViewPager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences prefs =
                getSharedPreferences("Settings", Context.MODE_PRIVATE);

        String ip = prefs.getString("ip", "192.168.8.100");

        mModel.setServerIp(ip);
        mModel.start();


        // Create the adapter that will return a fragment for each of the three
        // primary sections of the activity.
        mSectionsPagerAdapter = new ViewPagerAdapter(getSupportFragmentManager());

        mSectionsPagerAdapter.addFragment(new SensorFragment(), "FRAG1");
        mSectionsPagerAdapter.addFragment(new SensorLogFragment(),"FRAG2");
        mSectionsPagerAdapter.addFragment(new ZoneFragment(), "FRAG3");
        mSectionsPagerAdapter.addFragment(new ModesFragment(), "FRAG5");
        mSectionsPagerAdapter.addFragment(new TapsFragment(), "FRAG6");
        mSectionsPagerAdapter.addFragment(new ConfigFragment(), "FRAG7");

        mViewPager.setAdapter(mSectionsPagerAdapter);
    }

    @Override
    public int getLayoutId() {
        return R.layout.activity_main;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        ModelJardin.getInstance().stop();
    }

    // Adapter for the viewpager using FragmentPagerAdapter
    class ViewPagerAdapter extends FragmentPagerAdapter {
        private final List<Fragment> mFragmentList = new ArrayList<>();
        private final List<String> mFragmentTitleList = new ArrayList<>();

        public ViewPagerAdapter(FragmentManager manager) {
            super(manager);
        }

        @Override
        public Fragment getItem(int position) {
            return mFragmentList.get(position);
        }

        @Override
        public int getCount() {
            return mFragmentList.size();
        }

        public void addFragment(Fragment fragment, String title) {
            mFragmentList.add(fragment);
            mFragmentTitleList.add(title);
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return mFragmentTitleList.get(position);
        }
    }
}
