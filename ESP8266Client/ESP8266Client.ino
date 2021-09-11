#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <String.h>


/***************
 * class Timer
 ***************/
class Timer {
  private:
    unsigned long _lastTick;
    unsigned long _currentTick;
    
    Timer() { }

    ~Timer() { }
  public:
  static Timer* getInstance() {
    static Timer* instance = new Timer();
    return instance; 
  }
  
  /*
   * Gọi trong hàm setup 1 lần duy nhất
   */
  void initialize() {
    _lastTick = millis(); 
  }
  
  /*
   * Gọi đầu tiên nhất trong hàm loop()
   */
  void update() {
    _currentTick = millis();
  }
  
  /*
   * Trả về thời gian giữa 2 lần loop()
   * giá trị delta là millisecond
   */
  unsigned long delta() {
    return _currentTick - _lastTick;
  }
  
  /*
   * Yêu cầu gọi cuối cùng trong hàm loop()
   */
  void resetTick() {
    _lastTick = _currentTick;
  }
};


/**************************
 * class WorkScheduler
 *************************/

class WorkScheduler {
  private:
    unsigned long _ellapsedTime;
    unsigned long _workTime;
    void (*func)();
  public:
    WorkScheduler(unsigned long time,void (*func)()) {
      _workTime = time;
      _ellapsedTime = 0;
      this->func = func;
    }
    
    ~WorkScheduler() {
      _workTime = 0; 
      _ellapsedTime = 0;
      func = NULL;
    }
    
    void update() {
      _ellapsedTime += Timer::getInstance()->delta();
      if (_ellapsedTime >= _workTime) {
        _ellapsedTime -= _workTime;
       if (func != NULL) {
         func();
       } 
      }
    }
};


/***
 * ESP8266 Client
 */

const char* ssid = "**********";
const char* password = "*********";
//const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* mqtt_server = "broker.emqx.io";
WiFiClient espClient;
PubSubClient client(espClient);

// DHT11
const int DHT_PIN = 14;
const int DHT_TYPE = DHT11;
DHT dht(DHT_PIN, DHT_TYPE);

// distance sensor
const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

// relay pin
const int RELAY_FAN = 13;
const int RELAY_PUMP = 15;

// topic for MQTT publish
const char* AIR_HUMIDITY_TOPIC = "garden/sensor/air_humidity";
const char* SOIL_HUMIDITY_TOPIC = "garden/sensor/soil_humidity";
const char* WATER_TOPIC = "garden/sensor/water";

// Scheduler
WorkScheduler *airHumiditySensorWorkScheduler;
WorkScheduler *soilHumiditySensorWorkScheduler;
WorkScheduler *distanceSensorWorkScheduler;


// function for work scheduler

void onAirSensorWork() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    return;
  }
//  Serial.print("Humidity: ");
//  Serial.print(h);
//  Serial.print(" %\t");
//  Serial.println();
  //Serial.print("Humidity:");
  //Serial.println(String(h).c_str());
  client.publish(AIR_HUMIDITY_TOPIC, String(h).c_str(), true);
  if (h < 75.0f) {
    digitalWrite(RELAY_FAN, LOW);
  } else if (h > 80.0f) {
    digitalWrite(RELAY_FAN, HIGH);
  }
}

void onSoilSensorWork() {
  int value = analogRead(A0);
//  Serial.println(value);
  int percent = map(value, 0, 1023, 100, 0);
  client.publish(SOIL_HUMIDITY_TOPIC, String(percent).c_str(), true);
  if (percent < 60) {
    digitalWrite(RELAY_PUMP, HIGH);
  } else if (percent > 65) {
    digitalWrite(RELAY_PUMP, LOW);
  }

//  Serial.print("Soil Humidity");
//  Serial.print(percent);
//  Serial.print("%");
//  Serial.println();
}

void onDistanceSensorWork() {
  unsigned long duration; // biến đo thời gian
  int distance;           // biến lưu khoảng cách
  int extant;             // độ cao nước còn lại
  float percent;          // phần trăm lượng nước còn lại
    
  /* Phát xung từ chân trig */
  digitalWrite(TRIG_PIN,LOW);   // tắt chân trig
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN,HIGH);   // phát xung từ chân trig
  delayMicroseconds(5);   // xung có độ dài 5 microSeconds
  digitalWrite(TRIG_PIN,LOW);   // tắt chân trig
    
  /* Tính toán thời gian */
  // Đo độ rộng xung HIGH ở chân echo. 
  duration = pulseIn(ECHO_PIN,HIGH);  
  // Tính khoảng cách đến vật.
  distance = int(duration/2/29.412);
  extant = 19 - distance;
  percent = ((float)extant / 17.0f) * 100;
  client.publish(WATER_TOPIC, String(percent).c_str(), true);
    
  /* In kết quả ra Serial Monitor */
//  Serial.print(distance);
//  Serial.print("cm");
//  Serial.println();
//  Serial.print("water = ");
//  Serial.print(percent);
//  Serial.print("%");
//  Serial.println();
//  Serial.println();
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  dht.begin();
  
  Serial.begin(9600);
  
  // hàm thực hiện chức năng kết nối Wifi và in ra địa chỉ IP của ESP8266
  setup_wifi();
  
  // cài đặt server là broker.mqtt-dashboard.com và lắng nghe client ở port 1883
  client.setServer(mqtt_server, 1883);
  
  // gọi hàm callback để thực hiện các chức năng publish/subcribe
  client.setCallback(callback);
  
  // gọi hàm reconnect() để thực hiện kết nối lại với server khi bị mất kết nối
  reconnect();

  Timer::getInstance()->initialize();

  airHumiditySensorWorkScheduler = new WorkScheduler(30000UL, onAirSensorWork);
  
  soilHumiditySensorWorkScheduler = new WorkScheduler(30000UL, onSoilSensorWork);
  
  distanceSensorWorkScheduler = new WorkScheduler(30000UL, onDistanceSensorWork);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // kết nối đến mạng Wifi
  WiFi.begin(ssid, password);
  
  // in ra dấu . nếu chưa kết nối được đến mạng Wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // in ra thông báo đã kết nối và địa chỉ IP của ESP8266
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  //in ra tên của topic và nội dung nhận được từ kênh MQTT lens đã publish
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // lặp cho đến khi được kết nối trở lại
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266")) {
      Serial.println("connected");
    } else {
      // in ra màn hình trạng thái của client khi không kết nối được với MQTT broker
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // delay 5s trước khi thử lại
      //delay(5000);
    }
  }
}

void loop() {
  // kiểm tra nếu ESP8266 chưa kết nối được thì sẽ thực hiện kết nối lại
  if (!client.connected()) {
      reconnect();
  }
  client.loop();

  Timer::getInstance()->update();
  
  distanceSensorWorkScheduler->update();
  
  soilHumiditySensorWorkScheduler->update();

  airHumiditySensorWorkScheduler->update();
  
  

  Timer::getInstance()->resetTick();
}
