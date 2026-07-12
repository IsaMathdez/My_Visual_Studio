
// Codigo Sensor de Temperatura DS18B20

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Definimos una etiqueta para los logs
static const char *TAG = "MAIN";

void app_main(void)
{
    // Opción 1: Usando el sistema de logs de ESP-IDF (Recomendado)
    ESP_LOGI(TAG, "¡Hola Mundo desde ESP-IDF con ESP_LOGI!");

    // Opción 2: Usando el printf clásico de C
    printf("¡Hola Mundo desde ESP-IDF con printf!\n");

    // Bucle infinito para que el programa no termine abruptamente
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1 segundo
    }
}