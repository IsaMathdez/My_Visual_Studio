// Isaias Matos
// Entrada: 3 botones (Azul, Amarillo, Rojo)
// Salida: 3 LEDs (Azul, Amarillo, Rojo)
// Lógica: Máximo 2 LEDs encendidos. Si se intenta encender un tercero, se apaga el más antiguo.
// Version 1.0

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// --- 1. CAPA UNIVERSAL (Lógica pura) ---

// Definición de colores para el control de la cola
typedef enum { NONE = -1, BLUE = 0, YELLOW = 1, RED = 2 } color_t;

// Estructura que abstrae el hardware
typedef struct {
    bool btn_blue;
    bool btn_yellow;
    bool btn_red;
    
    bool led_blue;
    bool led_yellow;
    bool led_red;
} IO_t;

// Variables globales para la lógica de "máximo 2"
color_t active_leds[2] = { NONE, NONE }; 
bool last_btn_state[3] = { false, false, false };

void process_logic(IO_t *io) {
    bool current_btns[3] = { io->btn_blue, io->btn_yellow, io->btn_red };
    bool *leds[3] = { &io->led_blue, &io->led_yellow, &io->led_red };

    for (int i = 0; i < 3; i++) {
        // Detectar flanco de subida (Toggle)
        if (current_btns[i] && !last_btn_state[i]) {
            
            if (*leds[i]) {
                // Si ya está encendido, lo apagamos y quitamos de la cola
                *leds[i] = false;
                if (active_leds[0] == i) active_leds[0] = NONE;
                else if (active_leds[1] == i) active_leds[1] = NONE;
            } else {
                // Si queremos encenderlo:
                // Si ya hay dos encendidos, apagamos el más viejo (active_leds[0])
                if (active_leds[0] != NONE && active_leds[1] != NONE) {
                    int oldest = active_leds[0];
                    if (oldest == 0) io->led_blue = false;
                    if (oldest == 1) io->led_yellow = false;
                    if (oldest == 2) io->led_red = false;
                    
                    // Desplazamos la cola
                    active_leds[0] = active_leds[1];
                    active_leds[1] = (color_t)i;
                } else {
                    // Si hay espacio, lo añadimos
                    if (active_leds[0] == NONE) active_leds[0] = (color_t)i;
                    else active_leds[1] = (color_t)i;
                }
                *leds[i] = true;
            }
        }
        last_btn_state[i] = current_btns[i];
    }
    printf("Program by Isaias Matos!!\n\n");
}


// --- 2. CAPA ESP32 (Asignación de Pines) ---

#define PIN_BTN_BLUE   18
#define PIN_BTN_YELLOW 19
#define PIN_BTN_RED    21

#define PIN_LED_BLUE   4
#define PIN_LED_YELLOW 5
#define PIN_LED_RED    2

void init_hw() {
    // Configurar LEDs como salida
    gpio_set_direction(PIN_LED_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED_YELLOW, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED_RED, GPIO_MODE_OUTPUT);

    // Configurar Botones como entrada con Pull-down interno
    gpio_set_direction(PIN_BTN_BLUE, GPIO_MODE_INPUT);
    gpio_pullup_dis(PIN_BTN_BLUE);
    gpio_pulldown_en(PIN_BTN_BLUE);

    gpio_set_direction(PIN_BTN_YELLOW, GPIO_MODE_INPUT);
    gpio_pullup_dis(PIN_BTN_YELLOW);
    gpio_pulldown_en(PIN_BTN_YELLOW);

    gpio_set_direction(PIN_BTN_RED, GPIO_MODE_INPUT);
    gpio_pullup_dis(PIN_BTN_RED);
    gpio_pulldown_en(PIN_BTN_RED);
}

void app_main(void) {
    init_hw();
    IO_t sistema_io = {0}; // Inicializar todo en falso

    while (1) {
        // 1. Leer Hardware y pasarlo a la estructura
        sistema_io.btn_blue = gpio_get_level(PIN_BTN_BLUE);
        sistema_io.btn_yellow = gpio_get_level(PIN_BTN_YELLOW);
        sistema_io.btn_red = gpio_get_level(PIN_BTN_RED);

        // 2. Procesar lógica universal
        process_logic(&sistema_io);

        // 3. Escribir resultados al Hardware
        gpio_set_level(PIN_LED_BLUE, sistema_io.led_blue);
        gpio_set_level(PIN_LED_YELLOW, sistema_io.led_yellow);
        gpio_set_level(PIN_LED_RED, sistema_io.led_red);

        vTaskDelay(pdMS_TO_TICKS(50)); // Debounce simple y respiro al CPU
    }
}















