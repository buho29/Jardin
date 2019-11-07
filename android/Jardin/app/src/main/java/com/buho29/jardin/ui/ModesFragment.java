package com.buho29.jardin.ui;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.buho29.jardin.R;
import com.buho29.jardin.model.AbsObservable;
import com.buho29.jardin.model.ModelJardin.Mode;
import com.buho29.jardin.model.ModelJardin.EventType;

import java.util.List;

import butterknife.BindView;


public class ModesFragment extends AbsFragment implements AdapterView.OnItemClickListener {

    @BindView(R.id.listAlarms) ListView mList;
    @BindView(R.id.prediction) TextView mPrediction;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View v = super.onCreateView(inflater, container, savedInstanceState);

        mList.setOnItemClickListener(this);

        return v;
    }

    @Override
    public void onStart() {
        super.onStart();
        model.addListener(EventType.Modes,this);
        model.callGetModes();
    }

    @Override
    public int getLayoutId() {
        return R.layout.fragment_modes;
    }

    @Override
    public void onChanged(AbsObservable.Event event) {
        mPrediction.setText(model.getPrediction());
        mList.setAdapter(new ModesAdapter(getContext(),(List<Mode>) event.getData()));
    }

    /**
     * Callback method to be invoked when an item in this AdapterView has
     * been clicked.
     * <p>
     * Implementers can call getItemAtPosition(position) if they need
     * to access the data associated with the selected item.
     *
     * @param parent   The AdapterView where the click happened.
     * @param view     The view within the AdapterView that was clicked (this
     *                 will be a view provided by the adapter)
     * @param position The position of the view in the adapter.
     * @param id       The row id of the item that was clicked.
     */
    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id)
    {
        model.callGetMode(position);
    }

    public class ModesAdapter extends ArrayAdapter<Mode> {

        public ModesAdapter(Context context, List<Mode> modes) {
            super(context, 0, modes);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {

            // Check if an existing view is being reused, otherwise inflate the view
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(R.layout.list_item, parent, false);
            }

            // Get the data item for this position
            Mode mode = (Mode) getItem(position);

            TextView tv = (TextView) convertView.findViewById(R.id.tv);
                // Populate the data into the template view using the data object

            tv.setText(mode.getName());
            //tv.set
            tv.setSelected(mode.isEnabled());

            // Return the completed view to render on screen
            return convertView;
        }
    }
}
