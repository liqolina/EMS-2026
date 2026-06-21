/**************************************************************************
  
  Author             : Lutfi Aundrie Hermawan
  Date               : 05/11/2025
  Version            : 13.1.0
  GitHub Author      : https://github.com/liqolina
  Penyunting/Editor  : 
  GitHub Editor      : 

**************************************************************************/

#include <Arduino.h>           
#include "core0Task.h"          
#include "core1Task.h"                         
#include "mutex_global.h"
#include "variable_global.h"

TaskHandle_t Task0A_Handle; 
TaskHandle_t Task0B_Handle; 
TaskHandle_t Task0C_Handle; 
TaskHandle_t Task0D_Handle; 
TaskHandle_t Task0E_Handle;  

TaskHandle_t Task1A_Handle;  
TaskHandle_t Task1B_Handle;
TaskHandle_t Task1C_Handle; 
TaskHandle_t Task1D_Handle;  
TaskHandle_t Task1E_Handle; 

void setup() {
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(10));

    esp_log_level_set("*", ESP_LOG_NONE);

    esp_log_level_set("TASK_0A", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_0B", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_0C", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_0D", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_0E", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_1A", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_1B", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_1C", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_1D", ESP_LOG_DEBUG);
    esp_log_level_set("TASK_1E", ESP_LOG_DEBUG);

    xTaskCreatePinnedToCore(
        Task0A,        
        "Task0A",     
        8192,      
        NULL,          
        1,        
        &Task0A_Handle, 
        0           
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task0B,         
        "Task0B",       
        8192,         
        NULL,       
        1,             
        &Task0B_Handle, 
        0             
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task0C,         
        "Task0C",       
        8192,         
        NULL,       
        1,             
        &Task0C_Handle, 
        0             
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task0D,         
        "Task0D",       
        8192,         
        NULL,       
        1,             
        &Task0D_Handle, 
        0             
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task0E,         
        "Task0E",       
        8192,         
        NULL,       
        1,             
        &Task0E_Handle, 
        0             
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task1A,         
        "Task1A",       
        8192,         
        NULL,       
        1,             
        &Task1A_Handle, 
        1              
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task1B,         
        "Task1B",       
        8192,         
        NULL,       
        1,             
        &Task1B_Handle, 
        1              
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task1C,         
        "Task1C",       
        8192,         
        NULL,       
        1,             
        &Task1C_Handle, 
        1              
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task1D,         
        "Task1D",       
        8192,         
        NULL,       
        1,             
        &Task1D_Handle, 
        1              
    );
    vTaskDelay(pdMS_TO_TICKS(1));

    xTaskCreatePinnedToCore(
        Task1E,         
        "Task1E",       
        8192,         
        NULL,       
        1,             
        &Task1E_Handle, 
        1              
    );
    vTaskDelay(pdMS_TO_TICKS(10)); 

}

void loop() {

    vTaskDelay(portMAX_DELAY);
}