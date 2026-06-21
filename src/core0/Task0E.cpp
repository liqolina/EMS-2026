#include "core0/core0Task.h"
#include "Globals.h"

void Task0E(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2100));

    for (;;) {


        vTaskDelay(portMAX_DELAY); 
    }
}
