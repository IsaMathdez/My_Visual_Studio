
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buoy_data.h"
#include "tds_sensor.h"
#include "ds18b20_sensor.h"

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

    ds18b20_init();

    xTaskCreate(
        ds18b20_task,
        "DS18B20",
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