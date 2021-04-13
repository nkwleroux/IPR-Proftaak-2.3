#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//Wifi
#include "wifi-connect.h"

//LCD
#include "lcd-menu.h"

//ClockSync
#include "clock-sync.h"

void app_main(void)
{
    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 2, NULL);
    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 1, NULL);
    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 1, NULL);
}


