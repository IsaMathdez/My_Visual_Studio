// Hydroscan main application file v5.0.1
// Made by Isaias Matos

// CAMBIOS v5.0.1
// OBJETIVO: Correr codigo actual v4.0.4.8 en el nuevo modulo. YA FUNCIONA
// Parametros: Declarado Hz tanto en mpu6050_sensor.h como en wave_task.h
//  frecuencia de muestreo a 5 Hz, tiempo de rafaga a 120 s. Alta resolucion.
// Modificado: asignado pines, board.h, set esp32s3.
// NOTA: Recomendable hacer mas pruebas del oleaje

// A MEJORAR EN v5.0.2
// Reajustar ecuacion del tds sensor
// Integrar codigo de GPS
// Integrar codigo de comunicacion LTE

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

    printf("Sistema inicializado. v5.0.1\n");


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