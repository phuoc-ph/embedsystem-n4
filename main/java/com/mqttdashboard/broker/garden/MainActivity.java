package com.mqttdashboard.broker.garden;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    public static TextView txtAirHumidity, txtSoilHumidity, txtWater;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        txtAirHumidity = findViewById(R.id.txtAirHumidity);
        txtSoilHumidity = findViewById(R.id.txtSoilHumidity);
        txtWater = findViewById(R.id.txtWater);

        Intent intent = new Intent(MainActivity.this, MqttMessageService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }
}