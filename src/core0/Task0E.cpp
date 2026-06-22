#include "core1Task.h"
#include "mutex_global.h"
#include "variable_global.h"


void Task0E(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(15000));


    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(1000);

    for (;;) {


        vTaskDelayUntil(&lastWakeTime, samplingInterval);
    }
}