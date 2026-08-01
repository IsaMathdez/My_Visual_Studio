
/*
 * ============================================================
 *                      HYDROSCAN
 * ------------------------------------------------------------
 * Archivo      : modem.c
 * Descripción  : Modem de comunicacion A7608 del modulo LILYGO
 *
 * Autor        : Hydroscan Project
 * ============================================================
 */

#include "modem.h"
#include "utilities.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "MODEM";

static bool modem_ready = false;

/*==============================================================
                        Inicializar UART
==============================================================*/

static esp_err_t modem_uart_init(void)
{
    uart_config_t uart_config =
    {
        .baud_rate = MODEM_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(
        uart_driver_install(
            MODEM_UART_NUM,
            4096,
            4096,
            0,
            NULL,
            0));

    ESP_ERROR_CHECK(
        uart_param_config(
            MODEM_UART_NUM,
            &uart_config));

    ESP_ERROR_CHECK(
        uart_set_pin(
            MODEM_UART_NUM,
            MODEM_TX_PIN,
            MODEM_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE));

    return ESP_OK;
}

/*==============================================================
                        ENCENDER EL MODEM
==============================================================*/

static void modem_power_on(void)
{
    gpio_config_t io =
    {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask =
            (1ULL<<BOARD_PWRKEY_PIN) |
            (1ULL<<MODEM_RESET_PIN) |
            (1ULL<<MODEM_DTR_PIN)
    };

    gpio_config(&io);

    gpio_set_level(MODEM_DTR_PIN,0);

    gpio_set_level(MODEM_RESET_PIN,!MODEM_RESET_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(MODEM_RESET_PIN,MODEM_RESET_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(2600));

    gpio_set_level(MODEM_RESET_PIN,!MODEM_RESET_LEVEL);

    gpio_set_level(BOARD_PWRKEY_PIN,0);
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(BOARD_PWRKEY_PIN,1);
    vTaskDelay(pdMS_TO_TICKS(MODEM_POWERON_PULSE_WIDTH_MS));

    gpio_set_level(BOARD_PWRKEY_PIN,0);

    ESP_LOGI(TAG,"Power sequence complete");
}

/*==============================================================
                        ENVIAR COMANDO AT
==============================================================*/

 esp_err_t modem_send_at(const char *cmd)
{
    uart_write_bytes(
        MODEM_UART_NUM,
        cmd,
        strlen(cmd));

    uart_write_bytes(
        MODEM_UART_NUM,
        "\r\n",
        2);

    return ESP_OK;
}

/*==============================================================
                        LEER RESPUESTA
==============================================================*/

static bool modem_wait_response(
    const char *expected,
    uint32_t timeout_ms)
{
    char buffer[512];

    int len =
        uart_read_bytes(
            MODEM_UART_NUM,
            (uint8_t *)buffer,
            sizeof(buffer)-1,
            pdMS_TO_TICKS(timeout_ms));

    if(len<=0)
        return false;

    buffer[len]=0;

    ESP_LOGI(TAG,"%s",buffer);

    return strstr(buffer,expected)!=NULL;
}

/*==============================================================
                    VERIFICAR COMUNICACION AT
==============================================================*/

static bool modem_test_at(void)
{
    modem_send_at("AT");

    return modem_wait_response("OK",1000);
}

/*==============================================================
                    ESPERAR QUE EL MODEM RESPONDA
==============================================================*/

static bool modem_wait_ready(void)
{
    int retry=0;

    while(!modem_test_at())
    {
        ESP_LOGI(TAG,"Waiting modem...");

        retry++;

        if(retry>30)
        {
            modem_power_on();
            retry=0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return true;
}

/*==============================================================
                    VERIFICAR SIM
==============================================================*/

static bool modem_wait_sim(void)
{
    while(true)
    {
        modem_send_at("AT+CPIN?");

        if(modem_wait_response("READY",1000))
        {
            ESP_LOGI(TAG,"SIM READY");
            return true;
        }

        ESP_LOGI(TAG,"Waiting SIM...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*==============================================================
                    REGISTRO EN RED
==============================================================*/

static bool modem_wait_network(void)
{
    while(true)
    {
        modem_send_at("AT+CREG?");

        if(modem_wait_response(",1",1000))
            return true;

        if(modem_wait_response(",5",1000))
            return true;

        ESP_LOGI(TAG,"Registering network...");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*==============================================================
                        ACTIVAR LTE
==============================================================*/

static bool modem_activate_network(void)
{
    modem_send_at("AT+CGATT=1"); // NOs quedamos aqui en bucle, faltaba conectar antena LTE

    if(!modem_wait_response("OK",5000))
        return false;

    return true;
}

/*==============================================================
                        PARA LEER
==============================================================*/

int modem_read_response(char *buffer,
                        size_t max_len,
                        uint32_t timeout_ms)
{
    size_t index = 0;
    uint8_t c;

    int64_t start = esp_timer_get_time() / 1000;

    while ((esp_timer_get_time()/1000 - start) < timeout_ms)
    {
        int len = uart_read_bytes(
                    MODEM_UART_NUM,
                    &c,
                    1,
                    pdMS_TO_TICKS(20));

        if(len > 0)
        {
            if(index < max_len - 1)
                buffer[index++] = c;

            buffer[index] = '\0';

            if(strstr(buffer, "\r\nOK\r\n"))
                return index;

            if(strstr(buffer, "\r\nERROR\r\n"))
                return index;

            if(strstr(buffer, "+CME ERROR"))
                return index;
        }
    }

    return -1;
}

/*==============================================================
                    FUNCION PRINCIPAL MODEM
==============================================================*/

esp_err_t modem_init(void)
{
    modem_ready=false;

    modem_uart_init();

    modem_power_on();

    if(!modem_wait_ready())
        return ESP_FAIL;

    if(!modem_wait_sim())
        return ESP_FAIL;

    if(!modem_wait_network())
        return ESP_FAIL;

    if(!modem_activate_network())
        return ESP_FAIL;

    modem_ready=true;

    ESP_LOGI(TAG,"MODEM READY");

    return ESP_OK;
}

/*==============================================================
                    ESTADO
==============================================================*/

bool modem_is_ready(void)
{
    return modem_ready;
}