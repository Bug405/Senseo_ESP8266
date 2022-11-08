package com.example.senseo;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import com.example.myfiles.MyFile;
import com.example.senseo.MyObjects.Settings;
import com.example.senseo.SenseoTimer.SenseoTimerFragment;
import com.example.senseo.Settings.SettingsFragment;
import com.example.websocketclient.Client;

import org.json.JSONException;
import org.json.JSONObject;

public class MainActivity extends AppCompatActivity implements Client.ClientListener, SettingsFragment.onSendSettings, SenseoTimerFragment.onSendTimerToSenseo {

    private SettingsFragment settingsFragment;

    private SenseoTimerFragment timerFragment;

    private String title;

    private Settings settings;

    private Client client;

    private String port = "80";

    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        title = getTitle().toString();

        //load settings
        MyFile file = new MyFile();
        file.setShowToast(false);
        Object object = file.loadFile(getApplicationContext(), "Settings.txt");


        if(object instanceof Settings) {
            settings = (Settings) object;
        } else {
            settings = new Settings();
        }

        //start client
        client = new Client(settings.getIp(), port);
        client.setListener(this);
        client.setMsg("INFO");
        client.connect();

        //load background picture
        setBackgroundHeight(findViewById(R.id.iv_background));

        //load buttons
        Button btn_power = findViewById(R.id.btn_power);
        setButtonWidth(btn_power);
        btn_power.setOnClickListener(powerListener);

        Button btn_oneCup = findViewById(R.id.btn_oneCup);
        setButtonWidth(btn_oneCup);
        btn_oneCup.setOnClickListener(oneCupListener);

        Button btn_twoCups = findViewById(R.id.btn_twoCups);
        setButtonWidth(btn_twoCups);
        btn_twoCups.setOnClickListener(twoCupsListener);
    }

    //options menu
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater menuInflater = getMenuInflater();
        menuInflater.inflate(R.menu.mainmenu, menu);
        return true;
    }

    //open settings fragment
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.ItemSettings) {
            settingsFragment = new SettingsFragment();
            settingsFragment.setSettings(settings);
            openFragment(settingsFragment);
        }

        if (item.getItemId() == R.id.ItemTimer) {
            timerFragment = new SenseoTimerFragment();
            openFragment(timerFragment);
        }

        return true;
    }

    //open fragment
    private void openFragment(Fragment fragment){
        if(fragment.isAdded()){
            getSupportFragmentManager().beginTransaction()
                    .replace(android.R.id.content, fragment)
                    .commit();
        }else {
            getSupportFragmentManager().beginTransaction()
                    .add(android.R.id.content, fragment)
                    .commit();
        }
    }

    //restart client after get new ip from settings
    @Override
    public void onSendSettings(Settings settings){
        this.settings = settings;

        client.close();

        client = new Client(settings.getIp(), port);
        client.setListener(this);
        client.setMsg("INFO");
        client.connect();
    };

    @Override
    public void onSendTimers(String msg) {
        //send msg to server
        if(client != null){
            client.sendMessage(msg);
        }

        //if client is not connect, show toast  "not connected"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    }

    //set senseo on / off
    private View.OnClickListener powerListener = e->{
        JSONObject object = new JSONObject();

        //send msg to server
        if(client != null){
            try {
                object.put("press_button", "power");
                client.sendMessage(object.toString());
            } catch (JSONException ex) {
                ex.printStackTrace();
            }
        }

        //if client is not connect, show toast  "not connected"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    };

    //make one cup of coffee
    private View.OnClickListener oneCupListener = e-> {
        JSONObject object = new JSONObject();

        //send msg to server
        if(client != null){
            try {
                object.put("press_button", "one_cup");
                client.sendMessage(object.toString());
            } catch (JSONException ex) {
                ex.printStackTrace();
            }
        }

        //if client is not connect, show toast "not connected"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    };

    //make two cups of coffee
    private View.OnClickListener twoCupsListener = e-> {
        JSONObject object = new JSONObject();

        //send msg to server
        if(client != null){
            try {
                object.put("press_button", "two_cups");
                client.sendMessage(object.toString());
            } catch (JSONException ex) {
                ex.printStackTrace();
            }
        }

        //if client is not connect, show toast "not connected"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    };

    @Override
    public String onClientGetMessage(String message) {
        try {
            JSONObject object = new JSONObject(message);

            if(object.has("state")){
                String state = object.getString("state");

                //state ready
                if(state.equals("ready")){
                    setPowerButton(Color.GREEN);
                }

                //state off
                else if(state.equals("off")){
                    setPowerButton(Color.LTGRAY);
                }

                //state heating or make coffee
                else if(state.equals("busy")){
                    setPowerButton(Color.YELLOW);
                }

                //state failure (no water)
                else if(state.equals("failure")){
                    setPowerButton(Color.RED);
                }
            }

            else if(timerFragment != null){
                //toast if timer update success
                if(object.has("timerUpdateSuccess")){
                    runOnUiThread(()->Toast.makeText(getApplicationContext(), getString(R.string.timer_success), Toast.LENGTH_LONG).show());
                }

                //add timers to layout
                else if(object.has("timerArray")){
                    runOnUiThread(()->timerFragment.addTimers(message));
                }
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return new String();
    }

    //client state
    @Override
    public void onClientConnected(boolean connected) {
        runOnUiThread(() -> {
            if(connected){
                setTitle(title);
            } else {
                setTitle(title + " - " + getString(R.string.connected));

                setPowerButton(Color.LTGRAY);
            }
        });
    }

    //change state of power button
    private void setPowerButton(int color){
        runOnUiThread(() -> {
            try {
                Button btn_power = findViewById(R.id.btn_power);
                btn_power.getBackground().setColorFilter(color, PorterDuff.Mode.MULTIPLY);
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    //set background picture size to display size
    private void setBackgroundHeight(ImageView imageView){
        imageView.post(() -> {
            ViewGroup.LayoutParams layoutParams = imageView.getLayoutParams();
            layoutParams.height = imageView.getHeight() - (imageView.getHeight() / 10);
            imageView.setLayoutParams(layoutParams);
        });
    }

    //set buttons width to display size
    private void setButtonWidth(Button button){
        button.setMinimumWidth(getDisplayWidht() / 3);
    }

    //reconnect to server
    @Override
    protected void onResume() {
        super.onResume();

        if(client != null && client.isClosed()) {
            client.connect();
        }
    }

    //close connection
    @Override
    protected void onPause() {
        super.onPause();

        client.close();
    }

    //if settings fragment is open close settings fragment
    //else close app
    @Override
    public void onBackPressed() {
        //if(settingsFragment.isFragmentActive()){
        //   settingsFragment.closeFragment();
        if(settingsFragment != null || timerFragment != null) {
            if (settingsFragment != null) {
                settingsFragment.closeFragment();
                settingsFragment = null;
            }

            if (timerFragment != null) {
                timerFragment.closeFragment();
                timerFragment = null;
            }
        } else {
            finish();
        }
    }

    //get display pixel width
    private int getDisplayWidht(){
        Display display =((WindowManager) getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        return display.getWidth();
    }
}