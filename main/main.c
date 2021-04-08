#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"

//Touch buttons
#include "esp_peripherals.h"
#include "input_key_service.h"

//LCD
#include "lcd-menu.h"

#undef USE_STDIN

#define MAINTAG "TouchButton"
#define BUTTONTAG "touchpad button"

char *btn_states[] = {  "INPUT_KEY_SERVICE_ACTION_UNKNOWN",        /*!< unknown action id */
                        "INPUT_KEY_SERVICE_ACTION_CLICK",          /*!< click action id */
                        "INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE",  /*!< click release action id */
                        "INPUT_KEY_SERVICE_ACTION_PRESS",          /*!< long press action id */
                        "INPUT_KEY_SERVICE_ACTION_PRESS_RELEASE"   /*!< long press release id */
                      };

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx);

periph_service_handle_t input_key_handle;
menu_t *menu;

void menu_task(void * pvParameter)
{
    menu = menu_create_menu();

    //menu_display_welcome_message(menu);
    menu_display_scroll_menu(menu);

    while(1)
    {
        vTaskDelay(2500 / portTICK_RATE_MS);
    }

    menu_free_menu(menu);
    vTaskDelete(NULL);
}

void app_main(void){

    // step 1. Initalize the peripherals set.
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    periph_cfg.extern_stack = true;
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // step 2. Setup the input key service
    audio_board_key_init(set);
    input_key_service_info_t input_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t key_serv_info = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    key_serv_info.based_cfg.extern_stack = false;
    key_serv_info.handle = set;
    input_key_handle = input_key_service_create(&key_serv_info);
    AUDIO_NULL_CHECK(BUTTONTAG, input_key_handle, return);
    input_key_service_add_key(input_key_handle, input_info, INPUT_KEY_NUM); //INPUT_KEY_NUM = 6 (the touch pads on ESP32-LyraT board)
    periph_service_set_callback(input_key_handle, input_key_service_cb, NULL);

    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
}

//callback for button presses.
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    //ESP_LOGD(BUTTONTAG, "type=%d, source=%d, data=%d, len=%d", evt->type, (int)evt->source, (int)evt->data, evt->len);
    switch ((int)evt->data) {   
        case INPUT_KEY_USER_ID_REC:
            ESP_LOGI(BUTTONTAG, "[ * ] [1 set 2] %s",btn_states[evt->type]);
            break;
        case  INPUT_KEY_USER_ID_MODE:
 
            ESP_LOGI(BUTTONTAG, "[ * ] [mode 1] %s",btn_states[evt->type]);

            if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
                menu_handle_key_event(menu, MENU_KEY_OK);
            }

            break;
        case INPUT_KEY_USER_ID_PLAY:
  
            ESP_LOGI(BUTTONTAG, "[ * ] [play] %s",btn_states[evt->type]);
            break;
        case INPUT_KEY_USER_ID_SET:
 
            ESP_LOGI(BUTTONTAG, "[ * ] [set] %s",btn_states[evt->type]);
            break;
        case INPUT_KEY_USER_ID_VOLDOWN:

            ESP_LOGI(BUTTONTAG, "[ * ] [volume down] %s",btn_states[evt->type]);

            if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
                menu_handle_key_event(menu, MENU_KEY_LEFT);
            }
            break;
        case INPUT_KEY_USER_ID_VOLUP:
            // if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK) {

            // }
            ESP_LOGI(BUTTONTAG, "[ * ] [volume up] %s",btn_states[evt->type]);

            if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
                menu_handle_key_event(menu, MENU_KEY_RIGHT);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}