/*
// Version 2.1 con MQTT

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

// --- CONFIGURACIÓN ---
#define WIFI_SSID      "Flia. Matos"
#define WIFI_PASS      "24091973"
#define MQTT_URI       "mqtt://192.168.100.73"
#define MQTT_TOPIC_SUB "leds/control"

// Pines (igual que antes)
#define PIN_BTN_BLUE 18, PIN_BTN_YELLOW 19, PIN_BTN_RED 21
#define PIN_LED_BLUE 4,  PIN_LED_YELLOW 5,  PIN_LED_RED 2

typedef enum { NONE = -1, BLUE = 0, YELLOW = 1, RED = 2 } color_t;

// Estructura para manejar entradas y salidas
typedef struct {
    bool btn_remote[3]; // Gatillos virtuales para MQTT
    bool btn_phys[3];   // Botones físicos
    bool led_state[3];
} IO_t;


IO_t sistema_io = {0};
color_t active_leds[2] = { NONE, NONE };
bool last_state[3] = { false, false, false };

// --- LÓGICA UNIVERSAL (Modificada para aceptar remoto) ---
void process_logic(IO_t *io) {
    for (int i = 0; i < 3; i++) {
        // Se activa si el botón físico tiene flanco de subida O si MQTT mandó un pulso
        bool trigger = (io->btn_phys[i] && !last_state[i]) || io->btn_remote[i];
        
        if (trigger) {
            if (io->led_state[i]) {
                io->led_state[i] = false;
                if (active_leds[0] == i) active_leds[0] = NONE;
                else if (active_leds[1] == i) active_leds[1] = NONE;
            } else {
                if (active_leds[0] != NONE && active_leds[1] != NONE) {
                    int oldest = active_leds[0];
                    io->led_state[oldest] = false;
                    active_leds[0] = active_leds[1];
                    active_leds[1] = (color_t)i;
                } else {
                    if (active_leds[0] == NONE) active_leds[0] = (color_t)i;
                    else active_leds[1] = (color_t)i;
                }
                io->led_state[i] = true;
            }
            io->btn_remote[i] = false; // Resetear el gatillo virtual después de procesar
        }
        last_state[i] = io->btn_phys[i];
    }
    printf("Program by Isaias Matos with MQTT\n");
}

// --- MANEJADOR DE EVENTOS MQTT ---
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    if (event_id == MQTT_EVENT_DATA) {
        printf("Mensaje recibido: %.*s\n", event->data_len, event->data);
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        
        if (strncmp(event->data, "BLUE", event->data_len) == 0)   sistema_io.btn_remote[BLUE] = true;
        if (strncmp(event->data, "YELLOW", event->data_len) == 0) sistema_io.btn_remote[YELLOW] = true;
        if (strncmp(event->data, "RED", event->data_len) == 0)    sistema_io.btn_remote[RED] = true;
    }
}

// --- INICIALIZACIÓN DE RED (Boilerplate de ESP-IDF) ---
void wifi_init() {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS } };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void mqtt_init() {
    esp_mqtt_client_config_t mqtt_cfg = { .broker.address.uri = MQTT_URI };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    esp_mqtt_client_subscribe(client, MQTT_TOPIC_SUB, 0);
}

// --- FUNCIÓN PRINCIPAL ---

void app_main(void) {
    // Inicializar hardware (pines 18, 19, 21 como input y 4, 5, 2 como output)
    // ... (Aquí iría el código de gpio_set_direction del ejemplo anterior) ...
    
    // Prueba de lámparas: enciende todo por 1 segundo al arrancar
    gpio_set_level(4, 1); gpio_set_level(5, 1); gpio_set_level(2, 1);
    vTaskDelay(pdMS_TO_TICKS(4000));
    gpio_set_level(4, 0); gpio_set_level(5, 0); gpio_set_level(2, 0);

    // Inicializar WiFi y MQTT
    wifi_init();
    mqtt_init();

    while (1) {
        // Leer botones físicos
        sistema_io.btn_phys[BLUE] = gpio_get_level(18);
        sistema_io.btn_phys[YELLOW] = gpio_get_level(19);
        sistema_io.btn_phys[RED] = gpio_get_level(21);

        process_logic(&sistema_io);

        // Actualizar LEDs
        gpio_set_level(4, sistema_io.led_state[BLUE]);
        gpio_set_level(5, sistema_io.led_state[YELLOW]);
        gpio_set_level(2, sistema_io.led_state[RED]);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// THE END

*/