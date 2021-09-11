package com.example.senseo;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.example.myfiles.MyFile;
import com.example.senseo.MyObjects.Settings;

public class SettingsFragment extends Fragment {

    private boolean fragmentActive = false;

    public onSendSettings sendSettings;

    public interface onSendSettings{
        void onSendSettings(Settings settings);
    }

    public void setSendSettings(onSendSettings sendSettings){
        this.sendSettings = sendSettings;
    }

    private EditText et_ip;

    private Settings settings;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View v = inflater.inflate(R.layout.settings_layout, container, false);

        //load ip address
        et_ip = v.findViewById(R.id.et_ip);

        if(settings != null && settings.getIp() != null) {
            et_ip.setText(settings.getIp());
        }

        //initialize button listeners
        v.findViewById(R.id.btn_back).setOnClickListener(onClick_btn_back);
        v.findViewById(R.id.btn_save).setOnClickListener(onClick_btn_save);

        return v;
    }

    //save new settings
    private View.OnClickListener onClick_btn_save = e -> {
        Settings settings = new Settings();
        settings.setIp(et_ip.getText().toString());

        //send new settings to MainActivity
        if(sendSettings != null){
            sendSettings.onSendSettings(settings);
        }

        //save settings file
        new MyFile().saveFile(getContext(),"Settings.txt", settings, true);
    };

    //close settings
    private View.OnClickListener onClick_btn_back = e -> {
        closeFragment();
    };

    //set setttings
    public void setSettings(Settings settings){
        this.settings = settings;
    }

    //is fragment  active
    public boolean isFragmentActive(){
        return fragmentActive;
    }

    //close fragment and get back to MainActivity
    public void closeFragment(){
        FragmentManager manager = getFragmentManager();
        FragmentTransaction transaction = manager.beginTransaction();
        manager.getBackStackEntryCount();
        transaction.remove(this);
        transaction.commit();
    }

    @Override
    public void onResume() {
        super.onResume();
        fragmentActive = true;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        fragmentActive = false;
    }
}
