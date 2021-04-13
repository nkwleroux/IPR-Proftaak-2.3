#include "lcd-menu.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "cJSON.h"

//HTTP request / weather API
#include "http-request.h"

//Touch buttons
#include "touch-buttons.h"

//Clock-sync
#include "clock-sync.h"

#define MENUTAG "Menu"

// Menu actions
void menu_next_item(void);
void menu_previous_item(void);
void menu_ok_item(void);

// Menu display methods
void menu_display_date_time(void);

void menu_enter_display_settings(void);
void menu_display_settings(void);

void menu_display_weather(void);

// Menu screen refresh
void menu_refresh_screen(void);

static i2c_lcd1602_info_t *_lcd_info;
static menu_t *_menu;

char *dataDateTime[MENU_DATE_TIME_SIZE + 1];
char** citySelectionList;
int selectedCityIndex = 0;
int scrollIndex = 1;
TaskHandle_t xHandleAPI = NULL;

void menu_task(void * pvParameter){

    xTaskCreate(&api_request_task, "api_request", 4096, NULL, 5, &xHandleAPI);

    init_touch_buttons();

    _menu = menu_create_menu();

    menu_display_welcome_message(_menu);
    menu_display_scroll_menu(_menu);

    while(1)
    {
        menu_refresh_screen();
        vTaskDelay(2500 / portTICK_RATE_MS);
    }

    menu_free_menu(_menu);
    vTaskDelete(NULL);
}

/**
 * @brief  Used to start a new task with the desired city.
 * @param  selectedCity: Index of the required city in the selectionList
 */
void menu_new_API_task(int selectedCity){

    select_city(selectedCity);
    
    vTaskDelete(xHandleAPI);
    
    xTaskCreate(&api_request_task, "api_request", 4096, NULL, 5, &xHandleAPI);
}

/**
 * @brief  Initializes i2c.
 */
void i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_LEN,
                       I2C_MASTER_TX_BUF_LEN, 0);
}

/**
 * @brief  Initializes the lcd and returns lcd_info for lcd usage
 * @return lcd_info: Used for all LCD functions.
 */
i2c_lcd1602_info_t * lcd_init()
{
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    smbus_init(smbus_info, i2c_num, address);
    smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

    // Set up the LCD1602 device with backlight off
    i2c_lcd1602_info_t * lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VIS_COLUMNS);

    // turn off backlight
    ESP_LOGI(MENUTAG, "backlight off");
    i2c_lcd1602_set_backlight(lcd_info, false);

    // turn on backlight
    ESP_LOGI(MENUTAG, "backlight on");
    i2c_lcd1602_set_backlight(lcd_info, true);

    // turn on cursor 
    ESP_LOGI(MENUTAG, "cursor on");
    i2c_lcd1602_set_cursor(lcd_info, true);
    
    // turn on cursor TODO MAYBE REMOVE
    ESP_LOGI(MENUTAG, "cursor on");
    i2c_lcd1602_set_cursor(lcd_info, false);

    return lcd_info;
}

menu_t *menu_create_menu()
{
    // I^2C initialization + the I^2C port
    i2c_master_init();
    
    // LCD initialization
    _lcd_info = lcd_init();

    // Allocate memory to the menuPointer
    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_DATE_TIME_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1},     {"MAIN MENU", "Date & Time"},   {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_1, {MENU_SETTINGS_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2},      {"MAIN MENU", "API Settings"},  {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_2, {MENU_WEATHER_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_0},       {"MAIN MENU", "Weather"},       {NULL, NULL, NULL}, NULL, NULL},

        //Date/time
        {MENU_DATE_TIME_ID_0, {MENU_DATE_TIME_ID_0, MENU_DATE_TIME_ID_0, MENU_DATE_TIME_ID_0},  {"DATE & TIME", ""}, {menu_ok_item, menu_previous_item, menu_next_item}, menu_display_date_time, NULL},
        
        //Weather
        {MENU_WEATHER_ID_0, {MENU_WEATHER_ID_0, MENU_WEATHER_ID_0, MENU_WEATHER_ID_0},  {"WEATHER", ""}, {menu_ok_item, NULL, NULL}, menu_display_weather, NULL},
      
        //API settings
        {MENU_SETTINGS_ID_0, {MENU_SETTINGS_ID_0, MENU_SETTINGS_ID_0, MENU_SETTINGS_ID_0},  {"API Settings", ""}, {menu_ok_item, menu_previous_item, menu_next_item}, menu_enter_display_settings, NULL},
      
    };

    // If allocation is succesful, set all values
    if(menuPointer != NULL){

        // Initialize menu with values
        menuPointer->lcd_info = _lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;

    }

    return menuPointer;
}

