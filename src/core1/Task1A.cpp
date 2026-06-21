#include "core1/core1Task.h"
#include "Globals.h"

#define PZEM_017
#include <PZEMPlus.h>

#include "esp_log.h"

constexpr const char* TAG = "TASK_1A";

// 1. Mengisi ID Perangkat
// DDMMYYMMLLSSSS: Category | Model | Year | Month | Batch | Serial
constexpr uint64_t sensorModuleId = 11102606010002ULL;

#define RX_PIN_1 16
#define TX_PIN_1 17
#define PZEM_RS485_EN 22

static HardwareSerial pzemSerial(2);
static PZEMPlus pzem(pzemSerial);

static float voltage = 0.0f;
static float current = 0.0f;
static float power = 0.0f;
static float energy = 0.0f;

static char sensorStatus[STATUS_SIZE] = "INIT";

static inline void readSensorTask();
static inline void processSensorStatus();
static inline void dispatchToStruct();

void Task1A(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(2600));

    pzemSerial.begin(9600, SERIAL_8N1, RX_PIN_1, TX_PIN_1);
    pzem.begin();


    #if defined(PZEM_RS485_EN)
    pzem.setEnable(PZEM_RS485_EN);
    #endif

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(1000);

    for (;;) {
        readSensorTask();
        processSensorStatus();
        dispatchToStruct();

        vTaskDelayUntil(&lastWakeTime, samplingInterval);
    }
}

static inline void readSensorTask()
{
    voltage     = pzem.readVoltage();
    current     = pzem.readCurrent();
    power       = pzem.readPower();
    energy      = pzem.readEnergy();
}

static inline void processSensorStatus()
{
    if (isnan(voltage)) {
        strlcpy(sensorStatus, "OFFLINE", sizeof(sensorStatus));
    }
    else if (voltage > 14.0f) {
        strlcpy(sensorStatus, "High Voltage DC", sizeof(sensorStatus));
    }
    else if (voltage < 11.0f) {
        strlcpy(sensorStatus, "Low Voltage DC", sizeof(sensorStatus));
    }
    else {
        strlcpy(sensorStatus, "Normal Voltage DC", sizeof(sensorStatus));
    }
}

static inline void dispatchToStruct()
{
    uint64_t deviceIdSnapshot = 0;

    xSemaphoreTake(deviceMutex, portMAX_DELAY);
        deviceIdSnapshot = systemInfo_Client.deviceId;
    xSemaphoreGive(deviceMutex);

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        sensor_A1.deviceId = deviceIdSnapshot;
        sensor_A1.moduleId = sensorModuleId;

        sensor_A1.value_1 = voltage;
        sensor_A1.value_2 = current;
        sensor_A1.value_3 = power;

        strlcpy(sensor_A1_Status.status, sensorStatus, sizeof(sensor_A1_Status.status));
    xSemaphoreGive(sensorMutex);

    ESP_LOGD(
        TAG,
        "Sensor updated. V: %.2f, I: %.2f, P: %.2f, Status: %s",
        voltage,
        current,
        power,
        sensorStatus
    );
}