package com.mqttdashboard.broker.garden;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class MqttMessageReceiver extends BroadcastReceiver {

    private static final String TAG = MqttMessageReceiver.class.getName();



    MqttAndroidClient client;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "on Receiver");
        Intent intentReceiver = new Intent(context, MqttNotificationService.class);
        // Mqtt connect
        client = new MqttAndroidClient(context.getApplicationContext(), Constants.HOST, Constants.CLIENT_ID);
        client.setCallback(new MqttCallback() {
            @Override
            public void connectionLost(Throwable cause) {

            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception {
                Log.d(TAG, "topic : " + topic + " + message : " + message.toString());

                if (topic.equals(Constants.AIR_HUMIDITY_TOPIC)) {
                    MainActivity.txtAirHumidity.setText(message.toString() + "%");
                }

                if (topic.equals(Constants.SOIL_HUMIDITY_TOPIC)) {
                    MainActivity.txtSoilHumidity.setText(message.toString() + "%");
                }

                if (topic.equals(Constants.WATER_TOPIC)) {
                    MainActivity.txtWater.setText("Còn lại: " + message.toString() + "%");
                    if (Float.parseFloat(message.toString()) < 30f) {
                        intentReceiver.putExtra("message", message.toString());
                        context.startService(intentReceiver);
                    }
                }

            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {

            }
        });

        try {
            IMqttToken token = client.connect();
            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    // We are connected
                    Log.d(TAG, "onSuccess");
                    subscribeTopic(Constants.AIR_HUMIDITY_TOPIC);
                    subscribeTopic(Constants.SOIL_HUMIDITY_TOPIC);
                    subscribeTopic(Constants.WATER_TOPIC);
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    // Something went wrong e.g. connection timeout or firewall problems
                    Log.d(TAG, "onFailure");

                }
            });
        } catch (MqttException e) {
            e.printStackTrace();
        }

    }

    public void subscribeTopic(String topic) {
        try {
            IMqttToken subToken = client.subscribe(topic, 1);
            subToken.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    // The message was published
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken,
                                      Throwable exception) {
                    // The subscription could not be performed, maybe the user was not
                    // authorized to subscribe on the specified topic e.g. using wildcards

                }
            });
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }
}
