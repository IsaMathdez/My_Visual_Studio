// Isaias Matos
// FSM Porton Electrico version 1.0
// main.c


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    while (1) {
        printf("HOLA!\n\n");
        printf("ADIOS!\n\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
