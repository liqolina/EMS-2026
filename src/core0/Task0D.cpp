#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"
#include "environment.h"

#include <WiFi.h>
#include <esp_now.h>
#include <inttypes.h>
#include <string.h>

/*
  =====================================================
  TAG SERIAL MONITOR
  =====================================================
*/

constexpr const char* TAG = "TASK_0D";

/*
  =====================================================
  GATEWAY ADDRESS
  =====================================================
*/

constexpr const uint8_t gatewayPeerAddress[] = GATEWAY_PEER_ADDRESS;

/*
  =====================================================
  DEKLARASI VOID
  =====================================================
*/

static inline void copy_VAR();
static inline void espnow_DataSend();
static inline void espnow_DataRecv(const uint8_t *incomingData, int len);

/*
  =====================================================
  Callback pengiriman
  =====================================================
*/

static inline void onDataSent(const esp_now_send_info_t *sendInfo, esp_now_send_status_t status)
{
    Serial.printf("[DEBUG][%s] Last packet send status: %s\n",
                  TAG,
                  (status == ESP_NOW_SEND_SUCCESS) ? "Delivery Success" : "Delivery Fail");
}

/*
  =====================================================
  Callback penerimaan
  =====================================================
*/

static inline void onDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len)
{
    if (!incomingData || len <= 0) {
        Serial.printf("[WARNING][%s] Invalid ESP-NOW RX data\n", TAG);
        return;
    }

    espnow_DataRecv(incomingData, len);
}

/*
  =====================================================
  ALL STRUCT
  =====================================================
*/

extern InfoESP g_InfoESP;
extern InfoWifiESP g_InfoWifiESP;
extern ValueSensor g_ValueSensor;
extern CalibSensor g_CalibSensor;
extern CmdAct g_CmdAct;
extern CalibAct g_CalibAct;
extern StatusNews g_StatusNews;

InfoESP local_InfoESP;
InfoWifiESP local_InfoWifiESP;
ValueSensor local_ValueSensor;
CalibSensor local_CalibSensor;
CmdAct local_CmdAct;
CalibAct local_CalibAct;
StatusNews local_StatusNews;

/*
  =====================================================
  TASK 0D
  =====================================================
*/

void Task0D(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(15000));

    while (!(wifiSTA_running.load() && wifiStatus_running.load())) {
        Serial.printf("[WARNING][%s] Waiting for WiFi mode before ESP-NOW init...\n", TAG);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (esp_now_init() != ESP_OK) {
        Serial.printf("[ERROR][%s] Error initializing ESP-NOW\n", TAG);
        vTaskDelete(NULL);
        return;
    }

    Serial.printf("[INFO][%s] ESP-NOW Service Started\n", TAG);

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gatewayPeerAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (!esp_now_is_peer_exist(gatewayPeerAddress)) {
        esp_err_t addResult = esp_now_add_peer(&peerInfo);
        if (addResult != ESP_OK) {
            Serial.printf("[ERROR][%s] Failed to add gateway peer. Error: %d\n", TAG, addResult);
        } else {
            Serial.printf("[INFO][%s] Gateway peer added\n", TAG);
        }
    }

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(1000);

    for (;;) {
        copy_VAR();
        espnow_DataSend();

        vTaskDelayUntil(&lastWakeTime, samplingInterval);
    }
}

/*
  =====================================================
  COPY VARIABLE STRUCT
  =====================================================
*/

static inline void copy_VAR() {
    {
        std::lock_guard<std::mutex> lock(wifi_mutex);
        local_InfoWifiESP = g_InfoWifiESP;
    }
    {
        std::lock_guard<std::mutex> lock(info_mutex);
        local_InfoESP = g_InfoESP;
    }
    {
        std::lock_guard<std::mutex> lock(sensor_mutex);
        local_ValueSensor = g_ValueSensor;
    }
    {
        std::lock_guard<std::mutex> lock(statusnews_mutex);
        local_StatusNews = g_StatusNews;
    }
}

/*
  =====================================================
  ESPNOW DATA SEND
  =====================================================
*/

static inline void espnow_DataSend() 
{
    esp_now_send(gatewayPeerAddress,
                 reinterpret_cast<const uint8_t*>(&local_InfoWifiESP),
                 sizeof(local_InfoWifiESP));
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_now_send(gatewayPeerAddress,
                 reinterpret_cast<const uint8_t*>(&local_InfoESP),
                 sizeof(local_InfoESP));
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_now_send(gatewayPeerAddress,
                 reinterpret_cast<const uint8_t*>(&local_ValueSensor),
                 sizeof(local_ValueSensor));
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_now_send(gatewayPeerAddress,
                 reinterpret_cast<const uint8_t*>(&local_StatusNews),
                 sizeof(local_StatusNews));
    vTaskDelay(pdMS_TO_TICKS(50));
}

/*
  =====================================================
  ESPNOW DATA RECEIVED
  =====================================================
*/

static inline void espnow_DataRecv(const uint8_t *incomingData, int len) 
{
    if (len == sizeof(g_CalibSensor)) {
        std::lock_guard<std::mutex> lock(sensor_calib_mutex);
        memcpy(&g_CalibSensor, incomingData, sizeof(g_CalibSensor));
        Serial.printf("[INFO][%s] RX Calibration Sensor ID: %s\n",
                      TAG, g_CalibSensor.id_sensor);
    }
    else if (len == sizeof(g_CmdAct)) {
        std::lock_guard<std::mutex> lock(act_mutex);
        memcpy(&g_CmdAct, incomingData, sizeof(g_CmdAct));
        Serial.printf("[INFO][%s] RX Actuator Commands ID: %s\n",
                      TAG, g_CmdAct.id_act);
    }
    else if (len == sizeof(g_CalibAct)) {
        std::lock_guard<std::mutex> lock(act_calib_mutex);
        memcpy(&g_CalibAct, incomingData, sizeof(g_CalibAct));
        Serial.printf("[INFO][%s] RX Calibration Actuators ID: %s\n",
                      TAG, g_CalibAct.id_act);
    }
    else {
        Serial.printf("[WARNING][%s] RX size mismatch: %d bytes\n",
                      TAG, len);
    }
}