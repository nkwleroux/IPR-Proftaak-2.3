#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//Wifi
#include "wifi-connect.h"

//Touch buttons
#include "touch-buttons.h"

//LCD
#include "lcd-menu.h"

//ClockSync
#include "clock-sync.h"

void app_main(void){

    initialize_wifi_connection();

    init_touch_buttons();

    xTaskCreate(&clock_task, "clock_task", 4096, NULL, 5, NULL);
    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
}


