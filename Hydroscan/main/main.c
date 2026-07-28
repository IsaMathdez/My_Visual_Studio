// Hydroscan main application file v4.0.4.7
// Made by Isaias Matos

// CAMBIOS v4.0.4.7
// OBJETIVO: Reportar fuera del rango, con nuevo rango 1 s - 20 s.
// Cambiado: Declarar Hz tanto en mpu6050_sensor.h como en wave_task.c 
//  frecuencia de muestreo a 5 Hz, tiempo de rafaga a 120 s.
// Modificado: 
// NOTA: Aun no funciona

// A MEJORAR EN v4.0.5
// Ajustar un poco mas el resultado de Hs
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

    printf("Sistema inicializado. v4.0.4.7\n");


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