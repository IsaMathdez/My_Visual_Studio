
// Codigo prueba del sensor TDS con 333 ohm en serie
// Funcionando a un 90%, falta corregir una ecuacion de calibracion mas precisa para el rango de 333 ohm, 
// actualmente esta ajustada para 1k ohm

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define ADC_UNIT           ADC_UNIT_1        
#define ADC_CHANNEL        ADC_CHANNEL_0     // GPIO 36 (Pin VP)
#define ADC_ATTEN          ADC_ATTEN_DB_12   
#define ADC_BITWIDTH       ADC_BITWIDTH_12   

#define VREF               3.3               
#define MUESTRAS           30                

static const char *TAG = "BOYA_TDS_V2";

void app_main(void) {
    // Inicialización del ADC
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = { .bitwidth = ADC_BITWIDTH, .atten = ADC_ATTEN };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    ESP_LOGI(TAG, "Curva de calibración optimizada aplicada. Monitoreando...");

    while (1) {
        int acumulador_adc = 0;
        int lectura_cruda = 0;

        for (int i = 0; i < MUESTRAS; i++) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &lectura_cruda));
            acumulador_adc += lectura_cruda;
            vTaskDelay(pdMS_TO_TICKS(15)); 
        }

        float promedio_adc = (float)acumulador_adc / MUESTRAS;
        float voltaje = (promedio_adc * VREF) / 4095.0;

        // --- NUEVA ECUACIÓN AJUSTADA A 333 OHM ---
        float salinidad_ppm = 0.0;
        
        if (voltaje >= 0.40) {
            // Regresión cuadrática exacta para el rango de 333 Ohm
            salinidad_ppm = (26928.5714 * pow(voltaje, 2)) - (42878.5714 * voltaje) + 13042.8571;
        } else {
            // Protección por si la sonda se saca del agua o baja de 0.4V
            salinidad_ppm = 0.0; 
        }


        // --- SALIDA POR CONSOLA DE TELEMETRÍA ---
        printf("\n==================================================\n");
        printf("       TELEMETRÍA ACTUALIZADA DE LA BOYA         \n");
        printf("--------------------------------------------------\n");
        printf("Voltaje leído     : %.3f V\n", voltaje);
        printf("Salinidad Mapeada : %.0f ppm\n", salinidad_ppm);
        printf("Análisis Marítimo : ");
        // --- AJUSTE DE ALERTAS POR PANTALLA (Modifica solo los umbrales de voltaje) ---
        if (voltaje >= 1.95 && voltaje <= 2.05) {
            printf("[ AGUA DE MAR TRADICIONAL ]\n");
            printf("Lectura óptima y esperada para aguas oceánicas.\n");
        } else if (voltaje > 2.05) {
            printf("[ ALTA CONCENTRACIÓN / HIPERSALINIDAD ]\n");
        } else if (voltaje >= 0.60 && voltaje < 1.95) {
            printf("[ AMBIENTE SALOBRE / TRANSICIÓN ]\n");
        } else {
            printf("[ RECURSO DULCE / AGUA DE INTERIOR ]\n");
        }
        printf("--------------------------------------------------\n\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}