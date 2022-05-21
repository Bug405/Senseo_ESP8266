package com.example.senseo.Settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.example.myfiles.MyFile;
import com.example.senseo.MyObjects.Settings;
import com.example.senseo.R;

import java.net.InetAddress;
import java.util.Objects;

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

    private LinearLayout address_layout;

    private Settings settings;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View v = inflater.inflate(R.layout.settings_layout, container, false);

        //set address layout
        address_layout = v.findViewById(R.id.layout_address);

        //load ip address
        et_ip = v.findViewById(R.id.et_ip);

        if(settings != null && settings.getIp() != null) {
            et_ip.setText(settings.getIp());
        }

        //initialize button listeners
        v.findViewById(R.id.btn_find_ip).setOnClickListener(onClick_btn_ip);
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

    //get senseo ip address
    private View.OnClickListener onClick_btn_ip = e -> {
        //clear address layout
        address_layout.removeAllViews();

        //search senseo
        findSenseo();
    };

    //set setttings
    public void setSettings(Settings settings){
        this.settings = settings;
    }

    //search senseo in network
    private void findSenseo(){
        new Thread(){
            @Override
            public void run() {
                super.run();

                try {
                    InetAddress[] addresses = InetAddress.getAllByName("Senseo");

                    //add address to layout
                    for (InetAddress address : addresses) {
                        Objects.requireNonNull(getActivity()).runOnUiThread(() ->
                                address_layout.addView(getIpView(address.getHostAddress())));
                    }
                } catch (Exception e) {
                    Objects.requireNonNull(getActivity()).runOnUiThread(() ->
                            address_layout.addView(getExceptionText(getString(R.string.exception_text))));

                    e.printStackTrace();
                }
            }
        }.start();
    }

    //get new ipView
    private IpView getIpView(String address){
        IpView view = new IpView(getContext());
        view.setAddress(address);
        view.setEditText(et_ip);

        return view;
    }

    //get text view for senseo not found exception
    private TextView getExceptionText(String text){
        TextView textView = new TextView(getContext());
        textView.setText(text);

        return textView;
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
