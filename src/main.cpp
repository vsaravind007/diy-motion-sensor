#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_AHTX0.h>
#include <ArduinoJson.h>

#define BUTTON_PIN_BITMASK 0x8004

#define wifi_ssid "<YOUR_WIFI_SSID_HERE>"
#define wifi_password "YOUR_WIFI_PASSWORD_HERE"

#define mqtt_server "192.168.68.58"
#define mqtt_user "<MQTT_USERNAME_HERE>"
#define mqtt_password "<MQTT_PASSWORD_HERE>"

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 600
#define TIME_TO_SLEEP_ERROR 600

#define LED_PIN 25

Adafruit_AHTX0 aht;
WiFiClient espClient;
PubSubClient client(espClient);

float temperature, humidity;

String stateTopic = "home/multi_sensors/mqtt/temp_motion_sensor_1/action";

void setup_wifi()
{
  delay(20);
  pinMode(LED_PIN, OUTPUT);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  int retry_count = 0;
  digitalWrite(LED_PIN, HIGH);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED && retry_count < 5)
  {
    delay(1000);
    Serial.print(".");
    retry_count++;
  }

  if (retry_count >= 5)
  {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_ERROR * uS_TO_S_FACTOR);
    Serial.println("No WIFI, retrying after 15 minutes");
    digitalWrite(LED_PIN, LOW);
    esp_deep_sleep_start();
  }

  Serial.println("");
  Serial.println("WiFi is OK ");
  Serial.print("=> ESP32 new IP address is: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

void reconnect()
{
  int retry_count = 0;
  while (!client.connected() && retry_count < 5)
  {
    Serial.print("Connecting to MQTT broker ...");
    if (client.connect("OutsideWeatherStation_2", mqtt_user, mqtt_password))
    {
      Serial.println("OK");
    }
    else
    {
      Serial.print("[Error] Not connected: ");
      Serial.print(client.state());
      Serial.println("Wait 5 seconds before retry.");
      retry_count++;

      if (retry_count >= 5)
      {
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_ERROR * uS_TO_S_FACTOR);
        Serial.println("No MQTT SERVER, retrying after 15 minutes");
        digitalWrite(LED_PIN, LOW);
        esp_deep_sleep_start();
      }
      delay(5000);
    }
  }
}

void publishData(boolean motion)
{
  Serial.println("Publishing Data TO MQTT");
  DynamicJsonDocument doc(256);
  char buffer[256];
  if(temperature != 0)
   doc["temperature"] = temperature;
  if(humidity != 0)
   doc["humidity"] = humidity;
  doc["motionDetected"] = motion ? "ON" : "OFF";;
  size_t n = serializeJson(doc, buffer);
  bool published = client.publish(stateTopic.c_str(), buffer, n);
  if (published) {
    Serial.println("Published data to MQTT");
  }
}


void sendMQTTMotionDiscoveryMsg()
{
  String discoveryTopic = "homeassistant/binary_sensor/temp_motion_sensors/config";
  DynamicJsonDocument doc(512);
  char buffer[256];
  doc["name"] = "Multi Sensor - Motion";
  doc["dev_cla"] = "motion";
  doc["stat_t"]   = stateTopic;
  doc["unique_id"] = "multi_sensor_1_motion";
  doc["val_tpl"] = "{{ value_json.motionDetected|default(0) }}";
  size_t n = serializeJson(doc, buffer);
  Serial.println(sizeof(doc));
  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTHumidityDiscoveryMsg()
{
  String discoveryTopic = "homeassistant/sensor/multi_sensor_1/humidity/config";
  DynamicJsonDocument doc(512);
  char buffer[512];
  doc["name"] = "Relative Humidity";
  doc["stat_t"] = stateTopic;
  doc["unit_of_meas"] = "rH";
  doc["dev_cla"] = "humidity";
  doc["unique_id"] = "multi_sensor_1_humidity";
  doc["frc_upd"] = true;
  doc["val_tpl"] = "{{ value_json.humidity|default(0) }}";
  size_t n = serializeJson(doc, buffer);
  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTTemperatureDiscoveryMsg()
{
  String discoveryTopic = "homeassistant/sensor/multi_sensor_1/temperature/config";
  DynamicJsonDocument doc(512);
  char buffer[512];
  doc["name"] = "Ambient Temperature";
  doc["stat_t"] = stateTopic;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["unique_id"] = "multi_sensor_1_temperature";
  doc["frc_upd"] = true;
  doc["val_tpl"] = "{{ value_json.temperature|default(0) }}";
  size_t n = serializeJson(doc, buffer);
  client.publish(discoveryTopic.c_str(), buffer, n);
}


void setup() {
  Serial.begin(9600);
  temperature = 0;
  humidity = 0;
  if (! aht.begin())  {
    Serial.println("Could not find AHT? Check wiring");
  } else {
    sensors_event_t hum, temp;
    aht.getEvent(&hum, &temp);
    temperature = temp.temperature;
    humidity = hum.relative_humidity;
    Serial.println(temperature);
    Serial.println(humidity);
  }
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setBufferSize(512);
  if (!client.connected()) {
    reconnect();
  }
  sendMQTTMotionDiscoveryMsg();
  delay(5);
  sendMQTTHumidityDiscoveryMsg();
  delay(5);
  sendMQTTTemperatureDiscoveryMsg();
  delay(10);
  publishData(1);
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT1){
    while(digitalRead(15)){
      
   }
   publishData(0);
  }
  delay(10);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.println("Sleeping now");
  digitalWrite(LED_PIN, LOW);
  esp_deep_sleep_start();
}
void loop() {
  // put your main code here, to run repeatedly:
}