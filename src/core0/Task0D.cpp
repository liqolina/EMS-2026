#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"
#include "environment.h"

#include <WiFi.h>
#include <esp_now.h>
#include "esp_log.h"
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

// constexpr const uint8_t gatewayPeerAddress[] = {
//     0x3C, 0x0F, 0x02, 0xD2, 0x1E, 0x20
// };

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
    ESP_LOGD(
        TAG,
        "Last packet send status: %s",
        status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail"
    );
}

/*
  =====================================================
  Callback penerimaan
  =====================================================
*/

static inline void onDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len)
{
    if (!incomingData || len <= 0) {
        ESP_LOGW(TAG, "Invalid ESP-NOW RX data");
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
        ESP_LOGW(TAG, "Waiting for WiFi mode before ESP-NOW init...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing ESP-NOW");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "ESP-NOW Service Started");

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
            ESP_LOGE(TAG, "Failed to add gateway peer. Error: %d", addResult);
        } else {
            ESP_LOGI(TAG, "Gateway peer added");
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
    esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&local_InfoWifiESP), sizeof(local_InfoWifiESP));
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&local_InfoESP), sizeof(local_InfoESP));
    vTaskDelay(pdMS_TO_TICKS(50));
    
    esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&local_ValueSensor), sizeof(local_ValueSensor));
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&local_StatusNews), sizeof(local_StatusNews));
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
        ESP_LOGI(TAG, "RX Calibration Sensor ID: %s", g_CalibSensor.id_sensor);
    }
    else if (len == sizeof(g_CmdAct)) {
        std::lock_guard<std::mutex> lock(act_mutex);
        memcpy(&g_CmdAct, incomingData, sizeof(g_CmdAct));
        ESP_LOGI(TAG, "RX Actuator Commands ID: %s", g_CmdAct.id_act);
    }
    else if (len == sizeof(g_CalibAct)) {
        std::lock_guard<std::mutex> lock(act_calib_mutex);
        memcpy(&g_CalibAct, incomingData, sizeof(g_CalibAct));
        ESP_LOGI(TAG, "RX Calibration Actuators ID: %s", g_CalibAct.id_act);
    }
    else {
        ESP_LOGW(TAG, "RX size mismatch: %d bytes", len);
    }
}