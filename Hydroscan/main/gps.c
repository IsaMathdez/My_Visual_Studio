
#include "gps.h"

#include "modem.h"
#include "utilities.h"
#include "buoy_data.h"

#include "driver/uart.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "GPS";

static bool gps_fix = false;

esp_err_t gps_init(void)
{
    char rx[256];

    if (!modem_is_ready())
        return ESP_FAIL;

    ESP_LOGI(TAG, "Initializing GPS...");

    /* Encender GNSS */
    modem_send_at("AT+CGNSSPWR=1");

    if(modem_read_response(rx,sizeof(rx),3000)<=0)
        return ESP_FAIL;

    ESP_LOGI(TAG,"%s",rx);

    if(strstr(rx,"OK")==NULL)
        return ESP_FAIL;

    /* Dar unos segundos al receptor */
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG,"GPS READY");

    return ESP_OK;
}

// Parte II

static bool gps_read_info(char *buffer, size_t len)
{
    modem_send_at("AT+CGNSSINFO");

    int n = modem_read_response(
                buffer,
                len,
                3000);

    if (n <= 0)
        return false;

    ESP_LOGI(TAG, "%s", buffer);

    return true;
}

static float gps_convert_coordinate(const char *coord, char hemi)
{
    if(coord == NULL || strlen(coord) < 4)
        return 0.0f;

    float value = atof(coord);

    int degrees = (int)(value / 100.0f);

    float minutes = value - degrees * 100.0f;

    float decimal = degrees + minutes / 60.0f;

    if(hemi == 'S' || hemi == 'W')
        decimal = -decimal;

    return decimal;
}

esp_err_t gps_update(void)
{
    char rx[256];

    char lat_str[20];
    char lon_str[20];

    char lat_hemi;
    char lon_hemi;

    char date[16];
    char utc[16];

    float altitude;
    float speed;

    //gps_init();

    /* Leer respuesta del módem */
    if (!gps_read_info(rx, sizeof(rx)))
    {
        ESP_LOGW(TAG, "No response from GPS");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s", rx);

    /* ¿Todavía no existe FIX? */
    if (strstr(rx, "+CGPSINFO: ,"))
    {
        gps_fix = false;

        buoy_data.latitude.valid  = false;
        buoy_data.longitude.valid = false;
        buoy_data.altitude.valid  = false;
        buoy_data.speed.valid     = false;

        ESP_LOGI(TAG, "Waiting GPS FIX...");

        return ESP_OK;
    }

    int fields =
    sscanf(
        rx,
        "+CGPSINFO: %19[^,],%c,%19[^,],%c,%15[^,],%15[^,],%f,%f",
        lat_str,
        &lat_hemi,
        lon_str,
        &lon_hemi,
        date,
        utc,
        &altitude,
        &speed);

    if (fields != 8)
    {
        ESP_LOGW(TAG, "Invalid GPS frame");

        gps_fix = false;

        return ESP_FAIL;
    }

    float latitude  = gps_convert_coordinate(lat_str, lat_hemi);
    float longitude = gps_convert_coordinate(lon_str, lon_hemi);

    gps_fix = true;

    uint32_t now =
        xTaskGetTickCount() * portTICK_PERIOD_MS;

    buoy_data.latitude.value = latitude;
    buoy_data.latitude.valid = true;
    buoy_data.latitude.last_update_ms = now;

    buoy_data.longitude.value = longitude;
    buoy_data.longitude.valid = true;
    buoy_data.longitude.last_update_ms = now;

    buoy_data.altitude.value = altitude;
    buoy_data.altitude.valid = true;
    buoy_data.altitude.last_update_ms = now;

    buoy_data.speed.value = speed;
    buoy_data.speed.valid = true;
    buoy_data.speed.last_update_ms = now;

    ESP_LOGI(TAG,
             "GPS FIX | Lat: %.6f | Lon: %.6f | Alt: %.2f m | Speed: %.2f km/h",
             latitude,
             longitude,
             altitude,
             speed);

    return ESP_OK;
}



