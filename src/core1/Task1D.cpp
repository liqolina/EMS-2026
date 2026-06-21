#include "core1/core1Task.h"
#include "Globals.h"

void Task1D(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(2600));

  for (;;) {


    vTaskDelay(portMAX_DELAY); // Block indefinitely
  }
}
