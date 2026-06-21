#include "core1/core1Task.h"
#include "Globals.h"

#include "esp_log.h"
#include <inttypes.h>

constexpr const char* TAG = "TASK_1B";

// 1. Mengisi ID Perangkat
// DDMMYYMMLLSSSS: Category | Model | Year | Month | Batch | Serial
constexpr uint64_t actModuleId = 11112606010002ULL;

#define RELAY_AC_PIN 2

static float value = 0;
static float voltage = 0;

static bool commandSnapshot = false;
static bool isRelayActive = false;

static inline void updateRelayStatusStruct(bool active);

void Task1B(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(2600));

    pinMode(RELAY_AC_PIN, OUTPUT);
    digitalWrite(RELAY_AC_PIN, LOW);

    updateRelayStatusStruct(false);

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t executionInterval = pdMS_TO_TICKS(1000);

    for (;;) {    

        xSemaphoreTake(actMutex, portMAX_DELAY);
            commandSnapshot = cmdAct_A1.command;
            value = cmdAct_A1_Calib.calib_1;
        xSemaphoreGive(actMutex);

        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            voltage = sensor_A1.value_1;
        xSemaphoreGive(sensorMutex);


        if (voltage > value) {
            isRelayActive = commandSnapshot;
        } else {
            isRelayActive = false;
        }

        digitalWrite(RELAY_AC_PIN, isRelayActive ? HIGH : LOW);

        updateRelayStatusStruct(isRelayActive);

        ESP_LOGI(
            TAG,
            "Relay Status: %s [%s]",
            isRelayActive ? "ACTIVE" : "INACTIVE",
            isRelayActive ? "HIGH" : "LOW"
        );

        vTaskDelayUntil(&lastWakeTime, executionInterval);
    }
}

static inline void updateRelayStatusStruct(bool active)
{
    uint64_t deviceIdSnapshot = 0;

    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        deviceIdSnapshot = systemInfo_Client.deviceId;
    xSemaphoreGive(deviceMutex);

    xSemaphoreTake(actMutex, portMAX_DELAY);
        cmdAct_A1_Status.deviceId = deviceIdSnapshot;
        cmdAct_A1_Status.moduleId = actModuleId;

        strlcpy(
            cmdAct_A1_Status.status,
            active ? "ACTIVE" : "INACTIVE",
            sizeof(cmdAct_A1_Status.status)
        );

        strlcpy(
            cmdAct_A1_Status.news,
            active ? "RELAY NORMALY OPEN" : "RELAY NORMALY CLOSE",
            sizeof(cmdAct_A1_Status.news)
        );
    xSemaphoreGive(actMutex);
}