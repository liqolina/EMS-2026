#include "core0/core0Task.h"
#include "Globals.h"

#include <PubSubClient.h>

// =======================
// KONFIGURASI WIFI
// =======================
const char* WIFI_SSID     = "NAMA_WIFI";
const char* WIFI_PASSWORD = "PASSWORD_WIFI";

// =======================
// BROKER MQTT 1
// =======================
const char* MQTT1_SERVER = "192.168.1.10";
const int   MQTT1_PORT   = 1883;
const char* MQTT1_CLIENT_ID = "ESP32S3_MQTT_BROKER_1";

const char* MQTT1_USER = "";
const char* MQTT1_PASS = "";

const char* MQTT1_TOPIC_SUB = "broker1/esp32/control";
const char* MQTT1_TOPIC_PUB = "broker1/esp32/status";

// =======================
// BROKER MQTT 2
// =======================
const char* MQTT2_SERVER = "192.168.1.20";
const int   MQTT2_PORT   = 1883;
const char* MQTT2_CLIENT_ID = "ESP32S3_MQTT_BROKER_2";

const char* MQTT2_USER = "";
const char* MQTT2_PASS = "";

const char* MQTT2_TOPIC_SUB = "broker2/esp32/control";
const char* MQTT2_TOPIC_PUB = "broker2/esp32/status";

// =======================
// OBJECT MQTT
// =======================
WiFiClient wifiClient1;
WiFiClient wifiClient2;

PubSubClient mqttClient1(wifiClient1);
PubSubClient mqttClient2(wifiClient2);

TaskHandle_t mqttTaskHandle = NULL;

// Timer
unsigned long lastPublish = 0;
unsigned long lastReconnectMQTT1 = 0;
unsigned long lastReconnectMQTT2 = 0;


// =======================
// CALLBACK BROKER 1
// =======================
void mqttCallback1(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println();
  Serial.println("[BROKER 1] Pesan masuk");
  Serial.print("Topic   : ");
  Serial.println(topic);
  Serial.print("Payload : ");
  Serial.println(message);

  if (message == "ON") {
    Serial.println("[BROKER 1] Perintah ON diterima");
  } 
  else if (message == "OFF") {
    Serial.println("[BROKER 1] Perintah OFF diterima");
  }
}


// =======================
// CALLBACK BROKER 2
// =======================
void mqttCallback2(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println();
  Serial.println("[BROKER 2] Pesan masuk");
  Serial.print("Topic   : ");
  Serial.println(topic);
  Serial.print("Payload : ");
  Serial.println(message);

  if (message == "ON") {
    Serial.println("[BROKER 2] Perintah ON diterima");
  } 
  else if (message == "OFF") {
    Serial.println("[BROKER 2] Perintah OFF diterima");
  }
}


// =======================
// KONEKSI WIFI
// =======================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println("[WiFi] Menghubungkan...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  Serial.println();
  Serial.println("[WiFi] Terhubung");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());
}


// =======================
// RECONNECT MQTT BROKER 1
// Non-blocking agar broker 2 tetap jalan
// =======================
void reconnectMQTT1() {
  if (mqttClient1.connected()) {
    return;
  }

  if (millis() - lastReconnectMQTT1 < 3000) {
    return;
  }

  lastReconnectMQTT1 = millis();

  Serial.println("[MQTT 1] Menghubungkan ke broker 1...");

  bool connected;

  if (strlen(MQTT1_USER) > 0) {
    connected = mqttClient1.connect(
      MQTT1_CLIENT_ID,
      MQTT1_USER,
      MQTT1_PASS
    );
  } else {
    connected = mqttClient1.connect(MQTT1_CLIENT_ID);
  }

  if (connected) {
    Serial.println("[MQTT 1] Terhubung");

    mqttClient1.subscribe(MQTT1_TOPIC_SUB);
    Serial.print("[MQTT 1] Subscribe: ");
    Serial.println(MQTT1_TOPIC_SUB);

    mqttClient1.publish(MQTT1_TOPIC_PUB, "ESP32-S3 connected to Broker 1");
  } 
  else {
    Serial.print("[MQTT 1] Gagal, rc=");
    Serial.println(mqttClient1.state());
  }
}


// =======================
// RECONNECT MQTT BROKER 2
// Non-blocking agar broker 1 tetap jalan
// =======================
void reconnectMQTT2() {
  if (mqttClient2.connected()) {
    return;
  }

  if (millis() - lastReconnectMQTT2 < 3000) {
    return;
  }

  lastReconnectMQTT2 = millis();

  Serial.println("[MQTT 2] Menghubungkan ke broker 2...");

  bool connected;

  if (strlen(MQTT2_USER) > 0) {
    connected = mqttClient2.connect(
      MQTT2_CLIENT_ID,
      MQTT2_USER,
      MQTT2_PASS
    );
  } else {
    connected = mqttClient2.connect(MQTT2_CLIENT_ID);
  }

  if (connected) {
    Serial.println("[MQTT 2] Terhubung");

    mqttClient2.subscribe(MQTT2_TOPIC_SUB);
    Serial.print("[MQTT 2] Subscribe: ");
    Serial.println(MQTT2_TOPIC_SUB);

    mqttClient2.publish(MQTT2_TOPIC_PUB, "ESP32-S3 connected to Broker 2");
  } 
  else {
    Serial.print("[MQTT 2] Gagal, rc=");
    Serial.println(mqttClient2.state());
  }
}

void Task0D(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(1600));

    mqttClient1.setServer(MQTT1_SERVER, MQTT1_PORT);
    mqttClient1.setCallback(mqttCallback1);
    mqttClient1.setKeepAlive(30);
    mqttClient1.setSocketTimeout(5);

    mqttClient2.setServer(MQTT2_SERVER, MQTT2_PORT);
    mqttClient2.setCallback(mqttCallback2);
    mqttClient2.setKeepAlive(30);
    mqttClient2.setSocketTimeout(5);

    for (;;) {

        reconnectMQTT1();
        reconnectMQTT2();

        vTaskDelay(portMAX_DELAY);
    }
}
