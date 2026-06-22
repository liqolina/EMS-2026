#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"

#include <WiFi.h>


/*
  =====================================================
  TAG SERIAL MONITOR
  =====================================================
*/

constexpr const char* TAG = "TASK_0A";

/*
  =====================================================
  STRUCT INFO WIFI
  =====================================================
*/

extern InfoWifiESP g_InfoWifiESP;

/*
  =====================================================
  MODE WIFI
  =====================================================
  WIFI_STA_ONLY = ESP32 konek ke router / WiFi utama
  WIFI_AP_ONLY  = ESP32 membuat hotspot sendiri
  WIFI_STA_AP   = ESP32 konek ke router + membuat hotspot
*/

enum WifiRunMode {
  WIFI_STA_ONLY,
  WIFI_AP_ONLY,
  WIFI_STA_AP
};

WifiRunMode wifiRunMode = WIFI_STA_ONLY;

/*
  =====================================================
  KONFIGURASI WIFI STATION / CLIENT
  =====================================================
*/

constexpr const char* STA_SSID     = "Berbayar";     
constexpr const char* STA_PASSWORD = "seribu1gb";  
constexpr const char* DEVICE_HOSTNAME  = "ESP32-SENSOR-CLIENT-DC-1";    

constexpr const bool USE_STATIC_IP = false;

IPAddress STA_LOCAL_IP(192, 168, 1, 50);
IPAddress STA_GATEWAY(192, 168, 1, 1);
IPAddress STA_SUBNET(255, 255, 255, 0);
IPAddress STA_DNS1(1, 1, 1, 1);
IPAddress STA_DNS2(8, 8, 8, 8);

/*
  =====================================================
  KONFIGURASI ACCESS POINT / HOTSPOT
  =====================================================
*/

constexpr const char* AP_SSID      = "ESP32_GATEWAY_NET";     
constexpr const char* AP_PASSWORD  = "12345678";  

IPAddress AP_LOCAL_IP(10, 10, 10, 1);
IPAddress AP_GATEWAY(10, 10, 10, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

constexpr const uint8_t AP_CHANNEL        = 6;
constexpr const uint8_t AP_MAX_CONNECTION = 4;
constexpr const bool AP_HIDDEN            = false;

/*
  =====================================================
  DEKLARASI VOID
  =====================================================
*/

static inline void saveWifi();
static inline void printSTA();
static inline void printAP();

/*
  =====================================================
  WIFI EVENT CALLBACK
  WiFi.onEvent(onWifiEvent);
  =====================================================
*/

void onWifiEvent(WiFiEvent_t event) {
    switch(event) {
        case ARDUINO_EVENT_WIFI_READY:
            ESP_LOGI(TAG, "[EVENT] WiFi interface ready");
            break;
      
        case ARDUINO_EVENT_WIFI_STA_START:
            wifiSTA_running.store(false);
            ESP_LOGI(TAG, "WiFi STA Started");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to Router. Waiting for IP...");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            wifiSTA_running.store(true);
            ESP_LOGI(TAG, "STA Connected! IP: %s", WiFi.localIP().toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            wifiSTA_running.store(false);
            ESP_LOGE(TAG, "STA Lost connection. Retrying...");
            // Reconnect otomatis tanpa blocking
            WiFi.begin(STA_SSID, AP_PASSWORD); 
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            wifiSTA_running.store(false);
            ESP_LOGI(TAG, "[EVENT] STA stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            wifiAP_running.store(false);
            ESP_LOGI(TAG, "WiFi AP Started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            wifiAP_running.store(true);
            ESP_LOGI(TAG, "Client connected to ESP32 AP");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            wifiAP_running.store(false);
            ESP_LOGI(TAG, "Client disconnected from ESP32 AP");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            wifiAP_running.store(false);
            ESP_LOGI(TAG, "[EVENT] AP stopped");
            break;
        default: 
            break;
    }
}

/*
  =====================================================
  SETUP MODE WIFI
  =====================================================
*/

static inline void setupWifiMode() {
    WiFi.persistent(false);
    WiFi.onEvent(onWifiEvent);   

    /*
    Catatan:
    setHostname() sebaiknya dipanggil sebelum WiFi.mode(),
    WiFi.begin(), atau WiFi.softAP().
    */
    WiFi.setHostname(DEVICE_HOSTNAME);
    WiFi.setSleep(false);

    if (wifiRunMode == WIFI_STA_ONLY) {
        WiFi.mode(WIFI_STA);
        ESP_LOGI(TAG, "[WIFI] Hostname : %s", WiFi.getHostname());
        ESP_LOGI(TAG, "[WIFI] Mode: STA ONLY");
    }
    else if (wifiRunMode == WIFI_AP_ONLY) {
        WiFi.mode(WIFI_AP);
        ESP_LOGI(TAG, "[WIFI] Hostname : %s", WiFi.getHostname());
        ESP_LOGI(TAG, "[WIFI] Mode: AP ONLY");
    }
    else if (wifiRunMode == WIFI_STA_AP) {
        WiFi.mode(WIFI_AP_STA);
        ESP_LOGI(TAG, "[WIFI] Hostname : %s", WiFi.getHostname());
        ESP_LOGI(TAG, "[WIFI] Mode: STA + AP");
    }
}

/*
  =====================================================
  START ACCESS POINT
  =====================================================
*/

static inline void startAP() {
    WiFi.softAPConfig(
        AP_LOCAL_IP,
        AP_GATEWAY,
        AP_SUBNET
    );
  
    WiFi.softAP(
        AP_SSID,
        AP_PASSWORD,
        AP_CHANNEL,
        AP_HIDDEN,
        AP_MAX_CONNECTION
    );
}

/*
  =====================================================
  CONNECT WIFI STATION
  =====================================================
*/

static inline void startSTA() {
    if (USE_STATIC_IP) {
        bool configOK = WiFi.config(
            STA_LOCAL_IP,
            STA_GATEWAY,
            STA_SUBNET,
            STA_DNS1,
            STA_DNS2
        );
    }

    WiFi.setAutoReconnect(true);
    WiFi.begin(STA_SSID, STA_PASSWORD);
}

/*
  =====================================================
  TASK 0A
  =====================================================
*/
void Task0A(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));

    setupWifiMode();


    if (wifiRunMode == WIFI_STA_ONLY) {
        startSTA(); 
    }
    else if (wifiRunMode == WIFI_AP_ONLY) {
        startAP();
    }
    else if (wifiRunMode == WIFI_STA_AP) {
        startAP();
        startSTA();
    }
    

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            wifiStatus_running.store(true);

            if(wifiSTA_running.load()) {
                printSTA();
                saveWifi();
            }
            if(wifiAP_running.load()) {
                printAP();
                saveWifi();
            }

            ESP_LOGD(TAG, "WiFi Status: Stable");
        } else {
            wifiStatus_running.store(false);

            ESP_LOGW(TAG, "WiFi Status: Reconnecting...");
        }

        // Delay panjang (20 detik) untuk monitoring status
        vTaskDelay(pdMS_TO_TICKS(20000)); 
    }
}

