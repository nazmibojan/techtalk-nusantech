#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define PUBLISH_INTERVAL 60000

String timeString;
const long utcOffsetInSeconds = 25200;
unsigned long lastPublish = 0;

String ssid = "Fopi";
String pass = "ramadhian";
String mqttServer = "mqtt.flexiot.xl.co.id"; // MQTT broker
String mqttUser = "generic_brand_2003-esp32_test-v4_4375";
String mqttPwd = "1587193228_4375";
String deviceId = "Device-1";
String deviceMac = "3022864002837125";
String pubTopic = String("generic_brand_2003/esp32_test/v4/common");
String subTopic =
    String("+/" + deviceMac + "/generic_brand_2003/esp32_test/v4/sub");
String mqttPort = "1883"; // server port number

WiFiClient ESPClient;
PubSubClient ESPMqtt(ESPClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setupWifi();
void setupMQTT();
void publishMessage();
void getLocalTime();
void mqttCallback(char *topic, byte *payload, long length);
void do_actions(const char *message);

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  delay(1000);

  // Disconnecting previous WiFi
  WiFi.disconnect();

  ESPMqtt.setServer(mqttServer.c_str(), mqttPort.toInt());
  ESPMqtt.setCallback(mqttCallback);

  setupWifi();
  setupMQTT();
  timeClient.begin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  if (WiFi.status() == WL_CONNECTED && !ESPMqtt.connected()) {
    setupMQTT();
  }

  if (millis() - lastPublish > PUBLISH_INTERVAL) {
    getLocalTime();
    publishMessage();

    lastPublish = millis();
  }

  ESPMqtt.loop();
}

void setupWifi() {
  delay(10);
  WiFi.begin(ssid.c_str(), pass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void setupMQTT() {
  while (!ESPMqtt.connected()) {
    Serial.println("ESP > Connecting to MQTT...");

    if (ESPMqtt.connect("ESP32Client", mqttUser.c_str(), mqttPwd.c_str())) {
      Serial.println("Connected to Server");
      // subscribe to the topic
      ESPMqtt.subscribe(subTopic.c_str());
    } else {
      Serial.print("ERROR > failed with state");
      Serial.print(ESPMqtt.state());
      Serial.print("\r\n");
      delay(2000);
    }
  }
}

void publishMessage() {
  char msgToSend[1024] = {0};
  const size_t capacity = JSON_OBJECT_SIZE(5);
  DynamicJsonDocument doc(capacity);

  doc["eventName"] = "sensorStatus";
  doc["status"] = "none";
  doc["temp"] = "";
  doc["humid"] = "";
  doc["mac"] = deviceMac;

  serializeJson(doc, msgToSend);

  Serial.println(msgToSend);
  ESPMqtt.publish(pubTopic.c_str(), msgToSend);
}

void getLocalTime() {
  timeClient.update();

  // Optional: Construct String object
  timeString = String(timeClient.getEpochTime());
  Serial.println(timeString);
}

void mqttCallback(char *topic, byte *payload, long length) {
  char msg[256];

  Serial.print("Message arrived [");
  Serial.print(subTopic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
  }
  do_actions(msg);
}

void do_actions(const char *message) {
  const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 110;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, message);

  const char *action = doc["action"];
  JsonObject param = doc["param"];
  const char *paramMac = param["mac"];
  const char *paramAddress = param["address"];
  const char *paramValue = param["value"];

  if (strcmp(paramMac, deviceMac.c_str()) == 0) {
  } else {
    Serial.println("Ignore message to another device");
  }
}