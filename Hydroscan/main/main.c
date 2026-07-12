
// Codigo Sensor de Temperatura DS18B20
// Version 1.0 de GPT

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "onewire_bus.h"
#include "ds18b20.h"

#define ONEWIRE_GPIO       4        // Cambia por el GPIO que estés usando
#define READ_PERIOD_MS     2000     // Tiempo entre mediciones (2 segundos)

void app_main(void)
{
    esp_err_t ret;

    //=============================
    // Inicializar bus OneWire
    //=============================
    onewire_bus_handle_t bus = NULL;

    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_GPIO,
    };

    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };

    ret = onewire_new_bus_rmt(&bus_config, &rmt_config, &bus);
    ESP_ERROR_CHECK(ret);

    //=============================
    // Buscar sensores
    //=============================
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t dev;

    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

    if (onewire_device_iter_get_next(iter, &dev) != ESP_OK) {
        printf("No se encontró ningún DS18B20.\n");
        return;
    }

    //=============================
    // Inicializar DS18B20
    //=============================
    ds18b20_device_handle_t ds18b20 = NULL;

    ds18b20_config_t sensor_config = {};

    ESP_ERROR_CHECK(ds18b20_new_device_from_enumeration(&dev, &sensor_config, &ds18b20));

    printf("DS18B20 detectado correctamente.\n");

    //=============================
    // Lectura periódica
    //=============================
    while (1) {

        float temperature;

        ret = ds18b20_trigger_temperature_conversion(ds18b20);
        ESP_ERROR_CHECK(ret);

        vTaskDelay(pdMS_TO_TICKS(750));   // Tiempo máximo de conversión

        ret = ds18b20_get_temperature(ds18b20, &temperature);

        if (ret == ESP_OK) {
            printf("Temperatura: %.2f °C\n", temperature);
        } else {
            printf("Error al leer la temperatura\n");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_PERIOD_MS));
    }
}
