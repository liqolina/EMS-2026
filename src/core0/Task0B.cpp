#include "core0/core0Task.h"
#include "Globals.h"

// Konstanta menggunakan UPPER_SNAKE_CASE
constexpr const char* TAG = "TASK_0B";

// 1. Mengisi ID Perangkat
// DDMMYYMMLLSSSS: Category | Model | Year | Month | Batch | Serial
constexpr uint64_t DEFAULT_DEVICE_ID = 10102606010002ULL; 

// 2. Mengisi Nama dan Lokasi (Standard string copying)
constexpr const char* DEFAULT_NAME    = "ESP32-SENSOR-CLIENT-DC-1";
constexpr const char* DEFAULT_LOC     = "BANDUNG";


/**
 * Task untuk mengelola identitas dan metadata perangkat.
 */
void Task0B(void *pvParameters) {
    // Memberikan waktu bagi sistem untuk stabil
    vTaskDelay(pdMS_TO_TICKS(600));

    // Menunggu koneksi WiFi agar bisa mengambil IP Address
    ESP_LOGI(TAG, "Waiting for network to capture IP...");
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Initializing Device Metadata...");

    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        systemInfo_Client.deviceId = DEFAULT_DEVICE_ID; 

        strlcpy(systemInfo_Client.deviceName, DEFAULT_NAME, sizeof(systemInfo_Client.deviceName));
        strlcpy(systemInfo_Client.location, DEFAULT_LOC, sizeof(systemInfo_Client.location));

        IPAddress localIp = WiFi.localIP();
        strlcpy(systemInfo_Client.ipAddress, localIp.toString().c_str(), sizeof(systemInfo_Client.ipAddress));
        strlcpy(systemInfo_Client.macAddress, WiFi.macAddress().c_str(), sizeof(systemInfo_Client.macAddress));

        strcpy(systemStatus_Client.status, "Booting");
        strcpy(systemStatus_Client.news, "Starting");

        systemInfo_Client.timestamps = millis() / 1000;

    xSemaphoreGive(deviceMutex);
    
    
    // Log hasil inisialisasi
    ESP_LOGI(TAG, "Device Identity Established:");
    ESP_LOGI(TAG, " > ID       : %llu", systemInfo_Client.deviceId); // Gunakan %llu untuk uint64_t
    ESP_LOGI(TAG, " > Name     : %s", systemInfo_Client.deviceName);
    ESP_LOGI(TAG, " > Location : %s", systemInfo_Client.location);
    ESP_LOGI(TAG, " > IP Addr  : %s", systemInfo_Client.ipAddress);
    ESP_LOGI(TAG, " > MAC Addr : %s", systemInfo_Client.macAddress);


    // Task ini selesai melakukan setup, masuk ke mode suspend/wait
    for (;;) {
        vTaskDelay(portMAX_DELAY); 
    }
}