package com.mqttdashboard.broker.garden;

public class Constants {
    public static final String AIR_HUMIDITY_TOPIC = "garden/sensor/air_humidity";
    public static final String SOIL_HUMIDITY_TOPIC = "garden/sensor/soil_humidity";
    public static final String WATER_TOPIC = "garden/sensor/water";

    public static final String HOST = "tcp://broker.mqtt-dashboard.com:1883";
    public static final String CLIENT_ID = "androidAppMqtt";
}
