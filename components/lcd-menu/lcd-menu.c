#include "lcd-menu.h"

#include <string.h>
#include <stdio.h>
#include "esp_log.h"

#define MENUTAG "Menu"

// Default menu event functions
void enter_menu_item(void);
void exit_menu_item(void);

void goto_menu_item(int menuItem);

static i2c_lcd1602_info_t *_lcd_info;
static menu_t *_menu;

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

// Inits the lcd and returns struct with lcd info for lcd usage
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

    return lcd_info;
}

// Creates and returnes a pointer to the menu 
menu_t *menu_create_menu()
{
    //I^2C initialization + the I^2C port
    i2c_master_init();
    
    //LCD initialization
    _lcd_info = lcd_init();

    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_DATETIME_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1},     {"MAIN MENU", "Date/Time"},     {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_MAIN_ID_1, {MENU_NOTES_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2},         {"MAIN MENU", "Notes"},         {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_MAIN_ID_2, {MENU_PLACEHOLDER_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_0},   {"MAIN MENU", "Placeholder"},   {NULL, NULL, NULL}, enter_menu_item, NULL},

        //currentitem          //ok                 //left              //right                 //title       //menuitem
        {MENU_DATETIME_ID_0, {MENU_DATETIME_ID_1, MENU_DATETIME_ID_2, MENU_DATETIME_ID_1},  {"Date Time", "Date"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_DATETIME_ID_1, {MENU_DATETIME_ID_2, MENU_DATETIME_ID_0, MENU_DATETIME_ID_2},  {"Date Time", "Time"}, {NULL, NULL, NULL}, enter_menu_item, NULL},
        {MENU_DATETIME_ID_2, {MENU_DATETIME_ID_0, MENU_DATETIME_ID_1, MENU_DATETIME_ID_0},  {"Date Time", "Back"}, {NULL, NULL, NULL}, enter_menu_item, NULL},

        //todo
        //{MENU_NOTES_ID_0, {MENU_NOTES_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1},    {"Notes", "Date"},      {NULL, NULL, NULL}, NULL, NULL},
        //{MENU_NOTES_ID_1, {MENU_NOTES_ID_1, MENU_MAIN_ID_0, MENU_MAIN_ID_2},    {"Notes", "Time"},      {NULL, NULL, NULL}, NULL, NULL},
        //{MENU_NOTES_ID_2, {MENU_MAIN_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_0},     {"Notes", "MAIN MENU"}, {NULL, NULL, NULL}, NULL, NULL},
      
      };
    
    // If allocation is succesful, set all values
    if(menuPointer != NULL)
    {
        // Initialize menu with values
        menuPointer->lcd_info = _lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;

        _menu = menuPointer;

        ESP_LOGI(MENUTAG, "malloc menu_t %p", menuPointer);
    }
    else 
    {
        ESP_LOGI(MENUTAG, "malloc menu_t failed");
    }

    return menuPointer;
}

// Frees memory used by menu pointer
void menu_free_menu(menu_t *menu)
{
    free(menu->lcd_info);
    menu->lcd_info = NULL;
    free(menu->menuItems);
    menu->menuItems = NULL;
    free(menu);
    menu = NULL;
}

// Displays a welcome message on lcd
void menu_display_welcome_message(menu_t *menu)
{
    i2c_lcd1602_set_cursor(menu->lcd_info, false);
    i2c_lcd1602_move_cursor(menu->lcd_info, 6, 1);

    i2c_lcd1602_write_string(menu->lcd_info, "Welcome");
    i2c_lcd1602_move_cursor(menu->lcd_info, 8, 2);
    i2c_lcd1602_write_string(menu->lcd_info, "User");

    vTaskDelay(2500 / portTICK_RATE_MS);
    i2c_lcd1602_clear(menu->lcd_info);
}


// Displays menu all lines from menu item on lcd 
void menu_display_menu_item(menu_t *menu, int menuItemId)
{
    i2c_lcd1602_clear(menu->lcd_info);

    for (size_t line = 0; line < MAX_LCD_LINES; line++) {
        const char* menuText = menu->menuItems[menuItemId].menuText[line];
        int textPosition = 10 - ((strlen(menuText) + 1) / 2);
        i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, line);
        i2c_lcd1602_write_string(menu->lcd_info, menuText);
    }
}

// Writes an menu item on given line
void menu_write_scroll_menu_item(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    int textPosition = 10 - ((strlen(text) + 1) / 2);
    i2c_lcd1602_move_cursor(lcd_info, textPosition, line);
    i2c_lcd1602_write_string(lcd_info, text);
}

// Displays menu scroll menu on lcd
// by writing currentMenuItem and the item before and after
void menu_display_scroll_menu(menu_t *menu)
{
    i2c_lcd1602_clear(menu->lcd_info);
    
    // Gets title of scroll menu
    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    //ESP_LOGI(MENUTAG,"menuText line:0 %s", menuText);
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 0);

    // Get item before currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    //ESP_LOGI(MENUTAG,"menuText line:1 %s --- id num = %d", menuText, menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]);
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 1);

    // Get currentMenuItem
    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    //ESP_LOGI(MENUTAG,"menuText line:2 %s --- id num = %d", menuText, menu->currentMenuItemId);
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 2);

    // Get item after currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    //ESP_LOGI(MENUTAG,"menuText line:3 %s --- id num = %d", menuText, menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]);
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 3);

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(menu->lcd_info, 17, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);
}

// Handles key press by switching to new item or doing an onKeyEvent
void menu_handle_key_event(menu_t *menu, int key)
{
    // If key press leads to the same ID as the currentMenuItemId
    // do not switch to a new menu item, instead call the onKey event
    if(menu->menuItems[menu->currentMenuItemId].otherIds[key] == menu->currentMenuItemId){
        // Call the onKey event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key] != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key])();
        }else{
            ESP_LOGI(MENUTAG,"error, null ok");
        }
    } else {
        // Call the onMenuExit event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent)();
        }

        //ESP_LOGI(MENUTAG,"before = %d", menu->currentMenuItemId);
        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[key];
       // ESP_LOGI(MENUTAG,"after = %d", menu->currentMenuItemId);

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

// Goes to given menu item
//Not used atm 
void goto_menu_item(int menuItem){
    _menu->currentMenuItemId = menuItem;

    menu_display_scroll_menu(_menu);

    if(_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
        (*_menu->menuItems[_menu->currentMenuItemId].fpOnMenuEntryEvent)();
    }
}

// Default enter event
void enter_menu_item(void) {
    //ESP_LOGI(MENUTAG,"test");
}

// Default exit event
void exit_menu_item(void) {

}




