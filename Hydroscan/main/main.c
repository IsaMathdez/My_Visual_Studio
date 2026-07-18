// Hydroscan main application file v4.0.2
// Made by Isaias Matos

// CAMBIOS v4.0.2
// PSD corregida, Compensación Hann, Hs físicamente correcto
// Modificado Spectrum.c

// A MEJORAR EN V4.0.3
// Implementar Welch real
// Arreglar Telemetry
// Reajustar ecuacion del tds sensor

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buoy_data.h"
#include "tds_sensor.h"
#include "ds18b20_sensor.h"
#include "telemetry.h"
#include "wave_task.h"
// ACTUALIZADO

buoy_data_t buoy_data;

void app_main(void)
{
    printf("\n");
    printf("=============================================\n");
    printf("        HYDROSCAN - BOYA OCEANOGRAFICA\n");
    printf("=============================================\n");

    printf("Sistema inicializado.\n");


    /*----------------------------------------------------------
                    Sensor DS18B20
    ----------------------------------------------------------*/

    ds18b20_init();

    xTaskCreate(
        ds18b20_task,
        "Temperature",
        4096,
        NULL,
        4,
        NULL);


    /*----------------------------------------------------------
                    Sensor TDS
    ----------------------------------------------------------*/

    tds_init();

    xTaskCreate(
        tds_task,
        "TDS",
        4096,
        NULL,
        4,
        NULL);


    /*----------------------------------------------------------
                    Sensor de Oleaje
    ----------------------------------------------------------*/

    //wave_initialize();

    xTaskCreate(
        wave_task,
        "Wave",
        8192,
        NULL,
        5,
        NULL);


    /*----------------------------------------------------------
                    Telemetría
    ----------------------------------------------------------*/

    telemetry_init();

    xTaskCreate(
        telemetry_task,
        "Telemetry",
        4096,
        NULL,
        2,
        NULL);


    printf("Todas las tareas fueron creadas correctamente.\n");

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}