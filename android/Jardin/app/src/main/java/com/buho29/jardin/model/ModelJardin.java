package com.buho29.jardin.model;

import android.os.AsyncTask;
import android.os.Handler;
import android.util.Log;

import com.buho29.jardin.net.HttpHandler;
import com.buho29.jardin.utils.EnQueue;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class ModelJardin extends AbsObservable {

    private String localHost = "192.168.8.100";
    private static final String http  = "http://";

    //singleton
    private static ModelJardin instance;
    public static ModelJardin getInstance(){
        if(instance == null){
            instance = new ModelJardin();
            Log.d("ModelJardin","new instance");
        }
        return instance;
    }

    public void setServerIp(String ip) {
        localHost = ip;
        loadData();
    }

    public static class EventType {
        public static final int All = -1;
        public static final int Error = 0;
        public static final int Sensor = 1;
        public static final int Taps = 2;
        public static final int Zone = 3;
        public static final int SensorLog = 4;
        public static final int Alarm = 5;
        public static final int Config = 6;
        public static final int Modes = 7;
    };

    //data object
    public static class Sensor {

        private float temp;
        private float press;
        private int hum;
        private long time;

        public Sensor(float temp, float press, int hum, int time) {
            this.temp = temp;
            this.press = press;
            this.hum = hum;
            this.time = time * 1000L ;
        }

        public float getTemp() { return temp;}
        public float getPress() { return press;}
        public int getHum() { return hum;}
        public long getTime() {return time;}

    }
    public static class Tap {

        private int pin;
        private boolean open;

        public Tap(int pin, int open) {
            this.pin = pin;
            this.open =  open != 0;
        }

        public int getPin() {return pin;}
        public void setPin(int pin) {this.pin = pin;}

        public boolean getOpen() {return open;}
        public void setOpen(boolean open) {this.open = open;}

    }
    public static class Alarm {

        private int pin;
        private int time;
        private int duration;

        public Alarm(int pin, int time, int duration) {
            this.pin = pin;
            this.time = time;
            this.duration = duration;
        }

        public int getPin() {return pin;}
        public void setPin(int pin) {this.pin = pin;}

        public int getTime() {return time;}
        public void setTime(int time) {this.time = time;}

        public int getDuration() {return duration;}
        public void setDuration(int duration) {this.duration = duration;}

    }
    public static class Zone {

        private int id;
        private String name;
        private boolean running;
        private boolean paused;

        private int duration;
        private int start;
        private int elapsed;
        private List<Alarm> alarms = new ArrayList<>();

        public Zone(int id, String name, boolean running,
                int duration , int start, int elapsed,
                boolean paused)
        {
            this.id = id;
            this.name = name;
            this.running = running;
            this.duration = duration;
            this.start = start;
            this.elapsed = elapsed;
            this.paused = paused;
        }

        public int getDuration() {return duration;}
        public int getStart() {return start;}
        public int getId() {return id;}
        public boolean isRunning(){
            return running;
        }

        public String getName() {return name;}
        public void setName(String name) {this.name = name;}

        public int getElapsed() {
            return elapsed;
        }

        public boolean isPaused() {
            return paused;
        }
    }
    public static class Status{
        public final static int Standby = 0;
        public final static int Manual = 1;
        public final static int Zone = 2;

        public int tapsInt;
        public int mode=-1;
        public int timeZoneChanged =-1;
        public int timeModesChanged =-1;
        public Status(String data) {
            data = data.replaceAll("\\n","");
            String[] splitStr = data.split("\\|");

            tapsInt = Integer.parseInt(splitStr[0]);
            timeZoneChanged = Integer.parseInt(splitStr[1]);
            timeModesChanged = Integer.parseInt(splitStr[2]);
        }
    }
    public static class Config{

        private int rssi;
        private String distanceRSSI;
        private String ssid;
        private String password;
        private String cityID;
        private String cityName;

        public Config(int rssi, String distanceRSSI,
                      String ssid,
                      String cityID, String cityName)
        {
            this.rssi = rssi; this.distanceRSSI = distanceRSSI;
            this.ssid = ssid;
            this.cityID = cityID; this.cityName = cityName;
        }

        public int getRssi() {return rssi;}
        public String getDistanceRSSI() { return distanceRSSI;}
        public String getSsid() {return ssid;}
        public String getCityID() {return cityID;}
        public String getCityName() {return cityName;}
    }
    public static class Mode {

        private String name;
        private boolean enabled;

        public Mode(String name, boolean enabled) {
            this.name = name;
            this.enabled = enabled;
        }

        public String getName() {
            return name;
        }

        public boolean isEnabled() {
            return enabled;
        }

    }

    private abstract class AbsTask extends AsyncTask<Void, Void, Void>{
        protected Boolean mSuccess = false;
        protected  String msg = "Couldn't get json from server.";

        protected abstract Void doInBackground(Void... params);
        protected abstract AbsTask clone();

        /**
         * Runs on the UI thread before {@link #doInBackground}.
         *
         * @see #onPostExecute
         * @see #doInBackground
         */
        @Override
        protected void onPreExecute() {
            super.onPreExecute();
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);

            if(!mSuccess){
                AbsTask clon = clone();
                if(clon != null)
                    enqueueCall(clon);
            }

            mCallsRunning = false;
            nextCall();
        }



        @Override
        protected void onCancelled(Void aVoid) {
            super.onCancelled(aVoid);

            mCallsRunning = false;
            nextCall();
        }
    }

    //http://192.168.8.100/sensor
    private class SensorTask extends AbsTask {

        /*
        {
          "temp": 20.75,
          "press": 963.4515,
          "hum": 55,
          "time": 1541847917
        }
        */

        @Override
        protected Void doInBackground(Void... arg0) {
            HttpHandler sh = new HttpHandler();
            // Making a request to url and getting response
            String url = http+localHost+"/sensor";
            String jsonStr = sh.makeServiceCall(url);

            if (jsonStr != null) {
                //Log.e("SensorTask", jsonStr);
                try {
                    JSONObject jsonObj = new JSONObject(jsonStr);

                    float temp = Float.valueOf(jsonObj.getString("temp"));
                    float  press = Float.valueOf(jsonObj.getString("press"));
                    int hum = jsonObj.getInt("hum");
                    int time = jsonObj.getInt("time");

                    mSuccess = true;
                    mSensor = new Sensor(temp,press,hum,time);

                } catch (final JSONException e) {

                    msg = "Json parsing error: " + e.getMessage();
                    Log.e("SensorTask", msg);
                }

            } else {
                Log.e("SensorTask", msg);
            }

            return null;
        }

        @Override
        protected AbsTask clone() {
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess){
                dispatch(new BaseEvent(EventType.Sensor, mSensor));
            }
        }
    }
    //http://192.168.8.100/taps?pin=27&open=1
    private class TapsTask extends AbsTask {

/*[
  {
    "pin": 27,
    "open": 1
  },
]*/

        public List<Tap> taps = new ArrayList<>();
        private Tap mTap;

        public TapsTask() {}
        public TapsTask(Tap tap) {
            mTap = tap;
        }

        @Override
        protected Void doInBackground(Void... arg0) {
            HttpHandler sh = new HttpHandler();
            // Making a request to url and getting response
            String url = http+localHost+"/taps";

            if(mTap != null){
                url+= "?pin=" + mTap.getPin();
                url+= "&cmd="+ (mTap.getOpen()? 1 : 0);
            }

            String jsonStr = sh.makeServiceCall(url);

            if (jsonStr != null) {
                //Log.e("TapsTask", jsonStr);
                try {
                    JSONArray jsonArray = new JSONArray(jsonStr);

                    for (int i = 0; i < jsonArray.length(); i++) {
                        JSONObject obj = jsonArray.getJSONObject(i);
                        int pin = obj.getInt("pin");
                        int open = obj.getInt(("open"));

                        Tap tapC = new Tap(pin,open);
                        taps.add(tapC);
                    }

                    mSuccess = true;

                } catch (final JSONException e) {

                    msg = "Json parsing error: " + e.getMessage();
                    Log.e("TapsTask", msg);
                }

            } else {
                Log.e("TapsTask", msg);
            }

            return null;
        }

        @Override
        protected AbsTask clone() {
            return new TapsTask(mTap);
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess)
                updateTaps(taps);
        }
    }

    //http://192.168.8.100/sensorLog
    private class SensorLogTask extends AbsTask{

/*
[
    {
    "temp": 10.79,
    "press": 971.9171,
    "hum": 62,
    "time": 1543471201
    },
*/
        @Override
        protected Void doInBackground(Void... params) {
            HttpHandler sh = new HttpHandler();
            // Making a request to url and getting response
            String url = http+localHost+"/sensorLog";
            String jsonStr = sh.makeServiceCall(url);

            if (jsonStr != null) {
                //Log.e("SensorLogTask", jsonStr);
                try {
                    JSONArray jsonArray = new JSONArray(jsonStr);

                    List<Sensor> sensorLog = new ArrayList<Sensor>();

                    for (int i = 0; i < jsonArray.length(); i++) {
                        JSONObject jsonObj = jsonArray.getJSONObject(i);


                        float temp = Float.valueOf(jsonObj.getString("temp"));
                        float  press = Float.valueOf(jsonObj.getString("press"));
                        int hum = jsonObj.getInt("hum");
                        int time = jsonObj.getInt("time");

                        sensorLog.add( new Sensor(temp,press,hum,time));

                    }

                    mSuccess = true;
                    mSensorLog = sensorLog;
                } catch (final JSONException e) {

                    msg = "Json parsing error: " + e.getMessage();
                    Log.e("SensorLogTask", msg);
                }

            } else {
                Log.e("SensorLogTask", msg);
            }

            return null;
        }

        @Override
        protected AbsTask clone() {
            return new SensorLogTask();
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess){
                dispatch(new BaseEvent(EventType.SensorLog, mSensorLog));
            }
        }
    }
    //http://192.168.8.100/status
    private class StatusTask extends AbsTask {

        @Override
        protected Void doInBackground(Void... arg) {
            // Making a request to url and getting response
            String url = http+localHost+"/status";

            HttpHandler sh = new HttpHandler();
            String r = sh.makeServiceCall(url);

            if (r != null) {
                mSuccess = true;
                //Log.e("StatusTask", r);
                updateStatus(new ModelJardin.Status(r));

            } else {
                Log.e("StatusTask", msg);
            }

            return null;
        }

        @Override
        protected AbsTask clone() {
            return null;
        }
    }

    //http://192.168.8.100/zones?zone=1&cmd=1
    private class ZonesTask extends AbsTask {

        private int mZone = -1;
        private int mCmd = -1;
        // cmd = 0 close
        // cmd = 1 open
        // cmd = 2 pause

        public ZonesTask(){}

        public ZonesTask(int position,int cmd) {
            mZone = position;
            mCmd = cmd;
        }


        public List<Zone> zones = new ArrayList<>();

/*[
  {
    "id": 0,
    "name": "Cesped D",
    "running": false,
    "duration": 60,
    "start": 25200
  },
]*/

        @Override
        protected Void doInBackground(Void... arg0) {
            HttpHandler sh = new HttpHandler();
            // Making a request to url and getting response
            String url = http+localHost+"/zones";

            if(mZone > -1){
                url+= "?id=" + mZone;
                url+= "&cmd="+ mCmd;
            }

            String jsonStr = sh.makeServiceCall(url);

            if (jsonStr != null) {
                //Log.e("ZonesTask", jsonStr);
                try {
                    JSONArray jsonZones = new JSONArray(jsonStr);

                    for (int i = 0; i < jsonZones.length(); i++) {
                        JSONObject jsonZone = jsonZones.getJSONObject(i);

                        String name = jsonZone.getString("name");
                        int id = jsonZone.getInt(("id"));
                        boolean running = jsonZone.getInt("running")>0;
                        boolean paused = jsonZone.getInt("paused")>0;
                        int duration =  jsonZone.getInt("duration");
                        int start = jsonZone.getInt("start");
                        int elapsed = jsonZone.getInt("elapsed");

                        Zone zone = new Zone(id,name,running,duration,start,elapsed,paused);
                        zones.add(zone);

                        mListAlarms.add(null);
                    }
                    mSuccess = true;

                } catch (final JSONException e) {
                    msg = "Json parsing error: " + e.getMessage();
                    Log.e("ZonesTask", msg);
                }

            } else {
                Log.e("ZonesTask", msg);
            }

            return null;
        }

        @Override
        protected AbsTask clone() {
            return new ZonesTask(mZone,mCmd);
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess)
                updateZones(zones);
        }
    }

    //http://192.168.8.100/modes?mode=1
    private class ModesTask extends AbsTask {
        private int mIndex = -1;
        private List<Mode> modes = new ArrayList<>();
        private String prediction;

        public ModesTask(int position) {
            mIndex = position;
        }
        public ModesTask() {}

        @Override
        protected Void doInBackground(Void... params) {

            String url = http+localHost+"/modes";
            if(mIndex >-1)
                url+= "?mode=" + mIndex;

            HttpHandler sh = new HttpHandler();
            String jsonStr = sh.makeServiceCall(url);

            if (jsonStr != null) {
                //Log.e("TapsTask", jsonStr);
                try {
                    JSONObject jsonObject = new JSONObject(jsonStr);

                    prediction = jsonObject.getString("prediction");

                    JSONArray jsonArray = jsonObject.getJSONArray("modes");

                    for (int i = 0; i < jsonArray.length(); i++) {
                        JSONObject obj = jsonArray.getJSONObject(i);
                        String name = obj.getString("name");
                        boolean enabled = obj.getBoolean(("enabled"));

                        Mode mode = new Mode(name,enabled);
                        modes.add(mode);
                    }

                    mSuccess = true;

                } catch (final JSONException e) {

                    msg = "Json parsing error: " + e.getMessage();
                    Log.e("ModesTask", msg);
                }

            } else {
                Log.e("ModesTask", msg);
            }
            return null;
        }


        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess)
                updateModes(modes, prediction);
        }

        @Override
        protected AbsTask clone() {
            return new ModesTask(mIndex);
        }
    }

    private class AlarmsTask extends AbsTask {
        //http://192.168.8.100/alarms?zone=1

        public List<Alarm> alarms = new ArrayList<>();
/*
,
  {
    "pin": 27,
    "time": 75600,
    "duration": 75620
  },

*/
        private int mZone;
        public AlarmsTask(int zone){
            mZone = zone;
        }


        @Override
        protected Void doInBackground(Void... arg0) {
            HttpHandler sh = new HttpHandler();
            // Making a request to url and getting response
            String url = http+localHost+"/alarms";
            url += "?zone="+mZone;
            String jsonStr = sh.makeServiceCall(url);

            if (jsonStr != null) {
                Log.e("AlarmTask", jsonStr);
                try {

                    JSONArray jsonAlarms = new JSONArray(jsonStr);

                    if(jsonAlarms != null && jsonAlarms.length() > 0)
                    {
                        for (int j = 0; j < jsonAlarms.length(); j++)
                        {
                            JSONObject jsonAlarm = jsonAlarms.getJSONObject(j);
                            Alarm alarm = new Alarm(
                                    jsonAlarm.getInt("pin"),
                                    jsonAlarm.getInt("time"),
                                    jsonAlarm.getInt("duration")
                            );
                            alarms.add(alarm);
                        }
                    }

                    mSuccess = true;

                } catch (final JSONException e) {
                    msg = "Json parsing error: " + e.getMessage();
                    Log.e("AlarmTask", msg);
                }

            } else {
                Log.e("AlarmTask", msg);
            }

            return null;
        }

        @Override
        protected AbsTask clone() {
            return new AlarmsTask(mZone);
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess)
                updateAlarms(mZone, alarms);
        }
    }

    private class ConfigTask extends AbsTask {
        //edita wifi		localhost/config?cmd=0&ssid=vomistar_69&pass=teta
        //edita meteo		localhost/config?cmd=0&cityID=1451030&cityName=Tintores
        //restar			localhost/config?cmd=1
        //imprime config	localhost/config
        private int mCmd = -1;
        private List<String> mKeys = new ArrayList<>();
        private List<String> mValues = new ArrayList<>();
        private Config mConfig;

        public ConfigTask(int cmd) {
            mCmd = cmd;
        }
        public ConfigTask() {}

        public void addParameter(String key,String value){
            mKeys.add(key);
            mValues.add(value);
        }

        @Override
        protected Void doInBackground(Void... params) {

            // Making a request to url and getting response
            String url = http+localHost+"/config";

            if(mKeys.size()>0){

                url+= "?cmd=0";

                for (int i = 0; i < mKeys.size(); i++) {
                    String key = mKeys.get(i);
                    String value = mValues.get(i);
                    url += "&" + key + "=" + value;
                }

            }else if(mCmd == 1){
                url+= "?cmd=1";
            }else if(mCmd == 2){
                url+= "?cmd=2";
            }else if(mCmd == 3){
                url+= "?cmd=3";
            }

            HttpHandler sh = new HttpHandler();
            String r = sh.makeServiceCall(url);

            if (r != null) {

                if(mKeys.size()<1){
                    try {
                        JSONObject obj = new JSONObject(r);

                        int RSSI = obj.getInt("RSSI");
                        String distanceRSSI = obj.getString("distanceRSSI");
                        String ssid = obj.getString("ssid");
                        String cityID = obj.getString("cityID");
                        String cityName = obj.getString("cityName");

                        mConfig = new Config(
                                RSSI, distanceRSSI,
                                ssid,
                                cityID, cityName
                        );

                    }catch (final JSONException e){
                        msg = "Json parsing error: " + e.getMessage();
                        Log.e("ConfigTask", msg);
                    }
                }

                mSuccess = true;

            } else {
                Log.e("ConfigTask", msg);
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            if(mSuccess && mConfig != null)
                dispatch(new BaseEvent(EventType.Config, mConfig));
            else if(mSuccess){
                callConfigLoadData();
            }
        }

        @Override
        protected AbsTask clone() {
            if(mKeys.size()>0){

                ConfigTask c = new ConfigTask();

                for (int i = 0; i < mKeys.size(); i++) {
                    String key = mKeys.get(i);
                    String value = mValues.get(i);
                    c.addParameter(key,value);
                }

                return c;
            }

            return new ConfigTask(mCmd);
        }
    }

    //nuevo		localhost/editZone?cmd=0&name=Huerta
    //edit		localhost/editZone?cmd=1&id=1&name=Lechugas
    //borrar	localhost/editZone?cmd=2&id=2
    private class EditZoneTask extends AbsTask {
        private int mZone;
        private int mCmd;
        private String mName;

        //nuevo
        public EditZoneTask(String name) {
            mCmd = 0;
            mName = name;
        }
        //edit
        public EditZoneTask(int zone ,  String name) {
            mCmd = 1;
            mZone = zone;
            mName = name;
        }
        //borrar
        public EditZoneTask(int zone) {
            mCmd = 2;
            mZone = zone;
        }

        @Override
        protected Void doInBackground(Void... params) {

            String url = http+localHost+"/editZone";

            switch (mCmd){
                case 0 :
                    url+= "?cmd=0";
                    url+= "&name=" + mName;
                    break;
                case 1 :
                    url+= "?cmd=1";
                    url+= "&name=" + mName;
                    url+= "&id=" + mZone;
                    break;
                case 2:
                    url+= "?cmd=2";
                    url+= "&id=" + mZone;
                    break;
            }

            HttpHandler sh = new HttpHandler();
            String r = sh.makeServiceCall(url);

            if (r != null) {
                Log.e("EditZoneTask", r);
                mSuccess = true;
            } else {
                Log.e("EditZoneTask", msg);
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess)
            {
                //actualizamos
                enqueueCall(new ZonesTask());
                nextCall();
            };
        }

        @Override
        protected AbsTask clone() {

            switch (mCmd){
                case 0 :
                    return new EditZoneTask(mName);
                case 1 :
                    return new EditZoneTask(mZone,mName);
                case 2:
                    return new EditZoneTask(mZone);
            }

            return null;
        }


    }
    //nuevo		localhost/editAlarm?cmd=0&idZone=2&pin=17&time=64800&duration=25
    //edit		localhost/editAlarm?cmd=1&idZone=2&pin=17&time=64800&duration=25&id=0
    //borrar	localhost/editAlarm?cmd=2&idZone=2&id=0
    private class EditAlarmTask extends AbsTask {
        private int mZone;
        private int mCmd;
        private int mAlarmId;
        private Alarm mAlarm;

        //nuevo
        public EditAlarmTask(int zone, Alarm alarm) {
            mCmd = 0;
            mZone = zone;
            mAlarm = alarm;
        }
        //edit
        public EditAlarmTask(int zone, Alarm alarm, int id) {
            mCmd = 1;
            mZone = zone;
            mAlarmId = id;
            mAlarm = alarm;
        }
        //borrar
        public EditAlarmTask(int zone , int id) {
            mCmd = 2;
            mZone = zone;
            mAlarmId = id;
        }

        @Override
        protected Void doInBackground(Void... params) {

            String url = http+localHost+"/editAlarm";

            switch (mCmd){
                case 0 :
                    url+= "?cmd=0";
                    url+= "&idZone=" + mZone;
                    url+= "&pin=" + mAlarm.getPin();
                    url+= "&time=" + mAlarm.getTime();
                    url+= "&duration=" + mAlarm.getDuration();
                    break;
                case 1 :
                    url+= "?cmd=1";
                    url+= "&idZone=" + mZone;
                    url+= "&pin=" + mAlarm.getPin();
                    url+= "&time=" + mAlarm.getTime();
                    url+= "&duration=" + mAlarm.getDuration();
                    url+= "&id=" + mAlarmId;

                    break;
                case 2:
                    url+= "?cmd=2";
                    url+= "&idZone=" + mZone;
                    url+= "&id=" + mAlarmId;
                    break;
            }

            HttpHandler sh = new HttpHandler();
            String r = sh.makeServiceCall(url);

            if (r != null) {
                Log.e("EditAlarmTask", r);
                mSuccess = true;
                if(r=="0" || r == "-1")
                    dispatch(new BaseEvent(EventType.Error,null));
            } else {
                Log.e("EditAlarmTask", msg);
            }
            return null;
        }


        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            if(mSuccess)
            {
                //actualizamos
                enqueueCall(new AlarmsTask(mZone));
                nextCall();
            };
        }

        @Override
        protected AbsTask clone() {

            switch (mCmd){
                case 0 :
                    return new EditAlarmTask(mZone,mAlarm);
                case 1 :
                    return new EditAlarmTask(mZone,mAlarm,mAlarmId);
                case 2:
                    return new EditAlarmTask(mZone,mAlarmId);
            }

            return null;
        }
    }

    // calls rest service
    public void callGetSensor(){
        enqueueCall( new SensorTask());
        nextCall();
    }
    public void callGetTaps(){
        if(mTaps != null)
            dispatch(new BaseEvent(EventType.Taps, mTaps));
    }
    public void callOpenTap(Tap tap){
        enqueueCall(new TapsTask(tap));
        nextCall();
    }
    public void callGetZones() {
        enqueueCall(new ZonesTask());
        nextCall();
    }
    public void callGetSensorLog() {
        if(mSensorLog.size() > 0)
            dispatch(new BaseEvent(EventType.SensorLog,mSensorLog));
    }
    public void callGetAlarms(int zone) {
        /*if(zone+1 > mListAlarms.size() || mListAlarms.get(zone) == null){

        }else{
            List<Alarm> data =  mListAlarms.get(zone);
            dispatch(new BaseEvent(EventType.Alarm,data));
        } */

        enqueueCall(new AlarmsTask(zone));
        nextCall();
    }
    public void callSaveZone(int zone, String name) {
        if(zone > -1){//edit
            enqueueCall(new EditZoneTask(zone,name));
            nextCall();
        }else {//new
            enqueueCall(new EditZoneTask(name));
            nextCall();
        }
    }
    public void callRemoveZone(int zone) {
        enqueueCall(new EditZoneTask(zone));
        nextCall();
    }
    public void callSaveAlarm(int zone, Alarm alarm,int index) {
        if(index > -1){//edit
            enqueueCall(new EditAlarmTask(zone,alarm,index));
            nextCall();
        }else {//new
            enqueueCall(new EditAlarmTask(zone,alarm));
            nextCall();
        }
    }
    public void callRemoveAlarm(int zone, int index) {
        enqueueCall(new EditAlarmTask(zone,index));
        nextCall();
    }
    public void callZone(int zone, int cmd) {
        enqueueCall(new ZonesTask(zone,cmd));
        nextCall();
    }
    public void callGetModes() {
        enqueueCall(new ModesTask());
        nextCall();
    }
    public void callGetMode(int index) {
        enqueueCall(new ModesTask(index));
        nextCall();
    }

    public void callConfigWeather(String cityID,String cityName) {
        //edita meteo		localhost/mConfig?cmd=0&cityID=1451030&cityName=Tintores
        ConfigTask config = new ConfigTask();
        config.addParameter("cityID",cityID);
        config.addParameter("cityName", cityName);
        enqueueCall(config);
        nextCall();
    }
    public void callConfigWiFi(String ssid,String password) {
        //edita wifi		localhost/mConfig?cmd=0&ssid=vomistar_69&pass=teta
        ConfigTask config = new ConfigTask();
        config.addParameter("ssid",ssid);
        config.addParameter("pass", password);
        enqueueCall(config);
        nextCall();
    }
    public void callConfigReBoot() {
        enqueueCall(new ConfigTask(2));
        nextCall();
    }
    public void callConfigResetData() {
        enqueueCall(new ConfigTask(1));
        nextCall();
    }
    public void callConfigResetDataSensor() {
        enqueueCall(new ConfigTask(3));
        nextCall();
    }
    public void callConfigLoadData() {
        //imprime config	localhost/config
        enqueueCall(new ConfigTask());
        nextCall();
    }

    private List<Tap> mTaps;
    private List<Mode> mModes;
    private String mPrediction;
    private List<Zone> mZones;
    private List<List<Alarm>> mListAlarms = new ArrayList<>();
    private Sensor mSensor;
    private EnQueue<AbsTask> mCalls = new EnQueue();
    private Timer mTimerStatus;
    private boolean mCallsRunning = false;
    private Status mStatus = new Status("0|0|0");
    private List<Sensor> mSensorLog = new ArrayList<Sensor>();
    private boolean firstRun = true;

    private void loadData(){

        enqueueCall(new TapsTask());
        enqueueCall(new ZonesTask());
        enqueueCall(new SensorLogTask());
        enqueueCall(new ModesTask());
        nextCall();

    }

    public void start() {
        if(firstRun){

            firstRun = false;

            final Handler handler = new Handler();
            mTimerStatus = new Timer();

            mTimerStatus.scheduleAtFixedRate(new TimerTask(){
                @Override
                public void run(){
                    handler.post(new Runnable() {
                        public void run() {
                            try {
                                new StatusTask().execute();
                            } catch (Exception e) {
                                // TODO Auto-generated catch block
                                Log.i("timer(model.loadData)", e.getMessage());
                            }
                        }
                    });
                }
            },1000,1000);
        }
    }

    public void stop() {
        if(mTimerStatus!=null) mTimerStatus.cancel();
        mTimerStatus = null;
        mCalls.clear();
        firstRun = true;
        mCallsRunning = false;
    }

    public List<Tap> getTaps() {
        return mTaps;
    }

    public Tap getTap(int index) {
        if(index > -1 && index < mTaps.size()){
            return mTaps.get(index);
        }
        return null;
    }

    public int getIndexTap(int pin) {
        for (int i = 0; i < mTaps.size(); i++) {
            if(pin == mTaps.get(i).getPin())
                return i;
        }
        return 0;
    }

    public Zone getZone(int zone) {
        if(zone >-1 && zone < mZones.size())
            return mZones.get(zone);
        else return null;
    }

    public int getZoneIndex(String name) {

        for (int i = 0; i < mZones.size(); i++) {
            Zone zone = mZones.get(i);
            if(zone.getName().equals(name))
                return i;
        }
        return -1;
    }

    public Alarm getAlarm(int zone, int index) {
        if(zone > -1 && zone < mListAlarms.size()){
            List<Alarm> alarms = mListAlarms.get(zone);
            if(index > -1 && index < alarms.size()){
                return alarms.get(index);
            }
        }
        return null;
    }

    public Alarm getLastAlarm(int zone) {
        if(zone > -1 && zone < mListAlarms.size()){
            List<Alarm> alarms = mListAlarms.get(zone);
            if(alarms != null && alarms.size()>0)
                return alarms.get(alarms.size()-1);
        }
        return null;
    }

    public String getPrediction() {
        return mPrediction;
    }

    private void updateTaps( List<Tap> taps){
        mTaps = taps;
        dispatch(new BaseEvent(EventType.Taps, mTaps));
    }

    private void updateModes( List<Mode> modes, String prediction){
        mModes = modes;
        mPrediction = prediction;
        dispatch(new BaseEvent(EventType.Modes, mModes));
    }

    private void updateZones( List<Zone> zones){
        mZones = zones;
        dispatch(new BaseEvent(EventType.Zone, mZones));
    }

    private void updateAlarms(int zone, List<Alarm> alarms) {
        mListAlarms.set(zone,alarms);

        dispatch(new BaseEvent(EventType.Alarm, alarms));
    }

    //encolamiento de AsyncTasks (llamadas al servidor)
    private void nextCall()
    {
        if(mCalls.size() == 0){
            mCallsRunning = false;
        }
        else if (!mCallsRunning){

        }
        if(mCalls.size()>0){
            //todo borrar esto
            mCallsRunning = true;
            AbsTask next = mCalls.dequeue();
            Log.d("model", "nextCall: "+next.getClass().getSimpleName());
            next.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            //next.execute();
        }
    }

    private  void enqueueCall(AbsTask task){
        mCalls.enqueue(task);
    }

    private void updateStatus(Status status) {

        if (mStatus.tapsInt != status.tapsInt){
            enqueueCall(new TapsTask());
            nextCall();
        }
        if(mStatus.timeZoneChanged != status.timeZoneChanged){
            enqueueCall(new ZonesTask());
            nextCall();
        }
        if(mStatus.timeModesChanged != status.timeModesChanged){
            enqueueCall(new ZonesTask());
            nextCall();
        }

        mStatus = status;
    }

    //https://github.com/cadewey/android_frameworks_base/blob/eldarerathis-7.1.x/wifi/java/android/net/wifi/WifiManager.java#L1530
    /** Anything worse than or equal to this will show 0 bars. */
    private static final int MIN_RSSI = -100;

    /** Anything better than or equal to this will show the max bars. */
    private static final int MAX_RSSI = -55;

    /**
     * Calculates the level of the signal. This should be used any time a signal
     * is being shown.
     *
     * @param rssi The power of the signal measured in RSSI.
     * @param numLevels The number of levels to consider in the calculated
     *            level.
     * @return A level of the signal, given in the range of 0 to numLevels-1
     *         (both inclusive).
     */
    public static int calculateSignalLevel(int rssi, int numLevels) {
        if (rssi <= MIN_RSSI) {
            return 0;
        } else if (rssi >= MAX_RSSI) {
            return numLevels - 1;
        } else {
            float inputRange = (MAX_RSSI - MIN_RSSI);
            float outputRange = (numLevels - 1);
            return (int)((float)(rssi - MIN_RSSI) * outputRange / inputRange);
        }
    }
}
