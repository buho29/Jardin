package com.buho29.jardin.model;

import android.util.Log;

import java.util.ArrayList;

public class AbsObservable {

    public interface OnChangedListener{
        public void onChanged(Event event);
    }

    private ArrayList<OnChangedListener> mListener = new ArrayList<>();
    private ArrayList<Integer> mType = new ArrayList<>();

    public synchronized void addListener(int type,OnChangedListener listener){

        if(mListener.contains(listener))
            return;

        mListener.add(listener);
        mType.add(type);

        //Log.e("ADD", mListener.size()+"");
    }
    public synchronized void removeListener(OnChangedListener listener){
        if(mListener.contains(listener)){
            int index = mListener.indexOf(listener);
            mType.remove(index);
            mListener.remove(listener);
        }else
            Log.e("error","removeListener");
    }

    public synchronized void removeAllListener(){
        mListener.clear();
        mType.clear();
    }

    public synchronized void dispatch(Event event){
        try {
            int type = event.getType();
            for (int i = 0; i < mListener.size(); i++) {
                OnChangedListener listener = mListener.get(i);
                if(type == mType.get(i) || mType.get(i) < 0)
                    listener.onChanged(event);
            }
        } catch (Exception e) {
            dispatch(new BaseEvent(0,e));
        }
    }

    public abstract class Event{
        protected int mType;
        protected int mError;

        public Event(int type) {mType = type;}

        public int getError() {return mError;}
        public void setError(int error) {mError = error;}
        public int getType() {return mType;}

        public abstract Object getData();
    }


    public class BaseEvent extends Event{
        private Object mObj;
        public BaseEvent(int type, Object e) {
            super(type);
            mObj = e;
        }

        @Override
        public Object getData() {
            return mObj;
        }

    }
}
