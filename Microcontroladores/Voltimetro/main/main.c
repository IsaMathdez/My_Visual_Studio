

// Voltimetro AC con ESP32 (GPIO34)
// ESP-IDF con auto-offset (corregido)

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_rom_sys.h"

#define ADC_CHANNEL ADC1_CHANNEL_6   // GPIO34
#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_11

#define NUM_SAMPLES 500
#define VREF 3.3

// Ajusta este valor SOLO después de calibrar
#define SCALE_FACTOR 418.0

void app_main(void)
{
    // Configurar ADC
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    printf("\n=== Voltimetro AC iniciado ===\n");

    while (1)
    {
        float sum = 0;
        float voltage_samples[NUM_SAMPLES];

        // =============================
        // 1. Leer muestras y calcular offset
        // =============================
        for (int i = 0; i < NUM_SAMPLES; i++)
        {
            int raw = adc1_get_raw(ADC_CHANNEL);
            float voltage = ((float)raw / 4095.0) * VREF;

            voltage_samples[i] = voltage;
            sum += voltage;

            esp_rom_delay_us(200); // muestreo ~5kHz
        }

        float offset = sum / NUM_SAMPLES;

        // =============================
        // 2. Calcular RMS real
        // =============================
        float sum_sq = 0;

        for (int i = 0; i < NUM_SAMPLES; i++)
        {
            float centered = voltage_samples[i] - offset;
            sum_sq += centered * centered;
        }

        float vrms = sqrt(sum_sq / NUM_SAMPLES);

        // =============================
        // 3. Escalar a voltaje real
        // =============================
        float real_voltage = vrms * SCALE_FACTOR;

        // =============================
        // 4. Mostrar resultados
        // =============================
        if (vrms < 0.05)  // umbral mejorado
        {
            printf("NO hay señal detectada\n");
        }
        else
        {
            printf("Voltaje AC: %.2f V | ADC RMS: %.3f V | Offset: %.3f V\n",
                   real_voltage, vrms, offset);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}