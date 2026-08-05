// Hydroscan main application file v5.0.2
// Made by Isaias Matos

// CAMBIOS v5.0.2 Parte IV Definitiva
// OBJETIVO: Agregar los codigos de LILYGO modulo, probar GPS y LTE
// Parametros: 
//      Sensor de oleaje: frecuencia de muestreo a 5 Hz, tiempo de rafaga a 120 s. Alta resolucion.
// AGREGADO: 
//      Nuevas funciones para enviar y recibir todos los datos de un comando AT  
// ARCHIVOS MODIFICADOS: main.c, firebase.c, modem.c/.h
// RESULTADOS: 
//      EL modem ya conecta y reconoce la tarjeta SIM.
//      El GPS ya obtiene la posicion y la guarda en buoy_data.
//      Agregado sistema mutex para evitar conflictos entre tareas de gps y firebase.
//      El modem ya obtienen IP y API
//      El modem ya envia datos a Firebase.
// NOTA: Recomendable hacer mas pruebas del oleaje

// A MEJORAR EN v5.0.3 y versiones futuras
//      Reajustar ecuacion del tds sensor
//      Recibir datos de firebase
//      Definir sistema de tareas final en appmain() 
//      Probar sistema final con todos los sensores y el modem

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "buoy_data.h"
#include "tds_sensor.h"
#include "ds18b20_sensor.h"
#include "telemetry.h"
#include "wave_task.h"

#include "modem.h"
#include "gps.h"
#include "firebase.h"

static const char *TAG = "MAIN";

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

    if(modem_is_ready())
    {
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
    } else {
        ESP_LOGI(TAG, "Modem not ready");
    }

    printf("\nTodas las tareas fueron creadas correctamente.\n");

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}