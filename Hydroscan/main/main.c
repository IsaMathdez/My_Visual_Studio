// Hydroscan main application file v5.0.2
// Made by Isaias Matos

// CAMBIOS v5.0.2 Parte II
// OBJETIVO: Agregar los codigos de LILYGO modulo, probar GPS y LTE
// Parametros: Sensor de oleaje: frecuencia de muestreo a 5 Hz, tiempo de rafaga a 120 s. Alta resolucion.
// AGREGADO: Firebase.c/.h,   
// Modificado: main.c, gps.c, modem.c/.h, CMakeList.txt
// RESULTADOS: 
//      EL modem ya conecta y reconoce la tarjeta SIM.
//      El GPS ya obtiene la posicion y la guarda en buoy_data.
//      Agregado sistema mutex para evitar conflictos entre tareas de gps y firebase
//      El modem ya obtienen IP y API
//      El modem aun no envia datos a Firebase.
// NOTA: Recomendable hacer mas pruebas del oleaje

// A MEJORAR EN v5.0.3
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

#include "modem.h"
#include "gps.h"
#include "firebase.h"

// ACTUALIZADO

buoy_data_t buoy_data;

void app_main(void)
{
    printf("\n");
    printf("=============================================\n");
    printf("        HYDROSCAN - BOYA OCEANOGRAFICA\n");
    printf("=============================================\n");

    printf("Sistema inicializado. v5.0.2\n");


    /*----------------------------------------------------------
                    Sensor DS18B20
    ----------------------------------------------------------*/
    /*
    ds18b20_init();

    xTaskCreate(
        ds18b20_task,
        "Temperature",
        4096,
        NULL,
        4,
        NULL); */


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

    /*
    xTaskCreate(
        wave_task,
        "Wave",
        8192,
        NULL,
        5,
        NULL); */


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

    /*----------------------------------------------------------
                    MODULO LILYGO en proceso
    ----------------------------------------------------------*/

    modem_init();

    gps_init();

    firebase_init();

    while (1)
    {
        gps_update();

        if (gps_has_fix())
        {
            firebase_send();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    printf("\nTodas las tareas fueron creadas correctamente.\n");

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}