#include "core0/core0Task.h"
#include "Globals.h"

#include <WiFi.h>
#include <esp_now.h>
#include "esp_log.h"
#include <inttypes.h>
#include <string.h>

constexpr const char* TAG = "TASK_0C";

constexpr const uint8_t gatewayPeerAddress[] = {
    0x3C, 0x0F, 0x02, 0xD2, 0x1E, 0x20
};

static inline void espnow_DataSend();
static inline void espnow_DataRecv(const uint8_t *incomingData, int len);

// ------------------------
// Fungsi salin payload TX
// ------------------------
static void copyTxPayloadSafely()
{
    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        systemInfo_Client.timestamps = millis();
    xSemaphoreGive(deviceMutex);
}

// ------------------------
// Callback pengiriman
// ------------------------
static inline void onDataSent(const esp_now_send_info_t *sendInfo, esp_now_send_status_t status)
{
    ESP_LOGD(
        TAG,
        "Last packet send status: %s",
        status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail"
    );
}

// ------------------------
// Callback penerimaan
// ------------------------
static inline void onDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len)
{
    if (!incomingData || len <= 0) {
        ESP_LOGW(TAG, "Invalid ESP-NOW RX data");
        return;
    }

    espnow_DataRecv(incomingData, len);
}

// ------------------------
// Task utama
// ------------------------
void Task0C(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(1200));

    while (WiFi.getMode() == WIFI_MODE_NULL) {
        ESP_LOGW(TAG, "Waiting for WiFi mode before ESP-NOW init...");
        vTaskDelay(pdMS_TO_TICKS(200));
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
    const TickType_t sendInterval = pdMS_TO_TICKS(1000);

    for (;;) {
        copyTxPayloadSafely();
        espnow_DataSend();

        vTaskDelayUntil(&lastWakeTime, sendInterval);
    }
}

static inline void espnow_DataSend() 
{
    // Kirim masing-masing struct secara terpisah
    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&systemInfo_Client), sizeof(systemInfo_Client));
    xSemaphoreGive(deviceMutex);
    vTaskDelay(pdMS_TO_TICKS(10));

    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&systemStatus_Client), sizeof(systemStatus_Client));
    xSemaphoreGive(deviceMutex);
    vTaskDelay(pdMS_TO_TICKS(10));

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&sensor_A1), sizeof(sensor_A1));
    xSemaphoreGive(sensorMutex);
    vTaskDelay(pdMS_TO_TICKS(10));

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&sensor_A1_Status), sizeof(sensor_A1_Status));
    xSemaphoreGive(sensorMutex);
    vTaskDelay(pdMS_TO_TICKS(10));

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&sensor_B1), sizeof(sensor_B1));
    xSemaphoreGive(sensorMutex);
    vTaskDelay(pdMS_TO_TICKS(10));

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&sensor_B1_Status), sizeof(sensor_B1_Status));
    xSemaphoreGive(sensorMutex);
    vTaskDelay(pdMS_TO_TICKS(10));

    xSemaphoreTake(actMutex, portMAX_DELAY);
        esp_now_send(gatewayPeerAddress, reinterpret_cast<const uint8_t*>(&cmdAct_A1_Status), sizeof(cmdAct_A1_Status));
    xSemaphoreGive(actMutex);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static inline void espnow_DataRecv(const uint8_t *incomingData, int len) 
{
    if (len == sizeof(DeviceInfo)) {
        xSemaphoreTake(deviceMutex, portMAX_DELAY);
        memcpy(&systemInfo_Server, incomingData, sizeof(DeviceInfo));
        xSemaphoreGive(deviceMutex);
        ESP_LOGI(TAG, "RX HOST SYSTEM INFO: %s", systemInfo_Server.deviceName);
    }
    else if (len == sizeof(CommandAct)) {
        xSemaphoreTake(actMutex, portMAX_DELAY);
        memcpy(&cmdAct_A1, incomingData, sizeof(CommandAct));
        xSemaphoreGive(actMutex);
        ESP_LOGI(TAG, "RX CommandAct: %s", cmdAct_A1.command ? "ON" : "OFF");
    }
    else if (len == sizeof(CalibrateAct)) {
        xSemaphoreTake(actMutex, portMAX_DELAY);
        memcpy(&cmdAct_A1_Calib, incomingData, sizeof(CalibrateAct));
        xSemaphoreGive(actMutex);
        ESP_LOGI(TAG, "RX CalibrateAct");
    }
    else {
        ESP_LOGW(TAG, "RX size mismatch: %d bytes", len);
    }
}