void menu_free_menu(menu_t *menu)
{
    free(menu->lcd_info);
    menu->lcd_info = NULL;
    free(menu->menuItems);
    menu->menuItems = NULL;
    free(menu);
    menu = NULL;
}

void menu_display_welcome_message(menu_t *menu)
{
    // Clears screen
    i2c_lcd1602_clear(menu->lcd_info);

    i2c_lcd1602_set_cursor(menu->lcd_info, false);
    i2c_lcd1602_move_cursor(menu->lcd_info, 6, 1);

    i2c_lcd1602_write_string(menu->lcd_info, "Welcome");
    i2c_lcd1602_move_cursor(menu->lcd_info, 8, 2);
    i2c_lcd1602_write_string(menu->lcd_info, "User");

    vTaskDelay(2500 / portTICK_RATE_MS);
    i2c_lcd1602_clear(menu->lcd_info);
}

/**
 * @brief  Helper method to write data to LCD.
 * @param  *lcd_info: Pointer for LCD.
 * @param  text: Text which to be displayed.
 * @param  position: Position of first letter of the Text.
 * @param  line: Which row the text will be displayed on.
 */
void menu_write_item(i2c_lcd1602_info_t *lcd_info, char* text, int position, int line)
{
    i2c_lcd1602_move_cursor(lcd_info, position, line);
    i2c_lcd1602_write_string(lcd_info, text);
}

void menu_display_menu_item(menu_t *menu, int menuItemId)
{
    // Clears screen
    i2c_lcd1602_clear(menu->lcd_info);

    // Displays each item on LCD
    for (size_t line = 0; line < MAX_LCD_LINES; line++) {
        char* menuText = menu->menuItems[menuItemId].menuText[line];
        int textPosition = 10 - ((strlen(menuText) + 1) / 2);
        menu_write_item(menu->lcd_info, menuText, textPosition, line);
    }
}

/**
 * @brief  Helper method to write data to LCD. Centers text.
 * @param  *lcd_info: Pointer for LCD.
 * @param  text: Text which to be displayed.
 * @param  line: Which row the text will be displayed on.
 */
void menu_write_scroll_menu_item(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    int textPosition = 10 - ((strlen(text) + 1) / 2);
    menu_write_item(lcd_info, text, textPosition, line);
}

void menu_display_scroll_menu(menu_t *menu)
{
    // Clears screen
    i2c_lcd1602_clear(menu->lcd_info);
    
    // Gets title of scroll menu
    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 0);

    // Get item before currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 1);

    // Get currentMenuItem
    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 2);

    // Get item after currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 3);

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(menu->lcd_info, 17, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);
}

/**
 * @brief  Displays DATE & TIME menu with the clock data.
 */
void menu_display_date_time()
{
    // Clears screen
    i2c_lcd1602_clear(_lcd_info);

    // Display scroll menu title
    char *menuText = "DATE & TIME";
    menu_write_scroll_menu_item(_lcd_info, menuText, 0);

    if(scrollIndex + 1 > MENU_DATE_TIME_SIZE + 1){
        scrollIndex = 0;
    } else if (scrollIndex - 1 < -1) {
        scrollIndex = MENU_DATE_TIME_SIZE;
    }

    // If below 0 loop back to top
    int indexPreviousDateTime = scrollIndex - 1 < 0 ? MENU_DATE_TIME_SIZE : scrollIndex - 1;
    menuText = dataDateTime[indexPreviousDateTime];
    ESP_LOGI(MENUTAG,"Display text previous %s", menuText);
    menu_write_scroll_menu_item(_lcd_info, menuText, 1);

    menuText = dataDateTime[scrollIndex];
    ESP_LOGI(MENUTAG,"Display text current %s", menuText);
    menu_write_scroll_menu_item(_lcd_info, menuText, 2);

    int indexNextDateTime = scrollIndex + 1 > MENU_DATE_TIME_SIZE ? 0 : scrollIndex + 1;
    menuText = dataDateTime[indexNextDateTime];
    ESP_LOGI(MENUTAG,"Display text next %s", menuText);
    menu_write_scroll_menu_item(_lcd_info, menuText, 3);  
    
    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 17, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);
}

