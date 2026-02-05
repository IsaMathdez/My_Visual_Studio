// Isaias Matos
// FSM Porton Electrico version 2.2
// Nuevo: Cmakelist
// ESP32 + FreeRTOS + WiFi + MQTT aun sin funcionar

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"

/* ================= WIFI / MQTT ================= */

#define WIFI_SSID "Docentes_Administrativos"
#define WIFI_PASS "Adm1N2584km"
#define MQTT_BROKER_URI "mqtt://test.mosquitto.org"

static const char *TAG = "PORTON";

/* ================= CONFIGURACIÓN DE PINES ================= */

// Entradas
#define PIN_BTN_OPEN 18
#define PIN_BTN_CLOSE 19
#define PIN_BTN_PAUSE 21
#define PIN_FCC 32
#define PIN_FCA 33
#define PIN_FCT 35

// Salidas
#define PIN_MOTOR_A 4
#define PIN_MOTOR_B 5
#define PIN_WARNING 2

// Tiempos
#define MOTOR_RUNTIME_MAX_MS 15000
#define LOOP_PERIOD_MS 100
#define WARNING_BLINK_MS 500

/* ================= ESTRUCTURAS IO ================= */

typedef struct
{
    int bo;
    int bc;
    int bp;
    int fcc;
    int fca;
    int ftc;
} IO_Input;

typedef struct
{
    int motor_a;
    int motor_b;
    int warning;
} IO_Output;

static IO_Input io_in;
static IO_Output io_out;

/* ================= FSM ================= */

typedef enum
{
    INIT,
    ABRIENDO,
    ABIERTO,
    CERRANDO,
    CERRADO,
    EN_MEDIO,
    ERROR
} estado_t;

static estado_t estado_actual = INIT;
static estado_t estado_siguiente = INIT;

static int motor_runtime = 0;
static int warning_timer = 0;

/* ================= FLAGS MQTT (botones virtuales) ================= */

static int bo_mqtt = 0;
static int bc_mqtt = 0;
static int bp_mqtt = 0;

static esp_mqtt_client_handle_t mqtt_client;

/* ================= IO ================= */

void IO_Init(void)
{
    gpio_config_t io_conf = {0};

    // Entradas con pulldown
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask =
        (1ULL << PIN_BTN_OPEN) |
        (1ULL << PIN_BTN_CLOSE) |
        (1ULL << PIN_BTN_PAUSE) |
        (1ULL << PIN_FCC) |
        (1ULL << PIN_FCA) |
        (1ULL << PIN_FCT);

    gpio_config(&io_conf);

    // Salidas
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pin_bit_mask =
        (1ULL << PIN_MOTOR_A) |
        (1ULL << PIN_MOTOR_B) |
        (1ULL << PIN_WARNING);

    gpio_config(&io_conf);
}

void IO_Update(void)
{
    // Entradas físicas OR MQTT
    io_in.bo = gpio_get_level(PIN_BTN_OPEN) | bo_mqtt;
    io_in.bc = gpio_get_level(PIN_BTN_CLOSE) | bc_mqtt;
    io_in.bp = gpio_get_level(PIN_BTN_PAUSE) | bp_mqtt;

    io_in.fcc = gpio_get_level(PIN_FCC);
    io_in.fca = gpio_get_level(PIN_FCA);
    io_in.ftc = gpio_get_level(PIN_FCT);

    // Limpiar pulsos MQTT (1 ciclo)
    bo_mqtt = bc_mqtt = bp_mqtt = 0;

    // Salidas
    gpio_set_level(PIN_MOTOR_A, io_out.motor_a);
    gpio_set_level(PIN_MOTOR_B, io_out.motor_b);
    gpio_set_level(PIN_WARNING, io_out.warning);
}

