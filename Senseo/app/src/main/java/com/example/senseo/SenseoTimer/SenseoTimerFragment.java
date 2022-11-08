package com.example.senseo.SenseoTimer;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Toast;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.example.senseo.MyObjects.Settings;
import com.example.senseo.R;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class SenseoTimerFragment extends Fragment {
    public onSendTimerToSenseo sendTimers;

    public interface onSendTimerToSenseo{
        void onSendTimers(String msg);
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        try{
            sendTimers = (onSendTimerToSenseo)activity;
        }catch (ClassCastException e){
            throw new ClassCastException(activity.toString() + " must implement OnFragmentSendText");
        }
    }

    private LinearLayout timerLayout;

    private final int maxTimers = 3;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View v = inflater.inflate(R.layout.senseo_timer_layout, container, false);

        timerLayout = v.findViewById(R.id.timerLayout);

        v.findViewById(R.id.btn_add_timer).setOnClickListener(addListener);
        Button btn_save = (Button) v.findViewById(R.id.btn_save_timer);
        btn_save.setOnClickListener(saveListener);

        ScrollView scrollView = v.findViewById(R.id.timerScrollView);

        v.post(()->{
            ViewGroup.LayoutParams params = scrollView.getLayoutParams();
            params.height = v.getHeight() - (btn_save.getHeight() * 3);

            scrollView.setLayoutParams(params);
        });

        if(sendTimers != null){
            sendTimers.onSendTimers("getTimers");
        }

        return v;
    }

    private View.OnClickListener addListener = e->{
        if(timerLayout.getChildCount() < maxTimers) {
            timerLayout.addView(new TimerLayout(getContext()));
        }
    };

    private View.OnClickListener saveListener = e->{
        JSONObject object = new JSONObject();
        JSONArray array = new JSONArray();

        for(int i = 0; i < timerLayout.getChildCount(); i++){
            if(timerLayout.getChildAt(i) instanceof TimerLayout){
                TimerLayout layout = (TimerLayout) timerLayout.getChildAt(i);
                array.put(layout.getJson());
            }
        }

        try {
            object.put("null", "null");
            object.put("timerArray", array);
        } catch (JSONException ex) {
            ex.printStackTrace();
        }

        if(sendTimers != null){
            sendTimers.onSendTimers(object.toString());
        }
    };

    public void addTimers(String message){
        try {
            JSONObject object = new JSONObject(message);
            JSONArray array = object.getJSONArray("timerArray");

            for(int i = 0; i < array.length(); i++){
                JSONObject jsonObject = array.getJSONObject(i);

                if(timerLayout.getChildCount() < maxTimers) {
                    TimerLayout layout = new TimerLayout(getContext());
                    layout.setFromJson(jsonObject);

                    if(jsonObject.optBoolean("visible")) {
                        timerLayout.addView(layout);
                    }
                }
            }

        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    //close fragment and get back to MainActivity
    public void closeFragment(){
        FragmentManager manager = getFragmentManager();
        FragmentTransaction transaction = manager.beginTransaction();
        manager.getBackStackEntryCount();
        transaction.remove(this);
        transaction.commit();
    }
}
