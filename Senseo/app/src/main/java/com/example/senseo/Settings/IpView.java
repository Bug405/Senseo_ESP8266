package com.example.senseo.Settings;

import android.content.Context;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.example.senseo.R;

public class IpView extends LinearLayout {

    private TextView tv_address;
    private EditText editText;
    private String address;

    public IpView(Context context){
        super(context);

        View view = inflate(getContext(), R.layout.ip_layout, null);
        addView(view);

        //add edit text
        tv_address = view.findViewById(R.id.et_address);

        //initialize button listeners
        view.findViewById(R.id.btn_add).setOnClickListener(addListener);
    }

    public void setAddress(String address) {
        tv_address.setText(address);
        this.address = address;
    }

    public void setEditText(EditText editText){
        this.editText = editText;
    }

    private View.OnClickListener addListener = (e)->{
        if(editText != null && address != null) {
            editText.setText(address);
        }
    };
}
