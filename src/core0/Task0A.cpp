#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"
#include "environment.h"

/*
  =====================================================
  TAG SERIAL MONITOR
  =====================================================
*/

constexpr const char* TAG = "TASK_0A";

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

void Task0A(void *pvParameters) {
    // Memberikan waktu bagi sistem untuk stabil
    vTaskDelay(pdMS_TO_TICKS(100));

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

    snprintf(g_InfoESP.id_esp, ID_SIZE, "%s", DEFAULT_ID_ESP);
    snprintf(g_InfoESP.name_esp, NAME_ID_SIZE, "%s", DEFAULT_NAME_ESP);
    snprintf(g_InfoESP.loc_esp, LOC_SIZE, "%s", DEFAULT_LOC_ESP);
}

/*
  =====================================================
  PRINT INFORMASI ESP
  =====================================================
*/

static inline void printESP() {
    Serial.printf("============== ESP INFO ==============\n");
    Serial.printf("ID ESP           : %s\n", DEFAULT_ID_ESP);
    Serial.printf("NAME ESP         : %s\n", DEFAULT_NAME_ESP);
    Serial.printf("LOCATION ESP     : %s\n", DEFAULT_LOC_ESP);
    Serial.printf("======================================\n");
}