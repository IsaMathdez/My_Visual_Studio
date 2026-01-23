
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    while (1) {
        printf("Que dificil es MicrocontroladoreS!\n");
        printf("I'M BATMAN\n\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
