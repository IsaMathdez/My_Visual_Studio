
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buoy_data.h"
#include "tds.h"

buoy_data_t buoy_data;

void app_main(void)
{
    printf("Hydroscan iniciado\n");

    tds_init();

    xTaskCreate(
        tds_task,
        "TDS",
        4096,
        NULL,
        5,
        NULL
    );

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}