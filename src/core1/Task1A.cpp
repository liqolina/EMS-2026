#include "core1Task.h"
#include "mutex_global.h"
#include "variable_global.h"


void Task1A(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(2600));


    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(1000);

    for (;;) {


        vTaskDelayUntil(&lastWakeTime, samplingInterval);
    }
}