/* ================= FSM ================= */
void FSM_Run(void)
{
    estado_actual = estado_siguiente;
    switch (estado_actual)
    {

    case INIT:
        io_out.motor_a = 0;
        io_out.motor_b = 0;
        io_out.warning = 0;
        printf("\nINICIADO\n\n");
        if (io_in.fcc && !io_in.fca)
            estado_siguiente = CERRADO;
        else if (!io_in.fcc && io_in.fca)
            estado_siguiente = ABIERTO;
        else if (!io_in.fcc && !io_in.fca)
            estado_siguiente = EN_MEDIO;
        else
            estado_siguiente = ERROR;
        break;

    case ABIERTO:
        io_out.motor_a = 0;
        io_out.motor_b = 0;
        io_out.warning = 0;
        printf("PORTON ABIERTO\n");
        if (io_in.bc && !io_in.ftc && !io_in.fcc)
        {
            motor_runtime = MOTOR_RUNTIME_MAX_MS / LOOP_PERIOD_MS;
            estado_siguiente = CERRANDO;
        }
        break;

    case CERRADO:
        io_out.motor_a = 0;
        io_out.motor_b = 0;
        io_out.warning = 0;
        printf("PORTON CERRADO\n");
        if (io_in.bo && !io_in.fca)
        {
            motor_runtime = MOTOR_RUNTIME_MAX_MS / LOOP_PERIOD_MS;
            estado_siguiente = ABRIENDO;
        }
        break;

    case EN_MEDIO:
        io_out.motor_a = 0;
        io_out.motor_b = 0;
        io_out.warning = 0;
        printf("PORTON EN MEDIO\n");
        if (io_in.bo && !io_in.fca)
        {
            motor_runtime = MOTOR_RUNTIME_MAX_MS / LOOP_PERIOD_MS;
            estado_siguiente = ABRIENDO;
        }
        else if (io_in.bc && !io_in.ftc && !io_in.fcc)
        {
            motor_runtime = MOTOR_RUNTIME_MAX_MS / LOOP_PERIOD_MS;
            estado_siguiente = CERRANDO;
        }
        break;

    case ABRIENDO:
        io_out.motor_a = 1;
        io_out.motor_b = 0;
        io_out.warning = 0;
        printf("ABRIENDO PORTON\n");
        if (motor_runtime <= 0)
            estado_siguiente = ERROR;
        else if (io_in.bp)
            estado_siguiente = EN_MEDIO;
        else if (io_in.fca)
            estado_siguiente = ABIERTO;
        else
            motor_runtime--;
        break;

    case CERRANDO:
        io_out.motor_a = 0;
        io_out.motor_b = 1;
        io_out.warning = 0;
        printf("CERRANDO PORTON\n");
        if (motor_runtime <= 0)
            estado_siguiente = ERROR;
        else if (io_in.ftc)
            estado_siguiente = ABRIENDO;
        else if (io_in.bp)
            estado_siguiente = EN_MEDIO;
        else if (io_in.fcc)
            estado_siguiente = CERRADO;
        else
            motor_runtime--;
        break;

    case ERROR:
        io_out.motor_a = 0;
        io_out.motor_b = 0;
        printf("\nERROR\n\n");
        warning_timer++;
        if (warning_timer >= (WARNING_BLINK_MS / LOOP_PERIOD_MS))
        {
            io_out.warning ^= 1;
            warning_timer = 0;
        }
        if (!(io_in.fcc && io_in.fca))
            estado_siguiente = INIT;
        break;
    }
}

/* ================= MQTT ================= */

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:

        esp_mqtt_client_subscribe(mqtt_client, "porton/isa/cmd", 0);
        printf("\nMQTT conectado al broker %s\n", MQTT_BROKER_URI);
        return;

    case MQTT_EVENT_DISCONNECTED:
        printf("\nMQTT desconectado\n");
        return;

    case MQTT_EVENT_SUBSCRIBED:
        printf("\nMQTT suscripcion exitosa\n");
        return;
    }

    if (event_id == MQTT_EVENT_DATA)
    {

        if (strncmp(event->data, "OPEN", event->data_len) == 0)
            bo_mqtt = 1;
        else if (strncmp(event->data, "CLOSE", event->data_len) == 0)
            bc_mqtt = 1;
        else if (strncmp(event->data, "PAUSE", event->data_len) == 0)
            bp_mqtt = 1;

        printf("\nMQTT CMD recibido: %.*s\n", event->data_len, event->data);
    }
}

void mqtt_start(void)
{
    printf("\nIniciando MQTT hacia broker: %s\n", MQTT_BROKER_URI);
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    printf("\nMQTT iniciado\n");
}

/* ================= WIFI ================= */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        printf("\nWiFi iniciado. Conectando a SSID: %s\n", WIFI_SSID);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        printf("WiFi conectado a %s | IP: " IPSTR "\n", WIFI_SSID, IP2STR(&event->ip_info.ip));
        mqtt_start();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("\nWiFi desconectado, reintentando...\n");
        esp_wifi_connect();
    }
}

void wifi_init(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                               &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

/* ================= MAIN ================= */

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    IO_Init();

    // Prueba de lámparas: enciende todo por 1 segundo al arrancar
    gpio_set_level(4, 1);
    gpio_set_level(5, 1);
    gpio_set_level(2, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(4, 0);
    gpio_set_level(5, 0);
    gpio_set_level(2, 0);

    wifi_init();

    while (1)
    {
        IO_Update();
        FSM_Run();
        vTaskDelay(pdMS_TO_TICKS(LOOP_PERIOD_MS));
    }
}

// THE END