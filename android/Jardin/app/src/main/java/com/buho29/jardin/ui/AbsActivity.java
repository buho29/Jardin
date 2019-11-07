package com.buho29.jardin.ui;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;

import com.buho29.jardin.model.ModelJardin;

import butterknife.ButterKnife;

public abstract class AbsActivity extends AppCompatActivity {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(getLayoutId());

        ButterKnife.bind(this);

        mModel = ModelJardin.getInstance();
    }

    public abstract int getLayoutId();


    protected ModelJardin mModel;
}
