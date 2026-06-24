#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"
#include "environment.h"
#include "ntp_time.h"

#include <WiFi.h>

/*
  =====================================================
  TAG SERIAL MONITOR
  =====================================================
*/

constexpr const char* TAG = "TASK_0B";

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

// WifiRunMode wifiRunMode = WIFI_STA_ONLY;

/*
  =====================================================
  KONFIGURASI WIFI STATION / CLIENT
  =====================================================
*/

// constexpr const char* STA_SSID     = "Berbayar";     
// constexpr const char* STA_PASSWORD = "seribu";  
// constexpr const char* DEVICE_HOSTNAME  = "ESP32-SENSOR-CLIENT-DC-1";    

// constexpr const bool USE_STATIC_IP = false;

// IPAddress STA_LOCAL_IP(192, 168, 1, 50);
// IPAddress STA_GATEWAY(192, 168, 1, 1);
// IPAddress STA_SUBNET(255, 255, 255, 0);
// IPAddress STA_DNS1(1, 1, 1, 1);
// IPAddress STA_DNS2(8, 8, 8, 8);

/*
  =====================================================
  KONFIGURASI ACCESS POINT / HOTSPOT
  =====================================================
*/

// constexpr const char* AP_SSID      = "ESP32_GATEWAY_NET";     
// constexpr const char* AP_PASSWORD  = "12345678";  

// IPAddress AP_LOCAL_IP(10, 10, 10, 1);
// IPAddress AP_GATEWAY(10, 10, 10, 1);
// IPAddress AP_SUBNET(255, 255, 255, 0);

// constexpr const uint8_t AP_CHANNEL        = 6;
// constexpr const uint8_t AP_MAX_CONNECTION = 4;
// constexpr const bool AP_HIDDEN            = false;

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
            Serial.printf("[INFO][%s] [EVENT] WiFi interface ready\n", TAG);
            break;
      
        case ARDUINO_EVENT_WIFI_STA_START:
            wifiSTA_running.store(false);
            Serial.printf("[INFO][%s] WiFi STA Started\n", TAG);
            break;

        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.printf("[INFO][%s] Connected to Router. Waiting for IP...\n", TAG);
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            wifiSTA_running.store(true);
            Serial.printf("[INFO][%s] STA Connected! IP: %s\n", TAG, WiFi.localIP().toString().c_str());
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            wifiSTA_running.store(false);
            Serial.printf("[ERROR][%s] STA Lost connection. Retrying...\n", TAG);

            // Reconnect otomatis tanpa blocking
            WiFi.begin(STA_SSID, STA_PASSWORD); 
            break;

        case ARDUINO_EVENT_WIFI_STA_STOP:
            wifiSTA_running.store(false);
            Serial.printf("[INFO][%s] [EVENT] STA stopped\n", TAG);
            break;

        case ARDUINO_EVENT_WIFI_AP_START:
            wifiAP_running.store(false);
            Serial.printf("[INFO][%s] WiFi AP Started\n", TAG);
            break;

        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            wifiAP_running.store(true);
            Serial.printf("[INFO][%s] Client connected to ESP32 AP\n", TAG);
            break;

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            wifiAP_running.store(false);
            Serial.printf("[INFO][%s] Client disconnected from ESP32 AP\n", TAG);
            break;

        case ARDUINO_EVENT_WIFI_AP_STOP:
            wifiAP_running.store(false);
            Serial.printf("[INFO][%s] [EVENT] AP stopped\n", TAG);
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
        Serial.printf("[INFO][%s] [WIFI] Hostname : %s\n", TAG, WiFi.getHostname());
        Serial.printf("[INFO][%s] [WIFI] Mode: STA ONLY\n", TAG);
    }
    else if (wifiRunMode == WIFI_AP_ONLY) {
        WiFi.mode(WIFI_AP);
        Serial.printf("[INFO][%s] [WIFI] Hostname : %s\n", TAG, WiFi.getHostname());
        Serial.printf("[INFO][%s] [WIFI] Mode: AP ONLY\n", TAG);
    }
    else if (wifiRunMode == WIFI_STA_AP) {
        WiFi.mode(WIFI_AP_STA);
        Serial.printf("[INFO][%s] [WIFI] Hostname : %s\n", TAG, WiFi.getHostname());
        Serial.printf("[INFO][%s] [WIFI] Mode: STA + AP\n", TAG);
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

        (void)configOK;
    }

    WiFi.setAutoReconnect(true);
    WiFi.begin(STA_SSID, STA_PASSWORD);
}

/*
  =====================================================
  TASK 0A
  =====================================================
*/

void Task0B(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(200));

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
    
    NTPTime::begin(
        "WIB-7",
        "pool.ntp.org",
        "time.google.com"
    );

    uint32_t lastCheckMs = millis();

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(30UL * 1000UL);

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            wifiStatus_running.store(true);
            Serial.printf("[DEBUG][%s] WiFi Status: Stable\n", TAG);
        } else {
            wifiStatus_running.store(false);
            Serial.printf("[WARNING][%s] WiFi Status: Reconnecting...\n", TAG);
        }

        if (millis() - lastCheckMs >= (1UL * 60UL * 1000UL)) {
            lastCheckMs = millis();

            if (wifiSTA_running.load()) {
                printSTA();
                saveWifi();
            }

            if (wifiAP_running.load()) {
                printAP();
                saveWifi();
            }

            // Get Time
            String t = NTPTime::getDateTimeString();
            Serial.printf("[INFO][%s] Time : %s\n", TAG, t.c_str());
        }

        vTaskDelayUntil(&lastWakeTime, samplingInterval);
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
    Serial.printf("[INFO][%s] ======== WIFI STATION INFO ========\n", TAG);
    Serial.printf("[INFO][%s] STA SSID     : %s\n", TAG, WiFi.SSID().c_str());
    Serial.printf("[INFO][%s] STA IP       : %s\n", TAG, WiFi.localIP().toString().c_str());
    Serial.printf("[INFO][%s] STA MAC      : %s\n", TAG, WiFi.macAddress().c_str());
    Serial.printf("[INFO][%s] STA RSSI     : %d dBm\n", TAG, WiFi.RSSI());
    Serial.printf("[INFO][%s] ===================================\n", TAG);
}

static inline void printAP() {
    Serial.printf("[INFO][%s] ======== WIFI ACCESS POINT INFO ========\n", TAG);
    Serial.printf("[INFO][%s] AP SSID      : %s\n", TAG, WiFi.softAPSSID().c_str());
    Serial.printf("[INFO][%s] AP IP        : %s\n", TAG, WiFi.softAPIP().toString().c_str());
    Serial.printf("[INFO][%s] AP MAC       : %s\n", TAG, WiFi.softAPmacAddress().c_str());
    Serial.printf("[INFO][%s] AP Clients   : %d\n", TAG, WiFi.softAPgetStationNum());
    Serial.printf("[INFO][%s] ===================================\n", TAG);
}