/**
 * @brief  Used to refresh the screen with updated values.
 */
void menu_refresh_screen(void)
{
    // Gets parsed clock data
    clock_sync_t* parsedClock = get_clock();

    //Checks if parsed clock data is NULL. If NULL set default values.
    if(parsedClock == NULL){
        //Initialize temp values for date & time.
        dataDateTime[0] = "No Date";
        dataDateTime[1] = "No Time";
    }else{
        dataDateTime[0] = parsedClock->date;
        dataDateTime[1] = parsedClock->time;
    }

    dataDateTime[2] = "Back";       

    // Displays the correct menu with the updated values
    if(_menu->currentMenuItemId == MENU_DATE_TIME_ID_0){
        menu_display_date_time();
    }else if(_menu->currentMenuItemId == MENU_WEATHER_ID_0){
        menu_display_weather();
    }
}

/**
 * @brief  Called when next/vol+ button is clicked.
 */
void menu_next_item(void)
{
    // Increases the scrollIndex
    scrollIndex++;

    // Displays the correct menu with the updated scrollIndex
    if(_menu->currentMenuItemId == MENU_DATE_TIME_ID_0){
        menu_display_date_time();
    }else if (_menu->currentMenuItemId == MENU_SETTINGS_ID_0){
        menu_display_settings();
    }
} 

/**
 * @brief  Called when previous/vol- button is clicked.
 */
void menu_previous_item(void)
{
    // Lowers the scrollIndex
    scrollIndex--;

    // Displays the correct menu with the updated scrollIndex
    if(_menu->currentMenuItemId == MENU_DATE_TIME_ID_0){
        menu_display_date_time();
    }else if (_menu->currentMenuItemId == MENU_SETTINGS_ID_0){
        menu_display_settings();
    }
}

/**
 * @brief  Called when ok/mode button is clicked.
 */
void menu_ok_item(void)
{
    // Check if current menu is DATE & TIME
    if(_menu->currentMenuItemId == MENU_DATE_TIME_ID_0){
        if(strcmp(dataDateTime[scrollIndex],"Back") == 0){
            scrollIndex = 1;
            menu_go_to_item(MENU_MAIN_ID_0);
        }
    // Check if current menu is API SETTINGS
    }else if(_menu->currentMenuItemId == MENU_SETTINGS_ID_0){
        if(strcmp(citySelectionList[scrollIndex],"Back") == 0){
            scrollIndex = 1;
            menu_go_to_item(MENU_MAIN_ID_1);
        }else{
            // Sets the selectedCity and makes a new task to update WEATHER
            // with new city data.
            selectedCityIndex = scrollIndex;
            menu_new_API_task(selectedCityIndex);
        }
    // Check if current menu is WEATHER
    }else if(_menu->currentMenuItemId == MENU_WEATHER_ID_0){
        menu_go_to_item(MENU_MAIN_ID_2);
    }
}

void menu_handle_key_event(menu_t *menu, int key)
{
    // Checks if menu is NULL. If NULL uses menu created when task is started.
    if(menu == NULL){
        menu = _menu;
    }

    // If key press leads to the same ID as the currentMenuItemId
    // do not switch to a new menu item, instead call the onKey event
    if(menu->menuItems[menu->currentMenuItemId].otherIds[key] == menu->currentMenuItemId){
        // Call the onKey event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key] != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key])();
        }else{
            ESP_LOGI(MENUTAG,"null ok");
        }
    } else {
        // Call the onMenuExit event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent)();
        }

        // Sets the currentMenuItemId
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[key];

        // Display menu on LCD
        if(strcmp(menu->menuItems[menu->currentMenuItemId].menuText[1], " ") == 0) {
            menu_display_menu_item(menu, menu->currentMenuItemId);
        } else {
            menu_display_scroll_menu(menu);
        }

        // Call the onMenuEntry event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }
    }
}