/*
  =====================================================
  SIMPAN INFORMASI WIFI
  =====================================================
*/
static inline void saveWifi() {
    std::lock_guard<std::mutex> lock(wifi_mutex);

    if (wifiRunMode == WIFI_STA_ONLY) {
        snprintf(g_InfoWifiESP.ip_sta, IP_SIZE, "%s", WiFi.localIP().toString().c_str());
        snprintf(g_InfoWifiESP.mac_sta, MAC_SIZE, "%s", WiFi.macAddress().c_str());
        snprintf(g_InfoWifiESP.ip_ap, IP_SIZE, "%s", "NOT MODE");
        snprintf(g_InfoWifiESP.mac_ap, MAC_SIZE, "%s", "NOT MODE");
    }
    else if (wifiRunMode == WIFI_AP_ONLY) {
        snprintf(g_InfoWifiESP.ip_sta, IP_SIZE, "%s", "NOT MODE");
        snprintf(g_InfoWifiESP.mac_sta, MAC_SIZE, "%s", "NOT MODE");
        snprintf(g_InfoWifiESP.ip_ap, IP_SIZE, "%s", WiFi.softAPIP().toString().c_str());
        snprintf(g_InfoWifiESP.mac_ap, MAC_SIZE, "%s", WiFi.softAPmacAddress().c_str());
    }
    else if (wifiRunMode == WIFI_STA_AP) {
        snprintf(g_InfoWifiESP.ip_sta, IP_SIZE, "%s", WiFi.localIP().toString().c_str());
        snprintf(g_InfoWifiESP.mac_sta, MAC_SIZE, "%s", WiFi.macAddress().c_str());
        snprintf(g_InfoWifiESP.ip_ap, IP_SIZE, "%s", WiFi.softAPIP().toString().c_str());
        snprintf(g_InfoWifiESP.mac_ap, MAC_SIZE, "%s", WiFi.softAPmacAddress().c_str());
    }

    
}

/*
  =====================================================
  PRINT INFORMASI WIFI
  =====================================================
*/

static inline void printSTA() {
    ESP_LOGI(TAG, "======== WIFI STATION INFO ========");
    ESP_LOGI(TAG, "STA SSID     : %s", WiFi.SSID().c_str());
    ESP_LOGI(TAG, "STA IP       : %s", WiFi.localIP().toString().c_str());
    ESP_LOGI(TAG, "STA MAC      : %s", WiFi.macAddress().c_str());
    ESP_LOGI(TAG, "STA RSSI     : %d dBm", WiFi.RSSI());
    ESP_LOGI(TAG, "===================================");
}

static inline void printAP() {
    ESP_LOGI(TAG, "======== WIFI ACCESS POINT INFO ========");
    ESP_LOGI(TAG, "AP SSID      : %s", WiFi.softAPSSID().c_str());
    ESP_LOGI(TAG, "AP IP        : %s", WiFi.softAPIP().toString().c_str());
    ESP_LOGI(TAG, "AP MAC       : %s", WiFi.softAPmacAddress().c_str());
    ESP_LOGI(TAG, "AP Clients   : %s", WiFi.softAPgetStationNum());
    ESP_LOGI(TAG, "===================================");
}