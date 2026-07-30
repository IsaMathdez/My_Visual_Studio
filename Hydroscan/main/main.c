// Hydroscan main application file v4.0.4.8
// Made by Isaias Matos

// CAMBIOS v4.0.4.8
// OBJETIVO: Reportar fuera del rango, con nuevo rango 1 s - 10 s. YA FUNCIONA
// Cambiado: Declarar Hz tanto en mpu6050_sensor.h como en wave_task.h
//  frecuencia de muestreo a 5 Hz, tiempo de rafaga a 120 s. Alta resolucion.
// Modificado: agregado highpass_filter(), solo por si acaso.
// NOTA: Resultados muy buenos, no se pudo ampliar rango hasta 20s

// A MEJORAR EN v4.0.5
// Ajustar un poco mas el resultado de Hs
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

    printf("Sistema inicializado. v4.0.4.8\n");


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