
/*
 * ============================================================
 *                      HYDROSCAN
 * ------------------------------------------------------------
 * Archivo      : telemetry.c
 * Descripción  : Telemetría general de la boya
 * ============================================================
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "buoy_data.h"
#include "telemetry.h"

/*==============================================================
                        CONFIGURACIÓN
==============================================================*/

#define TELEMETRY_PERIOD_MS     3000

/*==============================================================
                        VARIABLES
==============================================================*/

static const char *TAG = "TELEMETRY";

/*==============================================================
                FUNCIONES PRIVADAS
==============================================================*/

static const char *status_string(bool valid)
{
    return valid ? "OK" : "ERROR";
}

/*==============================================================
                    INICIALIZACIÓN
==============================================================*/

void telemetry_init(void)
{
    ESP_LOGI(TAG, "Telemetría inicializada.");
}

/*==============================================================
                        TAREA
==============================================================*/

void telemetry_task(void *pvParameters)
{
    while (1)
    {
        /*------------------------------------------------------
            Copia local de la estructura
        ------------------------------------------------------*/

        float temperature = buoy_data.temperature.value;
        bool temp_ok = buoy_data.temperature.valid;
        uint32_t temp_age =
            xTaskGetTickCount() -
            buoy_data.temperature.last_update_ms;

        float tds = buoy_data.tds.value;
        bool tds_ok = buoy_data.tds.valid;
        uint32_t tds_age =
            xTaskGetTickCount() -
            buoy_data.tds.last_update_ms;

        float salinity = buoy_data.salinity.value;
        bool sal_ok = buoy_data.salinity.valid;

        float hs = buoy_data.wave_height.value;
        bool hs_ok = buoy_data.wave_height.valid;

        float tp = buoy_data.wave_period.value;
        bool tp_ok = buoy_data.wave_period.valid;

        printf("\n");
        printf("=====================================================\n");
        printf("               HYDROSCAN TELEMETRY\n");
        printf("=====================================================\n\n");

        /*---------------- TEMPERATURA ----------------*/

        printf("[ TEMPERATURA ]\n");

        printf("Estado      : %s\n",
               status_string(temp_ok));

        if (temp_ok)
            printf("Valor       : %.2f °C\n", temperature);
        else
            printf("Valor       : ---\n");

        printf("Edad dato   : %lu ms\n\n",
               (unsigned long)pdTICKS_TO_MS(temp_age));

        /*------------------- TDS ---------------------*/

        printf("[ TDS ]\n");

        printf("Estado      : %s\n",
               status_string(tds_ok));

        if (tds_ok)
            printf("Valor       : %.0f ppm\n", tds);
        else
            printf("Valor       : ---\n");

        printf("Edad dato   : %lu ms\n",
               (unsigned long)pdTICKS_TO_MS(tds_age));

        printf("\n");

        /*---------------- SALINIDAD ------------------*/

        printf("[ SALINIDAD ]\n");

        printf("Estado      : %s\n",
               status_string(sal_ok));

        if (sal_ok)
            printf("Valor       : %.0f ppm\n", salinity);
        else
            printf("Valor       : ---\n");

        printf("\n");

        /*---------------- OLEAJE ---------------------*/

        printf("[ OLEAJE ]\n");

        printf("Altura Hs   : ");

        if (hs_ok)
            printf("%.2f m\n", hs);
        else
            printf("---\n");

        printf("Periodo Tp  : ");

        if (tp_ok)
            printf("%.2f s\n", tp);
        else
            printf("---\n");

        printf("\n");

        printf("=====================================================\n");

        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
    }
}