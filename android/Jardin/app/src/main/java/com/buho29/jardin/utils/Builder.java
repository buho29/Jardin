package com.buho29.jardin.utils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.buho29.jardin.R;

public class Builder {


    public static void showException(final Activity activity, final Exception e) {

        activity.runOnUiThread(new Runnable(){
            @Override
            public void run() {
                final View view =  LayoutInflater.from(activity).inflate(R.layout.dialog_ex, null);
                final TextView tv = (TextView) view.findViewById(R.id.tv);

                StringBuilder trace = new StringBuilder();
                trace.append(e.toString()+ "\n\n");

                StackTraceElement[] stack = e.getStackTrace();
                for (int i=0; i<stack.length; i++) {
                    trace.append(stack[i].toString() + "\n");
                }
                tv.setText(trace);
                new AlertDialog.Builder(activity)
                        .setView(view)
                        .setTitle("Error: "+activity.getClass().getSimpleName())
                        .show();
            }
        });
    }


    public static void makeDialog(Context c,int messageId,DialogInterface.OnClickListener r) {

        //LayoutInflater factory = LayoutInflater.from(c);
        //View view = factory.inflate(R.layout.note_dialog, null);

        new AlertDialog.Builder(c)
                .setTitle(R.string.error)
                .setMessage(messageId)
                .setPositiveButton(R.string.ok, r)
                //.setView(view)
                .show();
    }

    public static void makeDialog(Context c,int messageId) {
        makeDialog(c,messageId,null);
    }
}