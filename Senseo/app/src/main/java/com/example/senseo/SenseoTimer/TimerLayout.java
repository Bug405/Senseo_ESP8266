package com.example.senseo.SenseoTimer;

import android.content.Context;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TimePicker;

import com.example.senseo.MainActivity;
import com.example.senseo.R;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class TimerLayout extends LinearLayout {

    private View view;

    private GridLayout layout;

    private TimePicker timePicker;

    private Button btn_active;

    private Spinner spinner;

    private boolean active;

    public TimerLayout(Context context) {
        super(context);

        view = inflate(context, R.layout.timer_layout, null);
        addView(view);

        active = true;

        layout = findViewById(R.id.timerGridLayout);

        timePicker = findViewById(R.id.tp_time);
        timePicker.setIs24HourView(true);
        timePicker.setHour(0);
        timePicker.setMinute(0);

        spinner = findViewById(R.id.sp_choseFunction);
        spinner.setAdapter(getAdapter());

        findViewById(R.id.btn_remove).setOnClickListener(removeListener);

        btn_active = findViewById(R.id.btn_active);
        btn_active.setOnClickListener(activeListener);

        btn_active.post(()->{
            if((btn_active.getRight() + timePicker.getRight()) < getDisplayWidht()) {
                layout.setColumnCount(3);
            }
        });

        setActive(active);
    }

    private OnClickListener removeListener = e->{
        LinearLayout layout = (LinearLayout) getParent();
        layout.removeView(this);
    };

    private OnClickListener activeListener = e->{
        active = !btn_active.getText().equals(getContext().getString(R.string.timer_activated));

        setActive(active);
    };

    public JSONObject getJson(){
        int makeCoffeeWhenReady = spinner.getSelectedItemPosition();
        int hour = timePicker.getHour();
        int minute = timePicker.getMinute();

        JSONObject object = new JSONObject();

        try {
            object.put("makeCoffeeWhenReady", makeCoffeeWhenReady);
            object.put("hour", hour);
            object.put("minute", minute);
            object.put("active", active);
            object.put("visible", true);
        } catch (JSONException e) {
            e.printStackTrace();
        }

        Log.d("Senseo", object.toString());

        return object;
    }

    public void setFromJson(JSONObject object) throws JSONException {
        active = object.optBoolean("active");
        int makeCoffeeWhenReady = object.optInt("makeCoffeeWhenReady");
        int hour = object.optInt("hour");
        int minute = object.optInt("minute");

        setActive(active);
        spinner.setSelection(makeCoffeeWhenReady, true);
        timePicker.setHour(hour);
        timePicker.setMinute(minute);
    }

    private void setActive(boolean active){
        int color = Color.GREEN;
        String text = getContext().getString(R.string.timer_activated);
        Log.d("senseo", btn_active.getText().toString() + " " + R.string.timer_activated);

        if(!active){
            text = getContext().getString(R.string.timer_deactivated);
            color = Color.RED;
        }

        btn_active.setText(text);
        btn_active.getBackground().setColorFilter(color, PorterDuff.Mode.MULTIPLY);
    }


    private SpinnerAdapter getAdapter(){
        List<String> list = new ArrayList<>();
        list.add(getResources().getString(R.string.timer_powerOn));
        list.add(getResources().getString(R.string.timer_daily));
        list.add(getResources().getString(R.string.timer_one_cup));
        list.add(getResources().getString(R.string.timer_two_cups));

        return new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, list);
    }

    //get display pixel width
    private int getDisplayWidht(){
        Display display =((WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        return display.getWidth();
    }
}
