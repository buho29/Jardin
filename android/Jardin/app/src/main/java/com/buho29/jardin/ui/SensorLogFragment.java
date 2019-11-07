package com.buho29.jardin.ui;

import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v4.content.ContextCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin.*;
import com.buho29.jardin.widget.MyMarkerView;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.utils.ColorTemplate;
import com.github.mikephil.charting.formatter.ValueFormatter;

import com.github.mikephil.charting.utils.Utils;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.TimeZone;

import butterknife.BindView;


public class SensorLogFragment extends AbsFragment {

    @BindView(R.id.chartTemp) LineChart mChartTemp;
    @BindView(R.id.chartPress) LineChart mChartPress;
    @BindView(R.id.chartHum) LineChart mChartHum;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View v = super.onCreateView(inflater, container, savedInstanceState);

        initChart(mChartTemp);
        initChart(mChartPress);
        initChart(mChartHum);

        return v;
    }

    @Override
    public void onStart() {
        super.onStart();
        model.addListener(EventType.SensorLog,this);
        model.callGetSensorLog();
    }

    @Override
    public int getLayoutId() {
        return R.layout.fragment_sensor_log;
    }

    @Override
    public void onChanged(AbsObservable.Event event) {

        List<Sensor> log = (List<Sensor>) event.getData();

        ArrayList<Entry> valueTemp = new ArrayList<>();
        ArrayList<Entry> valuePress = new ArrayList<>();
        ArrayList<Entry> valueHum = new ArrayList<>();

        for (int i = 0; i < log.size(); i++) {
            Sensor sensor = log.get(i);
            long time = sensor.getTime();
            if(i+1 < log.size()){
                if(log.get(i+1).getTime()>time){
                    valueTemp.add(new Entry(time,sensor.getTemp()));
                    valuePress.add(new Entry(time,sensor.getPress()));
                    valueHum.add(new Entry(time,sensor.getHum()));
                }
            }else {
                valueTemp.add(new Entry(time,sensor.getTemp()));
                valuePress.add(new Entry(time,sensor.getPress()));
                valueHum.add(new Entry(time,sensor.getHum()));
            }
        }

        setData(mChartTemp,valueTemp);
        setData(mChartPress,valuePress);
        setData(mChartHum,valueHum);
    }


    private boolean mFirstRun = true;
    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        /*if (isVisibleToUser && mFirstRun) {
            mFirstRun = false;

            mChartTemp.animateY(1000);
            mChartPress.animateX(1000);
            mChartHum.animateX(1000);
        }*/
    }

    private void initChart(LineChart chart) {
        // no description text
        chart.getDescription().setEnabled(false);

        // enable touch gestures
        chart.setDoubleTapToZoomEnabled(false);

        chart.setDrawGridBackground(false);

        chart.setViewPortOffsets(12f, 12f, 12f, 60f);

        // get the legend (only possible after setting data)
        chart.getLegend().setEnabled(false);

        XAxis xAxis = chart.getXAxis();
        xAxis.setPosition(XAxis.XAxisPosition.BOTTOM);
        xAxis.setTextSize(12f);
        xAxis.setDrawGridLines(true);
        //xAxis.setAxisMaximum(new Date().getTime());

        xAxis.setDrawAxisLine(true);
        xAxis.setSpaceMin(60f*1000f);
        xAxis.setGranularity(1f); // one hour
        xAxis.setValueFormatter(new ValueFormatter() {
            private final SimpleDateFormat mFormat = new SimpleDateFormat("HH:mm");
            @Override
            public String getFormattedValue(float value) {
                mFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
                return mFormat.format((long) value);
            }
        });

        YAxis leftAxis = chart.getAxisLeft();
        leftAxis.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);
        leftAxis.setDrawGridLines(true);
        leftAxis.setYOffset(-9f);
/*        leftAxis.setGranularityEnabled(true);
        leftAxis.setTextColor(Color.rgb(255, 192, 56));*/

        YAxis rightAxis = chart.getAxisRight();
        rightAxis.setEnabled(false);


        // create marker to display box when values are selected
        MyMarkerView mv = new MyMarkerView(getContext(), R.layout.custom_marker_view);

        // Set the marker to the chart
        mv.setChartView(chart);
        chart.setMarker(mv);
        chart.setVisibility(View.VISIBLE);
    }

    private void setData(final LineChart chart, ArrayList<Entry> values) {
        LineDataSet set1;

        if (chart.getData() != null &&
                chart.getData().getDataSetCount() > 0) {
            set1 = (LineDataSet) chart.getData().getDataSetByIndex(0);
            set1.setValues(values);
            set1.notifyDataSetChanged();
            chart.getData().notifyDataChanged();
            chart.notifyDataSetChanged();
        } else {
            // create a dataset and give it a type
            set1 = new LineDataSet(values, "");

            int color = ContextCompat.getColor(getContext(),R.color.colorLigth);

            set1.setAxisDependency(YAxis.AxisDependency.LEFT);
            set1.setColor(color);
            set1.setLineWidth(2f);
            set1.setDrawValues(false);
            set1.setFillAlpha(10);
            set1.setMode(LineDataSet.Mode.CUBIC_BEZIER);
            set1.setCircleColor(color);
            set1.setHighLightColor(color);
            set1.setDrawCircleHole(false);

            // set the filled area
            set1.setDrawFilled(true);

            // set color of filled area
            if (Utils.getSDKInt() >= 18) {
                // drawables only supported on api level 18 and above
                Drawable drawable = ContextCompat.getDrawable(getContext(), R.drawable.fade);
                set1.setFillDrawable(drawable);
            } else {
                set1.setFillColor(color);//
            }
            /**/

            // create a data object with the data sets
            LineData data = new LineData(set1);
            data.setValueTextColor(Color.RED);
            // data.setValueTextSize(9f);

            // set data
            chart.setData(data);
            chart.invalidate();
        }
    }
}
