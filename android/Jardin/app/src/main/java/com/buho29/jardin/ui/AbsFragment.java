package com.buho29.jardin.ui;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin;

import butterknife.ButterKnife;

public abstract class AbsFragment extends Fragment implements AbsObservable.OnChangedListener{


    protected ModelJardin model;

    @Override
    public void onStart() {
        super.onStart();
        model = ModelJardin.getInstance();
    }

    @Override
    public void onStop() {
        super.onStop();
        if(model!=null)
            model.removeListener(this);
        else
            Log.e("AbsFragment", "model no existe" );
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View v = inflater.inflate(getLayoutId(), container, false);
        ButterKnife.bind(this,v);
        return v;
    }

    // Return the layout ID for this fragment
    public abstract int getLayoutId();


    @Override
    public  abstract void onChanged(AbsObservable.Event event);
}
