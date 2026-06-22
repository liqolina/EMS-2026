#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"
#include "environment.h"

/*
  =====================================================
  TAG SERIAL MONITOR
  =====================================================
*/

constexpr const char* TAG = "TASK_0B";

/*
  =====================================================
  STRUCT INFO ESP
  =====================================================
*/

extern InfoESP g_InfoESP;

/*
  =====================================================
  INFORMASI ESP32
  =====================================================
*/

// Format DEFAULT_ID_ESP 
// DDMMYYMMLLSSSS: Category | Model | Year | Month | Batch | Serial

// constexpr const char* DEFAULT_ID_ESP    = "10102606010002"; 

// constexpr const char* DEFAULT_NAME_ESP  = "ESP32-SENSOR-CLIENT-DC-1";
// constexpr const char* DEFAULT_LOC_ESP   = "BANDUNG";


/*
  =====================================================
  DEKLARASI VOID
  =====================================================
*/

static inline void saveESP();
static inline void printESP();

/*
  =====================================================
  TASK 0B
  =====================================================
*/
void Task0B(void *pvParameters) {
    // Memberikan waktu bagi sistem untuk stabil
    vTaskDelay(pdMS_TO_TICKS(200));

    printESP();
    saveESP();

    // Task ini selesai melakukan setup, masuk ke mode suspend/wait
    for (;;) {
        vTaskDelay(portMAX_DELAY); 
    }
}

/*
  =====================================================
  SIMPAN INFORMASI ESP
  =====================================================
*/
static inline void saveESP() {
    std::lock_guard<std::mutex> lock(info_mutex);

    snprintf(g_InfoESP.id_esp, IP_SIZE, "%s", DEFAULT_ID_ESP);
    snprintf(g_InfoESP.loc_esp, MAC_SIZE, "%s", DEFAULT_NAME_ESP);
    snprintf(g_InfoESP.name_esp, IP_SIZE, "%s", DEFAULT_LOC_ESP);
}

/*
  =====================================================
  PRINT INFORMASI ESP
  =====================================================
*/

static inline void printESP() {
    ESP_LOGI(TAG, "============== ESP INFO ==============");
    ESP_LOGI(TAG, "ID ESP           : %s", DEFAULT_ID_ESP);
    ESP_LOGI(TAG, "NAME ESP         : %s", DEFAULT_NAME_ESP);
    ESP_LOGI(TAG, "LOCATION ESP     : %s", DEFAULT_LOC_ESP);
    ESP_LOGI(TAG, "======================================");
}