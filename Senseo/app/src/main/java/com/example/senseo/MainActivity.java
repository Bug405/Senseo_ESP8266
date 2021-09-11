package com.example.senseo;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.os.Bundle;
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

import com.example.clientforesp8266.Client;
import com.example.myfiles.MyFile;
import com.example.senseo.MyObjects.Settings;

public class MainActivity extends AppCompatActivity implements Client.ClientListener {

    private SettingsFragment settingsFragment;

    private String title;

    private Settings settings;

    private Client client;

    private int port = 8266;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        title = getTitle().toString();

        //initialize settings fragment
        settingsFragment = new SettingsFragment();

        //settings Listener
        settingsFragment.setSendSettings(onSendSettings);

        //load settings
        Object object = new MyFile().loadFile(getApplicationContext(), "Settings.txt", true);

        if(object instanceof Settings) {
            settings = (Settings) object;
            settingsFragment.setSettings(settings);
        } else {
            settings = new Settings();
        }

        //start client
        client = new Client(settings.getIp(), port, true);
        client.setClientListener(this);
        client.setMessage("getState");
        client.start();

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
            openFragment(settingsFragment);
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
    private SettingsFragment.onSendSettings onSendSettings = settings -> {
        this.settings = settings;

        client.closeSocket();

        client = new Client(settings.getIp(), port, true);
        client.setClientListener(this);
        client.start();
    };

    //set senseo on / off
    private View.OnClickListener powerListener = e-> {

        //send msg to server
        if(client != null){
            client.sendMsg("power");
        }

        //if client is not connect, show toast "nicht Verbunden"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    };

    //make one cup of coffee
    private View.OnClickListener oneCupListener = e-> {

        //send msg to server
        if(client != null){
            client.sendMsg("one_cup");
        }

        //if client is not connect, show toast "nicht Verbunden"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    };

    //make two cups of coffee
    private View.OnClickListener twoCupsListener = e-> {

        //send msg to server
        if(client != null){
            client.sendMsg("two_cups");
        }

        //if client is not connect, show toast "nicht Verbunden"
        if(getTitle().toString().contains(getString(R.string.connected))){
            Toast.makeText(getApplicationContext(), getString(R.string.connected), Toast.LENGTH_LONG).show();
        }
    };

    @Override
    public void onClientGetMessage(String message) {
        //state ready
        if(message.equals("ready")){
            setPowerButton(Color.GREEN);
        }

        //state of
        else if(message.equals("off")){
            setPowerButton(Color.LTGRAY);
        }

        //state heating or make coffee
        else if(message.equals("busy")){
            setPowerButton(Color.YELLOW);
        }

        //state failure (no water)
        else if(message.equals("failure")){
            setPowerButton(Color.RED);
        }
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

        if(client != null && client.isclosed()){
            client.reconnect();
        }
    }

    //close connection
    @Override
    protected void onPause() {
        super.onPause();

        client.closeSocket();
    }

    //if settings fragment is open close settings fragment
    //else close app
    @Override
    public void onBackPressed() {
        if(settingsFragment.isFragmentActive()){
           settingsFragment.closeFragment();
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