void menu_go_to_item(int menuItem)
{
    // Set currentMenuItemId
    _menu->currentMenuItemId = menuItem;

    // Display menu with new itemId.
    menu_display_scroll_menu(_menu);

    // Call MenuEntryEvent if menu item has a entry event.
    if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
        (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
    }
}

/**
 * @brief  Displays the WEATHER menu with the data from the API.
 */
void menu_display_weather(void)
{
    // Gets API data from http-request.
    weatherAPI_t* parsedResponse = get_parsed_response();

    // Clears screen
    i2c_lcd1602_clear(_lcd_info);

    // Allocates memory for menuText.
    char *menuText = malloc(LCD_NUM_VIS_COLUMNS);

    // Check if response is not NULL. If NULL sets default values.
    if(parsedResponse == NULL){
        menu_write_scroll_menu_item(_lcd_info, "WEATHER", 0);
        menu_write_scroll_menu_item(_lcd_info, "No internet", 1);
    }else{

        // Write API data to the screen.
        sprintf(menuText,"T %.1lfC", parsedResponse->temp);
        menu_write_item(_lcd_info, menuText, 2, 0);

        sprintf(menuText,"FT %.1lfC", parsedResponse->feels_like);
        menu_write_item(_lcd_info, menuText, 10, 0);

        sprintf(menuText,"HUM %d%%", parsedResponse->humidity);
        menu_write_item(_lcd_info, menuText, 2, 1);

        sprintf(menuText,"Code: %s", parsedResponse->country_code);
        menu_write_item(_lcd_info, menuText, 10, 1);

        sprintf(menuText,"City: %s", parsedResponse->city);
        menu_write_item(_lcd_info, menuText, 2, 2);

    }
   
    // Add back option
    menuText = "Back";
    menu_write_scroll_menu_item(_lcd_info, menuText, 3);

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 14, 3);
    i2c_lcd1602_write_string(_lcd_info, cursor);
}

/**
 * @brief  Entry method for when the settings menu is first entered.
 */
void menu_enter_display_settings(void)
{
    // Sets the scrollIndex to the previously selected city.
    scrollIndex = selectedCityIndex;
    menu_display_settings();
}

/**
 * @brief  Displays the API SETTINGS menu with the correct values.
 */
void menu_display_settings(void)
{
    // Clears screen
    i2c_lcd1602_clear(_lcd_info);

    // Display scroll menu title
    char *menuText = "API SETTINGS";
    menu_write_scroll_menu_item(_lcd_info, menuText, 0);

    // Get the city selection list
    citySelectionList = city_selection_list();
    int size = city_selection_list_size();
    citySelectionList[size] = "Back";

    // Set scrollindex if index is outside of list.
    if(scrollIndex + 1 > size + 1){
        scrollIndex = 0;
    } else if (scrollIndex - 1 < -1) {
        scrollIndex = size;
    }

    // If below 0 loop back to top
    int indexPreviousCity = scrollIndex - 1 < 0 ? size : scrollIndex - 1;
    menuText = citySelectionList[indexPreviousCity];
    ESP_LOGI(MENUTAG,"Display text previous %s", menuText);
    menu_write_scroll_menu_item(_lcd_info, menuText, 1);

    menuText = citySelectionList[scrollIndex];
    ESP_LOGI(MENUTAG,"Display text current %s", menuText);
    menu_write_scroll_menu_item(_lcd_info, menuText, 2);

    int indexNextCity = scrollIndex + 1 > size ? 0 : scrollIndex + 1;
    menuText = citySelectionList[indexNextCity];
    ESP_LOGI(MENUTAG,"Display text next %s", menuText);
    menu_write_scroll_menu_item(_lcd_info, menuText, 3);  
    
    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 17, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);
}