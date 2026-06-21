#include "core0/core0Task.h"
#include "Globals.h"

// Penggunaan PascalCase untuk TAG (Konstanta Log)
constexpr const char* TAG = "TASK_0A";

// camelCase untuk variabel konfigurasi
constexpr const char* staSsid     = "ESP32_GATEWAY_NET";     
constexpr const char* staPassword = "12345678";  
constexpr const char* deviceName  = "ESP32-SENSOR-CLIENT-DC-1";    

// Penamaan IP menggunakan prefix yang jelas (sta vs ap)
IPAddress staLocalIp(192, 168, 1, 50);
IPAddress staGateway(192, 168, 1, 1);
IPAddress staSubnet(255, 255, 255, 0);
IPAddress dnsPrimary(1, 1, 1, 1);
IPAddress dnsSecondary(8, 8, 8, 8);

constexpr const char* apSsid      = "ESP32_GATEWAY_NET";     
constexpr const char* apPassword  = "12345678";  

IPAddress apIp(10, 10, 10, 1);
IPAddress apGateway(10, 10, 10, 1);
IPAddress apSubnet(255, 255, 255, 0);

// Deklarasi fungsi internal dengan camelCase
static inline void logWifiDetailsSTA();
static inline void logWifiDetailsIP();

/**
 * Handler Event WiFi menggunakan PascalCase pada switch-case 
 * untuk mencocokkan standar Enum Arduino/ESP-IDF.
 */
void onWifiEvent(WiFiEvent_t event) {
    switch(event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            ESP_LOGI(TAG, "WiFi Station Started");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to Router. Waiting for IP...");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG, "STA Connected! IP: %s", WiFi.localIP().toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            ESP_LOGE(TAG, "STA Lost connection. Retrying...");
            // Reconnect otomatis tanpa blocking
            WiFi.begin(staSsid, staPassword); 
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            ESP_LOGI(TAG, "Client connected to ESP32 AP");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "Client disconnected from ESP32 AP");
            break;
        default: 
            break;
    }
}

/**
 * Task Utama untuk Manajemen WiFi
 */
void Task0A(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));

    // Standar API: Daftarkan callback sebelum memulai service
    WiFi.onEvent(onWifiEvent);    
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(deviceName);
    WiFi.setSleep(false);

    // Konfigurasi Station (STA)
    // Jika ingin menggunakan IP Statis, uncomment blok di bawah:
    /*
    if (!WiFi.config(staLocalIp, staGateway, staSubnet, dnsPrimary, dnsSecondary)) {
        ESP_LOGE(TAG, "STA Static IP Failed to Configure");
    }
    */
    
    // Konfigurasi Access Point (AP)
    // WiFi.softAPConfig(apIp, apGateway, apSubnet);
    // WiFi.softAP(apSsid, apPassword, 1, 0, 4); 

    // Konfigurasi (STA)
    WiFi.begin(staSsid, staPassword);

    logWifiDetailsSTA();
    logWifiDetailsIP();

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            ESP_LOGD(TAG, "WiFi Status: Stable");
        } else {
            ESP_LOGW(TAG, "WiFi Status: Reconnecting...");
        }

        // Delay panjang (20 detik) untuk monitoring status
        vTaskDelay(pdMS_TO_TICKS(20000)); 
    }
}

/**
 * Logging informasi jaringan ke Serial Monitor
 */
static inline void logWifiDetailsSTA() {
    ESP_LOGI(TAG, "--- WIFI STATION INFO ---");
    ESP_LOGI(TAG, "SSID       : %s", WiFi.SSID().c_str());
    ESP_LOGI(TAG, "IP Address : %s", WiFi.localIP().toString().c_str());
    ESP_LOGI(TAG, "RSSI       : %d dBm", WiFi.RSSI());
    ESP_LOGI(TAG, "Hostname   : %s", WiFi.getHostname());
    ESP_LOGI(TAG, "-------------------------");
}

static inline void logWifiDetailsIP() {
    // ESP_LOGI(TAG, "--- WIFI ACCESS POINT INFO ---");
    // ESP_LOGI(TAG, "AP SSID    : %s", apSsid);
    // ESP_LOGI(TAG, "AP IP      : %s", WiFi.softAPIP().toString().c_str());
    // ESP_LOGI(TAG, "------------------------------");
}