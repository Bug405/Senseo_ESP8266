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

        //Textfelder laden
        et_ip = v.findViewById(R.id.et_ip);

        if(settings != null && settings.getIp() != null) {
            et_ip.setText(settings.getIp());
        }

        v.findViewById(R.id.btn_back).setOnClickListener(onClick_btn_back);
        v.findViewById(R.id.btn_save).setOnClickListener(onClick_btn_save);

        return v;
    }

    //Einstellungen speichern
    private View.OnClickListener onClick_btn_save = e -> {
        Settings settings = new Settings();
        settings.setIp(et_ip.getText().toString());

        //Settings an MainActivity übergeben
        if(sendSettings != null){
            sendSettings.onSendSettings(settings);
        }

        //Settings speichern
        new MyFile().saveFile(getContext(),"Settings.txt", settings, true);
    };

    //Einstellungen schließen
    private View.OnClickListener onClick_btn_back = e -> {
        closeFragment();
    };

    //Set setttings
    public void setSettings(Settings settings){
        this.settings = settings;
    }

    //Ist Fragment geöffnet
    public boolean isFragmentActive(){
        return fragmentActive;
    }

    //Fragment schließen und zur MainActivity zurückkehren
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
        fragmentActive = false;
        super.onDestroy();
    }
}
