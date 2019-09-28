#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

DHT sensor(D1, DHT11);
WiFiClient tcp;
PubSubClient mqtt(tcp);

const char* wifi_ssid = "ssid";
const char* wifi_passwd = "password";
const char* mqtt_server = "xxxxxxx.mqtt.iot.gz.baidubce.com";
const char* mqtt_clientid = "test_device";
const char* mqtt_user = "xxxxxxx/test_device";
const char* mqtt_passwd = "********";
const char* topic_delta = "$baidu/iot/shadow/test_device/delta";
const char* topic_update = "$baidu/iot/shadow/test_device/update";

void mqtt_callback(const char* topic, byte* payload, unsigned int length)
{
  Serial.println(topic);
  if (String(topic) == topic_delta)
  {
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(2) + 70;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, payload, length);

    bool ledswitch = doc["desired"]["switch"];
    if (ledswitch)
    {
      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  sensor.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_passwd);
  while (!WiFi.isConnected())
  {
    delay(100);
  }
  Serial.println("WiFi is connected");

  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqtt_callback);
}

int prev = 0;

void loop() {
  if (!mqtt.connected())
  {
    mqtt.connect(mqtt_clientid, mqtt_user, mqtt_passwd);
    mqtt.subscribe(topic_delta, 1);
    Serial.println("Baidu IoT Hub connected");
  }
  mqtt.loop();
  if (millis() - prev > 30000) //30s上报一次状态
  {
    prev = millis();
    float temp = sensor.readTemperature();
    float humi = sensor.readHumidity();
    const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3);
    DynamicJsonDocument doc(capacity);

    JsonObject reported = doc.createNestedObject("reported");
    reported["temperature"] = temp;
    reported["humidity"] = humi;
    reported["switch"] = (digitalRead(LED_BUILTIN) == LOW) ? true : false;

    String msg;
    serializeJson(doc, msg);
    Serial.println(msg);
    mqtt.publish(topic_update, msg.c_str());
  }
}
