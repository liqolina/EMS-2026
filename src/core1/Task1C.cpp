#include "core1/core1Task.h"
#include "Globals.h"

#include "esp_log.h"
#include <inttypes.h>
#include <math.h>       // isnan

constexpr const char* TAG = "TASK_1C";

// 1. Mengisi ID Perangkat
// DDMMYYMMLLSSSS: Category | Model | Year | Month | Batch | Serial     
constexpr const uint64_t sensorModuleId = 11112606010001ULL;
constexpr const char* SENSOR_ID = "TEMP_S1";

constexpr const uint8_t REQUIRED_SAMPLES = 5;
constexpr const bool useMedian = true;
constexpr const float gain = 1.00f;
constexpr const float offset = 0.00f;

static char sensorStatus[STATUS_SIZE] = "INIT";

SensorProcessor lm35;

#define ADC_VREF_mV 3300.0 // ESP32 ADC reference voltage in millivolts
#define ADC_RESOLUTION 4096.0 // 12-bit ADC resolution
constexpr const int lm35Pin = 34; // Pin connected to LM35 OUT (use GPIO 33)

static float temp = 0;
static float temp_Filtering = 0;  // rename dari temp_Filering

static inline void readSensorTask();
static inline void processSensorStatus();
static inline void dispatchToStruct();

void Task1C(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2600));

    pinMode(lm35Pin, INPUT);
    analogReadResolution(12);

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t executionInterval = pdMS_TO_TICKS(1000);

    for (;;) {
        readSensorTask();
        vTaskDelayUntil(&lastWakeTime, executionInterval);
    }
}

static inline void readSensorTask()
{
    // 1. Read the raw value from the sensor
    int adcVal = analogRead(lm35Pin);
    
    // 2. Convert ADC value to voltage in millivolts
    float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
    
    // 3. Convert millivolts to Celsius (LM35 outputs 10mV per degree C)
    float tempC = milliVolt / 10.0;

    bool enoughSamples = lm35.addSample(SENSOR_ID, REQUIRED_SAMPLES, tempC);

    if (enoughSamples) {
        // Jika sampel sudah cukup, proses data
        lm35.processData(SENSOR_ID, REQUIRED_SAMPLES, useMedian);

        // Kalibrasi hasil filter
        lm35.calibrate(SENSOR_ID, gain, offset);

        // Ambil output akhir
        temp_Filtering = lm35.getOutput(SENSOR_ID);

        // Proses status sebelum dispatch
        processSensorStatus();
        dispatchToStruct();

        ESP_LOGD(
            TAG,
            "Sensor updated raw. Temperature: %.2f C",
            tempC
        );

        ESP_LOGD(
            TAG,
            "Sensor updated filtered. Sensor: %.2f C",
            temp_Filtering
        );
    }
}

static inline void processSensorStatus()
{
    if (isnan(temp_Filtering)) {
        strlcpy(sensorStatus, "OFFLINE", sizeof(sensorStatus));
    }
    else if (temp_Filtering > 35.0f) {
        strlcpy(sensorStatus, "High Temperature Battery", sizeof(sensorStatus));
    }
    else if (temp_Filtering < 15.0f) {
        strlcpy(sensorStatus, "Low Temperature Battery", sizeof(sensorStatus));
    }
    else {
        strlcpy(sensorStatus, "Normal Temperature Battery", sizeof(sensorStatus));
    }
}

static inline void dispatchToStruct()
{
    uint64_t deviceIdSnapshot = 0;

    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        deviceIdSnapshot = systemInfo_Client.deviceId;
    xSemaphoreGive(deviceMutex);

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        sensor_B1.deviceId = deviceIdSnapshot;
        sensor_B1.moduleId = sensorModuleId;

        sensor_B1.value_1 = temp_Filtering;

        strlcpy(sensor_B1_Status.status, sensorStatus, sizeof(sensor_B1_Status.status));
    xSemaphoreGive(sensorMutex);
}